// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <vector>

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"

#include <glib.h>

#include "gio_async_seq.h"
#include "azure_c_shared_utility/lock.h"

DEFINE_MICROMOCK_ENUM_TO_STRING(GIO_ASYNCSEQ_RESULT, GIO_ASYNCSEQ_RESULT_VALUES);

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
};

typedef void (*PFN_DESTROY_NOTIFY)(gpointer data);

// This is a mock implementation of GLIB's pointer array type -
// GPtrArray - using std::vector<gpointer>. The first two members
// below MUST be where they are to match GPtrArray's memory layout.
class CGPtrArray
{
public:
    gpointer* pdata;
    guint len;
    std::vector<gpointer> _pointers;
    PFN_DESTROY_NOTIFY _destroy_notify;

public:
    CGPtrArray() :
        _destroy_notify(NULL),
        len(0),
        pdata(NULL)
    {}

    CGPtrArray(PFN_DESTROY_NOTIFY destroy_notify) :
        _destroy_notify(destroy_notify),
        len(0),
        pdata(NULL)
    {}

    ~CGPtrArray()
    {
        // invoke the destroy notify function for each item
        if (_destroy_notify != NULL)
        {
            for (auto ptr : _pointers)
            {
                _destroy_notify(ptr);
            }
        }
    }

    void add(gpointer data)
    {
        _pointers.push_back(data);
        len = _pointers.size();
        pdata = &(_pointers.front());
    }

    void remove_range(guint index, guint length)
    {
        auto first = _pointers.begin() + index;
        auto last = _pointers.begin() + index + length;

        // invoke the destroy notify function for each item
        // being removed from the vector
        if (_destroy_notify != NULL)
        {
            for (auto it = first; it != last; ++it)
            {
                _destroy_notify(*it);
            }
        }

        _pointers.erase(first, last);
        len = _pointers.size();
        pdata = &(_pointers.front());
    }
};

////////////////////////////////////////////////////////////
// The cached copy of the CGPtrArray object in a given test.
static CGPtrArray* g_cached_ptr_array = NULL;

static GAsyncReadyCallback g_resolve_callback = NULL;

TYPED_MOCK_CLASS(CGIOAsyncSeqMocks, CGlobalMock)
{
public:

    // memory
    MOCK_STATIC_METHOD_1(, void*, gballoc_malloc, size_t, size)
        void* result2 = BASEIMPLEMENTATION::gballoc_malloc(size);
    MOCK_METHOD_END(void*, result2);

    MOCK_STATIC_METHOD_1(, void, gballoc_free, void*, ptr)
        BASEIMPLEMENTATION::gballoc_free(ptr);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, GPtrArray*, g_ptr_array_new_with_free_func, GDestroyNotify, element_free_func)
        GPtrArray* result2 = (GPtrArray*)new CGPtrArray(element_free_func);
        g_cached_ptr_array = (CGPtrArray*)result2;
    MOCK_METHOD_END(GPtrArray*, result2);

    MOCK_STATIC_METHOD_1(, void, g_ptr_array_unref, GPtrArray*, array)
        CGPtrArray* parr = (CGPtrArray*)array;
        delete parr;
        g_cached_ptr_array = NULL;
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_2(, void, g_ptr_array_add, GPtrArray*, array, gpointer, data)
        CGPtrArray* parr = (CGPtrArray*)array;
        parr->add(data);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_3(, GPtrArray*, g_ptr_array_remove_range, GPtrArray*, array, guint, index, guint, length)
        CGPtrArray* parr = (CGPtrArray*)array;
        parr->remove_range(index, length);
    MOCK_METHOD_END(GPtrArray*, array);

    MOCK_STATIC_METHOD_1(, void, g_clear_error, GError**, err)
        if (*err != NULL)
        {
            gchar* msg = (gchar*)*err;
            delete[] msg;
            *err = NULL;
        }
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_3(, GError*, g_error_new_literal, GQuark, domain, gint, code, const gchar*, message)
        gchar* msg = new gchar[strlen(message) + 1];
        strcpy(msg, message);
        GError* result2 = (GError*)msg;
    MOCK_METHOD_END(GError*, result2);

    MOCK_STATIC_METHOD_4(, void, pass_NULL_user_data_start, GIO_ASYNCSEQ_HANDLE, async_seq_handle, gpointer, previous_result, gpointer, callback_context, GAsyncReadyCallback, async_callback)
        async_callback(NULL, NULL, NULL);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_4(, void, save_async_callback_start, GIO_ASYNCSEQ_HANDLE, async_seq_handle, gpointer, previous_result, gpointer, callback_context, GAsyncReadyCallback, async_callback)
        g_resolve_callback = async_callback;
        async_callback(NULL, NULL, (gpointer)async_seq_handle);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_4(, void, simple_forwarder_start, GIO_ASYNCSEQ_HANDLE, async_seq_handle, gpointer, previous_result, gpointer, callback_context, GAsyncReadyCallback, async_callback)
        ASSERT_IS_NOT_NULL(async_callback);
        async_callback(NULL, NULL, async_seq_handle);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_4(, void, add_callbacks_while_running_start, GIO_ASYNCSEQ_HANDLE, async_seq_handle, gpointer, previous_result, gpointer, callback_context, GAsyncReadyCallback, async_callback)
        auto result2 = GIO_Async_Seq_Add(
            async_seq_handle, NULL,
            simple_forwarder_start, simple_forwarder_finish,
            NULL
        );

        ASSERT_ARE_EQUAL(GIO_ASYNCSEQ_RESULT, GIO_ASYNCSEQ_ERROR, result2);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_4(, void, call_run_while_running_start, GIO_ASYNCSEQ_HANDLE, async_seq_handle, gpointer, previous_result, gpointer, callback_context, GAsyncReadyCallback, async_callback)
        auto result2 = GIO_Async_Seq_Run_Async(async_seq_handle);
        ASSERT_ARE_EQUAL(GIO_ASYNCSEQ_RESULT, GIO_ASYNCSEQ_ERROR, result2);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_4(, void, callback_context_checker_start, GIO_ASYNCSEQ_HANDLE, async_seq_handle, gpointer, previous_result, gpointer, callback_context, GAsyncReadyCallback, async_callback)
        ASSERT_ARE_EQUAL(void_ptr, callback_context, (gpointer)0x42);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_4(, void, previous_result_checker_start, GIO_ASYNCSEQ_HANDLE, async_seq_handle, gpointer, previous_result, gpointer, callback_context, GAsyncReadyCallback, async_callback)
        ASSERT_ARE_EQUAL(void_ptr, previous_result, (gpointer)0x42);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_4(, void, previous_result_is_NULL_checker_start, GIO_ASYNCSEQ_HANDLE, async_seq_handle, gpointer, previous_result, gpointer, callback_context, GAsyncReadyCallback, async_callback)
        ASSERT_IS_NULL(previous_result);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_4(, void, another_simple_forwarder_start, GIO_ASYNCSEQ_HANDLE, async_seq_handle, gpointer, previous_result, gpointer, callback_context, GAsyncReadyCallback, async_callback)
        ASSERT_IS_NOT_NULL(async_callback);
        async_callback(NULL, NULL, async_seq_handle);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_2(, void, sequence_complete, GIO_ASYNCSEQ_HANDLE, async_seq_handle, gpointer, previous_result)
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_2(, void, sequence_error, GIO_ASYNCSEQ_HANDLE, async_seq_handle, const GError*, err)
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_3(, gpointer, simple_forwarder_finish, GIO_ASYNCSEQ_HANDLE, async_seq_handle, GAsyncResult*, result_param, GError**, error)
        *error = NULL;
        auto result2 = (gpointer)0x42;
    MOCK_METHOD_END(gpointer, result2)

    MOCK_STATIC_METHOD_3(, gpointer, finish_with_error_finish, GIO_ASYNCSEQ_HANDLE, async_seq_handle, GAsyncResult*, result_param, GError**, error)
        *error = g_error_new_literal(
            0x01, GIO_ASYNCSEQ_ERROR, "Whoops!"
        );
        gpointer result2 = NULL;
    MOCK_METHOD_END(gpointer, result2)
};

DECLARE_GLOBAL_MOCK_METHOD_1(CGIOAsyncSeqMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CGIOAsyncSeqMocks, , void, gballoc_free, void*, ptr);
DECLARE_GLOBAL_MOCK_METHOD_1(CGIOAsyncSeqMocks, , GPtrArray*, g_ptr_array_new_with_free_func, GDestroyNotify, element_free_func);
DECLARE_GLOBAL_MOCK_METHOD_1(CGIOAsyncSeqMocks, , void, g_ptr_array_unref, GPtrArray*, array);
DECLARE_GLOBAL_MOCK_METHOD_2(CGIOAsyncSeqMocks, , void, g_ptr_array_add, GPtrArray*, array, gpointer, data);
DECLARE_GLOBAL_MOCK_METHOD_3(CGIOAsyncSeqMocks, , GPtrArray*, g_ptr_array_remove_range, GPtrArray*, array, guint, index, guint, length);
DECLARE_GLOBAL_MOCK_METHOD_1(CGIOAsyncSeqMocks, , void, g_clear_error, GError**, err);
DECLARE_GLOBAL_MOCK_METHOD_3(CGIOAsyncSeqMocks, , GError*, g_error_new_literal, GQuark, domain, gint, code, const gchar*, message);
DECLARE_GLOBAL_MOCK_METHOD_4(CGIOAsyncSeqMocks, , void, pass_NULL_user_data_start, GIO_ASYNCSEQ_HANDLE, async_seq_handle, gpointer, previous_result, gpointer, callback_context, GAsyncReadyCallback, async_callback);
DECLARE_GLOBAL_MOCK_METHOD_4(CGIOAsyncSeqMocks, , void, save_async_callback_start, GIO_ASYNCSEQ_HANDLE, async_seq_handle, gpointer, previous_result, gpointer, callback_context, GAsyncReadyCallback, async_callback);
DECLARE_GLOBAL_MOCK_METHOD_4(CGIOAsyncSeqMocks, , void, simple_forwarder_start, GIO_ASYNCSEQ_HANDLE, async_seq_handle, gpointer, previous_result, gpointer, callback_context, GAsyncReadyCallback, async_callback);
DECLARE_GLOBAL_MOCK_METHOD_4(CGIOAsyncSeqMocks, , void, add_callbacks_while_running_start, GIO_ASYNCSEQ_HANDLE, async_seq_handle, gpointer, previous_result, gpointer, callback_context, GAsyncReadyCallback, async_callback);
DECLARE_GLOBAL_MOCK_METHOD_4(CGIOAsyncSeqMocks, , void, call_run_while_running_start, GIO_ASYNCSEQ_HANDLE, async_seq_handle, gpointer, previous_result, gpointer, callback_context, GAsyncReadyCallback, async_callback);
DECLARE_GLOBAL_MOCK_METHOD_4(CGIOAsyncSeqMocks, , void, callback_context_checker_start, GIO_ASYNCSEQ_HANDLE, async_seq_handle, gpointer, previous_result, gpointer, callback_context, GAsyncReadyCallback, async_callback);
DECLARE_GLOBAL_MOCK_METHOD_4(CGIOAsyncSeqMocks, , void, previous_result_checker_start, GIO_ASYNCSEQ_HANDLE, async_seq_handle, gpointer, previous_result, gpointer, callback_context, GAsyncReadyCallback, async_callback);
DECLARE_GLOBAL_MOCK_METHOD_4(CGIOAsyncSeqMocks, , void, previous_result_is_NULL_checker_start, GIO_ASYNCSEQ_HANDLE, async_seq_handle, gpointer, previous_result, gpointer, callback_context, GAsyncReadyCallback, async_callback);
DECLARE_GLOBAL_MOCK_METHOD_4(CGIOAsyncSeqMocks, , void, another_simple_forwarder_start, GIO_ASYNCSEQ_HANDLE, async_seq_handle, gpointer, previous_result, gpointer, callback_context, GAsyncReadyCallback, async_callback);
DECLARE_GLOBAL_MOCK_METHOD_2(CGIOAsyncSeqMocks, , void, sequence_complete, GIO_ASYNCSEQ_HANDLE, async_seq_handle, gpointer, previous_result);
DECLARE_GLOBAL_MOCK_METHOD_2(CGIOAsyncSeqMocks, , void, sequence_error, GIO_ASYNCSEQ_HANDLE, async_seq_handle, const GError*, error);
DECLARE_GLOBAL_MOCK_METHOD_3(CGIOAsyncSeqMocks, , gpointer, simple_forwarder_finish, GIO_ASYNCSEQ_HANDLE, async_seq_handle, GAsyncResult*, result, GError**, error);
DECLARE_GLOBAL_MOCK_METHOD_3(CGIOAsyncSeqMocks, , gpointer, finish_with_error_finish, GIO_ASYNCSEQ_HANDLE, async_seq_handle, GAsyncResult*, result, GError**, error);

static GIO_ASYNCSEQ_RESULT call_GIO_Async_Seq_Adv(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    gpointer callback_context,
    ...
)
{
    GIO_ASYNCSEQ_RESULT result;
    va_list args;
    va_start(args, callback_context);
    result = GIO_Async_Seq_Addv(async_seq_handle, callback_context, args);
    va_end(args);

    return result;
}

BEGIN_TEST_SUITE(gio_async_seq_unittests)
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

        // resets all the counters to zero
        g_cached_ptr_array = NULL;
        g_resolve_callback = NULL;
    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
        if (!MicroMockReleaseMutex(g_testByTest))
        {
            ASSERT_FAIL("failure in test framework at ReleaseMutex");
        }
    }
    
    /*Tests_SRS_GIO_ASYNCSEQ_13_002: [GIO_Async_Seq_Create shall return NULL when any of the underlying platform calls fail.]*/
    TEST_FUNCTION(GIO_Async_Seq_Create_fails_when_malloc_fails)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) 
            .IgnoreArgument(1)
            .SetFailReturn((void*)NULL);

        ///act
        auto result = GIO_Async_Seq_Create(NULL, NULL, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_002: [GIO_Async_Seq_Create shall return NULL when any of the underlying platform calls fail.]*/
    TEST_FUNCTION(GIO_Async_Seq_Create_fails_when_g_ptr_array_new_with_free_func_fails)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) 
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_ptr_array_new_with_free_func(gballoc_free))
            .SetFailReturn((GPtrArray*)NULL);

        ///act
        auto result = GIO_Async_Seq_Create(NULL, NULL, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_001: [ GIO_Async_Seq_Create shall return a non-NULL handle on successful execution. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Create_succeeds)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) 
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_ptr_array_new_with_free_func(gballoc_free));

        ///act
        auto result = GIO_Async_Seq_Create(NULL, NULL, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NOT_NULL(result);

        ///cleanup
        GIO_Async_Seq_Destroy(result);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_005: [ GIO_Async_Seq_Create shall save the async_seq_context pointer so that it can be retrieved later by calling GIO_Async_Seq_GetContext. ]*/
    /*Tests_SRS_GIO_ASYNCSEQ_13_007: [ GIO_Async_Seq_GetContext shall return the value of the async_seq_context parameter that was passed in when calling GIO_Async_Seq_Create. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Create_saves_context_pointer)
    {
        ///arrange
        gpointer context = (gpointer)0x42;
        CGIOAsyncSeqMocks mocks;

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) 
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_ptr_array_new_with_free_func(gballoc_free));

        ///act
        auto result = GIO_Async_Seq_Create(context, NULL, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NOT_NULL(result);
        gpointer actual_context = GIO_Async_Seq_GetContext(result);
        ASSERT_ARE_EQUAL(void_ptr, context, actual_context);

        ///cleanup
        GIO_Async_Seq_Destroy(result);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_003: [ GIO_Async_Seq_Destroy shall do nothing if async_seq_handle is NULL. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Destroy_does_nothing_if_handle_is_null)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;

        ///act
        GIO_Async_Seq_Destroy(NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_004: [ GIO_Async_Seq_Destroy shall free all resources associated with the handle. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Destroy_frees_things)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;

        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, g_ptr_array_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        GIO_Async_Seq_Destroy(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_006: [ GIO_Async_Seq_GetContext shall return NULL if async_seq_handle is NULL. ]*/
    TEST_FUNCTION(GIO_Async_Seq_GetContext_returns_NULL_with_NULL_handle)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;

        ///act
        auto result = GIO_Async_Seq_GetContext(NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_008: [ GIO_Async_Seq_Add shall return GIO_ASYNCSEQ_ERROR if async_seq_handle is NULL. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Add_returns_error_for_NULL_handle)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;

        ///act
        auto result = GIO_Async_Seq_Add(NULL, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(GIO_ASYNCSEQ_RESULT, GIO_ASYNCSEQ_ERROR, result);

        ///cleanup
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_009: [ GIO_Async_Seq_Add shall return GIO_ASYNCSEQ_ERROR if the async sequence's state is not equal to GIO_ASYNCSEQ_STATE_PENDING. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Add_returns_error_if_state_is_not_pending)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, add_callbacks_while_running_start(handle, NULL, NULL, IGNORED_PTR_ARG))
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_ptr_array_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        ///act
        GIO_Async_Seq_Add(
            handle, NULL,
            add_callbacks_while_running_start, simple_forwarder_finish,
            NULL
        );
        (void)GIO_Async_Seq_Run_Async(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_010: [ GIO_Async_Seq_Add shall append the new async operations to the end of the existing list of async operations if any. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Add_appends_to_end_of_list)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_ptr_array_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        ///act
        (void)GIO_Async_Seq_Add(
            handle, NULL,
            simple_forwarder_start, simple_forwarder_finish,
            NULL
        );

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_011: [ GIO_Async_Seq_Add shall process callbacks from the variable arguments list till a callback whose value is NULL is encountered. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Add_check_NULL_sentinel)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_ptr_array_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        ///act
        (void)GIO_Async_Seq_Add(
            handle, NULL,
            simple_forwarder_start, simple_forwarder_finish,
            NULL,
            // The following pair SHOULD NOT get added
            simple_forwarder_start, simple_forwarder_finish
        );

        ///assert
        mocks.AssertActualAndExpectedCalls();
        auto actual_size = g_cached_ptr_array->_pointers.size();
        ASSERT_ARE_EQUAL(size_t, 1, actual_size);

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_012: [ When a GIO_ASYNCSEQ_CALLBACK is encountered in the varargs, the next argument MUST be non-NULL. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Add_callback_pairs_must_match)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        mocks.ResetAllCalls();

        ///act
        auto result = GIO_Async_Seq_Add(
            handle, NULL,
            simple_forwarder_start, NULL
        );

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(GIO_ASYNCSEQ_RESULT, GIO_ASYNCSEQ_ERROR, result);

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_013: [ GIO_Async_Seq_Add shall return GIO_ASYNCSEQ_ERROR when any of the underlying platform calls fail. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Add_fails_when_malloc_fails)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((void*)NULL);

        ///act
        auto result = GIO_Async_Seq_Add(
            handle, NULL,
            simple_forwarder_start, simple_forwarder_finish,
            NULL
        );

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(GIO_ASYNCSEQ_RESULT, GIO_ASYNCSEQ_ERROR, result);

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_014: [ The callback_context pointer value shall be saved so that they can be passed to the callbacks later when they are invoked. ]*/
    /*Tests_SRS_GIO_ASYNCSEQ_13_021: [ GIO_Async_Seq_Run shall pass the callback context that was supplied to GIO_Async_Seq_Add or GIO_Async_Seq_Addv when the first operation was added to the sequence when invoking the 'start' callback. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Add_callback_context_is_passed_along)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, callback_context_checker_start(handle, NULL, (gpointer)0x42, IGNORED_PTR_ARG))
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_ptr_array_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        ///act
        (void)GIO_Async_Seq_Add(
            handle, (gpointer)0x42,
            callback_context_checker_start, simple_forwarder_finish,
            NULL
        );
        (void)GIO_Async_Seq_Run_Async(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_015: [ The list of callbacks that had been added to the sequence before calling this API will remain unchanged after the function returns when an error occurs. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Add_is_transactional)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        (void)GIO_Async_Seq_Add(
            handle, NULL,
            simple_forwarder_start, simple_forwarder_finish,
            NULL
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, g_ptr_array_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, g_ptr_array_remove_range(IGNORED_PTR_ARG, 1, 1))
            .IgnoreArgument(1);
        // let things go haywire when the second function pair
        // is being added at which point the internal list should
        // have 2 items - one that was added above and one that
        // was added for the first function pair in this call
        // which should subsequently get rolled back
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((void*)NULL);

        ///act
        auto result = GIO_Async_Seq_Add(
            handle, NULL,
            simple_forwarder_start, simple_forwarder_finish,
            simple_forwarder_start, simple_forwarder_finish,
            NULL
        );

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(GIO_ASYNCSEQ_RESULT, GIO_ASYNCSEQ_ERROR, result);
        ASSERT_ARE_EQUAL(size_t, 1, g_cached_ptr_array->_pointers.size());

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_016: [ GIO_Async_Seq_Add shall return GIO_ASYNCSEQ_OK if the API executes successfully. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Add_succeeds)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_ptr_array_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, g_ptr_array_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        ///act
        auto result = GIO_Async_Seq_Add(
            handle, NULL,
            simple_forwarder_start, simple_forwarder_finish,
            simple_forwarder_start, simple_forwarder_finish,
            NULL
        );

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(GIO_ASYNCSEQ_RESULT, GIO_ASYNCSEQ_OK, result);
        ASSERT_ARE_EQUAL(size_t, 2, g_cached_ptr_array->_pointers.size());

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_017: [ GIO_Async_Seq_Run shall return GIO_ASYNCSEQ_ERROR if async_seq_handle is NULL. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Run_Async_returns_error_with_NULL_handle)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;

        ///act
        auto result = GIO_Async_Seq_Run_Async(NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(GIO_ASYNCSEQ_RESULT, GIO_ASYNCSEQ_ERROR, result);

        ///cleanup
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_018: [ GIO_Async_Seq_Run shall return GIO_ASYNCSEQ_ERROR if the sequence's state is already GIO_ASYNCSEQ_STATE_RUNNING. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Run_Async_returns_error_if_state_is_running)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        (void)GIO_Async_Seq_Add(
            handle, NULL,
            call_run_while_running_start, simple_forwarder_finish,
            NULL
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, call_run_while_running_start(handle, NULL, NULL, IGNORED_PTR_ARG))
            .IgnoreArgument(4);

        ///act
        (void)GIO_Async_Seq_Run_Async(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_019: [ GIO_Async_Seq_Run shall complete the sequence and invoke the sequence's complete callback if there are no asynchronous operations to process. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Run_Async_calls_sequence_complete_callback)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, sequence_complete);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, sequence_complete(handle, NULL));

        ///act
        (void)GIO_Async_Seq_Run_Async(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_020: [ GIO_Async_Seq_Run shall invoke the 'start' callback of the first async operation in the sequence. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Run_Async_calls_first_callback)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        (void)GIO_Async_Seq_Add(
            handle, NULL,
            simple_forwarder_start, simple_forwarder_finish,
            NULL
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, simple_forwarder_start(handle, NULL, NULL, IGNORED_PTR_ARG))
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, simple_forwarder_finish(handle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        ///act
        (void)GIO_Async_Seq_Run_Async(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_031: [ resolve_callback shall supply NULL as the value of the previous_result parameter of the 'start' callback. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Run_Async_first_callback_prev_result_is_null)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        (void)GIO_Async_Seq_Add(
            handle, NULL,
            previous_result_is_NULL_checker_start, simple_forwarder_finish,
            NULL
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, previous_result_is_NULL_checker_start(handle, NULL, NULL, IGNORED_PTR_ARG))
            .IgnoreArgument(4);

        ///act
        (void)GIO_Async_Seq_Run_Async(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_023: [ GIO_Async_Seq_Run shall supply a non-NULL pointer to a function as the value of the async_callback parameter when calling the 'start' callback. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Run_Async_start_callback_gets_non_NULL_async_callback)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        (void)GIO_Async_Seq_Add(
            handle, NULL,
            simple_forwarder_start, simple_forwarder_finish,
            NULL
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, simple_forwarder_start(handle, NULL, NULL, IGNORED_PTR_ARG))
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, simple_forwarder_finish(handle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        ///act
        (void)GIO_Async_Seq_Run_Async(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_022: [ GIO_Async_Seq_Run shall return GIO_ASYNCSEQ_OK when there are no errors. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Run_Async_succeeds)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        (void)GIO_Async_Seq_Add(
            handle, NULL,
            simple_forwarder_start, simple_forwarder_finish,
            simple_forwarder_start, simple_forwarder_finish,
            NULL
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, simple_forwarder_start(handle, NULL, NULL, IGNORED_PTR_ARG))
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, simple_forwarder_start(handle, (gpointer)0x42, NULL, IGNORED_PTR_ARG))
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, simple_forwarder_finish(handle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, simple_forwarder_finish(handle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        ///act
        auto result = GIO_Async_Seq_Run_Async(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(GIO_ASYNCSEQ_RESULT, GIO_ASYNCSEQ_OK, result);

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_024: [ resolve_callback shall do nothing else if user_data is NULL. ]*/
    TEST_FUNCTION(resolve_callback_calls_does_nothing_if_user_data_is_null)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        (void)GIO_Async_Seq_Add(
            handle, NULL,
            pass_NULL_user_data_start, simple_forwarder_finish,
            NULL
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, pass_NULL_user_data_start(handle, NULL, NULL, IGNORED_PTR_ARG))
            .IgnoreArgument(4);

        ///act
        (void)GIO_Async_Seq_Run_Async(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_025: [ resolve_callback shall invoke the sequence's error callback and suspend execution of the sequence if the state of the sequence is not equal to GIO_ASYNCSEQ_STATE_RUNNING. ]*/
    TEST_FUNCTION(resolve_callback_calls_sequence_error_if_not_running)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, sequence_error, NULL);
        (void)GIO_Async_Seq_Add(
            handle, NULL,
            save_async_callback_start, simple_forwarder_finish,
            NULL
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, save_async_callback_start(handle, NULL, NULL, IGNORED_PTR_ARG))
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, simple_forwarder_finish(handle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, sequence_error(handle, IGNORED_PTR_ARG))
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_error_new_literal(IGNORED_NUM_ARG, GIO_ASYNCSEQ_ERROR, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, g_clear_error(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        (void)GIO_Async_Seq_Run_Async(handle);

        // call the resolve callback function once more;
        // this should cause the sequence error handler
        // to get called since the sequence is now complete
        g_resolve_callback(NULL, NULL, handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_026: [ resolve_callback shall invoke the 'finish' callback of the async operation that was just concluded. ]*/
    /*Tests_SRS_GIO_ASYNCSEQ_13_033: [ resolve_callback shall supply a non-NULL pointer to a function as the value of the async_callback parameter when calling the 'start' callback. ]*/
    TEST_FUNCTION(resolve_callback_calls_finish_callback)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        (void)GIO_Async_Seq_Add(
            handle, NULL,
            simple_forwarder_start, simple_forwarder_finish,
            NULL
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, simple_forwarder_start(handle, NULL, NULL, IGNORED_PTR_ARG))
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, simple_forwarder_finish(handle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        ///act
        (void)GIO_Async_Seq_Run_Async(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_027: [ resolve_callback shall invoke the sequence's error callback and suspend execution of the sequence if the 'finish' callback returns a non-NULL GError pointer. ]*/
    /*Tests_SRS_GIO_ASYNCSEQ_13_028: [ resolve_callback shall free the GError object by calling g_clear_error if the 'finish' callback returns a non-NULL GError pointer. ]*/
    TEST_FUNCTION(resolve_callback_calls_sequence_error_when_callback_fails)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, sequence_error, NULL);
        (void)GIO_Async_Seq_Add(
            handle, NULL,
            simple_forwarder_start, finish_with_error_finish,
            NULL
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, g_error_new_literal(IGNORED_NUM_ARG, GIO_ASYNCSEQ_ERROR, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, g_clear_error(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, simple_forwarder_start(handle, NULL, NULL, IGNORED_PTR_ARG))
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, finish_with_error_finish(handle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, sequence_error(handle, IGNORED_PTR_ARG))
            .IgnoreArgument(2);

        ///act
        (void)GIO_Async_Seq_Run_Async(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_029: [ resolve_callback shall invoke the 'start' callback of the next async operation in the sequence. ]*/
    TEST_FUNCTION(resolve_callback_calls_second_callback_start)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        (void)GIO_Async_Seq_Add(
            handle, NULL,
            simple_forwarder_start, simple_forwarder_finish,
            another_simple_forwarder_start, simple_forwarder_finish,
            NULL
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, simple_forwarder_start(handle, NULL, NULL, IGNORED_PTR_ARG))
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, simple_forwarder_finish(handle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, simple_forwarder_finish(handle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, another_simple_forwarder_start(handle, (gpointer)0x42, NULL, IGNORED_PTR_ARG))
            .IgnoreArgument(4);

        ///act
        (void)GIO_Async_Seq_Run_Async(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_030: [ resolve_callback shall supply the result of calling the 'finish' callback of the current async operation as the value of the previous_result parameter of the 'start' callback of the next async operation. ]*/
    TEST_FUNCTION(resolve_callback_passes_result_of_first_callback_to_second_callback)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        (void)GIO_Async_Seq_Add(
            handle, NULL,
            simple_forwarder_start, simple_forwarder_finish,
            previous_result_checker_start, simple_forwarder_finish,
            NULL
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, simple_forwarder_start(handle, NULL, NULL, IGNORED_PTR_ARG))
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, simple_forwarder_finish(handle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, previous_result_checker_start(handle, (gpointer)0x42, NULL, IGNORED_PTR_ARG))
            .IgnoreArgument(4);

        ///act
        (void)GIO_Async_Seq_Run_Async(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_032: [ resolve_callback shall pass the callback context that was supplied to GIO_Async_Seq_Add when the next async operation was added to the sequence when invoking the 'start' callback. ]*/
    TEST_FUNCTION(resolve_callback_context_is_passed_along_for_second_async_operation)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        (void)GIO_Async_Seq_Add(
            handle, NULL,
            simple_forwarder_start, simple_forwarder_finish,
            NULL
        );
        (void)GIO_Async_Seq_Add(
            handle, (gpointer)0x42,
            callback_context_checker_start, simple_forwarder_finish,
            NULL
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, simple_forwarder_start(handle, NULL, NULL, IGNORED_PTR_ARG))
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, simple_forwarder_finish(handle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, callback_context_checker_start(handle, (gpointer)0x42, (gpointer)0x42, IGNORED_PTR_ARG))
            .IgnoreArgument(4);

        ///act
        (void)GIO_Async_Seq_Run_Async(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_034: [ resolve_callback shall complete the sequence and invoke the sequence's complete callback if there are no more asynchronous operations to process. ]*/
    TEST_FUNCTION(resolve_complete_calls_sequence_complete_callback)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, sequence_complete);
        (void)GIO_Async_Seq_Add(
            handle, NULL,
            simple_forwarder_start, simple_forwarder_finish,
            simple_forwarder_start, simple_forwarder_finish,
            NULL
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, simple_forwarder_start(handle, NULL, NULL, IGNORED_PTR_ARG))
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, simple_forwarder_start(handle, (gpointer)0x42, NULL, IGNORED_PTR_ARG))
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, simple_forwarder_finish(handle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, simple_forwarder_finish(handle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, sequence_complete(handle, (gpointer)0x42));

        ///act
        (void)GIO_Async_Seq_Run_Async(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_035: [ GIO_Async_Seq_Addv shall return GIO_ASYNCSEQ_ERROR if async_seq_handle is NULL. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Addv_returns_error_for_NULL_handle)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;

        ///act
        auto result = call_GIO_Async_Seq_Adv(NULL, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(GIO_ASYNCSEQ_RESULT, GIO_ASYNCSEQ_ERROR, result);

        ///cleanup
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_036: [ GIO_Async_Seq_Addv shall return GIO_ASYNCSEQ_ERROR if the async sequence's state is not equal to GIO_ASYNCSEQ_STATE_PENDING. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Addv_returns_error_if_state_is_not_pending)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, add_callbacks_while_running_start(handle, NULL, NULL, IGNORED_PTR_ARG))
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_ptr_array_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        ///act
        call_GIO_Async_Seq_Adv(
            handle, NULL,
            add_callbacks_while_running_start, simple_forwarder_finish,
            NULL
        );
        (void)GIO_Async_Seq_Run_Async(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_037: [ GIO_Async_Seq_Addv shall append the new async operations to the end of the existing list of async operations if any. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Addv_appends_to_end_of_list)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_ptr_array_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        ///act
        (void)call_GIO_Async_Seq_Adv(
            handle, NULL,
            simple_forwarder_start, simple_forwarder_finish,
            NULL
        );

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_038: [ GIO_Async_Seq_Addv shall add callbacks from the variable arguments list till a callback whose value is NULL is encountered. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Addv_check_NULL_sentinel)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_ptr_array_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        ///act
        (void)call_GIO_Async_Seq_Adv(
            handle, NULL,
            simple_forwarder_start, simple_forwarder_finish,
            NULL,
            // The following pair SHOULD NOT get added
            simple_forwarder_start, simple_forwarder_finish
        );

        ///assert
        mocks.AssertActualAndExpectedCalls();
        auto actual_size = g_cached_ptr_array->_pointers.size();
        ASSERT_ARE_EQUAL(size_t, 1, actual_size);

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_039: [ When a GIO_ASYNCSEQ_CALLBACK is encountered in the varargs, the next argument MUST be non-NULL. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Addv_callback_pairs_must_match)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        mocks.ResetAllCalls();

        ///act
        auto result = call_GIO_Async_Seq_Adv(
            handle, NULL,
            simple_forwarder_start, NULL
        );

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(GIO_ASYNCSEQ_RESULT, GIO_ASYNCSEQ_ERROR, result);

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_040: [ GIO_Async_Seq_Addv shall return GIO_ASYNCSEQ_ERROR when any of the underlying platform calls fail. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Addv_fails_when_malloc_fails)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((void*)NULL);

        ///act
        auto result = call_GIO_Async_Seq_Adv(
            handle, NULL,
            simple_forwarder_start, simple_forwarder_finish,
            NULL
        );

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(GIO_ASYNCSEQ_RESULT, GIO_ASYNCSEQ_ERROR, result);

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_041: [ The callback_context pointer value shall be saved so that they can be passed to the callbacks later when they are invoked. ]*/
    /*Tests_SRS_GIO_ASYNCSEQ_13_021: [ GIO_Async_Seq_Run shall pass the callback context that was supplied to GIO_Async_Seq_Add or GIO_Async_Seq_Addv when the first operation was added to the sequence when invoking the 'start' callback. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Addv_callback_context_is_passed_along)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, callback_context_checker_start(handle, NULL, (gpointer)0x42, IGNORED_PTR_ARG))
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_ptr_array_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        ///act
        (void)call_GIO_Async_Seq_Adv(
            handle, (gpointer)0x42,
            callback_context_checker_start, simple_forwarder_finish,
            NULL
        );
        (void)GIO_Async_Seq_Run_Async(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_042: [ The list of callbacks that had been added to the sequence before calling this API will remain unchanged after the function returns when an error occurs. ]*/
    TEST_FUNCTION(GIO_Async_Seq_Addv_is_transactional)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        (void)GIO_Async_Seq_Add(
            handle, NULL,
            simple_forwarder_start, simple_forwarder_finish,
            NULL
        );
        mocks.ResetAllCalls();

        // let things go haywire when the second function pair
        // is being added at which point the internal list should
        // have 2 items - one that was added above and one that
        // was added for the first function pair in this call
        // which should subsequently get rolled back
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((void*)NULL);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_ptr_array_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, g_ptr_array_remove_range(IGNORED_PTR_ARG, 1, 1))
            .IgnoreArgument(1);

        ///act
        auto result = call_GIO_Async_Seq_Adv(
            handle, NULL,
            simple_forwarder_start, simple_forwarder_finish,
            simple_forwarder_start, simple_forwarder_finish,
            NULL
        );

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(GIO_ASYNCSEQ_RESULT, GIO_ASYNCSEQ_ERROR, result);
        ASSERT_ARE_EQUAL(size_t, 1, g_cached_ptr_array->_pointers.size());

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

    /*Tests_SRS_GIO_ASYNCSEQ_13_043: [GIO_Async_Seq_Addv shall return GIO_ASYNCSEQ_OK if the API executes successfully.]*/
    TEST_FUNCTION(GIO_Async_Seq_Addv_succeeds)
    {
        ///arrange
        CGIOAsyncSeqMocks mocks;
        auto handle = GIO_Async_Seq_Create(NULL, NULL, NULL);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_ptr_array_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, g_ptr_array_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        ///act
        auto result = call_GIO_Async_Seq_Adv(
            handle, NULL,
            simple_forwarder_start, simple_forwarder_finish,
            simple_forwarder_start, simple_forwarder_finish,
            NULL
        );

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(GIO_ASYNCSEQ_RESULT, GIO_ASYNCSEQ_OK, result);
        ASSERT_ARE_EQUAL(size_t, 2, g_cached_ptr_array->_pointers.size());

        ///cleanup
        GIO_Async_Seq_Destroy(handle);
    }

END_TEST_SUITE(gio_async_seq_unittests)
