// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>

#include "event_system.h"
#include "module_loaders/dynamic_loader.h"

const char *hello_module_path = "../../../modules/hello_world/Debug/hello_world.dll";

void started_callback(GATEWAY_HANDLE gw, GATEWAY_EVENT event_type, GATEWAY_EVENT_CTX ctx, void* user_param)
{
    printf("Gateway is being started, called with param: \"%d\".\n", *(int*)user_param);
}

void module_callback(GATEWAY_HANDLE gw, GATEWAY_EVENT event_type, GATEWAY_EVENT_CTX ctx, void* user_param)
{
    VECTOR_HANDLE modules = (VECTOR_HANDLE)ctx;
    GATEWAY_MODULE_INFO *module_info = (GATEWAY_MODULE_INFO*)VECTOR_front(modules);
    printf("Gateway had a new module added, module callback with first module name: \"%s\"\n", module_info->module_name);
}

void my_callback(GATEWAY_HANDLE gw, GATEWAY_EVENT event_type, GATEWAY_EVENT_CTX ctx, void* user_param)
{
    printf("Gateway is being destroyed, first callback called with param: \"%d\".\n", *(int*)user_param);
}

void second_callback(GATEWAY_HANDLE gw, GATEWAY_EVENT event_type, GATEWAY_EVENT_CTX ctx, void* user_param)
{
    printf("Gateway is being destroyed, second callback called with param: \"%d\".\n", *(int*)user_param);
}

int main()
{
    int param1 = 1, param2 = 2, param3 = 3;
    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    
    
    Gateway_AddEventCallback(gw, GATEWAY_STARTED, started_callback, &param1);
    Gateway_AddEventCallback(gw, GATEWAY_MODULE_LIST_CHANGED, module_callback, NULL);
    Gateway_AddEventCallback(gw, GATEWAY_DESTROYED, my_callback, &param2);
    Gateway_AddEventCallback(gw, GATEWAY_DESTROYED, second_callback, &param3);
    
    printf("Gateway is running.\n");

    Gateway_Start(gw);

	STRING_HANDLE module_path_string = STRING_construct(hello_module_path);
	DYNAMIC_LOADER_ENTRYPOINT loader_cfg = {
		module_path_string
    };

    GATEWAY_MODULES_ENTRY new_module = {
        .module_name = "test module",
		.module_loader_info = {
			DynamicLoader_Get(),
			&loader_cfg
		},
        .module_configuration = NULL
    };

    Gateway_AddModule(gw, &new_module);
    Gateway_Destroy(gw);
    printf("Gateway was destroyed, exiting main function.\n");
}