// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "simulated_device.h"
#include "simulated_device_hl.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "message.h"
#include "messageproperties.h"

#include "module.h"
#include "broker.h"
#include "parson.h"

static void SimulatedDevice_HL_Start(MODULE_HANDLE moduleHandle)
{
	MODULE_APIS apis;
	MODULE_STATIC_GETAPIS(SIMULATED_DEVICE_MODULE)(&apis);
	if (apis.Module_Start != NULL)
	{
		apis.Module_Start(moduleHandle);
	}
}

static void SimulatedDevice_HL_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
	MODULE_APIS apis;
	MODULE_STATIC_GETAPIS(SIMULATED_DEVICE_MODULE)(&apis);
	apis.Module_Receive(moduleHandle, messageHandle);
}

static void SimulatedDevice_HL_Destroy(MODULE_HANDLE moduleHandle)
{
	MODULE_APIS apis;
	MODULE_STATIC_GETAPIS(SIMULATED_DEVICE_MODULE)(&apis);
	apis.Module_Destroy(moduleHandle);
}

static MODULE_HANDLE SimulatedDevice_HL_Create(BROKER_HANDLE broker, const void* configuration)
{
	MODULE_HANDLE result;
    if (broker == NULL || configuration == NULL)
    {
		LogError("invalid SIMULATED_DEVICE_HL module args.");
        result = NULL;
    }
    else
    {
        JSON_Value* json = json_parse_string((const char*)configuration);
        if (json == NULL)
        {
            LogError("unable to json_parse_string");
            result = NULL;
        }
        else
        {
            JSON_Object* root = json_value_get_object(json);
            if (root == NULL)
            {
                LogError("unable to json_value_get_object");
                result = NULL;
            }
            else
            {
                const char* macAddress = json_object_get_string(root, "macAddress");
                if (macAddress == NULL)
                {
                    LogError("unable to json_object_get_string");
                    result = NULL;
                }
                else
                {
					MODULE_APIS apis;
					MODULE_STATIC_GETAPIS(SIMULATED_DEVICE_MODULE)(&apis);
                    result = apis.Module_Create(broker, macAddress);
                }
            }
            json_value_free(json);
        }
    }
	return result;
}

/*
 *	Required for all modules:  the public API and the designated implementation functions.
 */
static const MODULE_APIS SimulatedDevice_HL_APIS_all =
{
	SimulatedDevice_HL_Create,
	SimulatedDevice_HL_Destroy,
	SimulatedDevice_HL_Receive, 
	SimulatedDevice_HL_Start
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT void MODULE_STATIC_GETAPIS(SIMULATED_DEVICE_HL_MODULE)(MODULE_APIS* apis)
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
		(*apis) = SimulatedDevice_HL_APIS_all;
	}
}
