// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "module.h"
#include "azure_c_shared_utility/xlogging.h"

#include "azure_c_shared_utility/threadapi.h"
#include "hello_world.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/lock.h"

typedef struct HELLOWORLD_HANDLE_DATA_TAG
{
    THREAD_HANDLE threadHandle;
    LOCK_HANDLE lockHandle;
    int stopThread;
    BROKER_HANDLE broker;

}HELLOWORLD_HANDLE_DATA;

#define HELLOWORLD_MESSAGE "hello world"

int helloWorldThread(void *param)
{
    HELLOWORLD_HANDLE_DATA* handleData = param;

    MESSAGE_CONFIG msgConfig;
    MAP_HANDLE propertiesMap = Map_Create(NULL);
    if(propertiesMap == NULL)
    {
        LogError("unable to create a Map");
    }
    else
    {
        if (Map_AddOrUpdate(propertiesMap, "helloWorld", "from Azure IoT Gateway SDK simple sample!") != MAP_OK)
        {
            LogError("unable to Map_AddOrUpdate");
        }
        else
        {
            msgConfig.size = strlen(HELLOWORLD_MESSAGE);
            msgConfig.source = HELLOWORLD_MESSAGE;
    
            msgConfig.sourceProperties = propertiesMap;

            MESSAGE_HANDLE helloWorldMessage = Message_Create(&msgConfig);
            if (helloWorldMessage == NULL)
            {
                LogError("unable to create \"hello world\" message");
            }
            else
            {
                while (1)
                {
                    if (Lock(handleData->lockHandle) == LOCK_OK)
                    {
                        if (handleData->stopThread)
                        {
                            (void)Unlock(handleData->lockHandle);
                            break; /*gets out of the thread*/
                        }
                        else
                        {
                            (void)Broker_Publish(handleData->broker, (MODULE_HANDLE)handleData, helloWorldMessage);
                            (void)Unlock(handleData->lockHandle);
                        }
                    }
                    else
                    {
                        /*shall retry*/
                    }
                    (void)ThreadAPI_Sleep(5000); /*every 5 seconds*/
                }
                Message_Destroy(helloWorldMessage);
            }
        }
    }
    return 0;
}

static MODULE_HANDLE HelloWorld_Create(BROKER_HANDLE broker, const void* configuration)
{
    HELLOWORLD_HANDLE_DATA* result;
    if (
        (broker == NULL) /*configuration is not used*/
        )
    {
        LogError("invalid arg broker=%p", broker);
        result = NULL;
    }
    else
    {
        result = malloc(sizeof(HELLOWORLD_HANDLE_DATA));
        if(result == NULL)
        {
            LogError("unable to malloc");
        }
        else
        {
            result->lockHandle = Lock_Init();
            if(result->lockHandle == NULL)
            {
                LogError("unable to Lock_Init");
                free(result);
                result = NULL;
            }
            else
            {
                result->stopThread = 0;
                result->broker = broker;
                result->threadHandle = NULL;
            }
        }
    }
    return result;
}

static void* HelloWorld_ParseConfigurationFromJson(const char* configuration)
{
	(void)configuration;
    return NULL;
}

static void HelloWorld_FreeConfiguration(void* configuration)
{
	(void)configuration;
}

static void HelloWorld_Start(MODULE_HANDLE module)
{
    HELLOWORLD_HANDLE_DATA* handleData = module;
    if (handleData != NULL)
    {
        if (Lock(handleData->lockHandle) != LOCK_OK)
        {
            LogError("not able to Lock, still setting the thread to finish");
            handleData->stopThread = 1;
        }
        else
        {
            if (ThreadAPI_Create(&handleData->threadHandle, helloWorldThread, handleData) != THREADAPI_OK)
            {
                LogError("failed to spawn a thread");
                handleData->threadHandle = NULL;
            }
            (void)Unlock(handleData->lockHandle);
        }
    }
}

static void HelloWorld_Destroy(MODULE_HANDLE module)
{
    /*first stop the thread*/
    HELLOWORLD_HANDLE_DATA* handleData = module;
    int notUsed;
    if (Lock(handleData->lockHandle) != LOCK_OK)
    {
        LogError("not able to Lock, still setting the thread to finish");
        handleData->stopThread = 1;
    }
    else
    {
        handleData->stopThread = 1;
        Unlock(handleData->lockHandle);
    }

    if (handleData->threadHandle != NULL &&
        ThreadAPI_Join(handleData->threadHandle, &notUsed) != THREADAPI_OK)
    {
        LogError("unable to ThreadAPI_Join, still proceeding in _Destroy");
    }
    
    (void)Lock_Deinit(handleData->lockHandle);
    free(handleData);
}

static void HelloWorld_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    /*no action, HelloWorld is not interested in any messages*/
}

static const MODULE_API_1 HelloWorld_API_all =
{
    {MODULE_API_VERSION_1},
    HelloWorld_ParseConfigurationFromJson,
	HelloWorld_FreeConfiguration,
    HelloWorld_Create,
    HelloWorld_Destroy,
    HelloWorld_Receive,
    HelloWorld_Start
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(HELLOWORLD_MODULE)(MODULE_API_VERSION gateway_api_version)
#else
MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version)
#endif
{
    const MODULE_API * api;
    if (gateway_api_version >= HelloWorld_API_all.base.version)
    {
        api= (const MODULE_API*)&HelloWorld_API_all;
    }
    else
    {
        api = NULL;
    }
    return api;
}
