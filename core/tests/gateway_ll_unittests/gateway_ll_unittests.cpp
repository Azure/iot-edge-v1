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

#include "gateway_ll.h"
#include "message_bus.h"
#include "module_loader.h"

#define DUMMY_LIBRARY_PATH "x.dll"

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

static size_t currentmalloc_call;
static size_t whenShallmalloc_fail;

static size_t currentMessageBus_AddModule_call;
static size_t whenShallMessageBus_AddModule_fail;
static size_t currentMessageBus_RemoveModule_call;
static size_t whenShallMessageBus_RemoveModule_fail;
static size_t currentMessageBus_Create_call;
static size_t whenShallMessageBus_Create_fail;
static size_t currentMessageBus_module_count;
static size_t currentMessageBus_ref_count;

static size_t currentModuleLoader_Load_call;
static size_t whenShallModuleLoader_Load_fail;

static size_t currentModule_Create_call;
static size_t currentModule_Destroy_call;

static size_t currentVECTOR_create_call;
static size_t whenShallVECTOR_create_fail;
static size_t currentVECTOR_push_back_call;
static size_t whenShallVECTOR_push_back_fail;
static size_t currentVECTOR_find_if_call;
static size_t whenShallVECTOR_find_if_fail;

static MODULE_APIS dummyAPIs;

TYPED_MOCK_CLASS(CGatewayLLMocks, CGlobalMock)
{
public:
	MOCK_STATIC_METHOD_2(, MODULE_HANDLE, mock_Module_Create, MESSAGE_BUS_HANDLE, busHandle, const void*, configuration)
		currentModule_Create_call++;
		MODULE_HANDLE result1;
		if (configuration != NULL && *((bool*)configuration) == false)
		{
			result1 = NULL;
		}
		else
		{
			result1 = (MODULE_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(currentModule_Create_call);
		}
	MOCK_METHOD_END(MODULE_HANDLE, result1);

	MOCK_STATIC_METHOD_1(, void, mock_Module_Destroy, MODULE_HANDLE, moduleHandle)
		currentModule_Destroy_call++;
		BASEIMPLEMENTATION::gballoc_free(moduleHandle);
	MOCK_VOID_METHOD_END();

	MOCK_STATIC_METHOD_2(, void, mock_Module_Receive, MODULE_HANDLE, moduleHandle, MESSAGE_HANDLE, messageHandle)
	MOCK_VOID_METHOD_END();

	MOCK_STATIC_METHOD_1(, void, MessageBus_DecRef, MESSAGE_BUS_HANDLE, bus)
		if (currentMessageBus_ref_count > 0)
		{
			--currentMessageBus_ref_count;
			if (currentMessageBus_ref_count == 0)
			{
				BASEIMPLEMENTATION::gballoc_free(bus);
			}
		}
	MOCK_VOID_METHOD_END();

	MOCK_STATIC_METHOD_1(, void, MessageBus_IncRef, MESSAGE_BUS_HANDLE, bus)
		++currentMessageBus_ref_count;
	MOCK_VOID_METHOD_END();

	MOCK_STATIC_METHOD_0(, MESSAGE_BUS_HANDLE, MessageBus_Create)
	MESSAGE_BUS_HANDLE result1;
	currentMessageBus_Create_call++;
	if (whenShallMessageBus_Create_fail >= 0 && whenShallMessageBus_Create_fail == currentMessageBus_Create_call)
	{
		result1 = NULL;
	}
	else
	{
		++currentMessageBus_ref_count;
		result1 = (MESSAGE_BUS_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1);
	}
	MOCK_METHOD_END(MESSAGE_BUS_HANDLE, result1);

	MOCK_STATIC_METHOD_1(, void, MessageBus_Destroy, MESSAGE_BUS_HANDLE, bus)
		if (currentMessageBus_ref_count > 0)
		{
			--currentMessageBus_ref_count;
			if (currentMessageBus_ref_count == 0)
			{
				BASEIMPLEMENTATION::gballoc_free(bus);
			}
		}
	MOCK_VOID_METHOD_END();

	MOCK_STATIC_METHOD_2(, MESSAGE_BUS_RESULT, MessageBus_AddModule, MESSAGE_BUS_HANDLE, handle, const MODULE*, module)
		currentMessageBus_AddModule_call++;
		MESSAGE_BUS_RESULT result1  = MESSAGE_BUS_ERROR;
		if (handle != NULL && module != NULL)
		{
			if (whenShallMessageBus_AddModule_fail != currentMessageBus_AddModule_call)
			{
				++currentMessageBus_module_count;
				result1 = MESSAGE_BUS_OK;
			}
		}
	MOCK_METHOD_END(MESSAGE_BUS_RESULT, result1);

	MOCK_STATIC_METHOD_2(, MESSAGE_BUS_RESULT, MessageBus_RemoveModule, MESSAGE_BUS_HANDLE, handle, const MODULE*, module)
		currentMessageBus_RemoveModule_call++;
		MESSAGE_BUS_RESULT result1 = MESSAGE_BUS_ERROR;
		if (handle != NULL && module != NULL && currentMessageBus_module_count > 0 && whenShallMessageBus_RemoveModule_fail != currentMessageBus_RemoveModule_call)
		{
			--currentMessageBus_module_count;
			result1 = MESSAGE_BUS_OK;
		}
	MOCK_METHOD_END(MESSAGE_BUS_RESULT, result1);

	MOCK_STATIC_METHOD_1(, MODULE_LIBRARY_HANDLE, ModuleLoader_Load, const char*, moduleLibraryFileName)
		currentModuleLoader_Load_call++;
		MODULE_LIBRARY_HANDLE handle = NULL;
		if (whenShallModuleLoader_Load_fail >= 0 && whenShallModuleLoader_Load_fail != currentModuleLoader_Load_call)
		{
			handle = (MODULE_LIBRARY_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1);
		}
	MOCK_METHOD_END(MODULE_LIBRARY_HANDLE, handle);

	MOCK_STATIC_METHOD_1(, const MODULE_APIS*, ModuleLoader_GetModuleAPIs, MODULE_LIBRARY_HANDLE, module_library_handle)
		const MODULE_APIS* apis = &dummyAPIs;
	MOCK_METHOD_END(const MODULE_APIS*, apis);

	MOCK_STATIC_METHOD_1(, void, ModuleLoader_Unload, MODULE_LIBRARY_HANDLE, moduleLibraryHandle)
		BASEIMPLEMENTATION::gballoc_free(moduleLibraryHandle);
	MOCK_VOID_METHOD_END();

	MOCK_STATIC_METHOD_1(, VECTOR_HANDLE, VECTOR_create, size_t, elementSize)
		currentVECTOR_create_call++;
		VECTOR_HANDLE vector = NULL;
		if (whenShallVECTOR_create_fail != currentVECTOR_create_call)
		{
			vector = BASEIMPLEMENTATION::VECTOR_create(elementSize);
		}
	MOCK_METHOD_END(VECTOR_HANDLE, vector);

	MOCK_STATIC_METHOD_1(, void, VECTOR_destroy, VECTOR_HANDLE, handle)
		BASEIMPLEMENTATION::VECTOR_destroy(handle);
	MOCK_VOID_METHOD_END();

	MOCK_STATIC_METHOD_3(, int, VECTOR_push_back, VECTOR_HANDLE, handle, const void*, elements, size_t, numElements)
		currentVECTOR_push_back_call++;
		int result1 = -1;
		if (whenShallVECTOR_push_back_fail != currentVECTOR_push_back_call)
		{
			result1 = BASEIMPLEMENTATION::VECTOR_push_back(handle, elements, numElements);
		}
	MOCK_METHOD_END(int, result1);

	MOCK_STATIC_METHOD_3(, void, VECTOR_erase, VECTOR_HANDLE, handle, void*, elements, size_t, index)
		BASEIMPLEMENTATION::VECTOR_erase(handle, elements, index);
	MOCK_VOID_METHOD_END();

	MOCK_STATIC_METHOD_2(, void*, VECTOR_element, const VECTOR_HANDLE, handle, size_t, index)
		auto element = BASEIMPLEMENTATION::VECTOR_element(handle, index);
	MOCK_METHOD_END(void*, element);

	MOCK_STATIC_METHOD_1(, void*, VECTOR_front, const VECTOR_HANDLE, handle)
		auto element = BASEIMPLEMENTATION::VECTOR_front(handle);
	MOCK_METHOD_END(void*, element);

	MOCK_STATIC_METHOD_1(, size_t, VECTOR_size, const VECTOR_HANDLE, handle)
		auto size = BASEIMPLEMENTATION::VECTOR_size(handle);
	MOCK_METHOD_END(size_t, size);

	MOCK_STATIC_METHOD_3(, void*, VECTOR_find_if, const VECTOR_HANDLE, handle, PREDICATE_FUNCTION, pred, const void*, value)
		currentVECTOR_find_if_call++;
		void* element = NULL;
		if (whenShallVECTOR_find_if_fail != currentVECTOR_find_if_call)
		{
			element = BASEIMPLEMENTATION::VECTOR_find_if(handle, pred, value);
		}
	MOCK_METHOD_END(void*, element);

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

	MOCK_STATIC_METHOD_2(, void*, gballoc_realloc, void*, ptr, size_t, size)
	MOCK_METHOD_END(void*, BASEIMPLEMENTATION::gballoc_realloc(ptr, size));

	MOCK_STATIC_METHOD_1(, void, gballoc_free, void*, ptr)
		BASEIMPLEMENTATION::gballoc_free(ptr);
	MOCK_VOID_METHOD_END()

};

DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayLLMocks, , MODULE_HANDLE, mock_Module_Create, MESSAGE_BUS_HANDLE, busHandle, const void*, configuration);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , void, mock_Module_Destroy, MODULE_HANDLE, moduleHandle);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayLLMocks, , void, mock_Module_Receive, MODULE_HANDLE, moduleHandle, MESSAGE_HANDLE, messageHandle);

DECLARE_GLOBAL_MOCK_METHOD_0(CGatewayLLMocks, , MESSAGE_BUS_HANDLE, MessageBus_Create);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , void, MessageBus_Destroy, MESSAGE_BUS_HANDLE, bus);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayLLMocks, , MESSAGE_BUS_RESULT, MessageBus_AddModule, MESSAGE_BUS_HANDLE, handle, const MODULE*, module);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayLLMocks, , MESSAGE_BUS_RESULT, MessageBus_RemoveModule, MESSAGE_BUS_HANDLE, handle, const MODULE*, module);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , void, MessageBus_IncRef, MESSAGE_BUS_HANDLE, bus);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , void, MessageBus_DecRef, MESSAGE_BUS_HANDLE, bus);

DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , MODULE_LIBRARY_HANDLE, ModuleLoader_Load, const char*, moduleLibraryFileName);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , const MODULE_APIS*, ModuleLoader_GetModuleAPIs, MODULE_LIBRARY_HANDLE, module_library_handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , void, ModuleLoader_Unload, MODULE_LIBRARY_HANDLE, moduleLibraryHandle);

DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , VECTOR_HANDLE, VECTOR_create, size_t, elementSize);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , void, VECTOR_destroy, VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_3(CGatewayLLMocks, , int, VECTOR_push_back, VECTOR_HANDLE, handle, const void*, elements, size_t, numElements);
DECLARE_GLOBAL_MOCK_METHOD_3(CGatewayLLMocks, , void, VECTOR_erase, VECTOR_HANDLE, handle, void*, elements, size_t, index);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayLLMocks, , void*, VECTOR_element, const VECTOR_HANDLE, handle, size_t, index);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , void*, VECTOR_front, const VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , size_t, VECTOR_size, const VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_3(CGatewayLLMocks, , void*, VECTOR_find_if, const VECTOR_HANDLE, handle, PREDICATE_FUNCTION, pred, const void*, value);


DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayLLMocks, , void*, gballoc_realloc, void*, ptr, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , void, gballoc_free, void*, ptr)

static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;
static MICROMOCK_MUTEX_HANDLE g_testByTest;

static GATEWAY_PROPERTIES* dummyProps;

BEGIN_TEST_SUITE(gateway_ll_unittests)

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

	currentMessageBus_AddModule_call = 0;
	whenShallMessageBus_AddModule_fail = 0;
	currentMessageBus_RemoveModule_call = 0;
	whenShallMessageBus_RemoveModule_fail = 0;
	currentMessageBus_Create_call = 0;
	whenShallMessageBus_Create_fail = 0;
	currentMessageBus_module_count = 0;
	currentMessageBus_ref_count = 0;

	currentModuleLoader_Load_call = 0;
	whenShallModuleLoader_Load_fail = 0;

	currentModule_Create_call = 0;
	currentModule_Destroy_call = 0;

	currentVECTOR_create_call = 0;
	whenShallVECTOR_create_fail = 0;
	currentVECTOR_push_back_call = 0;
	whenShallVECTOR_push_back_fail = 0;
	currentVECTOR_find_if_call = 0;
	whenShallVECTOR_find_if_fail = 0;

	dummyAPIs = {
		mock_Module_Create,
		mock_Module_Destroy,
		mock_Module_Receive
	};

	GATEWAY_PROPERTIES_ENTRY dummyEntry = {
		"dummy module",
		DUMMY_LIBRARY_PATH,
		NULL
	};

	dummyProps = (GATEWAY_PROPERTIES*)malloc(sizeof(GATEWAY_PROPERTIES));
	dummyProps->gateway_properties_entries = BASEIMPLEMENTATION::VECTOR_create(sizeof(GATEWAY_PROPERTIES_ENTRY));
	BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_properties_entries, &dummyEntry, 1);
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
	if (!MicroMockReleaseMutex(g_testByTest))
	{
		ASSERT_FAIL("failure in test framework at ReleaseMutex");
	}
	currentmalloc_call = 0;
	whenShallmalloc_fail = 0;

	currentMessageBus_Create_call = 0;
	whenShallMessageBus_Create_fail = 0;

	BASEIMPLEMENTATION::VECTOR_destroy(dummyProps->gateway_properties_entries);
	free(dummyProps);
}

/*Tests_SRS_GATEWAY_LL_14_001: [This function shall create a GATEWAY_HANDLE representing the newly created gateway.]*/
/*Tests_SRS_GATEWAY_LL_14_003: [This function shall create a new MESSAGE_BUS_HANDLE for the gateway representing this gateway's message bus. ]*/
/*Tests_SRS_GATEWAY_LL_14_033: [ The function shall create a vector to store each MODULE_DATA. ]*/
TEST_FUNCTION(Gateway_LL_Create_Creates_Handle_Success)
{
	//Arrange
	CGatewayLLMocks mocks;

	//Expectations
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, MessageBus_Create());
	STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
		.IgnoreArgument(1);

	//Act
	GATEWAY_HANDLE gateway = Gateway_LL_Create(NULL);

	//Assert
	ASSERT_IS_NOT_NULL(gateway);
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	Gateway_LL_Destroy(gateway);
}

/*Tests_SRS_GATEWAY_LL_14_002: [This function shall return NULL upon any memory allocation failure.]*/
TEST_FUNCTION(Gateway_LL_Create_Creates_Handle_Malloc_Failure)
{
	//Arrange
	CGatewayLLMocks mocks;

	//Expectations
	whenShallmalloc_fail = 1;
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);

	//Act
	GATEWAY_HANDLE gateway = Gateway_LL_Create(NULL);

	//Assert
	ASSERT_IS_NULL(gateway);
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	//Nothing to cleanup
}

/*Tests_SRS_GATEWAY_LL_14_004: [ This function shall return NULL if a MESSAGE_BUS_HANDLE cannot be created. ]*/
TEST_FUNCTION(Gateway_LL_Create_Cannot_Create_MessageBus_Handle_Failure)
{
	//Arrange
	CGatewayLLMocks mocks;

	//Expectations
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	whenShallMessageBus_Create_fail = 1;
	STRICT_EXPECTED_CALL(mocks, MessageBus_Create());
	STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	GATEWAY_HANDLE gateway = Gateway_LL_Create(NULL);

	//Assert
	ASSERT_IS_NULL(gateway);
	mocks.AssertActualAndExpectedCalls();
	
	//Cleanup
	//Nothing to cleanup
}

/*Tests_SRS_GATEWAY_LL_14_034: [ This function shall return NULL if a VECTOR_HANDLE cannot be created. ]*/
/*Tests_SRS_GATEWAY_LL_14_035: [ This function shall destroy the previously created MESSAGE_BUS_HANDLE and free the GATEWAY_HANDLE if the VECTOR_HANDLE cannot be created. ]*/
TEST_FUNCTION(Gateway_LL_Create_VECTOR_Create_Fails)
{
	//Arrange
	CGatewayLLMocks mocks;

	//Expectations
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(mocks, MessageBus_Create());

	whenShallVECTOR_create_fail = 1; 
	STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(mocks, MessageBus_Destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	//Act
	GATEWAY_HANDLE gateway = Gateway_LL_Create(NULL);
	
	//Assert
	ASSERT_IS_NULL(gateway);

	//Cleanup
	//Nothing to cleanup
}

TEST_FUNCTION(Gateway_LL_Create_VECTOR_push_back_Fails_To_Add_All_Modules_In_Props)
{
	//Arrange
	CGatewayLLMocks mocks;

	//Add another entry to the properties
	GATEWAY_PROPERTIES_ENTRY dummyEntry2 = {
		"dummy module 2",
		"x2.dll",
		NULL
	};

	BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_properties_entries, &dummyEntry2, 1);

	//Expectations
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, MessageBus_Create());
	STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_size(dummyProps->gateway_properties_entries));

	//Adding module 1 (Success)
	STRICT_EXPECTED_CALL(mocks, VECTOR_element(dummyProps->gateway_properties_entries, 0));
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Load(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_GetModuleAPIs(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, NULL))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, MessageBus_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, MessageBus_IncRef(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1)
		.IgnoreArgument(2);

	//Adding module 2 (Failure)
	STRICT_EXPECTED_CALL(mocks, VECTOR_element(dummyProps->gateway_properties_entries, 1));
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Load(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_GetModuleAPIs(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, NULL))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, MessageBus_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, MessageBus_IncRef(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	whenShallVECTOR_push_back_fail = 2;
	STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, MessageBus_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, MessageBus_DecRef(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Unload(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	//Removing previous module
	STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, MessageBus_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, MessageBus_DecRef(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_GetModuleAPIs(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Unload(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, MessageBus_Destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	GATEWAY_HANDLE gateway = Gateway_LL_Create(dummyProps);

	ASSERT_IS_NULL(gateway);
	ASSERT_ARE_EQUAL(size_t, 0, currentMessageBus_module_count);
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	Gateway_LL_Destroy(gateway);

}

/*Tests_SRS_GATEWAY_LL_14_036: [ If any MODULE_HANDLE is unable to be created from a GATEWAY_PROPERTIES_ENTRY the GATEWAY_HANDLE will be destroyed. ]*/
TEST_FUNCTION(Gateway_LL_Create_MessageBus_AddModule_Fails_To_Add_All_Modules_In_Props)
{
	//Arrange
	CGatewayLLMocks mocks;

	//Add another entry to the properties
	GATEWAY_PROPERTIES_ENTRY dummyEntry2 = {
		"dummy module 2",
		"x2.dll",
		NULL
	};

	BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_properties_entries, &dummyEntry2, 1);

	//Expectations
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, MessageBus_Create());
	STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_size(dummyProps->gateway_properties_entries));

	//Adding module 1 (Success)
	STRICT_EXPECTED_CALL(mocks, VECTOR_element(dummyProps->gateway_properties_entries, 0));
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Load(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_GetModuleAPIs(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, NULL))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, MessageBus_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, MessageBus_IncRef(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1)
		.IgnoreArgument(2);

	//Adding module 2 (Failure)
	STRICT_EXPECTED_CALL(mocks, VECTOR_element(dummyProps->gateway_properties_entries, 1));
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Load(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_GetModuleAPIs(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, NULL))
		.IgnoreArgument(1);
	whenShallMessageBus_AddModule_fail = 2;
	STRICT_EXPECTED_CALL(mocks, MessageBus_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Unload(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	//Removing previous module
	STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, MessageBus_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, MessageBus_DecRef(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_GetModuleAPIs(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Unload(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, MessageBus_Destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	GATEWAY_HANDLE gateway = Gateway_LL_Create(dummyProps);

	ASSERT_IS_NULL(gateway);
	ASSERT_ARE_EQUAL(size_t, 0, currentMessageBus_module_count);
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	Gateway_LL_Destroy(gateway);

}

/*Tests_SRS_GATEWAY_LL_14_009: [ The function shall use each GATEWAY_PROPERTIES_ENTRY use each of GATEWAY_PROPERTIES's gateway_properties_entries to create and add a module to the GATEWAY_HANDLE message bus. ]*/
TEST_FUNCTION(Gateway_LL_Create_Adds_All_Modules_In_Props_Success)
{
	//Arrange
	CGatewayLLMocks mocks;

	//Add another entry to the properties
	GATEWAY_PROPERTIES_ENTRY dummyEntry2 = {
		"dummy module 2",
		"x2.dll",
		NULL
	};

	BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_properties_entries, &dummyEntry2, 1);

	//Expectations
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, MessageBus_Create());
	STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_size(dummyProps->gateway_properties_entries));

	//Adding module 1 (Failure)
	STRICT_EXPECTED_CALL(mocks, VECTOR_element(dummyProps->gateway_properties_entries, 0));
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Load(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_GetModuleAPIs(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, NULL))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, MessageBus_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, MessageBus_IncRef(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1)
		.IgnoreArgument(2);

	//Adding module 2 (Success)
	STRICT_EXPECTED_CALL(mocks, VECTOR_element(dummyProps->gateway_properties_entries, 1));
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Load(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_GetModuleAPIs(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, NULL))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, MessageBus_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, MessageBus_IncRef(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1)
		.IgnoreArgument(2);

	//Act
	GATEWAY_HANDLE gateway = Gateway_LL_Create(dummyProps);

	//Assert
	ASSERT_IS_NOT_NULL(gateway);
	ASSERT_ARE_EQUAL(size_t, 2, currentMessageBus_module_count);
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	Gateway_LL_Destroy(gateway);
}

/*Tests_SRS_GATEWAY_LL_14_005: [ If gw is NULL the function shall do nothing. ]*/
TEST_FUNCTION(Gateway_LL_Destroy_Does_Nothing_If_NULL)
{
	//Arrange
	CGatewayLLMocks mocks;

	GATEWAY_HANDLE gateway = NULL;

	//Act
	Gateway_LL_Destroy(gateway);

	//Assert
	mocks.AssertActualAndExpectedCalls();
}

/*Tests_SRS_GATEWAY_LL_14_037: [ If GATEWAY_HANDLE_DATA's message bus cannot unlink module, the function shall log the error and continue unloading the module from the GATEWAY_HANDLE. ]*/
TEST_FUNCTION(Gateway_LL_Destroy_Continues_Unloading_If_MessageBus_RemoveModule_Fails)
{
	//Arrange
	CGatewayLLMocks mocks;

	//Add another entry to the properties
	GATEWAY_PROPERTIES_ENTRY dummyEntry2 = {
		"dummy module 2",
		"x2.dll",
		NULL
	};

	BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_properties_entries, &dummyEntry2, 1);

	GATEWAY_HANDLE gateway = Gateway_LL_Create(dummyProps);
	mocks.ResetAllCalls();

	//Expectations
	STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	whenShallMessageBus_RemoveModule_fail = 1;
	STRICT_EXPECTED_CALL(mocks, MessageBus_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, MessageBus_DecRef(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_GetModuleAPIs(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Unload(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1)
		.IgnoreArgument(2);

	STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, MessageBus_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, MessageBus_DecRef(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_GetModuleAPIs(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Unload(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1)
		.IgnoreArgument(2);

	STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, MessageBus_Destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	//Act
	Gateway_LL_Destroy(gateway);

	//Assert
	mocks.AssertActualAndExpectedCalls();
}

/*Tests_SRS_GATEWAY_LL_14_028: [ The function shall remove each module in GATEWAY_HANDLE_DATA's modules vector and destroy GATEWAY_HANDLE_DATA's modules. ]*/
/*Tests_SRS_GATEWAY_LL_14_006: [ The function shall destroy the GATEWAY_HANDLE_DATA's bus MESSAGE_BUS_HANDLE. ]*/
TEST_FUNCTION(Gateway_LL_Destroy_Removes_All_Modules_And_Destroys_Vector_Success)
{
	//Arrange
	CGatewayLLMocks mocks;

	//Add another entry to the properties
	GATEWAY_PROPERTIES_ENTRY dummyEntry2 = {
		"dummy module 2",
		"x2.dll",
		NULL
	};

	BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_properties_entries, &dummyEntry2, 1);

	GATEWAY_HANDLE gateway = Gateway_LL_Create(dummyProps);
	mocks.ResetAllCalls();

	//Gateway_LL_Destroy Expectations
	STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, MessageBus_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, MessageBus_DecRef(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_GetModuleAPIs(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Unload(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1)
		.IgnoreArgument(2);

	STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, MessageBus_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, MessageBus_DecRef(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_GetModuleAPIs(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Unload(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1)
		.IgnoreArgument(2);

	STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, MessageBus_Destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	//Act
	Gateway_LL_Destroy(gateway);

	//Assert
	ASSERT_ARE_EQUAL(size_t, 0, currentMessageBus_module_count);
	mocks.AssertActualAndExpectedCalls();
}

/*Tests_SRS_GATEWAY_LL_14_011: [ If gw, entry, or GATEWAY_PROPERTIES_ENTRY's module_path is NULL the function shall return NULL. ]*/
TEST_FUNCTION(Gateway_LL_AddModule_Returns_Null_For_Null_Gateway)
{
	//Arrange
	CGatewayLLMocks mocks;

	GATEWAY_HANDLE gw = Gateway_LL_Create(NULL);
	mocks.ResetAllCalls();

	//Act
	MODULE_HANDLE handle0 = Gateway_LL_AddModule(NULL, (GATEWAY_PROPERTIES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_properties_entries));
	
	//Assert
	ASSERT_IS_NULL(handle0);
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	Gateway_LL_Destroy(gw);
}

/*Tests_SRS_GATEWAY_LL_14_011: [ If gw, entry, or GATEWAY_PROPERTIES_ENTRY's module_path is NULL the function shall return NULL. ]*/
TEST_FUNCTION(Gateway_LL_AddModule_Returns_Null_For_Null_Module)
{
	//Arrange
	CGatewayLLMocks mocks;

	GATEWAY_HANDLE gw = Gateway_LL_Create(NULL);
	mocks.ResetAllCalls();

	//Act
	MODULE_HANDLE handle1 = Gateway_LL_AddModule(gw, NULL);

	//Assert
	ASSERT_IS_NULL(handle1);
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	Gateway_LL_Destroy(gw);
}

/*Tests_SRS_GATEWAY_LL_14_011: [ If gw, entry, or GATEWAY_PROPERTIES_ENTRY's module_path is NULL the function shall return NULL. ]*/
TEST_FUNCTION(Gateway_LL_AddModule_Returns_Null_For_Null_Params)
{
	//Arrange
	CGatewayLLMocks mocks;

	GATEWAY_HANDLE gw = Gateway_LL_Create(NULL);
	mocks.ResetAllCalls();

	//Act
	GATEWAY_PROPERTIES_ENTRY* entry = (GATEWAY_PROPERTIES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_properties_entries);
	entry->module_path = NULL;
	MODULE_HANDLE handle2 = Gateway_LL_AddModule(gw, (GATEWAY_PROPERTIES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_properties_entries));

	//Assert
	ASSERT_IS_NULL(handle2);
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	Gateway_LL_Destroy(gw);
}

/*Tests_SRS_GATEWAY_LL_14_012: [ The function shall load the module located at GATEWAY_PROPERTIES_ENTRY's module_path into a MODULE_LIBRARY_HANDLE. ]*/
/*Tests_SRS_GATEWAY_LL_14_013: [ The function shall get the const MODULE_APIS* from the MODULE_LIBRARY_HANDLE. ]*/
/*Tests_SRS_GATEWAY_LL_14_017: [ The function shall link the module to the GATEWAY_HANDLE_DATA's bus using a call to MessageBus_AddModule. ]*/
/*Tests_SRS_GATEWAY_LL_14_029: [ The function shall create a new MODULE_DATA containting the MODULE_HANDLE and MODULE_LIBRARY_HANDLE if the module was successfully linked to the message bus. ]*/
/*Tests_SRS_GATEWAY_LL_14_032: [ The function shall add the new MODULE_DATA to GATEWAY_HANDLE_DATA's modules if the module was successfully linked to the message bus. ]*/
/*Tests_SRS_GATEWAY_LL_14_019: [ The function shall return the newly created MODULE_HANDLE only if each API call returns successfully. ]*/
/*Tests_SRS_GATEWAY_LL_14_039: [ The function shall increment the MESSAGE_BUS_HANDLE reference count if the MODULE_HANDLE was successfully linked to the GATEWAY_HANDLE_DATA's bus. ]*/
/*Tests_SRS_GATEWAY_LL_99_011: [ The function shall assign `module_apis` to `MODULE::module_apis`. ]*/
TEST_FUNCTION(Gateway_LL_AddModule_Loads_Module_From_Library_Path)
{
	//Arrange
	CGatewayLLMocks mocks;

	GATEWAY_HANDLE gw = Gateway_LL_Create(NULL);
	mocks.ResetAllCalls();

	//Expectations
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Load(DUMMY_LIBRARY_PATH));
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_GetModuleAPIs(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, MessageBus_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, MessageBus_IncRef(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1)
		.IgnoreArgument(2);

	//Act
	MODULE_HANDLE handle = Gateway_LL_AddModule(gw, (GATEWAY_PROPERTIES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_properties_entries));

	//Assert
	ASSERT_IS_NOT_NULL(handle);
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	Gateway_LL_Destroy(gw);
}

/*Tests_SRS_GATEWAY_LL_14_031: [ If unsuccessful, the function shall return NULL. ]*/
TEST_FUNCTION(Gateway_LL_AddModule_Fails)
{
	//Arrange
	CGatewayLLMocks mocks;

	GATEWAY_HANDLE gw = Gateway_LL_Create(NULL);
	mocks.ResetActualCalls();

	//Expectations
	whenShallModuleLoader_Load_fail = 1;
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Load(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	MODULE_HANDLE handle = Gateway_LL_AddModule(gw, (GATEWAY_PROPERTIES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_properties_entries));

	//Assert
	ASSERT_IS_NULL(handle);
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	Gateway_LL_Destroy(gw);
}

/*Tests_SRS_GATEWAY_LL_14_015: [ The function shall use the MODULE_APIS to create a MODULE_HANDLE using the GATEWAY_PROPERTIES_ENTRY's module_properties. ]*/
/*Tests_SRS_GATEWAY_LL_14_039: [ The function shall increment the MESSAGE_BUS_HANDLE reference count if the MODULE_HANDLE was successfully linked to the GATEWAY_HANDLE_DATA's bus. ]*/
TEST_FUNCTION(Gateway_LL_AddModule_Creates_Module_Using_Module_Properties)
{
	//Arrange
	CGatewayLLMocks mocks;

	GATEWAY_HANDLE gw = Gateway_LL_Create(NULL);
	mocks.ResetAllCalls();
	bool* properties = (bool*)malloc(sizeof(bool));
	*properties = true;
	GATEWAY_PROPERTIES_ENTRY entry = {
		"Test module",
		DUMMY_LIBRARY_PATH,
		properties
	};

	//Expectations
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Load(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_GetModuleAPIs(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, properties))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, MessageBus_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, MessageBus_IncRef(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1)
		.IgnoreArgument(2);

	//Act
	MODULE_HANDLE handle = Gateway_LL_AddModule(gw, &entry);

	//Assert
	ASSERT_IS_NOT_NULL(handle);
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	Gateway_LL_Destroy(gw);
	free(properties);
}

/*Tests_SRS_GATEWAY_LL_14_016: [ If the module creation is unsuccessful, the function shall return NULL. ]*/
TEST_FUNCTION(Gateway_LL_AddModule_Module_Create_Fails)
{
	//Arrange
	CGatewayLLMocks mocks;

	GATEWAY_HANDLE gw = Gateway_LL_Create(NULL);
	mocks.ResetAllCalls();
	//Setting this boolean to false will cause mock_Module_Create to fail
	bool properties = false;
	GATEWAY_PROPERTIES_ENTRY entry = {
		"Test module",
		DUMMY_LIBRARY_PATH,
		&properties
	};

	//Expectations
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Load(DUMMY_LIBRARY_PATH));
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_GetModuleAPIs(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Unload(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	//Act
	MODULE_HANDLE handle = Gateway_LL_AddModule(gw, &entry);

	//Assert
	ASSERT_IS_NULL(handle);
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	Gateway_LL_Destroy(gw);
}

/*Tests_SRS_GATEWAY_LL_14_018: [ If the message bus linking is unsuccessful, the function shall return NULL. ]*/
TEST_FUNCTION(Gateway_LL_AddModule_MessageBus_AddModule_Fails)
{
	//Arrange
	CGatewayLLMocks mocks;

	GATEWAY_HANDLE gw = Gateway_LL_Create(NULL);
	mocks.ResetAllCalls();
	
	//Expectations
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Load(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_GetModuleAPIs(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	whenShallMessageBus_AddModule_fail = 1;
	STRICT_EXPECTED_CALL(mocks, MessageBus_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Unload(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	

	//Act
	MODULE_HANDLE handle = Gateway_LL_AddModule(gw, (GATEWAY_PROPERTIES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_properties_entries));

	ASSERT_IS_NULL(handle);
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	Gateway_LL_Destroy(gw);
}

/*Tests_SRS_GATEWAY_LL_14_030: [ If any internal API call is unsuccessful after a module is created, the library will be unloaded and the module destroyed. ]*/
/*Tests_SRS_GATEWAY_LL_14_039: [The function shall increment the MESSAGE_BUS_HANDLE reference count if the MODULE_HANDLE was successfully linked to the GATEWAY_HANDLE_DATA's bus. ]*/
TEST_FUNCTION(Gateway_LL_AddModule_Internal_API_Fail_Rollback_Module)
{
	//Arrange
	CGatewayLLMocks mocks;

	GATEWAY_HANDLE gw = Gateway_LL_Create(NULL);
	mocks.ResetAllCalls();

	//Expectations
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Load(DUMMY_LIBRARY_PATH));
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_GetModuleAPIs(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, NULL))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, MessageBus_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, MessageBus_IncRef(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	whenShallVECTOR_push_back_fail = 1;
	STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, MessageBus_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, MessageBus_DecRef(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Unload(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	//Act
	MODULE_HANDLE handle = Gateway_LL_AddModule(gw, (GATEWAY_PROPERTIES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_properties_entries));

	ASSERT_IS_NULL(handle);
	ASSERT_ARE_EQUAL(size_t, 0, currentMessageBus_module_count);
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	Gateway_LL_Destroy(gw);
}

/*Tests_SRS_GATEWAY_LL_14_020: [ If gw or module is NULL the function shall return. ]*/
TEST_FUNCTION(Gateway_LL_RemoveModule_Does_Nothing_If_Gateway_NULL)
{
	//Arrange
	CGatewayLLMocks mocks;
	GATEWAY_HANDLE gw = Gateway_LL_Create(NULL);
	MODULE_HANDLE handle = Gateway_LL_AddModule(gw, (GATEWAY_PROPERTIES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_properties_entries));
	mocks.ResetAllCalls();

	//Act
	Gateway_LL_RemoveModule(NULL, NULL);
	Gateway_LL_RemoveModule(NULL, handle);
	
	//Assert
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	Gateway_LL_Destroy(gw);
}

/*Tests_SRS_GATEWAY_LL_14_020: [ If gw or module is NULL the function shall return. ]*/
TEST_FUNCTION(Gateway_LL_RemoveModule_Does_Nothing_If_Module_NULL)
{
	//Arrange
	CGatewayLLMocks mocks;
	GATEWAY_HANDLE gw = Gateway_LL_Create(NULL);
	mocks.ResetAllCalls();

	//Expectations
	STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	//Act
	Gateway_LL_RemoveModule(gw, NULL);

	//Assert
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	Gateway_LL_Destroy(gw);
}

/*Tests_SRS_GATEWAY_LL_14_023: [ The function shall locate the MODULE_DATA object in GATEWAY_HANDLE_DATA's modules containing module and return if it cannot be found. ]*/
/*Tests_SRS_GATEWAY_LL_14_021: [ The function shall unlink module from the GATEWAY_HANDLE_DATA's bus MESSAGE_BUS_HANDLE. ]*/
/*Tests_SRS_GATEWAY_LL_14_024: [ The function shall use the MODULE_DATA's module_library_handle to retrieve the MODULE_APIS and destroy module. ]*/
/*Tests_SRS_GATEWAY_LL_14_025: [ The function shall unload MODULE_DATA's module_library_handle. ]*/
/*Tests_SRS_GATEWAY_LL_14_026: [ The function shall remove that MODULE_DATA from GATEWAY_HANDLE_DATA's modules. ]*/
/*Tests_SRS_GATEWAY_LL_14_038: [ The function shall decrement the MESSAGE_BUS_HANDLE reference count. ]*/
TEST_FUNCTION(Gateway_LL_RemoveModule_Finds_Module_Data_Success)
{
	//Arrange
	CGatewayLLMocks mocks;
	GATEWAY_HANDLE gw = Gateway_LL_Create(NULL);
	MODULE_HANDLE handle = Gateway_LL_AddModule(gw, (GATEWAY_PROPERTIES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_properties_entries));
	mocks.ResetAllCalls();

	//Expectations
	STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(mocks, MessageBus_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(mocks, MessageBus_DecRef(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_GetModuleAPIs(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(handle))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Unload(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	

	//Act
	Gateway_LL_RemoveModule(gw, handle);

	//Assert
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	Gateway_LL_Destroy(gw);
}

/*Tests_SRS_GATEWAY_LL_14_023: [ The function shall locate the MODULE_DATA object in GATEWAY_HANDLE_DATA's modules containing module and return if it cannot be found. ]*/
TEST_FUNCTION(Gateway_LL_RemoveModule_Finds_Module_Data_Failure)
{
	//Arrange
	CGatewayLLMocks mocks;
	GATEWAY_HANDLE gw = Gateway_LL_Create(NULL);
	MODULE_HANDLE handle = Gateway_LL_AddModule(gw, (GATEWAY_PROPERTIES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_properties_entries));
	mocks.ResetAllCalls();

	//Expectations
	STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, gw))
		.IgnoreAllArguments();

	//Act
	Gateway_LL_RemoveModule(gw, (MODULE_HANDLE)gw);

	//Assert
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	Gateway_LL_Destroy(gw);
}

/*Tests_SRS_GATEWAY_LL_14_022: [ If GATEWAY_HANDLE_DATA's bus cannot unlink module, the function shall log the error and continue unloading the module from the GATEWAY_HANDLE. ]*/
/*Tests_SRS_GATEWAY_LL_14_038: [ The function shall decrement the MESSAGE_BUS_HANDLE reference count. ]*/
TEST_FUNCTION(Gateway_LL_RemoveModule_MessageBus_RemoveModule_Failure)
{
	//Arrange
	CGatewayLLMocks mocks;
	GATEWAY_HANDLE gw = Gateway_LL_Create(NULL);
	MODULE_HANDLE handle = Gateway_LL_AddModule(gw, (GATEWAY_PROPERTIES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_properties_entries));

	mocks.ResetAllCalls();

	//Expectations
	STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	whenShallMessageBus_RemoveModule_fail = 1;
	STRICT_EXPECTED_CALL(mocks, MessageBus_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, MessageBus_DecRef(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_GetModuleAPIs(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Unload(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1)
		.IgnoreArgument(2);

	//Act
	Gateway_LL_RemoveModule(gw, handle);

	//Assert
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	Gateway_LL_Destroy(gw);
}

END_TEST_SUITE(gateway_ll_unittests)
