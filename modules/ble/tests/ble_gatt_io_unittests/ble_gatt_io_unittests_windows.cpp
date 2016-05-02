// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"

/*general macros useful for MOCKS*/
#define CURRENT_API_CALL(API) C2(C2(current, API), _call)
#define WHEN_SHALL_API_FAIL(API) C2(C2(whenShall, API), _fail)
#define DEFINE_FAIL_VARIABLES(API) static size_t CURRENT_API_CALL(API); static size_t WHEN_SHALL_API_FAIL(API);
#define MAKE_FAIL(API, WHEN) do{WHEN_SHALL_API_FAIL(API) = CURRENT_API_CALL(API) + WHEN;} while(0)
#define RESET_API_COUNTERS(API) CURRENT_API_CALL(API) = WHEN_SHALL_API_FAIL(API) = 0;

#define LIST_OF_COUNTED_APIS      \
    gballoc_malloc                \

FOR_EACH_1(DEFINE_FAIL_VARIABLES, LIST_OF_COUNTED_APIS)

#include "azure_c_shared_utility/lock.h"

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

TYPED_MOCK_CLASS(CBLEGATTIOMocks, CGlobalMock)
{
public:

    // memory
    MOCK_STATIC_METHOD_1(, void*, gballoc_malloc, size_t, size)
        void* result2 = BASEIMPLEMENTATION::gballoc_malloc(size);
    MOCK_METHOD_END(void*, result2);

    MOCK_STATIC_METHOD_1(, void, gballoc_free, void*, ptr)
        BASEIMPLEMENTATION::gballoc_free(ptr);
    MOCK_VOID_METHOD_END()
};

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEGATTIOMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEGATTIOMocks, , void, gballoc_free, void*, ptr);

static void mocks_ResetAllCounters(void)
{
    FOR_EACH_1(RESET_API_COUNTERS, LIST_OF_COUNTED_APIS);
}

BEGIN_TEST_SUITE(ble_gatt_io_unittests)
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

        mocks_ResetAllCounters();
    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
        if (!MicroMockReleaseMutex(g_testByTest))
        {
            ASSERT_FAIL("failure in test framework at ReleaseMutex");
        }
    }

    TEST_FUNCTION(BLEGATTIO_noop_test)
    {
        ///arrange
        CBLEGATTIOMocks mocks;

        ///act

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }
END_TEST_SUITE(ble_gatt_io_unittests)
