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

#include "logger.h"
#include "logger_hl.h"

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
MODULE_HANDLE Logger_Create(MESSAGE_BUS_HANDLE busHandle, const void* configuration);
/*this destroys (frees resources) of the module parameter*/
void Logger_Destroy(MODULE_HANDLE moduleHandle);
/*this is the module's callback function - gets called when a message is to be received by the module*/
void Logger_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle);

static MODULE_APIS Logger_APIS =
{
    Logger_Create,
    Logger_Destroy,
    Logger_Receive
};

TYPED_MOCK_CLASS(CLoggerMocks, CGlobalMock)
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
    MOCK_METHOD_END(const char*, (strcmp(name, "filename")==0)?"log.txt":NULL);

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

    MOCK_STATIC_METHOD_1(, STRING_HANDLE, STRING_construct, const char*, source)
        STRING_HANDLE result2 = (STRING_HANDLE)malloc(4);
    MOCK_METHOD_END(STRING_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, void, STRING_delete, STRING_HANDLE, s)
        free(s);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_2(, int, STRING_concat, STRING_HANDLE, s1, const char*, s2)
    MOCK_METHOD_END(int, 0);

    MOCK_STATIC_METHOD_2(, int, STRING_concat_with_STRING, STRING_HANDLE, s1, STRING_HANDLE, s2)
    MOCK_METHOD_END(int, 0);

    MOCK_STATIC_METHOD_1(, const char*, STRING_c_str, STRING_HANDLE, s)
    MOCK_METHOD_END(const char*, "thisIsRandomContent")

    MOCK_STATIC_METHOD_1(, MAP_HANDLE, ConstMap_CloneWriteable, CONSTMAP_HANDLE, handle)
        MAP_HANDLE result2 = (MAP_HANDLE)malloc(5);
    MOCK_METHOD_END(MAP_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, void, Map_Destroy, MAP_HANDLE, ptr)
        free(ptr);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, STRING_HANDLE, Map_ToJSON, MAP_HANDLE, handle)
        STRING_HANDLE result2 = (STRING_HANDLE)malloc(6);
    MOCK_METHOD_END(STRING_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, CONSTMAP_HANDLE, Message_GetProperties, MESSAGE_HANDLE, message)
        CONSTMAP_HANDLE result2 = (CONSTMAP_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1);
    MOCK_METHOD_END(CONSTMAP_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, void, ConstMap_Destroy, CONSTMAP_HANDLE, handle)
        free(handle);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_2(, STRING_HANDLE, Base64_Encode_Bytes, const unsigned char*, source, size_t, size);
        STRING_HANDLE result2 = (STRING_HANDLE)malloc(7);
    MOCK_METHOD_END(STRING_HANDLE, result2);

    MOCK_STATIC_METHOD_2(, FILE*, gb_fopen, const char*, filename, const char*, mode)
        FILE* result2 = (FILE*)malloc(8);
    MOCK_METHOD_END(FILE*, result2);

    MOCK_STATIC_METHOD_1(, int, gb_fclose, FILE*, file)
        free(file);
    int result2 = 0;
    MOCK_METHOD_END(int, result2);

    MOCK_STATIC_METHOD_3(, int, gb_fseek, FILE *, stream, long int, offset, int, whence)
        int result2 = 0;
    MOCK_METHOD_END(int, result2);

    MOCK_STATIC_METHOD_1(, long int, gb_ftell, FILE *, stream)
        long int result2 = 0;
    MOCK_METHOD_END(long int, result2);

    MOCK_STATIC_METHOD_1(, time_t, gb_time, time_t *, timer)
        time_t result2 = (time_t)1; /*assume "1" is valid time_t*/
    MOCK_METHOD_END(time_t, result2);

    MOCK_STATIC_METHOD_1(, struct tm*, gb_localtime, const time_t *, timer)
    struct tm* result2 = (struct tm*)0x42;
    MOCK_METHOD_END(struct tm*, result2);

    MOCK_STATIC_METHOD_0(, const MODULE_APIS*, MODULE_STATIC_GETAPIS(LOGGER_MODULE))
    MOCK_METHOD_END(const MODULE_APIS*, (const MODULE_APIS*)&Logger_APIS);

    MOCK_STATIC_METHOD_2( , MODULE_HANDLE, Logger_Create, MESSAGE_BUS_HANDLE, busHandle, const void*, configuration)
    MOCK_METHOD_END(MODULE_HANDLE, malloc(1));

    MOCK_STATIC_METHOD_1(, void, Logger_Destroy, MODULE_HANDLE, moduleHandle);
    {
        free(moduleHandle);
    }
    MOCK_VOID_METHOD_END()
    
    MOCK_STATIC_METHOD_2(, void, Logger_Receive, MODULE_HANDLE, moduleHandle, MESSAGE_HANDLE, messageHandle)
    MOCK_VOID_METHOD_END()
};

DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , JSON_Value*, json_parse_file, const char *, filename);
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , JSON_Value*, json_parse_string, const char *, filename);
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , JSON_Object*, json_value_get_object, const JSON_Value*, value);
DECLARE_GLOBAL_MOCK_METHOD_2(CLoggerMocks, , JSON_Array*, json_object_get_array, const JSON_Object*, object, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , size_t, json_array_get_count, const JSON_Array*, arr);
DECLARE_GLOBAL_MOCK_METHOD_2(CLoggerMocks, , JSON_Object*, json_array_get_object, const JSON_Array*, arr, size_t, index);
DECLARE_GLOBAL_MOCK_METHOD_2(CLoggerMocks, , const char*, json_object_get_string, const JSON_Object*, object, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_2(CLoggerMocks, , JSON_Value*, json_object_get_value, const JSON_Object*, object, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , char*, json_serialize_to_string, const JSON_Value*, value);
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , void, json_value_free, JSON_Value*, value);
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , void, json_free_serialized_string, char*, string);

DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , void, gballoc_free, void*, ptr);

DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , STRING_HANDLE, STRING_construct, const char*, s);

DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , void, STRING_delete, STRING_HANDLE, s);
DECLARE_GLOBAL_MOCK_METHOD_2(CLoggerMocks, , int, STRING_concat, STRING_HANDLE, s1, const char*, s2);
DECLARE_GLOBAL_MOCK_METHOD_2(CLoggerMocks, , int, STRING_concat_with_STRING, STRING_HANDLE, s1, STRING_HANDLE, s2);
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , const char*, STRING_c_str, STRING_HANDLE, s);

DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , MAP_HANDLE, ConstMap_CloneWriteable, CONSTMAP_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , STRING_HANDLE, Map_ToJSON, MAP_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , void, Map_Destroy, MAP_HANDLE, ptr);

DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , CONSTMAP_HANDLE, Message_GetProperties, MESSAGE_HANDLE, message);
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , void, ConstMap_Destroy, CONSTMAP_HANDLE, handle);

DECLARE_GLOBAL_MOCK_METHOD_2(CLoggerMocks, , STRING_HANDLE, Base64_Encode_Bytes, const unsigned char*, source, size_t, size);

DECLARE_GLOBAL_MOCK_METHOD_2(CLoggerMocks, , FILE*, gb_fopen, const char*, filename, const char*, mode);
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , int, gb_fclose, FILE*, stream);
DECLARE_GLOBAL_MOCK_METHOD_3(CLoggerMocks, , int, gb_fseek, FILE *, stream, long int, offset, int, whence)
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , long int, gb_ftell, FILE*, stream)

DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , time_t, gb_time, time_t*, timer);
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , struct tm*, gb_localtime, const time_t*, timer);

DECLARE_GLOBAL_MOCK_METHOD_0(CLoggerMocks, , const MODULE_APIS*, MODULE_STATIC_GETAPIS(LOGGER_MODULE));

DECLARE_GLOBAL_MOCK_METHOD_2(CLoggerMocks, , MODULE_HANDLE, Logger_Create, MESSAGE_BUS_HANDLE, busHandle, const void*, configuration);
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , void, Logger_Destroy, MODULE_HANDLE, moduleHandle);
DECLARE_GLOBAL_MOCK_METHOD_2(CLoggerMocks, , void, Logger_Receive, MODULE_HANDLE, moduleHandle, MESSAGE_HANDLE, messageHandle);

/*definitions of cached functions, initialized in TEST_FIUCNTION_INIT*/

MODULE_HANDLE (*Logger_HL_Create)(MESSAGE_BUS_HANDLE busHandle, const void* configuration);
/*this destroys (frees resources) of the module parameter*/
void (*Logger_HL_Destroy)(MODULE_HANDLE moduleHandle);
/*this is the module's callback function - gets called when a message is to be received by the module*/
void (*Logger_HL_Receive)(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle);

static MESSAGE_BUS_HANDLE validBusHandle = (MESSAGE_BUS_HANDLE)0x1;
static MESSAGE_HANDLE VALID_MESSAGE_HANDLE  = (MESSAGE_HANDLE)0x02;
#define VALID_CONFIG_STRING "{\"filename\":\"log.txt\"}"

static MICROMOCK_MUTEX_HANDLE g_testByTest;
static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;
BEGIN_TEST_SUITE(logger_hl_unittests)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
        g_testByTest = MicroMockCreateMutex();
        ASSERT_IS_NOT_NULL(g_testByTest);

        Logger_HL_Create = Module_GetAPIS()->Module_Create;
        Logger_HL_Destroy = Module_GetAPIS()->Module_Destroy;
        Logger_HL_Receive = Module_GetAPIS()->Module_Receive;

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

    /*Tests_SRS_LOGGER_HL_02_001: [ If busHandle is NULL then Logger_HL_Create shall fail and return NULL. ]*/
    TEST_FUNCTION(Logger_HL_Create_with_NULL_busHandle_fails)
    {
        ///arrage
        CLoggerMocks mocks;
         
        ///act
        auto result = Logger_HL_Create(NULL, "someConfig");

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_LOGGER_HL_02_003: [ If configuration is NULL then Logger_HL_Create shall fail and return NULL. ]*/
    TEST_FUNCTION(Logger_HL_Create_with_NULL_configuration_fails)
    {
        ///arrage
        CLoggerMocks mocks;

        ///act
        auto result = Logger_HL_Create(validBusHandle, NULL);

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_LOGGER_HL_02_005: [ Logger_HL_Create shall pass busHandle and the filename to Logger_Create. ]*/
    /*Tests_SRS_LOGGER_HL_02_006: [ If Logger_Create succeeds then Logger_HL_Create shall succeed and return a non-NULL value. ]*/
    TEST_FUNCTION(Logger_HL_Create_happy_path_succeeds)
    {
        ///arrage
        CLoggerMocks mocks;

        STRICT_EXPECTED_CALL(mocks, json_parse_string(VALID_CONFIG_STRING)); /*this is creating the JSON from the string*/
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG)) /*this is destroy of the json value created from the string*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG)) /*getting the json object out of the json value*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "filename")) /*this is getting a json string that is what follows "filename": in the json*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, C1(MODULE_STATIC_GETAPIS(LOGGER_MODULE)())); /*this is finding the Logger's API*/
        STRICT_EXPECTED_CALL(mocks, Logger_Create(validBusHandle, IGNORED_PTR_ARG)) /*this is calling Logger_Create function*/
            .IgnoreArgument(2);

        ///act
        auto result = Logger_HL_Create(validBusHandle, VALID_CONFIG_STRING);

        ///assert
        ASSERT_IS_NOT_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Logger_HL_Destroy(result);
    }

    /*Tests_SRS_LOGGER_HL_02_007: [ If Logger_Create fails then Logger_HL_Create shall fail and return NULL. ]*/
    TEST_FUNCTION(Logger_HL_Create_fails_when_Logger_Create_fails)
    {
        ///arrage
        CLoggerMocks mocks;

        STRICT_EXPECTED_CALL(mocks, json_parse_string(VALID_CONFIG_STRING)); /*this is creating the JSON from the string*/
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG)) /*this is destroy of the json value created from the string*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG)) /*getting the json object out of the json value*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "filename")) /*this is getting a json string that is what follows "filename": in the json*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, MODULE_STATIC_GETAPIS(LOGGER_MODULE)()); /*this is finding the Logger's API*/
        STRICT_EXPECTED_CALL(mocks, Logger_Create(validBusHandle, IGNORED_PTR_ARG)) /*this is calling Logger_Create function*/
            .IgnoreArgument(2)
            .SetFailReturn((MODULE_HANDLE)NULL);

        ///act
        auto result = Logger_HL_Create(validBusHandle, VALID_CONFIG_STRING);

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_LOGGER_HL_02_012: [ If the JSON object does not contain a value named "filename" then Logger_HL_Create shall fail and return NULL. ]*/
    TEST_FUNCTION(Logger_HL_Create_fails_when_json_object_get_string_fails)
    {
        ///arrage
        CLoggerMocks mocks;

        STRICT_EXPECTED_CALL(mocks, json_parse_string(VALID_CONFIG_STRING)); /*this is creating the JSON from the string*/
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG)) /*this is destroy of the json value created from the string*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG)) /*getting the json object out of the json value*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "filename")) /*this is getting a json string that is what follows "filename": in the json*/
            .IgnoreArgument(1)
            .SetFailReturn((const char*)NULL);

        ///act
        auto result = Logger_HL_Create(validBusHandle, VALID_CONFIG_STRING);

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_LOGGER_HL_02_012: [ If the JSON object does not contain a value named "filename" then Logger_HL_Create shall fail and return NULL. ]*/
    TEST_FUNCTION(Logger_HL_Create_fails_when_json_value_get_object_fails)
    {
        ///arrage
        CLoggerMocks mocks;

        STRICT_EXPECTED_CALL(mocks, json_parse_string(VALID_CONFIG_STRING)); /*this is creating the JSON from the string*/
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG)) /*this is destroy of the json value created from the string*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG)) /*getting the json object out of the json value*/
            .IgnoreArgument(1)
            .SetFailReturn((JSON_Object*)NULL);

        ///act
        auto result = Logger_HL_Create(validBusHandle, VALID_CONFIG_STRING);

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_LOGGER_HL_02_011: [ If configuration is not a JSON object, then Logger_HL_Create shall fail and return NULL. ]*/
    TEST_FUNCTION(Logger_HL_Create_fails_when_json_parse_string_fails)
    {
        ///arrage
        CLoggerMocks mocks;

        STRICT_EXPECTED_CALL(mocks, json_parse_string(VALID_CONFIG_STRING)) /*this is creating the JSON from the string*/
            .SetFailReturn((JSON_Value*)NULL);

        ///act
        auto result = Logger_HL_Create(validBusHandle, VALID_CONFIG_STRING);

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_LOGGER_HL_02_008: [ Logger_HL_Receive shall pass the received parameters to the underlying Logger's _Receive function. ]*/
    TEST_FUNCTION(Logger_HL_Receive_passthrough_succeeds)
    {
        ///arrage
        CLoggerMocks mocks;
        MODULE_HANDLE handle = Logger_HL_Create(validBusHandle, VALID_CONFIG_STRING);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, MODULE_STATIC_GETAPIS(LOGGER_MODULE)()); /*this is finding the Logger's API*/
        STRICT_EXPECTED_CALL(mocks, Logger_Receive(handle, VALID_MESSAGE_HANDLE)); /*this is calling Logger_Receive function*/

        ///act
        Logger_HL_Receive(handle, VALID_MESSAGE_HANDLE);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Logger_HL_Destroy(handle);
    }

    /*Tests_SRS_LOGGER_HL_02_009: [ Logger_HL_Destroy shall destroy all used resources. ]*/
    TEST_FUNCTION(Logger_HL_Destroy_passthrough_succeeds)
    {
        ///arrage
        CLoggerMocks mocks;
        MODULE_HANDLE handle = Logger_HL_Create(validBusHandle, VALID_CONFIG_STRING);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, MODULE_STATIC_GETAPIS(LOGGER_MODULE)()); /*this is finding the Logger's API*/
        STRICT_EXPECTED_CALL(mocks, Logger_Destroy(handle)); /*this is calling Logger_Receive function*/

        ///act
        Logger_HL_Destroy(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_LOGGER_HL_02_010: [ Module_GetAPIS shall return a non-NULL pointer to a structure of type MODULE_APIS that has all fields non-NULL. ]*/
    TEST_FUNCTION(Module_GetAPIS_returns_non_NULL)
    {
        ///arrage
        CLoggerMocks mocks;
        
        ///act
        const MODULE_APIS* apis = Module_GetAPIS();

        ///assert
        ASSERT_IS_NOT_NULL(apis);
        ASSERT_IS_NOT_NULL(apis->Module_Destroy);
        ASSERT_IS_NOT_NULL(apis->Module_Create);
        ASSERT_IS_NOT_NULL(apis->Module_Receive);

        ///cleanup
    }

END_TEST_SUITE(logger_hl_unittests)
