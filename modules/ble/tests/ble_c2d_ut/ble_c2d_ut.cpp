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
#include "ble.h"
#include "ble_c2d.h"
#include "message.h"
#include "azure_c_shared_utility/constmap.h"
#include "azure_c_shared_utility/map.h"
#include "messageproperties.h"
#include "module_access.h"

static MICROMOCK_MUTEX_HANDLE g_testByTest;
static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

/*these are simple cached variables*/
static pfModule_ParseConfigurationFromJson  BLE_C2D_ParseConfigurationFromJson = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_FreeConfiguration  BLE_C2D_FreeConfiguration = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Create  BLE_C2D_Create = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Destroy BLE_C2D_Destroy = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Receive BLE_C2D_Receive = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/

static size_t gMessageSize;
static const unsigned char *gMessageSource;

static BUFFER_HANDLE gLastBuffer = NULL;
static STRING_HANDLE gLastString = NULL;

#define FAKE_CONFIG "" \
"{" \
"  \"modules\": [" \
"    {" \
"      \"module name\": \"BLE Printer\"," \
"      \"module path\": \"/ble_printer.so\"," \
"      \"args\": \"\"" \
"    }," \
"    {" \
"      \"module name\": \"SensorTag\"," \
"      \"module path\": \"/ble.so\"," \
"      \"args\": {" \
"        \"controller_index\": 0," \
"        \"device_mac_address\": \"AA:BB:CC:DD:EE:FF\"," \
"        \"instructions\": [" \
"          {" \
"            \"type\": \"read_once\"," \
"            \"characteristic_uuid\": \"00002A24-0000-1000-8000-00805F9B34FB\"" \
"          }," \
"          {" \
"            \"type\": \"write_at_init\"," \
"            \"characteristic_uuid\": \"F000AA02-0451-4000-B000-000000000000\"," \
"            \"data\": \"AQ==\"" \
"          }," \
"          {" \
"            \"type\": \"read_periodic\"," \
"            \"characteristic_uuid\": \"F000AA01-0451-4000-B000-000000000000\"," \
"            \"interval_in_ms\": 1000" \
"          }," \
"          {" \
"            \"type\": \"write_at_exit\"," \
"            \"characteristic_uuid\": \"F000AA02-0451-4000-B000-000000000000\"," \
"            \"data\": \"AA==\"" \
"          }" \
"        ]" \
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

TYPED_MOCK_CLASS(CBLEC2DMocks, CGlobalMock)
{
public:
    MOCK_STATIC_METHOD_1(, VECTOR_HANDLE, VECTOR_create, size_t, elementSize)
        auto result2 = BASEIMPLEMENTATION::VECTOR_create(elementSize);
    MOCK_METHOD_END(VECTOR_HANDLE, result2)
    
    MOCK_STATIC_METHOD_1(, void, VECTOR_destroy, VECTOR_HANDLE, vector)
        BASEIMPLEMENTATION::VECTOR_destroy(vector);
    MOCK_VOID_METHOD_END()
    
    MOCK_STATIC_METHOD_1(, size_t, VECTOR_size, VECTOR_HANDLE, vector)
        size_t result2 = BASEIMPLEMENTATION::VECTOR_size(vector);
    MOCK_METHOD_END(size_t, result2)
    
    MOCK_STATIC_METHOD_2(, void*, VECTOR_element, VECTOR_HANDLE, vector, size_t, index)
        void* result2 = BASEIMPLEMENTATION::VECTOR_element(vector, index);
    MOCK_METHOD_END(void*, result2)
    
    MOCK_STATIC_METHOD_3(, int, VECTOR_push_back, VECTOR_HANDLE, handle, const void*, elements, size_t, numElements)
        auto result2 = BASEIMPLEMENTATION::VECTOR_push_back(handle, elements, numElements);
    MOCK_METHOD_END(int, result2)
    
    MOCK_STATIC_METHOD_1(, STRING_HANDLE, STRING_construct, const char*, source)
        auto result2 = gLastString = BASEIMPLEMENTATION::STRING_construct(source);
    MOCK_METHOD_END(STRING_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, const char*, STRING_c_str, STRING_HANDLE, handle)
    MOCK_METHOD_END(const char*, BASEIMPLEMENTATION::STRING_c_str(handle))

    MOCK_STATIC_METHOD_1(, void, STRING_delete, STRING_HANDLE, handle)
        BASEIMPLEMENTATION::STRING_delete(handle);
    MOCK_VOID_METHOD_END()
    
    MOCK_STATIC_METHOD_1(, void, BUFFER_delete, BUFFER_HANDLE, handle)
        BASEIMPLEMENTATION::BUFFER_delete(handle);
    MOCK_VOID_METHOD_END()
    
    MOCK_STATIC_METHOD_1(, BUFFER_HANDLE, Base64_Decoder, const char*, source)
        auto result2 = gLastBuffer = BASEIMPLEMENTATION::BUFFER_create((const unsigned char*)"abc", 3);
    MOCK_METHOD_END(BUFFER_HANDLE, result2)
    
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
        if(strcmp(name, "device_mac_address") == 0)
        {
            result2 = "AA:BB:CC:DD:EE:FF";
        }
        else if(strcmp(name, "data") == 0)
        {
            result2 = "AA==";
        }
        else if(strcmp(name, "type") == 0)
        {
            result2 = "read_once";
        }
        else if(strcmp(name, "characteristic_uuid") == 0)
        {
            result2 = "00002A24-0000-1000-8000-00805F9B34FB";
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

    // Message
    MOCK_STATIC_METHOD_1(, MESSAGE_HANDLE, Message_Create, const MESSAGE_CONFIG*, cfg)
        gMessageSize = cfg->size;
        gMessageSource = cfg->source;
    MOCK_METHOD_END(MESSAGE_HANDLE, (MESSAGE_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1))

    MOCK_STATIC_METHOD_1(, CONSTMAP_HANDLE, Message_GetProperties, MESSAGE_HANDLE, message)
    MOCK_METHOD_END(CONSTMAP_HANDLE, (CONSTMAP_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1))

    MOCK_STATIC_METHOD_1(, const CONSTBUFFER*, Message_GetContent, MESSAGE_HANDLE, message)
    MOCK_METHOD_END(const CONSTBUFFER*, (const CONSTBUFFER*)NULL);

    MOCK_STATIC_METHOD_1(, void, Message_Destroy, MESSAGE_HANDLE, message)
        BASEIMPLEMENTATION::gballoc_free(message);
    MOCK_VOID_METHOD_END()

    // CONSTMAP
    MOCK_STATIC_METHOD_1(, MAP_HANDLE, ConstMap_CloneWriteable, CONSTMAP_HANDLE, handle)
    MOCK_METHOD_END(MAP_HANDLE, (MAP_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1))

    MOCK_STATIC_METHOD_2(, const char*, ConstMap_GetValue, CONSTMAP_HANDLE, handle, const char*, key)
    MOCK_METHOD_END(const char*, (const char*)NULL)

    MOCK_STATIC_METHOD_1(, void, ConstMap_Destroy, CONSTMAP_HANDLE, handle)
        BASEIMPLEMENTATION::gballoc_free(handle);
    MOCK_VOID_METHOD_END()

    //MAP
    MOCK_STATIC_METHOD_3(, MAP_RESULT, Map_AddOrUpdate, MAP_HANDLE, handle, const char*, key, const char*, value)
    MOCK_METHOD_END(MAP_RESULT, MAP_OK)

    MOCK_STATIC_METHOD_1(, void, Map_Destroy, MAP_HANDLE, handle)
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_3(, BROKER_RESULT, Broker_Publish, BROKER_HANDLE, broker, MODULE_HANDLE, source, MESSAGE_HANDLE, message)
        auto result2 = BROKER_OK;
    MOCK_METHOD_END(BROKER_RESULT, result2)
};

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEC2DMocks, , JSON_Value*, json_parse_string, const char *, filename);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEC2DMocks, , JSON_Object*, json_value_get_object, const JSON_Value*, value);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEC2DMocks, , double, json_object_get_number, const JSON_Object*, value, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEC2DMocks, , const char*, json_object_get_string, const JSON_Object*, object, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEC2DMocks, , JSON_Array*, json_object_get_array, const JSON_Object*, object, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEC2DMocks, , void, json_value_free, JSON_Value*, value);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEC2DMocks, , size_t, json_array_get_count, const JSON_Array*, arr);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEC2DMocks, , JSON_Object*, json_array_get_object, const JSON_Array*, arr, size_t, index);

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEC2DMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEC2DMocks, , void, gballoc_free, void*, ptr);

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEC2DMocks, , VECTOR_HANDLE, VECTOR_create, size_t, elementSize);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEC2DMocks, , void, VECTOR_destroy, VECTOR_HANDLE, vector);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEC2DMocks, , size_t, VECTOR_size, VECTOR_HANDLE, vector);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEC2DMocks, , void*, VECTOR_element, VECTOR_HANDLE, vector, size_t, index);
DECLARE_GLOBAL_MOCK_METHOD_3(CBLEC2DMocks, , int, VECTOR_push_back, VECTOR_HANDLE, handle, const void*, elements, size_t, numElements);

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEC2DMocks, , BUFFER_HANDLE, Base64_Decoder, const char*, source);

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEC2DMocks, , STRING_HANDLE, STRING_construct, const char*, source);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEC2DMocks, , const char*, STRING_c_str, STRING_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEC2DMocks, , void, STRING_delete, STRING_HANDLE, handle);

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEC2DMocks, , void, BUFFER_delete, BUFFER_HANDLE, handle);

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEC2DMocks, , MESSAGE_HANDLE, Message_Create, const MESSAGE_CONFIG*, cfg);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEC2DMocks, , CONSTMAP_HANDLE, Message_GetProperties, MESSAGE_HANDLE, message);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEC2DMocks, , const CONSTBUFFER*, Message_GetContent, MESSAGE_HANDLE, message);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEC2DMocks, , void, Message_Destroy, MESSAGE_HANDLE, message);

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEC2DMocks, , MAP_HANDLE, ConstMap_CloneWriteable, CONSTMAP_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEC2DMocks, , const char*, ConstMap_GetValue, CONSTMAP_HANDLE, handle, const char*, key);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEC2DMocks, , void, ConstMap_Destroy, CONSTMAP_HANDLE, handle);

DECLARE_GLOBAL_MOCK_METHOD_3(CBLEC2DMocks, , MAP_RESULT, Map_AddOrUpdate, MAP_HANDLE, handle, const char*, key, const char*, value);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEC2DMocks, , void, Map_Destroy, MAP_HANDLE, handle);

DECLARE_GLOBAL_MOCK_METHOD_3(CBLEC2DMocks, , BROKER_RESULT, Broker_Publish, BROKER_HANDLE, broker, MODULE_HANDLE, source, MESSAGE_HANDLE, message);

BEGIN_TEST_SUITE(ble_c2d_ut)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
        g_testByTest = MicroMockCreateMutex();
        ASSERT_IS_NOT_NULL(g_testByTest);

        const MODULE_API *apis = Module_GetApi(MODULE_API_VERSION_1);
        BLE_C2D_ParseConfigurationFromJson = MODULE_PARSE_CONFIGURATION_FROM_JSON(apis);
        BLE_C2D_FreeConfiguration = MODULE_FREE_CONFIGURATION(apis);
        BLE_C2D_Create = MODULE_CREATE(apis);
        BLE_C2D_Destroy = MODULE_DESTROY(apis);
        BLE_C2D_Receive = MODULE_RECEIVE(apis);
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

    /*Tests_SRS_BLE_CTOD_17_027: [ BLE_C2D_ParseConfigurationFromJson shall return NULL. ]*/
    TEST_FUNCTION(BLE_C2D_ParseConfigurationFromJson_returns_NULL_when_config_is_NULL)
    {
        ///arrange
        CBLEC2DMocks mocks;

        ///act
        auto result = BLE_C2D_ParseConfigurationFromJson(NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_CTOD_17_027: [ BLE_C2D_ParseConfigurationFromJson shall return NULL. ]*/
    TEST_FUNCTION(BLE_C2D_ParseConfigurationFromJson_returns_NULL_when_config_is_not_null)
    {
        ///arrange
        CBLEC2DMocks mocks;

        ///act
        auto result = BLE_C2D_ParseConfigurationFromJson((const char *)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_CTOD_17_028: [ BLE_C2D_FreeConfiguration shall do nothing. ]*/
    TEST_FUNCTION(BLE_C2D_FreeConfiguration_empty)
    {
        ///arrange
        CBLEC2DMocks mocks;

        ///act
        auto result = BLE_C2D_ParseConfigurationFromJson((const char *)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }


    /*Tests_SRS_BLE_CTOD_13_001: [ BLE_C2D_Create shall return NULL if the broker parameter is NULL. ]*/
    TEST_FUNCTION(BLE_C2D_Create_returns_NULL_when_broker_is_NULL)
    {
        ///arrange
        CBLEC2DMocks mocks;

        ///act
        auto result = BLE_C2D_Create(NULL, (const void*)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_CTOD_13_002: [ BLE_C2D_Create shall return NULL if any of the underlying platform calls fail. ]*/
    TEST_FUNCTION(BLE_C2D_Create_returns_NULL_when_malloc_fails)
    {
        ///arrange
        CBLEC2DMocks mocks;

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((void*)NULL);

        ///act
        auto result = BLE_C2D_Create((BROKER_HANDLE)0x42, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_CTOD_13_023: [ BLE_C2D_Create shall return a non-NULL handle when the function succeeds. ]*/
    TEST_FUNCTION(BLE_C2D_Create_succeeds)
    {
        ///arrange
        CBLEC2DMocks mocks;

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        ///act
        auto result = BLE_C2D_Create((BROKER_HANDLE)0x42, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NOT_NULL(result);

        ///cleanup
        BLE_C2D_Destroy(result);
    }

    /*Tests_SRS_BLE_CTOD_26_001: [ `Module_GetApi` shall return a pointer to a `MODULE_API` structure. ]*/
    TEST_FUNCTION(Module_GetApi_returns_non_NULL)
    {
        ///arrage
        CBLEC2DMocks mocks;

        ///act
        const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);


        ///assert
        ASSERT_IS_NOT_NULL(apis);
        ASSERT_IS_TRUE(MODULE_DESTROY(apis) != NULL);
        ASSERT_IS_TRUE(MODULE_CREATE(apis) != NULL);
        ASSERT_IS_TRUE(MODULE_RECEIVE(apis) != NULL);

        ///cleanup
    }

    /*Tests_SRS_BLE_CTOD_13_017: [ BLE_C2D_Destroy shall do nothing if module is NULL. ]*/
    TEST_FUNCTION(BLE_C2D_Destroy_does_nothing_with_NULL_handle)
    {
        ///arrange
        CBLEC2DMocks mocks;

        ///act
        BLE_C2D_Destroy(NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_BLE_CTOD_13_015: [ BLE_C2D_Destroy shall destroy all used resources. ]*/
    TEST_FUNCTION(BLE_C2D_Destroy_frees_resources)
    {
        ///arrange
        CBLEC2DMocks mocks;

        auto handle = BLE_C2D_Create((BROKER_HANDLE)0x42, NULL);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_C2D_Destroy(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_BLE_CTOD_13_018: [ BLE_C2D_Receive shall publish the new message to the broker. ]*/
    /*Tests_SRS_BLE_CTOD_17_023: [ BLE_C2D_Receive shall create a new message by calling Message_Create with new map and BLE_INSTRUCTION as the buffer. ]*/
    /*Tests_SRS_BLE_CTOD_17_020: [ BLE_C2D_Receive shall call add a property with key of "source" and value of "bleCommand". ]*/
    /*Tests_SRS_BLE_CTOD_17_014: [ BLE_C2D_Receive shall parse the json object to fill in a new BLE_INSTRUCTION. ]*/
    /*Tests_SRS_BLE_CTOD_17_006: [ BLE_C2D_Receive shall parse the message contents as a JSON object. ]*/
    TEST_FUNCTION(BLE_C2D_Receive_publishes_message)
    {
        ///arrange
        CBLEC2DMocks mocks;
        unsigned char fake = '\0';
        CONSTBUFFER messageBuffer;
        messageBuffer.buffer = &fake;
        messageBuffer.size = 1;
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)1);

        auto module = BLE_C2D_Create((BROKER_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)GW_IDMAP_MODULE);
        STRICT_EXPECTED_CALL(mocks, Message_GetContent(fakeMessage))
            .SetReturn((const CONSTBUFFER *)&messageBuffer);
        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn("write_once");
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "characteristic_uuid"))
            .IgnoreArgument(1)
            .SetReturn("F000AA02-0451-4000-B000-000000000000");
        STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "data"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Base64_Decoder(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((MAP_HANDLE)0x42);
        STRICT_EXPECTED_CALL(mocks, Map_Destroy((MAP_HANDLE)0x42));

        STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY, GW_SOURCE_BLE_COMMAND))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Message_Create(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Message_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Broker_Publish((BROKER_HANDLE)0x42, module, IGNORED_PTR_ARG))
            .IgnoreArgument(3);

        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_C2D_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_C2D_Destroy(module);
        BUFFER_delete(gLastBuffer);
        STRING_delete(gLastString);
    }

    /*Tests_SRS_BLE_CTOD_13_016: [ BLE_C2D_Receive shall do nothing if module is NULL. ]*/
    TEST_FUNCTION(BLE_C2D_Receive_does_nothing_for_NULL_module)
    {
        ///arrange
        CBLEC2DMocks mocks;

        ///act
        BLE_C2D_Receive(NULL, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_BLE_CTOD_17_001: [ BLE_C2D_Receive shall do nothing if message_handle is NULL. ]*/
    TEST_FUNCTION(BLE_C2D_Receive_does_nothing_for_NULL_message)
    {
        ///arrange
        CBLEC2DMocks mocks;

        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)1);

        auto module = BLE_C2D_Create((BROKER_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        ///act
        BLE_C2D_Receive(module, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_C2D_Destroy(module);
    }

    /*Tests_SRS_BLE_CTOD_17_002: [ If message_handle properties does not contain "macAddress" property, then this function shall do nothing. ]*/
    TEST_FUNCTION(BLE_C2D_Receive_does_nothing_when_no_mac_address)
    {
        ///arrange
        CBLEC2DMocks mocks;
        unsigned char fake = '\0';
        CONSTBUFFER messageBuffer;
        messageBuffer.buffer = &fake;
        messageBuffer.size = 1;
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)1);

        auto module = BLE_C2D_Create((BROKER_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetFailReturn((const char *)NULL);
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_C2D_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_C2D_Destroy(module);
    }

    /*Tests_SRS_BLE_CTOD_17_004: [ If message_handle properties does not contain "source" property, then this function shall do nothing. ]*/
    TEST_FUNCTION(BLE_C2D_Receive_does_nothing_when_no_source)
    {
        ///arrange
        CBLEC2DMocks mocks;
        unsigned char fake = '\0';
        CONSTBUFFER messageBuffer;
        messageBuffer.buffer = &fake;
        messageBuffer.size = 1;
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)1);

        auto module = BLE_C2D_Create((BROKER_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
            .IgnoreArgument(1)
            .SetFailReturn((const char *)NULL);
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_C2D_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_C2D_Destroy(module);
    }

    /*Tests_SRS_BLE_CTOD_17_005: [ If the source of the message properties is not "mapping", then this function shall do nothing. ]*/
    TEST_FUNCTION(BLE_C2D_Receive_does_nothing_when_source_is_not_mapping)
    {
        ///arrange
        CBLEC2DMocks mocks;
        unsigned char fake = '\0';
        CONSTBUFFER messageBuffer;
        messageBuffer.buffer = &fake;
        messageBuffer.size = 1;
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)1);

        auto module = BLE_C2D_Create((BROKER_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"Nope. Not mapping");
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_C2D_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_C2D_Destroy(module);
    }

    /*Tests_SRS_BLE_CTOD_13_024: [ BLE_C2D_Receive shall do nothing if an underlying API call fails. ]*/
    TEST_FUNCTION(BLE_C2D_Receive_does_nothing_when_Message_GetContent_fails)
    {
        ///arrange
        CBLEC2DMocks mocks;
        unsigned char fake = '\0';
        CONSTBUFFER messageBuffer;
        messageBuffer.buffer = &fake;
        messageBuffer.size = 1;
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)1);

        auto module = BLE_C2D_Create((BROKER_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)GW_IDMAP_MODULE);
        STRICT_EXPECTED_CALL(mocks, Message_GetContent(fakeMessage))
            .SetFailReturn((const CONSTBUFFER *)NULL);

        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_C2D_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_C2D_Destroy(module);
    }

    /*Tests_SRS_BLE_CTOD_17_007: [ If the message contents do not parse, then BLE_C2D_Receive shall do nothing. ]*/
    TEST_FUNCTION(BLE_C2D_Receive_does_nothing_when_json_parse_string_fails)
    {
        ///arrange
        CBLEC2DMocks mocks;
        unsigned char fake = '\0';
        CONSTBUFFER messageBuffer;
        messageBuffer.buffer = &fake;
        messageBuffer.size = 1;
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)1);

        auto module = BLE_C2D_Create((BROKER_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)GW_IDMAP_MODULE);
        STRICT_EXPECTED_CALL(mocks, Message_GetContent(fakeMessage))
            .SetReturn((const CONSTBUFFER *)&messageBuffer);
        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((JSON_Value*)NULL);

        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_C2D_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_C2D_Destroy(module);
    }

    /*Tests_SRS_BLE_CTOD_17_014: [ BLE_C2D_Receive shall parse the json object to fill in a new BLE_INSTRUCTION. ]*/
    TEST_FUNCTION(BLE_C2D_Receive_does_nothing_when_json_root_is_not_object)
    {
        ///arrange
        CBLEC2DMocks mocks;
        unsigned char fake = '\0';
        CONSTBUFFER messageBuffer;
        messageBuffer.buffer = &fake;
        messageBuffer.size = 1;
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)1);

        auto module = BLE_C2D_Create((BROKER_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)GW_IDMAP_MODULE);
        STRICT_EXPECTED_CALL(mocks, Message_GetContent(fakeMessage))
            .SetReturn((const CONSTBUFFER *)&messageBuffer);
        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((JSON_Object*)NULL);

        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_C2D_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_C2D_Destroy(module);
    }

    /*Tests_SRS_BLE_CTOD_17_008: [ BLE_C2D_Receive shall return if the JSON object does not contain the following fields: "type" and "characteristic_uuid". ]*/
    TEST_FUNCTION(BLE_C2D_Receive_does_nothing_when_json_has_no_type_prop)
    {
        ///arrange
        CBLEC2DMocks mocks;
        unsigned char fake = '\0';
        CONSTBUFFER messageBuffer;
        messageBuffer.buffer = &fake;
        messageBuffer.size = 1;
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)1);

        auto module = BLE_C2D_Create((BROKER_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)GW_IDMAP_MODULE);
        STRICT_EXPECTED_CALL(mocks, Message_GetContent(fakeMessage))
            .SetReturn((const CONSTBUFFER *)&messageBuffer);
        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetFailReturn((const char*)NULL);

        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_C2D_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_C2D_Destroy(module);
    }

    /*Tests_SRS_BLE_CTOD_17_008: [ BLE_C2D_Receive shall return if the JSON object does not contain the following fields: "type" and "characteristic_uuid". ]*/
    TEST_FUNCTION(BLE_C2D_Receive_does_nothing_when_json_has_no_characteristic_uuid_prop)
    {
        ///arrange
        CBLEC2DMocks mocks;
        unsigned char fake = '\0';
        CONSTBUFFER messageBuffer;
        messageBuffer.buffer = &fake;
        messageBuffer.size = 1;
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)1);

        auto module = BLE_C2D_Create((BROKER_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)GW_IDMAP_MODULE);
        STRICT_EXPECTED_CALL(mocks, Message_GetContent(fakeMessage))
            .SetReturn((const CONSTBUFFER *)&messageBuffer);
        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn("write_once");
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "characteristic_uuid"))
            .IgnoreArgument(1)
            .SetFailReturn((const char*)NULL);

        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_C2D_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_C2D_Destroy(module);
    }

    /*Tests_SRS_BLE_CTOD_13_024: [ BLE_C2D_Receive shall do nothing if an underlying API call fails. ]*/
    TEST_FUNCTION(BLE_C2D_Receive_does_nothing_when_STRING_construct_fails)
    {
        ///arrange
        CBLEC2DMocks mocks;
        unsigned char fake = '\0';
        CONSTBUFFER messageBuffer;
        messageBuffer.buffer = &fake;
        messageBuffer.size = 1;
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)1);

        auto module = BLE_C2D_Create((BROKER_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)GW_IDMAP_MODULE);
        STRICT_EXPECTED_CALL(mocks, Message_GetContent(fakeMessage))
            .SetReturn((const CONSTBUFFER *)&messageBuffer);
        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn("write_once");
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "characteristic_uuid"))
            .IgnoreArgument(1)
            .SetReturn("F000AA02-0451-4000-B000-000000000000");
        STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((STRING_HANDLE)NULL);

        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_C2D_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_C2D_Destroy(module);
    }

    /*Tests_SRS_BLE_CTOD_17_026: [ If the json object does not parse, BLE_C2D_Receive shall return. ]*/
    TEST_FUNCTION(BLE_C2D_Receive_does_nothing_when_json_has_no_data_prop)
    {
        ///arrange
        CBLEC2DMocks mocks;
        unsigned char fake = '\0';
        CONSTBUFFER messageBuffer;
        messageBuffer.buffer = &fake;
        messageBuffer.size = 1;
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)1);

        auto module = BLE_C2D_Create((BROKER_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)GW_IDMAP_MODULE);
        STRICT_EXPECTED_CALL(mocks, Message_GetContent(fakeMessage))
            .SetReturn((const CONSTBUFFER *)&messageBuffer);
        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn("write_once");
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "characteristic_uuid"))
            .IgnoreArgument(1)
            .SetReturn("F000AA02-0451-4000-B000-000000000000");
        STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "data"))
            .IgnoreArgument(1)
            .SetFailReturn((const char*)NULL);

        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_C2D_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_C2D_Destroy(module);
    }

    /*Tests_SRS_BLE_CTOD_13_024: [ BLE_C2D_Receive shall do nothing if an underlying API call fails. ]*/
    TEST_FUNCTION(BLE_C2D_Receive_does_nothing_when_Base64_Decoder_fails)
    {
        ///arrange
        CBLEC2DMocks mocks;
        unsigned char fake = '\0';
        CONSTBUFFER messageBuffer;
        messageBuffer.buffer = &fake;
        messageBuffer.size = 1;
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)1);

        auto module = BLE_C2D_Create((BROKER_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)GW_IDMAP_MODULE);
        STRICT_EXPECTED_CALL(mocks, Message_GetContent(fakeMessage))
            .SetReturn((const CONSTBUFFER *)&messageBuffer);
        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn("write_once");
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "characteristic_uuid"))
            .IgnoreArgument(1)
            .SetReturn("F000AA02-0451-4000-B000-000000000000");
        STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "data"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Base64_Decoder(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((BUFFER_HANDLE)NULL);

        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_C2D_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_C2D_Destroy(module);
    }

    /*Tests_SRS_BLE_CTOD_13_024: [ BLE_C2D_Receive shall do nothing if an underlying API call fails. ]*/
    TEST_FUNCTION(BLE_C2D_Receive_does_nothing_when_ConstMap_CloneWriteable_fails)
    {
        ///arrange
        CBLEC2DMocks mocks;
        unsigned char fake = '\0';
        CONSTBUFFER messageBuffer;
        messageBuffer.buffer = &fake;
        messageBuffer.size = 1;
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)1);

        auto module = BLE_C2D_Create((BROKER_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)GW_IDMAP_MODULE);
        STRICT_EXPECTED_CALL(mocks, Message_GetContent(fakeMessage))
            .SetReturn((const CONSTBUFFER *)&messageBuffer);
        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn("write_once");
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "characteristic_uuid"))
            .IgnoreArgument(1)
            .SetReturn("F000AA02-0451-4000-B000-000000000000");
        STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "data"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Base64_Decoder(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((MAP_HANDLE)NULL);

        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_C2D_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_C2D_Destroy(module);
    }

    /*Tests_SRS_BLE_CTOD_13_024: [ BLE_C2D_Receive shall do nothing if an underlying API call fails. ]*/
    TEST_FUNCTION(BLE_C2D_Receive_does_nothing_when_Map_AddOrUpdate_fails)
    {
        ///arrange
        CBLEC2DMocks mocks;
        unsigned char fake = '\0';
        CONSTBUFFER messageBuffer;
        messageBuffer.buffer = &fake;
        messageBuffer.size = 1;
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)1);

        auto module = BLE_C2D_Create((BROKER_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)GW_IDMAP_MODULE);
        STRICT_EXPECTED_CALL(mocks, Message_GetContent(fakeMessage))
            .SetReturn((const CONSTBUFFER *)&messageBuffer);
        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn("write_once");
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "characteristic_uuid"))
            .IgnoreArgument(1)
            .SetReturn("F000AA02-0451-4000-B000-000000000000");
        STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "data"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Base64_Decoder(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((MAP_HANDLE)0x42);

        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Map_Destroy((MAP_HANDLE)0x42));

        STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY, GW_SOURCE_BLE_COMMAND))
            .IgnoreArgument(1)
            .SetFailReturn((MAP_RESULT)MAP_ERROR);

        ///act
        BLE_C2D_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_C2D_Destroy(module);
    }

    /*Tests_SRS_BLE_CTOD_17_024: [ If creating new message fails, BLE_C2D_Receive shall de-allocate all resources and return. ]*/
    TEST_FUNCTION(BLE_C2D_Receive_does_nothing_when_Message_Create_fails)
    {
        ///arrange
        CBLEC2DMocks mocks;
        unsigned char fake = '\0';
        CONSTBUFFER messageBuffer;
        messageBuffer.buffer = &fake;
        messageBuffer.size = 1;
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)1);

        auto module = BLE_C2D_Create((BROKER_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)GW_IDMAP_MODULE);
        STRICT_EXPECTED_CALL(mocks, Message_GetContent(fakeMessage))
            .SetReturn((const CONSTBUFFER *)&messageBuffer);
        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn("write_once");
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "characteristic_uuid"))
            .IgnoreArgument(1)
            .SetReturn("F000AA02-0451-4000-B000-000000000000");
        STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "data"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Base64_Decoder(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((MAP_HANDLE)0x42);

        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Map_Destroy((MAP_HANDLE)0x42));

        STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY, GW_SOURCE_BLE_COMMAND))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Message_Create(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((MESSAGE_HANDLE)NULL);

        ///act
        BLE_C2D_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_C2D_Destroy(module);
    }

    /*Tests_SRS_BLE_CTOD_13_024: [ BLE_C2D_Receive shall do nothing if an underlying API call fails. ]*/
    TEST_FUNCTION(BLE_C2D_Receive_does_nothing_when_Broker_Publish_fails)
    {
        ///arrange
        CBLEC2DMocks mocks;
        unsigned char fake = '\0';
        CONSTBUFFER messageBuffer;
        messageBuffer.buffer = &fake;
        messageBuffer.size = 1;
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)1);

        auto module = BLE_C2D_Create((BROKER_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)GW_IDMAP_MODULE);
        STRICT_EXPECTED_CALL(mocks, Message_GetContent(fakeMessage))
            .SetReturn((const CONSTBUFFER *)&messageBuffer);
        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn("write_once");
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "characteristic_uuid"))
            .IgnoreArgument(1)
            .SetReturn("F000AA02-0451-4000-B000-000000000000");
        STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "data"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Base64_Decoder(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((MAP_HANDLE)0x42);

        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Map_Destroy((MAP_HANDLE)0x42));

        STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY, GW_SOURCE_BLE_COMMAND))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Message_Create(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Message_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Broker_Publish((BROKER_HANDLE)0x42, module, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .SetFailReturn((BROKER_RESULT)BROKER_ERROR);

        ///act
        BLE_C2D_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_C2D_Destroy(module);
    }

END_TEST_SUITE(ble_c2d_ut)
