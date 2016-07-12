// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef __CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/iot_logging.h"
#include "azure_c_shared_utility/gballoc.h"
#include "java_module_host_manager.h"

static JAVA_MODULE_HOST_MANAGER_HANDLE instance = NULL;

typedef struct JAVA_MODULE_HOST_MANAGER_DATA_TAG {
	int module_count;
	LOCK_HANDLE lock;
} JAVA_MODULE_HOST_MANAGER_HANDLE_DATA;

static JAVA_MODULE_HOST_MANAGER_RESULT internal_inc_dec(JAVA_MODULE_HOST_MANAGER_HANDLE manager, int addend);

JAVA_MODULE_HOST_MANAGER_HANDLE JavaModuleHostManager_Create()
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
				/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_006: [ The function shall set the module_count member variable to 0. ]*/
				/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_007: [ The function shall shall set the global JAVA_MODULE_HOST_MANAGER_HANDLE instance. ]*/
				result->module_count = 0;
				instance = result;
			}
		}
	}
	else
	{
		/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_001: [ The function shall return the global JAVA_MODULE_HOST_MANAGER_HANDLE instance if it is not NULL. ]*/
		result = instance;
	}

	/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_008: [ The function shall return the global JAVA_MODULE_HOST_MANAGER_HANDLE instance. ]*/
	return result;
}

void JavaModuleHostManager_Destroy(JAVA_MODULE_HOST_MANAGER_HANDLE manager)
{
	if (manager == NULL || manager != instance)
	{
		/*Codes_SRS_JAVA_MODULE_HOST_MANAGER_14_009: [ The function shall do nothing if handle is NULL. ]*/
		LogError("[FATAL]: JAVA_MODULE_HOST_MANAGER_HANDLE is invalid. JAVA_MODULE_HOST_MANAGER_HANDLE = [%p] while instance = [%p].", manager, instance);
	}
	else
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
