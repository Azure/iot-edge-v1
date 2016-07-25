// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <cstdlib>
#include <signal.h>

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"
#include "azure_c_shared_utility/condition.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/list.h"
#include "message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/refcount.h"

static MICROMOCK_MUTEX_HANDLE g_testByTest;
static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

#define GBALLOC_H

extern "C" int gballoc_init(void);
extern "C" void gballoc_deinit(void);
extern "C" void* gballoc_malloc(size_t size);
extern "C" void* gballoc_calloc(size_t nmemb, size_t size);
extern "C" void* gballoc_realloc(void* ptr, size_t size);
extern "C" void gballoc_free(void* ptr);

namespace BASEIMPLEMENTATION
{
#define Lock(x) (LOCK_OK + gballocState - gballocState) /*compiler warning about constant in if condition*/
#define Unlock(x) (LOCK_OK + gballocState - gballocState)
#define Lock_Init() (LOCK_HANDLE)0x42
#define Lock_Deinit(x) (LOCK_OK + gballocState - gballocState)
#include "gballoc.c"
#undef Lock
#undef Unlock
#undef Lock_Init
#undef Lock_Deinit
#include "vector.c"
};

#include "message_bus.h"
#include "azure_c_shared_utility/lock.h"

DEFINE_MICROMOCK_ENUM_TO_STRING(MESSAGE_BUS_RESULT, MESSAGE_BUS_RESULT_VALUES);

static size_t currentmalloc_call;
static size_t whenShallmalloc_fail;

static size_t currentVECTOR_create_call;
static size_t whenShallVECTOR_create_fail;

static size_t currentVECTOR_push_back_call;
static size_t whenShallVECTOR_push_back_fail;

static size_t currentVECTOR_find_if_call;
static size_t whenShallVECTOR_find_if_fail;

static size_t currentlist_find_call;
static size_t whenShalllist_find_fail;

static size_t currentlist_create_call;
static size_t whenShalllist_create_fail;

static size_t currentlist_add_call;
static size_t whenShalllist_add_fail;


static size_t currentLock_Init_call;
static size_t whenShallLock_Init_fail;

static size_t currentLock_call;
static size_t whenShallLock_fail;

static size_t currentUnlock_call;

static size_t currentCond_Init_call;
static size_t whenShallCond_Init_fail;

static size_t currentCond_Post_call;
static size_t whenShallCond_Post_fail;

static size_t currentThreadAPI_Create_call;
static size_t whenShallThreadAPI_Create_fail;

typedef struct LIST_ITEM_INSTANCE_TAG
{
    const void* item;
    void* next;
} LIST_ITEM_INSTANCE;

typedef struct LIST_INSTANCE_TAG
{
    LIST_ITEM_INSTANCE* head;
} LIST_INSTANCE;

static size_t current_list_index;
static const void *fake_list[10];

static bool shouldThreadAPI_Create_invoke_callback;
static THREAD_START_FUNC thread_func_to_call;
static void* thread_func_args;

struct FakeModule_Receive_Call_Status
{
    MESSAGE_HANDLE messageHandle;
    bool was_called;
};
static FakeModule_Receive_Call_Status call_status_for_FakeModule_Receive;

// intercept variables for Condition_Wait mock
typedef COND_RESULT(*PFN_CONDITION_WAIT_INTERCEPT)(void);
static bool shouldIntercept_Condition_Wait;
static void* interceptArgs_for_Condition_Wait;
static PFN_CONDITION_WAIT_INTERCEPT intercept_for_Condition_Wait;

static MODULE_HANDLE fake_module_handle = (MODULE_HANDLE)0x42;
class DummyGatewayModule : public IInternalGatewayModule
{
public:
    void Module_Receive(MESSAGE_HANDLE messageHandle) 
    {
        call_status_for_FakeModule_Receive.was_called = true;
        ASSERT_ARE_EQUAL(void_ptr, messageHandle, call_status_for_FakeModule_Receive.messageHandle);
    }
};
static DummyGatewayModule dummyGatewayModule;

MODULE fake_module =
{
    &dummyGatewayModule,
};


class RefCountObject
{
private:
    size_t ref_count;

public:
    RefCountObject() : ref_count(1)
    {
    }

    size_t inc_ref()
    {
        return ++ref_count;
    }

    void dec_ref()
    {
        if (--ref_count == 0)
        {
            delete this;
        }
    }
};

TYPED_MOCK_CLASS(CMessageBusMocks, CGlobalMock)
{
public:

    MOCK_STATIC_METHOD_1(, void*, gballoc_malloc, size_t, size)
        void* result2;
        currentmalloc_call++;
        if (currentmalloc_call == whenShallmalloc_fail)
        {
            result2 = NULL;
        }
        else
        {
            result2 = BASEIMPLEMENTATION::gballoc_malloc(size);
        }
    MOCK_METHOD_END(void*, result2);

    MOCK_STATIC_METHOD_1(, void, gballoc_free, void*, ptr)
        BASEIMPLEMENTATION::gballoc_free(ptr);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_0(, LOCK_HANDLE, Lock_Init)
        LOCK_HANDLE result2;
        ++currentLock_Init_call;
        if ((whenShallLock_Init_fail > 0) &&
            (currentLock_Init_call == whenShallLock_Init_fail))
        {
            result2 = NULL;
        }
        else
        {
            result2 = (LOCK_HANDLE)malloc(1);
        }
    MOCK_METHOD_END(LOCK_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, LOCK_RESULT, Lock, LOCK_HANDLE, lock)
        LOCK_RESULT result2;
        ++currentLock_call;
        if ((whenShallLock_fail > 0) &&
            (currentLock_call == whenShallLock_fail))
        {
            result2 = LOCK_ERROR;
        }
        else
        {
            result2 = LOCK_OK;
        }
    MOCK_METHOD_END(LOCK_RESULT, result2)

    MOCK_STATIC_METHOD_1(, LOCK_RESULT, Unlock, LOCK_HANDLE, lock)
        ASSERT_IS_TRUE((currentLock_call - currentUnlock_call) > 0);
        ++currentUnlock_call;
        auto result2 = LOCK_OK;
    MOCK_METHOD_END(LOCK_RESULT, result2)

    MOCK_STATIC_METHOD_1(, LOCK_RESULT, Lock_Deinit, LOCK_HANDLE, lock)
        free(lock);
        auto result2 = LOCK_OK;
    MOCK_METHOD_END(LOCK_RESULT, result2)

    MOCK_STATIC_METHOD_0(, COND_HANDLE, Condition_Init)
        COND_HANDLE result2;
        ++currentCond_Init_call;
        if ((whenShallCond_Init_fail > 0) &&
            (currentCond_Init_call == whenShallCond_Init_fail))
        {
            result2 = NULL;
        }
        else
        {
            result2 = (COND_HANDLE)malloc(2);
        }
    MOCK_METHOD_END(COND_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, COND_RESULT, Condition_Post, COND_HANDLE, handle)
        COND_RESULT result2;
        ++currentCond_Post_call;
        if ((whenShallCond_Post_fail > 0) &&
            (currentCond_Post_call == whenShallCond_Post_fail))
        {
            result2 = COND_ERROR;
        }
        else
        {
            result2 = COND_OK;
        }
    MOCK_METHOD_END(COND_RESULT, result2)

    MOCK_STATIC_METHOD_3(, COND_RESULT, Condition_Wait, COND_HANDLE, handle, LOCK_HANDLE, lock, int, timeout_milliseconds)
        auto result2 = COND_OK;
        if (shouldIntercept_Condition_Wait == true)
        {
            result2 = intercept_for_Condition_Wait();
        }
    MOCK_METHOD_END(COND_RESULT, result2)

    MOCK_STATIC_METHOD_1(, void, Condition_Deinit, COND_HANDLE, handle)
        free(handle);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, VECTOR_HANDLE, VECTOR_create, size_t, elementSize)
        VECTOR_HANDLE result2;
        ++currentVECTOR_create_call;
        if ((whenShallVECTOR_create_fail > 0) &&
            (currentVECTOR_create_call == whenShallVECTOR_create_fail))
        {
            result2 = NULL;
        }
        else
        {
            result2 = BASEIMPLEMENTATION::VECTOR_create(elementSize);
        }
    MOCK_METHOD_END(VECTOR_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, void, VECTOR_destroy, VECTOR_HANDLE, vector)
        BASEIMPLEMENTATION::VECTOR_destroy(vector);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_3(, int, VECTOR_push_back, VECTOR_HANDLE, vector, const void*, elements, size_t, numElements)
        int result2;
        ++currentVECTOR_push_back_call;
        if ((whenShallVECTOR_push_back_fail > 0) &&
            (currentVECTOR_push_back_call == whenShallVECTOR_push_back_fail))
        {
            result2 = __LINE__;
        }
        else
        {
            result2 = BASEIMPLEMENTATION::VECTOR_push_back(vector, elements, numElements);
        }
    MOCK_METHOD_END(int, result2)

    MOCK_STATIC_METHOD_3(, void, VECTOR_erase, VECTOR_HANDLE, vector, void*, elements, size_t, numElements)
        BASEIMPLEMENTATION::VECTOR_erase(vector, elements, numElements);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_2(, void*, VECTOR_element, VECTOR_HANDLE, vector, size_t, index)
        void* result2 = BASEIMPLEMENTATION::VECTOR_element(vector, index);
    MOCK_METHOD_END(void*, result2)

    MOCK_STATIC_METHOD_1(, void*, VECTOR_front, VECTOR_HANDLE, vector)
        void* result2 = BASEIMPLEMENTATION::VECTOR_front(vector);
    MOCK_METHOD_END(void*, result2)

    MOCK_STATIC_METHOD_1(, void*, VECTOR_back, const VECTOR_HANDLE, vector)
        void* result2 = BASEIMPLEMENTATION::VECTOR_back(vector);
    MOCK_METHOD_END(void*, result2)

    MOCK_STATIC_METHOD_3(, void*, VECTOR_find_if, VECTOR_HANDLE, vector, PREDICATE_FUNCTION, pred, const void*, value)
        void* result2;
        ++currentVECTOR_find_if_call;
        if ((whenShallVECTOR_find_if_fail > 0) &&
            (currentVECTOR_find_if_call == whenShallVECTOR_find_if_fail))
        {
            result2 = NULL;
        }
        else
        {
            result2 = BASEIMPLEMENTATION::VECTOR_find_if(vector, pred, value);
        }
    MOCK_METHOD_END(void*, result2)

    MOCK_STATIC_METHOD_1(, size_t, VECTOR_size, VECTOR_HANDLE, vector)
        size_t result2 = BASEIMPLEMENTATION::VECTOR_size(vector);
    MOCK_METHOD_END(size_t, result2)

    MOCK_STATIC_METHOD_3(, THREADAPI_RESULT, ThreadAPI_Create, THREAD_HANDLE*, threadHandle, THREAD_START_FUNC, func, void*, arg)
        THREADAPI_RESULT result2;
        ++currentThreadAPI_Create_call;
        if ((whenShallThreadAPI_Create_fail > 0) &&
            (currentThreadAPI_Create_call == whenShallThreadAPI_Create_fail))
        {
            result2 = THREADAPI_ERROR;
        }
        else
        {
            *threadHandle = (THREAD_HANDLE*)malloc(3);
            thread_func_to_call = func;
            thread_func_args = arg;

            result2 = THREADAPI_OK;
            if (shouldThreadAPI_Create_invoke_callback == true)
            {
                func(arg);
            }
        
        
        }
    MOCK_METHOD_END(THREADAPI_RESULT, result2)

    MOCK_STATIC_METHOD_2(, THREADAPI_RESULT, ThreadAPI_Join, THREAD_HANDLE, threadHandle, int*, res)
        free(threadHandle);
        auto result2 = THREADAPI_OK;
    MOCK_METHOD_END(THREADAPI_RESULT, result2)

    MOCK_STATIC_METHOD_1(, MESSAGE_HANDLE, Message_Create, const MESSAGE_CONFIG*, cfg)
        MESSAGE_HANDLE result2 = (MESSAGE_HANDLE)(new RefCountObject());
    MOCK_METHOD_END(MESSAGE_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, MESSAGE_HANDLE, Message_Clone, MESSAGE_HANDLE, message)
        ((RefCountObject*)message)->inc_ref();
    MOCK_METHOD_END(MESSAGE_HANDLE, message)

    MOCK_STATIC_METHOD_1(, void, Message_Destroy, MESSAGE_HANDLE, message)
        ((RefCountObject*)message)->dec_ref();
    MOCK_VOID_METHOD_END()

    // list.h

    MOCK_STATIC_METHOD_0(, LIST_HANDLE, list_create)
        ++currentlist_create_call;
        LIST_HANDLE result1;
        if (currentlist_create_call == whenShalllist_create_fail)
        {
            result1 = NULL;
        }
        else
        {
            result1 = (LIST_HANDLE)(new RefCountObject());
        }
    MOCK_METHOD_END(LIST_HANDLE, result1)

    MOCK_STATIC_METHOD_1(, void, list_destroy, LIST_HANDLE, list)
        ((RefCountObject*)list)->dec_ref();
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_2(, LIST_ITEM_HANDLE, list_add, LIST_HANDLE, list, const void*, item)
        LIST_ITEM_HANDLE result1;
        ++currentlist_add_call;
        if (currentlist_add_call == whenShalllist_add_fail)
        {
            result1 = NULL;
        }
        else
        {
            if ((list == NULL) ||
                (item == NULL))
            {
                result1 = NULL;
            }
            else
            {
                fake_list[current_list_index++] = item;
                result1 = (LIST_ITEM_HANDLE)item;
            }
        }
    MOCK_METHOD_END(LIST_ITEM_HANDLE, result1)

    MOCK_STATIC_METHOD_2(, int, list_remove, LIST_HANDLE, list, LIST_ITEM_HANDLE, item)
        int result2;
        if ((list == NULL) ||
            (item == NULL))
        {
            result2 = __LINE__;
        }
        else
        {
            /* do I need anything more compicated here? */
            result2 = 0;
        }

    MOCK_METHOD_END(int, result2)

    MOCK_STATIC_METHOD_1(, LIST_ITEM_HANDLE, list_get_head_item, LIST_HANDLE, list)
        LIST_ITEM_HANDLE result1;
        if (list == NULL)
        {
            result1 = NULL;
        }
        else
        {
            result1 = (LIST_ITEM_HANDLE)fake_list[0];
        }
    MOCK_METHOD_END(LIST_ITEM_HANDLE, result1)

    MOCK_STATIC_METHOD_1(, LIST_ITEM_HANDLE, list_get_next_item, LIST_ITEM_HANDLE, item_handle)
        LIST_ITEM_HANDLE result1;
        if (item_handle == NULL)
        {
            result1 = NULL;
        }
        else
        {
            result1 = (LIST_ITEM_HANDLE)(fake_list[current_list_index]);
        }
    MOCK_METHOD_END(LIST_ITEM_HANDLE, result1)

    MOCK_STATIC_METHOD_3(, LIST_ITEM_HANDLE, list_find, LIST_HANDLE, list, LIST_MATCH_FUNCTION, match_function, const void*, match_context)
        LIST_ITEM_HANDLE result1;
        currentlist_find_call++;
        if (currentlist_find_call == whenShalllist_find_fail)
        {
            result1 = NULL;
        }
        else
        {
            if ((list == NULL) ||
                (match_function == NULL))
            {
                result1 = NULL;
            }
            else if ((void*)&fake_module == (void*)match_context)
            {
                result1 = (LIST_ITEM_HANDLE)fake_list[0];
            }
            else
            {
                result1 = NULL;
                for (size_t i = 0; i < current_list_index; i++)
                {
                    if ((void*)fake_list[i] == (void*)match_context)
                    {
                        result1 = (LIST_ITEM_HANDLE)fake_list[i];
                        break;
                    }
                }
            }
        }
    MOCK_METHOD_END(LIST_ITEM_HANDLE, result1)

    MOCK_STATIC_METHOD_1(, const void*, list_item_get_value, LIST_ITEM_HANDLE, item_handle)
        const void* result1;
        if (item_handle == NULL)
        {
            result1 = NULL;
        }
        else
        {
            result1 = item_handle;
        }
    MOCK_METHOD_END(const void*, result1)
};

DECLARE_GLOBAL_MOCK_METHOD_1(CMessageBusMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CMessageBusMocks, , void, gballoc_free, void*, ptr);

DECLARE_GLOBAL_MOCK_METHOD_0(CMessageBusMocks, , LOCK_HANDLE, Lock_Init);
DECLARE_GLOBAL_MOCK_METHOD_1(CMessageBusMocks, , LOCK_RESULT, Lock, LOCK_HANDLE, lock);
DECLARE_GLOBAL_MOCK_METHOD_1(CMessageBusMocks, , LOCK_RESULT, Unlock, LOCK_HANDLE, lock);
DECLARE_GLOBAL_MOCK_METHOD_1(CMessageBusMocks, , LOCK_RESULT, Lock_Deinit, LOCK_HANDLE, lock);

DECLARE_GLOBAL_MOCK_METHOD_1(CMessageBusMocks, , VECTOR_HANDLE, VECTOR_create, size_t, elementSize);
DECLARE_GLOBAL_MOCK_METHOD_1(CMessageBusMocks, , void, VECTOR_destroy, VECTOR_HANDLE, vector);
DECLARE_GLOBAL_MOCK_METHOD_3(CMessageBusMocks, , int, VECTOR_push_back, VECTOR_HANDLE, vector, const void*, elements, size_t, numElements);
DECLARE_GLOBAL_MOCK_METHOD_3(CMessageBusMocks, , void, VECTOR_erase, VECTOR_HANDLE, vector, void*, elements, size_t, numElements);
DECLARE_GLOBAL_MOCK_METHOD_2(CMessageBusMocks, , void*, VECTOR_element, VECTOR_HANDLE, vector, size_t, index);
DECLARE_GLOBAL_MOCK_METHOD_1(CMessageBusMocks, , void*, VECTOR_front, VECTOR_HANDLE, vector);
DECLARE_GLOBAL_MOCK_METHOD_1(CMessageBusMocks, , void*, VECTOR_back, VECTOR_HANDLE, vector);
DECLARE_GLOBAL_MOCK_METHOD_3(CMessageBusMocks, , void*, VECTOR_find_if, VECTOR_HANDLE, vector, PREDICATE_FUNCTION, pred, const void*, value);
DECLARE_GLOBAL_MOCK_METHOD_1(CMessageBusMocks, , size_t, VECTOR_size, VECTOR_HANDLE, vector);

DECLARE_GLOBAL_MOCK_METHOD_0(CMessageBusMocks, , COND_HANDLE, Condition_Init);
DECLARE_GLOBAL_MOCK_METHOD_1(CMessageBusMocks, , COND_RESULT, Condition_Post, COND_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_3(CMessageBusMocks, , COND_RESULT, Condition_Wait, COND_HANDLE, handle, LOCK_HANDLE, lock, int, timeout_milliseconds);
DECLARE_GLOBAL_MOCK_METHOD_1(CMessageBusMocks, , void, Condition_Deinit, COND_HANDLE, handle);

DECLARE_GLOBAL_MOCK_METHOD_3(CMessageBusMocks, , THREADAPI_RESULT, ThreadAPI_Create, THREAD_HANDLE*, threadHandle, THREAD_START_FUNC, func, void*, arg);
DECLARE_GLOBAL_MOCK_METHOD_2(CMessageBusMocks, , THREADAPI_RESULT, ThreadAPI_Join, THREAD_HANDLE, threadHandle, int*, res);

DECLARE_GLOBAL_MOCK_METHOD_1(CMessageBusMocks, , MESSAGE_HANDLE, Message_Create, const MESSAGE_CONFIG*, cfg);
DECLARE_GLOBAL_MOCK_METHOD_1(CMessageBusMocks, , MESSAGE_HANDLE, Message_Clone, MESSAGE_HANDLE, message);
DECLARE_GLOBAL_MOCK_METHOD_1(CMessageBusMocks, , void, Message_Destroy, MESSAGE_HANDLE, message);

// list.h
DECLARE_GLOBAL_MOCK_METHOD_0(CMessageBusMocks, , LIST_HANDLE, list_create);
DECLARE_GLOBAL_MOCK_METHOD_1(CMessageBusMocks, , void, list_destroy, LIST_HANDLE, list);
DECLARE_GLOBAL_MOCK_METHOD_2(CMessageBusMocks, , LIST_ITEM_HANDLE, list_add, LIST_HANDLE, list, const void*, item);
DECLARE_GLOBAL_MOCK_METHOD_2(CMessageBusMocks, , int, list_remove, LIST_HANDLE, list, LIST_ITEM_HANDLE, item_handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CMessageBusMocks, , LIST_ITEM_HANDLE, list_get_head_item, LIST_HANDLE, list);
DECLARE_GLOBAL_MOCK_METHOD_1(CMessageBusMocks, , LIST_ITEM_HANDLE, list_get_next_item, LIST_ITEM_HANDLE, item_handle);
DECLARE_GLOBAL_MOCK_METHOD_3(CMessageBusMocks, , LIST_ITEM_HANDLE, list_find, LIST_HANDLE, list, LIST_MATCH_FUNCTION, match_function, const void*, match_context);
DECLARE_GLOBAL_MOCK_METHOD_1(CMessageBusMocks, , const void*, list_item_get_value, LIST_ITEM_HANDLE, item_handle);


BEGIN_TEST_SUITE(message_bus_uwp_unittests)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    g_testByTest = MicroMockCreateMutex();
    ASSERT_IS_NOT_NULL(g_testByTest);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    MicroMockDestroyMutex(g_testByTest);
    TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (!MicroMockAcquireMutex(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    currentmalloc_call = 0;
    whenShallmalloc_fail = 0;

    currentVECTOR_create_call = 0;
    whenShallVECTOR_create_fail = 0;

    currentVECTOR_push_back_call = 0;
    whenShallVECTOR_push_back_fail = 0;

    currentVECTOR_find_if_call = 0;
    whenShallVECTOR_find_if_fail = 0;

    currentLock_Init_call = 0;
    whenShallLock_Init_fail = 0;

    currentlist_find_call = 0;
    whenShalllist_find_fail = 0;

    currentlist_create_call = 0;
    whenShalllist_create_fail = 0;

    currentlist_add_call = 0;
    whenShalllist_add_fail = 0;

    currentLock_call = 0;
    whenShallLock_fail = 0;

    currentUnlock_call = 0;

    currentCond_Init_call = 0;
    whenShallCond_Init_fail = 0;

    currentCond_Post_call = 0;
    whenShallCond_Post_fail = 0;

    currentThreadAPI_Create_call = 0;
    whenShallThreadAPI_Create_fail = 0;

    current_list_index = 0;
    for (int l = 0; l < 10; l++)
    {
        fake_list[l] = NULL;
    }

    shouldThreadAPI_Create_invoke_callback = false;
    thread_func_to_call = NULL;
    thread_func_args = NULL;

    shouldIntercept_Condition_Wait = false;
    interceptArgs_for_Condition_Wait = NULL;
    intercept_for_Condition_Wait = NULL;

    call_status_for_FakeModule_Receive.messageHandle = NULL;
    call_status_for_FakeModule_Receive.was_called = false;
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    if (!MicroMockReleaseMutex(g_testByTest))
    {
        ASSERT_FAIL("failure in test framework at ReleaseMutex");
    }
}

//Tests_SRS_MESSAGE_BUS_99_015: [If `module_instance` is `NULL` the function shall return `MESSAGE_BUS_INVALIDARG`.]
TEST_FUNCTION(MessageBus_AddModule_fails_with_null_bus)
{
    ///arrange
    CMessageBusMocks mocks;

    MODULE fake_module {
        NULL
    };
    
    ///act
    auto result = MessageBus_AddModule(NULL, &fake_module);

    ///assert
    ASSERT_ARE_EQUAL(MESSAGE_BUS_RESULT, result, MESSAGE_BUS_INVALIDARG);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
}

struct Condition_Wait_Callback_Input
{
    MESSAGE_BUS_HANDLE bus;
    MESSAGE_HANDLE message;
};

static COND_RESULT module_publish_worker_calls_module_receive_Condition_Wait2(void)
{
    Condition_Wait_Callback_Input* input = (Condition_Wait_Callback_Input*)interceptArgs_for_Condition_Wait;

    // cause the worker to quit during the next iteration;
    // first we get the modules vector from the bus handle

    // We added one module for the test, that's our fake list...
    unsigned char* module_info = (unsigned char*)fake_list[0];
    sig_atomic_t* quit_worker = (sig_atomic_t*)(module_info + BUS_offsetof_quit_worker);
    *quit_worker = 1;

    return COND_OK;
}

static COND_RESULT module_publish_worker_calls_module_receive_Condition_Wait(void)
{
    // publish a message on to the bus
    Condition_Wait_Callback_Input* input = (Condition_Wait_Callback_Input*)interceptArgs_for_Condition_Wait;
    auto result = MessageBus_Publish(input->bus, NULL, input->message);
    ASSERT_ARE_EQUAL(MESSAGE_BUS_RESULT, MESSAGE_BUS_OK, result);

    // schedule module_publish_worker_calls_module_receive_Condition_Wait2 to be
    // called during the next intercept of the Condition_Wait mock
    intercept_for_Condition_Wait = module_publish_worker_calls_module_receive_Condition_Wait2;

    return COND_OK;
}

// Tests_SRS_MESSAGE_BUS_13_089: [ This function shall acquire the lock on module_info->mq_lock. ]
// Tests_SRS_MESSAGE_BUS_13_068: [ This function shall run a loop that keeps running while module_info->quit_worker is equal to 0. ]
// Tests_SRS_MESSAGE_BUS_13_071: [ For every iteration of the loop the function will first wait on module_info->mq_cond using module_info->mq_lock as the corresponding mutex to be used by the condition variable. ]
// Tests_SRS_MESSAGE_BUS_13_090: [ When module_info->mq_cond has been signaled this function shall kick off another loop predicated on module_info->quit_worker being equal to 0 and module_info->mq not being empty. This thread has the lock on module_info->mq_lock at this point. ]
// Tests_SRS_MESSAGE_BUS_13_069: [ The function shall dequeue a message from the module's message queue. ]
// Tests_SRS_MESSAGE_BUS_13_091: [ The function shall unlock module_info->mq_lock. ]
// Tests_SRS_MESSAGE_BUS_99_012: [ The function shall deliver the message to the module's Receive function via the `IInternalGatewayModule` interface. ]
// Tests_SRS_MESSAGE_BUS_13_093: [ The function shall destroy the message that was dequeued by calling Message_Destroy. ]
// Tests_SRS_MESSAGE_BUS_13_094: [ The function shall re-acquire the lock on module_info->mq_lock. ]
// Tests_SRS_MESSAGE_BUS_13_095: [ When the function exits the outer loop predicated on module_info->quit_worker being 0 it shall unlock module_info->mq_lock before exiting from the function. ]
// Tests_SRS_MESSAGE_BUS_13_026: [ This function shall assign user_data to a local variable called module_info of type MESSAGE_BUS_MODULEINFO*. ]
// Tests_SRS_MESSAGE_BUS_04_001: [** This function shall immediately start processing messages when `module->mq` is not empty without waiting on `module->mq_cond`.]
TEST_FUNCTION(module_publish_worker_calls_module_receive)
{
    /**
    * This test warrants some documentation as to how it works because
    * we are going to great lengths here to test threaded code without
    * using threads. Here's what happens:
    *
    *  [1] The `ThreadAPI_Create` mock will directly call the callback
    *      function when `shouldThreadAPI_Create_invoke_callback` is `true`.
    *      We set this variable to `true` in this test.
    *
    *  [2] We call `MessageBus_AddModule` in this test. That function will eventually
    *      call `ThreadAPI_Create`. `ThreadAPI_Create` will call the
    *      `module_publish_worker` function which, among other things, will
    *      call `Condition_Wait` which we have also mocked.
    *
    *  [3] When we receive the call to the mocked version of `Condition_Wait`
    *      from `module_publish_worker` we have it invoke a function pointed at
    *      by the variable `intercept_for_Condition_Wait` when `shouldIntercept_Condition_Wait`
    *      is `true`. We initialize it to true and assign
    *      `module_publish_worker_calls_module_receive_Condition_Wait` to `intercept_for_Condition_Wait`.
    *
    *  [4] Here's what the call stack looks like right now:
    *
    *          MessageBus_AddModule -> ThreadAPI_Create -> module_publish_worker ->
    *          Condition_Wait -> module_publish_worker_calls_module_receive_Condition_Wait
    *
    *  [5] `module_publish_worker_calls_module_receive_Condition_Wait` uses the data pointed at by
    *      the global variable `interceptArgs_for_Condition_Wait` to get a handle to the
    *      message bus and the message to be published on the message bus. We assign a pointer to a local
    *      variable of type `Condition_Wait_Callback_Input` in the test case
    *      (i.e. `module_publish_worker_calls_module_receive`) to 'interceptArgs_for_Condition_Wait'.
    *
    *  [6] `module_publish_worker_calls_module_receive_Condition_Wait` publishes a message on to
    *      the message bus via `MessageBus_Publish` and replaces `intercept_for_Condition_Wait` so that it
    *      points to the function `module_publish_worker_calls_module_receive_Condition_Wait2`.
    *
    *  [7] Control then goes back to `module_publish_worker` which proceeds to consume the newly
    *      enqueued message and delivers it to the fake module. `FakeModule_Receive` gets called
    *      where we verify that the module handle and the message handle have expected values.
    *
    *  [8] `module_publish_worker` then loops around and calls `Condition_Wait` again. And this
    *      time the `module_publish_worker_calls_module_receive_Condition_Wait2` function ends up
    *      being called. In this function our goal is to assign `1` to the `quit_worker`
    *      variable in the module's `MESSAGE_BUS_MODULEINFO` struct. Since we don't have access to the
    *      structs backing the message bus we use the global variables `MESSAGE_BUS_offsetof_modules` and
    *      `MESSAGE_BUS_offsetof_quit_worker` provided by the message bus API to figure out the location where
    *      the `quit_worker` variable resides in memory and assign `1` to it.
    *
    *  [9] Since `quit_worker` is now `1`, `module_publish_worker` will exit its processing loop
    *      and the entire stack will unwind and control comes back to the test case when
    *      `MessageBus_AddModule` returns.
    */

    ///arrange
    CMessageBusMocks mocks;
    auto bus = MessageBus_Create();

    // make ThreadAPI_Create mock call the callback function
    shouldThreadAPI_Create_invoke_callback = true;

    // we want to intercept Condition_Wait when it is called
    shouldIntercept_Condition_Wait = true;
    Condition_Wait_Callback_Input input{ bus, NULL };
    interceptArgs_for_Condition_Wait = (void*)&input;
    intercept_for_Condition_Wait = module_publish_worker_calls_module_receive_Condition_Wait;

    // setup fake module's validation data
    unsigned char fake;
    MESSAGE_CONFIG c = { 1, &fake, (MAP_HANDLE)&fake };
    auto message = Message_Create(&c);
    input.message = message;
    call_status_for_FakeModule_Receive.messageHandle = message;

    mocks.ResetAllCalls();

    // this is for MessageBus_Publish
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, list_get_head_item(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, list_get_next_item(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, list_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Message_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Condition_Post(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // this is for the MessageBus_AddModule call
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module_info*/
        .IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module*/
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, list_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, Lock_Init());
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Condition_Init());
    STRICT_EXPECTED_CALL(mocks, ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    // this is for module_publish_worker
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG)) 
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Condition_Wait(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, Condition_Wait(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Message_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);


    ///act
    auto result = MessageBus_AddModule(bus, &fake_module);

    ///assert
    ASSERT_ARE_EQUAL(MESSAGE_BUS_RESULT, result, MESSAGE_BUS_OK);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Message_Destroy(message);
    MessageBus_RemoveModule(bus, &fake_module);
    MessageBus_Destroy(bus);
}

END_TEST_SUITE(message_bus_uwp_unittests)
