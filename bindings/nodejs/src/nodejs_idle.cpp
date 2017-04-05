// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <vector>
#include <functional>

#include "uv.h"
#include "lock.h"
#include "nodejs_utils.h"
#include "nodejs_idle.h"

using namespace nodejs_module;

NodeJSIdle::NodeJSIdle() :
    m_initialized(false)
{}

NodeJSIdle::~NodeJSIdle()
{}

NodeJSIdle* NodeJSIdle::Get()
{
    static NodeJSIdle* instance = nullptr;
    static LOCK_HANDLE lock = Lock_Init();

    if (lock == nullptr)
    {
        LogError("Could not instantiate LOCK_HANDLE");
        // 'instance' is already NULL
    }
    else if (instance == nullptr)
    {
        if (::Lock(lock) == LOCK_OK)
        {
            if (instance == nullptr)
            {
                try
                {
                    instance = new NodeJSIdle();
                }
                catch (std::bad_alloc& err)
                {
                    LogError("new operator failed with %s", err.what());
                    instance = nullptr;
                }
            }

            if (::Unlock(lock) != LOCK_OK)
            {
                LogError("Could not unlock LOCK_HANDLE");
                delete instance;
                instance = nullptr;
            }
        }
        else
        {
            LogError("Could not lock LOCK_HANDLE");
            // 'instance' is already NULL
        }
    }

    return instance;
}

bool nodejs_module::NodeJSIdle::IsInitialized() const
{
    LockGuard<NodeJSIdle> lock_guard{ *this };
    return m_initialized;
}

bool nodejs_module::NodeJSIdle::Initialize()
{
    LockGuard<NodeJSIdle> lock_guard{ *this };

    if (m_initialized == false)
    {
        auto loop = uv_default_loop();
        if (loop == nullptr)
        {
            LogError("uv_default_loop() failed.");
            m_initialized = false;
        }
        else
        {
            if (uv_async_init(loop, &m_uv_async, NodeJSIdle::OnIdle) != 0)
            {
                LogError("uv_async_init() failed.");
                m_initialized = false;
            }
            else
            {
                m_initialized = true;
            }
        }
    }

    return m_initialized;
}

void nodejs_module::NodeJSIdle::DeInitialize()
{
    LockGuard<NodeJSIdle> lock_guard{ *this };
    uv_close(reinterpret_cast<uv_handle_t*>(&m_uv_async), nullptr);
    m_initialized = false;
}

void NodeJSIdle::AcquireLock() const
{
    m_lock.AcquireLock();
}

void NodeJSIdle::ReleaseLock() const
{
    m_lock.ReleaseLock();
}

void NodeJSIdle::InvokeCallbacks()
{
    AcquireLock();
    while (m_callbacks.empty() == false)
    {
        auto cb = m_callbacks.front();
        m_callbacks.pop();
        ReleaseLock();
        cb();
        AcquireLock();
    }
    ReleaseLock();
}

void NodeJSIdle::OnIdle(uv_async_t* handle)
{
    (void)handle;
    NodeJSIdle::Get()->InvokeCallbacks();
}
