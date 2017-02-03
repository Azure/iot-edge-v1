// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <new>
#include <memory>

#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/xlogging.h"
#include "modules_manager.h"

using namespace dotnetcore_module;


ModulesManager* ModulesManager::m_instance = nullptr;

LOCK_HANDLE ModulesManager::m_lock_handle = nullptr;

ModulesManager::ModulesManager() :
     m_moduleid_counter(0)
{}

ModulesManager::~ModulesManager()
{

}


ModulesManager* ModulesManager::Get()
{
    if (m_lock_handle == nullptr)
    {
        /* Codes_ SRS_DOTNET_CORE_MODULES_MGR_04_001: [ This method shall acquire the lock on a static lock object. ]*/
        m_lock_handle = Lock_Init();
    }

	if (m_lock_handle == nullptr)
	{ 
		/* Codes_SRS_DOTNET_CORE_MODULES_MGR_04_002: [ This method shall return NULL if an underlying API call fails. ] */
		LogError("Could not instantiate LOCK_HANDLE");
	}
	else if (m_instance == nullptr)
	{
        /* Codes_ SRS_DOTNET_CORE_MODULES_MGR_04_001: [ This method shall acquire the lock on a static lock object. ]*/
		if (::Lock(m_lock_handle) == LOCK_OK)
		{
			if (m_instance == nullptr)
			{
				try
				{
                    /* Codes_SRS_DOTNET_CORE_MODULES_MGR_04_003: [ This method shall create an instance of ModulesManager if this is the first call. ] */
                    m_instance = new ModulesManager();
				}
				catch (std::bad_alloc& err)
				{
					LogError("new operator failed with %s", err.what());
				}
			}

            /* Codes_SRS_DOTNET_CORE_MODULES_MGR_04_016: [ This method shall release the lock on the static lock object before returning. ] */
			if (::Unlock(m_lock_handle) != LOCK_OK)
			{
				LogError("Could not unlock LOCK_HANDLE");
				delete m_instance;
                m_instance = nullptr;
			}
		}
		else
		{
            LogError("Could not lock LOCK_HANDLE");
            if (Lock_Deinit(m_lock_handle) == LOCK_OK)
            {
                m_lock_handle = nullptr;
            }
            else
            {
                LogError("Could not Deinit LOCK_HANDLE");
            }           			
		}
	}

    /* Codes_SRS_DOTNET_CORE_MODULES_MGR_04_004: [ This method shall return a non-NULL pointer to a ModulesManager instance when the object has been successfully insantiated. ] */
	return m_instance;
}

void ModulesManager::AcquireLock() const
{
    /* Codes_SRS_DOTNET_CORE_MODULES_MGR_04_017: [ AcquireLock shall acquire a lock by calling AcquireLock on Lock object. ] */
    m_lock.AcquireLock();
}

void ModulesManager::ReleaseLock() const
{
    /* Codes_SRS_DOTNET_CORE_MODULES_MGR_04_018: [ ReleaseLock shall release the lock by calling ReleaseLock on Lock object. ] */
    m_lock.ReleaseLock();
}

bool ModulesManager::HasModule(size_t module_id) const
{
    /* Codes_SRS_DOTNET_CORE_MODULES_MGR_04_019: [ HasModule shall make sure it's thread safe and call lock_guard. ] */
    LockGuard<ModulesManager> lock_guard(*this);
    /* Codes_SRS_DOTNET_CORE_MODULES_MGR_04_020: [ HasModule shall call find passing module id and return result. ] */
    return m_modules.find(module_id) != m_modules.end();
}

DOTNET_CORE_HOST_HANDLE_DATA& ModulesManager::GetModuleFromId(size_t module_id)
{
    /* Codes_SRS_DOTNET_CORE_MODULES_MGR_04_021: [ GetModuleFromId shall make sure it's thread safe and call lock_guard. ] */
    LockGuard<ModulesManager> lock_guard(*this);
    /* Codes_SRS_DOTNET_CORE_MODULES_MGR_04_022: [ GetModuleFromId shall return the instance based on module_id. ] */
    return m_modules[module_id];
}

DOTNET_CORE_HOST_HANDLE_DATA* ModulesManager::AddModule(const DOTNET_CORE_HOST_HANDLE_DATA& handle_data)
{
    DOTNET_CORE_HOST_HANDLE_DATA* result;
    
    /* Codes_SRS_DOTNET_CORE_MODULES_MGR_04_005: [ AddModule shall acquire a lock on the ModulesManager object. ] */
    /* Codes_SRS_DOTNET_CORE_MODULES_MGR_04_006: [ AddModule shall return NULL if an underlying API call fails. ] */
    LockGuard<ModulesManager> lock_guard(*this);
    auto module_id = m_moduleid_counter++;
    try
    {
        /* Codes_SRS_DOTNET_CORE_MODULES_MGR_04_007: [ AddModule shall add the module to it's internal collection of modules. ] */
        m_modules.insert(
            std::make_pair(
                module_id,
                DOTNET_CORE_HOST_HANDLE_DATA(handle_data, module_id)
            )
        );

        result = &(m_modules[module_id]);
    }
    catch (std::bad_alloc& err)
    {
        /* Codes_SRS_DOTNET_CORE_MODULES_MGR_04_006: [ AddModule shall return NULL if an underlying API call fails. ] */
        LogError("Memory allocation error occurred with %s", err.what());
        result = nullptr;
    }

    return result;
    /* Codes_SRS_DOTNET_CORE_MODULES_MGR_04_008: [ AddModule shall release the lock on the ModulesManager object. ] */
    // destructor of LockGuard releases the lock
}

void ModulesManager::RemoveModule(size_t module_id)
{
    /* Codes_SRS_DOTNET_CORE_MODULES_MGR_04_012: [ RemoveModule shall acquire a lock on the ModulesManager object. ] */
    LockGuard<ModulesManager> lock_guard(*this);
    auto entry = m_modules.find(module_id);
    if (entry != m_modules.end())
    {
        /* Codes_SRS_DOTNET_CORE_MODULES_MGR_04_014: [ RemoveModule shall remove the module from it's internal collection of modules. ] */
        m_modules.erase(entry);
    }
    /* Codes_SRS_DOTNET_CORE_MODULES_MGR_04_013: [ RemoveModule shall do nothing if an underlying API call fails. ] */
    /* Codes_SRS_DOTNET_CORE_MODULES_MGR_04_015: [ RemoveModule shall release the lock on the ModulesManager object. ] */
    // destructor of LockGuard releases the lock
}