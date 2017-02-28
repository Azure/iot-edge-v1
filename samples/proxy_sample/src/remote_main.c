// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include "proxy_gateway.h"
#include "module.h"

void* test_ParseConfigurationFromJson(const char* configuration)
{
    printf("test_ParseConfigurationFromJson: %s\n", configuration);
    return (void*)configuration;
}
void test_FreeConfiguration(void* configuration)
{
    printf("test_FreeConfiguration: %s\n", (char *)configuration);
}
MODULE_HANDLE test_Create(BROKER_HANDLE broker, const void* configuration)
{
    printf("test_Create\n");
    MODULE_HANDLE m = (MODULE_HANDLE)"remote module";
    return m;
}
void test_Destroy(MODULE_HANDLE moduleHandle)
{
    printf("test_Destroy: %s\n", (char *)moduleHandle);
}
void test_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    printf("test_Receive: %s, %p\n", (char *)moduleHandle, messageHandle);
}
void test_Start(MODULE_HANDLE moduleHandle)
{
    printf("test_Start: %s\n", (char *)moduleHandle);
}
 
const MODULE_API_1 oopModule =
{
	{MODULE_API_VERSION_1},

	test_ParseConfigurationFromJson,
	test_FreeConfiguration,
	test_Create,
	test_Destroy,
	test_Receive,
	test_Start
};

const MODULE_API * pOopModule = (const MODULE_API *)&oopModule;


int main(int argc, char** argv)
{
    REMOTE_MODULE_HANDLE remote_module;
    if (argc != 2)
    {
        printf("usage: proxy_sample_sample control_channel_id\n");
        printf("where control_channel_id is the name of the control channel (used in URI).\n");
    }
    else
    {
        if ((remote_module = ProxyGateway_Attach(pOopModule, argv[1])) == NULL)
        {
            printf("failed to attach remote module from JSON\n");
        }
        else
        {
            (void)RemoteModule_StartWorkerThread(remote_module);
            printf("remote module successfully created\n");
            printf("remote module shall run until ENTER is pressed\n");
            (void)getchar();
			ProxyGateway_Detach(remote_module);
        }
    }
    return 0;
}
