// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>

#include "gateway_ll.h"

const char *hello_module_path = "../../modules/hello_world/Debug/hello_world.dll";

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

int main(int argc, char** argv)
{
	int param1 = 1, param2 = 2;
    GATEWAY_HANDLE gw = Gateway_LL_Create(NULL);
	
	Gateway_LL_AddEventCallback(gw, GATEWAY_MODULE_LIST_CHANGED, module_callback, NULL);
    Gateway_LL_AddEventCallback(gw, GATEWAY_DESTROYED, my_callback, &param1);
	Gateway_LL_AddEventCallback(gw, GATEWAY_DESTROYED, second_callback, &param2);
    
	printf("Gateway is running.\n");

	GATEWAY_MODULES_ENTRY new_module = {
		"test module",
		hello_module_path,
		NULL
	};

	Gateway_LL_AddModule(gw, &new_module);
    Gateway_LL_Destroy(gw);
	printf("Gateway was destroyed, exiting main function.\n");
}