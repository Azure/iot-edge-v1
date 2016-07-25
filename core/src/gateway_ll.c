// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"

#include <stddef.h>

#include "gateway_ll.h"
#include "message_bus.h"
#include "module_loader.h"

typedef struct GATEWAY_HANDLE_DATA_TAG {
	/** @brief Vector of MODULE_DATA modules that the Gateway must track */
	VECTOR_HANDLE modules;

	/** @brief The message bus contained within this Gateway */
	MESSAGE_BUS_HANDLE bus;
} GATEWAY_HANDLE_DATA;

typedef struct MODULE_DATA_TAG {
	/** @brief The MODULE_LIBRARY_HANDLE associated with 'module'*/
	MODULE_LIBRARY_HANDLE module_library_handle;

	/** @brief The MODULE_HANDLE of the same module that lives on the message bus.*/
	MODULE_HANDLE module;
} MODULE_DATA;

static MODULE_HANDLE gateway_addmodule_internal(GATEWAY_HANDLE gw, const char* module_path, const void* module_configuration);
static void gateway_removemodule_internal(GATEWAY_HANDLE gw, MODULE_DATA* module);
static bool module_data_find(const void* element, const void* value);

GATEWAY_HANDLE Gateway_LL_Create(const GATEWAY_PROPERTIES* properties)
{
	/*Codes_SRS_GATEWAY_LL_14_001: [This function shall create a GATEWAY_HANDLE representing the newly created gateway.]*/
	GATEWAY_HANDLE_DATA* gateway = (GATEWAY_HANDLE_DATA*)malloc(sizeof(GATEWAY_HANDLE_DATA));
	if (gateway != NULL) 
	{
		/*Codes_SRS_GATEWAY_LL_14_003: [This function shall create a new MESSAGE_BUS_HANDLE for the gateway representing this gateway's message bus. ]*/
		gateway->bus = MessageBus_Create();
		if (gateway->bus == NULL) 
		{
			/*Codes_SRS_GATEWAY_LL_14_004: [This function shall return NULL if a MESSAGE_BUS_HANDLE cannot be created.]*/
			free(gateway);
			gateway = NULL;
			LogError("Gateway_LL_Create(): MessageBus_Create() failed.");
		}
		else
		{
			/*Codes_SRS_GATEWAY_LL_14_033: [ The function shall create a vector to store each MODULE_DATA. ]*/
			gateway->modules = VECTOR_create(sizeof(MODULE_DATA));
			if (gateway->modules == NULL)
			{
				/*Codes_SRS_GATEWAY_LL_14_034: [ This function shall return NULL if a VECTOR_HANDLE cannot be created. ]*/
				/*Codes_SRS_GATEWAY_LL_14_035: [ This function shall destroy the previously created MESSAGE_BUS_HANDLE and free the GATEWAY_HANDLE if the VECTOR_HANDLE cannot be created. ]*/
				MessageBus_Destroy(gateway->bus);
				free(gateway);
				gateway = NULL;
				LogError("Gateway_LL_Create(): VECTOR_create failed.");
			}
			else
			{
				if (properties != NULL && properties->gateway_properties_entries != NULL)
				{
					/*Codes_SRS_GATEWAY_LL_14_009: [The function shall use each GATEWAY_PROPERTIES_ENTRY use each of GATEWAY_PROPERTIES's gateway_properties_entries to create and add a module to the GATEWAY_HANDLE message bus. ]*/
					size_t entries_count = VECTOR_size(properties->gateway_properties_entries);
					if (entries_count > 0)
					{
						//Add the first module, if successfull add others
						GATEWAY_PROPERTIES_ENTRY* entry = (GATEWAY_PROPERTIES_ENTRY*)VECTOR_element(properties->gateway_properties_entries, 0);
						MODULE_HANDLE module = gateway_addmodule_internal(gateway, entry->module_path, entry->module_configuration);

						//Continue adding modules until all are added or one fails
						for (size_t properties_index = 1; properties_index < entries_count && module != NULL; ++properties_index)
						{
							entry = (GATEWAY_PROPERTIES_ENTRY*)VECTOR_element(properties->gateway_properties_entries, properties_index);
							module = gateway_addmodule_internal(gateway, entry->module_path, entry->module_configuration);
						}

						/*Codes_SRS_GATEWAY_LL_14_036: [ If any MODULE_HANDLE is unable to be created from a GATEWAY_PROPERTIES_ENTRY the GATEWAY_HANDLE will be destroyed. ]*/
						if (module == NULL)
						{
							LogError("Gateway_LL_Create(): Unable to add module '%s'. The gateway will be destroyed.", entry->module_name);
							while (gateway->modules != NULL && VECTOR_size(gateway->modules) > 0)
							{
								MODULE_DATA* module_data = (MODULE_DATA*)VECTOR_front(gateway->modules);
								//By design, there will be no NULL module_data pointers in the vector
								gateway_removemodule_internal(gateway, module_data);
							}
							VECTOR_destroy(gateway->modules);
							MessageBus_Destroy(gateway->bus);
							free(gateway);
							gateway = NULL;
						}
					}
				}
			}
		}
	}
	/*Codes_SRS_GATEWAY_LL_14_002: [This function shall return NULL upon any memory allocation failure.]*/
	else 
	{
		LogError("Gateway_LL_Create(): malloc failed.");
	}

	return gateway;
}

void Gateway_LL_Destroy(GATEWAY_HANDLE gw)
{
	/*Codes_SRS_GATEWAY_LL_14_005: [If gw is NULL the function shall do nothing.]*/
	if (gw != NULL)
	{
		GATEWAY_HANDLE_DATA* gateway_handle = (GATEWAY_HANDLE_DATA*)gw;

		/*Codes_SRS_GATEWAY_LL_14_028: [The function shall remove each module in GATEWAY_HANDLE_DATA's modules vector and destroy GATEWAY_HANDLE_DATA's modules.]*/
		while (gateway_handle->modules != NULL && VECTOR_size(gateway_handle->modules) > 0)
		{
			MODULE_DATA* module_data = (MODULE_DATA*)VECTOR_front(gateway_handle->modules);
			//By design, there will be no NULL module_data pointers in the vector
			/*Codes_SRS_GATEWAY_LL_14_037: [If GATEWAY_HANDLE_DATA's message bus cannot unlink module, the function shall log the error and continue unloading the module from the GATEWAY_HANDLE. ]*/
			gateway_removemodule_internal(gateway_handle, module_data);
		}

		VECTOR_destroy(gateway_handle->modules);

		/*Codes_SRS_GATEWAY_LL_14_006: [The function shall destroy the GATEWAY_HANDLE_DATA's `bus` `MESSAGE_BUS_HANDLE`. ]*/
		MessageBus_Destroy(gateway_handle->bus);

		free(gateway_handle);
	}
	else
	{
		LogError("Gateway_LL_Destroy(): The GATEWAY_HANDLE is null.");
	}
}

MODULE_HANDLE Gateway_LL_AddModule(GATEWAY_HANDLE gw, const GATEWAY_PROPERTIES_ENTRY* entry)
{
	MODULE_HANDLE module;
	/*Codes_SRS_GATEWAY_LL_14_011: [ If gw, entry, or GATEWAY_PROPERTIES_ENTRY's module_path is NULL the function shall return NULL. ]*/
	if (gw != NULL && entry != NULL)
	{
		module = gateway_addmodule_internal(gw, entry->module_path, entry->module_configuration);

		if (module == NULL)
		{
			LogError("Gateway_LL_AddModule(): Unable to add module '%s'.", entry->module_name);
		}
	}
	else
	{
		module = NULL;
		LogError("Gateway_LL_AddModule(): Unable to add module to NULL GATEWAY_HANDLE or from NULL GATEWAY_PROPERTIES_ENTRY*. gw = %p, entry = %p.", gw, entry);
	}

	return module;
}

void Gateway_LL_RemoveModule(GATEWAY_HANDLE gw, MODULE_HANDLE module)
{
	/*Codes_SRS_GATEWAY_LL_14_020: [ If gw or module is NULL the function shall return. ]*/
	if (gw != NULL)
	{
		GATEWAY_HANDLE_DATA* gateway_handle = (GATEWAY_HANDLE_DATA*)gw;

		/*Codes_SRS_GATEWAY_LL_14_023: [The function shall locate the MODULE_DATA object in GATEWAY_HANDLE_DATA's modules containing module and return if it cannot be found. ]*/
		MODULE_DATA* module_data = (MODULE_DATA*)VECTOR_find_if(gateway_handle->modules, module_data_find, module);

		if (module_data != NULL)
		{
			gateway_removemodule_internal(gateway_handle, module_data);
		}
		else
		{
			LogError("Gateway_LL_RemoveModule(): Failed to remove module because the MODULE_DATA pointer is NULL.");
		}
	}
	else
	{
		LogError("Gateway_LL_RemoveModule(): Failed to remove module because the GATEWA_HANDLE is NULL.");
	}
}

/*Private*/

static MODULE_HANDLE gateway_addmodule_internal(GATEWAY_HANDLE_DATA* gateway_handle, const char* module_path, const void* module_configuration)
{
	MODULE_HANDLE module_result;
	if (module_path != NULL)
	{
		/*Codes_SRS_GATEWAY_LL_14_012: [The function shall load the module located at GATEWAY_PROPERTIES_ENTRY's module_path into a MODULE_LIBRARY_HANDLE. ]*/
		MODULE_LIBRARY_HANDLE module_library_handle = ModuleLoader_Load(module_path);
		/*Codes_SRS_GATEWAY_LL_14_031: [If unsuccessful, the function shall return NULL.]*/
		if (module_library_handle == NULL)
		{
			module_result = NULL;
			LogError("Failed to add module because the module located at [%s] could not be loaded.", module_path);
		}
		else
		{
			//Should always be a safe call.
			/*Codes_SRS_GATEWAY_LL_14_013: [The function shall get the const MODULE_APIS* from the MODULE_LIBRARY_HANDLE.]*/
			const MODULE_APIS* module_apis = ModuleLoader_GetModuleAPIs(module_library_handle);

			/*Codes_SRS_GATEWAY_LL_14_015: [The function shall use the MODULE_APIS to create a MODULE_HANDLE using the GATEWAY_PROPERTIES_ENTRY's module_configuration. ]*/
			MODULE_HANDLE module_handle = module_apis->Module_Create(gateway_handle->bus, module_configuration);
			/*Codes_SRS_GATEWAY_LL_14_016: [If the module creation is unsuccessful, the function shall return NULL.]*/
			if (module_handle == NULL)
			{
				module_result = NULL;
				ModuleLoader_Unload(module_library_handle);
				LogError("Module_Create failed.");
			}
			else
			{
				/*Codes_SRS_GATEWAY_LL_99_011: [The function shall assign `module_apis` to `MODULE::module_apis`. ]*/
				MODULE module;
#ifdef UWP_BINDING
				module.module_instance = NULL;
#else
				module.module_apis = module_apis;
				module.module_handle = module_handle;
#endif // UWP_BINDING

				/*Codes_SRS_GATEWAY_LL_14_017: [The function shall link the module to the GATEWAY_HANDLE_DATA's bus using a call to MessageBus_AddModule. ]*/
				/*Codes_SRS_GATEWAY_LL_14_018: [If the message bus linking is unsuccessful, the function shall return NULL.]*/
				if (MessageBus_AddModule(gateway_handle->bus, &module) != MESSAGE_BUS_OK)
				{
					module_result = NULL;
					LogError("Failed to add module to the gateway bus.");
				}
				else
				{
					/*Codes_SRS_GATEWAY_LL_14_039: [ The function shall increment the MESSAGE_BUS_HANDLE reference count if the MODULE_HANDLE was successfully linked to the GATEWAY_HANDLE_DATA's bus. ]*/
					MessageBus_IncRef(gateway_handle->bus);
					/*Codes_SRS_GATEWAY_LL_14_029: [The function shall create a new MODULE_DATA containting the MODULE_HANDLE and MODULE_LIBRARY_HANDLE if the module was successfully linked to the message bus.]*/
					MODULE_DATA module_data =
					{
						module_library_handle,
						module_handle
					};
					/*Codes_SRS_GATEWAY_LL_14_032: [The function shall add the new MODULE_DATA to GATEWAY_HANDLE_DATA's modules if the module was successfully linked to the message bus. ]*/
					if (VECTOR_push_back(gateway_handle->modules, &module_data, 1) != 0)
					{
						MessageBus_DecRef(gateway_handle->bus);
						module_result = NULL;
						if (MessageBus_RemoveModule(gateway_handle->bus, &module) != MESSAGE_BUS_OK)
						{
							LogError("Failed to remove module [%p] from the gateway message bus. This module will remain linked.", &module);
						}
						LogError("Unable to add MODULE_DATA* to the gateway module vector.");
					}
					else
					{
						/*Codes_SRS_GATEWAY_LL_14_019: [The function shall return the newly created MODULE_HANDLE only if each API call returns successfully.]*/
						module_result = module_handle;
					}
				}

				/*Codes_SRS_GATEWAY_LL_14_030: [If any internal API call is unsuccessful after a module is created, the library will be unloaded and the module destroyed.]*/
				if (module_result == NULL)
				{
					module_apis->Module_Destroy(module_handle);
					ModuleLoader_Unload(module_library_handle);
				}
			}
		}
	}
	/*Codes_SRS_GATEWAY_LL_14_011: [If gw, entry, or GATEWAY_PROPERTIES_ENTRY's module_path is NULL the function shall return NULL. ]*/
	else
	{
		module_result = NULL;
		LogError("Failed to add module because either the GATEWAY_HANDLE is NULL or the module_path string is NULL or empty. gw = %p, module_path = '%s'.", gateway_handle, module_path);
	}
	return module_result;
}

static bool module_data_find(const void* element, const void* value)
{
	return ((MODULE_DATA*)element)->module == value;
}

static void gateway_removemodule_internal(GATEWAY_HANDLE_DATA* gateway_handle, MODULE_DATA* module_data)
{
	MODULE module;
#ifdef UWP_BINDING
	module.module_instance = NULL;
#else
	module.module_apis = NULL;
	module.module_handle = module_data->module;
#endif // UWP_BINDING

	/*Codes_SRS_GATEWAY_LL_14_021: [ The function shall unlink module from the GATEWAY_HANDLE_DATA's bus MESSAGE_BUS_HANDLE. ]*/
	/*Codes_SRS_GATEWAY_LL_14_022: [ If GATEWAY_HANDLE_DATA's bus cannot unlink module, the function shall log the error and continue unloading the module from the GATEWAY_HANDLE. ]*/
	if (MessageBus_RemoveModule(gateway_handle->bus, &module) != MESSAGE_BUS_OK)
	{
		LogError("Failed to remove module [%p] from the message bus. This module will remain linked to the message bus but will be removed from the gateway.", module_data->module);
	}
	/*Codes_SRS_GATEWAY_LL_14_038: [ The function shall decrement the MESSAGE_BUS_HANDLE reference count. ]*/
	MessageBus_DecRef(gateway_handle->bus);
	/*Codes_SRS_GATEWAY_LL_14_024: [ The function shall use the MODULE_DATA's module_library_handle to retrieve the MODULE_APIS and destroy module. ]*/
	ModuleLoader_GetModuleAPIs(module_data->module_library_handle)->Module_Destroy(module_data->module);
	/*Codes_SRS_GATEWAY_LL_14_025: [The function shall unload MODULE_DATA's module_library_handle. ]*/
	ModuleLoader_Unload(module_data->module_library_handle);
	/*Codes_SRS_GATEWAY_LL_14_026:[The function shall remove that MODULE_DATA from GATEWAY_HANDLE_DATA's modules. ]*/
	VECTOR_erase(gateway_handle->modules, module_data, 1);
}

#ifdef UWP_BINDING

GATEWAY_HANDLE Gateway_LL_UwpCreate(const VECTOR_HANDLE modules, MESSAGE_BUS_HANDLE bus)
{
	/*Codes_SRS_GATEWAY_LL_99_001: [This function shall create a `GATEWAY_HANDLE` representing the newly created gateway.]*/
	GATEWAY_HANDLE_DATA* gateway = (GATEWAY_HANDLE_DATA*)malloc(sizeof(GATEWAY_HANDLE_DATA));
	if (gateway != NULL)
	{
		gateway->bus = bus;
		if (gateway->bus == NULL)
		{
			/*Codes_SRS_GATEWAY_LL_99_003: [This function shall return `NULL` if a `NULL` `MESSAGE_BUS_HANDLE` is received.]*/
			free(gateway);
			gateway = NULL;
			LogError("Gateway_LL_UwpCreate(): bus must be non-NULL.");
		}
		else
		{
			gateway->modules = modules;
			if (gateway->modules == NULL)
			{
				/*Codes_SRS_GATEWAY_LL_99_004: [This function shall return `NULL` if a `NULL` `VECTOR_HANDLE` is received.]*/
				free(gateway);
				gateway = NULL;
				LogError("Gateway_LL_UwpCreate(): modules must be non-NULL.");
			}
			else
			{
				size_t entries_count = VECTOR_size(gateway->modules);
				//Continue adding modules until all are added or one fails
				for (size_t index = 0; index < entries_count; ++index)
				{
					MODULE* module = (MODULE*)VECTOR_element(gateway->modules, index);
					auto result = MessageBus_AddModule(gateway->bus, module);
					if (result != MESSAGE_BUS_OK)
					{
						free(gateway);
						gateway = NULL;
						LogError("Failed to add module to the gateway bus.");
					}
					else
					{
						/*Codes_SRS_GATEWAY_LL_99_005: [ The function shall increment the MESSAGE_BUS_HANDLE reference count if the MODULE_HANDLE was successfully linked to the GATEWAY_HANDLE_DATA's bus. ]*/
						MessageBus_IncRef(gateway->bus);
					}
				}
			}
		}
	}
	/*Codes_SRS_GATEWAY_LL_99_002: [This function shall return `NULL` upon any memory allocation failure.]*/
	else
	{
		LogError("Gateway_LL_Create(): malloc failed.");
	}

	return gateway;
}

void Gateway_LL_UwpDestroy(GATEWAY_HANDLE gw)
{
	/*Codes_SRS_GATEWAY_LL_99_006: [If gw is NULL the function shall do nothing.]*/
	if (gw != NULL)
	{
		GATEWAY_HANDLE_DATA* gateway_handle = (GATEWAY_HANDLE_DATA*)gw;

		size_t entries_count = VECTOR_size(gateway_handle->modules);
		//Iterate through gateway modules and remove
		for (size_t index = 0; index < entries_count; ++index)
		{
			MODULE* module = (MODULE*)VECTOR_element(gateway_handle->modules, index);

			//By design, there will be no NULL module_data pointers in the vector
			/*Codes_SRS_GATEWAY_LL_99_007: [ The function shall unlink module from the GATEWAY_HANDLE_DATA's bus MESSAGE_BUS_HANDLE. ]*/
			/*Codes_SRS_GATEWAY_LL_99_008: [ If GATEWAY_HANDLE_DATA's bus cannot unlink module, the function shall log the error and continue unloading the module from the GATEWAY_HANDLE. ]*/
			if (MessageBus_RemoveModule(gateway_handle->bus, module) != MESSAGE_BUS_OK)
			{
				LogError("Failed to remove module [%p] from the message bus. This module will remain linked to the message bus but will be removed from the gateway.", module);
			}
			/*Codes_SRS_GATEWAY_LL_99_009: [ The function shall decrement the MESSAGE_BUS_HANDLE reference count. ]*/
			MessageBus_DecRef(gateway_handle->bus);
		}

		/*Codes_SRS_GATEWAY_LL_99_010: [The function shall destroy the GATEWAY_HANDLE_DATA's `bus` `MESSAGE_BUS_HANDLE`. ]*/
		MessageBus_Destroy(gateway_handle->bus);

		free(gateway_handle);
	}
	else
	{
		LogError("Gateway_LL_UwpDestroy(): The GATEWAY_HANDLE is null.");
	}
}

#endif // UWP_BINDING

