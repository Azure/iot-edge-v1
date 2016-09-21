// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "azure_c_shared_utility/gballoc.h"

#include "iothub_client.h"
#include "iothubtransport.h"
#include "iothubtransporthttp.h"
#include "iothub_message.h"
#include "iothubdeviceping.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/threadapi.h"
#include "messageproperties.h"
#include "broker.h"

#ifndef _MESSAGECONTENT
#define MESSAGECONTENT "deviceping"
#endif

typedef struct IOTHUBDEVICEPING_HANDLE_DATA_TAG
{
    IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;
    BROKER_HANDLE brokerHandle;
} IOTHUBDEVICEPING_HANDLE_DATA;

static MODULE_HANDLE IoTHubDevicePing_Create(BROKER_HANDLE brokerHandle, const void *configuration)
{
    IOTHUBDEVICEPING_HANDLE_DATA *result;
    if (
        (brokerHandle == NULL) ||
        (configuration == NULL) ||
        (((const IOTHUBDEVICEPING_CONFIG *)configuration)->DeviceConnectionString == "NULL") // NULL
        )
    {
        LogError("invalid arg brokerHandle=%p, configuration=%p, IoTHubName=%s IoTHubSuffix=%s\r\n", brokerHandle, configuration, (configuration != NULL) ? ((const IOTHUBDEVICEPING_CONFIG *)configuration)->DeviceConnectionString : "undefined behavior");
        result = NULL;
    }
    else
    {
        const char *devConStr = ((const IOTHUBDEVICEPING_CONFIG *)configuration)->DeviceConnectionString;
        result = malloc(sizeof(IOTHUBDEVICEPING_HANDLE_DATA));
        if (result == NULL)
        {
            LogError("malloc returned NULL\r\n");
            /*return as is*/
        }
        else
        {
            result->iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(devConStr, HTTP_Protocol);
            if (result->iotHubClientHandle == NULL)
            {
                free(result);
                result = NULL;
                LogError("iotHubClientHandle returned NULL\r\n");
            }
            else
            {
                result->brokerHandle = brokerHandle;
            }
        }
    }
    return result;
}

static void IoTHubDevicePing_Destroy(MODULE_HANDLE moduleHandle)
{
    if (moduleHandle == NULL)
    {
        LogError("moduleHandle parameter was NULL\r\n");
    }
    else
    {
        IOTHUBDEVICEPING_HANDLE_DATA *handleData = moduleHandle;
        if (handleData->iotHubClientHandle != NULL)
        {
            IoTHubClient_LL_Destroy(handleData->iotHubClientHandle);
        }
        free(handleData);
    }
}

static IOTHUB_MESSAGE_HANDLE IoTHubMessage_CreateFromGWMessage(MESSAGE_HANDLE message)
{
    IOTHUB_MESSAGE_HANDLE result;
    const CONSTBUFFER *content = Message_GetContent(message);

    result = IoTHubMessage_CreateFromByteArray(content->buffer, content->size);
    if (result == NULL)
    {
        LogError("IoTHubMessage_CreateFromByteArray failed\r\n");
        /*return as is*/
    }
    else
    {
        MAP_HANDLE iothubMessageProperties = IoTHubMessage_Properties(result);
        CONSTMAP_HANDLE gwMessageProperties = Message_GetProperties(message);
        const char *const *keys;
        const char *const *values;
        size_t nProperties;
        if (ConstMap_GetInternals(gwMessageProperties, &keys, &values, &nProperties) != CONSTMAP_OK)
        {
            LogError("unable to get properties of the GW message\r\n");
            IoTHubMessage_Destroy(result);
            result = NULL;
        }
        else
        {
            size_t i;
            for (i = 0; i < nProperties; i++)
            {
                if (
                    (strcmp(keys[i], "deviceName") != 0) &&
                    (strcmp(keys[i], "deviceKey") != 0))
                {

                    if (Map_AddOrUpdate(iothubMessageProperties, keys[i], values[i]) != MAP_OK)
                    {
                        LogError("unable to Map_AddOrUpdate\r\n");
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
                IoTHubMessage_Destroy(result);
                result = NULL;
            }
        }
        ConstMap_Destroy(gwMessageProperties);
    }
    return result;
}

static void sendCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *userContextCallback)
{
    if (result == IOTHUB_CLIENT_CONFIRMATION_OK)
    {
        LogInfo("Message sent: %s\r\n", (char *)userContextCallback);
    }
}

static void IoTHubDevicePing_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    if (moduleHandle == NULL)
    {
        LogError("invalid arg moduleHandle=%p\r\n", moduleHandle);
    }
    else
    {
        MESSAGE_HANDLE msgBusMessage;
        if (messageHandle == NULL)
        {
            MAP_HANDLE propertiesMap = Map_Create(NULL);
            if (propertiesMap == NULL)
            {
                LogError("unable to create a Map");
            }
            else
            {
                if (Map_AddOrUpdate(propertiesMap, "MESSAGECONTENT", MESSAGECONTENT) != MAP_OK)
                {
                    LogError("unable to Map_AddOrUpdate MESSAGECONTENT");
                }
                MESSAGE_CONFIG msgConfig;
                msgConfig.size = strlen(MESSAGECONTENT);
                msgConfig.source = MESSAGECONTENT;
                msgConfig.sourceProperties = propertiesMap;
                msgBusMessage = Message_Create(&msgConfig);
                if (msgBusMessage == NULL)
                {
                    LogError("unable to create \"deviceping\" message");
                }
            }
        }
        else
        {
            msgBusMessage = messageHandle;
        }

        CONSTMAP_HANDLE properties = Message_GetProperties(messageHandle);
        const char *msgcontent = ConstMap_GetValue(properties, MESSAGECONTENT);
        if (msgcontent == NULL)
        {
            /*do nothing, not a message for this module*/
            printf("MESSAGECONTENT is NULL\r\n");
        }
        else
        {
            IOTHUBDEVICEPING_HANDLE_DATA *moduleHandleData = moduleHandle;
            IOTHUB_MESSAGE_HANDLE iotHubMessage = IoTHubMessage_CreateFromGWMessage(messageHandle);
            if (iotHubMessage == NULL)
            {
                LogError("unable to IoTHubMessage_CreateFromGWMessage (internal)\r\n");
            }
            else
            {
                if (IoTHubClient_LL_SendEventAsync(moduleHandleData->iotHubClientHandle, iotHubMessage, sendCallback, (void *)msgcontent) != IOTHUB_CLIENT_OK)
                    {
                        LogError("unable to IoTHubClient_SendEventAsync\r\n");
                    }
                else
                {
                    LogInfo("all is fine, message has been accepted for delivery\r\n");
                }
                IoTHubClient_LL_DoWork(moduleHandleData->iotHubClientHandle);
                ThreadAPI_Sleep(1000);
                IoTHubMessage_Destroy(iotHubMessage);
            }
        }
        ConstMap_Destroy(properties);
    }
}

static const MODULE_APIS Module_GetAPIS_Impl =
    {
        /*Codes_SRS_IOTHUBHTTP_02_026: [The MODULE_APIS structure shall have non-NULL Module_Create, Module_Destroy, and Module_Receive fields.]*/
        IoTHubDevicePing_Create,
        IoTHubDevicePing_Destroy,
        IoTHubDevicePing_Receive};

/*Codes_SRS_IOTHUBHTTP_02_025: [Module_GetAPIS shall return a non-NULL pointer.]*/
#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_APIS *MODULE_STATIC_GETAPIS(IOTHUBDEVICEPING_MODULE)(void)
#else
MODULE_EXPORT const MODULE_APIS *Module_GetAPIS(void)
#endif
{
    return &Module_GetAPIS_Impl;
}