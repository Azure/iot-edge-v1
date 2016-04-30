// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"

#include "dynamic_library.h"
#include "gb_library.h"

#include "azure_c_shared_utility/lock.h"

#ifndef GB_LIBRARY_INTERCEPT
#error these unit tests require the symbol GB_LIBRARY_INTERCEPT to be defined
#else
extern "C"
{
	extern void *gb_dlopen(const char *__file, int __mode);
	extern int gb_dlclose(void *__handle);
	extern void *gb_dlsym(void * __handle, const char * __name);
}
#endif


#define LOAD_LIBRARY_RETURN (void*)1
#define GET_PROC_ADDR_RETURN (void*)2
#define HMODULE_HANDLE (void*)3
#define LIBRARY_NAME "LIBRARY1"
#define SYMBOL_NAME "Symbol1"

TYPED_MOCK_CLASS(CDynamicLibraryMocks, CGlobalMock)
{
public:
	MOCK_STATIC_METHOD_2(, void*, gb_dlopen, const char*, __file, int, __mode)
		MOCK_METHOD_END(void*, (void*)LOAD_LIBRARY_RETURN)

		MOCK_STATIC_METHOD_1(, int, gb_dlclose, void*, library)
		MOCK_METHOD_END(int, 1)

		MOCK_STATIC_METHOD_2(, void*, gb_dlsym, void*, library, const char*, symbolName)
		MOCK_METHOD_END(void*, (void*)GET_PROC_ADDR_RETURN)

};

DECLARE_GLOBAL_MOCK_METHOD_2(CDynamicLibraryMocks, , void*, gb_dlopen, const char*, __file, int, __mode);
DECLARE_GLOBAL_MOCK_METHOD_1(CDynamicLibraryMocks, , int, gb_dlclose, void*, library);
DECLARE_GLOBAL_MOCK_METHOD_2(CDynamicLibraryMocks, , void*, gb_dlsym, void*, library, const char*, symbolName);


static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;
static MICROMOCK_MUTEX_HANDLE g_testByTest;

BEGIN_TEST_SUITE(dynamic_library_unittests)

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

// Tests_SRS_DYNAMIC_LIBRARY_17_001: [DynamicLibrary_LoadLibrary shall make the OS system call to load the named library, returning an opaque pointer as a library reference.]
TEST_FUNCTION(DynamicLibrary_LoadLibrary_returns_correct_value)
{
	CDynamicLibraryMocks mocks;

	///arrange
	const char* moduleFileName = LIBRARY_NAME;

	STRICT_EXPECTED_CALL(mocks, gb_dlopen(moduleFileName, RTLD_LAZY))
		.IgnoreArgument(2);


	///act
	auto lib = DynamicLibrary_LoadLibrary(moduleFileName);

	///assert
	ASSERT_ARE_EQUAL(void_ptr, lib, LOAD_LIBRARY_RETURN);

	///cleanup
}

// Tests_SRS_DYNAMIC_LIBRARY_17_002: [DynamicLibrary_UnloadLibrary shall make the OS system call to unload the library referenced by libraryHandle.]
TEST_FUNCTION(DynamicLibrary_UnloadLibrary_Success)
{
	CDynamicLibraryMocks mocks;

	///arrange

	void* library = HMODULE_HANDLE;

	STRICT_EXPECTED_CALL(mocks, gb_dlclose(library));

	///act
	DynamicLibrary_UnloadLibrary(library);

	///assert

	///cleanup
}

// Tests_SRS_DYNAMIC_LIBRARY_17_003: [DynamicLibrary_FindSymbol shall make the OS system call to look up symbolName in the library referenced by libraryHandle.]
TEST_FUNCTION(DynamicLibrary_FindSymbol_returns_correct_value)
{
	CDynamicLibraryMocks mocks;

	///arrange
	void* library = HMODULE_HANDLE;
	const char * symbol = SYMBOL_NAME;

	STRICT_EXPECTED_CALL(mocks, gb_dlsym(library, symbol));

	///act
	auto result = DynamicLibrary_FindSymbol(library, symbol);

	///assert
	ASSERT_ARE_EQUAL(void_ptr, result, GET_PROC_ADDR_RETURN);

	///cleanup
}

END_TEST_SUITE(dynamic_library_unittests)




