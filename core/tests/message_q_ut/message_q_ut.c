// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#define GATEWAY_EXPORT_H
#define GATEWAY_EXPORT

static bool malloc_will_fail = false;
static size_t malloc_fail_count = 0;
static size_t malloc_count = 0;

void* my_gballoc_malloc(size_t size)
{
    ++malloc_count;

    void* result;
    if (malloc_will_fail == true && malloc_count == malloc_fail_count)
    {
        result = NULL;
    }
    else
    {
        result = malloc(size);
    }

    return result;
}

void my_gballoc_free(void* ptr)
{
    free(ptr);
}

#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umock_c_negative_tests.h"
#include "umocktypes_charptr.h"
#include "umocktypes_bool.h"
#include "umocktypes_stdint.h"

#define ENABLE_MOCKS
#define GATEWAY_EXPORT_H
#define GATEWAY_EXPORT

#include "message.h"
#include "azure_c_shared_utility/doublylinkedlist.h"
#include "azure_c_shared_utility/gballoc.h"

#undef ENABLE_MOCKS

// Well, this was the easiest way to "mock" DList.
void real_DList_InitializeListHead(PDLIST_ENTRY ListHead)
{
	ListHead->Flink = ListHead->Blink = ListHead;
	return;
}

int real_DList_IsListEmpty(const PDLIST_ENTRY ListHead)
{
	return (ListHead->Flink == ListHead);
}

int real_DList_RemoveEntryList(PDLIST_ENTRY Entry)
{
	PDLIST_ENTRY Blink;
	PDLIST_ENTRY Flink;

	Flink = Entry->Flink;
	Blink = Entry->Blink;
	Blink->Flink = Flink;
	Flink->Blink = Blink;
	return (Flink == Blink);
}

PDLIST_ENTRY real_DList_RemoveHeadList(PDLIST_ENTRY ListHead)
{
	PDLIST_ENTRY Flink;
	PDLIST_ENTRY Entry;

	Entry = ListHead->Flink;
	Flink = Entry->Flink;
	ListHead->Flink = Flink;
	Flink->Blink = ListHead;
	return Entry;
}

void real_DList_InsertTailList(PDLIST_ENTRY ListHead, PDLIST_ENTRY Entry)
{
	PDLIST_ENTRY Blink;
	Blink = ListHead->Blink;
	Entry->Flink = ListHead;
	Entry->Blink = Blink;
	Blink->Flink = Entry;
	ListHead->Blink = Entry;
	return;
}


void real_DList_AppendTailList(PDLIST_ENTRY ListHead,PDLIST_ENTRY ListToAppend)
{
	PDLIST_ENTRY ListEnd = ListHead->Blink;
	ListHead->Blink->Flink = ListToAppend;
	ListHead->Blink = ListToAppend->Blink;
	ListToAppend->Blink->Flink = ListHead;
	ListToAppend->Blink = ListEnd;
	return;
}

void real_DList_InsertHeadList(PDLIST_ENTRY listHead, PDLIST_ENTRY entry)
{
	entry->Blink = listHead;
	entry->Flink = listHead->Flink;
	listHead->Flink->Blink = entry;
	listHead->Flink = entry;
}

#include "message_queue.h"
//=============================================================================
//Globals
//=============================================================================

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error");
}

BEGIN_TEST_SUITE(message_q_ut)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
	TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
	g_testByTest = TEST_MUTEX_CREATE();
	ASSERT_IS_NOT_NULL(g_testByTest);

	umock_c_init(on_umock_c_error);
	umocktypes_charptr_register_types();
	umocktypes_stdint_register_types();


	REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_HANDLE, void*);
	REGISTER_UMOCK_ALIAS_TYPE(PDLIST_ENTRY, void *);
	REGISTER_UMOCK_ALIAS_TYPE(const PDLIST_ENTRY, const void*);

	// malloc/free hooks
	REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
	REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

	//doubly linked list hooks
	REGISTER_GLOBAL_MOCK_HOOK(DList_InitializeListHead, real_DList_InitializeListHead);
	REGISTER_GLOBAL_MOCK_HOOK(DList_IsListEmpty, real_DList_IsListEmpty);
	REGISTER_GLOBAL_MOCK_HOOK(DList_InsertTailList, real_DList_InsertTailList);
	REGISTER_GLOBAL_MOCK_HOOK(DList_InsertHeadList, real_DList_InsertHeadList);
	REGISTER_GLOBAL_MOCK_HOOK(DList_AppendTailList, real_DList_AppendTailList);
	REGISTER_GLOBAL_MOCK_HOOK(DList_RemoveEntryList, real_DList_RemoveEntryList);
	REGISTER_GLOBAL_MOCK_HOOK(DList_RemoveHeadList, real_DList_RemoveHeadList);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
	umock_c_deinit();

	TEST_MUTEX_DESTROY(g_testByTest);
	TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
	if (TEST_MUTEX_ACQUIRE(g_testByTest) != 0)
	{
		ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
	}

	umock_c_reset_all_calls();
	malloc_will_fail = false;
	malloc_fail_count = 0;
	malloc_count = 0;
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
	TEST_MUTEX_RELEASE(g_testByTest);
}

/*Tests_SRS_MESSAGE_QUEUE_17_001: [ On a successful call, MESSAGE_QUEUE_create shall return a non-NULL value in MESSAGE_QUEUE_HANDLE. ]*/
TEST_FUNCTION(MESSAGE_QUEUE_create_success)
{
	///arrange
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	///act
	MESSAGE_QUEUE_HANDLE mq = MESSAGE_QUEUE_create();

	///assert
	ASSERT_IS_NOT_NULL(mq);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///ablutions
	MESSAGE_QUEUE_destroy(mq);
}

/*Tests_SRS_MESSAGE_QUEUE_17_003: [ On a failure, MESSAGE_QUEUE_create shall return NULL. ]*/
TEST_FUNCTION(MESSAGE_QUEUE_create_fails_with_alloc_fail)
{
	///arrange
	malloc_will_fail = true;
	malloc_fail_count = 1;
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);

	///act
	MESSAGE_QUEUE_HANDLE mq = MESSAGE_QUEUE_create();

	///assert
	ASSERT_IS_NULL(mq);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///ablutions
	MESSAGE_QUEUE_destroy(mq);
}

/*Tests_SRS_MESSAGE_QUEUE_17_004: [ MESSAGE_QUEUE_destroy shall not perform any actions on a NULL message queue. ]*/
TEST_FUNCTION(MESSAGE_QUEUE_destroy_does_nothing_with_nothing) 
{
	///arrange
	///act
	MESSAGE_QUEUE_destroy(NULL);
	///assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///ablutions
}

/*Tests_SRS_MESSAGE_QUEUE_17_005: [ If the message queue is not empty, MESSAGE_QUEUE_destroy shall destroy all messages in the queue. ]*/
/*Tests_SRS_MESSAGE_QUEUE_17_006: [ MESSAGE_QUEUE_destroy shall free all allocated resources. ]*/
TEST_FUNCTION(MESSAGE_QUEUE_destroy_does_something_with_a_nonempty_queue)
{
	///arrange
	MESSAGE_HANDLE mh = (MESSAGE_HANDLE)(0x42);
	MESSAGE_QUEUE_HANDLE mq = MESSAGE_QUEUE_create();
	MESSAGE_QUEUE_push(mq, mh);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Message_Destroy(mh));
	STRICT_EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	///act
	MESSAGE_QUEUE_destroy(mq);

	///assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///ablutions
}

/*Tests_SRS_MESSAGE_QUEUE_17_006: [ MESSAGE_QUEUE_destroy shall free all allocated resources. ]*/
TEST_FUNCTION(MESSAGE_QUEUE_destroy_does_something_with_an_empty_queue)
{
	///arrange
	MESSAGE_QUEUE_HANDLE mq = MESSAGE_QUEUE_create();
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	///act
	MESSAGE_QUEUE_destroy(mq);

	///assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///ablutions
}

/*Tests_SRS_MESSAGE_QUEUE_17_007: [ MESSAGE_QUEUE_push shall return a non-zero value if handle or element are NULL. ]*/
TEST_FUNCTION(MESSAGE_QUEUE_push_does_nothing_with_null_params)
{
	///arrange
	MESSAGE_QUEUE_HANDLE handle = (MESSAGE_QUEUE_HANDLE)(0x42);
	MESSAGE_HANDLE element = (MESSAGE_HANDLE)0x42;

	///act
	int mp1 = MESSAGE_QUEUE_push(NULL, element);
	int mp2 = MESSAGE_QUEUE_push(handle, NULL);

	///assert
	ASSERT_ARE_NOT_EQUAL(int, 0, mp1);
	ASSERT_ARE_NOT_EQUAL(int, 0, mp2);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///ablutions
}

/*Tests_SRS_MESSAGE_QUEUE_17_008: [ MESSAGE_QUEUE_push shall return zero on success. ]*/
TEST_FUNCTION(MESSAGE_QUEUE_push_success)
{
	///arrange
	MESSAGE_HANDLE element = (MESSAGE_HANDLE)0x42;
	MESSAGE_QUEUE_HANDLE mq = MESSAGE_QUEUE_create();
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(DList_AppendTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	///act
	int mp1 = MESSAGE_QUEUE_push(mq, element);

	///assert
	ASSERT_ARE_EQUAL(int, 0, mp1);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	ASSERT_IS_FALSE(MESSAGE_QUEUE_is_empty(mq));

	///ablutions
	MESSAGE_QUEUE_destroy(mq);
}

/*Tests_SRS_MESSAGE_QUEUE_17_009: [ MESSAGE_QUEUE_push shall return a non-zero value if any system call fails. ]*/
TEST_FUNCTION(MESSAGE_QUEUE_push_alloc_element_fails)
{
	///arrange
	MESSAGE_HANDLE element = (MESSAGE_HANDLE)0x42;
	MESSAGE_QUEUE_HANDLE mq = MESSAGE_QUEUE_create();
	umock_c_reset_all_calls();

	malloc_will_fail = true;
	malloc_fail_count = malloc_count +1;
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);

	///act
	int mp1 = MESSAGE_QUEUE_push(mq, element);

	///assert
	ASSERT_ARE_NOT_EQUAL(int, 0, mp1);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	ASSERT_IS_TRUE(MESSAGE_QUEUE_is_empty(mq));

	///ablutions
	MESSAGE_QUEUE_destroy(mq);
}

/*Tests_SRS_MESSAGE_QUEUE_17_012: [ MESSAGE_QUEUE_pop shall return NULL on a NULL message queue. ]*/
TEST_FUNCTION(MESSAGE_QUEUE_pop_returns_null_with_null)
{
	///arrange
	///act
	MESSAGE_HANDLE mh = MESSAGE_QUEUE_pop(NULL);
	///assert

	ASSERT_IS_NULL(mh);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///ablutions
}

/*Tests_SRS_MESSAGE_QUEUE_17_013: [ MESSAGE_QUEUE_pop shall return NULL on an empty message queue. ]*/
TEST_FUNCTION(MESSAGE_QUEUE_pop_returns_null_on_empty_queue)
{
	///arrange
	MESSAGE_QUEUE_HANDLE mq = MESSAGE_QUEUE_create();
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	///act
	MESSAGE_HANDLE mh1 = MESSAGE_QUEUE_pop(mq);

	///assert
	ASSERT_IS_NULL(mh1);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///ablutions
	MESSAGE_QUEUE_destroy(mq);
}

/*Tests_SRS_MESSAGE_QUEUE_17_014: [ MESSAGE_QUEUE_pop shall remove messages from the queue in a first-in-first-out order. ]*/
/*Tests_SRS_MESSAGE_QUEUE_17_015: [ A successful call to MESSAGE_QUEUE_pop on a queue with one message will cause the message queue to be empty. ]*/
TEST_FUNCTION(MESSAGE_QUEUE_pop_success)
{
	///arrange
	MESSAGE_HANDLE mh = (MESSAGE_HANDLE)(0x42);
	MESSAGE_QUEUE_HANDLE mq = MESSAGE_QUEUE_create();
	MESSAGE_QUEUE_push(mq, mh);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	///act
	MESSAGE_HANDLE mh1 = MESSAGE_QUEUE_pop(mq);

	///assert
	ASSERT_IS_TRUE((mh1 == mh));
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	ASSERT_IS_TRUE(MESSAGE_QUEUE_is_empty(mq));

	///ablutions
	MESSAGE_QUEUE_destroy(mq);
}

/*Tests_SRS_MESSAGE_QUEUE_17_016: [ MESSAGE_QUEUE_is_empty shall return true if handle is NULL. ]*/
TEST_FUNCTION(MESSAGE_QUEUE_is_empty_returns_true_with_null)
{
	///arrange
	///act
	bool is_empty = MESSAGE_QUEUE_is_empty(NULL);
	///assert
	ASSERT_IS_TRUE(is_empty);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///ablutions
}

/*Tests_SRS_MESSAGE_QUEUE_17_002: [ A newly created message queue shall be empty. ]*/
/*Tests_SRS_MESSAGE_QUEUE_17_017: [ MESSAGE_QUEUE_is_empty shall return true if there are no messages on the queue. ]*/
TEST_FUNCTION(MESSAGE_QUEUE_is_empty_returns_true_with_empty_queue)
{
	///arrange
	MESSAGE_QUEUE_HANDLE mq = MESSAGE_QUEUE_create();
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	///act
	bool is_empty = MESSAGE_QUEUE_is_empty(mq);
	///assert
	ASSERT_IS_TRUE(is_empty);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///ablutions
	MESSAGE_QUEUE_destroy(mq);
}

/*Tests_SRS_MESSAGE_QUEUE_17_018: [ MESSAGE_QUEUE_is_empty shall return false if one or more messages have been pushed on the queue. ]*/
TEST_FUNCTION(MESSAGE_QUEUE_is_empty_returns_true_with_non_empty_queue)
{
	///arrange
	MESSAGE_HANDLE mh = (MESSAGE_HANDLE)(0x42);
	MESSAGE_QUEUE_HANDLE mq = MESSAGE_QUEUE_create();
	MESSAGE_QUEUE_push(mq, mh);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	///act
	bool is_empty = MESSAGE_QUEUE_is_empty(mq);
	///assert
	ASSERT_IS_FALSE(is_empty);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///ablutions
	MESSAGE_QUEUE_destroy(mq);
}

/*Tests_SRS_MESSAGE_QUEUE_17_019: [ MESSAGE_QUEUE_front shall return NULL if handle is NULL. ]*/
TEST_FUNCTION(MESSAGE_QUEUE_front_returns_null_with_null)
{
	///arrange
	///act
	MESSAGE_HANDLE mh = MESSAGE_QUEUE_front(NULL);
	///assert
	ASSERT_IS_NULL(mh);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///ablutions
}

/*Tests_SRS_MESSAGE_QUEUE_17_011: [ Messages shall be pushed into the queue in a first-in-first-out order. ]*/
/*Tests_SRS_MESSAGE_QUEUE_17_021: [ On a non-empty queue, MESSAGE_QUEUE_front shall return the first remaining element that was pushed onto the message queue. ]*/
/*Tests_SRS_MESSAGE_QUEUE_17_022: [ The content of the message queue shall not be changed after calling MESSAGE_QUEUE_front. ]*/
TEST_FUNCTION(MESSAGE_QUEUE_front_returns_front_of_queue)
{
	///arrange
	MESSAGE_HANDLE mh1 = (MESSAGE_HANDLE)(0x42);
	MESSAGE_HANDLE mh2 = (MESSAGE_HANDLE)(0x43);
	MESSAGE_QUEUE_HANDLE mq = MESSAGE_QUEUE_create();
	MESSAGE_QUEUE_push(mq, mh1);
	MESSAGE_QUEUE_push(mq, mh2);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(DList_InsertHeadList(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);

	///act
	MESSAGE_HANDLE mh1_front = MESSAGE_QUEUE_front(mq);

	///assert
	ASSERT_IS_TRUE((mh1 == mh1_front));
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	MESSAGE_HANDLE mh1_pop = MESSAGE_QUEUE_pop(mq);
	ASSERT_IS_TRUE((mh1_pop == mh1_front));

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(DList_InsertHeadList(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);

	///act
	MESSAGE_HANDLE mh2_front = MESSAGE_QUEUE_front(mq);

	///assert
	ASSERT_IS_TRUE((mh2 == mh2_front));
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///ablutions
	MESSAGE_QUEUE_destroy(mq);
}

/*Tests_SRS_MESSAGE_QUEUE_17_020: [ MESSAGE_QUEUE_front shall return NULL if the message queue is empty. ]*/
TEST_FUNCTION(MESSAGE_QUEUE_front_returns_null_empty_queue)
{
	///arrange
	MESSAGE_QUEUE_HANDLE mq = MESSAGE_QUEUE_create();
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	///act
	MESSAGE_HANDLE front = MESSAGE_QUEUE_front(mq);

	///assert
	ASSERT_IS_NULL(front);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///ablutions
	MESSAGE_QUEUE_destroy(mq);
}

///arrange
///act
///assert
///ablutions
END_TEST_SUITE(message_q_ut);


