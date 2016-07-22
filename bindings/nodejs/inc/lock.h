// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef NODEJS_LOCK_H
#define NODEJS_LOCK_H

#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/lock.h"

namespace nodejs_module
{
    class Lock
    {
    private:
        LOCK_HANDLE m_lock;

    public:
        Lock()
        {
            m_lock = ::Lock_Init();
            if (m_lock == nullptr)
            {
                LogError("Lock_Init() failed");
                throw LOCK_ERROR;
            }
        }

        ~Lock()
        {
            if (m_lock != nullptr)
            {
                ::Lock_Deinit(m_lock);
            }
        }

        void AcquireLock() const
        {
            LOCK_RESULT result;
            result = ::Lock(m_lock);
            if (result != LOCK_OK)
            {
                LogError("Lock() failed");
                throw result;
            }
        }

        void ReleaseLock() const
        {
            LOCK_RESULT result = ::Unlock(m_lock);
            if (result != LOCK_OK)
            {
                LogError("Unlock() failed");
                throw result;
            }
        }
    };

    template <typename T>
    class LockGuard
    {
    private:
        const T& m_lockable;

    public:
        explicit LockGuard(const T& lockable) : m_lockable{ lockable }
        {
            m_lockable.AcquireLock();
        }

        ~LockGuard()
        {
            m_lockable.ReleaseLock();
        }

        LockGuard(const LockGuard&) = delete;
        LockGuard(LockGuard&&) = delete;
        LockGuard& operator=(const LockGuard&) = delete;
        LockGuard& operator=(LockGuard&&) = delete;
    };
};

#endif // NODEJS_LOCK_H
