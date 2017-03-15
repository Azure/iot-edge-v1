// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include "proxy_gateway.h"
#include "native_module_host.h"

int main(int argc, char** argv)
{
    REMOTE_MODULE_HANDLE remote_module;
    if (argc != 2)
    {
        printf("usage: native_host_sample control_channel_id\n");
        printf("where control_channel_id is the name of the control channel (used in URI).\n");
    }
    else
    {
        if ((remote_module = ProxyGateway_Attach(Module_GetApi(MODULE_API_VERSION_1), argv[1])) == NULL)
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
