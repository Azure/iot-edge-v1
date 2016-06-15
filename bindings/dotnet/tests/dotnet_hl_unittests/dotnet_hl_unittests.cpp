// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/base64.h"
#include "dotnet.h"
#include "dotnet_hl.h"
#include "message.h"
#include "azure_c_shared_utility/constmap.h"
#include "azure_c_shared_utility/map.h"


static MICROMOCK_MUTEX_HANDLE g_testByTest;
static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

/*these are simple cached variables*/
static pfModule_Create  DotNET_HL_Create = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Destroy DotNET_HL_Destroy = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Receive DotNET_HL_Receive = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/

static size_t gMessageSize;
static const unsigned char * gMessageSource;

#define FAKE_CONFIG "" \
"{" \
"  \"modules\": [" \
"    {" \
"      \"module name\": \"csharp_hello_world\"," \
"      \"module path\": \"/csharp_hello_world.dll\"," \
"      \"args\": \"\"" \
"    }," \
"    {" \
"      \"dotnet_module_path\": \"SensorTag\"," \
"      \"dotnet_module_entry_class\": \"/dotnet_hl.so\"," \
"      \"dotnet_module_args\": {"\
"       \"module configuration"\
"      }" \
"    }" \
"  ]" \
"}"

#define GBALLOC_H

extern "C" int   gballoc_init(void);
extern "C" void  gballoc_deinit(void);
extern "C" void* gballoc_malloc(size_t size);
extern "C" void* gballoc_calloc(size_t nmemb, size_t size);
extern "C" void* gballoc_realloc(void* ptr, size_t size);
extern "C" void  gballoc_free(void* ptr);

namespace BASEIMPLEMENTATION
{
    /*if malloc is defined as gballoc_malloc at this moment, there'd be serious trouble*/
#define Lock(x) (LOCK_OK + gballocState - gballocState) /*compiler warning about constant in if condition*/
#define Unlock(x) (LOCK_OK + gballocState - gballocState)
#define Lock_Init() (LOCK_HANDLE)0x42
#define Lock_Deinit(x) (LOCK_OK + gballocState - gballocState)
#include "gballoc.c"
#undef Lock
#undef Unlock
#undef Lock_Init
#undef Lock_Deinit

#include <stddef.h>   /* size_t */    

    /* Types and enums */
    typedef struct json_object_t JSON_Object;
    typedef struct json_array_t  JSON_Array;
    typedef struct json_value_t  JSON_Value;

    enum json_value_type {
        JSONError = -1,
        JSONNull = 1,
        JSONString = 2,
        JSONNumber = 3,
        JSONObject = 4,
        JSONArray = 5,
        JSONBoolean = 6
    };
    typedef int JSON_Value_Type;

    enum json_result_t {
        JSONSuccess = 0,
        JSONFailure = -1
    };
    typedef int JSON_Status;

    typedef void * (*JSON_Malloc_Function)(size_t);
    typedef void(*JSON_Free_Function)(void *);

    /* Call only once, before calling any other function from parson API. If not called, malloc and free
    from stdlib will be used for all allocations */
    void json_set_allocation_functions(JSON_Malloc_Function malloc_fun, JSON_Free_Function free_fun);

    /* Parses first JSON value in a file, returns NULL in case of error */
    JSON_Value * json_parse_file(const char *filename);

    /* Parses first JSON value in a file and ignores comments (/ * * / and //),
    returns NULL in case of error */
    JSON_Value * json_parse_file_with_comments(const char *filename);

    /*  Parses first JSON value in a string, returns NULL in case of error */
    JSON_Value * json_parse_string(const char *string);

    /*  Parses first JSON value in a string and ignores comments (/ * * / and //),
    returns NULL in case of error */
    JSON_Value * json_parse_string_with_comments(const char *string);

    /* Serialization */
    size_t      json_serialization_size(const JSON_Value *value); /* returns 0 on fail */
    JSON_Status json_serialize_to_buffer(const JSON_Value *value, char *buf, size_t buf_size_in_bytes);
    JSON_Status json_serialize_to_file(const JSON_Value *value, const char *filename);
    char *      json_serialize_to_string(const JSON_Value *value);

    /* Pretty serialization */
    size_t      json_serialization_size_pretty(const JSON_Value *value); /* returns 0 on fail */
    JSON_Status json_serialize_to_buffer_pretty(const JSON_Value *value, char *buf, size_t buf_size_in_bytes);
    JSON_Status json_serialize_to_file_pretty(const JSON_Value *value, const char *filename);
    char *      json_serialize_to_string_pretty(const JSON_Value *value);

    void        json_free_serialized_string(char *string); /* frees string from json_serialize_to_string and json_serialize_to_string_pretty */

                                                           /* Comparing */
    int  json_value_equals(const JSON_Value *a, const JSON_Value *b);

    /* Validation
    This is *NOT* JSON Schema. It validates json by checking if object have identically
    named fields with matching types.
    For example schema {"name":"", "age":0} will validate
    {"name":"Joe", "age":25} and {"name":"Joe", "age":25, "gender":"m"},
    but not {"name":"Joe"} or {"name":"Joe", "age":"Cucumber"}.
    In case of arrays, only first value in schema is checked against all values in tested array.
    Empty objects ({}) validate all objects, empty arrays ([]) validate all arrays,
    null validates values of every type.
    */
    JSON_Status json_validate(const JSON_Value *schema, const JSON_Value *value);

    /*
    * JSON Object
    */
    JSON_Value  * json_object_get_value(const JSON_Object *object, const char *name);
    const char  * json_object_get_string(const JSON_Object *object, const char *name);
    JSON_Object * json_object_get_object(const JSON_Object *object, const char *name);
    JSON_Array  * json_object_get_array(const JSON_Object *object, const char *name);
    double        json_object_get_number(const JSON_Object *object, const char *name); /* returns 0 on fail */
    int           json_object_get_boolean(const JSON_Object *object, const char *name); /* returns -1 on fail */

                                                                                        /* dotget functions enable addressing values with dot notation in nested objects,
                                                                                        just like in structs or c++/java/c# objects (e.g. objectA.objectB.value).
                                                                                        Because valid names in JSON can contain dots, some values may be inaccessible
                                                                                        this way. */
    JSON_Value  * json_object_dotget_value(const JSON_Object *object, const char *name);
    const char  * json_object_dotget_string(const JSON_Object *object, const char *name);
    JSON_Object * json_object_dotget_object(const JSON_Object *object, const char *name);
    JSON_Array  * json_object_dotget_array(const JSON_Object *object, const char *name);
    double        json_object_dotget_number(const JSON_Object *object, const char *name); /* returns 0 on fail */
    int           json_object_dotget_boolean(const JSON_Object *object, const char *name); /* returns -1 on fail */

                                                                                           /* Functions to get available names */
    size_t        json_object_get_count(const JSON_Object *object);
    const char  * json_object_get_name(const JSON_Object *object, size_t index);

    /* Creates new name-value pair or frees and replaces old value with a new one.
    * json_object_set_value does not copy passed value so it shouldn't be freed afterwards. */
    JSON_Status json_object_set_value(JSON_Object *object, const char *name, JSON_Value *value);
    JSON_Status json_object_set_string(JSON_Object *object, const char *name, const char *string);
    JSON_Status json_object_set_number(JSON_Object *object, const char *name, double number);
    JSON_Status json_object_set_boolean(JSON_Object *object, const char *name, int boolean);
    JSON_Status json_object_set_null(JSON_Object *object, const char *name);

    /* Works like dotget functions, but creates whole hierarchy if necessary.
    * json_object_dotset_value does not copy passed value so it shouldn't be freed afterwards. */
    JSON_Status json_object_dotset_value(JSON_Object *object, const char *name, JSON_Value *value);
    JSON_Status json_object_dotset_string(JSON_Object *object, const char *name, const char *string);
    JSON_Status json_object_dotset_number(JSON_Object *object, const char *name, double number);
    JSON_Status json_object_dotset_boolean(JSON_Object *object, const char *name, int boolean);
    JSON_Status json_object_dotset_null(JSON_Object *object, const char *name);

    /* Frees and removes name-value pair */
    JSON_Status json_object_remove(JSON_Object *object, const char *name);

    /* Works like dotget function, but removes name-value pair only on exact match. */
    JSON_Status json_object_dotremove(JSON_Object *object, const char *key);

    /* Removes all name-value pairs in object */
    JSON_Status json_object_clear(JSON_Object *object);

    /*
    *JSON Array
    */
    JSON_Value  * json_array_get_value(const JSON_Array *array, size_t index);
    const char  * json_array_get_string(const JSON_Array *array, size_t index);
    JSON_Object * json_array_get_object(const JSON_Array *array, size_t index);
    JSON_Array  * json_array_get_array(const JSON_Array *array, size_t index);
    double        json_array_get_number(const JSON_Array *array, size_t index); /* returns 0 on fail */
    int           json_array_get_boolean(const JSON_Array *array, size_t index); /* returns -1 on fail */
    size_t        json_array_get_count(const JSON_Array *array);

    /* Frees and removes value at given index, does nothing and returns JSONFailure if index doesn't exist.
    * Order of values in array may change during execution.  */
    JSON_Status json_array_remove(JSON_Array *array, size_t i);

    /* Frees and removes from array value at given index and replaces it with given one.
    * Does nothing and returns JSONFailure if index doesn't exist.
    * json_array_replace_value does not copy passed value so it shouldn't be freed afterwards. */
    JSON_Status json_array_replace_value(JSON_Array *array, size_t i, JSON_Value *value);
    JSON_Status json_array_replace_string(JSON_Array *array, size_t i, const char* string);
    JSON_Status json_array_replace_number(JSON_Array *array, size_t i, double number);
    JSON_Status json_array_replace_boolean(JSON_Array *array, size_t i, int boolean);
    JSON_Status json_array_replace_null(JSON_Array *array, size_t i);

    /* Frees and removes all values from array */
    JSON_Status json_array_clear(JSON_Array *array);

    /* Appends new value at the end of array.
    * json_array_append_value does not copy passed value so it shouldn't be freed afterwards. */
    JSON_Status json_array_append_value(JSON_Array *array, JSON_Value *value);
    JSON_Status json_array_append_string(JSON_Array *array, const char *string);
    JSON_Status json_array_append_number(JSON_Array *array, double number);
    JSON_Status json_array_append_boolean(JSON_Array *array, int boolean);
    JSON_Status json_array_append_null(JSON_Array *array);

    /*
    *JSON Value
    */
    JSON_Value * json_value_init_object(void);
    JSON_Value * json_value_init_array(void);
    JSON_Value * json_value_init_string(const char *string); /* copies passed string */
    JSON_Value * json_value_init_number(double number);
    JSON_Value * json_value_init_boolean(int boolean);
    JSON_Value * json_value_init_null(void);
    JSON_Value * json_value_deep_copy(const JSON_Value *value);
    void         json_value_free(JSON_Value *value);

    JSON_Value_Type json_value_get_type(const JSON_Value *value);
    JSON_Object *   json_value_get_object(const JSON_Value *value);
    JSON_Array  *   json_value_get_array(const JSON_Value *value);
    const char  *   json_value_get_string(const JSON_Value *value);
    double          json_value_get_number(const JSON_Value *value);
    int             json_value_get_boolean(const JSON_Value *value);

    /* Same as above, but shorter */
    JSON_Value_Type json_type(const JSON_Value *value);
    JSON_Object *   json_object(const JSON_Value *value);
    JSON_Array  *   json_array(const JSON_Value *value);
    const char  *   json_string(const JSON_Value *value);
    double          json_number(const JSON_Value *value);
    int             json_boolean(const JSON_Value *value);

#define parson_parson_h
#ifdef _CRT_SECURE_NO_WARNINGS
    #undef _CRT_SECURE_NO_WARNINGS
    #include "parson.c"
    #define _CRT_SECURE_NO_WARNINGS
#else
    #include "parson.c"
#endif

#include "buffer.c"
#include "vector.c"
#include "strings.c"
};

#undef parson_parson_h
#include "parson.h"

/*forward declarations*/
MODULE_HANDLE DotNET_Create(MESSAGE_BUS_HANDLE busHandle, const void* configuration);
/*this destroys (frees resources) of the module parameter*/
void DotNET_Destroy(MODULE_HANDLE moduleHandle);
/*this is the module's callback function - gets called when a message is to be received by the module*/
void DotNET_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle);

static MODULE_APIS DOTNET_APIS =
{
    DotNET_Create,
    DotNET_Destroy,
    DotNET_Receive
};

TYPED_MOCK_CLASS(CDOTNETHLMocks, CGlobalMock)
{
public:
    // memory
    MOCK_STATIC_METHOD_1(, void*, gballoc_malloc, size_t, size)
        void* result2 = BASEIMPLEMENTATION::gballoc_malloc(size);
    MOCK_METHOD_END(void*, result2);

    MOCK_STATIC_METHOD_1(, void, gballoc_free, void*, ptr)
        BASEIMPLEMENTATION::gballoc_free(ptr);
    MOCK_VOID_METHOD_END()
    
    /*Parson Mocks*/
    MOCK_STATIC_METHOD_1(, JSON_Value*, json_parse_string, const char *, filename)
        JSON_Value* value = NULL;
        if (filename != NULL)
        {
            value = (JSON_Value*)malloc(sizeof(BASEIMPLEMENTATION::JSON_Value));
        }
    MOCK_METHOD_END(JSON_Value*, value);

    MOCK_STATIC_METHOD_1(, JSON_Value*, json_parse_file, const char *, filename)
        JSON_Value* value = NULL;
        if (filename != NULL)
        {
            value = (JSON_Value*)malloc(sizeof(BASEIMPLEMENTATION::JSON_Value));
        }
    MOCK_METHOD_END(JSON_Value*, value);

    MOCK_STATIC_METHOD_1(, JSON_Object*, json_value_get_object, const JSON_Value*, value)
        JSON_Object* object = NULL;
        if (value != NULL)
        {
            object = (JSON_Object*)0x42;
        }
    MOCK_METHOD_END(JSON_Object*, object);

    MOCK_STATIC_METHOD_2(, JSON_Array*, json_object_get_array, const JSON_Object*, object, const char*, name)
        JSON_Array* arr = NULL;
        if (object != NULL && name != NULL)
        {
            arr = (JSON_Array*)0x42;
        }
    MOCK_METHOD_END(JSON_Array*, arr);

    MOCK_STATIC_METHOD_1(, size_t, json_array_get_count, const JSON_Array*, arr)
        size_t size = 4;
    MOCK_METHOD_END(size_t, size);
    
    MOCK_STATIC_METHOD_2(, double, json_object_get_number, const JSON_Object *, object, const char *, name)
        double result2 = 0;
    MOCK_METHOD_END(double, result2);

    MOCK_STATIC_METHOD_2(, JSON_Object*, json_array_get_object, const JSON_Array*, arr, size_t, index)
        JSON_Object* object = NULL;
        if (arr != NULL && index >= 0)
        {
            object = (JSON_Object*)0x42;
        }
    MOCK_METHOD_END(JSON_Object*, object);

    MOCK_STATIC_METHOD_2(, const char*, json_object_get_string, const JSON_Object*, object, const char*, name)
        const char* result2;
		if (strcmp(name, "dotnet_module_path") == 0)
		{
			result2 = "/path/to/csharp_module.dll";
		}
		else if (strcmp(name, "dotnet_module_entry_class") == 0)
		{
			result2 = "mycsharpmodule.classname";
		}
		else if (strcmp(name, "dotnet_module_args") == 0)
		{
			result2 = "module configuration";
		}
		else 
		{
			result2 = NULL;
		}   
    MOCK_METHOD_END(const char*, result2);
    
    MOCK_STATIC_METHOD_2(, JSON_Value*, json_object_get_value, const JSON_Object*, object, const char*, name)
        JSON_Value* value = NULL;
        if (object != NULL && name != NULL)
        {
            value = (JSON_Value*)0x42;
        }
    MOCK_METHOD_END(JSON_Value*, value);

    MOCK_STATIC_METHOD_1(, void, json_value_free, JSON_Value*, value)
        free(value);
    MOCK_VOID_METHOD_END();
    
    MOCK_STATIC_METHOD_0(, const MODULE_APIS*, MODULE_STATIC_GETAPIS(DOTNET_HOST))
    MOCK_METHOD_END(const MODULE_APIS*, (const MODULE_APIS*)&DOTNET_APIS);

	MOCK_STATIC_METHOD_2(, MODULE_HANDLE, DotNET_Create, MESSAGE_BUS_HANDLE, busHandle, const void*, configuration)
		MODULE_HANDLE result2 = NULL;
		if (busHandle != NULL)
		{
			DOTNET_HOST_CONFIG *config = (DOTNET_HOST_CONFIG*)malloc(sizeof(DOTNET_HOST_CONFIG));
			memcpy(config, configuration, sizeof(DOTNET_HOST_CONFIG));
			result2 = (MODULE_HANDLE)config;
		}
    MOCK_METHOD_END(MODULE_HANDLE, result2);

    MOCK_STATIC_METHOD_1(, void, DotNET_Destroy, MODULE_HANDLE, moduleHandle);
    {
        if(moduleHandle != NULL)
        {
            DOTNET_HOST_CONFIG* config = (DOTNET_HOST_CONFIG*)moduleHandle;
            free(config);
        }
    }
    MOCK_VOID_METHOD_END()

        MOCK_STATIC_METHOD_2(, void, DotNET_Receive, MODULE_HANDLE, moduleHandle, MESSAGE_HANDLE, messageHandle)
        if (moduleHandle != NULL)
        {
            DOTNET_HOST_CONFIG* config = (DOTNET_HOST_CONFIG*)moduleHandle;
        }

    MOCK_VOID_METHOD_END()
};

DECLARE_GLOBAL_MOCK_METHOD_1(CDOTNETHLMocks, , JSON_Value*, json_parse_string, const char *, filename);
DECLARE_GLOBAL_MOCK_METHOD_1(CDOTNETHLMocks, , JSON_Object*, json_value_get_object, const JSON_Value*, value);
DECLARE_GLOBAL_MOCK_METHOD_2(CDOTNETHLMocks, , double, json_object_get_number, const JSON_Object*, value, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_2(CDOTNETHLMocks, , const char*, json_object_get_string, const JSON_Object*, object, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_2(CDOTNETHLMocks, , JSON_Array*, json_object_get_array, const JSON_Object*, object, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_1(CDOTNETHLMocks, , void, json_value_free, JSON_Value*, value);
DECLARE_GLOBAL_MOCK_METHOD_1(CDOTNETHLMocks, , size_t, json_array_get_count, const JSON_Array*, arr);
DECLARE_GLOBAL_MOCK_METHOD_2(CDOTNETHLMocks, , JSON_Object*, json_array_get_object, const JSON_Array*, arr, size_t, index);

DECLARE_GLOBAL_MOCK_METHOD_1(CDOTNETHLMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CDOTNETHLMocks, , void, gballoc_free, void*, ptr);

DECLARE_GLOBAL_MOCK_METHOD_2(CDOTNETHLMocks, , MODULE_HANDLE, DotNET_Create, MESSAGE_BUS_HANDLE, busHandle, const void*, configuration);
DECLARE_GLOBAL_MOCK_METHOD_1(CDOTNETHLMocks, , void, DotNET_Destroy, MODULE_HANDLE, moduleHandle);
DECLARE_GLOBAL_MOCK_METHOD_2(CDOTNETHLMocks, , void, DotNET_Receive, MODULE_HANDLE, moduleHandle, MESSAGE_HANDLE, messageHandle);
DECLARE_GLOBAL_MOCK_METHOD_0(CDOTNETHLMocks, , const MODULE_APIS*, MODULE_STATIC_GETAPIS(DOTNET_HOST));



BEGIN_TEST_SUITE(dotnet_hl_unittests)
    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
        g_testByTest = MicroMockCreateMutex();
        ASSERT_IS_NOT_NULL(g_testByTest);

		DotNET_HL_Create = Module_GetAPIS()->Module_Create;
		DotNET_HL_Destroy = Module_GetAPIS()->Module_Destroy;
		DotNET_HL_Receive = Module_GetAPIS()->Module_Receive;
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        MicroMockDestroyMutex(g_testByTest);
        TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
    }

    TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
    {
        if (!MicroMockAcquireMutex(g_testByTest))
        {
            ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
        }
    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
        if (!MicroMockReleaseMutex(g_testByTest))
        {
            ASSERT_FAIL("failure in test framework at ReleaseMutex");
        }
    }

    /* Tests_SRS_DOTNET_HL_04_001: [ If busHandle is NULL then DotNET_HL_Create shall fail and return NULL. ]  */
    TEST_FUNCTION(dotnet_hl_Create_returns_NULL_when_bus_is_NULL)
    {
        ///arrange
        CDOTNETHLMocks mocks;

        ///act
        auto result = DotNET_HL_Create(NULL, (const void*)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

	/* Tests_SRS_DOTNET_HL_04_002: [If configuration is NULL then DotNET_HL_Create shall fail and return NULL.] */
	TEST_FUNCTION(dotnet_hl_Create_returns_NULL_when_configuration_is_NULL)
	{
		///arrange
		CDOTNETHLMocks mocks;

		///act
		auto result = DotNET_HL_Create((MESSAGE_BUS_HANDLE)0x42, NULL);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_HL_04_003: [ If configuration is not a JSON object, then DotNET_HL_Create shall fail and return NULL. ] */
	TEST_FUNCTION(dotnet_hl_Create_Create_returns_NULL_when_json_parse_string_fails)
	{
		///arrange
		CDOTNETHLMocks mocks;

		STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.SetFailReturn((JSON_Value*)NULL);

		///act
		auto result = DotNET_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_HL_04_003: [ If configuration is not a JSON object, then DotNET_HL_Create shall fail and return NULL. ] */
	TEST_FUNCTION(dotnet_hl_Create_Create_returns_NULL_when_json_value_get_object_fails)
	{
		///arrange
		CDOTNETHLMocks mocks;

		STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.SetFailReturn((JSON_Object*)NULL);

		///act
		auto result = DotNET_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_HL_04_004: [ If the JSON object does not contain a value named "dotnet_module_path" then DotNET_HL_Create shall fail and return NULL. ] */
	TEST_FUNCTION(dotnet_hl_Create_Create_returns_NULL_when_configuration_string_do_not_contain_dotnet_module_path)
	{
		///arrange
		CDOTNETHLMocks mocks;

		STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "dotnet_module_path"))
			.IgnoreArgument(1)
			.SetFailReturn((const char*)NULL);

		///act
		auto result = DotNET_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_HL_04_005: [ If the JSON object does not contain a value named "dotnet_module_entry_class" then DotNET_HL_Create shall fail and return NULL. ] */
	TEST_FUNCTION(dotnet_hl_Create_Create_returns_NULL_when_configuration_string_do_not_contain_dotnet_module_entry_class)
	{
		///arrange
		CDOTNETHLMocks mocks;

		STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "dotnet_module_path"))
			.IgnoreArgument(1);
		
		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "dotnet_module_entry_class"))
		.IgnoreArgument(1)
		.SetFailReturn((const char*)NULL);

		///act
		auto result = DotNET_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_HL_04_006: [ If the JSON object does not contain a value named "dotnet_module_args" then DotNET_HL_Create shall fail and return NULL. ]*/
	TEST_FUNCTION(dotnet_hl_Create_Create_returns_NULL_when_configuration_string_do_not_contain_dotnet_module_args)
	{
		///arrange
		CDOTNETHLMocks mocks;

		STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "dotnet_module_path"))
			.IgnoreArgument(1);
		
		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "dotnet_module_entry_class"))
		.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "dotnet_module_args"))
			.IgnoreArgument(1)
			.SetFailReturn((const char*)NULL);

		///act
		auto result = DotNET_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_HL_04_007: [ DotNET_HL_Create shall pass busHandle and const void* configuration ( with DOTNET_HOST_CONFIG) to DotNET_Create. ]*/
	/* Tests_SRS_DOTNET_HL_04_009: [ If DotNET_Create fails then DotNET_HL_Create shall fail and return NULL. ] */
	TEST_FUNCTION(dotnet_hl_Create_Create_returns_NULL_Module_Create_fails)
	{
		///arrange
		CDOTNETHLMocks mocks;

		STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "dotnet_module_path"))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "dotnet_module_entry_class"))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "dotnet_module_args"))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, MODULE_STATIC_GETAPIS(DOTNET_HOST)());

		STRICT_EXPECTED_CALL(mocks, DotNET_Create((MESSAGE_BUS_HANDLE)0x42, IGNORED_PTR_ARG))
			.IgnoreArgument(2)
			.SetFailReturn((MODULE_HANDLE*)NULL);

		///act
		auto result = DotNET_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}
		
	/* Tests_SRS_DOTNET_HL_04_007: [ DotNET_HL_Create shall pass busHandle and const void* configuration ( with DOTNET_HOST_CONFIG) to DotNET_Create. ]*/
	/* Tests_SRS_DOTNET_HL_04_008: [ If DotNET_Create succeeds then DotNET_HL_Create shall succeed and return a non-NULL value. ] */
	TEST_FUNCTION(DOTNET_HL_Create_succeeds)
	{
		///arrange
		CDOTNETHLMocks mocks;

		STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "dotnet_module_path"))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "dotnet_module_entry_class"))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "dotnet_module_args"))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, MODULE_STATIC_GETAPIS(DOTNET_HOST)());

		STRICT_EXPECTED_CALL(mocks, DotNET_Create((MESSAGE_BUS_HANDLE)0x42, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		///act
		auto result = DotNET_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NOT_NULL(result);

		///cleanup
		DotNET_HL_Destroy(result);
	}

	/* Tests_SRS_DOTNET_HL_04_010: [ DotNET_HL_Receive shall pass the received parameters to the underlying DotNET Host Module's _Receive function. ] */
	TEST_FUNCTION(DOTNET_HL_Receive_forwards_call)
	{
		///arrange
		CDOTNETHLMocks mocks;
		unsigned char fake = '\0';
		CONSTBUFFER messageBuffer;
		messageBuffer.buffer = &fake;
		messageBuffer.size = 1;

		auto module = DotNET_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);
		mocks.ResetAllCalls();

		MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;

		STRICT_EXPECTED_CALL(mocks, MODULE_STATIC_GETAPIS(DOTNET_HOST)());

		STRICT_EXPECTED_CALL(mocks, DotNET_Receive((MODULE_HANDLE)module, (MESSAGE_HANDLE)0x42));

		///act
		DotNET_HL_Receive(module, (MESSAGE_HANDLE)0x42);

		///assert
		mocks.AssertActualAndExpectedCalls();

		///cleanup
		DotNET_HL_Destroy(module);
	}

	/* Tests_SRS_DOTNET_HL_04_013: [ DotNET_HL_Destroy shall do nothing if module is NULL. ] */
	TEST_FUNCTION(DOTNET_HL_Destroy_does_nothing_with_null_input)
	{
		///arrange
		CDOTNETHLMocks mocks;

		///act
		DotNET_HL_Destroy(NULL);

		///assert
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}

	/* Tests_SRS_DOTNET_HL_04_011: [ DotNET_HL_Destroy shall destroy all used resources for the associated module. ] */
	TEST_FUNCTION(DotNET_HL_Destroy_frees_resources)
	{
		///arrange
		CDOTNETHLMocks mocks;

		auto module = DotNET_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);
		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, MODULE_STATIC_GETAPIS(DOTNET_HOST)());

		STRICT_EXPECTED_CALL(mocks, DotNET_Destroy(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

		///act
		DotNET_HL_Destroy(module);

		///assert
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}

	/* Tests_SRS_DOTNET_HL_04_012: [ Module_GetAPIS shall return a non-NULL pointer to a structure of type MODULE_APIS that has all fields non-NULL. ] */
	TEST_FUNCTION(Module_GetAPIS_returns_non_NULL)
	{
		///arrage
		CDOTNETHLMocks mocks;

		///act
		const MODULE_APIS* apis = Module_GetAPIS();

		///assert
		ASSERT_IS_NOT_NULL(apis);
		ASSERT_IS_NOT_NULL(apis->Module_Destroy);
		ASSERT_IS_NOT_NULL(apis->Module_Create);
		ASSERT_IS_NOT_NULL(apis->Module_Receive);

		///cleanup
	}

END_TEST_SUITE(dotnet_hl_unittests)