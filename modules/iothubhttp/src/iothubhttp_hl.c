// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "azure_c_shared_utility/gballoc.h"

#include "iothubhttp_hl.h"
#include "iothubhttp.h"
#include "azure_c_shared_utility/xlogging.h"
#include "parson.h"

#define SUFFIX "IoTHubSuffix"
#define HUBNAME "IoTHubName"

static MODULE_HANDLE IoTHubHttp_HL_Create(MESSAGE_BUS_HANDLE busHandle, const void* configuration)
{
    MODULE_HANDLE *result;
    if ((busHandle == NULL) || (configuration == NULL))
    {
        /*Codes_SRS_IOTHUBHTTP_HL_17_001: [If busHandle is NULL then IoTHubHttp_HL_Create shall fail and return NULL.]*/
        /*Codes_SRS_IOTHUBHTTP_HL_17_002: [If configuration is NULL then IoTHubHttp_HL_Create shall fail and return NULL.]*/
        LogError("Invalid NULL parameter, busHandle=[%p] configuration=[%p]", busHandle, configuration);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_IOTHUBHTTP_HL_17_004: [ IoTHubHttp_HL_Create shall parse the configuration as a JSON string. ]*/
        JSON_Value* json = json_parse_string((const char*)configuration);
        if (json == NULL)
        {
            /*Codes_SRS_IOTHUBHTTP_HL_17_003: [ If configuration is not a JSON string, then IoTHubHttp_HL_Create shall fail and return NULL. ]*/
            LogError("Unable to parse json string");
            result = NULL;
        }
        else
        {
            JSON_Object* obj = json_value_get_object(json);
            if (obj == NULL)
            {
                /*Codes_SRS_IOTHUBHTTP_HL_17_005: [ If parsing configuration fails, IoTHubHttp_HL_Create shall fail and return NULL. ]*/
                LogError("Expected a JSON Object in configuration");
                result = NULL;
            }
            else
            {
                const char * IoTHubName;
                const char * IoTHubSuffix;
                if ((IoTHubName = json_object_get_string(obj, HUBNAME)) == NULL)
                {
                    /*Codes_SRS_IOTHUBHTTP_HL_17_006: [ If the JSON object does not contain a value named "IoTHubName" then IoTHubHttp_HL_Create shall fail and return NULL. ]*/
                    LogError("Did not find expected %s configuration", HUBNAME);
                    result = NULL;
                }
                else if ((IoTHubSuffix = json_object_get_string(obj, SUFFIX)) == NULL)
                {
                    /*Codes_SRS_IOTHUBHTTP_HL_17_007: [ If the JSON object does not contain a value named "IoTHubSuffix" then IoTHubHttp_HL_Create shall fail and return NULL. ]*/
                    LogError("Did not find expected %s configuration", SUFFIX);
                    result = NULL;
                }
                else
                {
                    /*Codes_SRS_IOTHUBHTTP_HL_17_008: [ IoTHubHttp_HL_Create shall invoke iothubhttp Module's create, using the busHandle, IotHubName, and IoTHubSuffix. ]*/
                    IOTHUBHTTP_CONFIG llConfiguration;
                    llConfiguration.IoTHubName = IoTHubName;
                    llConfiguration.IoTHubSuffix = IoTHubSuffix;
                    /*Codes_SRS_IOTHUBHTTP_HL_17_009: [ When the lower layer IoTHubHttp create succeeds, IoTHubHttp_HL_Create shall succeed and return a non-NULL value. ]*/
                    /*Codes_SRS_IOTHUBHTTP_HL_17_010: [ If the lower layer IoTHubHttp create fails, IoTHubHttp_HL_Create shall fail and return NULL. ]*/
                    result = MODULE_STATIC_GETAPIS(IOTHUBHTTP_MODULE)()->Module_Create(busHandle, &llConfiguration);
                }
            }
            json_value_free(json);
        }
    }
    return result;
}

static void IoTHubHttp_HL_Destroy(MODULE_HANDLE moduleHandle)
{
    /*Codes_SRS_IOTHUBHTTP_HL_17_012: [ IoTHubHttp_HL_Destroy shall free all used resources. ]*/
    MODULE_STATIC_GETAPIS(IOTHUBHTTP_MODULE)()->Module_Destroy(moduleHandle);
}


static void IoTHubHttp_HL_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    /*Codes_SRS_IOTHUBHTTP_HL_17_011: [ IoTHubHttp_HL_Receive shall pass the received parameters to the underlying IoTHubHttp receive function. ]*/
    MODULE_STATIC_GETAPIS(IOTHUBHTTP_MODULE)()->Module_Receive(moduleHandle, messageHandle);
}

/*Codes_SRS_IOTHUBHTTP_HL_17_014: [ The MODULE_APIS structure shall have non-NULL Module_Create, Module_Destroy, and Module_Receive fields. ]*/
static const MODULE_APIS IoTHubHttp_HL_GetAPIS_Impl =
{
    IoTHubHttp_HL_Create,
    IoTHubHttp_HL_Destroy,
    IoTHubHttp_HL_Receive
};


#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(IOTHUBHTTP_MODULE_HL)(void)
#else
MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void)
#endif
{
    /*Codes_SRS_IOTHUBHTTP_HL_17_013: [ Module_GetAPIS shall return a non-NULL pointer. ]*/
    return &IoTHubHttp_HL_GetAPIS_Impl;
}