// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "azure_c_shared_utility/gballoc.h"

#include "iothubdeviceping_hl.h"
#include "iothubdeviceping.h"
#include "iothubtransporthttp.h"
#include "azure_c_shared_utility/xlogging.h"
#include "parson.h"

static MODULE_HANDLE IoTHubDevicePing_HL_Create(BROKER_HANDLE brokerHandle, const void *configuration)
{
    MODULE_HANDLE *result;
    if ((brokerHandle == NULL) || (configuration == NULL))
    {
        LogError("Invalid NULL parameter, brokerHandle=[%p] configuration=[%p]", brokerHandle, configuration);
        result = NULL;
    }
    else
    {
        JSON_Value *json = json_parse_string((const char *)configuration);
        if (json == NULL)
        {
            LogError("Unable to parse json string");
            result = NULL;
        }
        else
        {
            JSON_Object *obj = json_value_get_object(json);
            if (obj == NULL)
            {
                LogError("Expected a JSON Object in configuration");
                result = NULL;
            }
            else
            {
                const char *DeviceConnectionString;
                const char *eventHubHost;
                const char *eventHubKeyName;
                const char *eventHubKey;
                const char *eventHubCompatibleName;
                const char *eventHubPartitionNum;
                if ((DeviceConnectionString = json_object_get_string(obj, "DeviceConnectionString")) == "NULL" ||
                    (eventHubHost = json_object_get_string(obj, "EH_HOST")) == "NULL" ||
                    (eventHubKeyName = json_object_get_string(obj, "EH_KEY_NAME")) == "NULL" ||
                    (eventHubKey = json_object_get_string(obj, "EH_KEY")) == "NULL" ||
                    (eventHubCompatibleName = json_object_get_string(obj, "EH_COMP_NAME")) == "NULL" ||
                    (eventHubPartitionNum = json_object_get_string(obj, "EH_PARTITION_NUM")) == "NULL") // NULL
                {
                    LogError("Did not find expected configuration");
                    result = NULL;
                }
                else
                {
                    MODULE_APIS apis;
                    MODULE_STATIC_GETAPIS(IOTHUBDEVICEPING_MODULE)(&apis);
                    IOTHUBDEVICEPING_CONFIG llConfiguration;
                    llConfiguration.DeviceConnectionString = DeviceConnectionString;
                    llConfiguration.EH_HOST = eventHubHost;
                    llConfiguration.EH_KEY_NAME = eventHubKeyName;
                    llConfiguration.EH_KEY = eventHubKey;
                    llConfiguration.EH_COMP_NAME = eventHubCompatibleName;
                    llConfiguration.EH_PARTITION_NUM = eventHubPartitionNum;
                    result = apis.Module_Create(brokerHandle, &llConfiguration);
                }
            }
            json_value_free(json);
        }
    }
    return result;
}

static void IoTHubDevicePing_HL_Destroy(MODULE_HANDLE moduleHandle)
{
    MODULE_APIS apis;
    MODULE_STATIC_GETAPIS(IOTHUBDEVICEPING_MODULE)(&apis);
    apis.Module_Destroy(moduleHandle);
}

static void IoTHubDevicePing_HL_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    MODULE_APIS apis;
    MODULE_STATIC_GETAPIS(IOTHUBDEVICEPING_MODULE)(&apis);
    apis.Module_Receive(moduleHandle, messageHandle);
}

static const MODULE_APIS IoTHubDevicePing_HL_GetAPIS_Impl =
{
    IoTHubDevicePing_HL_Create,
    IoTHubDevicePing_HL_Destroy,
    IoTHubDevicePing_HL_Receive
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT void MODULE_STATIC_GETAPIS(IOTHUBDEVICEPING_MODULE_HL)(MODULE_APIS* apis)
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
        (*apis) = IoTHubDevicePing_HL_GetAPIS_Impl;
    }
}