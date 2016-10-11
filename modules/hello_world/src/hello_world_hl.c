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

static MODULE_HANDLE HelloWorld_HL_Create(BROKER_HANDLE broker, const void* configuration)
{
    MODULE_HANDLE result;
	MODULE_APIS apis;
	MODULE_STATIC_GETAPIS(HELLOWORLD_MODULE)(&apis);
    if((result = apis.Module_Create(broker, configuration))==NULL)
    {
        LogError("unable to Module_Create HELLOWORLD static");
    }
    else
    {
        /*all is fine, return as is*/
    }
    return result;
}

static void HelloWorld_HL_Start(MODULE_HANDLE moduleHandle)
{
	MODULE_APIS apis;
	MODULE_STATIC_GETAPIS(HELLOWORLD_MODULE)(&apis);
	if (apis.Module_Start != NULL)
	{
		apis.Module_Start(moduleHandle);
	}
}

static void HelloWorld_HL_Destroy(MODULE_HANDLE module)
{
	MODULE_APIS apis;
	MODULE_STATIC_GETAPIS(HELLOWORLD_MODULE)(&apis);
	apis.Module_Destroy(module);
}

static void HelloWorld_HL_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
	MODULE_APIS apis;
	MODULE_STATIC_GETAPIS(HELLOWORLD_MODULE)(&apis);
	apis.Module_Receive(moduleHandle, messageHandle);
}

static const MODULE_APIS HelloWorld_HL_APIS_all =
{
	HelloWorld_HL_Create,
	HelloWorld_HL_Destroy,
	HelloWorld_HL_Receive,
	HelloWorld_HL_Start
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT void MODULE_STATIC_GETAPIS(HELLOWORLD_HL_MODULE)(MODULE_APIS* apis)
#else
MODULE_EXPORT void Module_GetAPIS(MODULE_APIS* apis)
#endif
{
	if (!apis)
	{
		LogError("NULL passed to Module_GetAPIS");
	}
	else
	{
		(*apis) = HelloWorld_HL_APIS_all;
	}
}
