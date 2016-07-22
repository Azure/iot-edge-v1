// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "e2e_module.h"
#include "azure_c_shared_utility/threadapi.h"
#include "message.h"
#include "messageproperties.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"

#include "module.h"
#include "message_bus.h"

#define GW_E2E_MODULE "E2EMODULE"

typedef struct E2E_MODULE_DATA_TAG
{
	MESSAGE_BUS_HANDLE  bus;
    THREAD_HANDLE       e2eModulethread;
	char*               fakeMacAddress;
	char*               dataToSend;
} E2E_MODULE_DATA;


static void E2EModule_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
	(void)moduleHandle;
	(void)messageHandle;
    return;
}

static void E2EModule_Destroy(MODULE_HANDLE moduleHandle)
{
	if (moduleHandle == NULL)
	{
		LogError("Attempt to destroy NULL module");
	}
	else
	{
		E2E_MODULE_DATA* module_data = (E2E_MODULE_DATA*)moduleHandle;
		int result;

		/* join the thread */
        if (ThreadAPI_Join(module_data->e2eModulethread, &result) != THREADAPI_OK)
		{
			LogError("Attempt to ThreadAPI_Join Failed.");
		}		

		free((void*)module_data->fakeMacAddress);
		free((void*)module_data->dataToSend);
		free(module_data);
	}
}

static int e2e_module_worker(void * user_data)
{
    E2E_MODULE_DATA* module_data = (E2E_MODULE_DATA*)user_data;

	MESSAGE_CONFIG newMessageCfg;
	MAP_HANDLE newProperties = Map_Create(NULL);

	if (newProperties == NULL)
	{
		LogError("Failed to create message properties");
	}
	else
	{
		if (Map_Add(newProperties, GW_SOURCE_PROPERTY, GW_E2E_MODULE) != MAP_OK)
		{
			LogError("Failed to set source property");
		}
		else if (Map_Add(newProperties, GW_MAC_ADDRESS_PROPERTY, module_data->fakeMacAddress) != MAP_OK)
		{
			LogError("Failed to set macAddress property");
		}
		else
		{
			MESSAGE_HANDLE newMessage;

			newMessageCfg.sourceProperties = newProperties;
			newMessageCfg.size = strlen(module_data->dataToSend);
			newMessageCfg.source = (const unsigned char*)module_data->dataToSend;

			newMessage = Message_Create(&newMessageCfg);
			if (newMessage == NULL)
			{
				LogError("Failed to create new message");
			}
			else
			{
				if (MessageBus_Publish(module_data->bus, (MODULE_HANDLE)module_data, newMessage) != MESSAGE_BUS_OK)
				{
					LogError("Failed to publish module data to the message bus.");
				}
				Message_Destroy(newMessage);
			}
		}
		Map_Destroy(newProperties);
	}

	return 0;
}


static MODULE_HANDLE E2EModule_Create(MESSAGE_BUS_HANDLE busHandle, const void* configuration)
{
    E2E_MODULE_DATA * result;
    if (busHandle == NULL || configuration == NULL)
    {
		LogError("invalid Fake E2E module args.");
        result = NULL;
    }
    else
    {
        /* allocate module data struct */
        result = (E2E_MODULE_DATA*)malloc(sizeof(E2E_MODULE_DATA));
        if (result == NULL)
        {
			LogError("couldn't allocate memory for E2E Module");
        }
        else
        {
			E2EMODULE_CONFIG* e2eModuleConfig = (E2EMODULE_CONFIG*)configuration;
			int status;

			/* save the message bus */
			result->bus = busHandle;

			/* save fake MacAddress */
			status = mallocAndStrcpy_s(&(result->fakeMacAddress), e2eModuleConfig->macAddress);
			if (status != 0)
			{
				LogError("MacAddress did not copy");
				free(result);
				result = NULL;
			}
			else
			{
				status = mallocAndStrcpy_s(&(result->dataToSend), e2eModuleConfig->sendData);
				if (status != 0) 
				{
					LogError("MacAddress did not copy");
					free(result->fakeMacAddress);
					free(result);
					result = NULL;
				}
				else
				{
					/* Create a fake data thread.  */
					if (ThreadAPI_Create(
						&(result->e2eModulethread),
						e2e_module_worker,
						(void*)result) != THREADAPI_OK)
					{
						LogError("ThreadAPI_Create failed");
						free(result->dataToSend);
						free(result->fakeMacAddress);
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
    }
	return result;
}

/*
 *	Required for all modules:  the public API and the designated implementation functions.
 */
static const MODULE_APIS E2EModule_APIS_all =
{
	E2EModule_Create,
	E2EModule_Destroy,
	E2EModule_Receive
};

MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void)
{
	return &E2EModule_APIS_all;
}
