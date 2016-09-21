// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "azure_c_shared_utility/gballoc.h"

#include "iothubdeviceping_hl.h"
#include "iothubdeviceping.h"
#include "azure_c_shared_utility/xlogging.h"
#include "parson.h"

#define MESSAGECONTENT "MessageContent"
#define DEVICECONNECTIONSTRING "DeviceConnectionString"


static MODULE_HANDLE IoTHubDevicePing_HL_Create(BROKER_HANDLE brokerHandle, const void* configuration)
{
    MODULE_HANDLE *result;
    if ((brokerHandle == NULL) || (configuration == NULL))
    {
        LogError("Invalid NULL parameter, brokerHandle=[%p] configuration=[%p]", brokerHandle, configuration);
        result = NULL;
    }
    else
    {
        JSON_Value* json = json_parse_string((const char*)configuration);
        if (json == NULL)
        {
            LogError("Unable to parse json string");
            result = NULL;
        }
        else
        {
            JSON_Object* obj = json_value_get_object(json);
            if (obj == NULL)
            {
                LogError("Expected a JSON Object in configuration");
                result = NULL;
            }
            else
            {
                const char * DeviceConnectionString;
                if ((DeviceConnectionString = json_object_get_string(obj, DEVICECONNECTIONSTRING)) == "NULL") // NULL
                {
                    /*Codes_SRS_IOTHUBHTTP_HL_17_006: [ If the JSON object does not contain a value named "IoTHubName" then IoTHubHttp_HL_Create shall fail and return NULL. ]*/
                    LogError("Did not find expected %s configuration", DEVICECONNECTIONSTRING);
                    result = NULL;
                }
                else
                {
                    /*Codes_SRS_IOTHUBHTTP_HL_17_008: [ IoTHubHttp_HL_Create shall invoke iothubhttp Module's create, using the brokerHandle, IotHubName, and IoTHubSuffix. ]*/
                    IOTHUBDEVICEPING_CONFIG llConfiguration;
                    llConfiguration.DeviceConnectionString = DeviceConnectionString;
                    result = MODULE_STATIC_GETAPIS(IOTHUBDEVICEPING_MODULE)()->Module_Create(brokerHandle, &llConfiguration);
                }
            }
            json_value_free(json);
        }
    }
    return result;
}

static void IoTHubDevicePing_HL_Destroy(MODULE_HANDLE moduleHandle)
{
    /*Codes_SRS_IOTHUBHTTP_HL_17_012: [ IoTHubHttp_HL_Destroy shall free all used resources. ]*/
    MODULE_STATIC_GETAPIS(IOTHUBDEVICEPING_MODULE)()->Module_Destroy(moduleHandle);
}


static void IoTHubDevicePing_HL_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    /*Codes_SRS_IOTHUBHTTP_HL_17_011: [ IoTHubHttp_HL_Receive shall pass the received parameters to the underlying IoTHubHttp receive function. ]*/
    MODULE_STATIC_GETAPIS(IOTHUBDEVICEPING_MODULE)()->Module_Receive(moduleHandle, messageHandle);
}

/*Codes_SRS_IOTHUBHTTP_HL_17_014: [ The MODULE_APIS structure shall have non-NULL Module_Create, Module_Destroy, and Module_Receive fields. ]*/
static const MODULE_APIS IoTHubDevicePing_HL_GetAPIS_Impl =
{
    IoTHubDevicePing_HL_Create,
    IoTHubDevicePing_HL_Destroy,
    IoTHubDevicePing_HL_Receive
};


#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(IOTHUBDEVICEPING_MODULE_HL)(void)
#else
MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void)
#endif
{
    /*Codes_SRS_IOTHUBHTTP_HL_17_013: [ Module_GetAPIS shall return a non-NULL pointer. ]*/
    return &IoTHubDevicePing_HL_GetAPIS_Impl;
}