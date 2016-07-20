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
#include "message_bus.h"
#include "parson.h"

static void SimulatedDevice_HL_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
	MODULE_STATIC_GETAPIS(SIMULATED_DEVICE_MODULE)()->Module_Receive(moduleHandle, messageHandle);
}

static void SimulatedDevice_HL_Destroy(MODULE_HANDLE moduleHandle)
{
	MODULE_STATIC_GETAPIS(SIMULATED_DEVICE_MODULE)()->Module_Destroy(moduleHandle);
}

static MODULE_HANDLE SimulatedDevice_HL_Create(MESSAGE_BUS_HANDLE busHandle, const void* configuration)
{
	MODULE_HANDLE result;
    if (busHandle == NULL || configuration == NULL)
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
                    result = MODULE_STATIC_GETAPIS(SIMULATED_DEVICE_MODULE)()->Module_Create(busHandle, macAddress);
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
	SimulatedDevice_HL_Receive
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(SIMULATED_DEVICE_HL_MODULE)(void)
#else
MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void)
#endif
{
	return &SimulatedDevice_HL_APIS_all;
}
