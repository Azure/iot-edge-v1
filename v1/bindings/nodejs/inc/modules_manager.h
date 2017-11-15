// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef NODEJS_MODULESMANAGER_H
#define NODEJS_MODULESMANAGER_H

#include <stdlib.h>
#include <map>
#include <functional>
#include <memory>

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
        std::map<size_t, NODEJS_MODULE_HANDLE_DATA> m_modules;

        /**
         * Lock used to protect access to members of this object.
         */
        Lock m_lock;

        /**
         * Counter used to assign module identifiers.
         */
        size_t m_moduleid_counter;

        /**
         * List of functions to be invoked when Node initializes completely.
         */
        std::vector<std::shared_ptr<std::function<void()>>> m_call_on_init;

        /**
         * Tracks whether NodeJS has been initialized.
         */
        bool m_node_initialized;

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
         * Cause Node's thread to exit.
         */
        void ExitNodeThread() const;
        void JoinNodeThread() const;

        /**
         * Add/remove modules.
         */
        NODEJS_MODULE_HANDLE_DATA* AddModule(const NODEJS_MODULE_HANDLE_DATA& handle_data);
        void RemoveModule(size_t module_id);
        bool HasModule(size_t module_id) const;
        NODEJS_MODULE_HANDLE_DATA& GetModuleFromId(size_t module_id);
        bool IsNodeInitialized() const;

    private:
        THREADAPI_RESULT StartNode();
        bool StartModule(size_t module_id);
        int NodeJSRunner();

        static int NodeJSRunnerInternal(void* user_data);
        static void OnNodeInitComplete(const v8::FunctionCallbackInfo<v8::Value>& info);
    };
};

#endif // NODEJS_MODULESMANAGER_H
