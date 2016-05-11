// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <vector>
#include <new>

#include "uv.h"
#include "v8.h"
#include "node.h"

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/lock.h"

#include "azure_c_shared_utility/iot_logging.h"

#include "lock.h"
#include "nodejs_common.h"
#include "modules_manager.h"

using namespace nodejs_module;

ModulesManager::ModulesManager() :
    m_nodejs_thread{ NULL },
    m_moduleid_counter{ 0 }
{}

ModulesManager::~ModulesManager()
{
    // We don't do anything here because, well, this instance is expected to
    // run pretty much indefinitely. The only clean-up we could potentially do
    // here is to end the thread that Node JS is running on. But since Node
    // isn't really amenable to a clean exit the only alternative left is a
    // process exit at which point it really doesn't matter if we exit this
    // thread or not.
}

ModulesManager* ModulesManager::Get()
{
    static ModulesManager* instance = nullptr;
    static LOCK_HANDLE lock = Lock_Init();

    if (lock == NULL)
    {
        /*Codes_SRS_NODEJS_MODULES_MGR_13_002: [ This method shall return NULL if an underlying API call fails. ]*/
        LogError("Could not instantiate LOCK_HANDLE");
        // 'instance' is already NULL
    }
    else if (instance == nullptr)
    {
        /*Codes_SRS_NODEJS_MODULES_MGR_13_001: [ This method shall acquire the lock on a static lock object. ]*/
        if (::Lock(lock) == LOCK_OK)
        {
            if (instance == nullptr)
            {
                try
                {
                    /*Codes_SRS_NODEJS_MODULES_MGR_13_003: [ This method shall create an instance of ModulesManager if this is the first call. ]*/
                    /*Codes_SRS_NODEJS_MODULES_MGR_13_004: [ This method shall return a non-NULL pointer to a ModulesManager instance when the object has been successfully insantiated. ]*/
                    instance = new ModulesManager();
                }
                catch (std::bad_alloc& err)
                {
                    /*Codes_SRS_NODEJS_MODULES_MGR_13_002: [ This method shall return NULL if an underlying API call fails. ]*/
                    LogError("new operator failed with %s", err.what());
                    instance = nullptr;
                }
            }
            if (::Unlock(lock) != LOCK_OK)
            {
                /*Codes_SRS_NODEJS_MODULES_MGR_13_002: [ This method shall return NULL if an underlying API call fails. ]*/
                LogError("Could not unlock LOCK_HANDLE");
                delete instance;
                instance = NULL;
            }
        }
        else
        {
            /*Codes_SRS_NODEJS_MODULES_MGR_13_002: [ This method shall return NULL if an underlying API call fails. ]*/
            LogError("Could not lock LOCK_HANDLE");
            // 'instance' is already NULL
        }
    }

    return instance;
}

LOCK_RESULT ModulesManager::Lock() const
{
    return m_lock.Acquire();
}

LOCK_RESULT ModulesManager::Unlock() const
{
    return m_lock.Release();
}

bool ModulesManager::HasModule(size_t module_id) const
{
    return m_modules.find(module_id) != m_modules.end();
}

NODEJS_MODULE_HANDLE_DATA& ModulesManager::GetModuleFromId(size_t module_id)
{
    return m_modules[module_id];
}

NODEJS_MODULE_HANDLE_DATA* ModulesManager::AddModule(const NODEJS_MODULE_HANDLE_DATA& handle_data)
{
    NODEJS_MODULE_HANDLE_DATA* result;

    /*Codes_SRS_NODEJS_MODULES_MGR_13_005: [ AddModule shall acquire a lock on the ModulesManager object. ]*/
    LockGuard<ModulesManager> lock_guard{ *this };

    if (lock_guard.GetResult() == LOCK_OK)
    {
        // start NodeJS if not already started
        if (m_nodejs_thread == NULL)
        {
            /*Codes_SRS_NODEJS_MODULES_MGR_13_007: [ If this is the first time that this method is being called then it shall start a new thread and start up Node JS from the thread. ]*/
            StartNode();
        }

        // if m_nodejs_thread is still NULL then StartNode failed
        if (m_nodejs_thread != NULL)
        {
            auto module_id = m_moduleid_counter++;
            try
            {
                /*Codes_SRS_NODEJS_MODULES_MGR_13_008: [ AddModule shall add the module to it's internal collection of modules. ]*/
                m_modules.insert(
                    std::make_pair(
                        module_id,
                        NODEJS_MODULE_HANDLE_DATA{ handle_data, module_id }
                    )
                );

                // start up the module
                if (StartModule(module_id) == false)
                {
                    LogError("Could not start Node JS module from path '%s'", handle_data.main_path.c_str());
                    result = &(m_modules[module_id]);
                }
                else
                {
                    /*Codes_SRS_NODEJS_MODULES_MGR_13_006: [ AddModule shall return NULL if an underlying API call fails. ]*/
                    // remove the module from the map
                    m_modules.erase(module_id);
                    result = NULL;
                }
            }
            catch (std::bad_alloc& err)
            {
                /*Codes_SRS_NODEJS_MODULES_MGR_13_006: [ AddModule shall return NULL if an underlying API call fails. ]*/
                LogError("Memory allocation error occurred with %s", err.what());
                result = NULL;
            }
        }
        else
        {
            /*Codes_SRS_NODEJS_MODULES_MGR_13_006: [ AddModule shall return NULL if an underlying API call fails. ]*/
            LogError("Could not start Node JS thread.");
            result = NULL;
        }
    }
    else
    {
        /*Codes_SRS_NODEJS_MODULES_MGR_13_006: [ AddModule shall return NULL if an underlying API call fails. ]*/
        LogError("LockGuard failed");
        result = NULL;
    }

    return result;

    /*Codes_SRS_NODEJS_MODULES_MGR_13_011: [ AddModule shall release the lock on the ModulesManager object. ]*/
    // destructor of LockGuard releases the lock
}

void ModulesManager::RemoveModule(size_t module_id)
{
    /*Codes_SRS_NODEJS_MODULES_MGR_13_012: [ RemoveModule shall acquire a lock on the ModulesManager object. ]*/
    LockGuard<ModulesManager> lock_guard{ *this };
    if (lock_guard.GetResult() == LOCK_OK)
    {
        auto entry = m_modules.find(module_id);
        if (entry != m_modules.end())
        {
            /*Codes_SRS_NODEJS_MODULES_MGR_13_014: [ RemoveModule shall remove the module from it's internal collection of modules. ]*/
            m_modules.erase(entry);
        }
    }
    else
    {
        /*Codes_SRS_NODEJS_MODULES_MGR_13_013: [ RemoveModule shall do nothing if an underlying API call fails. ]*/
        LogError("LockGuard failed");
    }

    /*Codes_SRS_NODEJS_MODULES_MGR_13_015: [ RemoveModule shall release the lock on the ModulesManager object. ]*/
    // destructor of LockGuard releases the lock
}

int ModulesManager::NodeJSRunner()
{
    // start up Node.js!
    int argc = 1;
    char *argv[] = { "node" };
    return node::Start(argc, argv);
}

int ModulesManager::NodeJSRunnerInternal(void* user_data)
{
    return reinterpret_cast<ModulesManager*>(user_data)->NodeJSRunner();
}

THREADAPI_RESULT ModulesManager::StartNode()
{
    // The caller of this method would have already acquired an
    // object lock.
    auto result = ThreadAPI_Create(
        &m_nodejs_thread,
        ModulesManager::NodeJSRunnerInternal,
        reinterpret_cast<void*>(this)
    );
    if (result != THREADAPI_OK)
    {
        LogError("ThreadAPI_Create failed");
        m_nodejs_thread = NULL;
    }

    return result;
}

void ModulesManager::OnStartModuleInternal(uv_idle_t* handle)
{
    size_t module_id = reinterpret_cast<size_t>(
        reinterpret_cast<uv_handle_t*>(handle)->data
    );
    ModulesManager::Get()->OnStartModule(module_id, handle);
}

void ModulesManager::OnStartModule(size_t module_id, uv_idle_t* handle)
{
    auto& handle_data = m_modules[module_id];

    // save the v8 isolate in the handle's data
    handle_data.v8_isolate = v8::Isolate::GetCurrent();

    /*Codes_SRS_NODEJS_MODULES_MGR_13_010: [ When the callback is invoked via libuv, it shall invoke the NODEJS_MODULE_HANDLE_DATA::on_module_start function. ]*/
    handle_data.on_module_start(&handle_data);
    uv_idle_stop(handle);
}

bool ModulesManager::StartModule(size_t module_id)
{
    bool result;

    uv_loop_t* default_loop = uv_default_loop();
    if (default_loop == NULL)
    {
        LogError("uv_default_loop() failed");
        result = false;
    }
    else
    {
        uv_idle_t idler;
        if (uv_idle_init(default_loop, &idler) != 0)
        {
            LogError("uv_idle_init failed");
            result = false;
        }
        else
        {
            // save the handle data so we can later use this from the callback
            reinterpret_cast<uv_handle_t*>(&idler)->data = reinterpret_cast<void*>(module_id);

            /*Codes_SRS_NODEJS_MODULES_MGR_13_009: [ AddModule shall schedule a callback to be invoked on the Node engine's event loop via libuv. ]*/
            if (uv_idle_start(&idler, ModulesManager::OnStartModuleInternal) != 0)
            {
                LogError("uv_idle_start failed");
                uv_idle_stop(&idler);
                result = false;
            }
            else
            {
                result = true;
            }
        }
    }

    return result;
}
