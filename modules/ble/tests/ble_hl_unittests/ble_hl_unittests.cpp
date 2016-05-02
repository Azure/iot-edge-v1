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
#include "ble_hl.h"
#include "message.h"
#include "azure_c_shared_utility/constmap.h"
#include "azure_c_shared_utility/map.h"
#include "messageproperties.h"

static MICROMOCK_MUTEX_HANDLE g_testByTest;
static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

/*these are simple cached variables*/
static pfModule_Create  BLE_HL_Create = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Destroy BLE_HL_Destroy = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Receive BLE_HL_Receive = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/

static size_t gMessageSize;
static const unsigned char * gMessageSource;

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
"      \"module path\": \"/ble_hl.so\"," \
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

/*forward declarations*/
MODULE_HANDLE BLE_Create(MESSAGE_BUS_HANDLE busHandle, const void* configuration);
/*this destroys (frees resources) of the module parameter*/
void BLE_Destroy(MODULE_HANDLE moduleHandle);
/*this is the module's callback function - gets called when a message is to be received by the module*/
void BLE_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle);

static MODULE_APIS BLE_APIS =
{
    BLE_Create,
    BLE_Destroy,
    BLE_Receive
};

TYPED_MOCK_CLASS(CBLEHLMocks, CGlobalMock)
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
        auto result2 = BASEIMPLEMENTATION::STRING_construct(source);
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
        auto result2 = BASEIMPLEMENTATION::BUFFER_create((const unsigned char*)"abc", 3);
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
    
    MOCK_STATIC_METHOD_0(, const MODULE_APIS*, MODULE_STATIC_GETAPIS(BLE_MODULE))
    MOCK_METHOD_END(const MODULE_APIS*, (const MODULE_APIS*)&BLE_APIS);

    MOCK_STATIC_METHOD_2(, MODULE_HANDLE, BLE_Create, MESSAGE_BUS_HANDLE, busHandle, const void*, configuration)
        BLE_CONFIG *config = (BLE_CONFIG*)malloc(sizeof(BLE_CONFIG));
        memcpy(config, configuration, sizeof(BLE_CONFIG));
        MODULE_HANDLE result2 = (MODULE_HANDLE)config;
    MOCK_METHOD_END(MODULE_HANDLE, result2);

    MOCK_STATIC_METHOD_1(, void, BLE_Destroy, MODULE_HANDLE, moduleHandle);
    {
        if(moduleHandle != NULL)
        {
            BLE_CONFIG* config = (BLE_CONFIG*)moduleHandle;
            if(config->instructions != NULL)
            {
                size_t len = VECTOR_size(config->instructions);
                for (size_t i = 0; i < len; i++)
                {
                    BLE_INSTRUCTION* instr = (BLE_INSTRUCTION*)VECTOR_element(config->instructions, i);

                    // free resources
                    if (instr->characteristic_uuid != NULL)
                    {
                        STRING_delete(instr->characteristic_uuid);
                    }
                    if (
                            (
                                instr->instruction_type == WRITE_AT_INIT
                                ||
                                instr->instruction_type == WRITE_AT_EXIT
                                ||
                                instr->instruction_type == WRITE_ONCE
                            )
                            &&
                            instr->data.buffer != NULL
                       )
                    {
                        BUFFER_delete(instr->data.buffer);
                    }
                }

                VECTOR_destroy(config->instructions);
            }

            free(config);
        }
    }
    MOCK_VOID_METHOD_END()

        MOCK_STATIC_METHOD_2(, void, BLE_Receive, MODULE_HANDLE, moduleHandle, MESSAGE_HANDLE, messageHandle)
        if (moduleHandle != NULL)
        {
            BLE_CONFIG* config = (BLE_CONFIG*)moduleHandle;
            if (config->instructions == NULL)
            {
                config->instructions = VECTOR_create(sizeof(BLE_INSTRUCTION));
            }
            if (config->instructions != NULL)
            {
                BLE_INSTRUCTION* instr = (BLE_INSTRUCTION*)gMessageSource;
                if (instr != NULL)
                {
                    BASEIMPLEMENTATION::VECTOR_push_back(config->instructions, instr, 1);
                }
            }
        }

    MOCK_VOID_METHOD_END()

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
};

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEHLMocks, , JSON_Value*, json_parse_string, const char *, filename);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEHLMocks, , JSON_Object*, json_value_get_object, const JSON_Value*, value);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEHLMocks, , double, json_object_get_number, const JSON_Object*, value, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEHLMocks, , const char*, json_object_get_string, const JSON_Object*, object, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEHLMocks, , JSON_Array*, json_object_get_array, const JSON_Object*, object, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEHLMocks, , void, json_value_free, JSON_Value*, value);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEHLMocks, , size_t, json_array_get_count, const JSON_Array*, arr);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEHLMocks, , JSON_Object*, json_array_get_object, const JSON_Array*, arr, size_t, index);

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEHLMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEHLMocks, , void, gballoc_free, void*, ptr);

DECLARE_GLOBAL_MOCK_METHOD_2(CBLEHLMocks, , MODULE_HANDLE, BLE_Create, MESSAGE_BUS_HANDLE, busHandle, const void*, configuration);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEHLMocks, , void, BLE_Destroy, MODULE_HANDLE, moduleHandle);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEHLMocks, , void, BLE_Receive, MODULE_HANDLE, moduleHandle, MESSAGE_HANDLE, messageHandle);
DECLARE_GLOBAL_MOCK_METHOD_0(CBLEHLMocks, , const MODULE_APIS*, MODULE_STATIC_GETAPIS(BLE_MODULE));

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEHLMocks, , VECTOR_HANDLE, VECTOR_create, size_t, elementSize);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEHLMocks, , void, VECTOR_destroy, VECTOR_HANDLE, vector);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEHLMocks, , size_t, VECTOR_size, VECTOR_HANDLE, vector);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEHLMocks, , void*, VECTOR_element, VECTOR_HANDLE, vector, size_t, index);
DECLARE_GLOBAL_MOCK_METHOD_3(CBLEHLMocks, , int, VECTOR_push_back, VECTOR_HANDLE, handle, const void*, elements, size_t, numElements);

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEHLMocks, , BUFFER_HANDLE, Base64_Decoder, const char*, source);

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEHLMocks, , STRING_HANDLE, STRING_construct, const char*, source);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEHLMocks, , const char*, STRING_c_str, STRING_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEHLMocks, , void, STRING_delete, STRING_HANDLE, handle);

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEHLMocks, , void, BUFFER_delete, BUFFER_HANDLE, handle);

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEHLMocks, , MESSAGE_HANDLE, Message_Create, const MESSAGE_CONFIG*, cfg);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEHLMocks, , CONSTMAP_HANDLE, Message_GetProperties, MESSAGE_HANDLE, message);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEHLMocks, , const CONSTBUFFER*, Message_GetContent, MESSAGE_HANDLE, message);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEHLMocks, , void, Message_Destroy, MESSAGE_HANDLE, message);

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEHLMocks, , MAP_HANDLE, ConstMap_CloneWriteable, CONSTMAP_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEHLMocks, , const char*, ConstMap_GetValue, CONSTMAP_HANDLE, handle, const char*, key);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEHLMocks, , void, ConstMap_Destroy, CONSTMAP_HANDLE, handle);

DECLARE_GLOBAL_MOCK_METHOD_3(CBLEHLMocks, , MAP_RESULT, Map_AddOrUpdate, MAP_HANDLE, handle, const char*, key, const char*, value);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEHLMocks, , void, Map_Destroy, MAP_HANDLE, handle);

BEGIN_TEST_SUITE(ble_hl_unittests)
    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
        g_testByTest = MicroMockCreateMutex();
        ASSERT_IS_NOT_NULL(g_testByTest);

        BLE_HL_Create = Module_GetAPIS()->Module_Create;
        BLE_HL_Destroy = Module_GetAPIS()->Module_Destroy;
        BLE_HL_Receive = Module_GetAPIS()->Module_Receive;
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

    /*Tests_SRS_BLE_HL_13_001: [ BLE_HL_Create shall return NULL if the bus or configuration parameters are NULL. ]*/
    TEST_FUNCTION(BLE_HL_Create_returns_NULL_when_bus_is_NULL)
    {
        ///arrange
        CBLEHLMocks mocks;

        ///act
        auto result = BLE_HL_Create(NULL, (const void*)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_HL_13_001: [ BLE_HL_Create shall return NULL if the bus or configuration parameters are NULL. ]*/
    TEST_FUNCTION(BLE_HL_Create_returns_NULL_when_configuration_is_NULL)
    {
        ///arrange
        CBLEHLMocks mocks;

        ///act
        auto result = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_HL_13_002: [ BLE_HL_Create shall return NULL if any of the underlying platform calls fail. ]*/
    TEST_FUNCTION(BLE_HL_Create_returns_NULL_when_json_parse_string_fails)
    {
        ///arrange
        CBLEHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((JSON_Value*)NULL);

        ///act
        auto result = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_HL_13_003: [ BLE_HL_Create shall return NULL if the JSON does not start with an object. ]*/
    TEST_FUNCTION(BLE_HL_Create_returns_NULL_when_json_value_get_object_for_root_fails)
    {
        ///arrange
        CBLEHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((JSON_Object*)NULL);

        ///act
        auto result = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_HL_13_005: [ BLE_HL_Create shall return NULL if the controller_index value in the JSON is less than zero. ]*/
    TEST_FUNCTION(BLE_HL_Create_returns_NULL_when_controller_index_is_negative)
    {
        ///arrange
        CBLEHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_number(IGNORED_PTR_ARG, "controller_index"))
            .IgnoreArgument(1)
            .SetFailReturn((int)-1);

        ///act
        auto result = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_HL_13_004: [ BLE_HL_Create shall return NULL if there is no device_mac_address property in the JSON. ]*/
    TEST_FUNCTION(BLE_HL_Create_returns_NULL_when_mac_address_is_NULL)
    {
        ///arrange
        CBLEHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_number(IGNORED_PTR_ARG, "controller_index"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "device_mac_address"))
            .IgnoreArgument(1)
            .SetFailReturn((const char*)NULL);

        ///act
        auto result = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_HL_13_006: [ BLE_HL_Create shall return NULL if the instructions array does not exist in the JSON. ]*/
    TEST_FUNCTION(BLE_HL_Create_returns_NULL_when_instructions_is_NULL)
    {
        ///arrange
        CBLEHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_number(IGNORED_PTR_ARG, "controller_index"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "device_mac_address"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "instructions"))
            .IgnoreArgument(1)
            .SetFailReturn((JSON_Array *)NULL);

        ///act
        auto result = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_HL_13_020: [ BLE_HL_Create shall return NULL if the instructions array length is equal to zero. ]*/
    TEST_FUNCTION(BLE_HL_Create_returns_NULL_when_instructions_is_empty)
    {
        ///arrange
        CBLEHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_number(IGNORED_PTR_ARG, "controller_index"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "device_mac_address"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "instructions"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)0);

        ///act
        auto result = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_HL_13_002: [ BLE_HL_Create shall return NULL if any of the underlying platform calls fail. ]*/
    TEST_FUNCTION(BLE_HL_Create_returns_NULL_when_VECTOR_create_fails)
    {
        ///arrange
        CBLEHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_number(IGNORED_PTR_ARG, "controller_index"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "device_mac_address"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "instructions"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(BLE_INSTRUCTION)))
            .IgnoreArgument(1)
            .SetFailReturn((VECTOR_HANDLE)NULL);

        ///act
        auto result = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_HL_13_007: [ BLE_HL_Create shall return NULL if each instruction is not an object. ]*/
    TEST_FUNCTION(BLE_HL_Create_returns_NULL_when_instruction_entry_is_NULL)
    {
        ///arrange
        CBLEHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(BLE_INSTRUCTION)))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_number(IGNORED_PTR_ARG, "controller_index"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "device_mac_address"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "instructions"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
            .IgnoreArgument(1)
            .SetFailReturn((JSON_Object*)NULL);

        ///act
        auto result = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_HL_13_008: [ BLE_HL_Create shall return NULL if a given instruction does not have a type property. ]*/
    TEST_FUNCTION(BLE_HL_Create_returns_NULL_when_instruction_type_is_NULL)
    {
        ///arrange
        CBLEHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(BLE_INSTRUCTION)))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        
        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        
        STRICT_EXPECTED_CALL(mocks, json_object_get_number(IGNORED_PTR_ARG, "controller_index"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "device_mac_address"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetFailReturn((const char*)NULL);
        STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "instructions"))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
            .IgnoreArgument(1);

        ///act
        auto result = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_HL_13_009: [ BLE_HL_Create shall return NULL if a given instruction does not have a characteristic_uuid property. ]*/
    TEST_FUNCTION(BLE_HL_Create_returns_NULL_when_instruction_characteristic_is_NULL)
    {
        ///arrange
        CBLEHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(BLE_INSTRUCTION)))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_object_get_number(IGNORED_PTR_ARG, "controller_index"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "device_mac_address"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "characteristic_uuid"))
            .IgnoreArgument(1)
            .SetFailReturn((const char*)NULL);
        STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "instructions"))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
            .IgnoreArgument(1);

        ///act
        auto result = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_HL_13_002: [ BLE_HL_Create shall return NULL if any of the underlying platform calls fail. ]*/
    TEST_FUNCTION(BLE_HL_Create_returns_NULL_when_STRING_construct_fails)
    {
        ///arrange
        CBLEHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(BLE_INSTRUCTION)))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((STRING_HANDLE)NULL);

        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_object_get_number(IGNORED_PTR_ARG, "controller_index"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "device_mac_address"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "characteristic_uuid"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "instructions"))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
            .IgnoreArgument(1);

        ///act
        auto result = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_HL_13_010: [ BLE_HL_Create shall return NULL if the interval_in_ms value for a read_periodic instruction isn't greater than zero. ]*/
    TEST_FUNCTION(BLE_HL_Create_returns_NULL_when_read_periodic_interval_is_zero)
    {
        ///arrange
        CBLEHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(BLE_INSTRUCTION)))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_object_get_number(IGNORED_PTR_ARG, "controller_index"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "device_mac_address"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"read_periodic");
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "characteristic_uuid"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_number(IGNORED_PTR_ARG, "interval_in_ms"))
            .IgnoreArgument(1)
            .SetReturn((uint32_t)0);
        STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "instructions"))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
            .IgnoreArgument(1);

        ///act
        auto result = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_HL_13_011: [ BLE_HL_Create shall return NULL if an instruction of type write_at_init or write_at_exit does not have a data property. ]*/
    TEST_FUNCTION(BLE_HL_Create_returns_NULL_when_write_instr_data_is_NULL)
    {
        ///arrange
        CBLEHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(BLE_INSTRUCTION)))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_object_get_number(IGNORED_PTR_ARG, "controller_index"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "device_mac_address"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "characteristic_uuid"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "data"))
            .IgnoreArgument(1)
            .SetFailReturn((const char*)NULL);
        STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "instructions"))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
            .IgnoreArgument(1);

        ///act
        auto result = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_HL_13_012: [ BLE_HL_Create shall return NULL if an instruction of type write_at_init or write_at_exit has a data property whose value does not decode successfully from base 64. ]*/
    TEST_FUNCTION(BLE_HL_Create_returns_NULL_when_write_instr_base64_decode_fails)
    {
        ///arrange
        CBLEHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(BLE_INSTRUCTION)))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Base64_Decoder(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((BUFFER_HANDLE)NULL);

        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_object_get_number(IGNORED_PTR_ARG, "controller_index"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "device_mac_address"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "characteristic_uuid"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "data"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "instructions"))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
            .IgnoreArgument(1);

        ///act
        auto result = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_HL_13_021: [ BLE_HL_Create shall return NULL if a given instruction's type property is unrecognized. ]*/
    TEST_FUNCTION(BLE_HL_Create_returns_NULL_when_instr_type_is_unknown)
    {
        ///arrange
        CBLEHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(BLE_INSTRUCTION)))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_object_get_number(IGNORED_PTR_ARG, "controller_index"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "device_mac_address"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"booyah_yah");
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "characteristic_uuid"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "instructions"))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
            .IgnoreArgument(1);

        ///act
        auto result = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_HL_13_002: [ BLE_HL_Create shall return NULL if any of the underlying platform calls fail. ]*/
    TEST_FUNCTION(BLE_HL_Create_returns_NULL_when_VECTOR_push_back_fails)
    {
        ///arrange
        CBLEHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(BLE_INSTRUCTION)))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .SetFailReturn((int)__LINE__);

        STRICT_EXPECTED_CALL(mocks, Base64_Decoder(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_object_get_number(IGNORED_PTR_ARG, "controller_index"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "device_mac_address"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "data"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "characteristic_uuid"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "instructions"))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
            .IgnoreArgument(1);

        ///act
        auto result = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_HL_13_002: [ BLE_HL_Create shall return NULL if any of the underlying platform calls fail. ]*/
    TEST_FUNCTION(BLE_HL_Create_returns_NULL_and_frees_first_instr_when_second_fails)
    {
        ///arrange
        CBLEHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(BLE_INSTRUCTION)))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        // cause the second VECTOR_push_back to fail
        STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .SetFailReturn((int)__LINE__);

        STRICT_EXPECTED_CALL(mocks, Base64_Decoder(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Base64_Decoder(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_object_get_number(IGNORED_PTR_ARG, "controller_index"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "device_mac_address"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_exit");
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "data"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "data"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "characteristic_uuid"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "characteristic_uuid"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "instructions"))
            .IgnoreArgument(1);

        // return 2 instructions
        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)2);
        STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 1))
            .IgnoreArgument(1);

        ///act
        auto result = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_HL_13_013: [ BLE_HL_Create shall return NULL if the device_mac_address property's value is not a well-formed MAC address. ]*/
    TEST_FUNCTION(BLE_HL_Create_returns_NULL_when_mac_address_is_invalid)
    {
        ///arrange
        CBLEHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(BLE_INSTRUCTION)))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(mocks, Base64_Decoder(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_object_get_number(IGNORED_PTR_ARG, "controller_index"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "device_mac_address"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"no mac address here");
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "data"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "characteristic_uuid"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "instructions"))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)1);
        STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
            .IgnoreArgument(1);

        ///act
        auto result = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_HL_13_013: [ BLE_HL_Create shall return NULL if the device_mac_address property's value is not a well-formed MAC address. ]*/
    TEST_FUNCTION(BLE_HL_Create_returns_NULL_when_mac_address_is_invalid2)
    {
        ///arrange
        CBLEHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(BLE_INSTRUCTION)))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(mocks, Base64_Decoder(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_object_get_number(IGNORED_PTR_ARG, "controller_index"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "device_mac_address"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"AA-BB-CC-DD-EE:FF");
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "data"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "characteristic_uuid"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "instructions"))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)1);
        STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
            .IgnoreArgument(1);

        ///act
        auto result = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_HL_13_002: [ BLE_HL_Create shall return NULL if any of the underlying platform calls fail. ]*/
    TEST_FUNCTION(BLE_HL_Create_returns_NULL_when_malloc_fails)
    {
        ///arrange
        CBLEHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((void*)NULL);

        STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(BLE_INSTRUCTION)))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(mocks, Base64_Decoder(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_object_get_number(IGNORED_PTR_ARG, "controller_index"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "device_mac_address"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "data"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "characteristic_uuid"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "instructions"))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)1);
        STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
            .IgnoreArgument(1);

        ///act
        auto result = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLE_HL_13_022: [ BLE_HL_Create shall return NULL if calling the underlying module's create function fails. ]*/
    TEST_FUNCTION(BLE_HL_Create_returns_NULL_when_low_level_module_create_fails)
    {
        ///arrange
        CBLEHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, MODULE_STATIC_GETAPIS(BLE_MODULE)());

        STRICT_EXPECTED_CALL(mocks, BLE_Create((MESSAGE_BUS_HANDLE)0x42, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .SetFailReturn((MODULE_HANDLE)NULL);

        STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(BLE_INSTRUCTION)))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(mocks, Base64_Decoder(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_object_get_number(IGNORED_PTR_ARG, "controller_index"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "device_mac_address"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "data"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "characteristic_uuid"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "instructions"))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)1);
        STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
            .IgnoreArgument(1);

        ///act
        auto result = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    TEST_FUNCTION(BLE_HL_Create_returns_NULL_when_macAddr_string_create_fails)
    {
        ///arrange
        CBLEHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(BLE_INSTRUCTION)))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(mocks, Base64_Decoder(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((STRING_HANDLE)NULL);

        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_object_get_number(IGNORED_PTR_ARG, "controller_index"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "device_mac_address"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "data"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "characteristic_uuid"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "instructions"))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)1);
        STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
            .IgnoreArgument(1);

        ///act
        auto result = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup

    }
    /*Tests_SRS_BLE_HL_13_014: [ BLE_HL_Create shall call the underlying module's 'create' function. ]*/
    /*Tests_SRS_BLE_HL_13_023: [ BLE_HL_Create shall return a non-NULL handle if calling the underlying module's create function succeeds. ]*/
    TEST_FUNCTION(BLE_HL_Create_succeeds)
    {
        ///arrange
        CBLEHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, MODULE_STATIC_GETAPIS(BLE_MODULE)());

        STRICT_EXPECTED_CALL(mocks, BLE_Create((MESSAGE_BUS_HANDLE)0x42, IGNORED_PTR_ARG))
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument(1);


        STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(BLE_INSTRUCTION)))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(mocks, Base64_Decoder(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_object_get_number(IGNORED_PTR_ARG, "controller_index"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "device_mac_address"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "data"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "characteristic_uuid"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "instructions"))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)1);
        STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
            .IgnoreArgument(1);

        ///act
        auto result = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NOT_NULL(result);

        ///cleanup
        BLE_HL_Destroy(result);
    }

    /*Tests_SRS_BLE_HL_13_017: [ BLE_HL_Destroy shall do nothing if module is NULL. ]*/
    TEST_FUNCTION(BLE_HL_Destroy_does_nothing_with_null_input)
    {
        ///arrange
        CBLEHLMocks mocks;

        ///act
        BLE_HL_Destroy(NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_BLE_HL_13_015: [ BLE_HL_Destroy shall destroy all used resources. ]*/
    TEST_FUNCTION(BLE_HL_Destroy_frees_resources)
    {
        ///arrange
        CBLEHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)1);

        auto module = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, MODULE_STATIC_GETAPIS(BLE_MODULE)());

        STRICT_EXPECTED_CALL(mocks, BLE_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_HL_Destroy(module);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_BLE_HL_13_018: [ BLE_HL_Receive shall forward the call to the underlying module. ]*/
    //Tests_SRS_BLE_HL_17_006: [ BLE_HL_Receive shall parse the message contents as a JSON object. ]
    //Tests_SRS_BLE_HL_17_016: [ BLE_HL_Receive shall set characteristic_uuid to the created STRING. ]
    //Tests_SRS_BLE_HL_17_014: [ BLE_HL_Receive shall parse the json object to fill in a new BLE_INSTRUCTION. ]
    //Tests_SRS_BLE_HL_17_018: [ BLE_HL_Receive shall call ConstMap_CloneWriteable on the message properties. ]
    //Tests_SRS_BLE_HL_17_020: [ BLE_HL_Receive shall call Map_AddOrUpdate with key of "source" and value of "BLE". ]
    //Tests_SRS_BLE_HL_17_023: [ BLE_HL_Receive shall create a new message by calling Message_Create with new map and BLE_INSTRUCTION as the buffer. ]
    //Tests_SRS_BLE_HL_17_025: [ BLE_HL_Receive shall free all resources created. ]
    TEST_FUNCTION(BLE_HL_Receive_forwards_call)
    {
        ///arrange
        CBLEHLMocks mocks;
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

        auto module = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
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

        STRICT_EXPECTED_CALL(mocks, MODULE_STATIC_GETAPIS(BLE_MODULE)());
        STRICT_EXPECTED_CALL(mocks, BLE_Receive(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_HL_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_HL_Destroy(module);
    }

    //Tests_SRS_BLE_HL_17_024: [ If creating new message fails, BLE_HL_Receive shall deallocate all resources and return. ]
    TEST_FUNCTION(BLE_HL_Receive_message_create_fails)
    {
        ///arrange
        CBLEHLMocks mocks;
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

        auto module = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)GW_IDMAP_MODULE);
        STRICT_EXPECTED_CALL(mocks, Message_GetContent(fakeMessage))
            .SetReturn((const CONSTBUFFER *)&messageBuffer);
        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
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
            .IgnoreArgument(1)
            .SetFailReturn((MESSAGE_HANDLE)NULL);


        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_HL_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_HL_Destroy(module);
    }

    TEST_FUNCTION(BLE_HL_Receive_map_update_fails)
    {
        ///arrange
        CBLEHLMocks mocks;
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

        auto module = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)GW_IDMAP_MODULE);
        STRICT_EXPECTED_CALL(mocks, Message_GetContent(fakeMessage))
            .SetReturn((const CONSTBUFFER *)&messageBuffer);
        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
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
            .IgnoreArgument(1)
            .SetFailReturn(MAP_ERROR);

        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_HL_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_HL_Destroy(module);
    }

    //Tests_SRS_BLE_HL_17_019: [ If ConstMap_CloneWriteable fails, BLE_HL_Receive shall return. ]
    TEST_FUNCTION(BLE_HL_Receive_constmap_clonewriteable_fails)
    {
        ///arrange
        CBLEHLMocks mocks;
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

        auto module = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)GW_IDMAP_MODULE);
        STRICT_EXPECTED_CALL(mocks, Message_GetContent(fakeMessage))
            .SetReturn((const CONSTBUFFER *)&messageBuffer);
        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
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
            .SetFailReturn((MAP_HANDLE)NULL);

        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_HL_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_HL_Destroy(module);
    }

    //Tests_SRS_BLE_HL_17_008: [ BLE_HL_Receive shall return if the JSON object does not contain the following fields: "type", "characteristic_uuid", and "data". ]
    //Tests_SRS_BLE_HL_17_026: [ If the json object does not parse, BLE_HL_Receive shall return. ]
    TEST_FUNCTION(BLE_HL_Receive_parse_instruction_fails)
    {
        ///arrange
        CBLEHLMocks mocks;
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

        auto module = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)GW_IDMAP_MODULE);
        STRICT_EXPECTED_CALL(mocks, Message_GetContent(fakeMessage))
            .SetReturn((const CONSTBUFFER *)&messageBuffer);
        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
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
            .IgnoreArgument(1)
            .SetFailReturn((BUFFER_HANDLE)NULL);

        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_HL_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_HL_Destroy(module);
    }

    //Tests_SRS_BLE_HL_17_012: [ BLE_HL_Receive shall create a STRING_HANDLE from the characteristic_uuid data field. ]
    //Tests_SRS_BLE_HL_17_013: [ If the string creation fails, BLE_HL_Receive shall return. ]
    TEST_FUNCTION(BLE_HL_Receive_uuid_construction_fails)
    {
        ///arrange
        CBLEHLMocks mocks;
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

        auto module = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)GW_IDMAP_MODULE);
        STRICT_EXPECTED_CALL(mocks, Message_GetContent(fakeMessage))
            .SetReturn((const CONSTBUFFER *)&messageBuffer);
        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
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

        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_HL_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_HL_Destroy(module);
    }

    //Tests_SRS_BLE_HL_17_008: [ BLE_HL_Receive shall return if the JSON object does not contain the following fields: "type", "characteristic_uuid", and "data". ]
    TEST_FUNCTION(BLE_HL_Receive_uuid_not_found)
    {
        ///arrange
        CBLEHLMocks mocks;
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

        auto module = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)GW_IDMAP_MODULE);
        STRICT_EXPECTED_CALL(mocks, Message_GetContent(fakeMessage))
            .SetReturn((const CONSTBUFFER *)&messageBuffer);
        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn("write_once");
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "characteristic_uuid"))
            .IgnoreArgument(1)
            .SetReturn((const char*)NULL);

        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_HL_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_HL_Destroy(module);
    }

    //Tests_SRS_BLE_HL_17_008: [ BLE_HL_Receive shall return if the JSON object does not contain the following fields: "type", "characteristic_uuid", and "data". ]
    TEST_FUNCTION(BLE_HL_Receive_type_returns_null)
    {
        ///arrange
        CBLEHLMocks mocks;
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

        auto module = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)GW_IDMAP_MODULE);
        STRICT_EXPECTED_CALL(mocks, Message_GetContent(fakeMessage))
            .SetReturn((const CONSTBUFFER *)&messageBuffer);
        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*) NULL);

        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_HL_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_HL_Destroy(module);
    }

    //Tests_SRS_BLE_HL_17_007: [ If the message contents do not parse, then BLE_HL_Receive shall return. ]
    TEST_FUNCTION(BLE_HL_Receive_json_value_get_object_fails)
    {
        ///arrange
        CBLEHLMocks mocks;
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

        auto module = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)GW_IDMAP_MODULE);
        STRICT_EXPECTED_CALL(mocks, Message_GetContent(fakeMessage))
            .SetReturn((const CONSTBUFFER *)&messageBuffer);
        STRICT_EXPECTED_CALL(mocks, json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((JSON_Object*)NULL);

        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_HL_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_HL_Destroy(module);
    }

    //Tests_SRS_BLE_HL_17_007: [ If the message contents do not parse, then BLE_HL_Receive shall return. ]
    TEST_FUNCTION(BLE_HL_Receive_json_parse_string_fails)
    {
        ///arrange
        CBLEHLMocks mocks;
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

        auto module = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
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
        BLE_HL_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_HL_Destroy(module);
    }

    //Tests_SRS_BLE_HL_17_007: [ If the message contents do not parse, then BLE_HL_Receive shall return. ]
    TEST_FUNCTION(BLE_HL_Receive_message_no_content_fails)
    {
        ///arrange
        CBLEHLMocks mocks;
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

        auto module = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)GW_IDMAP_MODULE);
        STRICT_EXPECTED_CALL(mocks, Message_GetContent(fakeMessage))
            .SetReturn((const CONSTBUFFER *)NULL);

        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_HL_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_HL_Destroy(module);
    }

    //Tests_SRS_BLE_HL_17_005: [ If the source of the message properties is not "mapping", then this function shall return. ]
    TEST_FUNCTION(BLE_HL_Receive_message_source_not_mapping)
    {
        ///arrange
        CBLEHLMocks mocks;
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

        auto module = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:CC:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"BLE");


        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_HL_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_HL_Destroy(module);
    }

    //Tests_SRS_BLE_HL_17_003: [ If macAddress of the message property does not match this module's MAC address, then this function shall return. ]
    TEST_FUNCTION(BLE_HL_Receive_message_wrong_mac_address)
    {
        ///arrange
        CBLEHLMocks mocks;
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

        auto module = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage));
        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
            .IgnoreArgument(1)
            .SetReturn((const char *)"AA:BB:DD:DD:EE:FF");
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLE_HL_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_HL_Destroy(module);
    }

    //Tests_SRS_BLE_HL_17_002: [ If messageHandle properties does not contain "macAddress" property, then this function shall return. ]
    //Tests_SRS_BLE_HL_17_004: [ If messageHandle properties does not contain "source" property, then this function shall return. ]
    TEST_FUNCTION(BLE_HL_Receive_message_no_properties)
    {
        ///arrange
        CBLEHLMocks mocks;
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

        auto module = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        MESSAGE_HANDLE fakeMessage = (MESSAGE_HANDLE)0x42;
        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(fakeMessage))
            .SetFailReturn((CONSTMAP_HANDLE)NULL);

        ///act
        BLE_HL_Receive(module, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_HL_Destroy(module);
    }

    /*Tests_SRS_BLE_HL_13_016: [ BLE_HL_Receive shall do nothing if module is NULL. ]*/
    TEST_FUNCTION(BLE_HL_Receive_does_nothing_with_null_input1)
    {
        ///arrange
        CBLEHLMocks mocks;

        ///act
        BLE_HL_Receive(NULL, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    //Tests_SRS_BLE_HL_17_001: [ BLE_HL_Receive shall do nothing if message_handle is NULL. ]
    TEST_FUNCTION(BLE_HL_Receive_does_nothing_with_null_input2)
    {
        ///arrange
        CBLEHLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "type"))
            .IgnoreArgument(1)
            .SetReturn((const char*)"write_at_init");
        STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn((size_t)1);

        auto module = BLE_HL_Create((MESSAGE_BUS_HANDLE)0x42, (const void*)FAKE_CONFIG);
        mocks.ResetAllCalls();

        ///act
        BLE_HL_Receive(module, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLE_HL_Destroy(module);
    }

    /*Tests_SRS_BLE_HL_13_019: [Module_GetAPIS shall return a non - NULL pointer to a structure of type MODULE_APIS that has all fields initialized to non - NULL values.]*/
    TEST_FUNCTION(Module_GetAPIS_returns_non_NULL)
    {
        ///arrage
        CBLEHLMocks mocks;

        ///act
        const MODULE_APIS* apis = Module_GetAPIS();

        ///assert
        ASSERT_IS_NOT_NULL(apis);
        ASSERT_IS_NOT_NULL(apis->Module_Destroy);
        ASSERT_IS_NOT_NULL(apis->Module_Create);
        ASSERT_IS_NOT_NULL(apis->Module_Receive);

        ///cleanup
    }

END_TEST_SUITE(ble_hl_unittests)