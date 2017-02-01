.NET Core modules manager requirements
======================================
References
----------
  - [High level design document](./dotnet_core_binding_hld.md)
  - [Low level API requirements](./dotnet_core_binding_requirements.md)

Overview
--------

This document specifies the requirements for the *modules manager* class that is
a part of the implementation of the .NET Core hosting module. This class is
responsible for the following tasks:

  - Maintain a list of .NET Core module instances in a thread safe manner

Class interface
----------------
```c++
namespace dotnetcore_module
{
    class ModulesManager
    {
    private:
        /**
         * The list of active modules.
         */
        std::vector<DOTNET_CORE_HOST_HANDLE_DATA> m_modules;

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
         */
        static ModulesManager* Get();

        /**
         * Object lock/unlock methods.
         */
        void AcquireLock() const;
        void ReleaseLock() const;

        /**
         * Add/remove modules.
         */
        DOTNET_CORE_HOST_HANDLE_DATA* AddModule(const DOTNET_CORE_HOST_HANDLE_DATA& handle_data);
        void RemoveModule(size_t module_id);
        bool HasModule(size_t module_id) const;
        DOTNET_CORE_HOST_HANDLE_DATA& GetModuleFromId(size_t module_id);
    };
};
```

Get
---
```c++
static ModulesManager* ModulesManager::Get()
```

**SRS_DOTNET_CORE_MODULES_MGR_04_001: [** This method shall acquire the lock on a static lock object. **]**

**SRS_DOTNET_CORE_MODULES_MGR_04_002: [** This method shall return `NULL` if an underlying API call fails. **]**

**SRS_DOTNET_CORE_MODULES_MGR_04_003: [** This method shall create an instance of `ModulesManager` if this is the first call. **]**

**SRS_DOTNET_CORE_MODULES_MGR_04_004: [** This method shall return a non-`NULL` pointer to a `ModulesManager` instance when the object has been successfully insantiated. **]**

**SRS_DOTNET_CORE_MODULES_MGR_04_016: [** This method shall release the lock on the static lock object before returning. **]**


AddModule
---------
```c++
DOTNET_CORE_HOST_HANDLE_DATA* ModulesManager::AddModule(const DOTNET_CORE_HOST_HANDLE_DATA& handle_data)
```

**SRS_DOTNET_CORE_MODULES_MGR_04_005: [** `AddModule` shall acquire a lock on the `ModulesManager` object. **]**

**SRS_DOTNET_CORE_MODULES_MGR_04_006: [** `AddModule` shall return `NULL` if an underlying API call fails. **]**

**SRS_DOTNET_CORE_MODULES_MGR_04_007: [** `AddModule` shall add the module to it's internal collection of modules. **]**

**SRS_DOTNET_CORE_MODULES_MGR_04_008: [** `AddModule` shall release the lock on the `ModulesManager` object. **]**

RemoveModule
------------
```c++
void ModulesManager::RemoveModule(size_t module_id)
```

**SRS_DOTNET_CORE_MODULES_MGR_04_012: [** `RemoveModule` shall acquire a lock on the `ModulesManager` object. **]**

**SRS_DOTNET_CORE_MODULES_MGR_04_013: [** `RemoveModule` shall do nothing if an underlying API call fails. **]**

**SRS_DOTNET_CORE_MODULES_MGR_04_014: [** `RemoveModule` shall remove the module from it's internal collection of modules. **]**

**SRS_DOTNET_CORE_MODULES_MGR_04_015: [** `RemoveModule` shall release the lock on the `ModulesManager` object. **]**



AcquireLock
-----------
```c++
void AcquireLock() const;
```
**SRS_DOTNET_CORE_MODULES_MGR_04_017: [** `AcquireLock` shall acquire a lock by calling `AcquireLock` on `Lock` object. **]**

ReleaseLock
-----------
```c++
void ReleaseLock() const;
```
**SRS_DOTNET_CORE_MODULES_MGR_04_018: [** `ReleaseLock` shall release the lock by calling `ReleaseLock` on `Lock` object. **]**

HasModule
---------
```c++
bool HasModule(size_t module_id) const;
```
**SRS_DOTNET_CORE_MODULES_MGR_04_019: [** `HasModule` shall make sure it's thread safe and call `lock_guard`. **]**

**SRS_DOTNET_CORE_MODULES_MGR_04_020: [** `HasModule` shall call `find` passing module id and return result. **]**

GetModuleFromId
---------------
```c++
DOTNET_CORE_HOST_HANDLE_DATA& GetModuleFromId(size_t module_id);
```
**SRS_DOTNET_CORE_MODULES_MGR_04_021: [** `GetModuleFromId` shall make sure it's thread safe and call `lock_guard`. **]**

**SRS_DOTNET_CORE_MODULES_MGR_04_022: [** `GetModuleFromId` shall return the instance based on module_id. **]**