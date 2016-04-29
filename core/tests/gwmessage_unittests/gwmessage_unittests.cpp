// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"
#include "azure_c_shared_utility/map.h"
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

#include "message.h"

static size_t currentmalloc_call;
static size_t whenShallmalloc_fail;

static size_t currentConstMap_Create_call;
static size_t whenShallConstMap_Create_fail;

static size_t currentConstMap_Clone_call;
static size_t whenShallConstMap_Clone_fail;

static size_t currentCONSTBUFFER_Create_call;
static size_t whenShallCONSTBUFFER_Create_fail;
static size_t currentCONSTBUFFER_refCount;

static size_t currentCONSTBUFFER_Clone_call;
static size_t whenShallCONSTBUFFER_Clone_fail;

TYPED_MOCK_CLASS(CMessageMocks, CGlobalMock)
{
public:

	// memory
    MOCK_STATIC_METHOD_1(, void*, gballoc_malloc, size_t, size)
        void* result2;
    currentmalloc_call++;
    if (whenShallmalloc_fail>0)
    {
        if (currentmalloc_call == whenShallmalloc_fail)
        {
            result2 = NULL;
        }
        else
        {
            result2 = BASEIMPLEMENTATION::gballoc_malloc(size);
        }
    }
    else
    {
        result2 = BASEIMPLEMENTATION::gballoc_malloc(size);
    }
    MOCK_METHOD_END(void*, result2);

    MOCK_STATIC_METHOD_1(, void, gballoc_free, void*, ptr)
        BASEIMPLEMENTATION::gballoc_free(ptr);
    MOCK_VOID_METHOD_END()

	// ConstMap mocks
    MOCK_STATIC_METHOD_1(, CONSTMAP_HANDLE, ConstMap_Create, MAP_HANDLE, sourceMap)
        CONSTMAP_HANDLE result2;
        currentConstMap_Create_call ++;
        if (whenShallConstMap_Create_fail == currentConstMap_Create_call)
        {
            result2 = NULL;
        }
        else
        {
            result2 = (CONSTMAP_HANDLE)malloc(1);
			*(unsigned char*)result2 = 1;
        }
    MOCK_METHOD_END(CONSTMAP_HANDLE, result2)

	MOCK_STATIC_METHOD_1(, CONSTMAP_HANDLE, ConstMap_Clone, CONSTMAP_HANDLE, handle)
		CONSTMAP_HANDLE result3;
		currentConstMap_Clone_call++;
		if (whenShallConstMap_Clone_fail == currentConstMap_Clone_call)
		{
			result3 = NULL;
		}
		else
		{
			result3 = handle;
			*(unsigned char*)result3 += 1;
		}
	MOCK_METHOD_END(CONSTMAP_HANDLE, result3)

	MOCK_STATIC_METHOD_1(, void, ConstMap_Destroy, CONSTMAP_HANDLE, map)
 		unsigned char refCount = --(*(unsigned char*)map);
		if (refCount == 0)
			free(map);
	MOCK_VOID_METHOD_END()

	// CONSTBUFFER mocks.
	MOCK_STATIC_METHOD_2(, CONSTBUFFER_HANDLE, CONSTBUFFER_Create, const unsigned char*, source, size_t, size)
		CONSTBUFFER_HANDLE result1;
		currentCONSTBUFFER_Create_call++;
		if (whenShallCONSTBUFFER_Create_fail == currentCONSTBUFFER_Create_call)
		{
			result1 = NULL;
		}
		else
		{
			result1 = (CONSTBUFFER_HANDLE)malloc(sizeof(CONSTBUFFER));
			(*(CONSTBUFFER*)result1).size = size;
			if (size == 0)
			{
				(*(CONSTBUFFER*)result1).buffer = NULL;
			}
			else
			{
				unsigned char* temp = (unsigned char*)malloc(size);
				memcpy(temp, source, size);
				(*(CONSTBUFFER*)result1).buffer = temp;
			}
			currentCONSTBUFFER_refCount = 1;
		}
	MOCK_METHOD_END(CONSTBUFFER_HANDLE, result1)

	MOCK_STATIC_METHOD_1(, CONSTBUFFER_HANDLE, CONSTBUFFER_Clone, CONSTBUFFER_HANDLE, constbufferHandle)
		CONSTBUFFER_HANDLE result2;
		currentCONSTBUFFER_Clone_call++;
		if (currentCONSTBUFFER_Clone_call == whenShallCONSTBUFFER_Clone_fail)
		{
			result2 = NULL;
		}
		else
		{
			result2 = constbufferHandle;
			currentCONSTBUFFER_refCount++;
		}
	MOCK_METHOD_END(CONSTBUFFER_HANDLE, result2)

	MOCK_STATIC_METHOD_1(, const CONSTBUFFER*, CONSTBUFFER_GetContent, CONSTBUFFER_HANDLE, constbufferHandle)
		CONSTBUFFER* result3 = (CONSTBUFFER*)constbufferHandle;
	MOCK_METHOD_END(const CONSTBUFFER*, result3)

	MOCK_STATIC_METHOD_1(, void, CONSTBUFFER_Destroy, CONSTBUFFER_HANDLE, constbufferHandle)
		--currentCONSTBUFFER_refCount;
		if (currentCONSTBUFFER_refCount == 0)
		{
			CONSTBUFFER * fakeBuffer = (CONSTBUFFER*)constbufferHandle;
			if (fakeBuffer->buffer != NULL)
			{
				free((void*)fakeBuffer->buffer);
			}
			free(fakeBuffer);
		}
	MOCK_VOID_METHOD_END()

};

DECLARE_GLOBAL_MOCK_METHOD_1(CMessageMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CMessageMocks, , void, gballoc_free, void*, ptr);
DECLARE_GLOBAL_MOCK_METHOD_1(CMessageMocks, , CONSTMAP_HANDLE, ConstMap_Create, MAP_HANDLE, sourceMap);
DECLARE_GLOBAL_MOCK_METHOD_1(CMessageMocks, , CONSTMAP_HANDLE, ConstMap_Clone, CONSTMAP_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CMessageMocks, , void, ConstMap_Destroy, CONSTMAP_HANDLE, map);
DECLARE_GLOBAL_MOCK_METHOD_2(CMessageMocks, , CONSTBUFFER_HANDLE, CONSTBUFFER_Create, const unsigned char*, source, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CMessageMocks, , CONSTBUFFER_HANDLE, CONSTBUFFER_Clone, CONSTBUFFER_HANDLE, constbufferHandle);
DECLARE_GLOBAL_MOCK_METHOD_1(CMessageMocks, , const CONSTBUFFER*, CONSTBUFFER_GetContent, CONSTBUFFER_HANDLE, constbufferHandle);
DECLARE_GLOBAL_MOCK_METHOD_1(CMessageMocks, , void, CONSTBUFFER_Destroy, CONSTBUFFER_HANDLE, constbufferHandle);

BEGIN_TEST_SUITE(gwmessage_unittests)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        INITIALIZE_MEMORY_DEBUG(g_dllByDll);
        g_testByTest = MicroMockCreateMutex();
        ASSERT_IS_NOT_NULL(g_testByTest);
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        MicroMockDestroyMutex(g_testByTest);
        DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
    }

    TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
    {
        if (!MicroMockAcquireMutex(g_testByTest))
        {
            ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
        }

        currentmalloc_call = 0;
        whenShallmalloc_fail = 0;

		currentConstMap_Create_call = 0;
		whenShallConstMap_Create_fail = 0;
		currentConstMap_Clone_call = 0;
		whenShallConstMap_Clone_fail = 0;
		currentCONSTBUFFER_Create_call = 0;
		whenShallCONSTBUFFER_Create_fail = 0;
		currentCONSTBUFFER_refCount = 0;
		currentCONSTBUFFER_Clone_call = 0;
		whenShallCONSTBUFFER_Clone_fail = 0;

    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
        if (!MicroMockReleaseMutex(g_testByTest))
        {
            ASSERT_FAIL("failure in test framework at ReleaseMutex");
        }
    }

    /*Tests_SRS_MESSAGE_02_002: [If cfg is NULL then Message_Create shall return NULL.]*/
    TEST_FUNCTION(Message_Create_with_NULL_parameter_fails)
    {
        ///arrange
        CMessageMocks mocks;

        ///act
        auto r = Message_Create(NULL);

        ///assert
        ASSERT_IS_NULL(r);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_003: [If field source of cfg is NULL and size is not zero, then Message_Create shall fail and return NULL.]*/
    TEST_FUNCTION(Message_Create_with_NULL_source_and_non_zero_size_fails)
    {
        ///arrange
        CMessageMocks mocks;
        MESSAGE_CONFIG c = {1, NULL, (MAP_HANDLE)&mocks};

        ///act
        auto r = Message_Create(&c);

        ///assert
        ASSERT_IS_NULL(r);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_006: [Otherwise, Message_Create shall return a non-NULL handle and shall set the internal ref count to "1".]*/
    /*Tests_SRS_MESSAGE_02_019: [Message_Create shall clone the sourceProperties to a readonly CONSTMAP.] */
	/*Tests_SRS_MESSAGE_17_003: [Message_Create shall copy the source to a readonly CONSTBUFFER.]*/
    TEST_FUNCTION(Message_Create_happy_path)
    {
        ///arrange
        CMessageMocks mocks;
        unsigned char fake;
        MESSAGE_CONFIG c = { 1, &fake, (MAP_HANDLE)&fake};

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
            .IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Create(&fake, 1)); /*this is copying the buffer*/

		STRICT_EXPECTED_CALL(mocks, ConstMap_Create((MAP_HANDLE)&fake)); /*this is copying the properties*/

        ///act
        auto r = Message_Create(&c);

        ///assert
        ASSERT_IS_NOT_NULL(r);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Message_Destroy(r);
    }

    /*Tests_SRS_MESSAGE_02_004: [Mesages shall be allowed to be created from zero-size content.]*/
    TEST_FUNCTION(Message_Create_happy_path_zero_size_1)
    {
        ///arrange
        CMessageMocks mocks;
        unsigned char fake;
        MESSAGE_CONFIG c = { 0, &fake, (MAP_HANDLE)&fake };

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
            .IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Create(&fake, 0)); /* this is copying the (empty buffer)*/

		STRICT_EXPECTED_CALL(mocks, ConstMap_Create((MAP_HANDLE)&fake)); /*this is copying the properties*/

        ///act
        auto r = Message_Create(&c);

        ///assert
        ASSERT_IS_NOT_NULL(r);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Message_Destroy(r);
    }

    /*Tests_SRS_MESSAGE_02_004: [Mesages shall be allowed to be created from zero-size content.]*/
    TEST_FUNCTION(Message_Create_happy_path_zero_size_2)
    {
        ///arrange
        CMessageMocks mocks;
        unsigned char fake;
        MESSAGE_CONFIG c = { 0, NULL, (MAP_HANDLE)&fake }; /*<---- this is NULL , in the testbefore it was non-NULL*/

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
            .IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Create(NULL, 0)); /* this is copying the (empty buffer)*/

		STRICT_EXPECTED_CALL(mocks, ConstMap_Create((MAP_HANDLE)&fake)); /*this is copying the properties*/

        ///act
        auto r = Message_Create(&c);

        ///assert
        ASSERT_IS_NOT_NULL(r);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Message_Destroy(r);
    }

    /*Tests_SRS_MESSAGE_02_005: [If Message_Create encounters an error while building the internal structures of the message, then it shall return NULL.]*/
    TEST_FUNCTION(Message_Create_zero_size_fails_when_Map_Clone_fails)
    {
        ///arrange
        CMessageMocks mocks;
        unsigned char fake;
        MESSAGE_CONFIG c = { 0, NULL, (MAP_HANDLE)&fake }; /*<---- this is NULL , in the testbefore it was non-NULL*/

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Create(NULL, 0)); /* this is copying the (empty buffer)*/
		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Destroy(IGNORED_PTR_ARG)) /* this is copying the (empty buffer)*/
			.IgnoreArgument(1);

        whenShallConstMap_Create_fail = 1;
        STRICT_EXPECTED_CALL(mocks, ConstMap_Create((MAP_HANDLE)&fake)) /*this is copying the properties*/
            .IgnoreArgument(1);
            
        ///act
        auto r = Message_Create(&c);

        ///assert
        ASSERT_IS_NULL(r);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_005: [If Message_Create encounters an error while building the internal structures of the message, then it shall return NULL.]*/
    TEST_FUNCTION(Message_Create_zero_size_fails_when_malloc_fails)
    {
        ///arrange
        CMessageMocks mocks;
        unsigned char fake;
        MESSAGE_CONFIG c = { 0, NULL, (MAP_HANDLE)&fake }; /*<---- this is NULL , in the testbefore it was non-NULL*/

        whenShallmalloc_fail = 1;
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
            .IgnoreArgument(1);

        ///act
        auto r = Message_Create(&c);

        ///assert
        ASSERT_IS_NULL(r);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_005: [If Message_Create encounters an error while building the internal structures of the message, then it shall return NULL.]*/
    TEST_FUNCTION(Message_Create_nonzero_size_fails_when_Map_Clone_fails)
    {
        ///arrange
        CMessageMocks mocks;
        unsigned char fake;
        MESSAGE_CONFIG c = { 1, &fake, (MAP_HANDLE)&fake };

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Create(&fake, 1)); /* this is copying the buffer*/
		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Destroy(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

        whenShallConstMap_Create_fail = 1;
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create((MAP_HANDLE)&fake)); /*this is copying the properties*/

        ///act
        auto r = Message_Create(&c);

        ///assert
        ASSERT_IS_NULL(r);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_005: [If Message_Create encounters an error while building the internal structures of the message, then it shall return NULL.]*/
    TEST_FUNCTION(Message_Create_nonzero_size_fails_when_CONSTBUFFER_fails_1)
    {
        ///arrange
        CMessageMocks mocks;
        unsigned char fake;
        MESSAGE_CONFIG c = { 1, &fake, (MAP_HANDLE)&fake };

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

		whenShallCONSTBUFFER_Create_fail = 1;
		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Create(&fake, 1)); /* this is copying the buffer*/


        ///act
        auto r = Message_Create(&c);

        ///assert
        ASSERT_IS_NULL(r);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_005: [If Message_Create encounters an error while building the internal structures of the message, then it shall return NULL.]*/
    TEST_FUNCTION(Message_Create_nonzero_size_fails_when_malloc_fails_2)
    {
        ///arrange
        CMessageMocks mocks;
        unsigned char fake;
        MESSAGE_CONFIG c = { 1, &fake, (MAP_HANDLE)&fake };

        whenShallmalloc_fail = 1;
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
            .IgnoreArgument(1);

        ///act
        auto r = Message_Create(&c);

        ///assert
        ASSERT_IS_NULL(r);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

	/* Tests_SRS_MESSAGE_17_008: [ If cfg is NULL then Message_CreateFromBuffer shall return NULL.]*/
	TEST_FUNCTION(Message_CreateFromBuffer_with_NULL_parameter_fails)
	{
		///arrange
		CMessageMocks mocks;

		///act
		auto r = Message_CreateFromBuffer(NULL);

		///assert
		ASSERT_IS_NULL(r);
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}

	/*Tests_SRS_MESSAGE_17_009: [If field sourceContent of cfg is NULL, then Message_CreateFromBuffer shall fail and return NULL.]*/
	TEST_FUNCTION(Message_CreateFromBuffer_with_NULL_content_fails)
	{
		///arrange
		CMessageMocks mocks;
		MESSAGE_BUFFER_CONFIG cfg =
		{
			NULL,
			NULL
		};
	/*	typedef struct MESSAGE_BUFFER_CONFIG_TAG
		{
			CONSTBUFFER_HANDLE sourceContent;
			MAP_HANDLE sourceProperties;
		}MESSAGE_BUFFER_CONFIG;*/

		///act
		auto r = Message_CreateFromBuffer(&cfg);

		///assert
		ASSERT_IS_NULL(r);
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}

	/*Tests_SRS_MESSAGE_17_010: [If field sourceProperties of cfg is NULL, then Message_CreateFromBuffer shall fail and return NULL.]*/
	TEST_FUNCTION(Message_CreateFromBuffer_with_NULL_properties_fails)
	{
		///arrange
		CMessageMocks mocks;
		CONSTBUFFER_HANDLE buffer = CONSTBUFFER_Create(NULL, 0);

		mocks.ResetAllCalls();

		MESSAGE_BUFFER_CONFIG cfg =
		{
			buffer,
			NULL
		};

		///act
		auto r = Message_CreateFromBuffer(&cfg);

		///assert
		ASSERT_IS_NULL(r);
		mocks.AssertActualAndExpectedCalls();

		///cleanup
		CONSTBUFFER_Destroy(buffer);

	}

	/*Tests_SRS_MESSAGE_17_014: [On success, Message_CreateFromBuffer shall return a non-NULL handle and set the internal ref count to "1".]*/
	/*Tests_SRS_MESSAGE_17_012: [Message_CreateFromBuffer shall copy the sourceProperties to a readonly CONSTMAP.]*/
	/*Tests_SRS_MESSAGE_17_013: [Message_CreateFromBuffer shall clone the CONSTBUFFER sourceBuffer.]*/
	TEST_FUNCTION(Message_CreateFromBuffer_Success)
	{
		///arrange
		CMessageMocks mocks;
		unsigned char fake;
		CONSTBUFFER_HANDLE buffer = CONSTBUFFER_Create(&fake, 1);
		MESSAGE_BUFFER_CONFIG cfg =
		{
			buffer,
			(MAP_HANDLE)&fake
		};

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Clone(buffer)); /*this is copying the buffer*/

		STRICT_EXPECTED_CALL(mocks, ConstMap_Create((MAP_HANDLE)&fake)); /*this is copying the properties*/


		///act
		auto r = Message_CreateFromBuffer(&cfg);

		///assert
		ASSERT_IS_NOT_NULL(r);
		mocks.AssertActualAndExpectedCalls();

		///cleanup
		Message_Destroy(r);
		CONSTBUFFER_Destroy(buffer);

	}

	/*Tests_SRS_MESSAGE_17_011: [If Message_CreateFromBuffer encounters an error while building the internal structures of the message, then it shall return NULL.]*/
	TEST_FUNCTION(Message_CreateFromBuffer_Allocation_Failed)
	{
		///arrange
		CMessageMocks mocks;
		unsigned char fake;
		CONSTBUFFER_HANDLE buffer = CONSTBUFFER_Create(&fake, 1);
		MESSAGE_BUFFER_CONFIG cfg =
		{
			buffer,
			(MAP_HANDLE)&fake
		};

		whenShallmalloc_fail = 1;

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
			.IgnoreArgument(1);

		///act
		auto r = Message_CreateFromBuffer(&cfg);

		///assert
		ASSERT_IS_NULL(r);
		mocks.AssertActualAndExpectedCalls();

		///cleanup
		CONSTBUFFER_Destroy(buffer);

	}

	/*Tests_SRS_MESSAGE_17_011: [If Message_CreateFromBuffer encounters an error while building the internal structures of the message, then it shall return NULL.]*/
	TEST_FUNCTION(Message_CreateFromBuffer_Buffer_Clone_Failed)
	{
		///arrange
		CMessageMocks mocks;
		unsigned char fake;
		CONSTBUFFER_HANDLE buffer = CONSTBUFFER_Create(&fake, 1);
		MESSAGE_BUFFER_CONFIG cfg =
		{
			buffer,
			(MAP_HANDLE)&fake
		};

		whenShallCONSTBUFFER_Clone_fail = 1;
		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Clone(buffer)); /*this is copying the buffer*/

		///act
		auto r = Message_CreateFromBuffer(&cfg);

		///assert
		ASSERT_IS_NULL(r);
		mocks.AssertActualAndExpectedCalls();

		//cleanup
		CONSTBUFFER_Destroy(buffer);
	}

	/*Tests_SRS_MESSAGE_17_011: [If Message_CreateFromBuffer encounters an error while building the internal structures of the message, then it shall return NULL.]*/
	TEST_FUNCTION(Message_CreateFromBuffer_Properties_Copy_Failed)
	{
		///arrange
		CMessageMocks mocks;
		unsigned char fake;
		CONSTBUFFER_HANDLE buffer = CONSTBUFFER_Create(&fake, 1);
		MESSAGE_BUFFER_CONFIG cfg =
		{
			buffer,
			(MAP_HANDLE)&fake
		};

		whenShallConstMap_Create_fail = 1;
		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Clone(buffer)); /*this is copying the buffer*/
		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Destroy(buffer));

		STRICT_EXPECTED_CALL(mocks, ConstMap_Create((MAP_HANDLE)&fake)); /*this is copying the properties*/


		///act
		auto r = Message_CreateFromBuffer(&cfg);

		///assert
		ASSERT_IS_NULL(r);
		mocks.AssertActualAndExpectedCalls();

		///cleanup
		CONSTBUFFER_Destroy(buffer);
	}

    /*Tests_SRS_MESSAGE_02_007: [If messageHandle is NULL then Message_Clone shall return NULL.] */
    TEST_FUNCTION(Message_Clone_with_NULL_argument_returns_NULL)
    {
        ///arrange
        CMessageMocks mocks;

        ///act
        auto r = Message_Clone(NULL);

        ///assert
        ASSERT_IS_NULL(r);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_010: [Message_Clone shall return messageHandle.]*/
	/*Tests_SRS_MESSAGE_17_001: [Message_Clone shall clone the CONSTMAP handle.]*/
	/*Tests_SRS_MESSAGE_17_004: [Message_Clone shall clone the CONSTBUFFER handle]*/
    TEST_FUNCTION(Message_Clone_increments_ref_count_1)
    {
        ///arrange
        CMessageMocks mocks;
        MESSAGE_CONFIG c = {0, NULL, (MAP_HANDLE)&mocks};
        auto aMessage = Message_Create(&c);
        mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, ConstMap_Clone(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Clone(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

        ///act
        auto r = Message_Clone(aMessage);

        ///assert
        ASSERT_ARE_EQUAL(void_ptr, aMessage, r);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Message_Destroy(r);
        Message_Destroy(aMessage);
    }

    /*Tests_SRS_MESSAGE_02_008: [Otherwise, Message_Clone shall increment the internal ref count.] */
    TEST_FUNCTION(Message_Clone_increments_ref_count_2)
    {
        ///arrange
        CMessageMocks mocks;
        MESSAGE_CONFIG c = { 0, NULL, (MAP_HANDLE)&mocks };
        auto aMessage = Message_Create(&c);
        auto r = Message_Clone(aMessage);
        mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Destroy(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

        ///act
        Message_Destroy(r);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Message_Destroy(aMessage);
    }

    /*Tests_SRS_MESSAGE_02_008: [Otherwise, Message_Clone shall increment the internal ref count.] */
    TEST_FUNCTION(Message_Clone_increments_ref_count_3)
    {
        ///arrange
        CMessageMocks mocks;
        MESSAGE_CONFIG c = { 0, NULL, (MAP_HANDLE)&mocks };
        auto aMessage = Message_Create(&c);
        auto r = Message_Clone(aMessage);
        Message_Destroy(r);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Destroy(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)) /*only 1 because the message is 0 size*/
			.IgnoreArgument(1);

        ///act
        Message_Destroy(aMessage);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_011: [If message is NULL then Message_GetProperties shall return NULL.] */
    TEST_FUNCTION(Message_GetProperties_with_NULL_messageHandle_returns_NULL)
    {
        ///arrange
        CMessageMocks mocks;
        mocks.ResetAllCalls();

        ///act
        auto r = Message_GetProperties(NULL);

        ///assert
        ASSERT_IS_NULL(r);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_012: [Otherwise, Message_GetProperties shall shall clone and return the CONSTMAP handle representing the properties of the message.]*/
    TEST_FUNCTION(Message_GetProperties_happy_path)
    {
        ///arrange
        CMessageMocks mocks;
        MESSAGE_CONFIG c = { 0, NULL, (MAP_HANDLE)&mocks };
        auto aMessage = Message_Create(&c);
        mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, ConstMap_Clone(IGNORED_PTR_ARG)).IgnoreArgument(1);

        ///act
        auto theProperties = Message_GetProperties(aMessage);

        ///assert
        ASSERT_IS_NOT_NULL(theProperties);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Message_Destroy(aMessage);
		ConstMap_Destroy(theProperties);
    }

    /*Tests_SRS_MESSAGE_02_013: [If message is NULL then Message_GetContent shall return NULL.] */
    TEST_FUNCTION(Message_GetContent_with_NULL_message_returns_NULL)
    {
        ///arrange
        CMessageMocks mocks;

        ///act
        auto content = Message_GetContent(NULL);

        ///assert
        ASSERT_IS_NULL(content);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_014: [Otherwise, Message_GetContent shall return a non-NULL const pointer to a structure of type CONSTBUFFER.]*/
    /*Tests_SRS_MESSAGE_02_015: [The MESSAGE_CONTENT's field size shall have the same value as the cfg's field size.]*/
    /*Tests_SRS_MESSAGE_02_016: [The MESSAGE_CONTENT's field data shall compare equal byte-by-byte to the cfg's field source.]*/
    TEST_FUNCTION(Message_GetContent_with_non_NULL_message_zero_size_succeeds)
    {
        ///arrange
        CMessageMocks mocks;
        MESSAGE_CONFIG c = { 0, NULL, (MAP_HANDLE)&mocks };
        auto msg = Message_Create(&c);
        mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_GetContent(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

        ///act
        auto content = Message_GetContent(msg);

        ///assert
        ASSERT_IS_NOT_NULL(content);
        ASSERT_ARE_EQUAL(size_t, 0, content->size);
        ASSERT_IS_NULL(content->buffer);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Message_Destroy(msg);
    }

    /*Tests_SRS_MESSAGE_02_014: [Otherwise, Message_GetContent shall return a non-NULL const pointer to a structure of type CONSTBUFFER.]*/
    /*Tests_SRS_MESSAGE_02_015: [The MESSAGE_CONTENT's field size shall have the same value as the cfg's field size.]*/
    /*Tests_SRS_MESSAGE_02_016: [The MESSAGE_CONTENT's field data shall compare equal byte-by-byte to the cfg's field source.]*/
    TEST_FUNCTION(Message_GetContent_with_non_NULL_message_nonzero_size_succeeds)
    {
        ///arrange
        CMessageMocks mocks;
        char t ='3';
        MESSAGE_CONFIG c = { sizeof(t), (unsigned char*)&t, (MAP_HANDLE)&mocks };
        auto msg = Message_Create(&c);
        mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_GetContent(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

        ///act
        auto content = Message_GetContent(msg);

        ///assert
        ASSERT_IS_NOT_NULL(content);
        ASSERT_ARE_EQUAL(size_t, 1, content->size);
        ASSERT_ARE_EQUAL(int, 0, memcmp(content->buffer, &t, 1));
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Message_Destroy(msg);
    }

	/*Tests_SRS_MESSAGE_17_006: [If message is NULL then Message_GetContentHandle shall return NULL.]*/
	TEST_FUNCTION(Message_GetContentHandle_with_NULL_message_returns_NULL)
	{
		///arrange
		CMessageMocks mocks;

		///act
		auto content = Message_GetContentHandle(NULL);

		///assert
		ASSERT_IS_NULL(content);
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}

	/*Tests_SRS_MESSAGE_17_007: [Otherwise, Message_GetContentHandle shall shall clone and return the CONSTBUFFER_HANDLE representing the message content.]*/
	TEST_FUNCTION(Message_GetContentHandle_with_non_NULL_message_zero_size_succeeds)
	{
		///arrange
		CMessageMocks mocks;
		MESSAGE_CONFIG c = { 0, NULL, (MAP_HANDLE)&mocks };
		auto msg = Message_Create(&c);
		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Clone(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

		///act
		auto content = Message_GetContentHandle(msg);

		///assert
		ASSERT_IS_NOT_NULL(content);
		mocks.AssertActualAndExpectedCalls();

		const CONSTBUFFER * contentBuffer = CONSTBUFFER_GetContent(content);
		ASSERT_ARE_EQUAL(size_t, 0, contentBuffer->size);
		ASSERT_IS_NULL(contentBuffer->buffer);

		///cleanup
		Message_Destroy(msg);
		CONSTBUFFER_Destroy(content);

	}

	/*Tests_SRS_MESSAGE_17_007: [Otherwise, Message_GetContentHandle shall shall clone and return the CONSTBUFFER_HANDLE representing the message content.]*/
	TEST_FUNCTION(Message_GetContentHandle_with_non_NULL_message_nonzero_size_succeeds)
	{
		///arrange
		CMessageMocks mocks;
		char t = '3';
		MESSAGE_CONFIG c = { sizeof(t), (unsigned char*)&t, (MAP_HANDLE)&mocks };
		auto msg = Message_Create(&c);
		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Clone(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

		///act
		auto content = Message_GetContentHandle(msg);

		///assert
		ASSERT_IS_NOT_NULL(content);
		mocks.AssertActualAndExpectedCalls();

		const CONSTBUFFER * contentBuffer = CONSTBUFFER_GetContent(content);
		ASSERT_ARE_EQUAL(size_t, 1, contentBuffer->size);
		ASSERT_ARE_EQUAL(int, 0, memcmp(contentBuffer->buffer, &t, 1));

		///cleanup
		Message_Destroy(msg);
		CONSTBUFFER_Destroy(content);
	}

    /*Tests_SRS_MESSAGE_02_017: [If message is NULL then Message_Destroy shall do nothing.] */
    TEST_FUNCTION(Message_Destroy_with_NULL_argument_does_nothing)
    {
        ///arrange
        CMessageMocks mocks;

        ///act
        Message_Destroy(NULL);

        ///assert
        
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_020: [Otherwise, Message_Destroy shall decrement the internal ref count of the message.] 
    /*Tests_SRS_MESSAGE_02_021: [If the ref count is zero then the allocated resources are freed.]*/
	/*Tests_SRS_MESSAGE_17_002: [Message_Destroy shall destroy the CONSTMAP properties.]*/
	/*Tests_SRS_MESSAGE_17_005: [Message_Destroy shall destroy the CONSTBUFFER.]*/
    TEST_FUNCTION(Message_Destroy_happy_path)
    {
        ///arrange
        CMessageMocks mocks;
        char t = '3';
        MESSAGE_CONFIG c = { sizeof(t), (unsigned char*)&t, (MAP_HANDLE)&mocks };
        auto msg = Message_Create(&c);
        mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)) /*this is the map*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Destroy(IGNORED_PTR_ARG)) /*this is the map*/
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)) /*this is the handle*/
            .IgnoreArgument(1);

        ///act
        Message_Destroy(msg);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }
END_TEST_SUITE(gwmessage_unittests)
