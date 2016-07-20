// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "simulated_device.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "message.h"
#include "messageproperties.h"

#include "module.h"
#include "message_bus.h"

typedef struct SIMULATEDDEVICE_DATA_TAG
{
	MESSAGE_BUS_HANDLE	bus;
    THREAD_HANDLE       simulatedDeviceThread;
    const char *        fakeMacAddress;
    unsigned int                 simulatedDeviceRunning : 1;
} SIMULATEDDEVICE_DATA;

static char msgText[1024];

static void SimulatedDevice_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
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
					LogError("Failed to set source property");
				}
				else
				{
					newMessageCfg.sourceProperties = newProperties;
					if ((avgTemperature + additionalTemp) > maxSpeed)
						additionalTemp = 0.0;

					if (sprintf_s(msgText, sizeof(msgText), "{\"temperature\": %.2f}", avgTemperature + additionalTemp) < 0)
					{
						LogError("Failed to set message text");
					}
					else
					{
						newMessageCfg.size = strlen(msgText);
						newMessageCfg.source = (const unsigned char*)msgText;

						MESSAGE_HANDLE newMessage = Message_Create(&newMessageCfg);
						if (newMessage == NULL)
						{
							LogError("Failed to create new message");
						}
						else
						{
							if (MessageBus_Publish(module_data->bus, (MODULE_HANDLE)module_data, newMessage) != MESSAGE_BUS_OK)
							{
								LogError("Failed to create new message");
							}

							additionalTemp += 1.0;
							Message_Destroy(newMessage);
						}
					}
				}
				Map_Destroy(newProperties);
			}
			ThreadAPI_Sleep(10000);
		}
	}

	return 0;
}


static MODULE_HANDLE SimulatedDevice_Create(MESSAGE_BUS_HANDLE busHandle, const void* configuration)
{
    SIMULATEDDEVICE_DATA * result;
    if (busHandle == NULL || configuration == NULL)
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
			/* save the message bus */
			result->bus = busHandle;
			/* set module is running to true */
			result->simulatedDeviceRunning = 1;
			/* save fake MacAddress */
			char * newFakeAdress;
			int status = mallocAndStrcpy_s(&newFakeAdress, configuration);

			if (status != 0)
			{
				LogError("MacAddress did not copy");
			}
			else
			{
				result->fakeMacAddress = newFakeAdress;
				/* OK to start */
				/* Create a fake data thread.  */
				if (ThreadAPI_Create(
					&(result->simulatedDeviceThread),
					simulated_device_worker,
					(void*)result) != THREADAPI_OK)
				{
					LogError("ThreadAPI_Create failed");
					free(result);
					result = NULL;
				}
				else
				{
					/* Thread started, module created, all complete.*/
				}
			}

        }
    }
	return result;
}

/*
 *	Required for all modules:  the public API and the designated implementation functions.
 */
static const MODULE_APIS SimulatedDevice_APIS_all =
{
	SimulatedDevice_Create,
	SimulatedDevice_Destroy,
	SimulatedDevice_Receive
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(SIMULATED_DEVICE_MODULE)(void)
#else
MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void)
#endif
{
	return &SimulatedDevice_APIS_all;
}
