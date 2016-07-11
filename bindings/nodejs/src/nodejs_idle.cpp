// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <vector>
#include <functional>

#include "uv.h"
#include "v8.h"

#include "lock.h"
#include "nodejs_utils.h"
#include "nodejs_idle.h"

using namespace nodejs_module;

NodeJSIdle::NodeJSIdle()
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
        auto cb = m_callbacks.back();
        m_callbacks.pop_back();
        ReleaseLock();
        cb();
        AcquireLock();
    }
    ReleaseLock();
}

void NodeJSIdle::OnIdle(v8::Isolate* isolate)
{
    NodeJSIdle::Get()->InvokeCallbacks();
}
