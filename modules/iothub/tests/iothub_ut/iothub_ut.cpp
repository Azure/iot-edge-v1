// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#include <cstddef>
#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"

#include "module.h"
#include "module_access.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/vector_types_internal.h"
#include "azure_c_shared_utility/strings.h"
#include "iothubtransport.h"
#include "iothub_message.h"
#include "broker.h"
#include "message.h"
#include "messageproperties.h"

#include <parson.h>

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

#include "vector.c"
#include "strings.c"
};

#include "iothub.h"
#include "iothub_client.h"
#include "message.h"
#include "azure_c_shared_utility/constmap.h"
#include "azure_c_shared_utility/map.h"

DEFINE_MICROMOCK_ENUM_TO_STRING(IOTHUBMESSAGE_DISPOSITION_RESULT, IOTHUBMESSAGE_DISPOSITION_RESULT_VALUES);

static MICROMOCK_MUTEX_HANDLE g_testByTest;
static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

static size_t currentVECTOR_push_back_call;
static size_t whenShallVECTOR_push_back_fail;

/*different STRING constructors*/
static size_t currentSTRING_construct_call;
static size_t whenShallSTRING_construct_fail;

static size_t currentIoTHubMessage_CreateFromByteArray_call;
static size_t whenShallIoTHubMessage_CreateFromByteArray_fail;

static size_t currentIoTHubClient_Create_call;
static size_t whenShallIoTHubClient_Create_fail;

static IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC IotHub_Receive_message_callback_function;
static void * IotHub_Receive_message_userContext;
static const char * IotHub_Receive_message_content;
static size_t IotHub_Receive_message_size;

typedef struct PERSONALITY_TAG
{
    STRING_HANDLE deviceName;
    STRING_HANDLE deviceKey;
    IOTHUB_CLIENT_HANDLE iothubHandle;
    BROKER_HANDLE broker;
    MODULE_HANDLE module;
}PERSONALITY;

typedef PERSONALITY* PERSONALITY_PTR;

typedef struct IOTHUB_HANDLE_DATA_TAG
{
    VECTOR_HANDLE personalities; /*holds PERSONALITYs*/
    STRING_HANDLE IoTHubName;
    STRING_HANDLE IoTHubSuffix;
    IOTHUB_CLIENT_TRANSPORT_PROVIDER transportProvider;
    TRANSPORT_HANDLE transportHandle;
    BROKER_HANDLE broker;
}IOTHUB_HANDLE_DATA;

// NOTE Each of these dummy transport provider functions have to do something a
// little different (e.g. return a different ptr value), or else the optimizer
// will collapse them into one function in Release builds.
extern "C" const TRANSPORT_PROVIDER* HTTP_Protocol(void) { return (TRANSPORT_PROVIDER*)0x1; }
extern "C" const TRANSPORT_PROVIDER* AMQP_Protocol(void) { return (TRANSPORT_PROVIDER*)0x2; }
extern "C" const TRANSPORT_PROVIDER* MQTT_Protocol(void) { return (TRANSPORT_PROVIDER*)0x3; }
extern "C" const TRANSPORT_PROVIDER* ABCD_Protocol(void) { return (TRANSPORT_PROVIDER*)0x4; }

#define BROKER_HANDLE_VALID ((BROKER_HANDLE)(1))

#define IOTHUB_CLIENT_HANDLE_VALID ((IOTHUB_CLIENT_HANDLE)(2))
#define IOTHUB_MESSAGE_HANDLE_VALID ((IOTHUB_MESSAGE_HANDLE)7)

#define MESSAGE_HANDLE_VALID ((MESSAGE_HANDLE)(3))

#define MESSAGE_HANDLE_WITHOUT_SOURCE ((MESSAGE_HANDLE)(4))
#define CONSTMAP_HANDLE_WITHOUT_SOURCE ((CONSTMAP_HANDLE)(4))


#define MESSAGE_HANDLE_WITH_SOURCE_NOT_SET_TO_MAPPING ((MESSAGE_HANDLE)(5))
#define CONSTMAP_HANDLE_WITH_SOURCE_NOT_SET_TO_MAPPING ((CONSTMAP_HANDLE)(5))

#define MESSAGE_HANDLE_VALID_1 ((MESSAGE_HANDLE)(6))
#define CONSTMAP_HANDLE_VALID_1 ((CONSTMAP_HANDLE)(6))
static const CONSTBUFFER CONSTBUFFER_VALID_CONTENT1 =
{
    (unsigned char*)"5",
    1
};
static const CONSTBUFFER* CONSTBUFFER_VALID_1 = &CONSTBUFFER_VALID_CONTENT1;
static IOTHUB_CLIENT_HANDLE IOTHUB_CLIENT_HANDLE_VALID_1 = ((IOTHUB_CLIENT_HANDLE)(6));
static IOTHUB_MESSAGE_HANDLE IOTHUB_MESSAGE_HANDLE_VALID_1 = ((IOTHUB_MESSAGE_HANDLE)(6));
static MAP_HANDLE MAP_HANDLE_VALID_1 = ((MAP_HANDLE)(6));
static const char* CONSTMAP_KEYS_VALID_1[4] = {"source", "deviceName", "deviceKey", "somethingExtra"};
static const char* CONSTMAP_VALUES_VALID_1[4] = { "mapping", "firstDevice", "cheiaDeLaPoartaVerde", "blue" };

#define MESSAGE_HANDLE_VALID_2 ((MESSAGE_HANDLE)(7))
#define CONSTMAP_HANDLE_VALID_2 ((CONSTMAP_HANDLE)(7))
static const CONSTBUFFER CONSTBUFFER_VALID_CONTENT2 =
{
    NULL,
    0
};
static const CONSTBUFFER* CONSTBUFFER_VALID_2 = &CONSTBUFFER_VALID_CONTENT2;
static IOTHUB_CLIENT_HANDLE IOTHUB_CLIENT_HANDLE_VALID_2 = ((IOTHUB_CLIENT_HANDLE)(7));
static IOTHUB_MESSAGE_HANDLE IOTHUB_MESSAGE_HANDLE_VALID_2 = ((IOTHUB_MESSAGE_HANDLE)(7));
static MAP_HANDLE MAP_HANDLE_VALID_2 = ((MAP_HANDLE)(7));
static const char* CONSTMAP_KEYS_VALID_2[3] = { "source", "deviceName", "deviceKey"};
static const char* CONSTMAP_VALUES_VALID_2[3] = { "mapping", "secondDevice", "red"};

/*these are simple cached variables*/
static pfModule_ParseConfigurationFromJson Module_ParseConfigurationFromJson = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_FreeConfiguration Module_FreeConfiguration = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Create Module_Create = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Destroy Module_Destroy = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Receive Module_Receive = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/

const char name[] = "name";
const char suffix[] = "suffix";

IOTHUB_CONFIG CreateConfigWithTransport(IOTHUB_CLIENT_TRANSPORT_PROVIDER transport)
{
    char* name_ = (char*)malloc(sizeof(name));
    char* suffix_ = (char*)malloc(sizeof(suffix));
    strcpy(name_, name);
    strcpy(suffix_, suffix);

    return IOTHUB_CONFIG { name_, suffix_, transport };
}

IOTHUB_CONFIG CreateConfig() { return CreateConfigWithTransport(HTTP_Protocol); }

class AutoConfig
{
    IOTHUB_CONFIG config_;
    AutoConfig(const AutoConfig&);
    AutoConfig& operator=(const AutoConfig&);

public:
    AutoConfig() : config_(CreateConfig()) {}

    AutoConfig(IOTHUB_CLIENT_TRANSPORT_PROVIDER transport) :
        config_(CreateConfigWithTransport(transport))
    {}

    ~AutoConfig()
    {
        free((void*)config_.IoTHubName);
        free((void*)config_.IoTHubSuffix);
    }

    operator IOTHUB_CONFIG*() { return &config_; }
};


TYPED_MOCK_CLASS(IotHubMocks, CGlobalMock)
{
public:

    // memory
    MOCK_STATIC_METHOD_1(, void*, gballoc_malloc, size_t, size)
        void* result2 = BASEIMPLEMENTATION::gballoc_malloc(size);
    MOCK_METHOD_END(void*, result2);

    MOCK_STATIC_METHOD_1(, void, gballoc_free, void*, ptr)
        BASEIMPLEMENTATION::gballoc_free(ptr);
    MOCK_VOID_METHOD_END()

    // ConstMap mocks
    MOCK_STATIC_METHOD_1(, CONSTMAP_HANDLE, ConstMap_Clone, CONSTMAP_HANDLE, handle)
    MOCK_METHOD_END(CONSTMAP_HANDLE, handle)

    MOCK_STATIC_METHOD_1(, void, ConstMap_Destroy, CONSTMAP_HANDLE, map)
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_4(, CONSTMAP_RESULT, ConstMap_GetInternals, CONSTMAP_HANDLE, handle, const char*const**, keys, const char*const**, values, size_t*, count)
        if (handle == CONSTMAP_HANDLE_VALID_1)
        {
            *keys=CONSTMAP_KEYS_VALID_1;
            *values = CONSTMAP_VALUES_VALID_1;
            *count = 4;
        }
        else if (handle == CONSTMAP_HANDLE_VALID_2)
        {
            *keys = CONSTMAP_KEYS_VALID_2;
            *values = CONSTMAP_VALUES_VALID_2;
            *count = 3;
        }
        else
        {
            *count = 0;
        }
    MOCK_METHOD_END(CONSTMAP_RESULT, CONSTMAP_OK)

    MOCK_STATIC_METHOD_1(, VECTOR_HANDLE, VECTOR_create, size_t, elementSize)
        VECTOR_HANDLE result2 = BASEIMPLEMENTATION::VECTOR_create(elementSize);
    MOCK_METHOD_END(VECTOR_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, void, VECTOR_destroy, VECTOR_HANDLE, handle)
        BASEIMPLEMENTATION::VECTOR_destroy(handle);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_3(, int, VECTOR_push_back, VECTOR_HANDLE, handle, const void*, elements, size_t, numElements)
        int result2;
        currentVECTOR_push_back_call++;
        if (currentVECTOR_push_back_call == whenShallVECTOR_push_back_fail)
        {
            result2 = __LINE__;
        }
        else
        {
            result2 = BASEIMPLEMENTATION::VECTOR_push_back(handle, elements, numElements);
        }
    MOCK_METHOD_END(int, result2)

    MOCK_STATIC_METHOD_2(, void*, VECTOR_element, VECTOR_HANDLE, handle, size_t, index)
    MOCK_METHOD_END(void*, BASEIMPLEMENTATION::VECTOR_element(handle, index))

    MOCK_STATIC_METHOD_3(, void*, VECTOR_find_if, VECTOR_HANDLE, handle, PREDICATE_FUNCTION, pred, const void*, value)
    MOCK_METHOD_END(void*, BASEIMPLEMENTATION::VECTOR_find_if(handle, pred, value))

    MOCK_STATIC_METHOD_1(, size_t, VECTOR_size, VECTOR_HANDLE, handle)
    MOCK_METHOD_END(size_t, BASEIMPLEMENTATION::VECTOR_size(handle))

    MOCK_STATIC_METHOD_1(, void*, VECTOR_back, VECTOR_HANDLE, handle)
    MOCK_METHOD_END(void*, BASEIMPLEMENTATION::VECTOR_back(handle))

    MOCK_STATIC_METHOD_1(, void, STRING_delete, STRING_HANDLE, s)
        BASEIMPLEMENTATION::STRING_delete(s);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_0(, STRING_HANDLE, STRING_new)
        STRING_HANDLE result2 = BASEIMPLEMENTATION::STRING_new();
    MOCK_METHOD_END(STRING_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, STRING_HANDLE, STRING_clone, STRING_HANDLE, handle)
        STRING_HANDLE result2 = BASEIMPLEMENTATION::STRING_clone(handle);
    MOCK_METHOD_END(STRING_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, STRING_HANDLE, STRING_construct, const char*, source)
        STRING_HANDLE result2;
        currentSTRING_construct_call++;
        if (whenShallSTRING_construct_fail > 0)
        {
            if (currentSTRING_construct_call == whenShallSTRING_construct_fail)
            {
                result2 = (STRING_HANDLE)NULL;
            }
            else
            {
                result2 = BASEIMPLEMENTATION::STRING_construct(source);
            }
        }
        else
        {
            result2 = BASEIMPLEMENTATION::STRING_construct(source);
        }
    MOCK_METHOD_END(STRING_HANDLE, result2)

    MOCK_STATIC_METHOD_2(, int, STRING_concat, STRING_HANDLE, s1, const char*, s2)
    MOCK_METHOD_END(int, BASEIMPLEMENTATION::STRING_concat(s1, s2));

    MOCK_STATIC_METHOD_2(, int, STRING_concat_with_STRING, STRING_HANDLE, s1, STRING_HANDLE, s2)
    MOCK_METHOD_END(int, BASEIMPLEMENTATION::STRING_concat_with_STRING(s1, s2));

    MOCK_STATIC_METHOD_1(, int, STRING_empty, STRING_HANDLE, s)
    MOCK_METHOD_END(int, BASEIMPLEMENTATION::STRING_empty(s));

    MOCK_STATIC_METHOD_1(, const char*, STRING_c_str, STRING_HANDLE, s)
        MOCK_METHOD_END(const char*, BASEIMPLEMENTATION::STRING_c_str(s))


    MOCK_STATIC_METHOD_2(, IOTHUB_CLIENT_HANDLE, IoTHubClient_CreateWithTransport, TRANSPORT_HANDLE, transport, const IOTHUB_CLIENT_CONFIG*, config)
        IOTHUB_CLIENT_HANDLE result2;
        currentIoTHubClient_Create_call++;
        if (whenShallIoTHubClient_Create_fail == currentIoTHubClient_Create_call)
        {
            result2 = NULL;
        }
        else
        {
            result2 = (IOTHUB_CLIENT_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1);
        }
    MOCK_METHOD_END(IOTHUB_CLIENT_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, IOTHUB_CLIENT_HANDLE, IoTHubClient_Create, const IOTHUB_CLIENT_CONFIG*, config)
        IOTHUB_CLIENT_HANDLE result2 = (IOTHUB_CLIENT_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1);
    MOCK_METHOD_END(IOTHUB_CLIENT_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, void, IoTHubClient_Destroy, IOTHUB_CLIENT_HANDLE, iotHubClientHandle)
        BASEIMPLEMENTATION::gballoc_free(iotHubClientHandle);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, CONSTMAP_HANDLE, Message_GetProperties, MESSAGE_HANDLE, message)
        CONSTMAP_HANDLE result2;
        if (message == MESSAGE_HANDLE_WITHOUT_SOURCE)
        {
            result2 = CONSTMAP_HANDLE_WITHOUT_SOURCE;
        }
        else if (message == MESSAGE_HANDLE_WITH_SOURCE_NOT_SET_TO_MAPPING)
        {
            result2 = CONSTMAP_HANDLE_WITH_SOURCE_NOT_SET_TO_MAPPING;
        }
        else if (message == MESSAGE_HANDLE_VALID_1)
        {
            result2 = CONSTMAP_HANDLE_VALID_1;
        }
        else if (message == MESSAGE_HANDLE_VALID_2)
        {
            result2 = CONSTMAP_HANDLE_VALID_2;
        }
        else
        {
            result2 = NULL;
        }
    MOCK_METHOD_END(CONSTMAP_HANDLE, result2)

    MOCK_STATIC_METHOD_2(, const char*, ConstMap_GetValue, CONSTMAP_HANDLE, handle, const char*, key)
        const char* result2;
        if (handle == CONSTMAP_HANDLE_WITHOUT_SOURCE)
        {
            result2 = NULL;
        }
        else if (handle == CONSTMAP_HANDLE_WITH_SOURCE_NOT_SET_TO_MAPPING)
        {
            if (strcmp(key, "source") == 0)
            {
                result2 = "notMapping";
            }
            else
            {
                result2 = NULL;
            }
        }
        else if (handle == CONSTMAP_HANDLE_VALID_1)
        {
            size_t i;
            result2 = NULL;
            for (i = 0; i < sizeof(CONSTMAP_KEYS_VALID_1)/sizeof(CONSTMAP_KEYS_VALID_1[0]); i++)
            {
                if (strcmp(CONSTMAP_KEYS_VALID_1[i], key) == 0)
                {
                    result2 = CONSTMAP_VALUES_VALID_1[i];
                    break;
                }
            }
        }
        else if (handle == CONSTMAP_HANDLE_VALID_2)
        {
            size_t i;
            result2 = NULL;
            for (i = 0; i < sizeof(CONSTMAP_KEYS_VALID_2)/sizeof(CONSTMAP_KEYS_VALID_2[0]); i++)
            {
                if (strcmp(CONSTMAP_KEYS_VALID_2[i], key) == 0)
                {
                    result2 = CONSTMAP_VALUES_VALID_2[i];
                    break;
                }
            }
        }
        else
        {
            result2 = NULL;
        }
    MOCK_METHOD_END(const char*, result2)

    MOCK_STATIC_METHOD_3(, MAP_RESULT, Map_AddOrUpdate, MAP_HANDLE, handle, const char*, key, const char*, value)
    MOCK_METHOD_END(MAP_RESULT, MAP_OK)

    MOCK_STATIC_METHOD_3(, MAP_RESULT, Map_Add, MAP_HANDLE, handle, const char*, key, const char*, value)
    MOCK_METHOD_END(MAP_RESULT, MAP_OK)

    MOCK_STATIC_METHOD_4(, IOTHUB_CLIENT_RESULT, IoTHubClient_SendEventAsync, IOTHUB_CLIENT_HANDLE, iotHubClientHandle, IOTHUB_MESSAGE_HANDLE, eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK, eventConfirmationCallback, void*, userContextCallback)
    MOCK_METHOD_END(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK)

    MOCK_STATIC_METHOD_3(, IOTHUB_CLIENT_RESULT, IoTHubClient_SetMessageCallback, IOTHUB_CLIENT_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC, messageCallback, void*, userContextCallback)
        IotHub_Receive_message_callback_function = messageCallback;
        IotHub_Receive_message_userContext = userContextCallback;
        MOCK_METHOD_END(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK)


    //// GW Message
    MOCK_STATIC_METHOD_1(, MESSAGE_HANDLE, Message_Create, const MESSAGE_CONFIG*, cfg)
    MOCK_METHOD_END(MESSAGE_HANDLE, (MESSAGE_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1))

    MOCK_STATIC_METHOD_1(, void, Message_Destroy, MESSAGE_HANDLE, message)
        BASEIMPLEMENTATION::gballoc_free(message);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, const CONSTBUFFER *, Message_GetContent, MESSAGE_HANDLE, message)
        const CONSTBUFFER * result2;
        if (message == MESSAGE_HANDLE_VALID_1)
        {
            result2 = CONSTBUFFER_VALID_1;
        }
        else if (message == MESSAGE_HANDLE_VALID_2)
        {
            result2 = CONSTBUFFER_VALID_2;
        }
        else
        {
            result2 = NULL;
        }
    MOCK_METHOD_END(const CONSTBUFFER *, result2)

    // IoTHubMessage

    MOCK_STATIC_METHOD_1(, void, IoTHubMessage_Destroy, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle)
        BASEIMPLEMENTATION::gballoc_free(iotHubMessageHandle);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, MAP_HANDLE, IoTHubMessage_Properties, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle)
        MAP_HANDLE result2;
        if (iotHubMessageHandle == IOTHUB_MESSAGE_HANDLE_VALID_1)
        {
            result2 = MAP_HANDLE_VALID_1;
        }
        else if (iotHubMessageHandle == IOTHUB_MESSAGE_HANDLE_VALID_2)
        {
            result2 = MAP_HANDLE_VALID_2;
        }
        else
        {
            result2 = NULL;
        }
    MOCK_METHOD_END(MAP_HANDLE, result2)

    MOCK_STATIC_METHOD_2(, IOTHUB_MESSAGE_HANDLE, IoTHubMessage_CreateFromByteArray, const unsigned char*, byteArray, size_t, size)
        IOTHUB_MESSAGE_HANDLE result2;
        currentIoTHubMessage_CreateFromByteArray_call ++;

        if (whenShallIoTHubMessage_CreateFromByteArray_fail == currentIoTHubMessage_CreateFromByteArray_call)
        {
            result2 = NULL;
        }
        else
        {
            result2 = (IOTHUB_MESSAGE_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1);
        }

    MOCK_METHOD_END(IOTHUB_MESSAGE_HANDLE, result2);

    MOCK_STATIC_METHOD_3(, IOTHUB_MESSAGE_RESULT, IoTHubMessage_GetByteArray, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle, const unsigned char**, buffer, size_t*, size)
        *buffer = (const unsigned char*)IotHub_Receive_message_content;
        *size = IotHub_Receive_message_size;
    MOCK_METHOD_END(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_OK)

    MOCK_STATIC_METHOD_1(, const char*, IoTHubMessage_GetString, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle)
    MOCK_METHOD_END(const char*, IotHub_Receive_message_content)

    MOCK_STATIC_METHOD_1(, IOTHUBMESSAGE_CONTENT_TYPE, IoTHubMessage_GetContentType, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle)
    MOCK_METHOD_END(IOTHUBMESSAGE_CONTENT_TYPE, IOTHUBMESSAGE_BYTEARRAY)


    // transport mocks

    MOCK_STATIC_METHOD_3(,TRANSPORT_HANDLE, IoTHubTransport_Create, IOTHUB_CLIENT_TRANSPORT_PROVIDER, protocol, const char*, iotHubName, const char*, iotHubSuffix)
        TRANSPORT_HANDLE result2 = (TRANSPORT_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1);
    MOCK_METHOD_END(TRANSPORT_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, void, IoTHubTransport_Destroy, TRANSPORT_HANDLE, transportHlHandle)
        BASEIMPLEMENTATION::gballoc_free(transportHlHandle);
    MOCK_VOID_METHOD_END()

    // broker
    MOCK_STATIC_METHOD_3(, BROKER_RESULT, Broker_Publish, BROKER_HANDLE, broker, MODULE_HANDLE, source, MESSAGE_HANDLE, message)
    MOCK_METHOD_END(BROKER_RESULT, BROKER_OK)

    /* Parson Mocks */
    MOCK_STATIC_METHOD_1(, JSON_Value*, json_parse_string, const char *, parseString)
        JSON_Value* value = NULL;
        if (parseString != NULL)
        {
            value = (JSON_Value*)malloc(1);
        }
    MOCK_METHOD_END(JSON_Value*, value);

    MOCK_STATIC_METHOD_1(, JSON_Object*, json_value_get_object, const JSON_Value*, value)
        JSON_Object* object = NULL;
        if (value != NULL)
        {
            object = (JSON_Object*)0x42;
        }
    MOCK_METHOD_END(JSON_Object*, object);

    MOCK_STATIC_METHOD_2(, const char*, json_object_get_string, const JSON_Object*, object, const char*, name)
        const char * result2;
        if (strcmp(name, "IoTHubSuffix") == 0)
        {
            result2 = "suffix.name";
        }
        else if (strcmp(name, "IoTHubName") == 0)
        {
            result2 = "aHubName";
        }
        else if (strcmp(name, "Transport") == 0)
        {
            result2 = "abc";
        }
        else
        {
            result2 = NULL;
        }
    MOCK_METHOD_END(const char*, result2);

    MOCK_STATIC_METHOD_1(, void, json_value_free, JSON_Value*, value)
        free(value);
    MOCK_VOID_METHOD_END();
};

DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , void, gballoc_free, void*, ptr);
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , CONSTMAP_HANDLE, ConstMap_Clone, CONSTMAP_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , void, ConstMap_Destroy, CONSTMAP_HANDLE, map);
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , VECTOR_HANDLE, VECTOR_create, size_t, elementSize)
DECLARE_GLOBAL_MOCK_METHOD_3(IotHubMocks, , int, VECTOR_push_back, VECTOR_HANDLE, handle, const void*, elements, size_t, numElements)
DECLARE_GLOBAL_MOCK_METHOD_2(IotHubMocks, , void*, VECTOR_element, VECTOR_HANDLE, handle, size_t, index)
DECLARE_GLOBAL_MOCK_METHOD_3(IotHubMocks, , void*, VECTOR_find_if, VECTOR_HANDLE, handle, PREDICATE_FUNCTION, pred, const void*, value)
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , size_t, VECTOR_size, VECTOR_HANDLE, handle)
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , void, VECTOR_destroy, VECTOR_HANDLE, handle)
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , void, STRING_delete, STRING_HANDLE, s);
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , STRING_HANDLE, STRING_construct, const char*, source);
DECLARE_GLOBAL_MOCK_METHOD_2(IotHubMocks, , int, STRING_concat, STRING_HANDLE, s1, const char*, s2);
DECLARE_GLOBAL_MOCK_METHOD_2(IotHubMocks, , int, STRING_concat_with_STRING, STRING_HANDLE, s1, STRING_HANDLE, s2);
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , int, STRING_empty, STRING_HANDLE, s1);
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , const char*, STRING_c_str, STRING_HANDLE, s);
DECLARE_GLOBAL_MOCK_METHOD_2(IotHubMocks, , IOTHUB_CLIENT_HANDLE, IoTHubClient_CreateWithTransport, TRANSPORT_HANDLE, transport, const IOTHUB_CLIENT_CONFIG*, config)
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , IOTHUB_CLIENT_HANDLE, IoTHubClient_Create, const IOTHUB_CLIENT_CONFIG*, config)
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , void, IoTHubClient_Destroy, IOTHUB_CLIENT_HANDLE, iotHubClientHandle)
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , CONSTMAP_HANDLE, Message_GetProperties, MESSAGE_HANDLE, message)
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , MESSAGE_HANDLE, Message_Create, const MESSAGE_CONFIG*, cfg)
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , void, Message_Destroy, MESSAGE_HANDLE, message)
DECLARE_GLOBAL_MOCK_METHOD_2(IotHubMocks, , const char*, ConstMap_GetValue, CONSTMAP_HANDLE, handle, const char*, key)
DECLARE_GLOBAL_MOCK_METHOD_3(IotHubMocks, , MAP_RESULT, Map_AddOrUpdate, MAP_HANDLE, handle, const char*, key, const char*, value);
DECLARE_GLOBAL_MOCK_METHOD_3(IotHubMocks, , MAP_RESULT, Map_Add, MAP_HANDLE, handle, const char*, key, const char*, value);
DECLARE_GLOBAL_MOCK_METHOD_4(IotHubMocks, , CONSTMAP_RESULT, ConstMap_GetInternals, CONSTMAP_HANDLE, handle, const char*const**, keys, const char*const**, values, size_t*, count)
DECLARE_GLOBAL_MOCK_METHOD_4(IotHubMocks, ,IOTHUB_CLIENT_RESULT, IoTHubClient_SendEventAsync, IOTHUB_CLIENT_HANDLE, iotHubClientHandle, IOTHUB_MESSAGE_HANDLE, eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK, eventConfirmationCallback, void*, userContextCallback)
DECLARE_GLOBAL_MOCK_METHOD_3(IotHubMocks, , IOTHUB_CLIENT_RESULT, IoTHubClient_SetMessageCallback, IOTHUB_CLIENT_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC, messageCallback, void*, userContextCallback)
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , const CONSTBUFFER *, Message_GetContent, MESSAGE_HANDLE, message)
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , void, IoTHubMessage_Destroy, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle)
DECLARE_GLOBAL_MOCK_METHOD_2(IotHubMocks, , IOTHUB_MESSAGE_HANDLE, IoTHubMessage_CreateFromByteArray, const unsigned char*, byteArray, size_t, size)
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , MAP_HANDLE, IoTHubMessage_Properties, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle)
DECLARE_GLOBAL_MOCK_METHOD_3(IotHubMocks, , IOTHUB_MESSAGE_RESULT, IoTHubMessage_GetByteArray, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle, const unsigned char**, buffer, size_t*, size)
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , const char*, IoTHubMessage_GetString, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle)
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , IOTHUBMESSAGE_CONTENT_TYPE, IoTHubMessage_GetContentType, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle)
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , void*, VECTOR_back, VECTOR_HANDLE, handle)
DECLARE_GLOBAL_MOCK_METHOD_3(IotHubMocks, , TRANSPORT_HANDLE, IoTHubTransport_Create, IOTHUB_CLIENT_TRANSPORT_PROVIDER, protocol, const char*, iotHubName, const char*, iotHubSuffix)
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , void, IoTHubTransport_Destroy, TRANSPORT_HANDLE, transportHlHandle)
DECLARE_GLOBAL_MOCK_METHOD_3(IotHubMocks, , BROKER_RESULT, Broker_Publish, BROKER_HANDLE, broker, MODULE_HANDLE, source, MESSAGE_HANDLE, message)
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , JSON_Value*, json_parse_string, const char *, filename);
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , JSON_Object*, json_value_get_object, const JSON_Value*, value);
DECLARE_GLOBAL_MOCK_METHOD_2(IotHubMocks, , const char*, json_object_get_string, const JSON_Object*, object, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubMocks, , void, json_value_free, JSON_Value*, value);

BEGIN_TEST_SUITE(iothub_ut)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
        g_testByTest = MicroMockCreateMutex();
        ASSERT_IS_NOT_NULL(g_testByTest);

        const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
        Module_ParseConfigurationFromJson = MODULE_PARSE_CONFIGURATION_FROM_JSON(apis);
        Module_FreeConfiguration = MODULE_FREE_CONFIGURATION(apis);
        Module_Create = MODULE_CREATE(apis);
        Module_Destroy = MODULE_DESTROY(apis);
        Module_Receive = MODULE_RECEIVE(apis);
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

        currentVECTOR_push_back_call = 0;
        whenShallVECTOR_push_back_fail = 0;

        currentSTRING_construct_call = 0;
        whenShallSTRING_construct_fail = 0;

        currentIoTHubMessage_CreateFromByteArray_call = 0;
        whenShallIoTHubMessage_CreateFromByteArray_fail = 0;

        currentIoTHubClient_Create_call = 0;
        whenShallIoTHubClient_Create_fail = 0;

    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
        if (!MicroMockReleaseMutex(g_testByTest))
        {
            ASSERT_FAIL("failure in test framework at ReleaseMutex");
        }
    }

    /*Tests_SRS_IOTHUBMODULE_26_001: [ `Module_GetApi` shall return a pointer to a `MODULE_API` structure with the required function pointers. ]*/
    TEST_FUNCTION(Module_GetApi_returns_non_NULL_fields)
    {
        ///arrange
        IotHubMocks mocks;

        ///act
        const MODULE_API* MODULEAPIS = Module_GetApi(MODULE_API_VERSION_1);

        ///assert
        ASSERT_IS_NOT_NULL(MODULEAPIS);
        ASSERT_IS_TRUE(MODULE_PARSE_CONFIGURATION_FROM_JSON(MODULEAPIS) != NULL);
        ASSERT_IS_TRUE(MODULE_FREE_CONFIGURATION(MODULEAPIS) != NULL);
        ASSERT_IS_TRUE(MODULE_CREATE(MODULEAPIS) != NULL);
        ASSERT_IS_TRUE(MODULE_DESTROY(MODULEAPIS) != NULL);
        ASSERT_IS_TRUE(MODULE_RECEIVE(MODULEAPIS) != NULL);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }


    /*Tests_SRS_IOTHUBMODULE_05_004: [ `IotHub_ParseConfigurationFromJson` shall parse `configuration` as a JSON string. ]*/
    TEST_FUNCTION(IotHub_ParseConfigurationFromJson_success)
    {
        ///arrange
        IotHubMocks mocks;
        const char * validJsonString = "calling it valid makes it so";
        BROKER_HANDLE broker = (BROKER_HANDLE)0x42;

        STRICT_EXPECTED_CALL(mocks, json_parse_string(validJsonString));
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "IoTHubName"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "IoTHubSuffix"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "Transport"))
            .IgnoreArgument(1)
            .SetReturn("HTTP");
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(strlen("aHubName") + 1));
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(strlen("suffix.name") + 1));
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(IOTHUB_CONFIG)));
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        auto result = Module_ParseConfigurationFromJson(validJsonString);

        ///assert
        ASSERT_IS_NOT_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_FreeConfiguration(result);
    }

    TEST_FUNCTION(IotHub_ParseConfigurationFromJson_returns_NULL_when_malloc_fails_1)
    {
        ///arrange
        IotHubMocks mocks;
        const char * validJsonString = "calling it valid makes it so";
        BROKER_HANDLE broker = (BROKER_HANDLE)0x42;

        STRICT_EXPECTED_CALL(mocks, json_parse_string(validJsonString));
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "IoTHubName"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "IoTHubSuffix"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "Transport"))
            .IgnoreArgument(1)
            .SetReturn("HTTP");

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(strlen("aHubName") + 1));
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(strlen("suffix.name") + 1));
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(IOTHUB_CONFIG)))
            .SetFailReturn((void*)NULL);

        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        auto result = Module_ParseConfigurationFromJson(validJsonString);

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    TEST_FUNCTION(IotHub_ParseConfigurationFromJson_returns_NULL_when_malloc_fails_2)
    {
        ///arrange
        IotHubMocks mocks;
        const char * validJsonString = "calling it valid makes it so";
        BROKER_HANDLE broker = (BROKER_HANDLE)0x42;

        STRICT_EXPECTED_CALL(mocks, json_parse_string(validJsonString));
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "IoTHubName"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "IoTHubSuffix"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "Transport"))
            .IgnoreArgument(1)
            .SetReturn("HTTP");

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(strlen("aHubName") + 1));
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(strlen("suffix.name") + 1))
            .SetFailReturn((void*)NULL);

        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        auto result = Module_ParseConfigurationFromJson(validJsonString);

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    TEST_FUNCTION(IotHub_ParseConfigurationFromJson_returns_NULL_when_malloc_fails_3)
    {
        ///arrange
        IotHubMocks mocks;
        const char * validJsonString = "calling it valid makes it so";
        BROKER_HANDLE broker = (BROKER_HANDLE)0x42;

        STRICT_EXPECTED_CALL(mocks, json_parse_string(validJsonString));
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "IoTHubName"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "IoTHubSuffix"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "Transport"))
            .IgnoreArgument(1)
            .SetReturn("HTTP");

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(strlen("aHubName") + 1))
            .SetFailReturn((void*)NULL);

        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        auto result = Module_ParseConfigurationFromJson(validJsonString);

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    TEST_FUNCTION(IotHub_ParseConfigurationFromJson_interprets_HTTP_transport)
    {
        ///arrange
        CNiceCallComparer<IotHubMocks> mocks;

        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "Transport"))
            .IgnoreArgument(1)
            .SetReturn("HTTP");

        ///act
        auto result = Module_ParseConfigurationFromJson("don't care");

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_FreeConfiguration(result);
    }

    TEST_FUNCTION(IotHub_ParseConfigurationFromJson_interprets_AMQP_transport)
    {
        ///arrange
        CNiceCallComparer<IotHubMocks> mocks;

        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "Transport"))
            .IgnoreArgument(1)
            .SetReturn("AMQP");

        ///act
        auto result = Module_ParseConfigurationFromJson("don't care");

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_FreeConfiguration(result);
    }

    TEST_FUNCTION(IotHub_ParseConfigurationFromJson_interprets_MQTT_transport)
    {
        ///arrange
        CNiceCallComparer<IotHubMocks> mocks;

        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "Transport"))
            .IgnoreArgument(1)
            .SetReturn("MQTT");

        ///act
        auto result = Module_ParseConfigurationFromJson("don't care");

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_FreeConfiguration(result);
    }

    /*Tests_SRS_IOTHUBMODULE_05_012: [ If the value of "Transport" is not one of "HTTP", "AMQP", or "MQTT" (case-insensitive) then `IotHub_ParseConfigurationFromJson` shall fail and return NULL. ]*/
    TEST_FUNCTION(IotHub_ParseConfigurationFromJson_interprets_the_transport_string_without_regard_to_case)
    {
        CNiceCallComparer<IotHubMocks> mocks;

        const char* transportStrings[] = { "http", "amqp", "mqtt" };
        const IOTHUB_CLIENT_TRANSPORT_PROVIDER providers[] = { HTTP_Protocol, AMQP_Protocol, MQTT_Protocol };
        const size_t length = sizeof(transportStrings) / sizeof(transportStrings[0]);

        for (int i = 0; i < length; ++i)
        {
            mocks.ResetAllCalls();

            ///arrange
            STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "Transport"))
                .IgnoreArgument(1)
                .SetReturn(transportStrings[i]);

            ///act
            auto result = Module_ParseConfigurationFromJson("don't care");

            ///assert
            ASSERT_IS_NOT_NULL(result);
            mocks.AssertActualAndExpectedCalls();

            ///cleanup
            Module_FreeConfiguration(result);
        }
    }

    /*Tests_SRS_IOTHUBMODULE_05_012: [ If the value of "Transport" is not one of "HTTP", "AMQP", or "MQTT" (case-insensitive) then `IotHub_ParseConfigurationFromJson` shall fail and return NULL. ]*/
    TEST_FUNCTION(IotHub_ParseConfigurationFromJson_returns_null_when_transport_is_unknown)
    {
        ///arrange
        CNiceCallComparer<IotHubMocks> mocks;

        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "Transport"))
            .IgnoreArgument(1)
            .SetReturn("ABCD");

        ///act
        auto result = Module_ParseConfigurationFromJson("don't care");

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_IOTHUBMODULE_05_011: [ If the JSON object does not contain a value named "Transport" then `IotHub_ParseConfigurationFromJson` shall fail and return NULL. ]*/
    TEST_FUNCTION(IotHub_ParseConfigurationFromJson_returns_null_when_Transport_is_missing)
    {
        ///arrange
        IotHubMocks mocks;
        const char * validJsonString = "calling it valid makes it so";

        STRICT_EXPECTED_CALL(mocks, json_parse_string(validJsonString));
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "IoTHubName"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "IoTHubSuffix"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "Transport"))
            .IgnoreArgument(1)
            .SetFailReturn((const char *)NULL);
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        auto result = Module_ParseConfigurationFromJson(validJsonString);
        ///assert

        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_IOTHUBMODULE_05_007: [ If the JSON object does not contain a value named "IoTHubSuffix" then `IotHub_ParseConfigurationFromJson` shall fail and return NULL. ]*/
    TEST_FUNCTION(IotHub_ParseConfigurationFromJson_IoTHubSuffix_not_found_returns_null)
    {
        ///arrange
        IotHubMocks mocks;
        const char * validJsonString = "calling it valid makes it so";

        STRICT_EXPECTED_CALL(mocks, json_parse_string(validJsonString));
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "IoTHubName"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "IoTHubSuffix"))
            .IgnoreArgument(1)
            .SetFailReturn((const char *)NULL);
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        auto result = Module_ParseConfigurationFromJson(validJsonString);
        ///assert

        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_IOTHUBMODULE_05_006: [ If the JSON object does not contain a value named "IoTHubName" then `IotHub_ParseConfigurationFromJson` shall fail and return NULL. ]*/
    TEST_FUNCTION(IotHub_ParseConfigurationFromJson_IoTHubName_not_found_returns_null)
    {
        ///arrange
        IotHubMocks mocks;
        const char * validJsonString = "calling it valid makes it so";

        STRICT_EXPECTED_CALL(mocks, json_parse_string(validJsonString));
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "IoTHubName"))
            .IgnoreArgument(1)
            .SetFailReturn((const char *)NULL);
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        auto result = Module_ParseConfigurationFromJson(validJsonString);
        ///assert

        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_IOTHUBMODULE_05_005: [ If parsing of `configuration` fails, `IotHub_ParseConfigurationFromJson` shall fail and return NULL. ]*/
    TEST_FUNCTION(IotHub_ParseConfigurationFromJson_get_object_null_returns_null)
    {
        ///arrange
        IotHubMocks mocks;
        const char * validJsonString = "calling it valid makes it so";

        STRICT_EXPECTED_CALL(mocks, json_parse_string(validJsonString));
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((JSON_Object*)NULL);
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        auto result = Module_ParseConfigurationFromJson(validJsonString);
        ///assert

        ASSERT_IS_NULL(result);
    }

    /*Tests_SRS_IOTHUBMODULE_05_005: [ If parsing of `configuration` fails, `IotHub_ParseConfigurationFromJson` shall fail and return NULL. ]*/
    TEST_FUNCTION(IotHub_ParseConfigurationFromJson_parse_string_null_returns_null)
    {
        ///arrange
        IotHubMocks mocks;
        const char * invalidJsonString = "calling it invalid has the same effect";

        STRICT_EXPECTED_CALL(mocks, json_parse_string(invalidJsonString))
            .SetFailReturn((JSON_Value*)NULL);

        ///act
        auto result = Module_ParseConfigurationFromJson(invalidJsonString);
        ///assert

        ASSERT_IS_NULL(result);
    }

    /*Tests_SRS_IOTHUBMODULE_05_002: [ If `configuration` is NULL then `IotHub_ParseConfigurationFromJson` shall fail and return NULL. ]*/
    TEST_FUNCTION(IotHub_ParseConfigurationFromJson_config_null_returns_null)
    {
        ///arrange
        IotHubMocks mocks;

        ///act
        auto result = Module_ParseConfigurationFromJson(NULL);

        ///assert
        ASSERT_IS_NULL(result);
    }

    /*Tests_SRS_IOTHUBMODULE_05_014: [ If `configuration` is NULL then `IotHub_FreeConfiguration` shall do nothing. ]*/
    TEST_FUNCTION(IotHub_FreeConfiguration_does_nothing_if_configuration_is_NULL)
    {
        ///arrange
        IotHubMocks mocks;

        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .NeverInvoked();

        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .NeverInvoked();

        ///act
        Module_FreeConfiguration(NULL);

        ///assert
    }

    /*Tests_SRS_IOTHUBMODULE_05_015: [ `IotHub_FreeConfiguration` shall free the strings referenced by the `IoTHubName` and `IoTHubSuffix` data members, and then free the `IOTHUB_CONFIG` structure itself. ]*/
    TEST_FUNCTION(Module_FreeConfiguration_deallocates_configuration_memory)
    {
        ///arrange
        IotHubMocks mocks;
        IOTHUB_CONFIG config = CreateConfig();
        auto configp = (IOTHUB_CONFIG*)malloc(sizeof(IOTHUB_CONFIG));
        memcpy(configp, &config, sizeof(IOTHUB_CONFIG));
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        Module_FreeConfiguration(configp);

        ///assert
    }

    /*Tests_SRS_IOTHUBMODULE_02_001: [ If `broker` is `NULL` then `IotHub_Create` shall fail and return `NULL`. ]*/
    TEST_FUNCTION(IotHub_Create_with_NULL_BROKER_HANDLE_returns_NULL)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        mocks.ResetAllCalls();

        ///act
        auto result = Module_Create(NULL, config);

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_IOTHUBMODULE_02_002: [ If `configuration` is `NULL` then `IotHub_Create` shall fail and return `NULL`. ]*/
    TEST_FUNCTION(IotHub_Create_with_NULL_config_returns_NULL)
    {
        ///arrange
        IotHubMocks mocks;

        ///act
        auto result = Module_Create((BROKER_HANDLE)1, NULL);

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_IOTHUBMODULE_02_003: [ If `configuration->IoTHubName` is `NULL` then `IotHub_Create` shall and return `NULL`. ]*/
    TEST_FUNCTION(IotHub_Create_with_NULL_IoTHubName_fails)
    {
        ///arrange
        IotHubMocks mocks;
        IOTHUB_CONFIG config_with_NULL_IoTHubName = { NULL, suffix, HTTP_Protocol };
        mocks.ResetAllCalls();

        ///act
        auto result = Module_Create((BROKER_HANDLE)1, &config_with_NULL_IoTHubName);

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_IOTHUBMODULE_02_004: [ If `configuration->IoTHubSuffix` is `NULL` then `IotHub_Create` shall fail and return `NULL`. ]*/
    TEST_FUNCTION(IotHub_Create_with_NULL_IoTHubSuffix_fails)
    {
        ///arrange
        IotHubMocks mocks;
        IOTHUB_CONFIG config_with_NULL_IoTHubSuffix = { name, NULL, HTTP_Protocol };
        mocks.ResetAllCalls();

        ///act
        auto result = Module_Create((BROKER_HANDLE)1, &config_with_NULL_IoTHubSuffix);

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_IOTHUBMODULE_02_008: [ Otherwise, `IotHub_Create` shall return a non-`NULL` handle. ]*/
    /*Tests_SRS_IOTHUBMODULE_02_006: [ `IotHub_Create` shall create an empty `VECTOR` containing pointers to `PERSONALITY`s. ]*/
    /*Tests_SRS_IOTHUBMODULE_17_001: [ If `configuration->transportProvider` is `HTTP_Protocol` or `AMQP_Protocol`, `IotHub_Create` shall create a shared transport by calling `IoTHubTransport_Create`. ]*/
    /*Tests_SRS_IOTHUBMODULE_02_029: [ `IotHub_Create` shall create a copy of `configuration->IoTHubSuffix`. ]*/
    /*Tests_SRS_IOTHUBMODULE_02_028: [ `IotHub_Create` shall create a copy of `configuration->IoTHubName`. ]*/
    /*Tests_SRS_IOTHUBMODULE_17_004: [ `IotHub_Create` shall store the broker. ]*/
    TEST_FUNCTION(IotHub_Create_succeeds)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(name));

        STRICT_EXPECTED_CALL(mocks, STRING_construct(suffix));

        STRICT_EXPECTED_CALL(mocks, IoTHubTransport_Create(HTTP_Protocol, name, suffix));

        ///act
        auto module = Module_Create(BROKER_HANDLE_VALID, config);

        ///assert
        ASSERT_IS_NOT_NULL(module);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    TEST_FUNCTION(IotHub_Create_creates_a_transport_for_AMQP)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config(AMQP_Protocol);
        mocks.ResetAllCalls();

        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));

        EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG));

        EXPECTED_CALL(mocks, STRING_construct(name));

        EXPECTED_CALL(mocks, STRING_construct(suffix));

        STRICT_EXPECTED_CALL(mocks, IoTHubTransport_Create(AMQP_Protocol, name, suffix));

        ///act
        auto module = Module_Create(BROKER_HANDLE_VALID, config);

        ///assert
        ASSERT_IS_NOT_NULL(module);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    TEST_FUNCTION(IotHub_Create_does_not_create_a_transport_for_MQTT)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config(MQTT_Protocol);
        mocks.ResetAllCalls();

        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));

        EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG));

        EXPECTED_CALL(mocks, STRING_construct(name));

        EXPECTED_CALL(mocks, STRING_construct(suffix));

        EXPECTED_CALL(mocks, IoTHubTransport_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .NeverInvoked();

        ///act
        auto module = Module_Create(BROKER_HANDLE_VALID, config);

        ///assert
        ASSERT_IS_NOT_NULL(module);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    TEST_FUNCTION(IotHub_Create_does_not_create_an_unrecognized_transport)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config((IOTHUB_CLIENT_TRANSPORT_PROVIDER)0x42);
        mocks.ResetAllCalls();

        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));

        EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG));

        EXPECTED_CALL(mocks, STRING_construct(name));

        EXPECTED_CALL(mocks, STRING_construct(suffix));

        EXPECTED_CALL(mocks, IoTHubTransport_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .NeverInvoked();

        ///act
        auto module = Module_Create(BROKER_HANDLE_VALID, config);

        ///assert
        ASSERT_IS_NOT_NULL(module);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_02_027: [ When `IotHub_Create` encounters an internal failure it shall fail and return `NULL`. ]*/
    TEST_FUNCTION(IotHub_Create_fails_when_STRING_construct_fails_1)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, IoTHubTransport_Create(HTTP_Protocol, name, suffix));
        STRICT_EXPECTED_CALL(mocks, IoTHubTransport_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(name));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        whenShallSTRING_construct_fail = currentSTRING_construct_call + 2;
        STRICT_EXPECTED_CALL(mocks, STRING_construct(suffix));

        ///act
        auto module = Module_Create(BROKER_HANDLE_VALID, config);

        ///assert
        ASSERT_IS_NULL(module);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_02_027: [ When `IotHub_Create` encounters an internal failure it shall fail and return `NULL`. ]*/
    TEST_FUNCTION(IotHub_Create_fails_when_STRING_construct_fails_2)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, IoTHubTransport_Create(HTTP_Protocol, name, suffix))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, IoTHubTransport_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        whenShallSTRING_construct_fail = currentSTRING_construct_call + 1;
        STRICT_EXPECTED_CALL(mocks, STRING_construct(name));

        ///act
        auto module = Module_Create(BROKER_HANDLE_VALID, config);

        ///assert
        ASSERT_IS_NULL(module);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_17_002: [ If creating the shared transport fails, `IotHub_Create` shall fail and return `NULL`. ]*/
    TEST_FUNCTION(IotHub_Create_fails_when_transport_create_fails)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, IoTHubTransport_Create(HTTP_Protocol, name, suffix))
            .SetFailReturn((TRANSPORT_HANDLE)NULL);

        ///act
        auto module = Module_Create(BROKER_HANDLE_VALID, config);

        ///assert
        ASSERT_IS_NULL(module);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_02_007: [ If creating the personality vector fails then `IotHub_Create` shall fail and return `NULL`. ]*/
    TEST_FUNCTION(IotHub_Create_fails_when_VECTOR_create_fails)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((VECTOR_HANDLE)NULL);

        ///act
        auto module = Module_Create(BROKER_HANDLE_VALID, config);

        ///assert
        ASSERT_IS_NULL(module);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_IOTHUBMODULE_02_027: [ When `IotHub_Create` encounters an internal failure it shall fail and return `NULL`. ]*/
    TEST_FUNCTION(IotHub_Create_fails_when_gballoc_fails)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((void*)NULL);

        ///act
        auto module = Module_Create(BROKER_HANDLE_VALID, config);

        ///assert
        ASSERT_IS_NULL(module);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_IOTHUBMODULE_02_023: [ If `moduleHandle` is `NULL` then `IotHub_Destroy` shall return. ]*/
    TEST_FUNCTION(IotHub_Destroy_with_NULL_returns)
    {
        ///arrange
        IotHubMocks mocks;

        ///act
        Module_Destroy(NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_IOTHUBMODULE_02_024: [ Otherwise `IotHub_Destroy` shall free all used resources. ]*/
    TEST_FUNCTION(IotHub_Destroy_empty_module)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        mocks.ResetAllCalls();

        /* transport handle */
        STRICT_EXPECTED_CALL(mocks, IoTHubTransport_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        /*IoTHubName cache*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        /*IoTHubSuffix cache*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);


        ///act
        Module_Destroy(module);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_IOTHUBMODULE_02_024: [ Otherwise `IotHub_Destroy` shall free all used resources. ]*/
    TEST_FUNCTION(IotHub_Destroy_module_with_0_identity)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        mocks.ResetAllCalls();

        /*this is IoTHubName*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        /*this is IoTHubSuffix*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        /*this is the loop trying to dispose of all personalities*/
        /*none for this case*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        /*this is VECTOR<PERSONALITY>*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        /* transport handle */
        STRICT_EXPECTED_CALL(mocks, IoTHubTransport_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        /*this is allocated memory*/
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        Module_Destroy(module);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup - nothing
    }

    /*Tests_SRS_IOTHUBMODULE_02_024: [ Otherwise `IotHub_Destroy` shall free all used resources. ]*/
    TEST_FUNCTION(IotHub_Destroy_module_with_1_identity)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        (void)Module_Receive(module, MESSAGE_HANDLE_VALID_1);
        mocks.ResetAllCalls();

        /*this is IoTHubName*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        /*this is IoTHubSuffix*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        /*this is the loop trying to dispose of all personalities*/
        /*1 for this case*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, IoTHubClient_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        /* transport handle */
        STRICT_EXPECTED_CALL(mocks, IoTHubTransport_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        /*this is VECTOR<PERSONALITY>*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        /*this is allocated memory*/
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        Module_Destroy(module);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup - nothing
    }

    /*Tests_SRS_IOTHUBMODULE_02_024: [ Otherwise `IotHub_Destroy` shall free all used resources. ]*/
    TEST_FUNCTION(IotHub_Destroy_module_with_2_identity)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        (void)Module_Receive(module, MESSAGE_HANDLE_VALID_1);
        (void)Module_Receive(module, MESSAGE_HANDLE_VALID_2);
        mocks.ResetAllCalls();

        /*this is IoTHubName*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        /*this is IoTHubSuffix*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        /*this is the loop trying to dispose of all personalities*/
        /*1 for this case*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        /*first element*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, IoTHubClient_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        /*second element*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, IoTHubClient_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        /* transport handle */
        STRICT_EXPECTED_CALL(mocks, IoTHubTransport_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        /*this is VECTOR<PERSONALITY>*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        /*this is allocated memory*/
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        Module_Destroy(module);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup - nothing
    }

    /*Tests_SRS_IOTHUBMODULE_02_009: [ If `moduleHandle` or `messageHandle` is `NULL` then `IotHub_Receive` shall do nothing. ]*/
    TEST_FUNCTION(IotHub_Receive_with_NULL_moduleHandle_returns)
    {
        ///arrange
        IotHubMocks mocks;

        ///act
        Module_Receive(NULL, MESSAGE_HANDLE_VALID);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_IOTHUBMODULE_02_009: [ If `moduleHandle` or `messageHandle` is `NULL` then `IotHub_Receive` shall do nothing. ]*/
    TEST_FUNCTION(IotHub_Receive_with_NULL_messageHandle_returns)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        mocks.ResetAllCalls();

        ///act
        Module_Receive(module, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);

    }

    /*Tests_SRS_IOTHUBMODULE_02_013: [ If no personality exists with a device ID equal to the value of the `deviceName` property of the message, then `IotHub_Receive` shall create a new `PERSONALITY` with the ID and key values from the message. ]*/
    /*Tests_SRS_IOTHUBMODULE_05_013: [ If a new personality is created and the module's transport has already been created (in `IotHub_Create`), an `IOTHUB_CLIENT_HANDLE` will be added to the personality by a call to `IoTHubClient_CreateWithTransport`. ]*/
    /*Tests_SRS_IOTHUBMODULE_17_003: [ If a new personality is created, then the associated IoTHubClient will be set to receive messages by calling `IoTHubClient_SetMessageCallback` with callback function `IotHub_ReceiveMessageCallback`, and the personality as context. ]*/
    /*Tests_SRS_IOTHUBMODULE_02_018: [ `IotHub_Receive` shall create a new IOTHUB_MESSAGE_HANDLE having the same content as `messageHandle`, and the same properties with the exception of `deviceName` and `deviceKey`. ]*/
    /*Tests_SRS_IOTHUBMODULE_02_020: [ `IotHub_Receive` shall call IoTHubClient_SendEventAsync passing the IOTHUB_MESSAGE_HANDLE. ]*/
    /*Tests_SRS_IOTHUBMODULE_02_022: [ If `IoTHubClient_SendEventAsync` succeeds then `IotHub_Receive` shall return. ]*/
    TEST_FUNCTION(IotHub_Receive_succeeds)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(MESSAGE_HANDLE_VALID_1));
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "source"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceName"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceKey"));

        /*VECTOR_find_if incurs a STRING_c_str until it find the deviceName. None in this test*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        /*because the deviceName is brand new, it will be added as a new personality*/
        {/*separate scope for personality building*/
            /* create a new PERSONALITY */
            STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
                .IgnoreArgument(1);

            /*making a copy of the deviceName*/
            STRICT_EXPECTED_CALL(mocks, STRING_construct("firstDevice"));
            /*making a copy of the deviceKey*/
            STRICT_EXPECTED_CALL(mocks, STRING_construct("cheiaDeLaPoartaVerde"));

            /*getting the stored IoTHubName*/
            STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
            /*getting the stored IoTHubSuffix*/
            STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*creating the IOTHUB_CLIENT_HANDLE associated with the device*/
            STRICT_EXPECTED_CALL(mocks, IoTHubClient_CreateWithTransport(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(1)
                .IgnoreArgument(2);

            STRICT_EXPECTED_CALL(mocks, IoTHubClient_SetMessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(1)
                .IgnoreArgument(2)
                .IgnoreArgument(3);
        }

        /*adding the personality to the VECTOR or personalities*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        /*getting the location of the personality in the VECTOR*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_back(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        { /*scope for creating the IOTHUBMESSAGE from GWMESSAGE*/

            /*gettng the GW message content*/
            STRICT_EXPECTED_CALL(mocks, Message_GetContent(MESSAGE_HANDLE_VALID_1));

            /*creating a new IOTHUB_MESSAGE*/
            STRICT_EXPECTED_CALL(mocks, IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, 1))
                .ValidateArgumentBuffer(1, CONSTBUFFER_VALID_CONTENT1.buffer, 1);

            /*every creation has its destruction - this one actually happens AFTER SendEventAsync*/
            STRICT_EXPECTED_CALL(mocks, IoTHubMessage_Destroy(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*getting the IOTHUBMESSAGE properties*/
            STRICT_EXPECTED_CALL(mocks, IoTHubMessage_Properties(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*getting the GW message properties*/
            STRICT_EXPECTED_CALL(mocks, Message_GetProperties(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
            STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*getting the GW keys and values*/
            STRICT_EXPECTED_CALL(mocks, ConstMap_GetInternals(CONSTMAP_HANDLE_VALID_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(2)
                .IgnoreArgument(3)
                .IgnoreArgument(4);

            /*add "source"*/
            STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, "source", "mapping"))
                .IgnoreArgument(1);

            /*add "somethingExtra"*/
            STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, "somethingExtra", "blue"))
                .IgnoreArgument(1);
        }

        /*finally, send the message*/
        STRICT_EXPECTED_CALL(mocks, IoTHubClient_SendEventAsync(IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL, NULL))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        ///act
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);

    }

    /*Tests_SRS_IOTHUBMODULE_05_013: [ If a new personality is created and the module's transport has already been created (in `IotHub_Create`), an `IOTHUB_CLIENT_HANDLE` will be added to the personality by a call to `IoTHubClient_CreateWithTransport`. ]*/
    TEST_FUNCTION(IotHub_Receive_creates_a_client_with_an_existing_AMQP_transport)
    {
        ///arrange
        CNiceCallComparer<IotHubMocks> mocks;
        IOTHUB_CLIENT_TRANSPORT_PROVIDER transport = AMQP_Protocol;
        AutoConfig config(transport);

        STRICT_EXPECTED_CALL(mocks, IoTHubTransport_Create(transport, name, suffix));
        EXPECTED_CALL(mocks, IoTHubClient_CreateWithTransport(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .ValidateArgumentBuffer(2, &transport, sizeof(IOTHUB_CLIENT_TRANSPORT_PROVIDER));

        auto module = Module_Create(BROKER_HANDLE_VALID, config);

        ///act
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_05_003: [ If a new personality is created and the module's transport has not already been created, an `IOTHUB_CLIENT_HANDLE` will be added to the personality by a call to `IoTHubClient_Create` with the corresponding transport provider. ]*/
    TEST_FUNCTION(IotHub_Receive_creates_a_client_with_MQTT_transport)
    {
        ///arrange
        CNiceCallComparer<IotHubMocks> mocks;
        IOTHUB_CLIENT_TRANSPORT_PROVIDER transport = MQTT_Protocol;
        AutoConfig config(transport);

        EXPECTED_CALL(mocks, IoTHubClient_Create(IGNORED_PTR_ARG))
            .ValidateArgumentBuffer(1, &transport, sizeof(IOTHUB_CLIENT_TRANSPORT_PROVIDER), 0);

        auto module = Module_Create(BROKER_HANDLE_VALID, config);

        ///act
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_05_003: [ If a new personality is created and the module's transport has not already been created, an `IOTHUB_CLIENT_HANDLE` will be added to the personality by a call to `IoTHubClient_Create` with the corresponding transport provider. ]*/
    TEST_FUNCTION(IotHub_Receive_creates_a_client_with_custom_transport)
    {
        ///arrange
        CNiceCallComparer<IotHubMocks> mocks;
        IOTHUB_CLIENT_TRANSPORT_PROVIDER transport = ABCD_Protocol;
        AutoConfig config(transport);

        EXPECTED_CALL(mocks, IoTHubClient_Create(IGNORED_PTR_ARG))
            .ValidateArgumentBuffer(1, &transport, sizeof(IOTHUB_CLIENT_TRANSPORT_PROVIDER));

        auto module = Module_Create(BROKER_HANDLE_VALID, config);

        ///act
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_02_017: [ Otherwise `IotHub_Receive` shall not create a new personality. ]*/
    /*Tests_SRS_IOTHUBMODULE_02_020: [ `IotHub_Receive` shall call IoTHubClient_SendEventAsync passing the IOTHUB_MESSAGE_HANDLE. ]*/
    /*Tests_SRS_IOTHUBMODULE_02_022: [ If `IoTHubClient_SendEventAsync` succeeds then `IotHub_Receive` shall return. ]*/
    TEST_FUNCTION(IotHub_Receive_after_receive_succeeds)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(MESSAGE_HANDLE_VALID_1));
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "source"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceName"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceKey"));

        /*VECTOR_find_if incurs a STRING_c_str until it find the deviceName. One in this test*/
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        { /*scope for creating the IOTHUBMESSAGE from GWMESSAGE*/

          /*gettng the GW message content*/
            STRICT_EXPECTED_CALL(mocks, Message_GetContent(MESSAGE_HANDLE_VALID_1));

            /*creating a new IOTHUB_MESSAGE*/
            STRICT_EXPECTED_CALL(mocks, IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, 1))
                .ValidateArgumentBuffer(1, CONSTBUFFER_VALID_CONTENT1.buffer, 1);

            /*every creation has its destruction - this one actually happens AFTER SendEventAsync*/
            STRICT_EXPECTED_CALL(mocks, IoTHubMessage_Destroy(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*getting the IOTHUBMESSAGE properties*/
            STRICT_EXPECTED_CALL(mocks, IoTHubMessage_Properties(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*getting the GW message properties*/
            STRICT_EXPECTED_CALL(mocks, Message_GetProperties(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
            STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*getting the GW keys and values*/
            STRICT_EXPECTED_CALL(mocks, ConstMap_GetInternals(CONSTMAP_HANDLE_VALID_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(2)
                .IgnoreArgument(3)
                .IgnoreArgument(4);

            /*add "source"*/
            STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, "source", "mapping"))
                .IgnoreArgument(1);

            /*add "somethingExtra"*/
            STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, "somethingExtra", "blue"))
                .IgnoreArgument(1);
        }

        /*finally, send the message*/
        STRICT_EXPECTED_CALL(mocks, IoTHubClient_SendEventAsync(IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL, NULL))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        ///act
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);

    }

    /*Tests_SRS_IOTHUBMODULE_02_013: [ If no personality exists with a device ID equal to the value of the `deviceName` property of the message, then `IotHub_Receive` shall create a new `PERSONALITY` with the ID and key values from the message. ]*/
    /*Tests_SRS_IOTHUBMODULE_02_018: [ `IotHub_Receive` shall create a new IOTHUB_MESSAGE_HANDLE having the same content as `messageHandle`, and the same properties with the exception of `deviceName` and `deviceKey`. ]*/
    /*Tests_SRS_IOTHUBMODULE_02_020: [ `IotHub_Receive` shall call IoTHubClient_SendEventAsync passing the IOTHUB_MESSAGE_HANDLE. ]*/
    /*Tests_SRS_IOTHUBMODULE_02_022: [ If `IoTHubClient_SendEventAsync` succeeds then `IotHub_Receive` shall return. ]*/
    TEST_FUNCTION(IotHub_Receive_after_Receive_a_new_device_succeeds)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(MESSAGE_HANDLE_VALID_2));
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_2, "source"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_2, "deviceName"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_2, "deviceKey"));

        /*VECTOR_find_if incurs a STRING_c_str until it find the deviceName. One in this test*/
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        /*because the deviceName is brand new, it will be added as a new personality*/
        {/*separate scope for personality building*/
         /* create a new PERSONALITY */
            STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
                .IgnoreArgument(1);

         /*making a copy of the deviceName*/
            STRICT_EXPECTED_CALL(mocks, STRING_construct("secondDevice"));
            /*making a copy of the deviceKey*/
            STRICT_EXPECTED_CALL(mocks, STRING_construct("red"));

            /*getting the stored IoTHubName*/
            STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
            /*getting the stored IoTHubSuffix*/
            STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*creating the IOTHUB_CLIENT_HANDLE associated with the device*/
            STRICT_EXPECTED_CALL(mocks, IoTHubClient_CreateWithTransport(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(1)
                .IgnoreArgument(2);

            STRICT_EXPECTED_CALL(mocks, IoTHubClient_SetMessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(1)
                .IgnoreArgument(2)
                .IgnoreArgument(3);
        }

        /*adding the personality to the VECTOR or personalities*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        /*getting the location of the personality in the VECTOR*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_back(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        { /*scope for creating the IOTHUBMESSAGE from GWMESSAGE*/

          /*gettng the GW message content*/
            STRICT_EXPECTED_CALL(mocks, Message_GetContent(MESSAGE_HANDLE_VALID_2));

            /*creating a new IOTHUB_MESSAGE*/
            STRICT_EXPECTED_CALL(mocks, IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, 0))
                ;

            /*every creation has its destruction - this one actually happens AFTER SendEventAsync*/
            STRICT_EXPECTED_CALL(mocks, IoTHubMessage_Destroy(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*getting the IOTHUBMESSAGE properties*/
            STRICT_EXPECTED_CALL(mocks, IoTHubMessage_Properties(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*getting the GW message properties*/
            STRICT_EXPECTED_CALL(mocks, Message_GetProperties(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
            STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*getting the GW keys and values*/
            STRICT_EXPECTED_CALL(mocks, ConstMap_GetInternals(CONSTMAP_HANDLE_VALID_2, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(2)
                .IgnoreArgument(3)
                .IgnoreArgument(4);

            /*add "source"*/
            STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, "source", "mapping"))
                .IgnoreArgument(1);
        }

        /*finally, send the message*/
        STRICT_EXPECTED_CALL(mocks, IoTHubClient_SendEventAsync(IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL, NULL))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        ///act
        Module_Receive(module, MESSAGE_HANDLE_VALID_2);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);

    }

    /*Tests_SRS_IOTHUBMODULE_02_021: [ If `IoTHubClient_SendEventAsync` fails then `IotHub_Receive` shall return. ]*/
    TEST_FUNCTION(IotHub_Receive_when_IoTHubClient_SendEventAsync_fails_it_still_returns)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(MESSAGE_HANDLE_VALID_1));
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "source"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceName"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceKey"));

        /*VECTOR_find_if incurs a STRING_c_str until it find the deviceName. None in this test*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        /*because the deviceName is brand new, it will be added as a new personality*/
        {/*separate scope for personality building*/

         /* create a new PERSONALITY */
            STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
                .IgnoreArgument(1);
         /*making a copy of the deviceName*/
            STRICT_EXPECTED_CALL(mocks, STRING_construct("firstDevice"));
            /*making a copy of the deviceKey*/
            STRICT_EXPECTED_CALL(mocks, STRING_construct("cheiaDeLaPoartaVerde"));

            /*getting the stored IoTHubName*/
            STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
            /*getting the stored IoTHubSuffix*/
            STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*creating the IOTHUB_CLIENT_HANDLE associated with the device*/
            STRICT_EXPECTED_CALL(mocks, IoTHubClient_CreateWithTransport(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(1)
                .IgnoreArgument(2);

            STRICT_EXPECTED_CALL(mocks, IoTHubClient_SetMessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(1)
                .IgnoreArgument(2)
                .IgnoreArgument(3);
        }

        /*adding the personality to the VECTOR or personalities*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        /*getting the location of the personality in the VECTOR*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_back(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        { /*scope for creating the IOTHUBMESSAGE from GWMESSAGE*/

          /*gettng the GW message content*/
            STRICT_EXPECTED_CALL(mocks, Message_GetContent(MESSAGE_HANDLE_VALID_1));

            /*creating a new IOTHUB_MESSAGE*/
            STRICT_EXPECTED_CALL(mocks, IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, 1))
                .ValidateArgumentBuffer(1, CONSTBUFFER_VALID_CONTENT1.buffer, 1);

            /*every creation has its destruction - this one actually happens AFTER SendEventAsync*/
            STRICT_EXPECTED_CALL(mocks, IoTHubMessage_Destroy(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*getting the IOTHUBMESSAGE properties*/
            STRICT_EXPECTED_CALL(mocks, IoTHubMessage_Properties(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*getting the GW message properties*/
            STRICT_EXPECTED_CALL(mocks, Message_GetProperties(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
            STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*getting the GW keys and values*/
            STRICT_EXPECTED_CALL(mocks, ConstMap_GetInternals(CONSTMAP_HANDLE_VALID_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(2)
                .IgnoreArgument(3)
                .IgnoreArgument(4);

            /*add "source"*/
            STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, "source", "mapping"))
                .IgnoreArgument(1);

            /*add "somethingExtra"*/
            STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, "somethingExtra", "blue"))
                .IgnoreArgument(1);
        }

        /*finally, send the message*/
        STRICT_EXPECTED_CALL(mocks, IoTHubClient_SendEventAsync(IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL, NULL))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .SetReturn(IOTHUB_CLIENT_ERROR);

        ///act
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);

    }

    /*Tests_SRS_IOTHUBMODULE_02_019: [ If creating the IOTHUB_MESSAGE_HANDLE fails, then `IotHub_Receive` shall return. ]*/
    TEST_FUNCTION(IotHub_Receive_when_creating_IOTHUB_MESSAGE_HANDLE_fails_1)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(MESSAGE_HANDLE_VALID_1));
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "source"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceName"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceKey"));

        /*VECTOR_find_if incurs a STRING_c_str until it find the deviceName. None in this test*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        /*because the deviceName is brand new, it will be added as a new personality*/
        {/*separate scope for personality building*/

         /* create a new PERSONALITY */
            STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
                .IgnoreArgument(1);

         /*making a copy of the deviceName*/
            STRICT_EXPECTED_CALL(mocks, STRING_construct("firstDevice"));
            /*making a copy of the deviceKey*/
            STRICT_EXPECTED_CALL(mocks, STRING_construct("cheiaDeLaPoartaVerde"));

            /*getting the stored IoTHubName*/
            STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
            /*getting the stored IoTHubSuffix*/
            STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*creating the IOTHUB_CLIENT_HANDLE associated with the device*/
            STRICT_EXPECTED_CALL(mocks, IoTHubClient_CreateWithTransport(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(1)
                .IgnoreArgument(2);

            STRICT_EXPECTED_CALL(mocks, IoTHubClient_SetMessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(1)
                .IgnoreArgument(2)
                .IgnoreArgument(3);
        }

        /*adding the personality to the VECTOR or personalities*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        /*getting the location of the personality in the VECTOR*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_back(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        { /*scope for creating the IOTHUBMESSAGE from GWMESSAGE*/

          /*gettng the GW message content*/
            STRICT_EXPECTED_CALL(mocks, Message_GetContent(MESSAGE_HANDLE_VALID_1));

            /*creating a new IOTHUB_MESSAGE*/
            STRICT_EXPECTED_CALL(mocks, IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, 1))
                .ValidateArgumentBuffer(1, CONSTBUFFER_VALID_CONTENT1.buffer, 1);

            /*every creation has its destruction - this one actually happens AFTER SendEventAsync*/
            STRICT_EXPECTED_CALL(mocks, IoTHubMessage_Destroy(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*getting the IOTHUBMESSAGE properties*/
            STRICT_EXPECTED_CALL(mocks, IoTHubMessage_Properties(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*getting the GW message properties*/
            STRICT_EXPECTED_CALL(mocks, Message_GetProperties(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
            STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*getting the GW keys and values*/
            STRICT_EXPECTED_CALL(mocks, ConstMap_GetInternals(CONSTMAP_HANDLE_VALID_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(2)
                .IgnoreArgument(3)
                .IgnoreArgument(4);

            /*add "source"*/
            STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, "source", "mapping"))
                .IgnoreArgument(1);

            /*add "somethingExtra"*/
            STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, "somethingExtra", "blue"))
                .IgnoreArgument(1)
                .SetReturn(MAP_ERROR);
        }

        ///act
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);

    }

    /*Tests_SRS_IOTHUBMODULE_02_019: [ If creating the IOTHUB_MESSAGE_HANDLE fails, then `IotHub_Receive` shall return. ]*/
    TEST_FUNCTION(IotHub_Receive_when_creating_IOTHUB_MESSAGE_HANDLE_fails_2)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(MESSAGE_HANDLE_VALID_1));
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "source"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceName"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceKey"));

        /*VECTOR_find_if incurs a STRING_c_str until it find the deviceName. None in this test*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        /*because the deviceName is brand new, it will be added as a new personality*/
        {/*separate scope for personality building*/

         /* create a new PERSONALITY */
            STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
                .IgnoreArgument(1);

         /*making a copy of the deviceName*/
            STRICT_EXPECTED_CALL(mocks, STRING_construct("firstDevice"));
            /*making a copy of the deviceKey*/
            STRICT_EXPECTED_CALL(mocks, STRING_construct("cheiaDeLaPoartaVerde"));

            /*getting the stored IoTHubName*/
            STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
            /*getting the stored IoTHubSuffix*/
            STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*creating the IOTHUB_CLIENT_HANDLE associated with the device*/
            STRICT_EXPECTED_CALL(mocks, IoTHubClient_CreateWithTransport(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(1)
                .IgnoreArgument(2);

            STRICT_EXPECTED_CALL(mocks, IoTHubClient_SetMessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(1)
                .IgnoreArgument(2)
                .IgnoreArgument(3);
        }

        /*adding the personality to the VECTOR or personalities*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        /*getting the location of the personality in the VECTOR*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_back(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        { /*scope for creating the IOTHUBMESSAGE from GWMESSAGE*/

          /*gettng the GW message content*/
            STRICT_EXPECTED_CALL(mocks, Message_GetContent(MESSAGE_HANDLE_VALID_1));

            /*creating a new IOTHUB_MESSAGE*/
            STRICT_EXPECTED_CALL(mocks, IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, 1))
                .ValidateArgumentBuffer(1, CONSTBUFFER_VALID_CONTENT1.buffer, 1);

            /*every creation has its destruction - this one actually happens AFTER SendEventAsync*/
            STRICT_EXPECTED_CALL(mocks, IoTHubMessage_Destroy(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*getting the IOTHUBMESSAGE properties*/
            STRICT_EXPECTED_CALL(mocks, IoTHubMessage_Properties(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*getting the GW message properties*/
            STRICT_EXPECTED_CALL(mocks, Message_GetProperties(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
            STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*getting the GW keys and values*/
            STRICT_EXPECTED_CALL(mocks, ConstMap_GetInternals(CONSTMAP_HANDLE_VALID_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(2)
                .IgnoreArgument(3)
                .IgnoreArgument(4);

            /*add "source"*/
            STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, "source", "mapping"))
                .IgnoreArgument(1)
                .SetReturn(MAP_ERROR);
        }

        ///act
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);

    }

    /*Tests_SRS_IOTHUBMODULE_02_019: [ If creating the IOTHUB_MESSAGE_HANDLE fails, then `IotHub_Receive` shall return. ]*/
    TEST_FUNCTION(IotHub_Receive_when_creating_IOTHUB_MESSAGE_HANDLE_fails_3)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(MESSAGE_HANDLE_VALID_1));
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "source"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceName"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceKey"));

        /*VECTOR_find_if incurs a STRING_c_str until it find the deviceName. None in this test*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        /*because the deviceName is brand new, it will be added as a new personality*/
        {/*separate scope for personality building*/
         /* create a new PERSONALITY */
            STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
                .IgnoreArgument(1);
         /*making a copy of the deviceName*/
            STRICT_EXPECTED_CALL(mocks, STRING_construct("firstDevice"));
            /*making a copy of the deviceKey*/
            STRICT_EXPECTED_CALL(mocks, STRING_construct("cheiaDeLaPoartaVerde"));

            /*getting the stored IoTHubName*/
            STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
            /*getting the stored IoTHubSuffix*/
            STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*creating the IOTHUB_CLIENT_HANDLE associated with the device*/
            STRICT_EXPECTED_CALL(mocks, IoTHubClient_CreateWithTransport(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(1)
                .IgnoreArgument(2);

            STRICT_EXPECTED_CALL(mocks, IoTHubClient_SetMessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(1)
                .IgnoreArgument(2)
                .IgnoreArgument(3);
        }

        /*adding the personality to the VECTOR or personalities*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        /*getting the location of the personality in the VECTOR*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_back(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        { /*scope for creating the IOTHUBMESSAGE from GWMESSAGE*/

          /*gettng the GW message content*/
            STRICT_EXPECTED_CALL(mocks, Message_GetContent(MESSAGE_HANDLE_VALID_1));

            /*creating a new IOTHUB_MESSAGE*/
            STRICT_EXPECTED_CALL(mocks, IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, 1))
                .ValidateArgumentBuffer(1, CONSTBUFFER_VALID_CONTENT1.buffer, 1);

            /*every creation has its destruction - this one actually happens AFTER SendEventAsync*/
            STRICT_EXPECTED_CALL(mocks, IoTHubMessage_Destroy(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*getting the IOTHUBMESSAGE properties*/
            STRICT_EXPECTED_CALL(mocks, IoTHubMessage_Properties(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*getting the GW message properties*/
            STRICT_EXPECTED_CALL(mocks, Message_GetProperties(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
            STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*getting the GW keys and values*/
            STRICT_EXPECTED_CALL(mocks, ConstMap_GetInternals(CONSTMAP_HANDLE_VALID_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(2)
                .IgnoreArgument(3)
                .IgnoreArgument(4)
                .SetReturn(CONSTMAP_ERROR);
        }

        ///act
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);

    }

    /*Tests_SRS_IOTHUBMODULE_02_019: [ If creating the IOTHUB_MESSAGE_HANDLE fails, then `IotHub_Receive` shall return. ]*/
    TEST_FUNCTION(IotHub_Receive_when_creating_IOTHUB_MESSAGE_HANDLE_fails_4)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(MESSAGE_HANDLE_VALID_1));
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "source"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceName"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceKey"));

        /*VECTOR_find_if incurs a STRING_c_str until it find the deviceName. None in this test*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        /*because the deviceName is brand new, it will be added as a new personality*/
        {/*separate scope for personality building*/
         /* create a new PERSONALITY */
            STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
                .IgnoreArgument(1);
         /*making a copy of the deviceName*/
            STRICT_EXPECTED_CALL(mocks, STRING_construct("firstDevice"));
            /*making a copy of the deviceKey*/
            STRICT_EXPECTED_CALL(mocks, STRING_construct("cheiaDeLaPoartaVerde"));

            /*getting the stored IoTHubName*/
            STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
            /*getting the stored IoTHubSuffix*/
            STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*creating the IOTHUB_CLIENT_HANDLE associated with the device*/
            STRICT_EXPECTED_CALL(mocks, IoTHubClient_CreateWithTransport(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(1)
                .IgnoreArgument(2);

            STRICT_EXPECTED_CALL(mocks, IoTHubClient_SetMessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(1)
                .IgnoreArgument(2)
                .IgnoreArgument(3);
        }

        /*adding the personality to the VECTOR or personalities*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        /*getting the location of the personality in the VECTOR*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_back(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        { /*scope for creating the IOTHUBMESSAGE from GWMESSAGE*/

          /*gettng the GW message content*/
            STRICT_EXPECTED_CALL(mocks, Message_GetContent(MESSAGE_HANDLE_VALID_1));

            /*creating a new IOTHUB_MESSAGE*/
            whenShallIoTHubMessage_CreateFromByteArray_fail = currentIoTHubMessage_CreateFromByteArray_call + 1;
            STRICT_EXPECTED_CALL(mocks, IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, 1))
                .ValidateArgumentBuffer(1, CONSTBUFFER_VALID_CONTENT1.buffer, 1)
                ;
        }

        ///act
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_02_016: [ If adding a new personality to the vector fails, then `IoTHub_Receive` shall return. ]*/
    TEST_FUNCTION(IotHub_Receive_when_adding_the_personality_fails_it_fails)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(MESSAGE_HANDLE_VALID_1));
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "source"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceName"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceKey"));

        /*VECTOR_find_if incurs a STRING_c_str until it find the deviceName. None in this test*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        /*because the deviceName is brand new, it will be added as a new personality*/
        {/*separate scope for personality building*/
         /* create a new PERSONALITY */
            STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
                .IgnoreArgument(1);
            STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*making a copy of the deviceName*/
            STRICT_EXPECTED_CALL(mocks, STRING_construct("firstDevice"));
            STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*making a copy of the deviceKey*/
            STRICT_EXPECTED_CALL(mocks, STRING_construct("cheiaDeLaPoartaVerde"));
            STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG)) /*this is deviceName*/
                .IgnoreArgument(1);

            /*getting the stored IoTHubName*/
            STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
            /*getting the stored IoTHubSuffix*/
            STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*creating the IOTHUB_CLIENT_HANDLE associated with the device*/
            STRICT_EXPECTED_CALL(mocks, IoTHubClient_CreateWithTransport(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(1)
                .IgnoreArgument(2);
            STRICT_EXPECTED_CALL(mocks, IoTHubClient_Destroy(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            STRICT_EXPECTED_CALL(mocks, IoTHubClient_SetMessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(1)
                .IgnoreArgument(2)
                .IgnoreArgument(3);
        }

        /*adding the personality to the VECTOR or personalities*/
        whenShallVECTOR_push_back_fail = currentVECTOR_push_back_call + 1;
        STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        ///act
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_02_014: [ If creating the personality fails then `IotHub_Receive` shall return. ]*/
    TEST_FUNCTION(IotHub_Receive_when_creating_the_personality_fails_it_fails_1a)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(MESSAGE_HANDLE_VALID_1));
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "source"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceName"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceKey"));

        /*VECTOR_find_if incurs a STRING_c_str until it find the deviceName. None in this test*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        /*because the deviceName is brand new, it will be added as a new personality*/
        {/*separate scope for personality building*/
         /* create a new PERSONALITY */
            STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
                .IgnoreArgument(1);
            STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*making a copy of the deviceName*/
            STRICT_EXPECTED_CALL(mocks, STRING_construct("firstDevice"))
                .SetFailReturn((STRING_HANDLE)NULL);

        }

        ///act
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_02_014: [ If creating the personality fails then `IotHub_Receive` shall return. ]*/
    TEST_FUNCTION(IotHub_Receive_when_creating_the_personality_fails_it_fails_1b)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(MESSAGE_HANDLE_VALID_1));
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "source"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceName"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceKey"));

        /*VECTOR_find_if incurs a STRING_c_str until it find the deviceName. None in this test*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        /*because the deviceName is brand new, it will be added as a new personality*/
        {/*separate scope for personality building*/
         /* create a new PERSONALITY */
            STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
                .IgnoreArgument(1);
            STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

         /*making a copy of the deviceName*/
            STRICT_EXPECTED_CALL(mocks, STRING_construct("firstDevice"));
            STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*making a copy of the deviceKey*/
            STRICT_EXPECTED_CALL(mocks, STRING_construct("cheiaDeLaPoartaVerde"));
            STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG)) /*this is deviceName*/
                .IgnoreArgument(1);

            /*getting the stored IoTHubName*/
            STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
            /*getting the stored IoTHubSuffix*/
            STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*creating the IOTHUB_CLIENT_HANDLE associated with the device*/
            whenShallIoTHubClient_Create_fail = currentIoTHubClient_Create_call + 1;
            STRICT_EXPECTED_CALL(mocks, IoTHubClient_CreateWithTransport(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(1)
                .IgnoreArgument(2);

        }

        ///act
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_02_014: [ If creating the personality fails then `IotHub_Receive` shall return. ]*/
    TEST_FUNCTION(IotHub_Receive_when_creating_the_personality_fails_it_fails_1c)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(MESSAGE_HANDLE_VALID_1));
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "source"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceName"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceKey"));

        /*VECTOR_find_if incurs a STRING_c_str until it find the deviceName. None in this test*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        /*because the deviceName is brand new, it will be added as a new personality*/
        {/*separate scope for personality building*/
         /* create a new PERSONALITY */
            STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
                .IgnoreArgument(1);
            STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*making a copy of the deviceName*/
            STRICT_EXPECTED_CALL(mocks, STRING_construct("firstDevice"));
            STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*making a copy of the deviceKey*/
            STRICT_EXPECTED_CALL(mocks, STRING_construct("cheiaDeLaPoartaVerde"));
            STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG)) /*this is deviceName*/
                .IgnoreArgument(1);

            /*getting the stored IoTHubName*/
            STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
            /*getting the stored IoTHubSuffix*/
            STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*creating the IOTHUB_CLIENT_HANDLE associated with the device*/
            STRICT_EXPECTED_CALL(mocks, IoTHubClient_CreateWithTransport(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(1)
                .IgnoreArgument(2);
            STRICT_EXPECTED_CALL(mocks, IoTHubClient_Destroy(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            STRICT_EXPECTED_CALL(mocks, IoTHubClient_SetMessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument(1)
                .IgnoreArgument(2)
                .IgnoreArgument(3)
                .SetFailReturn(IOTHUB_CLIENT_ERROR);
        }

        ///act
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_02_014: [ If creating the personality fails then `IotHub_Receive` shall return. ]*/
    TEST_FUNCTION(IotHub_Receive_when_creating_the_personality_fails_it_fails_2)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(MESSAGE_HANDLE_VALID_1));
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "source"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceName"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceKey"));

        /*VECTOR_find_if incurs a STRING_c_str until it find the deviceName. None in this test*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        /*because the deviceName is brand new, it will be added as a new personality*/
        {/*separate scope for personality building*/
         /* create a new PERSONALITY */
            STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
                .IgnoreArgument(1);
            STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

         /*making a copy of the deviceName*/
            STRICT_EXPECTED_CALL(mocks, STRING_construct("firstDevice"));
            STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            /*making a copy of the deviceKey*/
            whenShallSTRING_construct_fail = currentSTRING_construct_call + 2;
            STRICT_EXPECTED_CALL(mocks, STRING_construct("cheiaDeLaPoartaVerde"));
        }

        ///act
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_02_014: [ If creating the personality fails then `IotHub_Receive` shall return. ]*/
    TEST_FUNCTION(IotHub_Receive_when_creating_the_personality_fails_it_fails_3)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(MESSAGE_HANDLE_VALID_1));
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "source"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceName"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceKey"));

        /*VECTOR_find_if incurs a STRING_c_str until it find the deviceName. None in this test*/
        STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        /*because the deviceName is brand new, it will be added as a new personality*/
        {/*separate scope for personality building*/
         /* create a new PERSONALITY */
            STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
                .IgnoreArgument(1)
                .SetFailReturn((void*)NULL);
        }

        ///act
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_02_012: [ If message properties do not contain a property called "deviceKey" having a non-`NULL` value then `IotHub_Receive` shall do nothing. ]*/
    TEST_FUNCTION(IotHub_Receive_when_deviceKey_doesn_t_exist_returns)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(MESSAGE_HANDLE_VALID_1));
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "source"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceName"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceKey"))
            .SetReturn((const char*)NULL);

        ///act
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_02_011: [ If message properties do not contain a property called "deviceName" having a non-`NULL` value then `IotHub_Receive` shall do nothing. ]*/
    TEST_FUNCTION(IotHub_Receive_when_deviceName_doesn_t_exist_returns)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(MESSAGE_HANDLE_VALID_1));
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "source"));

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "deviceName"))
            .SetReturn((const char*)NULL);

        ///act
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_02_010: [ If message properties do not contain a property called "source" having the value set to "mapping" then `IotHub_Receive` shall do nothing. ]*/
    TEST_FUNCTION(IotHub_Receive_when_source_mapping_doesn_t_exist_returns)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(MESSAGE_HANDLE_VALID_1));
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(CONSTMAP_HANDLE_VALID_1, "source"))
            .SetReturn((const char*)NULL);

        ///act
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_17_007: [ `IotHub_ReceiveMessageCallback` shall get properties from message by calling `IoTHubMessage_Properties`. ]*/
    /*Tests_SRS_IOTHUBMODULE_17_009: [ `IotHub_ReceiveMessageCallback` shall define a property "source" as "iothub". ]*/
    /*Tests_SRS_IOTHUBMODULE_17_010: [ `IotHub_ReceiveMessageCallback` shall define a property "deviceName" as the `PERSONALITY`'s deviceName. ]*/
    /*Tests_SRS_IOTHUBMODULE_17_011: [ `IotHub_ReceiveMessageCallback` shall combine message properties with the "source" and "deviceName" properties. ]*/
    /*Tests_SRS_IOTHUBMODULE_17_014: [ If Message content type is `IOTHUBMESSAGE_STRING`, `IotHub_ReceiveMessageCallback` shall get the buffer from results of `IoTHubMessage_GetString`. ]*/
    /*Tests_SRS_IOTHUBMODULE_17_015: [ If Message content type is `IOTHUBMESSAGE_STRING`, `IotHub_ReceiveMessageCallback` shall get the buffer size from the string length. ]*/
    /*Tests_SRS_IOTHUBMODULE_17_016: [ `IotHub_ReceiveMessageCallback` shall create a new message from combined properties, the size and buffer. ]*/
    /*Tests_SRS_IOTHUBMODULE_17_018: [ `IotHub_ReceiveMessageCallback` shall call `Broker_Publish` with the new message, this module's handle, and the `broker`. ]*/
    /*Tests_SRS_IOTHUBMODULE_17_020: [ `IotHub_ReceiveMessageCallback` shall destroy all resources it creates. ]*/
    /*Tests_SRS_IOTHUBMODULE_17_021: [ Upon success, `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ACCEPTED`. ]*/
    TEST_FUNCTION(IotHub_callback_string_success)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);
        IOTHUB_MESSAGE_HANDLE hubMsg = IOTHUB_MESSAGE_HANDLE_VALID;
        mocks.ResetAllCalls();

        IotHub_Receive_message_content = "a message";
        IotHub_Receive_message_size = 9;

        STRICT_EXPECTED_CALL(mocks, IoTHubMessage_GetContentType(hubMsg))
            .SetReturn(IOTHUBMESSAGE_STRING);
        STRICT_EXPECTED_CALL(mocks, IoTHubMessage_GetString(hubMsg));
        STRICT_EXPECTED_CALL(mocks, IoTHubMessage_Properties(hubMsg))
            .SetReturn(MAP_HANDLE_VALID_1);
        STRICT_EXPECTED_CALL(mocks, Map_Add(MAP_HANDLE_VALID_1, GW_SOURCE_PROPERTY, GW_IOTHUB_MODULE));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Map_Add(MAP_HANDLE_VALID_1, GW_DEVICENAME_PROPERTY, "firstDevice"));
        STRICT_EXPECTED_CALL(mocks, Message_Create(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Broker_Publish(BROKER_HANDLE_VALID, module, IGNORED_PTR_ARG))
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, Message_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act

        // IotHub_Receive_message_callback_function and IotHub_Receive_message_userContext set in mock
        auto result = IotHub_Receive_message_callback_function(hubMsg, IotHub_Receive_message_userContext);


        ///assert
        ASSERT_ARE_EQUAL(IOTHUBMESSAGE_DISPOSITION_RESULT, result, IOTHUBMESSAGE_ACCEPTED);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_17_007: [ `IotHub_ReceiveMessageCallback` shall get properties from message by calling `IoTHubMessage_Properties`. ]*/
    /*Tests_SRS_IOTHUBMODULE_17_009: [ `IotHub_ReceiveMessageCallback` shall define a property "source" as "iothub". ]*/
    /*Tests_SRS_IOTHUBMODULE_17_010: [ `IotHub_ReceiveMessageCallback` shall define a property "deviceName" as the `PERSONALITY`'s deviceName. ]*/
    /*Tests_SRS_IOTHUBMODULE_17_011: [ `IotHub_ReceiveMessageCallback` shall combine message properties with the "source" and "deviceName" properties. ]*/
    /*Tests_SRS_IOTHUBMODULE_17_013: [ If Message content type is `IOTHUBMESSAGE_BYTEARRAY`, `IotHub_ReceiveMessageCallback` shall get the size and buffer from the  results of `IoTHubMessage_GetByteArray`. ]*/
    /*Tests_SRS_IOTHUBMODULE_17_016: [ `IotHub_ReceiveMessageCallback` shall create a new message from combined properties, the size and buffer. ]*/
    /*Tests_SRS_IOTHUBMODULE_17_018: [ `IotHub_ReceiveMessageCallback` shall call `Broker_Publish` with the new message, this module's handle, and the `broker`. ]*/
    /*Tests_SRS_IOTHUBMODULE_17_020: [ `IotHub_ReceiveMessageCallback` shall destroy all resources it creates. ]*/
    /*Tests_SRS_IOTHUBMODULE_17_021: [ Upon success, `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ACCEPTED`. ]*/
    TEST_FUNCTION(IotHub_callback_byte_array_success)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);
        IOTHUB_MESSAGE_HANDLE hubMsg = IOTHUB_MESSAGE_HANDLE_VALID;
        mocks.ResetAllCalls();

        IotHub_Receive_message_content = "a message";
        IotHub_Receive_message_size = 9;

        STRICT_EXPECTED_CALL(mocks, IoTHubMessage_GetContentType(hubMsg))
            .SetReturn(IOTHUBMESSAGE_BYTEARRAY);
        STRICT_EXPECTED_CALL(mocks, IoTHubMessage_GetByteArray(hubMsg, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, IoTHubMessage_Properties(hubMsg))
            .SetReturn(MAP_HANDLE_VALID_1);
        STRICT_EXPECTED_CALL(mocks, Map_Add(MAP_HANDLE_VALID_1, GW_SOURCE_PROPERTY, GW_IOTHUB_MODULE));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Map_Add(MAP_HANDLE_VALID_1, GW_DEVICENAME_PROPERTY, "firstDevice"));
        STRICT_EXPECTED_CALL(mocks, Message_Create(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Broker_Publish(BROKER_HANDLE_VALID, module, IGNORED_PTR_ARG))
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, Message_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act

        // IotHub_Receive_message_callback_function and IotHub_Receive_message_userContext set in mock
        auto result = IotHub_Receive_message_callback_function(hubMsg, IotHub_Receive_message_userContext);


        ///assert
        ASSERT_ARE_EQUAL(IOTHUBMESSAGE_DISPOSITION_RESULT, result, IOTHUBMESSAGE_ACCEPTED);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_17_019: [ If the message fails to publish, `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_REJECTED`. ]*/
    TEST_FUNCTION(IotHub_callback_broker_publish_fails)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);
        IOTHUB_MESSAGE_HANDLE hubMsg = IOTHUB_MESSAGE_HANDLE_VALID;
        mocks.ResetAllCalls();

        IotHub_Receive_message_content = "a message";
        IotHub_Receive_message_size = 9;

        STRICT_EXPECTED_CALL(mocks, IoTHubMessage_GetContentType(hubMsg))
            .SetReturn(IOTHUBMESSAGE_BYTEARRAY);
        STRICT_EXPECTED_CALL(mocks, IoTHubMessage_GetByteArray(hubMsg, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, IoTHubMessage_Properties(hubMsg))
            .SetReturn(MAP_HANDLE_VALID_1);
        STRICT_EXPECTED_CALL(mocks, Map_Add(MAP_HANDLE_VALID_1, GW_SOURCE_PROPERTY, GW_IOTHUB_MODULE));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Map_Add(MAP_HANDLE_VALID_1, GW_DEVICENAME_PROPERTY, "firstDevice"));
        STRICT_EXPECTED_CALL(mocks, Message_Create(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Broker_Publish(BROKER_HANDLE_VALID, module, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .SetFailReturn(BROKER_ERROR);
        STRICT_EXPECTED_CALL(mocks, Message_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act

        // IotHub_Receive_message_callback_function and IotHub_Receive_message_userContext set in mock
        auto result = IotHub_Receive_message_callback_function(hubMsg, IotHub_Receive_message_userContext);


        ///assert
        ASSERT_ARE_EQUAL(IOTHUBMESSAGE_DISPOSITION_RESULT, result, IOTHUBMESSAGE_REJECTED);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_17_017: [ If the message fails to create, `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_REJECTED`. ]*/
    TEST_FUNCTION(IotHub_callback_message_create_fails)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);
        IOTHUB_MESSAGE_HANDLE hubMsg = IOTHUB_MESSAGE_HANDLE_VALID;
        mocks.ResetAllCalls();

        IotHub_Receive_message_content = "a message";
        IotHub_Receive_message_size = 9;

        STRICT_EXPECTED_CALL(mocks, IoTHubMessage_GetContentType(hubMsg))
            .SetReturn(IOTHUBMESSAGE_BYTEARRAY);
        STRICT_EXPECTED_CALL(mocks, IoTHubMessage_GetByteArray(hubMsg, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, IoTHubMessage_Properties(hubMsg))
            .SetReturn(MAP_HANDLE_VALID_1);
        STRICT_EXPECTED_CALL(mocks, Map_Add(MAP_HANDLE_VALID_1, GW_SOURCE_PROPERTY, GW_IOTHUB_MODULE));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Map_Add(MAP_HANDLE_VALID_1, GW_DEVICENAME_PROPERTY, "firstDevice"));
        STRICT_EXPECTED_CALL(mocks, Message_Create(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((MESSAGE_HANDLE)NULL);

        ///act

        // IotHub_Receive_message_callback_function and IotHub_Receive_message_userContext set in mock
        auto result = IotHub_Receive_message_callback_function(hubMsg, IotHub_Receive_message_userContext);


        ///assert
        ASSERT_ARE_EQUAL(IOTHUBMESSAGE_DISPOSITION_RESULT, result, IOTHUBMESSAGE_REJECTED);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_17_022: [ If message properties fail to combine, `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ABANDONED`. ]*/
    TEST_FUNCTION(IotHub_callback_Map_Add_2_fails)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);
        IOTHUB_MESSAGE_HANDLE hubMsg = IOTHUB_MESSAGE_HANDLE_VALID;
        mocks.ResetAllCalls();

        IotHub_Receive_message_content = "a message";
        IotHub_Receive_message_size = 9;

        STRICT_EXPECTED_CALL(mocks, IoTHubMessage_GetContentType(hubMsg))
            .SetReturn(IOTHUBMESSAGE_BYTEARRAY);
        STRICT_EXPECTED_CALL(mocks, IoTHubMessage_GetByteArray(hubMsg, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, IoTHubMessage_Properties(hubMsg))
            .SetReturn(MAP_HANDLE_VALID_1);
        STRICT_EXPECTED_CALL(mocks, Map_Add(MAP_HANDLE_VALID_1, GW_SOURCE_PROPERTY, GW_IOTHUB_MODULE));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Map_Add(MAP_HANDLE_VALID_1, GW_DEVICENAME_PROPERTY, "firstDevice"))
            .SetFailReturn(MAP_ERROR);


        ///act

        // IotHub_Receive_message_callback_function and IotHub_Receive_message_userContext set in mock
        auto result = IotHub_Receive_message_callback_function(hubMsg, IotHub_Receive_message_userContext);


        ///assert
        ASSERT_ARE_EQUAL(IOTHUBMESSAGE_DISPOSITION_RESULT, result, IOTHUBMESSAGE_ABANDONED);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_17_022: [ If message properties fail to combine, `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ABANDONED`. ]*/
    TEST_FUNCTION(IotHub_callback_Map_Add_1_fails)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);
        IOTHUB_MESSAGE_HANDLE hubMsg = IOTHUB_MESSAGE_HANDLE_VALID;
        mocks.ResetAllCalls();

        IotHub_Receive_message_content = "a message";
        IotHub_Receive_message_size = 9;

        STRICT_EXPECTED_CALL(mocks, IoTHubMessage_GetContentType(hubMsg))
            .SetReturn(IOTHUBMESSAGE_BYTEARRAY);
        STRICT_EXPECTED_CALL(mocks, IoTHubMessage_GetByteArray(hubMsg, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, IoTHubMessage_Properties(hubMsg))
            .SetReturn(MAP_HANDLE_VALID_1);
        STRICT_EXPECTED_CALL(mocks, Map_Add(MAP_HANDLE_VALID_1, GW_SOURCE_PROPERTY, GW_IOTHUB_MODULE))
            .SetFailReturn(MAP_ERROR);


        ///act

        // IotHub_Receive_message_callback_function and IotHub_Receive_message_userContext set in mock
        auto result = IotHub_Receive_message_callback_function(hubMsg, IotHub_Receive_message_userContext);


        ///assert
        ASSERT_ARE_EQUAL(IOTHUBMESSAGE_DISPOSITION_RESULT, result, IOTHUBMESSAGE_ABANDONED);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_17_008: [ If message properties are `NULL`, `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ABANDONED`. ]*/
    TEST_FUNCTION(IotHub_callback_message_properties_fails)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);
        IOTHUB_MESSAGE_HANDLE hubMsg = IOTHUB_MESSAGE_HANDLE_VALID;
        mocks.ResetAllCalls();

        IotHub_Receive_message_content = "a message";
        IotHub_Receive_message_size = 9;

        STRICT_EXPECTED_CALL(mocks, IoTHubMessage_GetContentType(hubMsg))
            .SetReturn(IOTHUBMESSAGE_BYTEARRAY);
        STRICT_EXPECTED_CALL(mocks, IoTHubMessage_GetByteArray(hubMsg, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, IoTHubMessage_Properties(hubMsg))
            .SetReturn(MAP_HANDLE_VALID_1)
            .SetFailReturn((MAP_HANDLE)NULL);

        ///act

        // IotHub_Receive_message_callback_function and IotHub_Receive_message_userContext set in mock
        auto result = IotHub_Receive_message_callback_function(hubMsg, IotHub_Receive_message_userContext);


        ///assert
        ASSERT_ARE_EQUAL(IOTHUBMESSAGE_DISPOSITION_RESULT, result, IOTHUBMESSAGE_ABANDONED);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_17_023: [ If `IoTHubMessage_GetByteArray` fails, `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ABANDONED`. ]*/
    TEST_FUNCTION(IotHub_callback_message_get_byte_array_fails)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);
        IOTHUB_MESSAGE_HANDLE hubMsg = IOTHUB_MESSAGE_HANDLE_VALID;
        mocks.ResetAllCalls();

        IotHub_Receive_message_content = "a message";
        IotHub_Receive_message_size = 9;

        STRICT_EXPECTED_CALL(mocks, IoTHubMessage_GetContentType(hubMsg))
            .SetReturn(IOTHUBMESSAGE_BYTEARRAY);
        STRICT_EXPECTED_CALL(mocks, IoTHubMessage_GetByteArray(hubMsg, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .SetFailReturn(IOTHUB_MESSAGE_ERROR);

        ///act

        // IotHub_Receive_message_callback_function and IotHub_Receive_message_userContext set in mock
        auto result = IotHub_Receive_message_callback_function(hubMsg, IotHub_Receive_message_userContext);


        ///assert
        ASSERT_ARE_EQUAL(IOTHUBMESSAGE_DISPOSITION_RESULT, result, IOTHUBMESSAGE_ABANDONED);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_17_006: [ If Message Content type is `IOTHUBMESSAGE_UNKNOWN`, then `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ABANDONED`. ]*/
    TEST_FUNCTION(IotHub_callback_message_content_unknown_fails)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);
        IOTHUB_MESSAGE_HANDLE hubMsg = IOTHUB_MESSAGE_HANDLE_VALID;
        mocks.ResetAllCalls();

        IotHub_Receive_message_content = "a message";
        IotHub_Receive_message_size = 9;

        STRICT_EXPECTED_CALL(mocks, IoTHubMessage_GetContentType(hubMsg))
            .SetReturn(IOTHUBMESSAGE_UNKNOWN);

        ///act

        // IotHub_Receive_message_callback_function and IotHub_Receive_message_userContext set in mock
        auto result = IotHub_Receive_message_callback_function(hubMsg, IotHub_Receive_message_userContext);


        ///assert
        ASSERT_ARE_EQUAL(IOTHUBMESSAGE_DISPOSITION_RESULT, result, IOTHUBMESSAGE_ABANDONED);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

    /*Tests_SRS_IOTHUBMODULE_17_005: [ If `userContextCallback` is `NULL`, then `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ABANDONED`. ]*/
    TEST_FUNCTION(IotHub_callback_message_user_context_null)
    {
        ///arrange
        IotHubMocks mocks;
        AutoConfig config;
        auto module = Module_Create(BROKER_HANDLE_VALID, config);
        Module_Receive(module, MESSAGE_HANDLE_VALID_1);
        IOTHUB_MESSAGE_HANDLE hubMsg = IOTHUB_MESSAGE_HANDLE_VALID;
        mocks.ResetAllCalls();

        IotHub_Receive_message_content = "a message";
        IotHub_Receive_message_size = 9;


        ///act

        // IotHub_Receive_message_callback_function and IotHub_Receive_message_userContext set in mock
        auto result = IotHub_Receive_message_callback_function(hubMsg, NULL);


        ///assert
        ASSERT_ARE_EQUAL(IOTHUBMESSAGE_DISPOSITION_RESULT, result, IOTHUBMESSAGE_ABANDONED);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(module);
    }

END_TEST_SUITE(iothub_ut)
