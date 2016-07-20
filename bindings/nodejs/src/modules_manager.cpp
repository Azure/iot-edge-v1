// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <vector>
#include <new>
#include <memory>

#include "uv.h"
#include "node.h"

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/lock.h"

#include "azure_c_shared_utility/xlogging.h"

#include "lock.h"
#include "nodejs_common.h"
#include "nodejs_utils.h"
#include "modules_manager.h"
#include "nodejs_idle.h"

using namespace nodejs_module;

#define NODE_STARTUP_SCRIPT  ""                  \
"(function () {"                                 \
"    _gatewayInit.onNodeInitComplete();"         \
"    global._gatewayExit = false;"               \
"    function onTimer() {"                       \
"        if(global._gatewayExit === false) {"    \
"            setTimeout(onTimer, 150);"          \
"        }"                                      \
"    }"                                          \
"    onTimer();"                                 \
"})();"

ModulesManager::ModulesManager() :
    m_nodejs_thread{ nullptr },
    m_moduleid_counter{ 0 },
    m_node_initialized{ false }
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

    if (lock == nullptr)
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
                }
            }

            /*Codes_SRS_NODEJS_MODULES_MGR_13_016: [ This method shall release the lock on the static lock object before returning. ]*/
            if (::Unlock(lock) != LOCK_OK)
            {
                /*Codes_SRS_NODEJS_MODULES_MGR_13_002: [ This method shall return NULL if an underlying API call fails. ]*/
                LogError("Could not unlock LOCK_HANDLE");
                delete instance;
                instance = nullptr;
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

void ModulesManager::AcquireLock() const
{
    m_lock.AcquireLock();
}

void ModulesManager::ReleaseLock() const
{
    m_lock.ReleaseLock();
}

bool ModulesManager::HasModule(size_t module_id) const
{
    LockGuard<ModulesManager> lock_guard{ *this };
    return m_modules.find(module_id) != m_modules.end();
}

NODEJS_MODULE_HANDLE_DATA& ModulesManager::GetModuleFromId(size_t module_id)
{
    LockGuard<ModulesManager> lock_guard{ *this };
    return m_modules[module_id];
}

bool ModulesManager::IsNodeInitialized() const
{
    LockGuard<ModulesManager> lock_guard{ *this };
    return m_node_initialized;
}

NODEJS_MODULE_HANDLE_DATA* ModulesManager::AddModule(const NODEJS_MODULE_HANDLE_DATA& handle_data)
{
    NODEJS_MODULE_HANDLE_DATA* result;

    /*Codes_SRS_NODEJS_MODULES_MGR_13_005: [ AddModule shall acquire a lock on the ModulesManager object. ]*/
    LockGuard<ModulesManager> lock_guard{ *this };

    // start NodeJS if not already started
    if (m_nodejs_thread == nullptr)
    {
        /*Codes_SRS_NODEJS_MODULES_MGR_13_007: [ If this is the first time that this method is being called then it shall start a new thread and start up Node JS from the thread. ]*/
        StartNode();
    }

    // if m_nodejs_thread is still NULL then StartNode failed
    if (m_nodejs_thread != nullptr)
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
            if (StartModule(module_id) == true)
            {
                result = &(m_modules[module_id]);
            }
            else
            {
                /*Codes_SRS_NODEJS_MODULES_MGR_13_006: [ AddModule shall return NULL if an underlying API call fails. ]*/
                // remove the module from the map
                LogError("Could not start Node JS module from path '%s'", handle_data.main_path.c_str());
                m_modules.erase(module_id);
                result = nullptr;
            }
        }
        catch (std::bad_alloc& err)
        {
            /*Codes_SRS_NODEJS_MODULES_MGR_13_006: [ AddModule shall return NULL if an underlying API call fails. ]*/
            LogError("Memory allocation error occurred with %s", err.what());
            result = nullptr;
        }
    }
    else
    {
        /*Codes_SRS_NODEJS_MODULES_MGR_13_006: [ AddModule shall return NULL if an underlying API call fails. ]*/
        LogError("Could not start Node JS thread.");
        result = nullptr;
    }

    return result;

    /*Codes_SRS_NODEJS_MODULES_MGR_13_011: [ AddModule shall release the lock on the ModulesManager object. ]*/
    // destructor of LockGuard releases the lock
}

void ModulesManager::RemoveModule(size_t module_id)
{
    /*Codes_SRS_NODEJS_MODULES_MGR_13_012: [ RemoveModule shall acquire a lock on the ModulesManager object. ]*/
    LockGuard<ModulesManager> lock_guard{ *this };
    auto entry = m_modules.find(module_id);
    if (entry != m_modules.end())
    {
        /*Codes_SRS_NODEJS_MODULES_MGR_13_014: [ RemoveModule shall remove the module from it's internal collection of modules. ]*/
        m_modules.erase(entry);
    }

    /*Codes_SRS_NODEJS_MODULES_MGR_13_015: [ RemoveModule shall release the lock on the ModulesManager object. ]*/
    // destructor of LockGuard releases the lock
}

void ModulesManager::ExitNodeThread() const
{
    NodeJSIdle::Get()->AddCallback([]() {
        NodeJSUtils::RunWithNodeContext([](v8::Isolate* isolate, v8::Local<v8::Context> context) {
            NodeJSUtils::RunScript(isolate, context, "global._gatewayExit = true;");
        });
    });
}

void ModulesManager::JoinNodeThread() const
{
    THREAD_HANDLE thread;
    {
        LockGuard<ModulesManager> lock_guard{ *this };
        thread = m_nodejs_thread;
    }

    ThreadAPI_Join(thread, nullptr);
}

int ModulesManager::NodeJSRunner()
{
    // start up Node.js!
    int argc = 3;

    // libuv on Linux expects the argv to be formatted in a particular
    // way - i.e. all the argv values should be contiguous in memory;
    // doing the following:
    //
    //      const char argv[] = { "node", "-e", NODE_STARTUP_SCRIPT };
    //
    // does seem to allocate the string literals in contiguous memory
    // but there's apparently no guarantee that the compiler is obligated
    // to do so and besides there might be "holes" in the buffer for
    // memory alignment reasons which causes the libuv validation to fail
    // (which appears to be a bug in libuv - see the 'assert' call in
    // the function 'uv_setup_args' in 'proctitle.c')

    const char argv[] = "node" "\0" "-e" "\0" NODE_STARTUP_SCRIPT "\0";
    const char *p1 = &argv[0];
    const char *p2 = &argv[sizeof("node")];
    const char *p3 = p2 + sizeof("-e");
    const char *pargv[] = { p1, p2, p3 };

    int result = node::Start(argc, const_cast<char**>(pargv), NodeJSIdle::OnIdle);

    // reset object state to indicate that the node thread has now
    // left the building
    LockGuard<ModulesManager> lock_guard{ *this };
    m_nodejs_thread = nullptr;
    m_node_initialized = false;

    return result;
}

int ModulesManager::NodeJSRunnerInternal(void* user_data)
{
    return reinterpret_cast<ModulesManager*>(user_data)->NodeJSRunner();
}

void ModulesManager::OnNodeInitComplete(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    auto manager = ModulesManager::Get();
    LockGuard<ModulesManager> lock_guard{ *manager };
    manager->m_node_initialized = true;
    for (const auto& callback : manager->m_call_on_init)
    {
        (*callback)();
    }

    manager->m_call_on_init.clear();
}

THREADAPI_RESULT ModulesManager::StartNode()
{
    THREADAPI_RESULT result;

    // setup a method in the global context that the startup script
    // will invoke to notify us of completion of initialization of Node
    NodeJSIdle::Get()->AddCallback([]() {
        auto notify_init = nodejs_module::NodeJSUtils::CreateObjectWithMethod(
            "onNodeInitComplete",
            ModulesManager::OnNodeInitComplete
        );
        if (notify_init.IsEmpty() == true)
        {
            LogError("Could not create v8 object");
        }
        else
        {
            // add the object to global context
            if (nodejs_module::NodeJSUtils::AddObjectToGlobalContext("_gatewayInit", notify_init) == false)
            {
                LogError("Could not add v8 object to global context");
            }

            notify_init.Reset();
        }
    });

    // The caller of this method would have already acquired an
    // object lock.
    result = ThreadAPI_Create(
        &m_nodejs_thread,
        ModulesManager::NodeJSRunnerInternal,
        reinterpret_cast<void*>(this)
    );
    if (result != THREADAPI_OK)
    {
        LogError("ThreadAPI_Create failed");
        m_nodejs_thread = nullptr;
    }

    return result;
}

bool ModulesManager::StartModule(size_t module_id)
{
    bool result;

    // we cannot proceed till node has been initialized completely so
    // postpone addition of the module till then
    if (m_node_initialized == false)
    {
        auto callback = std::make_shared<std::function<void()>>([this, module_id]() {
            if (StartModule(module_id) == false)
            {
                LogInfo("Could not start module");
                RemoveModule(module_id);
            }
        });

        m_call_on_init.push_back(callback);
        result = true;
    }
    else
    {
        NodeJSIdle::Get()->AddCallback([module_id, this]() {
            // it is possible that the module has been deleted by the time
            // we get here; so we check that the module exists before we
            // do anything
            if (HasModule(module_id) == true)
            {
                auto& handle_data = GetModuleFromId(module_id);

                /*Codes_SRS_NODEJS_MODULES_MGR_13_010: [ When the callback is invoked via libuv, it shall invoke the NODEJS_MODULE_HANDLE_DATA::on_module_start function. ]*/
                handle_data.on_module_start(&handle_data);
            }
            else
            {
                LogError("Module has already been deleted");
            }
        });

        result = true;
    }

    return result;
}
