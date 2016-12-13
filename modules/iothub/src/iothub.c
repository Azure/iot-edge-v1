// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <ctype.h>
#include "azure_c_shared_utility/gballoc.h"

#include "iothub.h"
#include "iothub_client.h"
#include "iothubtransport.h"
#include "iothubtransporthttp.h"
#include "iothubtransportamqp.h"
#include "iothubtransportmqtt.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/strings.h"
#include "messageproperties.h"
#include "broker.h"

#include <parson.h>

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

#define SOURCE "source"
#define MAPPING "mapping"
#define DEVICENAME "deviceName"
#define DEVICEKEY "deviceKey"
#define SUFFIX "IoTHubSuffix"
#define HUBNAME "IoTHubName"
#define TRANSPORT "Transport"

static int strcmp_i(const char* lhs, const char* rhs)
{
    char lc, rc;
    int cmp;

    do
    {
        lc = *lhs++;
        rc = *rhs++;
        cmp = tolower(lc) - tolower(rc);
    } while (cmp == 0 && lc != 0 && rc != 0);

    return cmp;
}

static void* IotHub_ParseConfigurationFromJson(const char* configuration)
{
    IOTHUB_CONFIG* result;
    if (configuration == NULL)
    {
        /*Codes_SRS_IOTHUBMODULE_05_002: [ If `configuration` is NULL then `IotHub_ParseConfigurationFromJson` shall fail and return NULL. ]*/
        LogError("NULL parameter detected, configuration=[%p]", configuration);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_IOTHUBMODULE_05_004: [ `IotHub_ParseConfigurationFromJson` shall parse `configuration` as a JSON string. ]*/
        JSON_Value* json = json_parse_string((const char*)configuration);
        if (json == NULL)
        {
            /*Codes_SRS_IOTHUBMODULE_05_005: [ If parsing of `configuration` fails, `IotHub_ParseConfigurationFromJson` shall fail and return NULL. ]*/
            LogError("Unable to parse json string");
            result = NULL;
        }
        else
        {
            JSON_Object* obj = json_value_get_object(json);
            if (obj == NULL)
            {
                /*Codes_SRS_IOTHUBMODULE_05_005: [ If parsing of `configuration` fails, `IotHub_ParseConfigurationFromJson` shall fail and return NULL. ]*/
                LogError("Expected a JSON Object in configuration");
                result = NULL;
            }
            else
            {
                const char * IoTHubName;
                const char * IoTHubSuffix;
                const char * transport;
                if ((IoTHubName = json_object_get_string(obj, HUBNAME)) == NULL)
                {
                    /*Codes_SRS_IOTHUBMODULE_05_006: [ If the JSON object does not contain a value named "IoTHubName" then `IotHub_ParseConfigurationFromJson` shall fail and return NULL. ]*/
                    LogError("Did not find expected %s configuration", HUBNAME);
                    result = NULL;
                }
                else if ((IoTHubSuffix = json_object_get_string(obj, SUFFIX)) == NULL)
                {
                    /*Codes_SRS_IOTHUBMODULE_05_007: [ If the JSON object does not contain a value named "IoTHubSuffix" then `IotHub_ParseConfigurationFromJson` shall fail and return NULL. ]*/
                    LogError("Did not find expected %s configuration", SUFFIX);
                    result = NULL;
                }
                else if ((transport = json_object_get_string(obj, TRANSPORT)) == NULL)
                {
                    /*Codes_SRS_IOTHUBMODULE_05_011: [ If the JSON object does not contain a value named "Transport" then `IotHub_ParseConfigurationFromJson` shall fail and return NULL. ]*/
                    LogError("Did not find expected %s configuration", TRANSPORT);
                    result = NULL;
                }
                else
                {
                    char* name;
                    char* suffix;
                    IOTHUB_CONFIG* config;
                    if ((name = malloc(strlen(IoTHubName) + 1)) == NULL)
                    {
                        LogError("Could not allocate memory for IoTHubName");
                        result = NULL;
                    }
                    else if ((suffix = malloc(strlen(IoTHubSuffix) + 1)) == NULL)
                    {
                        LogError("Could not allocate memory for IoTHubSuffix");
                        free(name);
                        result = NULL;
                    }
                    else if ((config = malloc(sizeof(IOTHUB_CONFIG))) == NULL)
                    {
                        LogError("Could not allocate memory for configuration");
                        free(name);
                        free(suffix);
                        result = NULL;
                    }
                    else
                    {
                        /*Codes_SRS_IOTHUBMODULE_05_012: [ If the value of "Transport" is not one of "HTTP", "AMQP", or "MQTT" (case-insensitive) then `IotHub_ParseConfigurationFromJson` shall fail and return NULL. ]*/
                        if (strcmp_i(transport, "HTTP") == 0)
                        {
                            config->transportProvider = HTTP_Protocol;
                        }
                        else if (strcmp_i(transport, "AMQP") == 0)
                        {
                            config->transportProvider = AMQP_Protocol;
                        }
                        else if (strcmp_i(transport, "MQTT") == 0)
                        {
                            config->transportProvider = MQTT_Protocol;
                        }
                        else
                        {
                            free(name);
                            free(suffix);
                            free(config);
                            config = NULL;
                        }

                        if (config != NULL)
                        {
                            strcpy(name, IoTHubName);
                            strcpy(suffix, IoTHubSuffix);
                            config->IoTHubName = name;
                            config->IoTHubSuffix = suffix;
                        }

                        result = config;
                    }
                }
            }
            json_value_free(json);
        }
    }
    return result;
}

static void IotHub_FreeConfiguration(void* configuration)
{
    /*Codes_SRS_IOTHUBMODULE_05_014: [ If `configuration` is NULL then `IotHub_FreeConfiguration` shall do nothing. ]*/
    if (configuration != NULL)
    {
        IOTHUB_CONFIG* config = (IOTHUB_CONFIG*)configuration;

        /*Codes_SRS_IOTHUBMODULE_05_015: [ `IotHub_FreeConfiguration` shall free the strings referenced by the `IoTHubName` and `IoTHubSuffix` data members, and then free the `IOTHUB_CONFIG` structure itself. ]*/
        free((void*)config->IoTHubName);
        free((void*)config->IoTHubSuffix);
        free(config);
    }
}

static MODULE_HANDLE IotHub_Create(BROKER_HANDLE broker, const void* configuration)
{
    IOTHUB_HANDLE_DATA *result;
    const IOTHUB_CONFIG* config = configuration;

    /*Codes_SRS_IOTHUBMODULE_02_001: [ If `broker` is `NULL` then `IotHub_Create` shall fail and return `NULL`. ]*/
    /*Codes_SRS_IOTHUBMODULE_02_002: [ If `configuration` is `NULL` then `IotHub_Create` shall fail and return `NULL`. ]*/
    /*Codes_SRS_IOTHUBMODULE_02_003: [ If `configuration->IoTHubName` is `NULL` then `IotHub_Create` shall and return `NULL`. ]*/
    /*Codes_SRS_IOTHUBMODULE_02_004: [ If `configuration->IoTHubSuffix` is `NULL` then `IotHub_Create` shall fail and return `NULL`. ]*/
    if (
        (broker == NULL) ||
        (configuration == NULL)
        )
    {
        LogError("invalid arg broker=%p, configuration=%p", broker, configuration);
        result = NULL;
    }
    else if (
        (config->IoTHubName == NULL) ||
        (config->IoTHubSuffix == NULL) ||
        (config->transportProvider == NULL)
        )
    {
        LogError("invalid configuration IoTHubName=%s IoTHubSuffix=%s transportProvider=%p",
            (config != NULL) ? config->IoTHubName : "<null>",
            (config != NULL) ? config->IoTHubSuffix : "<null>",
            (config != NULL) ? config->transportProvider : 0);
        result = NULL;
    }
    else
    {
        result = malloc(sizeof(IOTHUB_HANDLE_DATA));
        /*Codes_SRS_IOTHUBMODULE_02_027: [ When `IotHub_Create` encounters an internal failure it shall fail and return `NULL`. ]*/
        if (result == NULL)
        {
            LogError("malloc returned NULL");
            /*return as is*/
        }
        else
        {
            /*Codes_SRS_IOTHUBMODULE_02_006: [ `IotHub_Create` shall create an empty `VECTOR` containing pointers to `PERSONALITY`s. ]*/
            result->personalities = VECTOR_create(sizeof(PERSONALITY_PTR));
            if (result->personalities == NULL)
            {
                /*Codes_SRS_IOTHUBMODULE_02_007: [ If creating the personality vector fails then `IotHub_Create` shall fail and return `NULL`. ]*/
                free(result);
                result = NULL;
                LogError("VECTOR_create returned NULL");
            }
            else
            {
                result->transportProvider = config->transportProvider;
                if (result->transportProvider == HTTP_Protocol ||
                    result->transportProvider == AMQP_Protocol)
                {
                    /*Codes_SRS_IOTHUBMODULE_17_001: [ If `configuration->transportProvider` is `HTTP_Protocol` or `AMQP_Protocol`, `IotHub_Create` shall create a shared transport by calling `IoTHubTransport_Create`. ]*/
                    result->transportHandle = IoTHubTransport_Create(config->transportProvider, config->IoTHubName, config->IoTHubSuffix);
                    if (result->transportHandle == NULL)
                    {
                        /*Codes_SRS_IOTHUBMODULE_17_002: [ If creating the shared transport fails, `IotHub_Create` shall fail and return `NULL`. ]*/
                        VECTOR_destroy(result->personalities);
                        free(result);
                        result = NULL;
                        LogError("VECTOR_create returned NULL");
                    }
                }
                else
                {
                    result->transportHandle = NULL;
                }

                if (result != NULL)
                {
                    /*Codes_SRS_IOTHUBMODULE_02_028: [ `IotHub_Create` shall create a copy of `configuration->IoTHubName`. ]*/
                    /*Codes_SRS_IOTHUBMODULE_02_029: [ `IotHub_Create` shall create a copy of `configuration->IoTHubSuffix`. ]*/
                    if ((result->IoTHubName = STRING_construct(config->IoTHubName)) == NULL)
                    {
                        LogError("STRING_construct returned NULL");
                        IoTHubTransport_Destroy(result->transportHandle);
                        VECTOR_destroy(result->personalities);
                        free(result);
                        result = NULL;
                    }
                    else if ((result->IoTHubSuffix = STRING_construct(config->IoTHubSuffix)) == NULL)
                    {
                        LogError("STRING_construct returned NULL");
                        STRING_delete(result->IoTHubName);
                        IoTHubTransport_Destroy(result->transportHandle);
                        VECTOR_destroy(result->personalities);
                        free(result);
                        result = NULL;
                    }
                    else
                    {
                        /*Codes_SRS_IOTHUBMODULE_17_004: [ `IotHub_Create` shall store the broker. ]*/
                        result->broker = broker;
                        /*Codes_SRS_IOTHUBMODULE_02_008: [ Otherwise, `IotHub_Create` shall return a non-`NULL` handle. ]*/
                    }
                }
            }
        }
    }
    return result;
}

static void IotHub_Destroy(MODULE_HANDLE moduleHandle)
{
    /*Codes_SRS_IOTHUBMODULE_02_023: [ If `moduleHandle` is `NULL` then `IotHub_Destroy` shall return. ]*/
    if (moduleHandle == NULL)
    {
        LogError("moduleHandle parameter was NULL");
    }
    else
    {
        /*Codes_SRS_IOTHUBMODULE_02_024: [ Otherwise `IotHub_Destroy` shall free all used resources. ]*/
        IOTHUB_HANDLE_DATA * handleData = moduleHandle;
        size_t vectorSize = VECTOR_size(handleData->personalities);
        for (size_t i = 0; i < vectorSize; i++)
        {
            PERSONALITY_PTR* personality = VECTOR_element(handleData->personalities, i);
            STRING_delete((*personality)->deviceKey);
            STRING_delete((*personality)->deviceName);
            IoTHubClient_Destroy((*personality)->iothubHandle);
            free(*personality);
        }
        IoTHubTransport_Destroy(handleData->transportHandle);
        VECTOR_destroy(handleData->personalities);
        STRING_delete(handleData->IoTHubName);
        STRING_delete(handleData->IoTHubSuffix);
        free(handleData);
    }
}

static bool lookup_DeviceName(const void* element, const void* value)
{
    return (strcmp(STRING_c_str((*(PERSONALITY_PTR*)element)->deviceName), value) == 0);
}

static IOTHUBMESSAGE_DISPOSITION_RESULT IotHub_ReceiveMessageCallback(IOTHUB_MESSAGE_HANDLE msg, void* userContextCallback)
{
    IOTHUBMESSAGE_DISPOSITION_RESULT result;
    if (userContextCallback == NULL)
    {
        /*Codes_SRS_IOTHUBMODULE_17_005: [ If `userContextCallback` is `NULL`, then `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ABANDONED`. ]*/
        LogError("No context to associate message");
        result = IOTHUBMESSAGE_ABANDONED;
    }
    else
    {
        PERSONALITY_PTR personality = (PERSONALITY_PTR)userContextCallback;
        IOTHUBMESSAGE_CONTENT_TYPE msgContentType = IoTHubMessage_GetContentType(msg);
        if (msgContentType == IOTHUBMESSAGE_UNKNOWN)
        {
            /*Codes_SRS_IOTHUBMODULE_17_006: [ If Message Content type is `IOTHUBMESSAGE_UNKNOWN`, then `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ABANDONED`. ]*/
            LogError("Message content type is unknown");
            result = IOTHUBMESSAGE_ABANDONED;
        }
        else
        {
            /* Handle message content */
            MESSAGE_CONFIG newMessageConfig;
            IOTHUB_MESSAGE_RESULT msgResult;
            if (msgContentType == IOTHUBMESSAGE_STRING)
            {
                /*Codes_SRS_IOTHUBMODULE_17_014: [ If Message content type is `IOTHUBMESSAGE_STRING`, `IotHub_ReceiveMessageCallback` shall get the buffer from results of `IoTHubMessage_GetString`. ]*/
                const char* sourceStr = IoTHubMessage_GetString(msg);
                newMessageConfig.source = (const unsigned char*)sourceStr;
                /*Codes_SRS_IOTHUBMODULE_17_015: [ If Message content type is `IOTHUBMESSAGE_STRING`, `IotHub_ReceiveMessageCallback` shall get the buffer size from the string length. ]*/
                newMessageConfig.size = strlen(sourceStr);
                msgResult = IOTHUB_MESSAGE_OK;
            }
            else
            {
                /* content type is byte array */
                /*Codes_SRS_IOTHUBMODULE_17_013: [ If Message content type is `IOTHUBMESSAGE_BYTEARRAY`, `IotHub_ReceiveMessageCallback` shall get the size and buffer from the  results of `IoTHubMessage_GetByteArray`. ]*/
                msgResult = IoTHubMessage_GetByteArray(msg, &(newMessageConfig.source), &(newMessageConfig.size));
            }
            if (msgResult != IOTHUB_MESSAGE_OK)
            {
                /*Codes_SRS_IOTHUBMODULE_17_023: [ If `IoTHubMessage_GetByteArray` fails, `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ABANDONED`. ]*/
                LogError("Failed to get message content");
                result = IOTHUBMESSAGE_ABANDONED;
            }
            else
            {
                /* Now, handle message properties. */
                MAP_HANDLE newProperties;
                /*Codes_SRS_IOTHUBMODULE_17_007: [ `IotHub_ReceiveMessageCallback` shall get properties from message by calling `IoTHubMessage_Properties`. ]*/
                newProperties = IoTHubMessage_Properties(msg);
                if (newProperties == NULL)
                {
                    /*Codes_SRS_IOTHUBMODULE_17_008: [ If message properties are `NULL`, `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ABANDONED`. ]*/
                    LogError("No Properties in IoTHub Message");
                    result = IOTHUBMESSAGE_ABANDONED;
                }
                else
                {
                    /*Codes_SRS_IOTHUBMODULE_17_009: [ `IotHub_ReceiveMessageCallback` shall define a property "source" as "iothub". ]*/
                    /*Codes_SRS_IOTHUBMODULE_17_010: [ `IotHub_ReceiveMessageCallback` shall define a property "deviceName" as the `PERSONALITY`'s deviceName. ]*/
                    /*Codes_SRS_IOTHUBMODULE_17_011: [ `IotHub_ReceiveMessageCallback` shall combine message properties with the "source" and "deviceName" properties. ]*/
                    if (Map_Add(newProperties, GW_SOURCE_PROPERTY, GW_IOTHUB_MODULE) != MAP_OK)
                    {
                        /*Codes_SRS_IOTHUBMODULE_17_022: [ If message properties fail to combine, `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ABANDONED`. ]*/
                        LogError("Property [%s] did not add properly", GW_SOURCE_PROPERTY);
                        result = IOTHUBMESSAGE_ABANDONED;
                    }
                    else if (Map_Add(newProperties, GW_DEVICENAME_PROPERTY, STRING_c_str(personality->deviceName)) != MAP_OK)
                    {
                        /*Codes_SRS_IOTHUBMODULE_17_022: [ If message properties fail to combine, `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ABANDONED`. ]*/
                        LogError("Property [%s] did not add properly", GW_DEVICENAME_PROPERTY);
                        result = IOTHUBMESSAGE_ABANDONED;
                    }
                    else
                    {
                        /*Codes_SRS_IOTHUBMODULE_17_016: [ `IotHub_ReceiveMessageCallback` shall create a new message from combined properties, the size and buffer. ]*/
                        newMessageConfig.sourceProperties = newProperties;
                        MESSAGE_HANDLE gatewayMsg = Message_Create(&newMessageConfig);
                        if (gatewayMsg == NULL)
                        {
                            /*Codes_SRS_IOTHUBMODULE_17_017: [ If the message fails to create, `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_REJECTED`. ]*/
                            LogError("Failed to create gateway message");
                            result = IOTHUBMESSAGE_REJECTED;
                        }
                        else
                        {
                            /*Codes_SRS_IOTHUBMODULE_17_018: [ `IotHub_ReceiveMessageCallback` shall call `Broker_Publish` with the new message, this module's handle, and the `broker`. ]*/
                            if (Broker_Publish(personality->broker, personality->module, gatewayMsg) != BROKER_OK)
                            {
                                /*Codes_SRS_IOTHUBMODULE_17_019: [ If the message fails to publish, `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_REJECTED`. ]*/
                                LogError("Failed to publish gateway message");
                                result = IOTHUBMESSAGE_REJECTED;
                            }
                            else
                            {
                                /*Codes_SRS_IOTHUBMODULE_17_021: [ Upon success, `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ACCEPTED`. ]*/
                                result = IOTHUBMESSAGE_ACCEPTED;
                            }
                            /*Codes_SRS_IOTHUBMODULE_17_020: [ `IotHub_ReceiveMessageCallback` shall destroy all resources it creates. ]*/
                            Message_Destroy(gatewayMsg);
                        }
                    }
                }
            }
        }
    }
    return result;
}

/*returns non-null if PERSONALITY has been properly populated*/
static PERSONALITY_PTR PERSONALITY_create(const char* deviceName, const char* deviceKey, IOTHUB_HANDLE_DATA* moduleHandleData)
{
    PERSONALITY_PTR result = (PERSONALITY_PTR)malloc(sizeof(PERSONALITY));
    if (result == NULL)
    {
        LogError("unable to allocate a personality for the device %s", deviceName);
    }
    else
    {
        if ((result->deviceName = STRING_construct(deviceName)) == NULL)
        {
            LogError("unable to STRING_construct");
            free(result);
            result = NULL;
        }
        else if ((result->deviceKey = STRING_construct(deviceKey)) == NULL)
        {
            LogError("unable to STRING_construct");
            STRING_delete(result->deviceName);
            free(result);
            result = NULL;
        }
        else
        {
            IOTHUB_CLIENT_CONFIG temp;
            temp.protocol = moduleHandleData->transportProvider;
            temp.deviceId = deviceName;
            temp.deviceKey = deviceKey;
            temp.deviceSasToken = NULL;
            temp.iotHubName = STRING_c_str(moduleHandleData->IoTHubName);
            temp.iotHubSuffix = STRING_c_str(moduleHandleData->IoTHubSuffix);
            temp.protocolGatewayHostName = NULL;

            /*Codes_SRS_IOTHUBMODULE_05_013: [ If a new personality is created and the module's transport has already been created (in `IotHub_Create`), an `IOTHUB_CLIENT_HANDLE` will be added to the personality by a call to `IoTHubClient_CreateWithTransport`. ]*/
            /*Codes_SRS_IOTHUBMODULE_05_003: [ If a new personality is created and the module's transport has not already been created, an `IOTHUB_CLIENT_HANDLE` will be added to the personality by a call to `IoTHubClient_Create` with the corresponding transport provider. ]*/
            result->iothubHandle = (moduleHandleData->transportHandle != NULL)
                ? IoTHubClient_CreateWithTransport(moduleHandleData->transportHandle, &temp)
                : IoTHubClient_Create(&temp);

            if (result->iothubHandle == NULL)
            {
                LogError("unable to create IoTHubClient");
                STRING_delete(result->deviceName);
                STRING_delete(result->deviceKey);
                free(result);
                result = NULL;
            }
            else
            {
                /*Codes_SRS_IOTHUBMODULE_17_003: [ If a new personality is created, then the associated IoTHubClient will be set to receive messages by calling `IoTHubClient_SetMessageCallback` with callback function `IotHub_ReceiveMessageCallback`, and the personality as context. ]*/
                if (IoTHubClient_SetMessageCallback(result->iothubHandle, IotHub_ReceiveMessageCallback, result) != IOTHUB_CLIENT_OK)
                {
                    LogError("unable to IoTHubClient_SetMessageCallback");
                    IoTHubClient_Destroy(result->iothubHandle);
                    STRING_delete(result->deviceName);
                    STRING_delete(result->deviceKey);
                    free(result);
                    result = NULL;
                }
                else
                {
                    /*it is all fine*/
                    result->broker = moduleHandleData->broker;
                    result->module = moduleHandleData;
                }
            }
        }
    }
    return result;
}

static void PERSONALITY_destroy(PERSONALITY* personality)
{
    STRING_delete(personality->deviceName);
    STRING_delete(personality->deviceKey);
    IoTHubClient_Destroy(personality->iothubHandle);
}

static PERSONALITY* PERSONALITY_find_or_create(IOTHUB_HANDLE_DATA* moduleHandleData, const char* deviceName, const char* deviceKey)
{
    /*Codes_SRS_IOTHUBMODULE_02_017: [ Otherwise `IotHub_Receive` shall not create a new personality. ]*/
    PERSONALITY* result;
    PERSONALITY_PTR* resultPtr = VECTOR_find_if(moduleHandleData->personalities, lookup_DeviceName, deviceName);
    if (resultPtr == NULL)
    {
        /*a new device has arrived!*/
        PERSONALITY_PTR personality;
        if ((personality = PERSONALITY_create(deviceName, deviceKey, moduleHandleData)) == NULL)
        {
            LogError("unable to create a personality for the device %s", deviceName);
            result = NULL;
        }
        else
        {
            if ((VECTOR_push_back(moduleHandleData->personalities, &personality, 1)) != 0)
            {
                /*Codes_SRS_IOTHUBMODULE_02_016: [ If adding a new personality to the vector fails, then `IoTHub_Receive` shall return. ]*/
                LogError("VECTOR_push_back failed");
                PERSONALITY_destroy(personality);
                free(personality);
                result = NULL;
            }
            else
            {
                resultPtr = VECTOR_back(moduleHandleData->personalities);
                result = *resultPtr;
            }
            }
        }
    else
    {
        result = *resultPtr;
    }
    return result;
}

static IOTHUB_MESSAGE_HANDLE IoTHubMessage_CreateFromGWMessage(MESSAGE_HANDLE message)
{
    IOTHUB_MESSAGE_HANDLE result;
    const CONSTBUFFER* content = Message_GetContent(message);
    /*Codes_SRS_IOTHUBMODULE_02_019: [ If creating the IOTHUB_MESSAGE_HANDLE fails, then `IotHub_Receive` shall return. ]*/
    result = IoTHubMessage_CreateFromByteArray(content->buffer, content->size);
    if (result == NULL)
    {
        LogError("IoTHubMessage_CreateFromByteArray failed");
        /*return as is*/
    }
    else
    {
        MAP_HANDLE iothubMessageProperties = IoTHubMessage_Properties(result);
        CONSTMAP_HANDLE gwMessageProperties = Message_GetProperties(message);
        const char* const* keys;
        const char* const* values;
        size_t nProperties;
        if (ConstMap_GetInternals(gwMessageProperties, &keys, &values, &nProperties) != CONSTMAP_OK)
        {
            /*Codes_SRS_IOTHUBMODULE_02_019: [ If creating the IOTHUB_MESSAGE_HANDLE fails, then `IotHub_Receive` shall return. ]*/
            LogError("unable to get properties of the GW message");
            IoTHubMessage_Destroy(result);
            result = NULL;
        }
        else
        {
            size_t i;
            for (i = 0; i < nProperties; i++)
            {
                /*add all the properties of the GW message to the IOTHUB message*/ /*with the exception*/
                /*Codes_SRS_IOTHUBMODULE_02_018: [ `IotHub_Receive` shall create a new IOTHUB_MESSAGE_HANDLE having the same content as `messageHandle`, and the same properties with the exception of `deviceName` and `deviceKey`. ]*/
                if (
                    (strcmp(keys[i], "deviceName") != 0) &&
                    (strcmp(keys[i], "deviceKey") != 0)
                    )
                {
                    if (Map_AddOrUpdate(iothubMessageProperties, keys[i], values[i]) != MAP_OK)
                    {
                        /*Codes_SRS_IOTHUBMODULE_02_019: [ If creating the IOTHUB_MESSAGE_HANDLE fails, then `IotHub_Receive` shall return. ]*/
                        LogError("unable to Map_AddOrUpdate");
                        break;
                    }
                }
            }

            if (i == nProperties)
            {
                /*all is fine, return as is*/
            }
            else
            {
                /*Codes_SRS_IOTHUBMODULE_02_019: [ If creating the IOTHUB_MESSAGE_HANDLE fails, then `IotHub_Receive` shall return. ]*/
                IoTHubMessage_Destroy(result);
                result = NULL;
            }
        }
        ConstMap_Destroy(gwMessageProperties);
    }
    return result;
}

static void IotHub_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    /*Codes_SRS_IOTHUBMODULE_02_009: [ If `moduleHandle` or `messageHandle` is `NULL` then `IotHub_Receive` shall do nothing. ]*/
    if (
        (moduleHandle == NULL) ||
        (messageHandle == NULL)
        )
    {
        LogError("invalid arg moduleHandle=%p, messageHandle=%p", moduleHandle, messageHandle);
        /*do nothing*/
    }
    else
    {
        CONSTMAP_HANDLE properties = Message_GetProperties(messageHandle);
        const char* source = ConstMap_GetValue(properties, SOURCE); /*properties is !=NULL by contract of Message*/

        /*Codes_SRS_IOTHUBMODULE_02_010: [ If message properties do not contain a property called "source" having the value set to "mapping" then `IotHub_Receive` shall do nothing. ]*/
        if (
            (source == NULL) ||
            (strcmp(source, MAPPING)!=0)
            )
        {
            /*do nothing, the properties do not contain either "source" or "source":"mapping"*/
        }
        else
        {
            /*Codes_SRS_IOTHUBMODULE_02_011: [ If message properties do not contain a property called "deviceName" having a non-`NULL` value then `IotHub_Receive` shall do nothing. ]*/
            const char* deviceName = ConstMap_GetValue(properties, DEVICENAME);
            if (deviceName == NULL)
            {
                /*do nothing, not a message for this module*/
            }
            else
            {
                /*Codes_SRS_IOTHUBMODULE_02_012: [ If message properties do not contain a property called "deviceKey" having a non-`NULL` value then `IotHub_Receive` shall do nothing. ]*/
                const char* deviceKey = ConstMap_GetValue(properties, DEVICEKEY);
                if (deviceKey == NULL)
                {
                    /*do nothing, missing device key*/
                }
                else
                {
                    IOTHUB_HANDLE_DATA* moduleHandleData = moduleHandle;
                    /*Codes_SRS_IOTHUBMODULE_02_013: [ If no personality exists with a device ID equal to the value of the `deviceName` property of the message, then `IotHub_Receive` shall create a new `PERSONALITY` with the ID and key values from the message. ]*/

                    PERSONALITY* whereIsIt = PERSONALITY_find_or_create(moduleHandleData, deviceName, deviceKey);
                    if (whereIsIt == NULL)
                    {
                        /*Codes_SRS_IOTHUBMODULE_02_014: [ If creating the personality fails then `IotHub_Receive` shall return. ]*/
                        /*do nothing, device was not added to the GW*/
                        LogError("unable to PERSONALITY_find_or_create");
                    }
                    else
                    {
                        IOTHUB_MESSAGE_HANDLE iotHubMessage = IoTHubMessage_CreateFromGWMessage(messageHandle);
                        if(iotHubMessage == NULL)
                        {
                            LogError("unable to IoTHubMessage_CreateFromGWMessage (internal)");
                        }
                        else
                        {
                            /*Codes_SRS_IOTHUBMODULE_02_020: [ `IotHub_Receive` shall call IoTHubClient_SendEventAsync passing the IOTHUB_MESSAGE_HANDLE. ]*/
                            if (IoTHubClient_SendEventAsync(whereIsIt->iothubHandle, iotHubMessage, NULL, NULL) != IOTHUB_CLIENT_OK)
                            {
                                /*Codes_SRS_IOTHUBMODULE_02_021: [ If `IoTHubClient_SendEventAsync` fails then `IotHub_Receive` shall return. ]*/
                                LogError("unable to IoTHubClient_SendEventAsync");
                            }
                            else
                            {
                                /*all is fine, message has been accepted for delivery*/
                            }
                            IoTHubMessage_Destroy(iotHubMessage);
                        }
                    }
                }
            }
        }
        ConstMap_Destroy(properties);
    }
    /*Codes_SRS_IOTHUBMODULE_02_022: [ If `IoTHubClient_SendEventAsync` succeeds then `IotHub_Receive` shall return. ]*/
}

static const MODULE_API_1 moduleInterface =
{
    {MODULE_API_VERSION_1},

    IotHub_ParseConfigurationFromJson,
    IotHub_FreeConfiguration,
    IotHub_Create,
    IotHub_Destroy,
    IotHub_Receive,
    NULL
};

/*Codes_SRS_IOTHUBMODULE_26_001: [ `Module_GetApi` shall return a pointer to a `MODULE_API` structure with the required function pointers. ]*/
#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(IOTHUB_MODULE)(MODULE_API_VERSION gateway_api_version)
#else
MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version)
#endif
{
    (void)gateway_api_version;
    return (const MODULE_API *)&moduleInterface;
}
