// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DOTNETCORE_MODULESMANAGER_H
#define DOTNETCORE_MODULESMANAGER_H

#include <stdlib.h>
#include <map>
#include <functional>
#include <memory>

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/lock.h"
#include "lock.h"
#include "dotnetcore_common.h"

namespace dotnetcore_module
{
    class ModulesManager
    {
    protected:
        static ModulesManager* m_instance;

        static LOCK_HANDLE m_lock_handle;

        /**
        * Counter used to assign module identifiers.
        */
        size_t m_moduleid_counter;
    private:
        /**
         * The list of active modules.
         */
        std::map<size_t, DOTNET_CORE_HOST_HANDLE_DATA> m_modules;

        /**
         * Lock used to protect access to members of this object.
         */
        Lock m_lock;

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

#endif // DOTNETCORE_MODULESMANAGER_H