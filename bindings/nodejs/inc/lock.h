// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef NODEJS_LOCK_H
#define NODEJS_LOCK_H

#include "azure_c_shared_utility/iot_logging.h"
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
            if (m_lock == NULL)
            {
                LogError("Lock_Init failed");
            }
        }

        ~Lock()
        {
            if (m_lock != NULL)
            {
                ::Lock_Deinit(m_lock);
            }
        }

        LOCK_RESULT Acquire() const
        {
            LOCK_RESULT result;
            if (m_lock == NULL)
            {
                result = LOCK_ERROR;
            }
            else
            {
                result = ::Lock(m_lock);
                if (result != LOCK_OK)
                {
                    LogError("Lock failed");
                }
            }

            return result;
        }

        LOCK_RESULT Release() const
        {
            LOCK_RESULT result;
            if (m_lock == NULL)
            {
                result = LOCK_ERROR;
            }
            else
            {
                result = ::Unlock(m_lock);
                if (result != LOCK_OK)
                {
                    LogError("Unlock failed");
                }
            }

            return result;
        }
    };

    template <typename T>
    class LockGuard
    {
    private:
        const T& m_lockable;
        LOCK_RESULT m_result;

    public:
        LockGuard(const T& lockable) : m_lockable{ lockable }
        {
            m_result = m_lockable.Lock();
        }

        ~LockGuard()
        {
            if (m_lockable.Unlock() != LOCK_OK)
            {
                LogError("Unlock failed");
            }
        }

        LOCK_RESULT GetResult()
        {
            return m_result;
        }
    };
};

#endif // NODEJS_LOCK_H
