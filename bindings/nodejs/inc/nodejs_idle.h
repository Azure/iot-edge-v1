// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef NODEJS_IDLE_H
#define NODEJS_IDLE_H

#include <queue>
#include <functional>

#include "uv.h"
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#include "v8.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include "lock.h"

namespace nodejs_module
{
    class NodeJSIdle
    {
    private:
        std::queue<std::function<void()>> m_callbacks;
        Lock m_lock;
        bool m_initialized;
        uv_async_t m_uv_async;

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
         * Has object been initialized?
         */
        bool IsInitialized() const;

        /**
         * Initialize the object by registering the libuv
         * async callback.
         */
        bool Initialize();
        void DeInitialize();

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
        static void OnIdle(uv_async_t* handle);

    private:
        void InvokeCallbacks();
    };

    template <typename TCallback>
    void NodeJSIdle::AddCallback(TCallback callback)
    {
        LockGuard<NodeJSIdle> lock_guard{ *this };
        m_callbacks.push(callback);

        // signal that the async function should be called on libuv's
        // thread; libuv's doc says that multiple calls to this function
        // will get coalesced if they occur before the call is actually
        // made; so it's safe to call this multiple times
        if (uv_is_active(reinterpret_cast<uv_handle_t*>(&m_uv_async)))
        {
            uv_async_send(&m_uv_async);
        }
    }
};

#endif // NODEJS_IDLE_H
