// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>
#include "module.h"

#include "httprestapi.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/constmap.h"
#include "azure_c_shared_utility/constbuffer.h"

#include <parson.h>

typedef void(*add_publish_callback)(void* handle, const char* mac_address, const char* mbuf, size_t mbuf_size);

extern void* httpServer_Create(void* broker, const char* url, const char* port);
extern void httpServer_Start(void* handle);
extern void httpServer_Stop(void* handle);
extern void httpServer_AddPublishCallback(void* handle, add_publish_callback callback);

typedef struct HTTP_RESTAPI_HANDLE_DATA_TAG
{
    // TODO: Add specific members 
    BROKER_HANDLE broker;
    void* httpRestServer;
} HTTP_RESTAPI_HANDLE_DATA;

typedef struct HTTP_RESTAPI_CONFIG_TAG
{
    const char* url;
    const char* port;
} HTTP_RESTAPI_CONFIG;

#define HTTP_RESTAPI_MESSAGE "Message From HttpRestApi module"
#define GW_SOURCE_PROPERTY "source"

void PublishMessage(void* handle, const char* macAddress, const char* buf, size_t bufSize)
{
    // TODO: Add logic which send message to broker bus.
    HTTP_RESTAPI_HANDLE_DATA* moduleHandle = (HTTP_RESTAPI_HANDLE_DATA*)handle;

    MESSAGE_CONFIG msgConfig;
    MAP_HANDLE propsMap = Map_Create(NULL);
    if (propsMap==NULL)
    {
        LogError("unable create a Map");
    }
    else
    {
        if (Map_AddOrUpdate(propsMap, MSG_KEY_SOURCE, MSG_VALUE_SOURCE) != MAP_OK)
        {
            LogError("unable add source in a Map");
        }
        else if (Map_AddOrUpdate(propsMap, MSG_KEY_MAC_ADDRESS, macAddress) != MAP_OK)
        {
            LogError("unable add mac address in a Map");
        }
        else
        {
            msgConfig.size = bufSize;
            msgConfig.source = buf;
            msgConfig.sourceProperties = propsMap;
            MESSAGE_HANDLE msg = Message_Create(&msgConfig);
            if (msg == NULL)
            {
                LogError("unable create message");
            }
            else
            {
                if (Broker_Publish(moduleHandle->broker, moduleHandle, msg) != BROKER_OK)
                {
                    LogError("unable publish message");
                }
                Message_Destroy(msg);
            }
        }
    }
}

static MODULE_HANDLE HttpRestApi_Create(BROKER_HANDLE broker, const void* configuration)
{
    HTTP_RESTAPI_HANDLE_DATA* result;
    HTTP_RESTAPI_CONFIG* config = (HTTP_RESTAPI_CONFIG*)configuration;

	if (broker == NULL)
    {
        LogError("invalid arg broker=%p", broker);
        result = NULL;
    }
    else
    {
        result = (HTTP_RESTAPI_HANDLE_DATA*)malloc(sizeof(HTTP_RESTAPI_HANDLE_DATA));
        result->broker = broker;
        result->httpRestServer = httpServer_Create(broker, config->url, config->port);
        httpServer_AddPublishCallback(result->httpRestServer, PublishMessage);
    }
    return result;
}

// args:{"url":"http://localhost","port":"123456"} - example 
static void* HttpRestApi_ParseConfigurationFromJson(const char* configuration)
{
    HTTP_RESTAPI_CONFIG* result = NULL;
    if (configuration!=NULL)
    {
        JSON_Value* json = json_parse_string(configuration);
        if (json==NULL)
        {
            LogError("Failed to parse json configuration");
        }
        else
        {
            result = (HTTP_RESTAPI_CONFIG*)malloc(sizeof(HTTP_RESTAPI_CONFIG));
            JSON_Object* obj = json_value_get_object(json);
            const char* url = json_object_get_string(obj, "url");
            result->url = (const char*)malloc(strlen(url)+1);
            strcpy((char*)(result->url), url);
            const char* port = json_object_get_string(obj,"port");
            result->port = (const char*)malloc(strlen(port)+1);
            strcpy((char*)(result->port), port);
        }
    }
    return result;
}

static void HttpRestApi_FreeConfiguration(void* configuration)
{
    if (configuration!=NULL)
    {
        HTTP_RESTAPI_CONFIG* config = (HTTP_RESTAPI_CONFIG*)configuration;
        if (config->url!=NULL)
        {
            free((void*)(config->url));
        }
        if (config->port != NULL)
        {
            free((void*)(config->port));
        }
        free(configuration);
    }
}

static void HttpRestApi_Start(MODULE_HANDLE module)
{
    HTTP_RESTAPI_HANDLE_DATA* handleData = module;
    if (handleData != NULL)
    {
        httpServer_Start(handleData->httpRestServer);
    }
}

static void HttpRestApi_Destroy(MODULE_HANDLE module)
{
    HTTP_RESTAPI_HANDLE_DATA* handleData = module;
    httpServer_Stop(handleData->httpRestServer);
    free(handleData);
}

static void HttpRestApi_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    HTTP_RESTAPI_HANDLE_DATA* httpRestApiHandle = (HTTP_RESTAPI_HANDLE_DATA*)moduleHandle;
    if (httpRestApiHandle!=NULL)
    {
        CONSTMAP_HANDLE props = Message_GetProperties(messageHandle);
        if (props != NULL)
        {
            const char* source = ConstMap_GetValue(props, GW_SOURCE_PROPERTY);
            if (source != NULL)
            {
                LogInfo("Recieved mseesage from %s", source);
            }
            const char* httpRestApi = ConstMap_GetValue(props, "http-restapi");
            if (httpRestApi != NULL)
            {
                LogInfo("http-restapi:%s", httpRestApi);
            }
        }
    }
    const CONSTBUFFER* buffer = Message_GetContent(messageHandle);
    if (buffer != NULL)
    {
        // Just a sample!
        size_t size = buffer->size;
        const unsigned char* content = buffer->buffer;
        char* messageContent = (char*)malloc(size*2+1);
        for(size_t i=0;i<size;i++)
        {
            sprintf(&messageContent[i*2],"%02x", content[i]);
        }
        messageContent[size*2] = '\0';
        LogInfo("Received Content:size=%d,%s", size, messageContent);
        free(messageContent);
    }
}

static const MODULE_API_1 HttpRestApi_API_all =
{
    {MODULE_API_VERSION_1},
    HttpRestApi_ParseConfigurationFromJson,
	HttpRestApi_FreeConfiguration,
    HttpRestApi_Create,
    HttpRestApi_Destroy,
    HttpRestApi_Receive,
    HttpRestApi_Start
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(HTTP_RESTAPI_MODULE)(MODULE_API_VERSION gateway_api_version)
#else
MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version)
#endif
{
    const MODULE_API * api;
    if (gateway_api_version >= HttpRestApi_API_all.base.version)
    {
        api= (const MODULE_API*)&HttpRestApi_API_all;
    }
    else
    {
        api = NULL;
    }
    return api;
}
