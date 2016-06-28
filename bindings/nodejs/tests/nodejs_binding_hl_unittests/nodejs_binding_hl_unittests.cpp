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

#include "nodejs.h"
#include "nodejs_hl.h"

#define GBALLOC_H
extern "C" int gballoc_init(void);
extern "C" void gballoc_deinit(void);
extern "C" void* gballoc_malloc(size_t size);
extern "C" void* gballoc_calloc(size_t nmemb, size_t size);
extern "C" void* gballoc_realloc(void* ptr, size_t size);
extern "C" void gballoc_free(void* ptr);

namespace BASEIMPLEMENTATION
{

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
};

#undef parson_parson_h
#include "parson.h"

/*forward declarations*/
MODULE_HANDLE NODEJS_Create(MESSAGE_BUS_HANDLE busHandle, const void* configuration);
/*this destroys (frees resources) of the module parameter*/
void NODEJS_Destroy(MODULE_HANDLE moduleHandle);
/*this is the module's callback function - gets called when a message is to be received by the module*/
void NODEJS_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle);

static MODULE_APIS NODEJS_APIS =
{
    NODEJS_Create,
    NODEJS_Destroy,
    NODEJS_Receive
};

TYPED_MOCK_CLASS(CNODEJSHLMocks, CGlobalMock)
{
public:

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
        size_t size = 0;
    MOCK_METHOD_END(size_t, size);

    MOCK_STATIC_METHOD_2(, JSON_Object*, json_array_get_object, const JSON_Array*, arr, size_t, index)
        JSON_Object* object = NULL;
        if (arr != NULL && index >= 0)
        {
            object = (JSON_Object*)0x42;
        }
    MOCK_METHOD_END(JSON_Object*, object);

    MOCK_STATIC_METHOD_2(, const char*, json_object_get_string, const JSON_Object*, object, const char*, name)
    MOCK_METHOD_END(const char*, (strcmp(name, "main_path")==0)?"/path/to/main.js":NULL);

    MOCK_STATIC_METHOD_2(, JSON_Value*, json_object_get_value, const JSON_Object*, object, const char*, name)
        JSON_Value* value = NULL;
        if (object != NULL && name != NULL)
        {
            value = (JSON_Value*)0x42;
        }
    MOCK_METHOD_END(JSON_Value*, value);

    MOCK_STATIC_METHOD_1(, char*, json_serialize_to_string, const JSON_Value*, value)
        char* serialized_string = NULL;
        const char* text = "[serialized string]";
        if (value != NULL)
        {
            serialized_string = (char*)malloc(sizeof(char) * strlen(text) + 1);
            strcpy(serialized_string, text);
        }
    MOCK_METHOD_END(char*, serialized_string);

    MOCK_STATIC_METHOD_1(, void, json_value_free, JSON_Value*, value)
        free(value);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_1(, void, json_free_serialized_string, char*, string)
        free(string);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_1(, void*, gballoc_malloc, size_t, size)
        void* result2 = BASEIMPLEMENTATION::gballoc_malloc(size);
    MOCK_METHOD_END(void*, result2);

    MOCK_STATIC_METHOD_1(, void, gballoc_free, void*, ptr)
        BASEIMPLEMENTATION::gballoc_free(ptr);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_0(, const MODULE_APIS*, MODULE_STATIC_GETAPIS(NODEJS_MODULE))
    MOCK_METHOD_END(const MODULE_APIS*, (const MODULE_APIS*)&NODEJS_APIS);

    MOCK_STATIC_METHOD_2( , MODULE_HANDLE, NODEJS_Create, MESSAGE_BUS_HANDLE, busHandle, const void*, configuration)
    MOCK_METHOD_END(MODULE_HANDLE, malloc(1));

    MOCK_STATIC_METHOD_1(, void, NODEJS_Destroy, MODULE_HANDLE, moduleHandle);
    {
        free(moduleHandle);
    }
    MOCK_VOID_METHOD_END()
    
    MOCK_STATIC_METHOD_2(, void, NODEJS_Receive, MODULE_HANDLE, moduleHandle, MESSAGE_HANDLE, messageHandle)
    MOCK_VOID_METHOD_END()
};

DECLARE_GLOBAL_MOCK_METHOD_1(CNODEJSHLMocks, , JSON_Value*, json_parse_file, const char *, filename);
DECLARE_GLOBAL_MOCK_METHOD_1(CNODEJSHLMocks, , JSON_Value*, json_parse_string, const char *, filename);
DECLARE_GLOBAL_MOCK_METHOD_1(CNODEJSHLMocks, , JSON_Object*, json_value_get_object, const JSON_Value*, value);
DECLARE_GLOBAL_MOCK_METHOD_2(CNODEJSHLMocks, , JSON_Array*, json_object_get_array, const JSON_Object*, object, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_1(CNODEJSHLMocks, , size_t, json_array_get_count, const JSON_Array*, arr);
DECLARE_GLOBAL_MOCK_METHOD_2(CNODEJSHLMocks, , JSON_Object*, json_array_get_object, const JSON_Array*, arr, size_t, index);
DECLARE_GLOBAL_MOCK_METHOD_2(CNODEJSHLMocks, , const char*, json_object_get_string, const JSON_Object*, object, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_2(CNODEJSHLMocks, , JSON_Value*, json_object_get_value, const JSON_Object*, object, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_1(CNODEJSHLMocks, , char*, json_serialize_to_string, const JSON_Value*, value);
DECLARE_GLOBAL_MOCK_METHOD_1(CNODEJSHLMocks, , void, json_value_free, JSON_Value*, value);
DECLARE_GLOBAL_MOCK_METHOD_1(CNODEJSHLMocks, , void, json_free_serialized_string, char*, string);

DECLARE_GLOBAL_MOCK_METHOD_1(CNODEJSHLMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CNODEJSHLMocks, , void, gballoc_free, void*, ptr);

DECLARE_GLOBAL_MOCK_METHOD_0(CNODEJSHLMocks, , const MODULE_APIS*, MODULE_STATIC_GETAPIS(NODEJS_MODULE));

DECLARE_GLOBAL_MOCK_METHOD_2(CNODEJSHLMocks, , MODULE_HANDLE, NODEJS_Create, MESSAGE_BUS_HANDLE, busHandle, const void*, configuration);
DECLARE_GLOBAL_MOCK_METHOD_1(CNODEJSHLMocks, , void, NODEJS_Destroy, MODULE_HANDLE, moduleHandle);
DECLARE_GLOBAL_MOCK_METHOD_2(CNODEJSHLMocks, , void, NODEJS_Receive, MODULE_HANDLE, moduleHandle, MESSAGE_HANDLE, messageHandle);

MODULE_HANDLE (*NODEJS_HL_Create)(MESSAGE_BUS_HANDLE busHandle, const void* configuration);
/*this destroys (frees resources) of the module parameter*/
void (*NODEJS_HL_Destroy)(MODULE_HANDLE moduleHandle);
/*this is the module's callback function - gets called when a message is to be received by the module*/
void (*NODEJS_HL_Receive)(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle);

static MESSAGE_BUS_HANDLE validBusHandle = (MESSAGE_BUS_HANDLE)0x1;
static MESSAGE_HANDLE VALID_MESSAGE_HANDLE  = (MESSAGE_HANDLE)0x02;

#define VALID_CONFIG_STRING ""                \
    "\"args\": {"                             \
    "   \"main_path\": \"/path/to/main.js\"," \
    "   \"args\": \"module configuration\""   \
    "}"

static MICROMOCK_MUTEX_HANDLE g_testByTest;
static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;
BEGIN_TEST_SUITE(nodejs_binding_hl_unittests)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
        g_testByTest = MicroMockCreateMutex();
        ASSERT_IS_NOT_NULL(g_testByTest);

        NODEJS_HL_Create = Module_GetAPIS()->Module_Create;
        NODEJS_HL_Destroy = Module_GetAPIS()->Module_Destroy;
        NODEJS_HL_Receive = Module_GetAPIS()->Module_Receive;

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

    /*Tests_SRS_NODEJS_HL_13_001: [ NODEJS_HL_Create shall return NULL if bus is NULL. ]*/
    TEST_FUNCTION(NODEJS_HL_Create_with_NULL_busHandle_fails)
    {
        ///arrage
        CNODEJSHLMocks mocks;
         
        ///act
        auto result = NODEJS_HL_Create(NULL, "someConfig");

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_NODEJS_HL_13_002: [ NODEJS_HL_Create shall return NULL if configuration is NULL. ]*/
    TEST_FUNCTION(NODEJS_HL_Create_with_NULL_configuration_fails)
    {
        ///arrage
        CNODEJSHLMocks mocks;

        ///act
        auto result = NODEJS_HL_Create(validBusHandle, NULL);

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_NODEJS_HL_13_012: [ NODEJS_HL_Create shall parse configuration as a JSON string. ]*/
    /*Tests_SRS_NODEJS_HL_13_013: [ NODEJS_HL_Create shall extract the value of the main_path property from the configuration JSON. ]*/
    /*Tests_SRS_NODEJS_HL_13_006: [ NODEJS_HL_Create shall extract the value of the args property from the configuration JSON. ]*/
    /*Tests_SRS_NODEJS_HL_13_005: [ NODEJS_HL_Create shall populate a NODEJS_MODULE_CONFIG object with the values of the main_path and args properties and invoke NODEJS_Create passing the bus handle the config object. ]*/
    /*Tests_SRS_NODEJS_HL_13_007: [ If NODEJS_Create succeeds then a valid MODULE_HANDLE shall be returned. ]*/
    TEST_FUNCTION(NODEJS_HL_Create_happy_path_succeeds)
    {
        ///arrage
        CNODEJSHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, json_parse_string(VALID_CONFIG_STRING));
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "main_path"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_value(IGNORED_PTR_ARG, "args"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_serialize_to_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, C1(MODULE_STATIC_GETAPIS(NODEJS_MODULE)()));
        STRICT_EXPECTED_CALL(mocks, NODEJS_Create(validBusHandle, IGNORED_PTR_ARG))
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        auto result = NODEJS_HL_Create(validBusHandle, VALID_CONFIG_STRING);

        ///assert
        ASSERT_IS_NOT_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        NODEJS_HL_Destroy(result);
    }

    /*Tests_SRS_NODEJS_HL_13_008: [ If NODEJS_Create fail then the value NULL shall be returned. ]*/
    TEST_FUNCTION(NODEJS_HL_Create_fails_when_NODEJS_Create_fails)
    {
        ///arrage
        CNODEJSHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, json_parse_string(VALID_CONFIG_STRING));
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "main_path"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_value(IGNORED_PTR_ARG, "args"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_serialize_to_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, C1(MODULE_STATIC_GETAPIS(NODEJS_MODULE)()));
        STRICT_EXPECTED_CALL(mocks, NODEJS_Create(validBusHandle, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .SetFailReturn((MODULE_HANDLE)NULL);

        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        auto result = NODEJS_HL_Create(validBusHandle, VALID_CONFIG_STRING);

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_NODEJS_HL_13_004: [ NODEJS_HL_Create shall return NULL if the configuration JSON does not contain a string property called main_path. ]*/
    TEST_FUNCTION(NODEJS_HL_Create_fails_when_json_object_get_string_fails)
    {
        ///arrage
        CNODEJSHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, json_parse_string(VALID_CONFIG_STRING));
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "main_path"))
            .IgnoreArgument(1)
            .SetFailReturn((const char *)NULL);

        ///act
        auto result = NODEJS_HL_Create(validBusHandle, VALID_CONFIG_STRING);

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_NODEJS_HL_13_014: [ NODEJS_HL_Create shall return NULL if the configuration JSON does not start with an object at the root. ]*/
    TEST_FUNCTION(NODEJS_HL_Create_fails_when_json_value_get_object_fails)
    {
        ///arrage
        CNODEJSHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, json_parse_string(VALID_CONFIG_STRING));
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((JSON_Object*)NULL);

        ///act
        auto result = NODEJS_HL_Create(validBusHandle, VALID_CONFIG_STRING);

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_NODEJS_HL_13_003: [ NODEJS_HL_Create shall return NULL if configuration is not a valid JSON string. ]*/
    TEST_FUNCTION(NODEJS_HL_Create_fails_when_json_parse_string_fails)
    {
        ///arrage
        CNODEJSHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, json_parse_string(VALID_CONFIG_STRING))
            .SetFailReturn((JSON_Value*)NULL);

        ///act
        auto result = NODEJS_HL_Create(validBusHandle, VALID_CONFIG_STRING);

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_NODEJS_HL_13_009: [ NODEJS_HL_Receive shall pass the received parameters to the underlying module's _Receive function. ]*/
    TEST_FUNCTION(NODEJS_HL_Receive_passthrough_succeeds)
    {
        ///arrage
        CNODEJSHLMocks mocks;
        MODULE_HANDLE handle = NODEJS_HL_Create(validBusHandle, VALID_CONFIG_STRING);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, MODULE_STATIC_GETAPIS(NODEJS_MODULE)());
        STRICT_EXPECTED_CALL(mocks, NODEJS_Receive(handle, VALID_MESSAGE_HANDLE));

        ///act
        NODEJS_HL_Receive(handle, VALID_MESSAGE_HANDLE);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        NODEJS_HL_Destroy(handle);
    }

    /*Tests_SRS_NODEJS_HL_13_010: [ NODEJS_HL_Destroy shall destroy all used resources. ]*/
    TEST_FUNCTION(NODEJS_HL_Destroy_passthrough_succeeds)
    {
        ///arrage
        CNODEJSHLMocks mocks;
        MODULE_HANDLE handle = NODEJS_HL_Create(validBusHandle, VALID_CONFIG_STRING);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, MODULE_STATIC_GETAPIS(NODEJS_MODULE)());
        STRICT_EXPECTED_CALL(mocks, NODEJS_Destroy(handle));

        ///act
        NODEJS_HL_Destroy(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_NODEJS_HL_13_011: [ Module_GetAPIS shall return a non-NULL pointer to a structure of type MODULE_APIS that has all fields non-NULL. ]*/
    TEST_FUNCTION(Module_GetAPIS_returns_non_NULL)
    {
        ///arrage
        CNODEJSHLMocks mocks;
        
        ///act
        const MODULE_APIS* apis = Module_GetAPIS();

        ///assert
        ASSERT_IS_NOT_NULL(apis);
        ASSERT_IS_NOT_NULL(apis->Module_Destroy);
        ASSERT_IS_NOT_NULL(apis->Module_Create);
        ASSERT_IS_NOT_NULL(apis->Module_Receive);

        ///cleanup
    }

END_TEST_SUITE(nodejs_binding_hl_unittests)
