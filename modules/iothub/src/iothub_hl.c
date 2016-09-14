// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <ctype.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "azure_c_shared_utility/gballoc.h"

#include "iothub_hl.h"
#include "iothub.h"
#include "iothubtransporthttp.h"
#include "iothubtransportamqp.h"
#include "iothubtransportmqtt.h"
#include "azure_c_shared_utility/xlogging.h"
#include "parson.h"

#define SUFFIX "IoTHubSuffix"
#define HUBNAME "IoTHubName"
#define TRANSPORT "Transport"

static int strcmp_i(const char* lhs, const char* rhs)
{
    char lc, rc;
    int cmp;

    do
    {
        lc = *lhs++;
        rc = *rhs++;
        cmp = tolower(lc) - tolower(rc);
    } while (cmp == 0 && lc != 0 && rc != 0);

    return cmp;
}

static MODULE_HANDLE IotHub_HL_Create(BROKER_HANDLE broker, const void* configuration)
{
    MODULE_HANDLE result;
    if ((broker == NULL) || (configuration == NULL))
    {
        /*Codes_SRS_IOTHUBMODULE_HL_17_001: [If `broker` is NULL then `IotHub_HL_Create` shall fail and return NULL.]*/
        /*Codes_SRS_IOTHUBMODULE_HL_17_002: [ If `configuration` is NULL then `IotHub_HL_Create` shall fail and return NULL. ]*/
        LogError("Invalid NULL parameter, broker=[%p] configuration=[%p]", broker, configuration);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_IOTHUBMODULE_HL_17_004: [ `IotHub_HL_Create` shall parse `configuration` as a JSON string. ]*/
        JSON_Value* json = json_parse_string((const char*)configuration);
        if (json == NULL)
        {
            /*Codes_SRS_IOTHUBMODULE_HL_17_003: [ If `configuration` is not a JSON string, then `IotHub_HL_Create` shall fail and return NULL. ]*/
            LogError("Unable to parse json string");
            result = NULL;
        }
        else
        {
            JSON_Object* obj = json_value_get_object(json);
            if (obj == NULL)
            {
                /*Codes_SRS_IOTHUBMODULE_HL_17_005: [ If parsing of `configuration` fails, `IotHub_HL_Create` shall fail and return NULL. ]*/
                LogError("Expected a JSON Object in configuration");
                result = NULL;
            }
            else
            {
                const char * IoTHubName;
                const char * IoTHubSuffix;
                const char * transport;
                if ((IoTHubName = json_object_get_string(obj, HUBNAME)) == NULL)
                {
                    /*Codes_SRS_IOTHUBMODULE_HL_17_006: [ If the JSON object does not contain a value named "IoTHubName" then `IotHub_HL_Create` shall fail and return NULL. ]*/
                    LogError("Did not find expected %s configuration", HUBNAME);
                    result = NULL;
                }
                else if ((IoTHubSuffix = json_object_get_string(obj, SUFFIX)) == NULL)
                {
                    /*Codes_SRS_IOTHUBMODULE_HL_17_007: [ If the JSON object does not contain a value named "IoTHubSuffix" then `IotHub_HL_Create` shall fail and return NULL. ]*/
                    LogError("Did not find expected %s configuration", SUFFIX);
                    result = NULL;
                }
                else if ((transport = json_object_get_string(obj, TRANSPORT)) == NULL)
                {
                    /*Codes_SRS_IOTHUBMODULE_HL_05_001: [ If the JSON object does not contain a value named "Transport" then `IotHub_HL_Create` shall fail and return NULL. ]*/
                    LogError("Did not find expected %s configuration", TRANSPORT);
                    result = NULL;
                }
                else
                {
                    /*Codes_SRS_IOTHUBMODULE_HL_05_002: [ If the value of "Transport" is not one of "HTTP", "AMQP", or "MQTT" (case-insensitive) then `IotHub_HL_Create` shall fail and return NULL. ]*/
                    /*Codes_SRS_IOTHUBMODULE_HL_17_008: [ `IotHub_HL_Create` shall invoke the IotHub module's create function, using the broker, IotHubName, IoTHubSuffix, and Transport. ]*/
                    bool foundTransport = true;
                    IOTHUB_CONFIG llConfiguration;
                    llConfiguration.IoTHubName = IoTHubName;
                    llConfiguration.IoTHubSuffix = IoTHubSuffix;

                    if (strcmp_i(transport, "HTTP") == 0)
                    {
                        llConfiguration.transportProvider = HTTP_Protocol;
                    }
                    else if (strcmp_i(transport, "AMQP") == 0)
                    {
                        llConfiguration.transportProvider = AMQP_Protocol;
                    }
                    else if (strcmp_i(transport, "MQTT") == 0)
                    {
                        llConfiguration.transportProvider = MQTT_Protocol;
                    }
                    else
                    {
                        foundTransport = false;
                        result = NULL;
                    }

                    if (foundTransport)
                    {
                        /*Codes_SRS_IOTHUBMODULE_HL_17_009: [ When the lower layer IotHub module creation succeeds, `IotHub_HL_Create` shall succeed and return a non-NULL value. ]*/
                        /*Codes_SRS_IOTHUBMODULE_HL_17_010: [ If the lower layer IotHub module creation fails, `IotHub_HL_Create` shall fail and return NULL. ]*/
                        result = MODULE_STATIC_GETAPIS(IOTHUB_MODULE)()->Module_Create(broker, &llConfiguration);
                    }
                }
            }
            json_value_free(json);
        }
    }
    return result;
}

static void IotHub_HL_Destroy(MODULE_HANDLE moduleHandle)
{
    /*Codes_SRS_IOTHUBMODULE_HL_17_012: [ IotHub_HL_Destroy shall free all used resources. ]*/
    MODULE_STATIC_GETAPIS(IOTHUB_MODULE)()->Module_Destroy(moduleHandle);
}


static void IotHub_HL_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    /*Codes_SRS_IOTHUBMODULE_HL_17_011: [ IotHub_HL_Receive shall pass the received parameters to the underlying IotHub module's receive function. ]*/
    MODULE_STATIC_GETAPIS(IOTHUB_MODULE)()->Module_Receive(moduleHandle, messageHandle);
}

/*Codes_SRS_IOTHUBMODULE_HL_17_014: [ The MODULE_APIS structure shall have non-NULL `Module_Create`, `Module_Destroy`, and `Module_Receive` fields. ]*/
static const MODULE_APIS moduleInterface =
{
    IotHub_HL_Create,
    IotHub_HL_Destroy,
    IotHub_HL_Receive
};


#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(IOTHUB_MODULE_HL)(void)
#else
MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void)
#endif
{
    /*Codes_SRS_IOTHUBMODULE_HL_17_013: [ `Module_GetAPIS` shall return a non-NULL pointer. ]*/
    return &moduleInterface;
}