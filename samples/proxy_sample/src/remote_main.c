// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include "proxy_gateway.h"
#include "module.h"

static BROKER_HANDLE theBrokerHandle;

void* RemoteSample_ParseConfigurationFromJson(const char* configuration)
{
    printf("Remote Module ParseConfigurationFromJson: %s\n", configuration);
    return (void*)configuration;
}
void RemoteSample_FreeConfiguration(void* configuration)
{
    printf("Remote Module FreeConfiguration: %s\n", (char *)configuration);
}
MODULE_HANDLE RemoteSample_Create(BROKER_HANDLE broker, const void* configuration)
{
    (void)configuration;
    printf("Remote Module Create\n");
    theBrokerHandle = broker;
    MODULE_HANDLE m = (MODULE_HANDLE)"remote module";
    return m;
}
void RemoteSample_Destroy(MODULE_HANDLE moduleHandle)
{
    printf("Remote Module Destroy: %s\n", (char *)moduleHandle);
}
void RemoteSample_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    printf("Remote Module Receive: %s, %p\n", (char *)moduleHandle, messageHandle);
    Broker_Publish(theBrokerHandle, moduleHandle, messageHandle);
}
void RemoteSample_Start(MODULE_HANDLE moduleHandle)
{
    printf("Remote Module Start: %s\n", (char *)moduleHandle);
}
 
const MODULE_API_1 remoteModuleApi =
{
	{MODULE_API_VERSION_1},

	RemoteSample_ParseConfigurationFromJson,
	RemoteSample_FreeConfiguration,
	RemoteSample_Create,
	RemoteSample_Destroy,
	RemoteSample_Receive,
	RemoteSample_Start
};

const MODULE_API * pRemoteModuleApi = (const MODULE_API *)&remoteModuleApi;


int main(int argc, char** argv)
{
    REMOTE_MODULE_HANDLE remote_module;

    // Log received parameters to file
    FILE * param_log = fopen("param.txt", "w+");
    if (param_log) {
        for (int i = 0; i < argc; ++i) {
            fprintf(param_log, "%s\n", argv[i]);
        }
        fclose(param_log);
    }

    if (argc != 2)
    {
        printf("usage: proxy_sample_remote control_channel_id\n");
        printf("where control_channel_id is the name of the control channel (used in URI).\n");
    }
    else
    {
        if ((remote_module = ProxyGateway_Attach(pRemoteModuleApi, argv[1])) == NULL)
        {
            printf("failed to attach remote module from JSON\n");
        }
        else if (0 != ProxyGateway_StartWorkerThread(remote_module))
        {
            printf("failed to start the worker thread\n");
        }
        else
        {
            printf("remote module successfully created\n");
            printf("remote module shall run until ENTER is pressed\n");
            (void)getchar();
			ProxyGateway_Detach(remote_module);
        }
    }
    return 0;
}
