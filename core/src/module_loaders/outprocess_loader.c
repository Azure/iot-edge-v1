// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/uniqueid.h"
#include "azure_c_shared_utility/urlencode.h"

#include "parson.h"
#include "module.h"
#include "module_loader.h"
#include "module_loaders/outprocess_loader.h"
#include "module_loaders/outprocess_module.h"

DEFINE_ENUM_STRINGS(OUTPROCESS_LOADER_ACTIVATION_TYPE, OUTPROCESS_LOADER_ACTIVATION_TYPE_VALUES);

#define LOADER_GUID_SIZE 37
#define IPC_URI_HEAD "ipc://"
#define IPC_URI_HEAD_SIZE 6
#define MESSAGE_URI_SIZE (INPROC_URI_HEAD_SIZE + LOADER_GUID_SIZE +1)

typedef struct OUTPROCESS_MODULE_HANDLE_DATA_TAG
{
	const MODULE_API* api;
} OUTPROCESS_MODULE_HANDLE_DATA;

static MODULE_LIBRARY_HANDLE OutprocessModuleLoader_Load(const MODULE_LOADER* loader, const void* entrypoint)
{
	OUTPROCESS_MODULE_HANDLE_DATA * result;

	if (loader == NULL || entrypoint == NULL)
	{
		/*Codes_SRS_OUTPROCESS_LOADER_17_001: [ If loader or entrypoint are NULL, then this function shall return NULL. ] */
		result = NULL;
		LogError(
			"invalid input - loader = %p, entrypoint = %p",
			loader, entrypoint
		);
	}
	else
	{
		if (loader->type != OUTPROCESS)
		{
			/*Codes_SRS_OUTPROCESS_LOADER_17_042: [ If the loader type is not OUTPROCESS, then this function shall return NULL. ] */
			result = NULL;
			LogError("loader->type is not remote");
		}
		else
		{
			/*Codes_SRS_OUTPROCESS_LOADER_17_002: [ If the entrypoint's outprocess_loader_args or control_id are NULL, then this function shall return NULL. ] */
			/*Codes_SRS_OUTPROCESS_LOADER_17_003: [ If the entrypoint's activation_type is not NONE, the this function shall return NULL. ] */
			OUTPROCESS_LOADER_ENTRYPOINT * outprocess_entry = (OUTPROCESS_LOADER_ENTRYPOINT*)entrypoint;
			if ((outprocess_entry->control_id == NULL) ||
				(outprocess_entry->activation_type != OUTPROCESS_LOADER_ACTIVATION_NONE))
			{
				result = NULL;
				LogError("Invalid arguments activation type=[%s]", 
					ENUM_TO_STRING(OUTPROCESS_LOADER_ACTIVATION_TYPE, outprocess_entry->activation_type));
			}
			else
			{
				/*Codes_SRS_OUTPROCESS_LOADER_17_004: [ The loader shall allocate memory for the loader handle. ] */
				result = (OUTPROCESS_MODULE_HANDLE_DATA*)malloc(sizeof(OUTPROCESS_MODULE_HANDLE_DATA));
				if (result == NULL)
				{
					/*Codes_SRS_OUTPROCESS_LOADER_17_008: [ If any call in this function fails, this function shall return NULL. ] */
					LogError("malloc(sizeof(OUTPROCESS_MODULE_HANDLE_DATA)) failed.");
				}
				else
				{
					/*Codes_SRS_OUTPROCESS_LOADER_17_006: [ The loader shall store the Outprocess_Module_API_all in the loader handle. ] */
					/*Codes_SRS_OUTPROCESS_LOADER_17_007: [ Upon success, this function shall return a valid pointer to the loader handle. ] */
					result->api = (const MODULE_API*)&Outprocess_Module_API_all;
				}
			}
		}
	}
	return result;
}

static const MODULE_API* OutprocessModuleLoader_GetModuleApi(const MODULE_LOADER* loader, MODULE_LIBRARY_HANDLE moduleLibraryHandle)
{
	(void)loader;

	const MODULE_API* result;

	if (moduleLibraryHandle == NULL)
	{
		result = NULL;
		LogError("moduleLibraryHandle is NULL");
	}
	else
	{
		/*Codes_SRS_OUTPROCESS_LOADER_17_009: [ This function shall return a valid pointer to the Outprocess_Module_API_all MODULE_API. ] */
		OUTPROCESS_MODULE_HANDLE_DATA* loader_data = moduleLibraryHandle;
		result = loader_data->api;
	}
	return result;
}

static void OutprocessModuleLoader_Unload(const struct MODULE_LOADER_TAG* loader, MODULE_LIBRARY_HANDLE handle)
{
	(void)loader;

	if (handle != NULL)
	{
		/*Codes_SRS_OUTPROCESS_LOADER_17_011: [ This function shall release all resources created by this loader. ] */
		OUTPROCESS_MODULE_HANDLE_DATA* loader_data = (OUTPROCESS_MODULE_HANDLE_DATA*)handle;

		free(loader_data);
	}
	else
	{
		/*Codes_SRS_OUTPROCESS_LOADER_17_010: [ This function shall do nothing if moduleLibraryHandle is NULL. ] */
		LogError("moduleLibraryHandle is NULL");
	}
}

static void* OutprocessModuleLoader_ParseEntrypointFromJson(const struct MODULE_LOADER_TAG* loader, const JSON_Value* json)
{
	(void)loader;
	// The input is a JSON object that looks like this:
	//  "entrypoint": {
	//		"activation.type" : "none",
	//		"control.id" : "outproc_module_control", 
	//		"message.id" : "outproc_module_message", (optional)
	//		}
	//  }
	OUTPROCESS_LOADER_ENTRYPOINT * config;
	if (json == NULL)
	{
		/*Codes_SRS_OUTPROCESS_LOADER_17_012: [ This function shall return NULL if json is NULL. ] */
		LogError("json input is NULL");
		config = NULL;
	}
	else
	{
		// "json" must be an "object" type
		if (json_value_get_type(json) != JSONObject)
		{
			LogError("'json' is not an object value");
			config = NULL;
		}
		else
		{
			JSON_Object* entrypoint = json_value_get_object(json);
			if (entrypoint == NULL)
			{
				LogError("json_value_get_object failed");

				config = NULL;
			}
			else
			{
				const char* activationType = json_object_get_string(entrypoint, "activation.type");
				const char* controlId = json_object_get_string(entrypoint, "control.id");
				const char* messageId = json_object_get_string(entrypoint, "message.id");
				/*Codes_SRS_OUTPROCESS_LOADER_17_013: [ This function shall return NULL if "activation.type" is not present in json. ] */
				/*Codes_SRS_OUTPROCESS_LOADER_17_014: [ This function shall return NULL if "activation.type is not "NONE".  */
				/*Codes_SRS_OUTPROCESS_LOADER_17_041: [ This function shall return NULL if "control.id" is not present in json. ] */
				if ((activationType == NULL) || (controlId ==NULL) || strcmp(activationType, "none"))
				{
					LogError("Invalid JSON parameters, activation type=[%s], controlURI=[%p]", 
						activationType, controlId);

					/*Codes_SRS_OUTPROCESS_LOADER_17_021: [ This function shall return NULL if any calls fails. ]*/
					config = NULL;
				}
				else
				{
					/*Codes_SRS_OUTPROCESS_LOADER_17_016: [ This function shall allocate a OUTPROCESS_LOADER_ENTRYPOINT structure. ] */
					config = (OUTPROCESS_LOADER_ENTRYPOINT*)malloc(sizeof(OUTPROCESS_LOADER_ENTRYPOINT));
					if (config == NULL)
					{
						/*Codes_SRS_OUTPROCESS_LOADER_17_021: [ This function shall return NULL if any calls fails. ] */
						LogError("entrypoint allocation failed");
					}
					else
					{
						/*Codes_SRS_OUTPROCESS_LOADER_17_017: [ This function shall assign the entrypoint activation_type to NONE. ] */
						config->activation_type = OUTPROCESS_LOADER_ACTIVATION_NONE;
						/*Codes_SRS_OUTPROCESS_LOADER_17_018: [ This function shall assign the entrypoint control_id to the string value of "control.id" in json, NULL if not present. ] */
						config->control_id = URL_EncodeString(controlId);
						if (config->control_id == NULL)
						{
							/*Codes_SRS_OUTPROCESS_LOADER_17_021: [ This function shall return NULL if any calls fails. ] */
							LogError("Could not allocate loader args string");
							free(config);
							config = NULL;
						}
						else
						{
							/*Codes_SRS_OUTPROCESS_LOADER_17_019: [ This function shall assign the entrypoint message_id to the string value of "message.id" in json, NULL if not present. ] */
							config->message_id = URL_EncodeString(messageId);
							/*Codes_SRS_OUTPROCESS_LOADER_17_022: [ This function shall return a valid pointer to an OUTPROCESS_LOADER_ENTRYPOINT on success. ]*/
						}
					}
				}
			}
		}
	}
	return (void*)config;
}

static void OutprocessModuleLoader_FreeEntrypoint(const struct MODULE_LOADER_TAG* loader, void* entrypoint)
{
	(void)loader;

	if (entrypoint != NULL)
	{
		/*Codes_SRS_OUTPROCESS_LOADER_17_023: [ This function shall release all resources allocated by OutprocessModuleLoader_ParseEntrypointFromJson. ] */
		OUTPROCESS_LOADER_ENTRYPOINT* ep = (OUTPROCESS_LOADER_ENTRYPOINT*)entrypoint;
		if (ep->message_id != NULL)
			STRING_delete(ep->message_id); 
		STRING_delete(ep->control_id);
		free(ep);
	}
	else
	{
		LogError("entrypoint is NULL");
	}
}

static MODULE_LOADER_BASE_CONFIGURATION* OutprocessModuleLoader_ParseConfigurationFromJson(const struct MODULE_LOADER_TAG* loader, const JSON_Value* json)
{
	(void)loader;
	/*Codes_SRS_OUTPROCESS_LOADER_17_024: [ The out of process loader does not have any configuration. So this method shall return NULL. ]*/
	return NULL;
}

static void OutprocessModuleLoader_FreeConfiguration(const struct MODULE_LOADER_TAG* loader, MODULE_LOADER_BASE_CONFIGURATION* configuration)
{
	(void)loader;
	(void)configuration;
	/*Codes_SRS_OUTPROCESS_LOADER_17_025: [ This function shall move along, nothing to free here. ]*/
}

static void* OutprocessModuleLoader_BuildModuleConfiguration(
	const MODULE_LOADER* loader,
	const void* entrypoint,
	const void* module_configuration
)
{
	(void)loader;
	OUTPROCESS_MODULE_CONFIG * fullModuleConfiguration;

	if ((entrypoint == NULL) || (module_configuration == NULL))
	{
		/*Codes_SRS_OUTPROCESS_LOADER_17_026: [ This function shall return NULL if loader, entrypoint, control_id, or module_configuration is NULL. ] */
		LogError("Remote Loader needs both entry point and module configuration");
		fullModuleConfiguration = NULL;
	}
	else
	{
		/*Codes_SRS_OUTPROCESS_LOADER_17_027: [ This function shall allocate a OUTPROCESS_MODULE_CONFIG structure. ]*/
		fullModuleConfiguration = (OUTPROCESS_MODULE_CONFIG*)malloc(sizeof(OUTPROCESS_MODULE_CONFIG));
		if (fullModuleConfiguration == NULL)
		{
			/*Codes_SRS_OUTPROCESS_LOADER_17_036: [ If any call fails, this function shall return NULL. ]*/
			LogError("couldn't allocate module config");
		}
		else
		{
			OUTPROCESS_LOADER_ENTRYPOINT* ep = (OUTPROCESS_LOADER_ENTRYPOINT*)entrypoint;
			char uuid[LOADER_GUID_SIZE];
			UNIQUEID_RESULT uuid_result = UNIQUEID_OK;
			if (ep->message_id == NULL)
			{
				/*Codes_SRS_OUTPROCESS_LOADER_17_029: [ If the entrypoint's message_id is NULL, then the loader shall construct an IPC uri. ]*/
				memset(uuid, 0, LOADER_GUID_SIZE);
				/*Codes_SRS_OUTPROCESS_LOADER_17_030: [ The loader shall create a unique id, if needed for URI constrution. ]*/
				uuid_result = UniqueId_Generate(uuid, LOADER_GUID_SIZE);
				if (uuid_result != UNIQUEID_OK)
				{
					LogError("Unable to generate unique Id.");
					fullModuleConfiguration->message_uri = NULL;
				}
				else
				{
					/*Codes_SRS_OUTPROCESS_LOADER_17_032: [ The message uri shall be composed of "ipc://" + unique id . ]*/
					fullModuleConfiguration->message_uri = STRING_construct_sprintf("%s%s", IPC_URI_HEAD, uuid);
				}
			}
			else
			{
				/*Codes_SRS_OUTPROCESS_LOADER_17_033: [ This function shall allocate and copy each string in OUTPROCESS_LOADER_ENTRYPOINT and assign them to the corresponding fields in OUTPROCESS_MODULE_CONFIG. ]*/
				fullModuleConfiguration->message_uri = STRING_construct_sprintf("%s%s", IPC_URI_HEAD, STRING_c_str(ep->message_id));
			}
			if (fullModuleConfiguration->message_uri == NULL)
			{
				/*Codes_SRS_OUTPROCESS_LOADER_17_036: [ If any call fails, this function shall return NULL. ]*/
				LogError("unable to create a message channel URI");
				free(fullModuleConfiguration);
				fullModuleConfiguration = NULL;
			}
			else 
			{
				/*Codes_SRS_OUTPROCESS_LOADER_17_033: [ This function shall allocate and copy each string in OUTPROCESS_LOADER_ENTRYPOINT and assign them to the corresponding fields in OUTPROCESS_MODULE_CONFIG. ]*/
				fullModuleConfiguration->control_uri = STRING_construct_sprintf("%s%s", IPC_URI_HEAD, STRING_c_str(ep->control_id));
				if (fullModuleConfiguration->control_uri == NULL)
				{
					/*Codes_SRS_OUTPROCESS_LOADER_17_036: [ If any call fails, this function shall return NULL. ]*/
					LogError("unable to allocate a control channel URI");
					STRING_delete(fullModuleConfiguration->message_uri);
					free(fullModuleConfiguration);
					fullModuleConfiguration = NULL;
				}
				else
				{
					/*Codes_SRS_OUTPROCESS_LOADER_17_033: [ This function shall allocate and copy each string in OUTPROCESS_LOADER_ENTRYPOINT and assign them to the corresponding fields in OUTPROCESS_MODULE_CONFIG. ]*/
					/*Codes_SRS_OUTPROCESS_LOADER_17_034: [ This function shall allocate and copy the module_configuration string and assign it the OUTPROCESS_MODULE_CONFIG::outprocess_module_args field. ]*/
					fullModuleConfiguration->outprocess_module_args = STRING_clone((STRING_HANDLE)module_configuration);
					if (fullModuleConfiguration->outprocess_module_args == NULL)
					{
						/*Codes_SRS_OUTPROCESS_LOADER_17_036: [ If any call fails, this function shall return NULL. ]*/
						LogError("unable to allocate a loader arguments string");
						STRING_delete(fullModuleConfiguration->message_uri);
						STRING_delete(fullModuleConfiguration->control_uri);
						free(fullModuleConfiguration);
						fullModuleConfiguration = NULL;
					}
					else
					{
						/*Codes_SRS_OUTPROCESS_LOADER_17_035: [ Upon success, this function shall return a valid pointer to an OUTPROCESS_MODULE_CONFIG structure. ]*/
						fullModuleConfiguration->lifecycle_model = OUTPROCESS_LIFECYCLE_SYNC;
					}
				}
			}
		}
	}

	return (void *)fullModuleConfiguration;
}

static void OutprocessModuleLoader_FreeModuleConfiguration(const struct MODULE_LOADER_TAG* loader, const void* module_configuration)
{
	(void)loader;
	if (module_configuration != NULL)
	{
		/*Codes_SRS_OUTPROCESS_LOADER_17_037: [ This function shall release all memory allocated by OutprocessModuleLoader_BuildModuleConfiguration. ]*/
		OUTPROCESS_MODULE_CONFIG * config = (OUTPROCESS_MODULE_CONFIG*)module_configuration;
		STRING_delete(config->control_uri);
		STRING_delete(config->message_uri);
		STRING_delete(config->outprocess_module_args);
		free(config);
	}
}


static MODULE_LOADER_API Outprocess_Module_Loader_API =
{
    .Load = OutprocessModuleLoader_Load,
    .Unload = OutprocessModuleLoader_Unload,
    .GetApi = OutprocessModuleLoader_GetModuleApi,

    .ParseEntrypointFromJson = OutprocessModuleLoader_ParseEntrypointFromJson,
    .FreeEntrypoint = OutprocessModuleLoader_FreeEntrypoint,

    .ParseConfigurationFromJson = OutprocessModuleLoader_ParseConfigurationFromJson,
    .FreeConfiguration = OutprocessModuleLoader_FreeConfiguration,

    .BuildModuleConfiguration = OutprocessModuleLoader_BuildModuleConfiguration,
    .FreeModuleConfiguration = OutprocessModuleLoader_FreeModuleConfiguration
};

/*Codes_SRS_OUTPROCESS_LOADER_17_039: [ MODULE_LOADER::type shall be OUTPROCESS. ]*/
/*Codes_SRS_OUTPROCESS_LOADER_17_040: [ MODULE_LOADER::name shall be the string 'outprocess'. ]*/
static MODULE_LOADER OutProcess_Module_Loader =
{
    OUTPROCESS,
    OUTPROCESS_LOADER_NAME,
    NULL,
    &Outprocess_Module_Loader_API
};

const MODULE_LOADER* OutprocessLoader_Get(void)
{
	/*Codes_SRS_OUTPROCESS_LOADER_17_038: [ OutprocessModuleLoader_Get shall return a non-NULL pointer to a MODULE_LOADER struct. ]*/
    return &OutProcess_Module_Loader;
}
