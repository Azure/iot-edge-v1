// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "simulated_device.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "messageproperties.h"
#include "message.h"
#include "module.h"
#include "broker.h"

#include <parson.h>

typedef struct SIMULATEDDEVICE_DATA_TAG
{
    BROKER_HANDLE       broker;
    THREAD_HANDLE       simulatedDeviceThread;
    const char *        fakeMacAddress;
    unsigned int        messagePeriod;
    unsigned int        simulatedDeviceRunning : 1;
} SIMULATEDDEVICE_DATA;

typedef struct SIMULATEDDEVICE_CONFIG_TAG
{
    char *              macAddress;
    unsigned int        messagePeriod;
} SIMULATEDDEVICE_CONFIG;

static void SimulatedDevice_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    // Print the properties & content of the received message
    CONSTMAP_HANDLE properties = Message_GetProperties(messageHandle);
    if (properties != NULL)
    {
        const char* addr = ((SIMULATEDDEVICE_DATA*)moduleHandle)->fakeMacAddress;

        // We're only interested in cloud-to-device (C2D) messages addressed to
        // this device
        if (ConstMap_ContainsKey(properties, GW_MAC_ADDRESS_PROPERTY) == true &&
            strcmp(addr, ConstMap_GetValue(properties, GW_MAC_ADDRESS_PROPERTY)) == 0)
        {
            const char* const * keys;
            const char* const * values;
            size_t count;

            if (ConstMap_GetInternals(properties, &keys, &values, &count) == CONSTMAP_OK)
            {
                const CONSTBUFFER* content = Message_GetContent(messageHandle);
                if (content != NULL)
                {
                    (void)printf(
                        "Received a message\r\n"
                        "Properties:\r\n"
                        );

                    for (size_t i = 0; i < count; ++i)
                    {
                        (void)printf("  %s = %s\r\n", keys[i], values[i]);
                    }

                    (void)printf("Content:\r\n");
                    (void)printf("  %.*s\r\n", (int)content->size, content->buffer);
                    (void)fflush(stdout);
                }
            }
        }

        ConstMap_Destroy(properties);
    }

    return;
}

static void SimulatedDevice_Destroy(MODULE_HANDLE moduleHandle)
{
    if (moduleHandle == NULL)
    {
        LogError("Attempt to destroy NULL module");
    }
    else
    {
        SIMULATEDDEVICE_DATA* module_data = (SIMULATEDDEVICE_DATA*)moduleHandle;
        int result;

        /* Tell thread to stop */
        module_data->simulatedDeviceRunning = 0;
        /* join the thread */
        ThreadAPI_Join(module_data->simulatedDeviceThread, &result);
        /* free module data */
        free((void*)module_data->fakeMacAddress);
        free(module_data);
    }
}

static int simulated_device_worker(void * user_data)
{
    SIMULATEDDEVICE_DATA* module_data = (SIMULATEDDEVICE_DATA*)user_data;
    double avgTemperature = 10.0;
    double additionalTemp = 0.0;
    double maxSpeed = 40.0;

    if (user_data != NULL)
    {

        while (module_data->simulatedDeviceRunning)
        {
            ThreadAPI_Sleep(module_data -> messagePeriod);

            MESSAGE_CONFIG newMessageCfg;
            MAP_HANDLE newProperties = Map_Create(NULL);
            if (newProperties == NULL)
            {
                LogError("Failed to create message properties");
            }

            else
            {
                if (Map_Add(newProperties, GW_SOURCE_PROPERTY, GW_SOURCE_BLE_TELEMETRY) != MAP_OK)
                {
                    LogError("Failed to set source property");
                }
                else if (Map_Add(newProperties, GW_MAC_ADDRESS_PROPERTY, module_data->fakeMacAddress) != MAP_OK)
                {
                    LogError("Failed to set address property");
                }
                else
                {
                    char msgText[128];

                    newMessageCfg.sourceProperties = newProperties;
                    if ((avgTemperature + additionalTemp) > maxSpeed)
                        additionalTemp = 0.0;

                    if (sprintf_s(msgText, sizeof(msgText), "{\"temperature\": %.2f}", avgTemperature + additionalTemp) < 0)
                    {
                        LogError("Failed to set message text");
                    }
                    else
                    {
                        (void)printf("Device: %s, Temperature: %.2f\r\n",
                            module_data->fakeMacAddress,
                            avgTemperature + additionalTemp
                            );
                        (void)fflush(stdout);

                        newMessageCfg.size = strlen(msgText);
                        newMessageCfg.source = (const unsigned char*)msgText;

                        MESSAGE_HANDLE newMessage = Message_Create(&newMessageCfg);
                        if (newMessage == NULL)
                        {
                            LogError("Failed to create new message");
                        }
                        else
                        {
                            if (Broker_Publish(module_data->broker, (MODULE_HANDLE)module_data, newMessage) != BROKER_OK)
                            {
                                LogError("Failed to publish new message");
                            }

                            additionalTemp += 1.0;
                            Message_Destroy(newMessage);
                        }
                    }
                }
                Map_Destroy(newProperties);
            }
        }
    }

    return 0;
}

static void SimulatedDevice_Start(MODULE_HANDLE moduleHandle)
{
    if (moduleHandle == NULL)
    {
        LogError("Attempt to start NULL module");
    }
    else
    {
        MESSAGE_CONFIG newMessageCfg;
        MAP_HANDLE newProperties = Map_Create(NULL);
        if (newProperties == NULL)
        {
            LogError("Failed to create message properties");
        }
        else
        {
            SIMULATEDDEVICE_DATA* module_data = (SIMULATEDDEVICE_DATA*)moduleHandle;

            if (Map_Add(newProperties, GW_SOURCE_PROPERTY, GW_SOURCE_BLE_TELEMETRY) != MAP_OK)
            {
                LogError("Failed to set source property");
            }
            else if (Map_Add(newProperties, GW_MAC_ADDRESS_PROPERTY, module_data->fakeMacAddress) != MAP_OK)
            {
                LogError("Failed to set address property");
            }
            else if (Map_Add(newProperties, "deviceFunction", "register") != MAP_OK)
            {
                LogError("Failed to set deviceFunction property");
            }
            
            else
            {
                    newMessageCfg.size = 0;
                    newMessageCfg.source = NULL;
                    newMessageCfg.sourceProperties = newProperties;

                    MESSAGE_HANDLE newMessage = Message_Create(&newMessageCfg);
                    if (newMessage == NULL)
                    {
                        LogError("Failed to create register message");
                    }
                    else
                    {
                        if (Broker_Publish(module_data->broker, (MODULE_HANDLE)module_data, newMessage) != BROKER_OK)
                        {
                            LogError("Failed to publish register message");
                        }

                        Message_Destroy(newMessage);
                    }
                }
            }
            Map_Destroy(newProperties);

            SIMULATEDDEVICE_DATA* module_data = (SIMULATEDDEVICE_DATA*)moduleHandle;
            /* OK to start */
            /* Create a fake data thread.  */
            if (ThreadAPI_Create(
                &(module_data->simulatedDeviceThread),
                simulated_device_worker,
                (void*)module_data) != THREADAPI_OK)
            {
                LogError("ThreadAPI_Create failed");
                module_data->simulatedDeviceThread = NULL;
            }
            else
            {
                /* Thread started, module created, all complete.*/
            }
    }
}

static MODULE_HANDLE SimulatedDevice_Create(BROKER_HANDLE broker, const void* configuration)
{
    SIMULATEDDEVICE_DATA * result;
    SIMULATEDDEVICE_CONFIG * config = (SIMULATEDDEVICE_CONFIG *) configuration;
    if (broker == NULL || config == NULL)
    {
        LogError("invalid SIMULATED DEVICE module args.");
        result = NULL;
    }
    else
    {
        /* allocate module data struct */
        result = (SIMULATEDDEVICE_DATA*)malloc(sizeof(SIMULATEDDEVICE_DATA));
        if (result == NULL)
        {
            LogError("couldn't allocate memory for BLE Module");
        }
        else
        {
            /* save the message broker */
            result->broker = broker;
            /* set module is running to true */
            result->simulatedDeviceRunning = 1;
            /* save fake MacAddress */
            char * newFakeAddress;
            int status = mallocAndStrcpy_s(&newFakeAddress, config -> macAddress);

            if (status != 0)
            {
                LogError("MacAddress did not copy");
            }
            else
            {
                result->fakeMacAddress = newFakeAddress;
                result -> messagePeriod = config -> messagePeriod;
                result->simulatedDeviceThread = NULL;

            }

        }
    }
    return result;
}

static void * SimulatedDevice_ParseConfigurationFromJson(const char* configuration)
{
    SIMULATEDDEVICE_CONFIG * result;
    if (configuration == NULL)
    {
        LogError("invalid module args.");
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
                SIMULATEDDEVICE_CONFIG config;
                const char* macAddress = json_object_get_string(root, "macAddress");
                if (macAddress == NULL)
                {
                    LogError("unable to json_object_get_string");
                    result = NULL;
                }
                else
                {
                    int period = (int)json_object_get_number(root, "messagePeriod");
                    if (period <= 0)
                    {
                        LogError("Invalid period time specified");
                        result = NULL;
                    }
                    else
                    {
                        if (mallocAndStrcpy_s(&(config.macAddress), macAddress) != 0)
                        {
                            result = NULL;
                        }
                        else
                        {
                            config.messagePeriod = period;
                            result = malloc(sizeof(SIMULATEDDEVICE_CONFIG));
                            if (result == NULL) {
                                free(config.macAddress);
                                LogError("allocation of configuration failed");
                            }
                            else
                            {
                                *result = config;
                            }
                        }
                    }
                }
            }
            json_value_free(json);
        }
    }
    return result;
}

void SimulatedDevice_FreeConfiguration(void * configuration)
{
    if (configuration != NULL)
    {
        SIMULATEDDEVICE_CONFIG * config = (SIMULATEDDEVICE_CONFIG *)configuration;
        free(config->macAddress);
        free(config);
    }
}

/*
 *    Required for all modules:  the public API and the designated implementation functions.
 */
static const MODULE_API_1 SimulatedDevice_APIS_all =
{
    {MODULE_API_VERSION_1},

    SimulatedDevice_ParseConfigurationFromJson,
    SimulatedDevice_FreeConfiguration,
    SimulatedDevice_Create,
    SimulatedDevice_Destroy,
    SimulatedDevice_Receive,
    SimulatedDevice_Start
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(SIMULATED_DEVICE_MODULE)(MODULE_API_VERSION gateway_api_version)
#else
MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version)
#endif
{
    (void)gateway_api_version;
    return (const MODULE_API *)&SimulatedDevice_APIS_all;
}
