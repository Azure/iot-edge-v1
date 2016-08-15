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
#include "broker.h"
#include "module_loader.h"
#include "internal/event_system.h"

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

static size_t currentBroker_AddModule_call;
static size_t whenShallBroker_AddModule_fail;
static size_t currentBroker_RemoveModule_call;
static size_t whenShallBroker_RemoveModule_fail;
static size_t currentBroker_Create_call;
static size_t whenShallBroker_Create_fail;
static size_t currentBroker_module_count;
static size_t currentBroker_ref_count;

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

class DummyGatewayModule : public IInternalGatewayModule
{
public:
    void Module_Receive(MESSAGE_HANDLE messageHandle) {}
};
static MODULE dummyModule;
static VECTOR_HANDLE dummyModules;


TYPED_MOCK_CLASS(CGatewayUwpLLMocks, CGlobalMock)
{
public:
	MOCK_STATIC_METHOD_2(, MODULE_HANDLE, mock_Module_Create, BROKER_HANDLE, broker, const void*, configuration)
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

	MOCK_STATIC_METHOD_1(, void, Broker_DecRef, BROKER_HANDLE, broker)
		if (currentBroker_ref_count > 0)
		{
			--currentBroker_ref_count;
			if (currentBroker_ref_count == 0)
			{
				BASEIMPLEMENTATION::gballoc_free(broker);
			}
		}
	MOCK_VOID_METHOD_END();

	MOCK_STATIC_METHOD_1(, void, Broker_IncRef, BROKER_HANDLE, broker)
		++currentBroker_ref_count;
	MOCK_VOID_METHOD_END();

	MOCK_STATIC_METHOD_0(, BROKER_HANDLE, Broker_Create)
	BROKER_HANDLE result1;
	currentBroker_Create_call++;
	if (whenShallBroker_Create_fail >= 0 && whenShallBroker_Create_fail == currentBroker_Create_call)
	{
		result1 = NULL;
	}
	else
	{
		++currentBroker_ref_count;
		result1 = (BROKER_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1);
	}
	MOCK_METHOD_END(BROKER_HANDLE, result1);

	MOCK_STATIC_METHOD_1(, void, Broker_Destroy, BROKER_HANDLE, broker)
		if (currentBroker_ref_count > 0)
		{
			--currentBroker_ref_count;
			if (currentBroker_ref_count == 0)
			{
				BASEIMPLEMENTATION::gballoc_free(broker);
			}
		}
	MOCK_VOID_METHOD_END();

	MOCK_STATIC_METHOD_2(, BROKER_RESULT, Broker_AddModule, BROKER_HANDLE, handle, const MODULE*, module)
		currentBroker_AddModule_call++;
		BROKER_RESULT result1  = BROKER_ERROR;
		if (handle != NULL && module != NULL)
		{
			if (whenShallBroker_AddModule_fail != currentBroker_AddModule_call)
			{
				++currentBroker_module_count;
				result1 = BROKER_OK;
			}
		}
	MOCK_METHOD_END(BROKER_RESULT, result1);

	MOCK_STATIC_METHOD_2(, BROKER_RESULT, Broker_RemoveModule, BROKER_HANDLE, handle, const MODULE*, module)
		currentBroker_RemoveModule_call++;
		BROKER_RESULT result1 = BROKER_ERROR;
		if (handle != NULL && module != NULL && currentBroker_module_count > 0 && whenShallBroker_RemoveModule_fail != currentBroker_RemoveModule_call)
		{
			--currentBroker_module_count;
			result1 = BROKER_OK;
		}
	MOCK_METHOD_END(BROKER_RESULT, result1);

	MOCK_STATIC_METHOD_1(, MODULE_LIBRARY_HANDLE, ModuleLoader_Load, const char*, moduleLibraryFileName)
		currentModuleLoader_Load_call++;
	MODULE_LIBRARY_HANDLE handle = NULL;
	if (whenShallModuleLoader_Load_fail >= 0 && whenShallModuleLoader_Load_fail != currentModuleLoader_Load_call)
	{
		handle = (MODULE_LIBRARY_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1);
	}
	MOCK_METHOD_END(MODULE_LIBRARY_HANDLE, handle);

	MOCK_STATIC_METHOD_1(, void, ModuleLoader_Unload, MODULE_LIBRARY_HANDLE, moduleLibraryHandle)
		BASEIMPLEMENTATION::gballoc_free(moduleLibraryHandle);
	MOCK_VOID_METHOD_END();

	MOCK_STATIC_METHOD_0(, EVENTSYSTEM_HANDLE, EventSystem_Init)
	MOCK_METHOD_END(EVENTSYSTEM_HANDLE, (EVENTSYSTEM_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1));

	MOCK_STATIC_METHOD_3(, void, EventSystem_AddEventCallback, EVENTSYSTEM_HANDLE, event_system, GATEWAY_EVENT, event_type, GATEWAY_CALLBACK, callback)
		// no-op
	MOCK_VOID_METHOD_END();

	MOCK_STATIC_METHOD_3(, void, EventSystem_ReportEvent, EVENTSYSTEM_HANDLE, event_system, GATEWAY_HANDLE, gw, GATEWAY_EVENT, event_type)
		// no-op
	MOCK_VOID_METHOD_END();

	MOCK_STATIC_METHOD_1(, void, EventSystem_Destroy, EVENTSYSTEM_HANDLE, handle)
		BASEIMPLEMENTATION::gballoc_free(handle);
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

DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayUwpLLMocks, , MODULE_HANDLE, mock_Module_Create, BROKER_HANDLE, broker, const void*, configuration);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayUwpLLMocks, , void, mock_Module_Destroy, MODULE_HANDLE, moduleHandle);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayUwpLLMocks, , void, mock_Module_Receive, MODULE_HANDLE, moduleHandle, MESSAGE_HANDLE, messageHandle);

DECLARE_GLOBAL_MOCK_METHOD_0(CGatewayUwpLLMocks, , BROKER_HANDLE, Broker_Create);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayUwpLLMocks, , void, Broker_Destroy, BROKER_HANDLE, broker);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayUwpLLMocks, , BROKER_RESULT, Broker_AddModule, BROKER_HANDLE, handle, const MODULE*, module);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayUwpLLMocks, , BROKER_RESULT, Broker_RemoveModule, BROKER_HANDLE, handle, const MODULE*, module);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayUwpLLMocks, , void, Broker_IncRef, BROKER_HANDLE, broker);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayUwpLLMocks, , void, Broker_DecRef, BROKER_HANDLE, broker);

DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayUwpLLMocks, , MODULE_LIBRARY_HANDLE, ModuleLoader_Load, const char*, moduleLibraryFileName);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayUwpLLMocks, , void, ModuleLoader_Unload, MODULE_LIBRARY_HANDLE, moduleLibraryHandle);

DECLARE_GLOBAL_MOCK_METHOD_0(CGatewayUwpLLMocks, , EVENTSYSTEM_HANDLE, EventSystem_Init);
DECLARE_GLOBAL_MOCK_METHOD_3(CGatewayUwpLLMocks, , void, EventSystem_AddEventCallback, EVENTSYSTEM_HANDLE, event_system, GATEWAY_EVENT, event_type, GATEWAY_CALLBACK, callback);
DECLARE_GLOBAL_MOCK_METHOD_3(CGatewayUwpLLMocks, , void, EventSystem_ReportEvent, EVENTSYSTEM_HANDLE, event_system, GATEWAY_HANDLE, gw, GATEWAY_EVENT, event_type);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayUwpLLMocks, , void, EventSystem_Destroy, EVENTSYSTEM_HANDLE, handle);

DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayUwpLLMocks, , VECTOR_HANDLE, VECTOR_create, size_t, elementSize);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayUwpLLMocks, , void, VECTOR_destroy, VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_3(CGatewayUwpLLMocks, , int, VECTOR_push_back, VECTOR_HANDLE, handle, const void*, elements, size_t, numElements);
DECLARE_GLOBAL_MOCK_METHOD_3(CGatewayUwpLLMocks, , void, VECTOR_erase, VECTOR_HANDLE, handle, void*, elements, size_t, index);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayUwpLLMocks, , void*, VECTOR_element, const VECTOR_HANDLE, handle, size_t, index);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayUwpLLMocks, , void*, VECTOR_front, const VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayUwpLLMocks, , size_t, VECTOR_size, const VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_3(CGatewayUwpLLMocks, , void*, VECTOR_find_if, const VECTOR_HANDLE, handle, PREDICATE_FUNCTION, pred, const void*, value);

DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayUwpLLMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayUwpLLMocks, , void*, gballoc_realloc, void*, ptr, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayUwpLLMocks, , void, gballoc_free, void*, ptr)

static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;
static MICROMOCK_MUTEX_HANDLE g_testByTest;

BEGIN_TEST_SUITE(gateway_ll_uwp_ut)

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

	currentBroker_AddModule_call = 0;
	whenShallBroker_AddModule_fail = 0;
	currentBroker_RemoveModule_call = 0;
	whenShallBroker_RemoveModule_fail = 0;
	currentBroker_Create_call = 0;
	whenShallBroker_Create_fail = 0;
	currentBroker_module_count = 0;
	currentBroker_ref_count = 0;

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

	IInternalGatewayModule *iigm = new DummyGatewayModule;
	dummyModule.module_instance = iigm;

	dummyModules = BASEIMPLEMENTATION::VECTOR_create(sizeof(MODULE));
	BASEIMPLEMENTATION::VECTOR_push_back(dummyModules, &dummyModule, 1);

}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
	if (!MicroMockReleaseMutex(g_testByTest))
	{
		ASSERT_FAIL("failure in test framework at ReleaseMutex");
	}
	currentmalloc_call = 0;
	whenShallmalloc_fail = 0;

	currentBroker_Create_call = 0;
	whenShallBroker_Create_fail = 0;

	delete dummyModule.module_instance;

	BASEIMPLEMENTATION::VECTOR_destroy(dummyModules);
}

/*Tests_SRS_GATEWAY_LL_99_001: [This function shall create a GATEWAY_HANDLE representing the newly created gateway.]*/
/*Tests_SRS_GATEWAY_LL_99_005: [The function shall increment the BROKER_HANDLE reference count if the MODULE_HANDLE was successfully linked to the GATEWAY_HANDLE_DATA's message broker.]*/
TEST_FUNCTION(Gateway_LL_UwpCreate_Creates_Handle_Success)
{
	//Arrange
	CGatewayUwpLLMocks mocks;

	BROKER_HANDLE broker = Broker_Create();
	mocks.ResetAllCalls();

	//Expectations
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_size(dummyModules));
	STRICT_EXPECTED_CALL(mocks, VECTOR_element(dummyModules, 0));
	STRICT_EXPECTED_CALL(mocks, Broker_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, Broker_IncRef(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	//Act
	GATEWAY_HANDLE gateway = Gateway_LL_UwpCreate(dummyModules, broker);

	//Assert
	ASSERT_IS_NOT_NULL(gateway);
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	Gateway_LL_UwpDestroy(gateway);
}

/*Tests_SRS_GATEWAY_LL_99_002: [This function shall return NULL upon any memory allocation failure.]*/
TEST_FUNCTION(Gateway_LL_UwpCreate_Creates_Handle_Malloc_Failure)
{
	//Arrange
	CGatewayUwpLLMocks mocks;

	//Expectations
	whenShallmalloc_fail = 1;
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);

	//Act
	GATEWAY_HANDLE gateway = Gateway_LL_UwpCreate(IGNORED_PTR_ARG, IGNORED_PTR_ARG);

	//Assert
	ASSERT_IS_NULL(gateway);
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	//Nothing to cleanup
}

/*Tests_SRS_GATEWAY_LL_99_003: [ This function shall return `NULL` if a `NULL` `BROKER_HANDLE` is received. ]*/
TEST_FUNCTION(Gateway_LL_UwpCreate_Null_Broker_Handle_Failure)
{
	//Arrange
	CGatewayUwpLLMocks mocks;

	//Expectations
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	GATEWAY_HANDLE gateway = Gateway_LL_UwpCreate(IGNORED_PTR_ARG, NULL);

	//Assert
	ASSERT_IS_NULL(gateway);
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	//Nothing to cleanup
}

/*Tests_SRS_GATEWAY_LL_99_004: [ This function shall return `NULL` if a `NULL` `VECTOR_HANDLE` is received. ]*/
TEST_FUNCTION(Gateway_LL_UwpCreate_Null_Vector_Handle_Failure)
{
	//Arrange
	CGatewayUwpLLMocks mocks;

	//Expectations
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	GATEWAY_HANDLE gateway = Gateway_LL_UwpCreate(NULL, IGNORED_PTR_ARG);

	//Assert
	ASSERT_IS_NULL(gateway);
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	//Nothing to cleanup
}


/*Tests_SRS_GATEWAY_LL_99_006: [ If gw is NULL the function shall do nothing. ]*/
TEST_FUNCTION(Gateway_LL_UwpDestroy_Does_Nothing_If_NULL)
{
	//Arrange
	CGatewayUwpLLMocks mocks;

	GATEWAY_HANDLE gateway = NULL;

	//Act
	Gateway_LL_UwpDestroy(gateway);

	//Assert
	mocks.AssertActualAndExpectedCalls();
}


/*Tests_SRS_GATEWAY_LL_99_010: [ The function shall destroy the GATEWAY_HANDLE_DATA's broker BROKER_HANDLE. ]*/
/*Tests_SRS_GATEWAY_LL_99_007: [ The function shall detach modules from the GATEWAY_HANDLE_DATA's broker BROKER_HANDLE. ]*/
/*Tests_SRS_GATEWAY_LL_99_009: [ The function shall decrement the BROKER_HANDLE reference count. ]*/
TEST_FUNCTION(Gateway_LL_UwpDestroy_Removes_All_Modules_And_Destroys_Broker_Success)
{
	//Arrange
	CGatewayUwpLLMocks mocks;

	BROKER_HANDLE broker = Broker_Create();
	GATEWAY_HANDLE gateway = Gateway_LL_UwpCreate(dummyModules, broker);
	mocks.ResetAllCalls();

	//Expectations
	STRICT_EXPECTED_CALL(mocks, VECTOR_size(dummyModules));
	STRICT_EXPECTED_CALL(mocks, VECTOR_element(dummyModules, 0));
	STRICT_EXPECTED_CALL(mocks, Broker_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, Broker_DecRef(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(mocks, Broker_Destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	//Act
	Gateway_LL_UwpDestroy(gateway);

	//Assert
	mocks.AssertActualAndExpectedCalls();
}

/*Tests_SRS_GATEWAY_LL_99_008: [ If GATEWAY_HANDLE_DATA's broker cannot detach a module, the function shall log the error and continue unloading the module from the GATEWAY_HANDLE. ]*/
TEST_FUNCTION(Gateway_LL_UwpDestroy_Continues_Unloading_If_Broker_RemoveModule_Fails)
{
	//Arrange
	CGatewayUwpLLMocks mocks;

	BROKER_HANDLE broker = Broker_Create();
	GATEWAY_HANDLE gateway = Gateway_LL_UwpCreate(dummyModules, broker);
	mocks.ResetAllCalls();

	//Expectations
	STRICT_EXPECTED_CALL(mocks, VECTOR_size(dummyModules));
	STRICT_EXPECTED_CALL(mocks, VECTOR_element(dummyModules, 0));
	whenShallBroker_RemoveModule_fail = 1;
	STRICT_EXPECTED_CALL(mocks, Broker_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, Broker_DecRef(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(mocks, Broker_Destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	//Act
	Gateway_LL_UwpDestroy(gateway);

	//Assert
	mocks.AssertActualAndExpectedCalls();
}

END_TEST_SUITE(gateway_ll_uwp_ut)
