// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef NODEJS_IDLE_H
#define NODEJS_IDLE_H

#include <vector>
#include <functional>

#include "uv.h"
#include "v8.h"

#include "lock.h"

namespace nodejs_module
{
    class NodeJSIdle
    {
    private:
        std::vector<std::function<void()>> m_callbacks;
        Lock m_lock;

        /**
         * Private constructor to enforce singleton instance.
         */
        NodeJSIdle();

    public:
        ~NodeJSIdle();

        /**
         * Gets a pointer to the singleton instance of NodeJSIdle.
         * Lazily creates an instance.
         */
        static NodeJSIdle* Get();

        /**
         * Object lock/unlock methods.
         */
        void AcquireLock() const;
        void ReleaseLock() const;

        /**
         * Add callbacks to be invoked during Node's idle processing.
         */
        template <typename TCallback>
        void AddCallback(TCallback callback);

        /**
         * Idle callback function called from Node's event loop.
         */
        static void OnIdle(v8::Isolate* isolate);

    private:
        void InvokeCallbacks();
    };

    template <typename TCallback>
    void NodeJSIdle::AddCallback(TCallback callback)
    {
        LockGuard<NodeJSIdle> lock_guard{ *this };
        m_callbacks.push_back(callback);
    }
};

#endif // NODEJS_IDLE_H
