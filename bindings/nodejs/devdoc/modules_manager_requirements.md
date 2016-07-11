Node JS modules manager requirements
====================================

References
----------

  - [High level design document](./nodejs_bindings_hld.md)
  - [Low level API requirements](./nodejs_bindings_requirements.md)

Overview
--------

This document specifies the requirements for the *modules manager* class that is
a part of the implementation of the Node JS hosting module. This class is
responsible for the following tasks:

  - Maintain a list Node JS module instances in a thread-safe manner
  - Lazily start up Node JS when invoked the first time
  - Load JS modules by running the `create` method on Node's event loop via
    libuv

Class interface
----------------
```c++
namespace nodejs_module
{
    class ModulesManager
    {
    private:
        /**
         * The handle to the NodeJS thread which runs the libuv loop.
         */
        THREAD_HANDLE m_nodejs_thread;

        /**
         * The list of active modules.
         */
        std::unordered_map<size_t, NODEJS_MODULE_HANDLE_DATA> m_modules;

        /**
         * Lock used to protect access to members of this object.
         */
        Lock m_lock;

        /**
         * Counter used to assign module identifiers.
         */
        size_t m_moduleid_counter;

        /**
         * Private constructor to enforce singleton instance.
         */
        ModulesManager();

    public:
        ~ModulesManager();

        /**
         * Gets a pointer to the singleton instance of ModulesManager.
         * Lazily creates an instance.
         */
        static ModulesManager* Get();

        /**
         * Object lock/unlock methods.
         */
        LOCK_RESULT Lock() const;
        LOCK_RESULT Unlock() const;

        /**
         * Add/remove modules.
         */
        NODEJS_MODULE_HANDLE_DATA* AddModule(const NODEJS_MODULE_HANDLE_DATA& handle_data);
        void RemoveModule(size_t module_id);
        bool HasModule(size_t module_id) const;
        NODEJS_MODULE_HANDLE_DATA& GetModuleFromId(size_t module_id);

    private:
        THREADAPI_RESULT StartNode();
        bool StartModule(size_t module_id);
        void OnStartModule(size_t module_id, uv_idle_t* handle);
        int NodeJSRunner();

        static int NodeJSRunnerInternal(void* user_data);
        static void OnStartModuleInternal(uv_idle_t* handle);
    };
};
```

Get
---
```c++
static ModulesManager* ModulesManager::Get()
```

**SRS_NODEJS_MODULES_MGR_13_001: [** This method shall acquire the lock on a static lock object. **]**

**SRS_NODEJS_MODULES_MGR_13_002: [** This method shall return `NULL` if an underlying API call fails. **]**

**SRS_NODEJS_MODULES_MGR_13_003: [** This method shall create an instance of `ModulesManager` if this is the first call. **]**

**SRS_NODEJS_MODULES_MGR_13_004: [** This method shall return a non-`NULL` pointer to a `ModulesManager` instance when the object has been successfully insantiated. **]**

**SRS_NODEJS_MODULES_MGR_13_016: [** This method shall release the lock on the static lock object before returning. **]**

AddModule
---------
```c++
NODEJS_MODULE_HANDLE_DATA* ModulesManager::AddModule(const NODEJS_MODULE_HANDLE_DATA& handle_data)
```

**SRS_NODEJS_MODULES_MGR_13_005: [** `AddModule` shall acquire a lock on the `ModulesManager` object. **]**

**SRS_NODEJS_MODULES_MGR_13_006: [** `AddModule` shall return `NULL` if an underlying API call fails. **]**

**SRS_NODEJS_MODULES_MGR_13_007: [** If this is the first time that this method is being called then it shall start a new thread and start up Node JS from the thread. **]**

**SRS_NODEJS_MODULES_MGR_13_008: [** `AddModule` shall add the module to it's internal collection of modules. **]**

**SRS_NODEJS_MODULES_MGR_13_009: [** `AddModule` shall schedule a callback to be invoked on the Node engine's event loop via libuv. **]**

**SRS_NODEJS_MODULES_MGR_13_010: [** When the callback is invoked via libuv, it shall invoke the `NODEJS_MODULE_HANDLE_DATA::on_module_start` function. **]**

**SRS_NODEJS_MODULES_MGR_13_011: [** `AddModule` shall release the lock on the `ModulesManager` object. **]**

RemoveModule
------------
```c++
void ModulesManager::RemoveModule(size_t module_id)
```

**SRS_NODEJS_MODULES_MGR_13_012: [** `RemoveModule` shall acquire a lock on the `ModulesManager` object. **]**

**SRS_NODEJS_MODULES_MGR_13_013: [** `RemoveModule` shall do nothing if an underlying API call fails. **]**

**SRS_NODEJS_MODULES_MGR_13_014: [** `RemoveModule` shall remove the module from it's internal collection of modules. **]**

**SRS_NODEJS_MODULES_MGR_13_015: [** `RemoveModule` shall release the lock on the `ModulesManager` object. **]**
