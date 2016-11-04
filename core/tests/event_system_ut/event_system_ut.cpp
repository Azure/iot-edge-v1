// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <vector>
#include <list>

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/condition.h"
#include "azure_c_shared_utility/singlylinkedlist.h"

#include "experimental/event_system.h"

#define GBALLOC_H

extern "C" int gballoc_init(void);
extern "C" void gballoc_deinit(void);
extern "C" void* gballoc_malloc(size_t size);
extern "C" void* gballoc_calloc(size_t nmemb, size_t size);
extern "C" void* gballoc_realloc(void* ptr, size_t size);
extern "C" void gballoc_free(void* ptr);

namespace BASEIMPLEMENTATION
{
    /*if malloc is defined as gballoc_malloc at this moment, there'd be serious trouble*/

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

static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;
static MICROMOCK_MUTEX_HANDLE g_testByTest;

static std::vector<GATEWAY_HANDLE> *callback_gw_history;
static int callback_per_event_count[GATEWAY_EVENTS_COUNT];
static int helper_counter;

static THREAD_START_FUNC last_thread_func;
static void* last_thread_arg;
static int last_thread_result;

static void* last_context;
static void* last_user_param;

static VECTOR_HANDLE module_list;

struct ListNode
{
    const void* item;
    ListNode *next, *prev;
};

TYPED_MOCK_CLASS(CEventSystemMocks, CGlobalMock)
{
public:
    MOCK_STATIC_METHOD_1(, VECTOR_HANDLE, VECTOR_create, size_t, elementSize)
    MOCK_METHOD_END(VECTOR_HANDLE, BASEIMPLEMENTATION::VECTOR_create(elementSize));

    MOCK_STATIC_METHOD_1(, void, VECTOR_destroy, VECTOR_HANDLE, handle)
        BASEIMPLEMENTATION::VECTOR_destroy(handle);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_3(, int, VECTOR_push_back, VECTOR_HANDLE, handle, const void*, elements, size_t, numElements)
    MOCK_METHOD_END(int, BASEIMPLEMENTATION::VECTOR_push_back(handle, elements, numElements));

    MOCK_STATIC_METHOD_3(, void, VECTOR_erase, VECTOR_HANDLE, handle, void*, elements, size_t, index)
        BASEIMPLEMENTATION::VECTOR_erase(handle, elements, index);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_2(, void*, VECTOR_element, const VECTOR_HANDLE, handle, size_t, index)
    MOCK_METHOD_END(void*, BASEIMPLEMENTATION::VECTOR_element(handle, index));

    MOCK_STATIC_METHOD_1(, void*, VECTOR_front, const VECTOR_HANDLE, handle)
    MOCK_METHOD_END(void*, BASEIMPLEMENTATION::VECTOR_front(handle));

    MOCK_STATIC_METHOD_1(, size_t, VECTOR_size, const VECTOR_HANDLE, handle)
    MOCK_METHOD_END(size_t, BASEIMPLEMENTATION::VECTOR_size(handle));

    MOCK_STATIC_METHOD_3(, void*, VECTOR_find_if, const VECTOR_HANDLE, handle, PREDICATE_FUNCTION, pred, const void*, value)
    MOCK_METHOD_END(void*, BASEIMPLEMENTATION::VECTOR_find_if(handle, pred, value));


    MOCK_STATIC_METHOD_0(, SINGLYLINKEDLIST_HANDLE, singlylinkedlist_create)
    MOCK_METHOD_END(SINGLYLINKEDLIST_HANDLE, (SINGLYLINKEDLIST_HANDLE)new ListNode());

    MOCK_STATIC_METHOD_1(, void, singlylinkedlist_destroy, SINGLYLINKEDLIST_HANDLE, list)
        ListNode *current = (ListNode*)list;
    while (current != nullptr)
    {
        auto tmp = current;
        current = current->next;
        delete tmp;
    }
    MOCK_VOID_METHOD_END()


    MOCK_STATIC_METHOD_2(, LIST_ITEM_HANDLE, singlylinkedlist_add, SINGLYLINKEDLIST_HANDLE, list, const void*, item)
        ListNode *new_item = new ListNode();
        new_item->item = item;
        ListNode *real_list = (ListNode*)list;
        ListNode *ptr = real_list;
        while (ptr->next != nullptr)
            ptr = ptr->next;
        ptr->next = new_item;
        new_item->prev = ptr;
    MOCK_METHOD_END(LIST_ITEM_HANDLE, (LIST_ITEM_HANDLE)new_item)

    MOCK_STATIC_METHOD_2(, int, singlylinkedlist_remove, SINGLYLINKEDLIST_HANDLE, list, LIST_ITEM_HANDLE, item)
        ListNode *node = (ListNode*)item;
        node->prev->next = node->next;
        if (node->next != nullptr)
            node->next->prev = node->prev;
        delete node;
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_1(, LIST_ITEM_HANDLE, singlylinkedlist_get_head_item, SINGLYLINKEDLIST_HANDLE, list)
        ListNode* my_result = NULL;
        if (list != NULL)
            my_result = ((ListNode*)list)->next;
    MOCK_METHOD_END(LIST_ITEM_HANDLE, (LIST_ITEM_HANDLE)my_result);

    MOCK_STATIC_METHOD_1(, LIST_ITEM_HANDLE, singlylinkedlist_get_next_item, LIST_ITEM_HANDLE, item_handle)
    MOCK_METHOD_END(LIST_ITEM_HANDLE, (LIST_ITEM_HANDLE)(((ListNode*)item_handle)->next));

    MOCK_STATIC_METHOD_1(, const void*, singlylinkedlist_item_get_value, LIST_ITEM_HANDLE, item_handle)
    MOCK_METHOD_END(const void*, ((ListNode*)item_handle)->item);


    MOCK_STATIC_METHOD_1(, void*, gballoc_malloc, size_t, size)
    MOCK_METHOD_END(void*, BASEIMPLEMENTATION::gballoc_malloc(size));

    MOCK_STATIC_METHOD_2(, void*, gballoc_realloc, void*, ptr, size_t, size)
    MOCK_METHOD_END(void*, BASEIMPLEMENTATION::gballoc_realloc(ptr, size));

    MOCK_STATIC_METHOD_1(, void, gballoc_free, void*, ptr)
        BASEIMPLEMENTATION::gballoc_free(ptr);
    MOCK_VOID_METHOD_END();


    MOCK_STATIC_METHOD_0(, LOCK_HANDLE, Lock_Init)
    MOCK_METHOD_END(LOCK_HANDLE, (LOCK_HANDLE)0x42);

    MOCK_STATIC_METHOD_1(, LOCK_RESULT, Lock, LOCK_HANDLE, handle)
    MOCK_METHOD_END(LOCK_RESULT, LOCK_OK);

    MOCK_STATIC_METHOD_1(, LOCK_RESULT, Unlock, LOCK_HANDLE, handle);
    MOCK_METHOD_END(LOCK_RESULT, LOCK_OK);

    MOCK_STATIC_METHOD_1(, LOCK_RESULT, Lock_Deinit, LOCK_HANDLE, handle)
    MOCK_METHOD_END(LOCK_RESULT, LOCK_OK);


    MOCK_STATIC_METHOD_3(, THREADAPI_RESULT, ThreadAPI_Create, THREAD_HANDLE*, threadHandle, THREAD_START_FUNC, func, void*, arg);
        last_thread_func = func;
        last_thread_arg = arg;
        (*threadHandle) = (THREAD_HANDLE*)BASEIMPLEMENTATION::gballoc_malloc(1);
    MOCK_METHOD_END(THREADAPI_RESULT, THREADAPI_OK);

    MOCK_STATIC_METHOD_2(, THREADAPI_RESULT, ThreadAPI_Join, THREAD_HANDLE, threadHandle, int*, res);
        (*res) = last_thread_result;
        BASEIMPLEMENTATION::gballoc_free(threadHandle);
    MOCK_METHOD_END(THREADAPI_RESULT, THREADAPI_OK);

    MOCK_STATIC_METHOD_1(, void, ThreadAPI_Exit, int, res)
        last_thread_result = res;
    MOCK_VOID_METHOD_END();


    MOCK_STATIC_METHOD_0(, COND_HANDLE, Condition_Init);
    MOCK_METHOD_END(COND_HANDLE, BASEIMPLEMENTATION::gballoc_malloc(1));

    MOCK_STATIC_METHOD_1(, COND_RESULT, Condition_Post, COND_HANDLE, handle);
    MOCK_METHOD_END(COND_RESULT, COND_OK);

    MOCK_STATIC_METHOD_3(, COND_RESULT, Condition_Wait, COND_HANDLE, handle, LOCK_HANDLE, lock, int, timeout_milliseconds);
    MOCK_METHOD_END(COND_RESULT, COND_OK);

    MOCK_STATIC_METHOD_1(, void, Condition_Deinit, COND_HANDLE, handle);
        BASEIMPLEMENTATION::gballoc_free(handle);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_1(, VECTOR_HANDLE, Gateway_GetModuleList, GATEWAY_HANDLE, gw);
    MOCK_METHOD_END(VECTOR_HANDLE, module_list);

    MOCK_STATIC_METHOD_1(, void, Gateway_DestroyModuleList, VECTOR_HANDLE, vec);
    MOCK_VOID_METHOD_END();
        
};

DECLARE_GLOBAL_MOCK_METHOD_1(CEventSystemMocks, , VECTOR_HANDLE, VECTOR_create, size_t, elementSize);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventSystemMocks, , void, VECTOR_destroy, VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_3(CEventSystemMocks, , int, VECTOR_push_back, VECTOR_HANDLE, handle, const void*, elements, size_t, numElements);
DECLARE_GLOBAL_MOCK_METHOD_3(CEventSystemMocks, , void, VECTOR_erase, VECTOR_HANDLE, handle, void*, elements, size_t, index);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventSystemMocks, , void*, VECTOR_element, const VECTOR_HANDLE, handle, size_t, index);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventSystemMocks, , void*, VECTOR_front, const VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventSystemMocks, , size_t, VECTOR_size, const VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_3(CEventSystemMocks, , void*, VECTOR_find_if, const VECTOR_HANDLE, handle, PREDICATE_FUNCTION, pred, const void*, value);

// singlylinkedlist.h
DECLARE_GLOBAL_MOCK_METHOD_0(CEventSystemMocks, , SINGLYLINKEDLIST_HANDLE, singlylinkedlist_create);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventSystemMocks, , void, singlylinkedlist_destroy, SINGLYLINKEDLIST_HANDLE, list);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventSystemMocks, , LIST_ITEM_HANDLE, singlylinkedlist_add, SINGLYLINKEDLIST_HANDLE, list, const void*, item);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventSystemMocks, , int, singlylinkedlist_remove, SINGLYLINKEDLIST_HANDLE, list, LIST_ITEM_HANDLE, item_handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventSystemMocks, , LIST_ITEM_HANDLE, singlylinkedlist_get_head_item, SINGLYLINKEDLIST_HANDLE, list);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventSystemMocks, , LIST_ITEM_HANDLE, singlylinkedlist_get_next_item, LIST_ITEM_HANDLE, item_handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventSystemMocks, , const void*, singlylinkedlist_item_get_value, LIST_ITEM_HANDLE, item_handle);

DECLARE_GLOBAL_MOCK_METHOD_1(CEventSystemMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventSystemMocks, , void*, gballoc_realloc, void*, ptr, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventSystemMocks, , void, gballoc_free, void*, ptr)

DECLARE_GLOBAL_MOCK_METHOD_0(CEventSystemMocks, , LOCK_HANDLE, Lock_Init)
DECLARE_GLOBAL_MOCK_METHOD_1(CEventSystemMocks, , LOCK_RESULT, Lock, LOCK_HANDLE, handle)
DECLARE_GLOBAL_MOCK_METHOD_1(CEventSystemMocks, , LOCK_RESULT, Unlock, LOCK_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventSystemMocks, , LOCK_RESULT, Lock_Deinit, LOCK_HANDLE, handle)

DECLARE_GLOBAL_MOCK_METHOD_3(CEventSystemMocks, , THREADAPI_RESULT, ThreadAPI_Create, THREAD_HANDLE*, threadHandle, THREAD_START_FUNC, func, void*, arg);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventSystemMocks, , THREADAPI_RESULT, ThreadAPI_Join, THREAD_HANDLE, threadHandle, int*, res);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventSystemMocks, , void, ThreadAPI_Exit, int, res)

DECLARE_GLOBAL_MOCK_METHOD_0(CEventSystemMocks, , COND_HANDLE, Condition_Init);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventSystemMocks, , COND_RESULT, Condition_Post, COND_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_3(CEventSystemMocks, , COND_RESULT, Condition_Wait, COND_HANDLE, handle, LOCK_HANDLE, lock, int, timeout_milliseconds);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventSystemMocks, , void, Condition_Deinit, COND_HANDLE, handle);

DECLARE_GLOBAL_MOCK_METHOD_1(CEventSystemMocks, , VECTOR_HANDLE, Gateway_GetModuleList, GATEWAY_HANDLE, gw);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventSystemMocks, , void, Gateway_DestroyModuleList, VECTOR_HANDLE, vec);

static void expectEventSystemDestroy(CEventSystemMocks &mocks, bool started_thread, int nodes_in_queue)
{
    EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, Condition_Post(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    if (started_thread)
    {
        EXPECTED_CALL(mocks, ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    EXPECTED_CALL(mocks, Condition_Deinit(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(2);
    
    EXPECTED_CALL(mocks, singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(nodes_in_queue + 1);
    EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(nodes_in_queue);
    EXPECTED_CALL(mocks, singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .ExpectedTimesExactly(nodes_in_queue);

    EXPECTED_CALL(mocks, singlylinkedlist_destroy(IGNORED_PTR_ARG));

    EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(GATEWAY_EVENTS_COUNT + nodes_in_queue);
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(nodes_in_queue + 1);
}

static void countingCallback(GATEWAY_HANDLE gw, GATEWAY_EVENT event_type, GATEWAY_EVENT_CTX ctx, void* user_param)
{
    ASSERT_IS_TRUE(event_type < GATEWAY_EVENTS_COUNT);
    callback_gw_history->push_back(gw);
    callback_per_event_count[event_type]++;
}

static void catch_context_callback(GATEWAY_HANDLE gw, GATEWAY_EVENT event_type, GATEWAY_EVENT_CTX ctx, void* user_param)
{
    last_context = ctx;
    last_user_param = user_param;
}

BEGIN_TEST_SUITE(event_system_ut)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    g_testByTest = MicroMockCreateMutex();
    ASSERT_IS_NOT_NULL(g_testByTest);

    callback_gw_history = new std::vector<GATEWAY_HANDLE>;
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{

    delete callback_gw_history;

    MicroMockDestroyMutex(g_testByTest);
    TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (!MicroMockAcquireMutex(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    callback_gw_history->clear();
    for (int i = 0; i < GATEWAY_EVENTS_COUNT; i++)
        callback_per_event_count[i] = 0;
    helper_counter = 0;
    last_thread_result = THREADAPI_ERROR;
    last_thread_arg = NULL;
    last_thread_func = NULL;
    module_list = NULL;
    last_context = NULL;
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    if (!MicroMockReleaseMutex(g_testByTest))
    {
        ASSERT_FAIL("failure in test framework at ReleaseMutex");
    }
}

/* Tests_SRS_EVENTSYSTEM_26_001: [ This function shall create EVENTSYSTEM_HANDLE representing the created event system. ] */
TEST_FUNCTION(EventSystem_Init_Basic)
{
    // Arrange
    CEventSystemMocks mocks;

    // Expectations
    EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, Lock_Init())
        .ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, Condition_Init());
    EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .ExpectedTimesExactly(GATEWAY_EVENTS_COUNT);
    EXPECTED_CALL(mocks, singlylinkedlist_create());

    // Act
    EVENTSYSTEM_HANDLE event_system = EventSystem_Init();

    // Assert
    mocks.AssertActualAndExpectedCalls();

    // Cleanup
    EventSystem_Destroy(event_system);
}

/* Tests_SRS_EVENTSYSTEM_26_002: [ This function shall return NULL upon any internal error during event system creation. ] */
TEST_FUNCTION(EventSystem_Init_Fail_Malloc)
{
    // Arrange
    CEventSystemMocks mocks;

    // Expectations
    EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .SetFailReturn((void*)NULL);

    // Act
    EVENTSYSTEM_HANDLE event_system = EventSystem_Init();

    // Assert
    ASSERT_IS_NULL(event_system);
    mocks.AssertActualAndExpectedCalls();
}

/* Tests_SRS_EVENTSYSTEM_26_002: [ This function shall return NULL upon any internal error during event system creation. ] */
TEST_FUNCTION(EventSystem_Init_Fail_Vector)
{
    // Arrange
    CEventSystemMocks mocks;

    // Expectations
    EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, Lock_Init())
        .ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, Condition_Init());
    EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .SetFailReturn((VECTOR_HANDLE)NULL);
    expectEventSystemDestroy(mocks, false, 0);

    // Act
    EVENTSYSTEM_HANDLE event_system = EventSystem_Init();

    // Assert
    ASSERT_IS_NULL(event_system);
    mocks.AssertActualAndExpectedCalls();
}

/* Tests_SRS_EVENTSYSTEM_26_002: [ This function shall return NULL upon any internal error during event system creation. ] */
TEST_FUNCTION(EventSystem_Init_Fail_Lock)
{
    // Arrange
    CEventSystemMocks mocks;

    // Expectations
    EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, Lock_Init())
        .ExpectedTimesExactly(2)
        .SetFailReturn((LOCK_HANDLE)NULL);
    EXPECTED_CALL(mocks, Condition_Init());

    // destroy
    EXPECTED_CALL(mocks, Condition_Deinit(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, singlylinkedlist_destroy(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(GATEWAY_EVENTS_COUNT);
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);

    // Act
    EVENTSYSTEM_HANDLE event_system = EventSystem_Init();

    // Assert
    ASSERT_IS_NULL(event_system);
    mocks.AssertActualAndExpectedCalls();
}

/* Tests_SRS_EVENTSYSTEM_26_002: [ This function shall return NULL upon any internal error during event system creation. ] */
TEST_FUNCTION(EventSystem_Init_Fail_Condition)
{
    // Arrange
    CEventSystemMocks mocks;

    // Expectations
    EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, Lock_Init())
        .ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, Condition_Init())
        .SetFailReturn((COND_HANDLE)NULL);

    // destroy
    EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, Condition_Deinit(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, singlylinkedlist_destroy(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(GATEWAY_EVENTS_COUNT);
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);

    // Act
    EVENTSYSTEM_HANDLE event_system = EventSystem_Init();

    // Assert
    ASSERT_IS_NULL(event_system);
    mocks.AssertActualAndExpectedCalls();
}

/* Tests_SRS_EVENTSYSTEM_26_002: [ This function shall return NULL upon any internal error during event system creation. ] */
TEST_FUNCTION(EventSystem_Init_Fail_List)
{
    // Arrange
    CEventSystemMocks mocks;

    // Expectations
    EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, Lock_Init())
        .ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, Condition_Init());
    EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .ExpectedTimesExactly(GATEWAY_EVENTS_COUNT);
    EXPECTED_CALL(mocks, singlylinkedlist_create())
        .SetFailReturn((SINGLYLINKEDLIST_HANDLE)NULL);

    expectEventSystemDestroy(mocks, false, 0);

    // Act
    EVENTSYSTEM_HANDLE event_system = EventSystem_Init();

    // Assert
    ASSERT_IS_NULL(event_system);
    mocks.AssertActualAndExpectedCalls();
}

/* Tests_SRS_EVENTSYSTEM_26_003: [ This function shall destroy and free resources of the given event system. ] */
TEST_FUNCTION(EventSystem_Destroy_Basic)
{
    // Arrange
    CEventSystemMocks mocks;
    EVENTSYSTEM_HANDLE event_system = EventSystem_Init();
    mocks.ResetAllCalls();

    // Expectations
    expectEventSystemDestroy(mocks, false, 0);

    // Act
    EventSystem_Destroy(event_system);

    // Assert
    mocks.AssertActualAndExpectedCalls();
}

/* Tests_SRS_EVENTSYSTEM_26_004: [ This function shall do nothing when `event_system` parameter is NULL. ] */
TEST_FUNCTION(EventSystem_Destroy_Give_Null)
{
    // Arrange
    CEventSystemMocks mocks;
    
    // Expectations
    // None! The function should be no-op

    // Act
    EventSystem_Destroy(NULL);

    // Assert
    mocks.AssertActualAndExpectedCalls();
}


/* Tests_SRS_EVENTSYSTEM_26_005: [ This function shall wait for all callbacks to finish before returning. ] */
TEST_FUNCTION(EventSystem_Joins_Thread)
{
    // Arrange
    CEventSystemMocks mocks;
    EVENTSYSTEM_HANDLE handle = EventSystem_Init();
    EventSystem_AddEventCallback(handle, GATEWAY_STARTED, countingCallback, NULL);
    EventSystem_AddEventCallback(handle, GATEWAY_STARTED, countingCallback, NULL);
    EventSystem_ReportEvent(handle, NULL, GATEWAY_STARTED);
    mocks.ResetAllCalls();

    // Expectations
    // checks for threadapi_join
    expectEventSystemDestroy(mocks, true, 1);

    // Act
    EventSystem_Destroy(handle);

    // Assert
    mocks.AssertActualAndExpectedCalls();
}

/* Tests_SRS_EVENTSYSTEM_26_003: [ This function shall destroy and free resources of the given event system. ] */
TEST_FUNCTION(EventSystem_Destroy_Clears_Queue)
{
    // Arrange
    CEventSystemMocks mocks;
    EVENTSYSTEM_HANDLE handle = EventSystem_Init();
    EventSystem_AddEventCallback(handle, GATEWAY_STARTED, countingCallback, NULL);
    EventSystem_AddEventCallback(handle, GATEWAY_STARTED, countingCallback, NULL);
    EventSystem_AddEventCallback(handle, GATEWAY_DESTROYED, countingCallback, NULL);
    EventSystem_ReportEvent(handle, NULL, GATEWAY_STARTED);
    EventSystem_ReportEvent(handle, NULL, GATEWAY_DESTROYED);

    mocks.ResetAllCalls();

    // Expectations
    expectEventSystemDestroy(mocks, true, 2);

    // Act
    EventSystem_Destroy(handle);
}

/* Tests_SRS_EVENTSYSTEM_26_006: [ This function shall call all registered callbacks for the given GATEWAY_EVENT. ] */
/* Tests_SRS_EVENTSYSTEM_26_007: [ This function shan't call any callbacks registered for any other GATEWAY_EVENT other than the one given as parameter. ] */
/* Tests_SRS_EVENTSYSTEM_26_008: [ This function shall call all registered callbacks on a seperate thread. ] */
/* Tests_SRS_EVENTSYSTEM_26_010: [ The given GATEWAY_CALLBACK function shall be called with proper GATEWAY_HANDLE and GATEWAY_EVENT as function parameters coresponding to the gateway and the event that occured. ] */
/* Tests_SRS_EVENTSYSTEM_26_011: [ This function shall register given GATEWAY_CALLBACK and call it when given GATEWAY_EVENT event happens inside of the gateway. ] */
TEST_FUNCTION(EventSystem_Report_CallAllRegistered)
{
    // Arrange
    CEventSystemMocks mocks;
    mocks.SetIgnoreUnexpectedCalls(true);
    EVENTSYSTEM_HANDLE event_system = EventSystem_Init();
    GATEWAY_HANDLE gw = (GATEWAY_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1);
    EventSystem_AddEventCallback(event_system, GATEWAY_STARTED, countingCallback, NULL);
    EventSystem_AddEventCallback(event_system, GATEWAY_DESTROYED, countingCallback, NULL);

    // Act
    EventSystem_ReportEvent(event_system, gw, GATEWAY_STARTED);
    EventSystem_ReportEvent(event_system, gw, GATEWAY_DESTROYED);
    EventSystem_ReportEvent(event_system, gw, GATEWAY_STARTED);
    // check that they weren't run in this thread
    ASSERT_IS_TRUE(callback_gw_history->empty());
    // simulate the thread running
    last_thread_func(last_thread_arg);

    // Assert
    ASSERT_ARE_EQUAL(size_t, callback_gw_history->size(), 3);
    for (int i = 0; i < 3; i++)
        ASSERT_IS_TRUE((*callback_gw_history)[i] == gw);
    ASSERT_ARE_EQUAL(int, callback_per_event_count[GATEWAY_STARTED], 2);
    ASSERT_ARE_EQUAL(int, callback_per_event_count[GATEWAY_DESTROYED], 1);

    // Clean-up
    free(gw);
    EventSystem_Destroy(event_system);
}

/* Tests_SRS_EVENTSYSTEM_26_013: [ Should the worker thread ever fail to be created or any internall callbacks fail, failure will be logged and no further callbacks will be called during gateway's lifecycle. ] */
TEST_FUNCTION(EventSystem_Thread_Creation_Fails)
{
    // Arrange
    CEventSystemMocks mocks;
    EVENTSYSTEM_HANDLE handle = EventSystem_Init();
    EventSystem_AddEventCallback(handle, GATEWAY_STARTED, countingCallback, NULL);
    mocks.ResetAllCalls();

    // Expect
    EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(5);
    EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(5);
    EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, Condition_Post(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetFailReturn(THREADAPI_ERROR)
        .ExpectedTimesExactly(1);
    // notice only one thing was added to the queue because we don't try to create thread again later
    EXPECTED_CALL(mocks, Condition_Deinit(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(2);

    EXPECTED_CALL(mocks, singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);

    EXPECTED_CALL(mocks, singlylinkedlist_destroy(IGNORED_PTR_ARG));

    EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(GATEWAY_EVENTS_COUNT + 1);
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(2);

    // Act
    EventSystem_ReportEvent(handle, NULL, GATEWAY_STARTED);
    EventSystem_ReportEvent(handle, NULL, GATEWAY_STARTED);
    EventSystem_Destroy(handle);

    // Assert
    mocks.AssertActualAndExpectedCalls();
}

/* Checks that even if the thread quits after finishing the job, that another thread is created properly */
TEST_FUNCTION(EventSystem_Recreates_Thread_When_Quit)
{
    // Arrange
    CEventSystemMocks mocks;
    EVENTSYSTEM_HANDLE handle = EventSystem_Init();
    EventSystem_AddEventCallback(handle, GATEWAY_STARTED, countingCallback, NULL);
    EventSystem_AddEventCallback(handle, GATEWAY_STARTED, countingCallback, NULL);

    // Act
    ASSERT_IS_NULL((void*)last_thread_func);
    
    EventSystem_ReportEvent(handle, NULL, GATEWAY_STARTED);
    
    ASSERT_IS_NOT_NULL((void*)last_thread_func);
    ASSERT_ARE_EQUAL(int, callback_gw_history->size(), 0);
    last_thread_func(last_thread_arg);
    ASSERT_ARE_EQUAL(int, callback_gw_history->size(), 2);
    last_thread_func = NULL;
    
    EventSystem_ReportEvent(handle, NULL, GATEWAY_STARTED);
    
    ASSERT_IS_NOT_NULL((void*)last_thread_func);
    last_thread_func(last_thread_arg);
    ASSERT_ARE_EQUAL(int, callback_gw_history->size(), 4);

    mocks.ResetAllCalls();

    // All callbacks should already be consumed
    EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, Condition_Post(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, Condition_Deinit(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(2);

    EXPECTED_CALL(mocks, singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(0);
    EXPECTED_CALL(mocks, singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .ExpectedTimesExactly(0);

    EXPECTED_CALL(mocks, singlylinkedlist_destroy(IGNORED_PTR_ARG));

    EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(GATEWAY_EVENTS_COUNT);
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);

    EventSystem_Destroy(handle);

    mocks.AssertActualAndExpectedCalls();
}

/* Tests_SRS_EVENTSYSTEM_26_009: [ This function shall call all registered callbacks in First-In-First-Out order in terms registration. ] */
TEST_FUNCTION(EventSystem_Report_CallOrder)
{
    // Arrange
    CEventSystemMocks mocks;
    mocks.SetIgnoreUnexpectedCalls(true);
    GATEWAY_HANDLE gw = (GATEWAY_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1);
    EVENTSYSTEM_HANDLE event_system = EventSystem_Init();
    // We can't have capturing lambdas used as function pointers so duplication is necessary
    EventSystem_AddEventCallback(event_system, GATEWAY_STARTED, [](GATEWAY_HANDLE gw, GATEWAY_EVENT event_type, GATEWAY_EVENT_CTX ctx, void* user_param) {
        ASSERT_ARE_EQUAL(int, helper_counter, 0);
        helper_counter++;
    }, NULL);
    EventSystem_AddEventCallback(event_system, GATEWAY_STARTED, [](GATEWAY_HANDLE gw, GATEWAY_EVENT event_type, GATEWAY_EVENT_CTX ctx, void* user_param) {
        ASSERT_ARE_EQUAL(int, helper_counter, 1);
        helper_counter++;
    }, NULL);
    EventSystem_AddEventCallback(event_system, GATEWAY_STARTED, [](GATEWAY_HANDLE gw, GATEWAY_EVENT event_type, GATEWAY_EVENT_CTX ctx, void* user_param) {
        ASSERT_ARE_EQUAL(int, helper_counter, 2);
        helper_counter++;
    }, NULL);

    // Act
    EventSystem_ReportEvent(event_system, gw, GATEWAY_STARTED);
    // simulate the thread
    last_thread_func(last_thread_arg);
    // Cleanup to force multi-threaded callbacks to finish
    free(gw);
    EventSystem_Destroy(event_system);

    // Assert
    ASSERT_ARE_EQUAL(int, helper_counter, 3);

    // Cleanup
    mocks.ResetAllCalls();
}

/* Tests_SRS_EVENTSYSTEM_26_014: [ This function shall do nothing when `event_system` parameter is NULL. ] */
TEST_FUNCTION(EventSystem_Report_NULL_EventSystem)
{
    // Arrange
    CEventSystemMocks mocks;
    GATEWAY_HANDLE gw = (GATEWAY_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1);
    EVENTSYSTEM_HANDLE event_system = EventSystem_Init();
    EventSystem_AddEventCallback(event_system, GATEWAY_DESTROYED, countingCallback, NULL);
    mocks.ResetAllCalls();

    // Expect
    expectEventSystemDestroy(mocks, false, 0);

    // Act
    EventSystem_ReportEvent(NULL, gw, GATEWAY_STARTED);
    // Cleanup to force multi-threaded callbacks to finish
    free(gw);
    EventSystem_Destroy(event_system);

    // Assert
    mocks.AssertActualAndExpectedCalls();
    ASSERT_IS_TRUE(callback_gw_history->empty());
}

/* Tests_SRS_EVENTSYSTEM_26_012: [ This function shall log a failure and do nothing else when either `event_system` or `callback` parameters are NULL. ] */
TEST_FUNCTION(EventSystem_AddEventCallback_NULLs)
{

    // Arrange
    CEventSystemMocks mocks;
    GATEWAY_HANDLE gw = (GATEWAY_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1);
    EVENTSYSTEM_HANDLE event_system = EventSystem_Init();
    mocks.ResetAllCalls();

    // Expect
    EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(4);
    EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(4);
    // Destroy ( + 1 lock/unlock above)
    EXPECTED_CALL(mocks, Condition_Post(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, Condition_Deinit(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, singlylinkedlist_destroy(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(GATEWAY_EVENTS_COUNT);
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

    // Act
    EventSystem_AddEventCallback(NULL, GATEWAY_STARTED, countingCallback, NULL);
    EventSystem_AddEventCallback(event_system, GATEWAY_DESTROYED, NULL, NULL);
    EventSystem_ReportEvent(event_system, gw, GATEWAY_STARTED);
    EventSystem_ReportEvent(event_system, gw, GATEWAY_DESTROYED);
    // Cleanup to force multi-threaded callbacks to finish
    free(gw);
    EventSystem_Destroy(event_system);

    // Assert
    mocks.AssertActualAndExpectedCalls();
    ASSERT_IS_TRUE(callback_gw_history->empty());
}

TEST_FUNCTION(EventSystem_AddEventCallback_Vector_fail)
{
    // Arrange
    CEventSystemMocks mocks;
    EVENTSYSTEM_HANDLE handle = EventSystem_Init();
    // This one will register but fail to be called because of failed AddEventCallback
    EventSystem_AddEventCallback(handle, GATEWAY_STARTED, countingCallback, NULL);
    mocks.ResetAllCalls();

    // Expect
    EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .SetFailReturn(1);

    // Act
    EventSystem_AddEventCallback(handle, GATEWAY_STARTED, countingCallback, NULL);
    EventSystem_ReportEvent(handle, NULL, GATEWAY_STARTED);

    // Assert
    ASSERT_IS_TRUE(callback_gw_history->empty());
    mocks.AssertActualAndExpectedCalls();

    // Cleanup
    EventSystem_Destroy(handle);
}

TEST_FUNCTION(EventSystem_ReportEvent_Vector_Create_Fail)
{
    // Arrange
    CEventSystemMocks mocks;
    EVENTSYSTEM_HANDLE handle = EventSystem_Init();
    EventSystem_AddEventCallback(handle, GATEWAY_STARTED, countingCallback, NULL);
    mocks.ResetAllCalls();

    // Expect
    EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .SetFailReturn((VECTOR_HANDLE)NULL)
        .ExpectedTimesExactly(1);

    // Act
    EventSystem_ReportEvent(handle, NULL, GATEWAY_STARTED);
    EventSystem_ReportEvent(handle, NULL, GATEWAY_STARTED);

    // Assert
    mocks.AssertActualAndExpectedCalls();

    // Cleanup
    EventSystem_Destroy(handle);
}

TEST_FUNCTION(EventSystem_ReportEvent_Vector_Pushback_Fail)
{
    // Arrange
    CEventSystemMocks mocks;
    EVENTSYSTEM_HANDLE handle = EventSystem_Init();
    EventSystem_AddEventCallback(handle, GATEWAY_STARTED, countingCallback, NULL);
    mocks.ResetAllCalls();

    // Expect
    EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .SetFailReturn(1)
        .ExpectedTimesExactly(1);

    // Act
    EventSystem_ReportEvent(handle, NULL, GATEWAY_STARTED);
    EventSystem_ReportEvent(handle, NULL, GATEWAY_STARTED);

    // Assert
    mocks.AssertActualAndExpectedCalls();

    // Cleanup
    EventSystem_Destroy(handle);
}

TEST_FUNCTION(EventSystem_ReportEvent_malloc_fail)
{
    // Arrange
    CEventSystemMocks mocks;
    EVENTSYSTEM_HANDLE handle = EventSystem_Init();
    EventSystem_AddEventCallback(handle, GATEWAY_STARTED, countingCallback, NULL);
    mocks.ResetAllCalls();

    // Expect
    EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .SetFailReturn((void*)NULL)
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);

    // Act
    EventSystem_ReportEvent(handle, NULL, GATEWAY_STARTED);
    EventSystem_ReportEvent(handle, NULL, GATEWAY_STARTED);

    // Assert
    mocks.AssertActualAndExpectedCalls();

    // Cleanup
    EventSystem_Destroy(handle);
}

TEST_FUNCTION(EventSystem_ReportEvent_singlylinkedlist_add_fail)
{
    // Arrange
    CEventSystemMocks mocks;
    EVENTSYSTEM_HANDLE handle = EventSystem_Init();
    EventSystem_AddEventCallback(handle, GATEWAY_STARTED, countingCallback, NULL);
    mocks.ResetAllCalls();

    // Expect
    EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(3);
    EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(3);
    EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetFailReturn((LIST_ITEM_HANDLE)NULL);
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, Condition_Post(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);

    // Act
    EventSystem_ReportEvent(handle, NULL, GATEWAY_STARTED);
    EventSystem_ReportEvent(handle, NULL, GATEWAY_STARTED);

    // Assert
    mocks.AssertActualAndExpectedCalls();

    // Cleanup
    EventSystem_Destroy(handle);
}

/* Tests_SRS_EVENTSYSTEM_26_016: [ This event shall provide `VECTOR_HANDLE` as returned from #Gateway_GetModuleList as the event context in callbacks ] */
/* Tests_SRS_EVENTSYSTEM_26_015: [ This event shall clean up the `VECTOR_HANDLE` of #Gateway_GetModuleList after finishing all the callbacks ] */
TEST_FUNCTION(EventSystem_ReportEvent_Modules_Proper_List_Given)
{
    // Arrange
    CEventSystemMocks mocks;
    module_list = BASEIMPLEMENTATION::VECTOR_create(1);
    EVENTSYSTEM_HANDLE handle = EventSystem_Init();
    EventSystem_AddEventCallback(handle, GATEWAY_MODULE_LIST_CHANGED, catch_context_callback, NULL);
    mocks.ResetAllCalls();

    // Expect
    EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(6);
    EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(6);
    EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, Gateway_GetModuleList(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, Condition_Post(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // simulated thread
    EXPECTED_CALL(mocks, singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(3);
    EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, Gateway_DestroyModuleList(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, Condition_Wait(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    // Act
    EventSystem_ReportEvent(handle, NULL, GATEWAY_MODULE_LIST_CHANGED);
    // simulate the thread running
    last_thread_func(last_thread_arg);

    // Assert
    ASSERT_IS_TRUE(module_list == (VECTOR_HANDLE)last_context);
    mocks.AssertActualAndExpectedCalls();

    // Cleanup
    BASEIMPLEMENTATION::VECTOR_destroy(module_list);
    EventSystem_Destroy(handle);
}

TEST_FUNCTION(EventSystem_ReportEvent_Modules_GetModuleList_Fails)
{
    // Arrange
    CEventSystemMocks mocks;
    EVENTSYSTEM_HANDLE handle = EventSystem_Init();
    EventSystem_AddEventCallback(handle, GATEWAY_MODULE_LIST_CHANGED, catch_context_callback, NULL);
    mocks.ResetAllCalls();

    // Expect
    EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, Gateway_GetModuleList(IGNORED_PTR_ARG))
        .SetFailReturn((VECTOR_HANDLE)NULL);
    EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG));

    // Act
    EventSystem_ReportEvent(handle, NULL, GATEWAY_MODULE_LIST_CHANGED);

    // Assert
    mocks.AssertActualAndExpectedCalls();

    // Cleanup
    EventSystem_Destroy(handle);
}

TEST_FUNCTION(EventSystem_ReportEvent_Modules_Pushback_Fails)
{
    // Arrange
    CEventSystemMocks mocks;
    module_list = BASEIMPLEMENTATION::VECTOR_create(1);
    EVENTSYSTEM_HANDLE handle = EventSystem_Init();
    EventSystem_AddEventCallback(handle, GATEWAY_MODULE_LIST_CHANGED, catch_context_callback, NULL);
    mocks.ResetAllCalls();

    // Expect
    EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, Gateway_GetModuleList(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .SetFailReturn(1);
    EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(2);

    // Act
    EventSystem_ReportEvent(handle, NULL, GATEWAY_MODULE_LIST_CHANGED);

    // Assert
    mocks.AssertActualAndExpectedCalls();

    // Cleanup
    EventSystem_Destroy(handle);
}

TEST_FUNCTION(EventSystem_ReportEvent_user_param_is_passed)
{
    // Arrange
    CNiceCallComparer<CEventSystemMocks> mocks;
    module_list = BASEIMPLEMENTATION::VECTOR_create(1);
    EVENTSYSTEM_HANDLE handle = EventSystem_Init();
    EventSystem_AddEventCallback(handle, GATEWAY_MODULE_LIST_CHANGED, catch_context_callback, (void*)0x42);
    mocks.ResetAllCalls();

    // Act
    EventSystem_ReportEvent(handle, NULL, GATEWAY_MODULE_LIST_CHANGED);
    // simulate the thread running
    last_thread_func(last_thread_arg);

    // Assert
    ASSERT_IS_TRUE(last_user_param == (void*)0x42);

    // Cleanup
    BASEIMPLEMENTATION::VECTOR_destroy(module_list);
    EventSystem_Destroy(handle);
}

END_TEST_SUITE(event_system_ut)
