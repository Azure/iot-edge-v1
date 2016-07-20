// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "module.h"
#include "azure_c_shared_utility/xlogging.h"
#include <stdio.h>
#include "hello_world.h"
#include "hello_world_hl.h"

static MODULE_HANDLE HelloWorld_HL_Create(MESSAGE_BUS_HANDLE busHandle, const void* configuration)
{
    MODULE_HANDLE result;
    if((result = MODULE_STATIC_GETAPIS(HELLOWORLD_MODULE)()->Module_Create(busHandle, configuration))==NULL)
    {
        LogError("unable to Module_Create HELLOWORLD static");
    }
    else
    {
        /*all is fine, return as is*/
    }
    return result;
}

static void HelloWorld_HL_Destroy(MODULE_HANDLE module)
{
    MODULE_STATIC_GETAPIS(HELLOWORLD_MODULE)()->Module_Destroy(module);
}

static void HelloWorld_HL_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    MODULE_STATIC_GETAPIS(HELLOWORLD_MODULE)()->Module_Receive(moduleHandle, messageHandle);
}

static const MODULE_APIS HelloWorld_HL_APIS_all =
{
	HelloWorld_HL_Create,
	HelloWorld_HL_Destroy,
	HelloWorld_HL_Receive
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(HELLOWORLD_HL_MODULE)(void)
#else
MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void)
#endif
{
	return &HelloWorld_HL_APIS_all;
}
