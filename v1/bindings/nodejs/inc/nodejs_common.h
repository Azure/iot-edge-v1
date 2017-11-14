// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef NODEJS_COMMON_H
#define NODEJS_COMMON_H

#include <future>
#include <string>
#include <utility>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#include "v8.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include "broker.h"
#include "lock.h"

struct NODEJS_MODULE_HANDLE_DATA;

typedef void(*PFNMODULE_START)(NODEJS_MODULE_HANDLE_DATA* handle_data);

enum class NodeModuleState
{
    error,
    initialized
};

struct NODEJS_MODULE_HANDLE_DATA
{
    NODEJS_MODULE_HANDLE_DATA()
        :
        broker(nullptr),
        v8_isolate(nullptr),
        module_id(0),
        on_module_start(nullptr),
        module_state(NodeModuleState::error)
    {
    }

    NODEJS_MODULE_HANDLE_DATA(
        BROKER_HANDLE broker,
        const char* path,
        const char* config,
        PFNMODULE_START module_start)
        :
        broker(broker),
        main_path(path),
        configuration_json(config == nullptr ? "null" : config),
        v8_isolate(nullptr),
        module_id(0),
        on_module_start(module_start),
        module_state(NodeModuleState::error)
    {
    }

    NODEJS_MODULE_HANDLE_DATA(NODEJS_MODULE_HANDLE_DATA&& rhs)
    {
        broker = rhs.broker;
        main_path = rhs.main_path;
        configuration_json = rhs.configuration_json;
        v8_isolate = rhs.v8_isolate;
        on_module_start = rhs.on_module_start;
        module_id = rhs.module_id;
        module_state = rhs.module_state;


        if (v8_isolate != nullptr && rhs.module_object.IsEmpty() == false)
        {
            module_object.Reset(v8_isolate, rhs.module_object);
            rhs.module_object.Reset();
        }
    }

    NODEJS_MODULE_HANDLE_DATA(const NODEJS_MODULE_HANDLE_DATA& rhs)
    {
        broker = rhs.broker;
        main_path = rhs.main_path;
        configuration_json = rhs.configuration_json;
        v8_isolate = rhs.v8_isolate;
        on_module_start = rhs.on_module_start;
        module_id = rhs.module_id;
        module_state = rhs.module_state;


        if (v8_isolate != nullptr && rhs.module_object.IsEmpty() == false)
        {
            module_object.Reset(v8_isolate, rhs.module_object);
        }
    }

    NODEJS_MODULE_HANDLE_DATA(const NODEJS_MODULE_HANDLE_DATA& rhs, size_t module_id)
    {
        broker = rhs.broker;
        main_path = rhs.main_path;
        configuration_json = rhs.configuration_json;
        v8_isolate = rhs.v8_isolate;
        on_module_start = rhs.on_module_start;
        this->module_id = module_id;
        module_state = rhs.module_state;

        if (v8_isolate != nullptr && rhs.module_object.IsEmpty() == false)
        {
            module_object.Reset(v8_isolate, rhs.module_object);
        }
    }

    void AcquireLock() const
    {
        object_lock.AcquireLock();
    }

    void ReleaseLock() const
    {
        object_lock.ReleaseLock();
    }

    NodeModuleState GetModuleState() const
    {
        nodejs_module::LockGuard<NODEJS_MODULE_HANDLE_DATA> lock_guard(*this);
        return module_state;
    }

    void SetModuleState(NodeModuleState state)
    {
        nodejs_module::LockGuard<NODEJS_MODULE_HANDLE_DATA> lock_guard(*this);
        module_state = state;
    }

    BROKER_HANDLE broker;
    std::string main_path;
    std::string configuration_json;
    v8::Isolate *v8_isolate;
    v8::Persistent<v8::Object> module_object;
    size_t module_id;
    PFNMODULE_START on_module_start;
    NodeModuleState module_state;
    nodejs_module::Lock object_lock;

    /*
     * WARNING: The promise follows the lifespan of the class, NOT
     *          the lifespan of the handle data. This means the
     *          std::future<NodeModuleState> associated with this
     *          class will NOT survive a copy or move operation.
     */
    std::promise<NodeModuleState> create_complete;
};

#endif /*NODEJS_COMMON_H*/
