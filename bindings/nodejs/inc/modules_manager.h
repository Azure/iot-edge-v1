// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef NODEJS_MODULESTATE_H
#define NODEJS_MODULESTATE_H

#include <stdlib.h>
#include <unordered_map>

#include "uv.h"

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/lock.h"

#include "lock.h"
#include "nodejs_common.h"

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

#endif // NODEJS_MODULESTATE_H
