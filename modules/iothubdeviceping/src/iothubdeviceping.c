// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "azure_c_shared_utility/gballoc.h"

#include "iothubdeviceping.h"
#include "iothub_client.h"
#include "iothubtransport.h"
#include "iothubtransporthttp.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/threadapi.h"
#include "messageproperties.h"
#include "broker.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_uamqp_c/message_receiver.h"
#include "azure_uamqp_c/message.h"
#include "azure_uamqp_c/messaging.h"
#include "azure_uamqp_c/amqpalloc.h"
#include "azure_uamqp_c/saslclientio.h"
#include "azure_uamqp_c/sasl_plain.h"

#ifndef _ECHOREUQEST
#define ECHOREUQEST "ECHOREUQEST"
#endif

#ifndef _ECHOREPLY
#define ECHOREPLY "ECHOREPLY"
#endif

#ifndef _POLLEVENTHUBTIMEOUT
#define POLLEVENTHUBTIMEOUT "POLLEVENTHUBTIMEOUT"
#endif

typedef struct IOTHUBDEVICEPING_HANDLE_DATA_TAG
{
    THREAD_HANDLE threadHandle;
    IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;
    LOCK_HANDLE lockHandle;
    BROKER_HANDLE broker;
    IOTHUBDEVICEPING_CONFIG *config;
} IOTHUBDEVICEPING_HANDLE_DATA;

static int g_echoreply = 0;

int devicepingThread(void *param)
{
    (void)ThreadAPI_Sleep(2000); /*wait for gw to start*/
    // send message to iot hub and publish it to broker
    IOTHUBDEVICEPING_HANDLE_DATA *moduleHandleData = param;
    MESSAGE_HANDLE msgBusMessage;
    MAP_HANDLE propertiesMap = Map_Create(NULL);
    int messageCount = 1;
    if (propertiesMap == NULL)
    {
        LogError("unable to create a Map");
        return 1;
    }
    else
    {
        if (Map_AddOrUpdate(propertiesMap, "ECHOREUQEST", ECHOREUQEST) != MAP_OK)
        {
            LogError("unable to Map_AddOrUpdate ECHOREUQEST");
        }
        else
        {
            MESSAGE_CONFIG msgConfig;
            msgConfig.size = strlen(ECHOREUQEST);
            msgConfig.source = ECHOREUQEST;
            msgConfig.sourceProperties = propertiesMap;

            MESSAGE_HANDLE msg = Message_Create(&msgConfig);
            if (msg == NULL)
            {
                LogError("unable to create \"hello world\" message");
            }
            if (createAndPublishGatewayMessage(msgConfig, moduleHandleData, &msgBusMessage, messageCount) != IOTHUBMESSAGE_ACCEPTED)
            {
                LogError("iot hub message rejected");
                return 1;
            }
            else
            {
                LogInfo("ECHOREUQEST published\r\n");
            }
            // ping iot hub device
            IOTHUB_MESSAGE_HANDLE iotHubMessage = IoTHubMessage_CreateFromGWMessage(msgBusMessage);
            if (iotHubMessage == NULL)
            {
                LogError("unable to IoTHubMessage_CreateFromGWMessage (internal)\r\n");
                Message_Destroy(msgBusMessage);
                return 1;
            }
            else
            {
                LogInfo("devicepingThread enter\r\n");
                if (IoTHubClient_LL_SendEventAsync(moduleHandleData->iotHubClientHandle, iotHubMessage, sendCallback, (void *)moduleHandleData) != IOTHUB_CLIENT_OK)
                {
                    LogError("unable to IoTHubClient_SendEventAsync\r\n");
                    Message_Destroy(msgBusMessage);
                    return 1;
                }
                else
                {
                    LogInfo("all is fine, message has been accepted for delivery\r\n");
                }
                IoTHubClient_LL_DoWork(moduleHandleData->iotHubClientHandle);
                ThreadAPI_Sleep(1000);
                IoTHubMessage_Destroy(iotHubMessage);
            }
            /*Codes_SRS_IOTHUBMODULE_17_020: [ `IotHub_ReceiveMessageCallback` shall destroy all resources it creates. ]*/
            Message_Destroy(msgBusMessage);
        }
    }
    return 0;
}
static IOTHUBMESSAGE_DISPOSITION_RESULT createAndPublishGatewayMessage(MESSAGE_CONFIG messageConfig, MODULE_HANDLE moduleHandle, MESSAGE_HANDLE *msghandle, int count)
{
    // poll message from event hub for echo
    IOTHUBMESSAGE_DISPOSITION_RESULT result;
    IOTHUBDEVICEPING_HANDLE_DATA *moduleHandleData = moduleHandle;
    *msghandle = Message_Create(&messageConfig);
    if (*msghandle == NULL)
    {
        /*Codes_SRS_IOTHUBMODULE_17_017: [ If the message fails to create, `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_REJECTED`. ]*/
        LogError("Failed to create gateway message");
        result = IOTHUBMESSAGE_REJECTED;
    }
    else
    {
        int i = count;
        while (i != 0)
        {
            i--;
            if (Lock(moduleHandleData->lockHandle) == LOCK_OK)
            {
                /*Codes_SRS_IOTHUBMODULE_17_018: [ `IotHub_ReceiveMessageCallback` shall call `Broker_Publish` with the new message, this module's handle, and the `broker`. ]*/
                if (Broker_Publish(moduleHandleData->broker, moduleHandleData, *msghandle) != BROKER_OK)
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
                (void)Unlock(moduleHandleData->lockHandle);
            }
        }
    }
    return result;
}

static AMQP_VALUE on_message_received(const void *context, MESSAGE_HANDLE message)
{
    LogInfo("on_message_received enter\r\n");
    //(void)message;
    IOTHUBDEVICEPING_HANDLE_DATA *moduleHandleData = (IOTHUBDEVICEPING_HANDLE_DATA *)context;
    AMQP_VALUE result;
    BINARY_DATA binary_data;
    MESSAGE_HANDLE msgBusMessage;

    // log message in logger
    MAP_HANDLE propertiesMap = Map_Create(NULL);
    if (propertiesMap == NULL)
    {
        LogError("unable to create a Map");
    }
    else
    {
        MESSAGE_CONFIG msgConfig;
        if (message_get_body_amqp_data(message, 0, &binary_data) != 0)
        {
            LogError("Cannot get message data");
            result = messaging_delivery_rejected("Rejected due to failure reading AMQP message", "Failed reading message body");
            if (Map_AddOrUpdate(propertiesMap, "messaging_delivery_rejected", ECHOREPLY) != MAP_OK)
            {
                LogError("unable to Map_AddOrUpdate ECHOREPLY");
                return result;
            }
            msgConfig.size = strlen("messaging_delivery_rejected");
            msgConfig.source = "messaging_delivery_rejected";
        }
        else
        {
            result = messaging_delivery_accepted();
            if (strstr((const char *)binary_data.bytes, ECHOREUQEST) != NULL)
            {
                if (Map_AddOrUpdate(propertiesMap, "Success Ping", ECHOREPLY) != MAP_OK)
                {
                    LogError("unable to Map_AddOrUpdate ECHOREPLY");
                    return result;
                }
                msgConfig.size = strlen("Success Ping");
                msgConfig.source = "Success Ping";
                g_echoreply = 1;
            }
        }
        LogInfo("Message received: %s; Length: %u\r\n", (const char *)binary_data.bytes, binary_data.length);
        msgConfig.sourceProperties = propertiesMap;
        if (createAndPublishGatewayMessage(msgConfig, moduleHandleData, &msgBusMessage, 1) != IOTHUBMESSAGE_ACCEPTED)
        {
            LogError("iot hub message rejected");
        }
        Message_Destroy(msgBusMessage);
    }
    return result;
}

static char *concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1);
    if (result == NULL)
    {
        LogError("malloc failed");
    }
    else
    {
        strcpy(result, s1);
        strcat(result, s2);
    }
    return result;
}

static int pollEventhub(IOTHUBDEVICEPING_HANDLE_DATA *handleData)
{
    LogInfo("pollEventhub enter\r\n");
    int result;
    XIO_HANDLE sasl_io = NULL;
    CONNECTION_HANDLE connection = NULL;
    SESSION_HANDLE session = NULL;
    LINK_HANDLE link = NULL;
    MESSAGE_RECEIVER_HANDLE message_receiver = NULL;
    const char *eventHubHost = ((const IOTHUBDEVICEPING_CONFIG *)(handleData->config))->EH_HOST;
    const char *eventHubKeyName = ((const IOTHUBDEVICEPING_CONFIG *)(handleData->config))->EH_KEY_NAME;
    const char *eventHubKey = ((const IOTHUBDEVICEPING_CONFIG *)(handleData->config))->EH_KEY;
    const char *eventHubCompatibleName = ((const IOTHUBDEVICEPING_CONFIG *)(handleData->config))->EH_COMP_NAME;
    const char *eventHubPartitionNum = ((const IOTHUBDEVICEPING_CONFIG *)(handleData->config))->EH_PARTITION_NUM;

    int partitionNum = (int)atoi(((const IOTHUBDEVICEPING_CONFIG *)(handleData->config))->EH_PARTITION_NUM);

    amqpalloc_set_memory_tracing_enabled(true);

    if (platform_init() != 0)
    {
        result = -1;
    }
    else
    {
        size_t last_memory_used = 0;
        char *result1 = concat("amqps://", eventHubHost);
        char *result2 = concat("/", eventHubCompatibleName);
        char *result3 = concat((const char *)result1, (const char *)result2);
        char *result4 = concat((const char *)result3, "/ConsumerGroups/$Default/Partitions/");

        char **addressArr = (char **)malloc(sizeof(char *) * partitionNum);
        int i = 0;
        for (i = 0; i < partitionNum; i++)
        {
            int length = snprintf(NULL, 0, "%d", i);
            char *str = malloc(length + 1);
            snprintf(str, length + 1, "%d", i);
            addressArr[i] = (char *)concat((const char *)result4, (const char *)str);
            free(str);
            LogInfo("addressArr[%d]: %s\r\n", i, addressArr[i]);
            /* create SASL plain handler */
            SASL_PLAIN_CONFIG sasl_plain_config = {eventHubKeyName, eventHubKey, NULL};
            SASL_MECHANISM_HANDLE sasl_mechanism_handle = saslmechanism_create(saslplain_get_interface(), &sasl_plain_config);
            XIO_HANDLE tls_io;

            /* create the TLS IO */
            TLSIO_CONFIG tls_io_config = {eventHubHost, 5671};
            const IO_INTERFACE_DESCRIPTION *tlsio_interface = platform_get_default_tlsio();
            tls_io = xio_create(tlsio_interface, &tls_io_config);

            /* create the SASL client IO using the TLS IO */
            SASLCLIENTIO_CONFIG sasl_io_config;
            sasl_io_config.underlying_io = tls_io;
            sasl_io_config.sasl_mechanism = sasl_mechanism_handle;
            sasl_io = xio_create(saslclientio_get_interface_description(), &sasl_io_config);

            /* create the connection, session and link */
            connection = connection_create(sasl_io, eventHubHost, "whatever", NULL, NULL);
            session = session_create(connection, NULL, NULL);

            /* set incoming window to 100 for the session */
            session_set_incoming_window(session, 100);

            AMQP_VALUE source = messaging_create_source(addressArr[i]);
            AMQP_VALUE target = messaging_create_target("messages/events");
            link = link_create(session, "receiver-link", role_receiver, source, target);
            link_set_rcv_settle_mode(link, receiver_settle_mode_first);
            amqpvalue_destroy(source);
            amqpvalue_destroy(target);

            /* create a message receiver */
            message_receiver = messagereceiver_create(link, NULL, NULL);
            // pass handleData into call back
            if ((message_receiver == NULL) ||
                (messagereceiver_open(message_receiver, on_message_received, handleData) != 0))
            {
                result = -1;
            }
            else
            {
                while (!g_echoreply)
                {
                    size_t current_memory_used;
                    size_t maximum_memory_used;

                    connection_dowork(connection);

                    current_memory_used = amqpalloc_get_current_memory_used();
                    maximum_memory_used = amqpalloc_get_maximum_memory_used();

                    if (current_memory_used != last_memory_used)
                    {
                        LogInfo("Current memory usage:%lu (max:%lu)\r\n", (unsigned long)current_memory_used, (unsigned long)maximum_memory_used);
                        last_memory_used = current_memory_used;
                    }
                }

                // 5 seconds timeout
                MESSAGE_HANDLE msgBusMessage;
                MESSAGE_CONFIG msgConfig;
                MAP_HANDLE propertiesMap = Map_Create(NULL);
                if (propertiesMap == NULL)
                {
                    LogError("unable to create a Map");
                }
                if (Map_AddOrUpdate(propertiesMap, "poll event hub 5s timeout reached", "POLLEVENTHUBTIMEOUT") != MAP_OK)
                {
                    LogError("unable to Map_AddOrUpdate POLLEVENTHUBTIMEOUT");
                    result = -1;
                }
                msgConfig.size = strlen("poll event hub 5s timeout reached");
                msgConfig.source = "poll event hub 5s timeout reached";
                msgConfig.sourceProperties = propertiesMap;
                if (createAndPublishGatewayMessage(msgConfig, handleData, &msgBusMessage, 1) != IOTHUBMESSAGE_ACCEPTED)
                {
                    LogError("iot hub message rejected");
                    result = -1;
                }
                Message_Destroy(msgBusMessage);
                result = 0;
            }

            messagereceiver_destroy(message_receiver);
            link_destroy(link);
            session_destroy(session);
            connection_destroy(connection);
            platform_deinit();

            LogInfo("Max memory usage:%lu\r\n", (unsigned long)amqpalloc_get_maximum_memory_used());
            LogInfo("Current memory usage:%lu\r\n", (unsigned long)amqpalloc_get_current_memory_used());

#ifdef _CRTDBG_MAP_ALLOC
            _CrtDumpMemoryLeaks();
#endif
        }
    }
    return result;
}

static void sendCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *userContextCallback)
{
    IOTHUBDEVICEPING_HANDLE_DATA *moduleHandleData = (IOTHUBDEVICEPING_HANDLE_DATA *)userContextCallback;
    LogInfo("sendCallback enter\r\n");
    if (result == IOTHUB_CLIENT_CONFIRMATION_OK)
    {
        // poll event hub
        if (pollEventhub(moduleHandleData) != 0)
        {
            LogError("pollEventhub failed");
        }
        else
        {
            LogInfo("pollEventhub succeeded\r\n");
        }
    }
}

static IOTHUBDEVICEPING_CONFIG *Config_Clone(const IOTHUBDEVICEPING_CONFIG *config)
{
    IOTHUBDEVICEPING_CONFIG *clonedConfig = malloc(sizeof(IOTHUBDEVICEPING_CONFIG));
    if (clonedConfig == NULL)
    {
        LogError("malloc returned NULL\r\n");
    }
    else
    {
        if ((clonedConfig->DeviceConnectionString = (const char *)malloc(strlen(config->DeviceConnectionString) + 1)) != NULL)
        {
            memcpy((char *)clonedConfig->DeviceConnectionString, (char *)config->DeviceConnectionString, strlen(config->DeviceConnectionString) + 1);
        }
        else
        {
            LogError("config->DeviceConnectionString malloc returned NULL\r\n");
        }
        if ((clonedConfig->EH_HOST = (const char *)malloc(strlen(config->EH_HOST) + 1)) != NULL)
        {
            memcpy((char *)clonedConfig->EH_HOST, (char *)config->EH_HOST, strlen(config->EH_HOST) + 1);
        }
        else
        {
            LogError("config->EH_HOST malloc returned NULL\r\n");
        }
        if ((clonedConfig->EH_KEY_NAME = (const char *)malloc(strlen(config->EH_KEY_NAME) + 1)) != NULL)
        {
            memcpy((char *)clonedConfig->EH_KEY_NAME, (char *)config->EH_KEY_NAME, strlen(config->EH_KEY_NAME) + 1);
        }
        else
        {
            LogError("config->EH_KEY_NAME malloc returned NULL\r\n");
        }
        if ((clonedConfig->EH_KEY = (const char *)malloc(strlen(config->EH_KEY) + 1)) != NULL)
        {
            memcpy((char *)clonedConfig->EH_KEY, (char *)config->EH_KEY, strlen(config->EH_KEY) + 1);
        }
        else
        {
            LogError("config->EH_KEY malloc returned NULL\r\n");
        }
        if ((clonedConfig->EH_COMP_NAME = (const char *)malloc(strlen(config->EH_COMP_NAME) + 1)) != NULL)
        {
            memcpy((char *)clonedConfig->EH_COMP_NAME, (char *)config->EH_COMP_NAME, strlen(config->EH_COMP_NAME) + 1);
        }
        else
        {
            LogError("config->EH_COMP_NAME malloc returned NULL\r\n");
        }
        if ((clonedConfig->EH_PARTITION_NUM = (const char *)malloc(strlen(config->EH_PARTITION_NUM) + 1)) != NULL)
        {
            memcpy((char *)clonedConfig->EH_PARTITION_NUM, (char *)config->EH_PARTITION_NUM, strlen(config->EH_PARTITION_NUM) + 1);
        }
        else
        {
            LogError("config->EH_PARTITION_NUM malloc returned NULL\r\n");
        }
    }
    return clonedConfig;
}

static MODULE_HANDLE IoTHubDevicePing_Create(BROKER_HANDLE brokerHandle, const void *configuration)
{
    g_echoreply = 0;
    IOTHUBDEVICEPING_HANDLE_DATA *result = NULL;
    if (
        (brokerHandle == NULL) ||
        (configuration == NULL) ||
        (((const IOTHUBDEVICEPING_CONFIG *)configuration)->DeviceConnectionString == "NULL") ||
        (((const IOTHUBDEVICEPING_CONFIG *)configuration)->EH_HOST == "NULL") ||
        (((const IOTHUBDEVICEPING_CONFIG *)configuration)->EH_KEY_NAME == "NULL") ||
        (((const IOTHUBDEVICEPING_CONFIG *)configuration)->EH_KEY == "NULL") ||
        (((const IOTHUBDEVICEPING_CONFIG *)configuration)->EH_COMP_NAME == "NULL"))
    {
        LogError("invalid arg brokerHandle=%p, configuration=%p\r\n", brokerHandle, configuration);
        result = NULL;
    }
    else
    {

        result = malloc(sizeof(IOTHUBDEVICEPING_HANDLE_DATA));
        if (result == NULL)
        {
            LogError("malloc returned NULL\r\n");
            /*return as is*/
        }
        else
        {
            result->config = (IOTHUBDEVICEPING_CONFIG *)Config_Clone(configuration);
            result->iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(((const IOTHUBDEVICEPING_CONFIG *)configuration)->DeviceConnectionString, HTTP_Protocol);
            if (result->iotHubClientHandle == NULL)
            {
                free(result);
                result = NULL;
                LogError("iotHubClientHandle returned NULL\r\n");
            }
            else
            {
                result->broker = brokerHandle;
                result->lockHandle = Lock_Init();
                if (result->lockHandle == NULL)
                {
                    LogError("unable to Lock_Init");
                    free(result);
                    result = NULL;
                }
                else
                {
                    if (ThreadAPI_Create(&result->threadHandle, devicepingThread, result) != THREADAPI_OK)
                    {
                        LogError("failed to spawn a thread");
                        (void)Lock_Deinit(result->lockHandle);
                        free(result);
                        result = NULL;
                    }
                    else
                    {
                        /*all is fine apparently*/
                    }
                }
            }
        }
    }
    return result;
}

static void IoTHubDevicePing_Destroy(MODULE_HANDLE moduleHandle)
{
    /*first stop the thread*/
    IOTHUBDEVICEPING_HANDLE_DATA *handleData = moduleHandle;
    int notUsed;
    if (handleData->iotHubClientHandle != NULL)
    {
        IoTHubClient_LL_Destroy(handleData->iotHubClientHandle);
    }
    if (Lock(handleData->lockHandle) != LOCK_OK)
    {
        LogError("not able to Lock, still setting the thread to finish");
    }
    else
    {
        Unlock(handleData->lockHandle);
    }
    if (ThreadAPI_Join(handleData->threadHandle, &notUsed) != THREADAPI_OK)
    {
        LogError("unable to ThreadAPI_Join, still proceeding in _Destroy");
    }
    (void)Lock_Deinit(handleData->lockHandle);
    free(handleData);
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

static void IoTHubDevicePing_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    (void)moduleHandle;
    (void)messageHandle;
    /*no action, HelloWorld is not interested in any messages*/
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