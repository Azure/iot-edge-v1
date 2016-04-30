// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"

#include "module_loader.h"
#include "dynamic_library.h"

#include "azure_c_shared_utility/lock.h"

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

static size_t currentmalloc_call;
static size_t whenShallmalloc_fail;

// Value for a good handle
#define TEST_MODULE_LIBRARY_GOOD_HANDLE (void*)0xDEAF
// Value for a valid file handle to a file that does not have a valid symbol
#define TEST_MODULE_LIBRARY_BADSYM_HANDLE (void*)0xDEDE
// Value for good library;
#define TEST_MODULE_LIBRARY_GOOD_NAME ("good")
// Value for a valid library but does not have a valid symbol.
#define TEST_MODULE_LIBRARY_BAD_SYM_NAME ("badsym")
// Value for a library that does not load.
#define TEST_MODULE_LIBRARY_BAD_NAME ("bad")


static bool test_getApi_func_success = true;

TYPED_MOCK_CLASS(CModuleLoaderMocks, CGlobalMock)
{
public:
	//extern void* DynamicLibrary_LoadLibrary(const char* dynamicLibraryFileName);
	MOCK_STATIC_METHOD_1(, void*, DynamicLibrary_LoadLibrary, const char*, dynamicLibraryFileName)
		void* result1;
		if (strcmp(dynamicLibraryFileName, TEST_MODULE_LIBRARY_BAD_NAME) == 0)
		{
			result1 = NULL;
		}
		else
		{
			result1 = malloc(sizeof(void*));
			if (strcmp(dynamicLibraryFileName, TEST_MODULE_LIBRARY_BAD_SYM_NAME) == 0)
			{
				*(void**)result1 = TEST_MODULE_LIBRARY_BADSYM_HANDLE;
			}
			else if (strcmp(dynamicLibraryFileName, TEST_MODULE_LIBRARY_GOOD_NAME) == 0)
			{
				*(void**)result1 = TEST_MODULE_LIBRARY_GOOD_HANDLE;
			}
		}
	MOCK_METHOD_END(void*, result1)

	//extern void  DynamicLibrary_UnloadLibrary(void* library);
	MOCK_STATIC_METHOD_1(, void, DynamicLibrary_UnloadLibrary, void*, library)
		if ((library != NULL) &&
			(library != TEST_MODULE_LIBRARY_BADSYM_HANDLE) &&
			(library != TEST_MODULE_LIBRARY_GOOD_HANDLE))
		{
			free(library);
		}
	MOCK_VOID_METHOD_END()

	//extern void* DynamicLibrary_FindSymbol(void* library, const char* symbolName);
	MOCK_STATIC_METHOD_2(, void*, DynamicLibrary_FindSymbol, void*, library, const char*, symbolName)
		void * result2 = (void*)&test_getApi_func;
		if ((library != NULL) &&
			(library != TEST_MODULE_LIBRARY_BADSYM_HANDLE) &&
			(library != TEST_MODULE_LIBRARY_GOOD_HANDLE))
		{
			if (*(void**)library == TEST_MODULE_LIBRARY_BADSYM_HANDLE)
			{
				result2 = NULL;
			}
		}
		if (library == TEST_MODULE_LIBRARY_BADSYM_HANDLE)
		{
			result2 = NULL;
		}
	MOCK_METHOD_END(void*, result2)

    // Mock GetAPIS function, returned on successful call of FindSymbol.
	MOCK_STATIC_METHOD_0(,void *, test_getApi_func)
		void * result3 = TEST_MODULE_LIBRARY_GOOD_HANDLE;
		if (test_getApi_func_success != true)
		{
			result3 = NULL;
		}
	MOCK_METHOD_END(void*, result3)

	MOCK_STATIC_METHOD_1(, void*, gballoc_malloc, size_t, size)
		void* result2;
		currentmalloc_call++;
		if (whenShallmalloc_fail>0)
		{
			if (currentmalloc_call == whenShallmalloc_fail)
			{
				result2 = (void*)NULL;
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

	MOCK_STATIC_METHOD_2(, void*, gballoc_realloc, void*, ptr, size_t, size)
		MOCK_METHOD_END(void*, BASEIMPLEMENTATION::gballoc_realloc(ptr, size));

	MOCK_STATIC_METHOD_1(, void, gballoc_free, void*, ptr)
		BASEIMPLEMENTATION::gballoc_free(ptr);
	MOCK_VOID_METHOD_END()

};
DECLARE_GLOBAL_MOCK_METHOD_1(CModuleLoaderMocks, , void*, DynamicLibrary_LoadLibrary, const char*, dynamicLibraryFileName);
DECLARE_GLOBAL_MOCK_METHOD_1(CModuleLoaderMocks, , void, DynamicLibrary_UnloadLibrary, void*, library);
DECLARE_GLOBAL_MOCK_METHOD_2(CModuleLoaderMocks, , void*, DynamicLibrary_FindSymbol, void*, library, const char*, symbolName);
DECLARE_GLOBAL_MOCK_METHOD_0(CModuleLoaderMocks, , void*, test_getApi_func);

DECLARE_GLOBAL_MOCK_METHOD_1(CModuleLoaderMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_2(CModuleLoaderMocks, , void*, gballoc_realloc, void*, ptr, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CModuleLoaderMocks, , void, gballoc_free, void*, ptr)

static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;
static MICROMOCK_MUTEX_HANDLE g_testByTest;

BEGIN_TEST_SUITE(module_loader_unittests)

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
		test_getApi_func_success = true;
    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
		if (!MicroMockReleaseMutex(g_testByTest))
		{
			ASSERT_FAIL("failure in test framework at ReleaseMutex");
		}
		currentmalloc_call = 0;
		whenShallmalloc_fail = 0;
		test_getApi_func_success = true;
    }

	/*Tests_SRS_MODULE_LOADER_17_001: [ModuleLoader_Load shall validate the moduleFileName, if it is NULL or an empty string, it will return NULL.]*/
	TEST_FUNCTION(ModuleLoader_Load_Name_Is_Null)
	{
		CModuleLoaderMocks mocks;
		///arrange
		const char* moduleFileName = NULL;

		///act
		MODULE_LIBRARY_HANDLE moduleHandle = ModuleLoader_Load(moduleFileName);

		///assert
		ASSERT_IS_NULL(moduleHandle);

		///cleanup
	}


	/*Tests_SRS_MODULE_LOADER_17_014: [If memory allocation is not successful, the load shall fail, and it shall return NULL.]*/
	TEST_FUNCTION(ModuleLoader_Load_Malloc_Fails)
	{
		CModuleLoaderMocks mocks;
		///arrange
		const char* moduleFileName = TEST_MODULE_LIBRARY_GOOD_NAME;

		whenShallmalloc_fail = 1;
		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
			.IgnoreArgument(1);

		///act
		MODULE_LIBRARY_HANDLE moduleHandle = ModuleLoader_Load(moduleFileName);

		///assert
		ASSERT_IS_NULL(moduleHandle);
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}

	/*Tests_SRS_MODULE_LOADER_17_012: [If load library is not successful, the load shall fail, and it shall return NULL.]*/
	TEST_FUNCTION(ModuleLoader_Load_Library_Fails)
	{
		CModuleLoaderMocks mocks;
		///arrange
		const char* moduleFileName = TEST_MODULE_LIBRARY_BAD_NAME;
		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, DynamicLibrary_LoadLibrary(moduleFileName));
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

		///act
		MODULE_LIBRARY_HANDLE moduleHandle = ModuleLoader_Load(moduleFileName);

		///assert
		ASSERT_IS_NULL(moduleHandle);
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}

	/*Tests_SRS_MODULE_LOADER_17_013: [If locating the function is not successful, the load shall fail, and it shall return NULL.]*/
	TEST_FUNCTION(ModuleLoader_Load_Symbol_Fails)
	{
		CModuleLoaderMocks mocks;
		///arrange
		const char* moduleFileName = TEST_MODULE_LIBRARY_BAD_SYM_NAME;
		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, DynamicLibrary_LoadLibrary(moduleFileName));
		STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol(IGNORED_PTR_ARG, MODULE_GETAPIS_NAME)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, DynamicLibrary_UnloadLibrary(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

		///act
		MODULE_LIBRARY_HANDLE moduleHandle = ModuleLoader_Load(moduleFileName);

		///assert
		ASSERT_IS_NULL(moduleHandle);
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}


	/* Tests_SRS_MODULE_LOADER_17_015: [If the get API call returns NULL, the load shall fail, and it shall return NULL.] */
	TEST_FUNCTION(ModuleLoader_Load_GetAPIs_Is_Null)
	{
		CModuleLoaderMocks mocks;
		///arrange
		test_getApi_func_success = false;
		const char* moduleFileName = TEST_MODULE_LIBRARY_GOOD_NAME;
		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, DynamicLibrary_LoadLibrary(moduleFileName));
		STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol(IGNORED_PTR_ARG, MODULE_GETAPIS_NAME)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, test_getApi_func());
		STRICT_EXPECTED_CALL(mocks, DynamicLibrary_UnloadLibrary(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

		///act
		MODULE_LIBRARY_HANDLE moduleHandle = ModuleLoader_Load(moduleFileName);

		///assert
		ASSERT_IS_NULL(moduleHandle);
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}
    /* Tests_SRS_MODULE_LOADER_17_002: [ModuleLoader_Load shall load the library as a file, the filename given by the moduleLibraryFileName.] */
	/* Tests_SRS_MODULE_LOADER_17_003: [ModuleLoader_Load shall locate the function defined by MODULE_GETAPIS_NAME in the open library.] */
	/* Tests_SRS_MODULE_LOADER_17_004: [ModuleLoader_Load shall call the function defined by MODULE_GETAPIS_NAME in the open library.]*/
	/* Tests_SRS_MODULE_LOADER_17_005: [ModuleLoader_Load shall allocate memory for the structure MODULE_LIBRARY_HANDLE.] */
	/* Tests_SRS_MODULE_LOADER_17_006: [ModuleLoader_Load shall return a non-NULL handle to a MODULE_LIBRARY_DATA_TAG upon success.]*/
	TEST_FUNCTION(ModuleLoader_Success)
	{
		CModuleLoaderMocks mocks;
		///arrange
		const char* moduleFileName = TEST_MODULE_LIBRARY_GOOD_NAME;
		test_getApi_func_success = true;
		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, DynamicLibrary_LoadLibrary(moduleFileName));
		STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol(IGNORED_PTR_ARG, MODULE_GETAPIS_NAME)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, test_getApi_func());

		///act
		MODULE_LIBRARY_HANDLE moduleHandle = ModuleLoader_Load(moduleFileName);

		///assert
		ASSERT_ARE_NOT_EQUAL(void_ptr, NULL, moduleHandle);
		mocks.AssertActualAndExpectedCalls();

		///cleanup
		ModuleLoader_Unload(moduleHandle);
	}

	/*Tests_SRS_MODULE_LOADER_17_007: [ModuleLoader_GetModuleAPIs shall return NULL if the moduleLibrary  is NULL.]*/
	TEST_FUNCTION(ModuleLoader_GetModuleAPIs_Library_Is_Null)
	{
		CModuleLoaderMocks mocks;
		///arrange
		MODULE_LIBRARY_HANDLE moduleLibrary  = NULL;

		///act
		const MODULE_APIS* apisHandle = ModuleLoader_GetModuleAPIs(moduleLibrary);

		///assert
		ASSERT_IS_NULL(apisHandle);

		///cleanup
	}

	/*Tests_SRS_MODULE_LOADER_17_008: [ModuleLoader_GetModuleAPIs shall return a valid pointer to MODULE_APIS on success.]*/
	TEST_FUNCTION(ModuleLoader_GetModuleAPIs_Name_Is_Good)
	{
		CModuleLoaderMocks mocks;
		///arrange
		const char* moduleFileName = TEST_MODULE_LIBRARY_GOOD_NAME;
		test_getApi_func_success = true;
		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, DynamicLibrary_LoadLibrary(moduleFileName));
		STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol(IGNORED_PTR_ARG, MODULE_GETAPIS_NAME));
		STRICT_EXPECTED_CALL(mocks, test_getApi_func());
		MODULE_LIBRARY_HANDLE moduleHandle = ModuleLoader_Load(moduleFileName);
		ASSERT_IS_NOT_NULL(moduleHandle);
		mocks.ResetAllCalls();
		
		///act
		const MODULE_APIS* apisHandle = ModuleLoader_GetModuleAPIs(moduleHandle );

		///assert
		ASSERT_ARE_EQUAL(void_ptr, TEST_MODULE_LIBRARY_GOOD_HANDLE, apisHandle);
		mocks.AssertActualAndExpectedCalls();

		///cleanup
		ModuleLoader_Unload(moduleHandle);

	}

	/*Tests_SRS_MODULE_LOADER_17_009: [ModuleLoader_Unload shall do nothing if the moduleLibrary  is NULL.]*/
	TEST_FUNCTION(ModuleLoader_Null_Module)
	{
		CModuleLoaderMocks mocks;
		///arrange
		MODULE_LIBRARY_HANDLE moduleLibrary  = NULL;

		///act
		ModuleLoader_Unload(moduleLibrary);

		///assert

		///cleanup
	}

	/*Tests_SRS_MODULE_LOADER_17_010 : [ModuleLoader_Unload shall attempt to unload the library.]*/
	/*Tests_SRS_MODULE_LOADER_17_011 : [ModuleLoader_UnLoad shall deallocate memory for the structure MODULE_LIBRARY_HANDLE.]*/
	TEST_FUNCTION(ModuleLoader_UnLoad_Success)
	{
		CModuleLoaderMocks mocks;

		///arrange
		const char* moduleFileName = TEST_MODULE_LIBRARY_GOOD_NAME;
		test_getApi_func_success = true;
		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, DynamicLibrary_LoadLibrary(moduleFileName));
		STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol(IGNORED_PTR_ARG, MODULE_GETAPIS_NAME));
		STRICT_EXPECTED_CALL(mocks, test_getApi_func());
		
		MODULE_LIBRARY_HANDLE moduleHandle = ModuleLoader_Load(moduleFileName);
		ASSERT_IS_NOT_NULL(moduleHandle);
		mocks.ResetAllCalls();
		
		STRICT_EXPECTED_CALL(mocks, DynamicLibrary_UnloadLibrary(TEST_MODULE_LIBRARY_GOOD_HANDLE)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

		///act
		ModuleLoader_Unload(moduleHandle);

		///assert
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}

END_TEST_SUITE(module_loader_unittests)
