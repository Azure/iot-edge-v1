// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"

#include "azure_c_shared_utility/lock.h"
#include "bleio_seq.h"

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

DEFINE_MICROMOCK_ENUM_TO_STRING(BLEIO_SEQ_RESULT, BLEIO_SEQ_RESULT_VALUES);

TYPED_MOCK_CLASS(CBLEIOSeqMocks, CGlobalMock)
{
public:

    // memory
    MOCK_STATIC_METHOD_1(, void*, gballoc_malloc, size_t, size)
        void* result2 = BASEIMPLEMENTATION::gballoc_malloc(size);
    MOCK_METHOD_END(void*, result2);

    MOCK_STATIC_METHOD_1(, void, gballoc_free, void*, ptr)
        BASEIMPLEMENTATION::gballoc_free(ptr);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_5(, void, on_write_complete, BLEIO_SEQ_HANDLE, bleio_seq_handle, void*, context, const char*, characteristic_uuid, BLEIO_SEQ_INSTRUCTION_TYPE, type, BLEIO_SEQ_RESULT, result2)
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_6(, void, on_read_complete, BLEIO_SEQ_HANDLE, bleio_seq_handle, void*, context, const char*, characteristic_uuid, BLEIO_SEQ_INSTRUCTION_TYPE, type, BLEIO_SEQ_RESULT, result2, BUFFER_HANDLE, data)
    MOCK_VOID_METHOD_END()
};

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEIOSeqMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEIOSeqMocks, , void, gballoc_free, void*, ptr);

DECLARE_GLOBAL_MOCK_METHOD_5(CBLEIOSeqMocks, , void, on_write_complete, BLEIO_SEQ_HANDLE, bleio_seq_handle, void*, context, const char*, characteristic_uuid, BLEIO_SEQ_INSTRUCTION_TYPE, type, BLEIO_SEQ_RESULT, result);
DECLARE_GLOBAL_MOCK_METHOD_6(CBLEIOSeqMocks, , void, on_read_complete, BLEIO_SEQ_HANDLE, bleio_seq_handle, void*, context, const char*, characteristic_uuid, BLEIO_SEQ_INSTRUCTION_TYPE, type, BLEIO_SEQ_RESULT, result, BUFFER_HANDLE, data);

BEGIN_TEST_SUITE(bleio_seq_unittests)
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
    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
        if (!MicroMockReleaseMutex(g_testByTest))
        {
            ASSERT_FAIL("failure in test framework at ReleaseMutex");
        }
    }

    /*Tests_SRS_BLEIO_SEQ_13_028: [On Windows, this function shall return NULL.]*/
    TEST_FUNCTION(BLEIO_Seq_Create_returns_NULL)
    {
        ///arrange
        CBLEIOSeqMocks mocks;

        ///act
        auto result = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, (VECTOR_HANDLE)0x42, on_read_complete, on_write_complete);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLEIO_SEQ_13_029: [ On Windows, this function shall do nothing. ]*/
    TEST_FUNCTION(BLEIO_Seq_Destroy_does_nothing)
    {
        ///arrange
        CBLEIOSeqMocks mocks;

        ///act
        BLEIO_Seq_Destroy((BLEIO_SEQ_HANDLE)0x42, NULL, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_BLEIO_SEQ_13_030: [ On Windows this function shall return BLEIO_SEQ_ERROR. ]*/
    TEST_FUNCTION(BLEIO_Seq_Run_returns_error)
    {
        ///arrange
        CBLEIOSeqMocks mocks;

        ///act
        auto result = BLEIO_Seq_Run((BLEIO_SEQ_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
    }

    /*Tests_SRS_BLEIO_SEQ_13_044: [ On Windows this function shall return BLEIO_SEQ_ERROR. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_returns_error)
    {
        ///arrange
        CBLEIOSeqMocks mocks;

        ///act
        auto result = BLEIO_Seq_AddInstruction((BLEIO_SEQ_HANDLE)0x42, (BLEIO_SEQ_INSTRUCTION*)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
    }

END_TEST_SUITE(bleio_seq_unittests)
