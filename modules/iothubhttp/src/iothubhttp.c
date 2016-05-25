// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "azure_c_shared_utility/gballoc.h"

#include "iothubhttp.h"
#include "iothub_client.h"
#include "iothubtransport.h"
#include "iothubtransporthttp.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/iot_logging.h"
#include "azure_c_shared_utility/strings.h"
#include "messageproperties.h"
#include "message_bus.h"

typedef struct PERSONALITY_TAG
{
    STRING_HANDLE deviceName;
    STRING_HANDLE deviceKey;
    IOTHUB_CLIENT_HANDLE iothubHandle;
    MESSAGE_BUS_HANDLE busHandle;
}PERSONALITY;

typedef PERSONALITY* PERSONALITY_PTR;

typedef struct IOTHUBHTTP_HANDLE_DATA_TAG
{
    VECTOR_HANDLE personalities; /*holds PERSONALITYs*/
    STRING_HANDLE IoTHubName;
    STRING_HANDLE IoTHubSuffix;
    TRANSPORT_HANDLE transportHandle;
    MESSAGE_BUS_HANDLE busHandle;
}IOTHUBHTTP_HANDLE_DATA;

#define SOURCE "source"
#define MAPPING "mapping"
#define DEVICENAME "deviceName"
#define DEVICEKEY "deviceKey"

static MODULE_HANDLE IoTHubHttp_Create(MESSAGE_BUS_HANDLE busHandle, const void* configuration)
{
    IOTHUBHTTP_HANDLE_DATA *result;
    /*Codes_SRS_IOTHUBHTTP_02_001: [If busHandle is NULL then IoTHubHttp_Create shall fail and return NULL.]*/
    /*Codes_SRS_IOTHUBHTTP_02_002: [If configuration is NULL then IoTHubHttp_Create shall fail and return NULL.]*/
    /*Codes_SRS_IOTHUBHTTP_02_003: [If configuration->IoTHubName is NULL then IoTHubHttp_Create shall and return NULL.]*/
    /*Codes_SRS_IOTHUBHTTP_02_004: [If configuration->IoTHubSuffix is NULL then IoTHubHttp_Create shall fail and return NULL.]*/
    if (
        (busHandle == NULL) ||
        (configuration == NULL) ||
        (((const IOTHUBHTTP_CONFIG*)configuration)->IoTHubName == NULL) ||
        (((const IOTHUBHTTP_CONFIG*)configuration)->IoTHubSuffix == NULL)
        )
    {
        LogError("invalid arg busHandle=%p, configuration=%p, IoTHubName=%s IoTHubSuffix=%s", busHandle, configuration, (configuration!=NULL)?((const IOTHUBHTTP_CONFIG*)configuration)->IoTHubName:"undefined behavior", (configuration != NULL) ? ((const IOTHUBHTTP_CONFIG*)configuration)->IoTHubSuffix : "undefined behavior");
        result = NULL;
    }
    else
    {
        result = malloc(sizeof(IOTHUBHTTP_HANDLE_DATA));
        /*Codes_SRS_IOTHUBHTTP_02_027: [When IoTHubHttp_Create encounters an internal failure it shall fail and return NULL.]*/
        if (result == NULL)
        {
            LogError("malloc returned NULL");
            /*return as is*/
        }
        else
        {
            /*Codes_SRS_IOTHUBHTTP_02_006: [IoTHubHttp_Create shall create an empty VECTOR containing pointers to PERSONALITYs.]*/
            result->personalities = VECTOR_create(sizeof(PERSONALITY_PTR));
            if (result->personalities == NULL)
            {
                /*Codes_SRS_IOTHUBHTTP_02_007: [If creating the VECTOR fails then IoTHubHttp_Create shall fail and return NULL.]*/
                free(result);
                result = NULL;
                LogError("VECTOR_create returned NULL");
            }
            else
            {
                /*Codes_SRS_IOTHUBHTTP_17_001: [ IoTHubHttp_Create shall create a shared transport by calling IoTHubTransport_Create. ]*/
                result->transportHandle = IoTHubTransport_Create(HTTP_Protocol, ((const IOTHUBHTTP_CONFIG*)configuration)->IoTHubName, ((const IOTHUBHTTP_CONFIG*)configuration)->IoTHubSuffix);
                if (result->transportHandle == NULL)
                {
                    /*Codes_SRS_IOTHUBHTTP_17_002: [ If creating the shared transport fails, IoTHubHttp_Create shall fail and return NULL. ]*/
                    VECTOR_destroy(result->personalities);
                    free(result);
                    result = NULL;
                    LogError("VECTOR_create returned NULL");
                }
                else
                {
                    /*Codes_SRS_IOTHUBHTTP_02_028: [IoTHubHttp_Create shall create a copy of configuration->IoTHubSuffix.]*/
                    /*Codes_SRS_IOTHUBHTTP_02_029: [IoTHubHttp_Create shall create a copy of configuration->IoTHubName.]*/
                    if ((result->IoTHubName = STRING_construct(((const IOTHUBHTTP_CONFIG*)configuration)->IoTHubName)) == NULL)
                    {
                        IoTHubTransport_Destroy(result->transportHandle);
                        VECTOR_destroy(result->personalities);
                        free(result);
                        result = NULL;
                    }
                    else if ((result->IoTHubSuffix = STRING_construct(((const IOTHUBHTTP_CONFIG*)configuration)->IoTHubSuffix)) == NULL)
                    {
                        STRING_delete(result->IoTHubName);
                        IoTHubTransport_Destroy(result->transportHandle);
                        VECTOR_destroy(result->personalities);
                        free(result);
                        result = NULL;
                    }
                    else
                    {
                        /*Codes_SRS_IOTHUBHTTP_17_004: [ IoTHubHttp_Create shall store the busHandle. ]*/
                        result->busHandle = busHandle;
                        /*Codes_SRS_IOTHUBHTTP_02_008: [Otherwise, IoTHubHttp_Create shall return a non-NULL handle.]*/
                    }
                }
            }
        }
    }
    return result;
}

static void IoTHubHttp_Destroy(MODULE_HANDLE moduleHandle)
{
    /*Codes_SRS_IOTHUBHTTP_02_023: [If moduleHandle is NULL then IoTHubHttp_Destroy shall return.]*/
    if (moduleHandle == NULL)
    {
        LogError("moduleHandle parameter was NULL");
    }
    else
    {
        /*Codes_SRS_IOTHUBHTTP_02_024: [Otherwise IoTHubHttp_Destroy shall free all used resources.]*/
        IOTHUBHTTP_HANDLE_DATA * handleData = moduleHandle;
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

static IOTHUBMESSAGE_DISPOSITION_RESULT IoTHubHttp_ReceiveMessageCallback(IOTHUB_MESSAGE_HANDLE msg, void* userContextCallback)
{
    IOTHUBMESSAGE_DISPOSITION_RESULT result;
    if (userContextCallback == NULL)
    {
        /*Codes_SRS_IOTHUBHTTP_17_005: [ If userContextCallback is NULL, then IoTHubHttp_ReceiveMessageCallback shall return IOTHUBMESSAGE_ABANDONED. ]*/
        LogError("No context to associate message");
        result = IOTHUBMESSAGE_ABANDONED;
    }
    else
    {
        PERSONALITY_PTR personality = (PERSONALITY_PTR)userContextCallback;
        IOTHUBMESSAGE_CONTENT_TYPE msgContentType = IoTHubMessage_GetContentType(msg);
        if (msgContentType == IOTHUBMESSAGE_UNKNOWN)
        {
            /*Codes_SRS_IOTHUBHTTP_17_006: [ If Message Content type is IOTHUBMESSAGE_UNKNOWN, then IoTHubHttp_ReceiveMessageCallback shall return IOTHUBMESSAGE_ABANDONED. ]*/
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
                /*Codes_SRS_IOTHUBHTTP_17_014: [ If Message content type is IOTHUBMESSAGE_STRING, IoTHubHttp_ReceiveMessageCallback shall get the buffer from results of IoTHubMessage_GetString. ]*/
                const char* sourceStr = IoTHubMessage_GetString(msg);
                newMessageConfig.source = (const unsigned char*)sourceStr;
                /*Codes_SRS_IOTHUBHTTP_17_015: [ If Message content type is IOTHUBMESSAGE_STRING, IoTHubHttp_ReceiveMessageCallback shall get the buffer size from the string length. ]*/
                newMessageConfig.size = strlen(sourceStr);
                msgResult = IOTHUB_MESSAGE_OK;
            }
            else
            {
                /* content type is byte array */
                /*Codes_SRS_IOTHUBHTTP_17_013: [ If Message content type is IOTHUBMESSAGE_BYTEARRAY, IoTHubHttp_ReceiveMessageCallback shall get the size and buffer from the results of IoTHubMessage_GetByteArray. ]*/
                msgResult = IoTHubMessage_GetByteArray(msg, &(newMessageConfig.source), &(newMessageConfig.size));
            }
            if (msgResult != IOTHUB_MESSAGE_OK)
            {
                /*Codes_SRS_IOTHUBHTTP_17_023: [ If IoTHubMessage_GetByteArray fails, IoTHubHttp_ReceiveMessageCallback shall return IOTHUBMESSAGE_ABANDONED. ]*/
                LogError("Failed to get message content");
                result = IOTHUBMESSAGE_ABANDONED;
            }
            else
            {
                /* Now, handle message properties. */
                MAP_HANDLE newProperties;
                /*Codes_SRS_IOTHUBHTTP_17_007: [ IoTHubHttp_ReceiveMessageCallback shall get properties from message by calling IoTHubMessage_Properties. */
                newProperties = IoTHubMessage_Properties(msg);
                if (newProperties == NULL)
                {
                    /*Codes_SRS_IOTHUBHTTP_17_008: [ If message properties are NULL, IoTHubHttp_ReceiveMessageCallback shall return IOTHUBMESSAGE_ABANDONED. ]*/
                    LogError("No Properties in IoTHub Message");
                    result = IOTHUBMESSAGE_ABANDONED;
                }
                else
                {
                    /*Codes_SRS_IOTHUBHTTP_17_009: [ IoTHubHttp_ReceiveMessageCallback shall define a property "source" as "IoTHubHTTP". ]*/
                    /*Codes_SRS_IOTHUBHTTP_17_010: [ IoTHubHttp_ReceiveMessageCallback shall define a property "deviceName" as the PERSONALITY's deviceName. ]*/
                    /*Codes_SRS_IOTHUBHTTP_17_011: [ IoTHubHttp_ReceiveMessageCallback shall combine message properties with the "source" and "deviceName" properties. ]*/
                    if (Map_Add(newProperties, GW_SOURCE_PROPERTY, GW_IOTHUB_MODULE) != MAP_OK)
                    {
                        /*Codes_SRS_IOTHUBHTTP_17_022: [ If message properties fail to combine, IoTHubHttp_ReceiveMessageCallback shall return IOTHUBMESSAGE_ABANDONED. ]*/
                        LogError("Property [%s] did not add properly", GW_SOURCE_PROPERTY);
                        result = IOTHUBMESSAGE_ABANDONED;
                    }
                    else if (Map_Add(newProperties, GW_DEVICENAME_PROPERTY, STRING_c_str(personality->deviceName)) != MAP_OK)
                    {
                        /*Codes_SRS_IOTHUBHTTP_17_022: [ If message properties fail to combine, IoTHubHttp_ReceiveMessageCallback shall return IOTHUBMESSAGE_ABANDONED. ]*/
                        LogError("Property [%s] did not add properly", GW_DEVICENAME_PROPERTY);
                        result = IOTHUBMESSAGE_ABANDONED;
                    }
                    else
                    {
                        /*Codes_SRS_IOTHUBHTTP_17_016: [ IoTHubHttp_ReceiveMessageCallback shall create a new message from combined properties, the size and buffer. ]*/
                        newMessageConfig.sourceProperties = newProperties;
                        MESSAGE_HANDLE gatewayMsg = Message_Create(&newMessageConfig);
                        if (gatewayMsg == NULL)
                        {
                            /*Codes_SRS_IOTHUBHTTP_17_017: [ If the message fails to create, IoTHubHttp_ReceiveMessageCallback shall return IOTHUBMESSAGE_REJECTED. ]*/
                            LogError("Failed to create gateway message");
                            result = IOTHUBMESSAGE_REJECTED;
}
                        else
                        {
                            /*Codes_SRS_IOTHUBHTTP_17_018: [ IoTHubHttp_ReceiveMessageCallback shall call MessageBus_Publish with the new message and the busHandle. ]*/
                            if (MessageBus_Publish(personality->busHandle, gatewayMsg) != MESSAGE_BUS_OK)
                            {
                                /*Codes_SRS_IOTHUBHTTP_17_019: [ If the message fails to publish, IoTHubHttp_ReceiveMessageCallback shall return IOTHUBMESSAGE_REJECTED. ]*/
                                LogError("Failed to publish gateway message");
                                result = IOTHUBMESSAGE_REJECTED;
                            }
                            else
                            {
                                /*Codes_SRS_IOTHUBHTTP_17_021: [ Upon success, IoTHubHttp_ReceiveMessageCallback shall return IOTHUBMESSAGE_ACCEPTED. ]*/
                                result = IOTHUBMESSAGE_ACCEPTED;
                            }
                            /*Codes_SRS_IOTHUBHTTP_17_020: [ IoTHubHttp_ReceiveMessageCallback shall destroy all resources it creates. ]*/
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
static PERSONALITY_PTR PERSONALITY_create(const char* deviceName, const char* deviceKey, IOTHUBHTTP_HANDLE_DATA* moduleHandleData)
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
            temp.deviceId = deviceName;
            temp.deviceKey = deviceKey;
            temp.iotHubName = STRING_c_str(moduleHandleData->IoTHubName);
            temp.iotHubSuffix = STRING_c_str(moduleHandleData->IoTHubSuffix);
            temp.protocol = HTTP_Protocol;
            temp.protocolGatewayHostName = NULL;
            temp.deviceSasToken = NULL;
            if ((result->iothubHandle = IoTHubClient_CreateWithTransport(moduleHandleData->transportHandle, &temp)) == NULL)
            {
                LogError("unable to IoTHubClient_CreateWithTransport");
                STRING_delete(result->deviceName);
                STRING_delete(result->deviceKey);
                free(result);
                result = NULL;
            }
            else
            {
                /*Codes_SRS_IOTHUBHTTP_17_003: [ If a new PERSONALITY is created, then the IoTHubClient will be set to receive messages, by calling IoTHubClient_SetMessageCallback with callback function IoTHubHttp_ReceiveMessageCallback and PERSONALITY as context.]*/
                if (IoTHubClient_SetMessageCallback(result->iothubHandle, IoTHubHttp_ReceiveMessageCallback, result) != IOTHUB_CLIENT_OK)
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
                    result->busHandle = moduleHandleData->busHandle;
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

static PERSONALITY* PERSONALITY_find_or_create(IOTHUBHTTP_HANDLE_DATA* moduleHandleData, const char* deviceName, const char* deviceKey)
{
    /*Codes_SRS_IOTHUBHTTP_02_017: [If the deviceName exists in the PERSONALITY collection then IoTHubHttp_Receive shall not create a new IOTHUB_CLIENT_HANDLE.]*/
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
                /*Codes_SRS_IOTHUBHTTP_02_016: [If adding the new triplet fails, then IoTHubClient_Create shall return.]*/
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
    /*Codes_SRS_IOTHUBHTTP_02_019: [If creating the IOTHUB_MESSAGE_HANDLE fails, then IoTHubHttp_Receive shall return.]*/
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
            /*Codes_SRS_IOTHUBHTTP_02_019: [If creating the IOTHUB_MESSAGE_HANDLE fails, then IoTHubHttp_Receive shall return.]*/
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
                /*Codes_SRS_IOTHUBHTTP_02_018: [IoTHubHttp_Receive shall create a new IOTHUB_MESSAGE_HANDLE having the same content as the messageHandle and same properties with the exception of deviceName and deviceKey properties.]*/
                if (
                    (strcmp(keys[i], "deviceName") != 0) &&
                    (strcmp(keys[i], "deviceKey") != 0)
                    )
                {
                   
                    if (Map_AddOrUpdate(iothubMessageProperties, keys[i], values[i]) != MAP_OK)
                    {
                        /*Codes_SRS_IOTHUBHTTP_02_019: [If creating the IOTHUB_MESSAGE_HANDLE fails, then IoTHubHttp_Receive shall return.]*/
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
                /*Codes_SRS_IOTHUBHTTP_02_019: [If creating the IOTHUB_MESSAGE_HANDLE fails, then IoTHubHttp_Receive shall return.]*/
                IoTHubMessage_Destroy(result);
                result = NULL;
            }
        }
        ConstMap_Destroy(gwMessageProperties);
    }
    return result;
}

static void IoTHubHttp_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    /*Codes_SRS_IOTHUBHTTP_02_009: [If moduleHandle or messageHandle is NULL then IoTHubHttp_Receive shall do nothing.]*/
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

        /*Codes_SRS_IOTHUBHTTP_02_010: [If message properties do not contain a property called "source" having the value set to "mapping" then IoTHubHttp_Receive shall do nothing.]*/
        if (
            (source == NULL) ||
            (strcmp(source, MAPPING)!=0)
            )
        {
            /*do nothing, the properties do not contain either "source" or "source":"mapping"*/
        }
        else
        {
            /*Codes_SRS_IOTHUBHTTP_02_011: [If message properties do not contain a property called "deviceName" having a non-NULL value then IoTHubHttp_Receive shall do nothing.]*/
            const char* deviceName = ConstMap_GetValue(properties, DEVICENAME);
            if (deviceName == NULL)
            {
                /*do nothing, not a message for this module*/
            }
            else
            {
                /*Codes_SRS_IOTHUBHTTP_02_012: [If message properties do not contain a property called "deviceKey" having a non-NULL value then IoTHubHttp_Receive shall do nothing.]*/
                const char* deviceKey = ConstMap_GetValue(properties, DEVICEKEY);
                if (deviceKey == NULL)
                {
                    /*do nothing, missing device key*/
                }
                else
                {
                    IOTHUBHTTP_HANDLE_DATA* moduleHandleData = moduleHandle;
                    /*Codes_SRS_IOTHUBHTTP_02_013: [If the deviceName does not exist in the PERSONALITY collection then IoTHubHttp_Receive shall create a new IOTHUB_CLIENT_HANDLE by a call to IoTHubClient_CreateWithTransport.]*/
                    
                    PERSONALITY* whereIsIt = PERSONALITY_find_or_create(moduleHandleData, deviceName, deviceKey);
                    if (whereIsIt == NULL)
                    {
                        /*Codes_SRS_IOTHUBHTTP_02_014: [If creating the PERSONALITY fails then IoTHubHttp_Receive shall return.]*/
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
                            /*Codes_SRS_IOTHUBHTTP_02_020: [IoTHubHttp_Receive shall call IoTHubClient_SendEventAsync passing the IOTHUB_MESSAGE_HANDLE.*/
                            if (IoTHubClient_SendEventAsync(whereIsIt->iothubHandle, iotHubMessage, NULL, NULL) != IOTHUB_CLIENT_OK)
                            {
                                /*Codes_SRS_IOTHUBHTTP_02_021: [If IoTHubClient_SendEventAsync fails then IoTHubHttp_Receive shall return.]*/
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
    /*Codes_SRS_IOTHUBHTTP_02_022: [IoTHubHttp_Receive shall return.]*/
}

static const MODULE_APIS Module_GetAPIS_Impl = 
{
    /*Codes_SRS_IOTHUBHTTP_02_026: [The MODULE_APIS structure shall have non-NULL Module_Create, Module_Destroy, and Module_Receive fields.]*/
    IoTHubHttp_Create,
    IoTHubHttp_Destroy,
    IoTHubHttp_Receive
};

/*Codes_SRS_IOTHUBHTTP_02_025: [Module_GetAPIS shall return a non-NULL pointer.]*/
#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(IOTHUBHTTP_MODULE)(void)
#else
MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void)
#endif
{
    return &Module_GetAPIS_Impl;
}