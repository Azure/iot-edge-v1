// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef __CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/gballoc.h"
#include "java_module_host_manager.h"

static JAVA_MODULE_HOST_MANAGER_HANDLE instance = NULL;

typedef struct JAVA_MODULE_HOST_MANAGER_DATA_TAG {
	int module_count;
	JAVA_MODULE_HOST_CONFIG* config;
	LOCK_HANDLE lock;
} JAVA_MODULE_HOST_MANAGER_HANDLE_DATA;

static JAVA_MODULE_HOST_MANAGER_RESULT internal_inc_dec(JAVA_MODULE_HOST_MANAGER_HANDLE manager, int addend);
static int config_options_compare(JAVA_MODULE_HOST_CONFIG* config1, JAVA_MODULE_HOST_CONFIG* config2);
static JAVA_MODULE_HOST_CONFIG* config_copy(JAVA_MODULE_HOST_CONFIG* config);
static void config_destroy(JAVA_MODULE_HOST_CONFIG* config);
static JVM_OPTIONS* options_copy(JVM_OPTIONS* options);
static int VECTOR_compare(VECTOR_HANDLE vector1, VECTOR_HANDLE vector2);
static VECTOR_HANDLE VECTOR_copy(VECTOR_HANDLE vector);

JAVA_MODULE_HOST_MANAGER_HANDLE JavaModuleHostManager_Create(JAVA_MODULE_HOST_CONFIG* config)
{
	JAVA_MODULE_HOST_MANAGER_HANDLE_DATA* result;

	if (instance == NULL)
	{
		/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_002: [ The function shall allocate memory for the JAVA_MODULE_HOST_MANAGER_HANDLE if the global instance is NULL. ]*/
		result = (JAVA_MODULE_HOST_MANAGER_HANDLE_DATA*)malloc(sizeof(JAVA_MODULE_HOST_MANAGER_HANDLE_DATA));
		if (result == NULL)
		{
			/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_003: [ The function shall return NULL if memory allocation fails. ]*/
			LogError("Failed to allocate memory for a JAVA_MODULE_HOST_MANAGER_HANDLE");
		}
		else
		{
			/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_004: [ The function shall initialize a new LOCK_HANDLE and set the JAVA_MODULE_HOST_MANAGER_HANDLE structures LOCK_HANDLE member. ]*/
			if ((result->lock = Lock_Init()) == NULL)
			{
				/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_005: [ The function shall return NULL if lock initialization fails. ]*/
				LogError("Lock_Init() failed.");
				free(result);
				result = NULL;
			}
			else
			{
				/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_028: [The function shall do a deep copy of the config parameter and keep a pointer to the copied JAVA_MODULE_HOST_CONFIG.]*/
				result->config = config_copy(config);
				if (result->config == NULL)
				{
					/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_029: [If the deep copy fails, the function shall return NULL.]*/
					LogError("Failed to copy the JAVA_MODULE_HOST_CONFIG structure.");
					Lock_Deinit(result->lock);
					free(result);
					result = NULL;
				}
				else
				{
					/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_006: [ The function shall set the module_count member variable to 0. ]*/
					/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_007: [ The function shall shall set the global JAVA_MODULE_HOST_MANAGER_HANDLE instance. ]*/
					result->module_count = 0;
					instance = result;
				}
			}
		}
	}
	else
	{
		/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_030: [The function shall check against the singleton JAVA_MODULE_HOST_MANAGER_HANDLE instances JAVA_MODULE_HOST_CONFIG structure for equality in each field.]*/
		if (config_options_compare(instance->config, config) != 0)
		{
			/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_031: [The function shall return NULL if the JAVA_MODULE_HOST_CONFIG structures do not match.]*/
			LogError("JAVA_MODULE_HOST_CONFIG does not match the JAVA_MODULE_HOST_CONFIG known by this manager.");
			result = NULL;
		}
		else
		{
			/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_001: [ The function shall return the global JAVA_MODULE_HOST_MANAGER_HANDLE instance if it is not NULL. ]*/
			result = instance;
		}
	}

	/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_008: [ The function shall return the global JAVA_MODULE_HOST_MANAGER_HANDLE instance. ]*/
	return result;
}

void JavaModuleHostManager_Destroy(JAVA_MODULE_HOST_MANAGER_HANDLE manager)
{
	if (manager != NULL && manager != instance)
	{
		/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_009: [ The function shall do nothing if handle is NULL. ]*/
		LogError("[FATAL]: JAVA_MODULE_HOST_MANAGER_HANDLE is invalid. JAVA_MODULE_HOST_MANAGER_HANDLE = [%p] while instance = [%p].", manager, instance);
	}
	else if(manager != NULL)
	{
		if (manager->module_count == 0)
		{
			/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_011: [ The function shall deinitialize handle->lock. ]*/
			if (Lock_Deinit(manager->lock) != LOCK_OK)
			{
				/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_012: [ The function shall do nothing if lock deinitialization fails. ]*/
				LogError("[FATAL]: Lock_Deinit() failed.");
			}
			else
			{
				/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_013: [ The function shall free the handle parameter. ]*/
				/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_032: [ The function shall destroy the JAVA_MODULE_HOST_CONFIG structure. ]*/
				config_destroy(manager->config);
				free(manager);
				manager = NULL;
				instance = NULL;
			}
		}
		else
		{
			/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_010: [ The function shall do nothing if handle->module_count is not 0. ]*/
			LogInfo("Manager module count is not 0 (%i) and will not be destroyed.", manager->module_count);
		}
	}
}

JAVA_MODULE_HOST_MANAGER_RESULT JavaModuleHostManager_Add(JAVA_MODULE_HOST_MANAGER_HANDLE manager)
{
	return internal_inc_dec(manager, 1);
}

JAVA_MODULE_HOST_MANAGER_RESULT JavaModuleHostManager_Remove(JAVA_MODULE_HOST_MANAGER_HANDLE manager)
{
	return internal_inc_dec(manager, -1);
}

size_t JavaModuleHostManager_Size(JAVA_MODULE_HOST_MANAGER_HANDLE manager)
{
	size_t size;
	if (manager == NULL)
	{
		/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_021: [ If handle is NULL the function shall return -1. ]*/
		size = -1;
	}
	else
	{
		/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_023: [The function shall acquire the handle parameters lock.]*/
		if (Lock(manager->lock) != LOCK_OK)
		{
			/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_024: [If lock acquisition fails, this function shall return -1.]*/
			LogError("Failed to acquire lock.");
			size = -1;
		}
		else
		{
			/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_022: [ The function shall return handle->module_count. ]*/
			size = manager->module_count;

			/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_025: [The function shall release the handle parameters lock.]*/
			if (Unlock(manager->lock) != LOCK_OK)
			{
				/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_026: [If the function fails to release the lock, this function shall report the error.]*/
				LogError("Failed to release lock.");
			}
		}
	}
	return size;
}

static JAVA_MODULE_HOST_MANAGER_RESULT internal_inc_dec(JAVA_MODULE_HOST_MANAGER_HANDLE manager, int addend)
{
	JAVA_MODULE_HOST_MANAGER_RESULT result;

	if (manager == NULL)
	{
		/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_014: [ The function shall return MANAGER_ERROR if handle is NULL. ]*/
		LogError("JAVA_MODULE_HOST_MANAGER_HANDLE is NULL.");
		result = MANAGER_ERROR;
	}
	else
	{
		/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_015: [ The function shall acquire the handle parameters lock. ]*/
		if (Lock(manager->lock) != LOCK_OK)
		{
			/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_016: [ If lock acquisition fails, this function shall return MANAGER_ERROR. ]*/
			LogError("Failed to acquire lock.");
			result = MANAGER_ERROR;
		}
		else
		{
			/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_027: [ The function shall return MANAGER_ERROR if an attempt is made to decrement past 0. ]*/
			if (manager->module_count + addend < 0)
			{
				LogError("Cannot remove from empty manager.");
				result = MANAGER_ERROR;
			}
			else
			{
				/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_017: [ The function shall increment or decrement handle->module_count. ]*/
				manager->module_count += addend;

				/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_020: [ The function shall return MANAGER_OK if all succeeds. ]*/
				result = MANAGER_OK;
			}

			/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_018: [ The function shall release the handle parameters lock. ]*/
			if (Unlock(manager->lock) != LOCK_OK)
			{
				LogError("Failed to release lock.");
			}
		}
	}
	return result;
}

static JAVA_MODULE_HOST_CONFIG* config_copy(JAVA_MODULE_HOST_CONFIG* config)
{
	JAVA_MODULE_HOST_CONFIG* new_config = malloc(sizeof(JAVA_MODULE_HOST_CONFIG));
	if (new_config == NULL)
	{
		LogError("Failed to allocate memory for JAVA_MODULE_HOST_CONFIG structure.");
	}
	else
	{
		//Copy the class name
		int status = mallocAndStrcpy_s((char**)(&new_config->class_name), config->class_name);
		if (status != 0)
		{
			LogError("Failed to allocate class_name.");
			free(new_config);
			new_config = NULL;
		}
		else
		{
			//Copy the configuration
			status = mallocAndStrcpy_s((char**)(&new_config->configuration_json), config->configuration_json);
			if (status != 0)
			{
				LogError("Failed to allocate configuration_json.");
				free((void*)new_config->class_name);
				free(new_config);
				new_config = NULL;
			}
			else
			{
				new_config->options = options_copy(config->options);
				if (new_config->options == NULL)
				{
					free((void*)new_config->class_name);
					free((void*)new_config->configuration_json);
					free(new_config);
					new_config = NULL;
				}
			}
		}
	}
	return new_config;
}

static JVM_OPTIONS* options_copy(JVM_OPTIONS* options)
{
	//Copy the VECTOR_HANDLE
	JVM_OPTIONS* new_options = (JVM_OPTIONS*)malloc(sizeof(JVM_OPTIONS));
	if (new_options == NULL)
	{
		LogError("Failed to allocate memory for JVM_OPTIONS structure.");
	}
	else
	{
		new_options->additional_options = VECTOR_copy(options->additional_options);
		if (new_options->additional_options == NULL)
		{
			LogError("Failed to copy additional_options VECTOR_HANDLE.");
			free(new_options);
			new_options = NULL;
		}
		else
		{
			//Copy the class path
			int status = mallocAndStrcpy_s((char**)(&new_options->class_path), options->class_path);
			if (status != 0)
			{
				LogError("Failed to allocate class_path.");
				VECTOR_destroy(new_options->additional_options);
				free(new_options);
				new_options = NULL;
			}
			else
			{
				//Copy the library path
				status = mallocAndStrcpy_s((char**)(&new_options->library_path), options->library_path);
				if (status != 0)
				{
					LogError("Failed to allocate library_path.");
					free((void*)new_options->class_path);
					VECTOR_destroy(new_options->additional_options);
					free(new_options);
					new_options = NULL;
				}
				else
				{
					new_options->debug = options->debug;
					new_options->debug_port = options->debug_port;
					new_options->verbose = options->verbose;
					new_options->version = options->version;
				}
			}
		}
	}
	return new_options;
}

static void config_destroy(JAVA_MODULE_HOST_CONFIG* config)
{
	free((void*)config->class_name);
	free((void*)config->configuration_json);
	if (config->options != NULL)
	{
		free((void*)config->options->class_path);
		free((void*)config->options->library_path);
		VECTOR_destroy(config->options->additional_options);
		free(config->options);
	}
	free(config);
}

static int config_options_compare(JAVA_MODULE_HOST_CONFIG* config1, JAVA_MODULE_HOST_CONFIG* config2)
{
	int result;

	if (config1 == NULL)
	{
		result = __LINE__;
	}
	else if (config2 == NULL)
	{
		result = __LINE__;
	}
	else if (config1->options == NULL)
	{
		result = __LINE__;
	}
	else if (config2->options == NULL)
	{
		result = __LINE__;
	}
	else if (config1 == config2)
	{
		result = 0;
	}
	else if (config1->options == config2->options)
	{
		result = 0;
	}
	else
	{
		JVM_OPTIONS* options1 = config1->options;
		JVM_OPTIONS* options2 = config2->options;
		
		if (
			strcmp(options1->class_path, options2->class_path) == 0 &&
			strcmp(options1->library_path, options2->library_path) == 0 &&
			options1->version == options2->version &&
			options1->debug == options2->debug &&
			options1->debug_port == options2->debug_port &&
			options1->verbose == options2->verbose &&
			VECTOR_compare(options1->additional_options, options2->additional_options) == 0
			)
		{
			result = 0;
		}
		else
		{
			result = __LINE__;
		}
		
	}

	return result;
}

static VECTOR_HANDLE VECTOR_copy(VECTOR_HANDLE vector)
{
	VECTOR_HANDLE new_vector = VECTOR_create(sizeof(STRING_HANDLE));

	size_t size = VECTOR_size(vector);
	for (size_t index = 0; index < size && new_vector != NULL; index++)
	{
		STRING_HANDLE* str = VECTOR_element(vector, index);
		if (str == NULL)
		{
			VECTOR_destroy(new_vector);
			new_vector = NULL;
		}
		else
		{
			STRING_HANDLE new_str = STRING_clone(*str);
			if (new_str == NULL)
			{
				VECTOR_destroy(new_vector);
				new_vector = NULL;
			}
			else
			{
				if (VECTOR_push_back(new_vector, &new_str, 1) != 0)
				{
					STRING_delete(new_str);
					VECTOR_destroy(new_vector);
					new_vector = NULL;
				}
			}
		}
	}

	return new_vector;
}

static int VECTOR_compare(VECTOR_HANDLE vector1, VECTOR_HANDLE vector2)
{
	int result = 0;
	if (vector1 == NULL)
	{
		result = __LINE__;
	}
	else if (vector2 == NULL)
	{
		result = __LINE__;
	}
	else if(VECTOR_size(vector1) != VECTOR_size(vector2))
	{
		result = __LINE__;
	}
	else
	{
		size_t size = VECTOR_size(vector1);
		for (size_t index = 0; index < size && result == 0; index++)
		{
			STRING_HANDLE* str1 = (STRING_HANDLE*)VECTOR_element(vector1, index);
			STRING_HANDLE* str2 = (STRING_HANDLE*)VECTOR_element(vector2, index);

			result = STRING_compare(*str1, *str2);
		}
	}

	return result;
}