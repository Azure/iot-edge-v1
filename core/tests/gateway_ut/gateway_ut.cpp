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

#include "gateway.h"
#include "broker.h"
#include "experimental/event_system.h"
#include "module_loader.h"

#define DUMMY_LIBRARY_PATH "x.dll"

#define GBALLOC_H

DEFINE_MICROMOCK_ENUM_TO_STRING(GATEWAY_ADD_LINK_RESULT, GATEWAY_ADD_LINK_RESULT_VALUES);
DEFINE_MICROMOCK_ENUM_TO_STRING(GATEWAY_START_RESULT, GATEWAY_START_RESULT_VALUES);
DEFINE_MICROMOCK_ENUM_TO_STRING(MODULE_LOADER_RESULT, MODULE_LOADER_RESULT_VALUES);

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


static size_t currentVECTOR_create_call;
static size_t whenShallVECTOR_create_fail;
static size_t currentVECTOR_push_back_call;
static size_t whenShallVECTOR_push_back_fail;
static size_t currentVECTOR_find_if_call;
static size_t whenShallVECTOR_find_if_fail;

static MODULE_API_1 dummyAPIs;

TYPED_MOCK_CLASS(CGatewayLLMocks, CGlobalMock)
{
public:
    MOCK_STATIC_METHOD_1(, void*, mock_Module_ParseConfigurationFromJson, const char*, configuration)
        void* result1;
        result1 = BASEIMPLEMENTATION::gballoc_malloc(1);
    MOCK_METHOD_END(MODULE_HANDLE, result1);

	MOCK_STATIC_METHOD_1(, void, mock_Module_FreeConfiguration, void*, configuration)
		BASEIMPLEMENTATION::gballoc_free(configuration);
	MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_2(, MODULE_HANDLE, mock_Module_Create, BROKER_HANDLE, broker, const void*, configuration)
        MODULE_HANDLE result1;
        result1 = (MODULE_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1);
    MOCK_METHOD_END(MODULE_HANDLE, result1);

    MOCK_STATIC_METHOD_1(, void, mock_Module_Destroy, MODULE_HANDLE, moduleHandle)
        BASEIMPLEMENTATION::gballoc_free(moduleHandle);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_2(, void, mock_Module_Receive, MODULE_HANDLE, moduleHandle, MESSAGE_HANDLE, messageHandle)
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_1(, void, mock_Module_Start, MODULE_HANDLE, moduleHandle)
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

    MOCK_STATIC_METHOD_2(, BROKER_RESULT, Broker_AddLink, BROKER_HANDLE, handle, const BROKER_LINK_DATA*, link)
    MOCK_METHOD_END(BROKER_RESULT, BROKER_OK)

    MOCK_STATIC_METHOD_2(, BROKER_RESULT, Broker_RemoveLink, BROKER_HANDLE, handle, const BROKER_LINK_DATA*, link)
    MOCK_METHOD_END(BROKER_RESULT, BROKER_OK)

    MOCK_STATIC_METHOD_2(, MODULE_LIBRARY_HANDLE, DynamicModuleLoader_Load, const struct MODULE_LOADER_TAG*, loader, const void*, entrypoint)
        currentModuleLoader_Load_call++;
        MODULE_LIBRARY_HANDLE handle = NULL;
        if (whenShallModuleLoader_Load_fail >= 0 && whenShallModuleLoader_Load_fail != currentModuleLoader_Load_call)
        {
            handle = (MODULE_LIBRARY_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1);
        }
    MOCK_METHOD_END(MODULE_LIBRARY_HANDLE, handle);

    MOCK_STATIC_METHOD_2(, const MODULE_API*, DynamicModuleLoader_GetModuleApi, const struct MODULE_LOADER_TAG*, loader, MODULE_LIBRARY_HANDLE, module_library_handle)
        const MODULE_API* apis = reinterpret_cast<const MODULE_API*>(&dummyAPIs);
    MOCK_METHOD_END(const MODULE_API*, apis);

    MOCK_STATIC_METHOD_2(, void, DynamicModuleLoader_Unload, const struct MODULE_LOADER_TAG*, loader, MODULE_LIBRARY_HANDLE, moduleLibraryHandle)
        BASEIMPLEMENTATION::gballoc_free(moduleLibraryHandle);
    MOCK_VOID_METHOD_END();

	MOCK_STATIC_METHOD_2(, void*, DynamicModuleLoader_ParseEntrypointFromJson, const struct MODULE_LOADER_TAG*, loader, const JSON_Value*, json)
		void* r = BASEIMPLEMENTATION::gballoc_malloc(1);
	MOCK_METHOD_END(void*, r);

	MOCK_STATIC_METHOD_2(, void, DynamicModuleLoader_FreeEntrypoint, const struct MODULE_LOADER_TAG*, loader, void*, entrypoint)
		BASEIMPLEMENTATION::gballoc_free(entrypoint);
	MOCK_VOID_METHOD_END();

	MOCK_STATIC_METHOD_2(, MODULE_LOADER_BASE_CONFIGURATION*, DynamicModuleLoader_ParseConfigurationFromJson, const struct MODULE_LOADER_TAG*, loader, const JSON_Value*, json)
		MODULE_LOADER_BASE_CONFIGURATION* r = (MODULE_LOADER_BASE_CONFIGURATION*)BASEIMPLEMENTATION::gballoc_malloc(1);
	MOCK_METHOD_END(MODULE_LOADER_BASE_CONFIGURATION*, r);

	MOCK_STATIC_METHOD_2(, void, DynamicModuleLoader_FreeConfiguration, const struct MODULE_LOADER_TAG*, loader, MODULE_LOADER_BASE_CONFIGURATION*, configuration)
		BASEIMPLEMENTATION::gballoc_free(configuration);
	MOCK_VOID_METHOD_END();

	MOCK_STATIC_METHOD_3(, void*, DynamicModuleLoader_BuildModuleConfiguration, const struct MODULE_LOADER_TAG*, loader, const void*, entrypoint, const void*, module_configuration)
		void* r = BASEIMPLEMENTATION::gballoc_malloc(1);
	MOCK_METHOD_END(void*, r);

	MOCK_STATIC_METHOD_2(, void, DynamicModuleLoader_FreeModuleConfiguration, const struct MODULE_LOADER_TAG*, loader, const void*, module_configuration)
		BASEIMPLEMENTATION::gballoc_free((void*)module_configuration);
	MOCK_VOID_METHOD_END();

	MOCK_STATIC_METHOD_0(, MODULE_LOADER_RESULT, ModuleLoader_Initialize);
	MOCK_METHOD_END(MODULE_LOADER_RESULT, MODULE_LOADER_SUCCESS);

	MOCK_STATIC_METHOD_0(, void, ModuleLoader_Destroy);
	MOCK_VOID_METHOD_END();


    MOCK_STATIC_METHOD_0(, EVENTSYSTEM_HANDLE, EventSystem_Init)
    MOCK_METHOD_END(EVENTSYSTEM_HANDLE, (EVENTSYSTEM_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1));

    MOCK_STATIC_METHOD_4(, void, EventSystem_AddEventCallback, EVENTSYSTEM_HANDLE, event_system, GATEWAY_EVENT, event_type, GATEWAY_CALLBACK, callback, void*, user_param)
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

    MOCK_STATIC_METHOD_1(, void*, VECTOR_back, const VECTOR_HANDLE, handle)
        auto element = BASEIMPLEMENTATION::VECTOR_back(handle);
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
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_2(, void*, gballoc_calloc, size_t, num, size_t, size)
    MOCK_METHOD_END(void*, BASEIMPLEMENTATION::gballoc_calloc(num, size));

    MOCK_STATIC_METHOD_2(, int, mallocAndStrcpy_s, char**, destination, const char*, source)
        (*destination) = (char*)malloc(strlen(source) + 1);
        strcpy(*destination, source);
    MOCK_METHOD_END(int, 0);
};

DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , void*, mock_Module_ParseConfigurationFromJson, const char*, configuration);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , void, mock_Module_FreeConfiguration, void*, configuration);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayLLMocks, , MODULE_HANDLE, mock_Module_Create, BROKER_HANDLE, broker, const void*, configuration);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , void, mock_Module_Destroy, MODULE_HANDLE, moduleHandle);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayLLMocks, , void, mock_Module_Receive, MODULE_HANDLE, moduleHandle, MESSAGE_HANDLE, messageHandle);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , void, mock_Module_Start, MODULE_HANDLE, moduleHandle);

DECLARE_GLOBAL_MOCK_METHOD_0(CGatewayLLMocks, , BROKER_HANDLE, Broker_Create);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , void, Broker_Destroy, BROKER_HANDLE, broker);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayLLMocks, , BROKER_RESULT, Broker_AddModule, BROKER_HANDLE, handle, const MODULE*, module);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayLLMocks, , BROKER_RESULT, Broker_RemoveModule, BROKER_HANDLE, handle, const MODULE*, module);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayLLMocks, , BROKER_RESULT, Broker_AddLink, BROKER_HANDLE, handle, const BROKER_LINK_DATA*, link);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayLLMocks, , BROKER_RESULT, Broker_RemoveLink, BROKER_HANDLE, handle, const BROKER_LINK_DATA*, link);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , void, Broker_IncRef, BROKER_HANDLE, broker);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , void, Broker_DecRef, BROKER_HANDLE, broker);

DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayLLMocks, , MODULE_LIBRARY_HANDLE, DynamicModuleLoader_Load, const struct MODULE_LOADER_TAG*, loader, const void*, entrypoint);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayLLMocks, , const MODULE_API*, DynamicModuleLoader_GetModuleApi, const struct MODULE_LOADER_TAG*, loader, MODULE_LIBRARY_HANDLE, module_library_handle);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayLLMocks, , void, DynamicModuleLoader_Unload, const struct MODULE_LOADER_TAG*, loader, MODULE_LIBRARY_HANDLE, moduleLibraryHandle);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayLLMocks, , void*, DynamicModuleLoader_ParseEntrypointFromJson, const struct MODULE_LOADER_TAG*, loader, const JSON_Value*, json);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayLLMocks, , void, DynamicModuleLoader_FreeEntrypoint, const struct MODULE_LOADER_TAG*, loader, void*, entrypoint);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayLLMocks, , MODULE_LOADER_BASE_CONFIGURATION*, DynamicModuleLoader_ParseConfigurationFromJson, const struct MODULE_LOADER_TAG*, loader, const JSON_Value*, json);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayLLMocks, , void, DynamicModuleLoader_FreeConfiguration, const struct MODULE_LOADER_TAG*, loader, MODULE_LOADER_BASE_CONFIGURATION*, configuration);
DECLARE_GLOBAL_MOCK_METHOD_3(CGatewayLLMocks, , void*, DynamicModuleLoader_BuildModuleConfiguration, const struct MODULE_LOADER_TAG*, loader, const void*, entrypoint, const void*, module_configuration);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayLLMocks, , void, DynamicModuleLoader_FreeModuleConfiguration, const struct MODULE_LOADER_TAG*, loader, const void*, module_configuration);
DECLARE_GLOBAL_MOCK_METHOD_0(CGatewayLLMocks, , MODULE_LOADER_RESULT, ModuleLoader_Initialize);
DECLARE_GLOBAL_MOCK_METHOD_0(CGatewayLLMocks, , void, ModuleLoader_Destroy);

DECLARE_GLOBAL_MOCK_METHOD_0(CGatewayLLMocks, , EVENTSYSTEM_HANDLE, EventSystem_Init);
DECLARE_GLOBAL_MOCK_METHOD_4(CGatewayLLMocks, , void, EventSystem_AddEventCallback, EVENTSYSTEM_HANDLE, event_system, GATEWAY_EVENT, event_type, GATEWAY_CALLBACK, callback, void*, user_param);
DECLARE_GLOBAL_MOCK_METHOD_3(CGatewayLLMocks, , void, EventSystem_ReportEvent, EVENTSYSTEM_HANDLE, event_system, GATEWAY_HANDLE, gw, GATEWAY_EVENT, event_type);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , void, EventSystem_Destroy, EVENTSYSTEM_HANDLE, handle);

DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , VECTOR_HANDLE, VECTOR_create, size_t, elementSize);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , void, VECTOR_destroy, VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_3(CGatewayLLMocks, , int, VECTOR_push_back, VECTOR_HANDLE, handle, const void*, elements, size_t, numElements);
DECLARE_GLOBAL_MOCK_METHOD_3(CGatewayLLMocks, , void, VECTOR_erase, VECTOR_HANDLE, handle, void*, elements, size_t, index);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayLLMocks, , void*, VECTOR_element, const VECTOR_HANDLE, handle, size_t, index);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , void*, VECTOR_front, const VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , void*, VECTOR_back, const VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , size_t, VECTOR_size, const VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_3(CGatewayLLMocks, , void*, VECTOR_find_if, const VECTOR_HANDLE, handle, PREDICATE_FUNCTION, pred, const void*, value);

DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayLLMocks, , void*, gballoc_realloc, void*, ptr, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayLLMocks, , void, gballoc_free, void*, ptr)
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayLLMocks, , void*, gballoc_calloc, size_t, num, size_t, size)

DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayLLMocks, , int, mallocAndStrcpy_s, char**, destination, const char*, source);

static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;
static MICROMOCK_MUTEX_HANDLE g_testByTest;

static GATEWAY_PROPERTIES* dummyProps;
static MODULE_LOADER_API module_loader_api =
{
    DynamicModuleLoader_Load,
    DynamicModuleLoader_Unload,
    DynamicModuleLoader_GetModuleApi,
	DynamicModuleLoader_ParseEntrypointFromJson,
	DynamicModuleLoader_FreeEntrypoint,
	DynamicModuleLoader_ParseConfigurationFromJson,
	DynamicModuleLoader_FreeConfiguration,
	DynamicModuleLoader_BuildModuleConfiguration,
	DynamicModuleLoader_FreeModuleConfiguration
};

static MODULE_LOADER dummyModuleLoader =
{
	NATIVE,
	"dummy loader",
	NULL,
	&module_loader_api
};

static GATEWAY_MODULE_LOADER_INFO dummyLoaderInfo =
{
	&dummyModuleLoader,
	(void*)0x42
};

static int sampleCallbackFuncCallCount;

static void expectEventSystemInit(CGatewayLLMocks &mocks)
{
    STRICT_EXPECTED_CALL(mocks, EventSystem_Init());
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, IGNORED_PTR_ARG, GATEWAY_CREATED))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, IGNORED_PTR_ARG, GATEWAY_MODULE_LIST_CHANGED))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
}

static void expectEventSystemDestroy(CGatewayLLMocks &mocks)
{
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, IGNORED_PTR_ARG, GATEWAY_DESTROYED))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, EventSystem_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
}

static void sampleCallbackFunc(GATEWAY_HANDLE gw, GATEWAY_EVENT event_type, GATEWAY_EVENT_CTX ctx, void* user_param)
{
    sampleCallbackFuncCallCount++;
}

BEGIN_TEST_SUITE(gateway_ut)

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


    currentVECTOR_create_call = 0;
    whenShallVECTOR_create_fail = 0;
    currentVECTOR_push_back_call = 0;
    whenShallVECTOR_push_back_fail = 0;
    currentVECTOR_find_if_call = 0;
    whenShallVECTOR_find_if_fail = 0;

    dummyAPIs =
    {
        {MODULE_API_VERSION_1},

		mock_Module_ParseConfigurationFromJson,
		mock_Module_FreeConfiguration,
        mock_Module_Create,
        mock_Module_Destroy,
        mock_Module_Receive,
        mock_Module_Start
    };



    GATEWAY_MODULES_ENTRY dummyEntry = {
        "dummy module",
        dummyLoaderInfo,
        NULL
    };

    dummyProps = (GATEWAY_PROPERTIES*)malloc(sizeof(GATEWAY_PROPERTIES));
    dummyProps->gateway_modules = BASEIMPLEMENTATION::VECTOR_create(sizeof(GATEWAY_MODULES_ENTRY));
    dummyProps->gateway_links = BASEIMPLEMENTATION::VECTOR_create(sizeof(GATEWAY_LINK_ENTRY));
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry, 1);
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

    BASEIMPLEMENTATION::VECTOR_destroy(dummyProps->gateway_modules);
    BASEIMPLEMENTATION::VECTOR_destroy(dummyProps->gateway_links);
    free(dummyProps);
}

/*Tests_SRS_GATEWAY_14_001: [This function shall create a GATEWAY_HANDLE representing the newly created gateway.]*/
/*Tests_SRS_GATEWAY_14_003: [This function shall create a new BROKER_HANDLE for the gateway representing this gateway's message broker. ]*/
/*Tests_SRS_GATEWAY_14_033: [ The function shall create a vector to store each MODULE_DATA. ]*/
/*Tests_SRS_GATEWAY_04_001: [ The function shall create a vector to store each LINK_DATA ] */
/*Tests_SRS_GATEWAY_17_016: [ This function shall initialize the default module loaders. ]*/
/*Tests_SRS_GATEWAY_17_015: [ The function shall use the module's specified loader and the module's entrypoint to get each module's MODULE_LIBRARY_HANDLE. ]*/
/*Tests_SRS_GATEWAY_17_018: [ The function shall construct module configuration from module's entrypoint and module's module_configuration. ]*/
/*Tests_SRS_GATEWAY_17_020: [ The function shall clean up any constructed resources. ]*/
/*Tests_SRS_GATEWAY_14_009: [ The function shall use each of GATEWAY_PROPERTIES's gateway_modules to create and add a module to the gateway's message broker. ]*/

TEST_FUNCTION(Gateway_Create_Creates_Handle_Success)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Expectations
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Initialize());
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Broker_Create());
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    expectEventSystemInit(mocks);

    //Act
    GATEWAY_HANDLE gateway = Gateway_Create(NULL);

    //Assert
    ASSERT_IS_NOT_NULL(gateway);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);
}

/*Tests_SRS_GATEWAY_14_011: [ If gw, entry, or GATEWAY_MODULES_ENTRY's loader_configuration or loader_api is NULL the function shall return NULL. ]*/
/*Tests_SRS_GATEWAY_17_017: [ This function shall destroy the default module loaders upon any failure. ]*/
TEST_FUNCTION(Gateway_Create_returns_null_on_bad_module_api_entry)
{
    //Arrange
    CGatewayLLMocks mocks;

	MODULE_LOADER badModuleLoader =
	{
		NATIVE,
		"dummy loader",
		NULL,
		NULL
	};

	static GATEWAY_MODULE_LOADER_INFO dummyLoaderInfo =
	{
		&badModuleLoader,
		(void*)0x42
	};
    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module 2",
        dummyLoaderInfo,
        NULL
    };

    GATEWAY_PROPERTIES newdummyProps;
    newdummyProps.gateway_modules = BASEIMPLEMENTATION::VECTOR_create(sizeof(GATEWAY_MODULES_ENTRY));
    ASSERT_IS_NOT_NULL(newdummyProps.gateway_modules);
    BASEIMPLEMENTATION::VECTOR_push_back(newdummyProps.gateway_modules, &dummyEntry2, 1);
    newdummyProps.gateway_links = NULL;


    //Expectations
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Initialize());
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Broker_Create());
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .IgnoreArgument(1); //modules
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .IgnoreArgument(1); //links
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(newdummyProps.gateway_modules));
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(newdummyProps.gateway_modules, 0));
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Broker_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //Act
    GATEWAY_HANDLE gateway = Gateway_Create(&newdummyProps);

    //Assert
    ASSERT_IS_NULL(gateway);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    BASEIMPLEMENTATION::VECTOR_destroy(newdummyProps.gateway_modules);
    
    //Nothing to cleanup
}

/*Codes_SRS_GATEWAY_14_002: [ This function shall return NULL upon any failure. ] */
TEST_FUNCTION(Gateway_Create_ModuleLoader_Init_Failure)
{
	//Arrange
	CGatewayLLMocks mocks;

	//Expectations
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Initialize())
		.SetFailReturn(MODULE_LOADER_ERROR);

	//Act
	GATEWAY_HANDLE gateway = Gateway_Create(NULL);

	//Assert
	ASSERT_IS_NULL(gateway);
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	//Nothing to cleanup
}

/*Tests_SRS_GATEWAY_14_002: [This function shall return NULL upon any failure.]*/
TEST_FUNCTION(Gateway_Create_Creates_Handle_Malloc_Failure)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Expectations
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Initialize());
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());
	whenShallmalloc_fail = 1;
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    //Act
    GATEWAY_HANDLE gateway = Gateway_Create(NULL);

    //Assert
    ASSERT_IS_NULL(gateway);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    //Nothing to cleanup
}

/*Tests_SRS_GATEWAY_14_004: [ This function shall return NULL if a BROKER_HANDLE cannot be created. ]*/
TEST_FUNCTION(Gateway_Create_Cannot_Create_Broker_Handle_Failure)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Expectations
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Initialize());
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    whenShallBroker_Create_fail = 1;
    STRICT_EXPECTED_CALL(mocks, Broker_Create());
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    GATEWAY_HANDLE gateway = Gateway_Create(NULL);

    //Assert
    ASSERT_IS_NULL(gateway);
    mocks.AssertActualAndExpectedCalls();
    
    //Cleanup
    //Nothing to cleanup
}

/*Tests_SRS_GATEWAY_14_034: [ This function shall return NULL if a VECTOR_HANDLE cannot be created. ]*/
/*Tests_SRS_GATEWAY_14_035: [ This function shall destroy the previously created BROKER_HANDLE and free the GATEWAY_HANDLE if the VECTOR_HANDLE cannot be created. ]*/
TEST_FUNCTION(Gateway_Create_VECTOR_Create_Fails)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Expectations
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Initialize());
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mocks, Broker_Create());

    whenShallVECTOR_create_fail = 1; 
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mocks, Broker_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);


    //Act
    GATEWAY_HANDLE gateway = Gateway_Create(NULL);
    
    //Assert
    ASSERT_IS_NULL(gateway);

    //Cleanup
    //Nothing to cleanup
}

/*Codes_SRS_GATEWAY_14_002: [ This function shall return NULL upon any failure. ] */
TEST_FUNCTION(Gateway_Create_VECTOR_Create2_Fails)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Expectations
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Initialize());
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mocks, Broker_Create());

    whenShallVECTOR_create_fail = 2;
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mocks, Broker_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);


    //Act
    GATEWAY_HANDLE gateway = Gateway_Create(NULL);

    //Assert
    ASSERT_IS_NULL(gateway);

    //Cleanup
    //Nothing to cleanup
}

/*Codes_SRS_GATEWAY_14_002: [ This function shall return NULL upon any failure. ] */
TEST_FUNCTION(Gateway_Create_VECTOR_push_back_Fails_To_Add_All_Modules_In_Props)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Add another entry to the properties
	GATEWAY_MODULES_ENTRY dummyEntry2 = {
		"dummy module 2",
		dummyLoaderInfo,
		NULL
	};

    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry2, 1);

    //Expectations
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Initialize());
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Broker_Create());
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .IgnoreArgument(1); //modules
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .IgnoreArgument(1); //links
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(dummyProps->gateway_modules));

    //Adding module 1 (Success)
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(dummyProps->gateway_modules, 0));
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Load(IGNORED_PTR_ARG, dummyEntry2.module_loader_info.entrypoint))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_BuildModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_IncRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_back(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //Adding module 2 (Failure)
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(dummyProps->gateway_modules, 1));
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Load(IGNORED_PTR_ARG, dummyEntry2.module_loader_info.entrypoint))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_BuildModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_IncRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    whenShallVECTOR_push_back_fail = 2;
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_DecRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Unload(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    //Removing previous module in Gateway_Destroy()
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_DecRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Unload(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Broker_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);

    ASSERT_IS_NULL(gateway);
    ASSERT_ARE_EQUAL(size_t, 0, currentBroker_module_count);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);

}

/*Tests_SRS_GATEWAY_14_036: [ If any MODULE_HANDLE is unable to be created from a GATEWAY_MODULES_ENTRY the GATEWAY_HANDLE will be destroyed. ]*/
TEST_FUNCTION(Gateway_Create_Broker_AddModule_Fails_To_Add_All_Modules_In_Props)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Add another entry to the properties
	GATEWAY_MODULES_ENTRY dummyEntry2 = {
		"dummy module 2",
		dummyLoaderInfo,
		NULL
	};

    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry2, 1);

    //Expectations
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Initialize());
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Broker_Create());
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .IgnoreArgument(1); //modules
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .IgnoreArgument(1); //links
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(dummyProps->gateway_modules));

    //Adding module 1 (Success)
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(dummyProps->gateway_modules, 0));
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Load(IGNORED_PTR_ARG, dummyEntry2.module_loader_info.entrypoint))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_BuildModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_IncRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_back(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //Adding module 2 (Failure)
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(dummyProps->gateway_modules, 1));
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Load(IGNORED_PTR_ARG, dummyEntry2.module_loader_info.entrypoint))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_BuildModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
    whenShallBroker_AddModule_fail = 2;
    STRICT_EXPECTED_CALL(mocks, Broker_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Unload(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    //Removing previous module in Gateway_Destroy()
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_DecRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Unload(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1); //Modules
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1); //Links
    STRICT_EXPECTED_CALL(mocks, Broker_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);

    ASSERT_IS_NULL(gateway);
    ASSERT_ARE_EQUAL(size_t, 0, currentBroker_module_count);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);

}


/*Tests_SRS_GATEWAY_04_004: [If a module with the same module_name already exists, this function shall fail and the GATEWAY_HANDLE will be destroyed.]*/
/*Tests_SRS_GATEWAY_17_020: [ The function shall clean up any constructed resources. ]*/
TEST_FUNCTION(Gateway_Create_AddModule_WithDuplicatedModuleName_Fails)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Add another entry to the properties
    GATEWAY_MODULES_ENTRY duplicatedEntry = {
        "dummy module",
		dummyLoaderInfo,
		NULL
    };

    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &duplicatedEntry, 1);
    
    //Expectations
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Initialize());
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Broker_Create());
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .IgnoreArgument(1); //modules
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .IgnoreArgument(1); //links
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(dummyProps->gateway_modules));

    //Adding module 1 (Success)
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(dummyProps->gateway_modules, 0));
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Load(IGNORED_PTR_ARG, duplicatedEntry.module_loader_info.entrypoint))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_BuildModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_IncRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_back(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //Adding module 2 (Failure)
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(dummyProps->gateway_modules, 1));
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    //Removing previous module
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_DecRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Unload(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Broker_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);

    ASSERT_IS_NULL(gateway);
    ASSERT_ARE_EQUAL(size_t, 0, currentBroker_module_count);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);

}


/*Tests_SRS_GATEWAY_17_015: [ The function shall use the module's specified loader and the module's entrypoint to get each module's MODULE_LIBRARY_HANDLE. ]*/
/*Tests_SRS_GATEWAY_17_018: [ The function shall construct module configuration from module's entrypoint and module's module_configuration. ]*/
TEST_FUNCTION(Gateway_Create_Adds_All_Modules_In_Props_Success)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Add another entry to the properties
    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module 2",
		dummyLoaderInfo,
        NULL
    };

    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry2, 1);

    //Expectations
    STRICT_EXPECTED_CALL(mocks, mock_Module_ParseConfigurationFromJson(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .NeverInvoked();

	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Initialize());
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Broker_Create());
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .IgnoreArgument(1); //modules vector.
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .IgnoreArgument(1); //links vector.
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(dummyProps->gateway_modules)); //Modules

    //Adding module 1 (Success)
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(dummyProps->gateway_modules, 0));
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Load(IGNORED_PTR_ARG, dummyEntry2.module_loader_info.entrypoint))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_BuildModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_IncRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_back(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //Adding module 2 (Success)
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(dummyProps->gateway_modules, 1));
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Load(IGNORED_PTR_ARG, dummyEntry2.module_loader_info.entrypoint))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_BuildModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_IncRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_back(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mocks, VECTOR_size(dummyProps->gateway_links)); //Links

    expectEventSystemInit(mocks);

    //Act
    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);

    //Assert
    ASSERT_IS_NOT_NULL(gateway);
    ASSERT_ARE_EQUAL(size_t, 2, currentBroker_module_count);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);
}

/*Tests_SRS_GATEWAY_04_002: [ The function shall use each GATEWAY_LINK_ENTRY of GATEWAY_PROPERTIES's gateway_links to add a LINK to GATEWAY_HANDLE's broker. ] */
TEST_FUNCTION(Gateway_Create_Adds_All_Modules_And_All_Links_In_Props_Success)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Add another entry to the properties
    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module 2",
		dummyLoaderInfo,
        NULL
    };

    GATEWAY_LINK_ENTRY dummyLink = {
        "dummy module",
        "dummy module 2"
    };

    // indirectly test that it's ok to set Module_CreateFromJson to NULL
    const MODULE_API_1 noCreateFromJson =
    {
        {MODULE_API_VERSION_1},

        NULL,
		NULL,
        mock_Module_Create,
        mock_Module_Destroy,
        mock_Module_Receive,
        mock_Module_Start
        
    };
    const MODULE_API* noCreateFromJsonApi = reinterpret_cast<const MODULE_API *>(&noCreateFromJson);

    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry2, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_links, &dummyLink, 1);

    //Expectations
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Initialize());

    EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, Broker_Create());
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .IgnoreArgument(1); //modules vector.
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .IgnoreArgument(1); //links vector.
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(dummyProps->gateway_modules));
    
    //Modules

    //Adding module 1 (Success)
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(dummyProps->gateway_modules, 0));
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Load(IGNORED_PTR_ARG, dummyEntry2.module_loader_info.entrypoint))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(noCreateFromJsonApi);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_BuildModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_IncRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_back(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //Adding module 2 (Success)
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(dummyProps->gateway_modules, 1));
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Load(IGNORED_PTR_ARG, dummyEntry2.module_loader_info.entrypoint))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(noCreateFromJsonApi);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_BuildModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_IncRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_back(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mocks, VECTOR_size(dummyProps->gateway_links)); //Links


    //Adding link1 (Success)
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(dummyProps->gateway_links, 0));
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments(); //Check if Link exists.
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments(); //Check if Source Module exists.
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments(); //Check if Sink Module exists.
    STRICT_EXPECTED_CALL(mocks, Broker_AddLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    expectEventSystemInit(mocks);

    //Act
    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);

    //Assert
    ASSERT_IS_NOT_NULL(gateway);
    ASSERT_ARE_EQUAL(size_t, 2, currentBroker_module_count);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);
}

/*Tests_SRS_GATEWAY_04_003: [If any GATEWAY_LINK_ENTRY is unable to be added to the broker the GATEWAY_HANDLE will be destroyed.]*/
TEST_FUNCTION(Gateway_Create_Adds_All_Modules_And_Links_fromNonExistingModule_Fail)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Add another entry to the properties
    GATEWAY_LINK_ENTRY dummyLink = {
        "Non Existing Module",
        "dummy module 2"
    };
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_links, &dummyLink, 1);

    //Expectations
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Initialize());
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());
	EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, Broker_Create());
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .IgnoreArgument(1); //modules vector.
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .IgnoreArgument(1); //links vector.
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(dummyProps->gateway_modules)); //Modules

    //Adding module 1 (Success)
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(dummyProps->gateway_modules, 0));
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Load(IGNORED_PTR_ARG, dummyLoaderInfo.entrypoint))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_BuildModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_IncRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_back(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mocks, VECTOR_size(dummyProps->gateway_links)); //Links

    //Adding link1 (Failure)
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(dummyProps->gateway_links, 0));
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments(); //Check if Link exists.
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments(); //Check if Source Module exists.


    //Removing previous module
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, Broker_DecRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Unload(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1); //Modules
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1); //Links
    STRICT_EXPECTED_CALL(mocks, Broker_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);

    ASSERT_IS_NULL(gateway);
    ASSERT_ARE_EQUAL(size_t, 0, currentBroker_module_count);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);
}

/*Tests_SRS_GATEWAY_14_005: [ If gw is NULL the function shall do nothing. ]*/
TEST_FUNCTION(Gateway_Destroy_destroys_loader_If_NULL)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_HANDLE gateway = NULL;
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());

    //Act
    Gateway_Destroy(gateway);

    //Assert
    mocks.AssertActualAndExpectedCalls();
}

/*Tests_SRS_GATEWAY_14_037: [ If GATEWAY_HANDLE_DATA's message broker cannot remove a module, the function shall log the error and continue removing modules from the GATEWAY_HANDLE. ]*/
TEST_FUNCTION(Gateway_Destroy_Continues_Unloading_If_Broker_RemoveModule_Fails)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Add another entry to the properties
    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module 2",
		dummyLoaderInfo,
        NULL
    };

    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry2, 1);

    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);
    mocks.ResetAllCalls();

    //Expectations
    expectEventSystemDestroy(mocks);

    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1); //Modules
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1); //Links
    STRICT_EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_DecRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Unload(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_DecRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Unload(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1); //Modules
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1); //links
    STRICT_EXPECTED_CALL(mocks, Broker_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());

    //Act
    Gateway_Destroy(gateway);

    //Assert
    mocks.AssertActualAndExpectedCalls();
}

/*Tests_SRS_GATEWAY_14_028: [ The function shall remove each module in GATEWAY_HANDLE_DATA's modules vector and destroy GATEWAY_HANDLE_DATA's modules. ]*/
/*Tests_SRS_GATEWAY_14_006: [ The function shall destroy the GATEWAY_HANDLE_DATA's broker BROKER_HANDLE. ]*/
/*Tests_SRS_GATEWAY_04_014: [ The function shall remove each link in GATEWAY_HANDLE_DATA's links vector and destroy GATEWAY_HANDLE_DATA's link. ]*/
/*Tests_SRS_GATEWAY_17_019: [ The function shall destroy the module loader list. ]*/
TEST_FUNCTION(Gateway_Destroy_Removes_All_Modules_And_Destroys_Vector_Success)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Add another entry to the properties
    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module 2",
		dummyLoaderInfo,
        NULL
    };
    
    GATEWAY_LINK_ENTRY dummyLink = {
        "dummy module",
        "dummy module 2"
    };

    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry2, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_links, &dummyLink, 1);

    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);
    mocks.ResetAllCalls();

    //Gateway_Destroy Expectations
    expectEventSystemDestroy(mocks);

    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_DecRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Unload(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_DecRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Unload(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);


    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1); //Modules
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1); //Links
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1); //Modules.
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1); //Links
    STRICT_EXPECTED_CALL(mocks, Broker_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());

    //Act
    Gateway_Destroy(gateway);

    //Assert
    ASSERT_ARE_EQUAL(size_t, 0, currentBroker_module_count);
    mocks.AssertActualAndExpectedCalls();
}

/*Tests_SRS_GATEWAY_14_011: [ If gw, entry, or GATEWAY_MODULES_ENTRY's loader_configuration or loader_api is NULL the function shall return NULL. ]*/
TEST_FUNCTION(Gateway_AddModule_Returns_Null_For_Null_Gateway)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    mocks.ResetAllCalls();

    //Act
    MODULE_HANDLE handle0 = Gateway_AddModule(NULL, (GATEWAY_MODULES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_modules));
    
    //Assert
    ASSERT_IS_NULL(handle0);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/*Tests_SRS_GATEWAY_14_011: [ If gw, entry, or GATEWAY_MODULES_ENTRY's specified loader or entrypoint is NULL the function shall return NULL. ]*/
TEST_FUNCTION(Gateway_AddModule_Returns_Null_For_Null_Module)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    mocks.ResetAllCalls();

    //Act
    MODULE_HANDLE handle1 = Gateway_AddModule(gw, NULL);

    //Assert
    ASSERT_IS_NULL(handle1);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/*Tests_SRS_GATEWAY_14_011: [ If gw, entry, or GATEWAY_MODULES_ENTRY's specified loader or entrypoint is NULL the function shall return NULL. ]*/
TEST_FUNCTION(Gateway_AddModule_Returns_Null_For_null_module_name)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    mocks.ResetAllCalls();

    //Act
    GATEWAY_MODULES_ENTRY* entry = (GATEWAY_MODULES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_modules);
    entry->module_name = NULL;
    MODULE_HANDLE handle2 = Gateway_AddModule(gw, (GATEWAY_MODULES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_modules));

    //Assert
    ASSERT_IS_NULL(handle2);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

//Tests_SRS_GATEWAY_17_001: [ This function shall not accept "*" as a module name. ]
TEST_FUNCTION(Gateway_AddModule_Returns_Null_For_star_module_name)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    mocks.ResetAllCalls();

    //Act
    GATEWAY_MODULES_ENTRY* entry = (GATEWAY_MODULES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_modules);
    entry->module_name = "*";
    MODULE_HANDLE handle2 = Gateway_AddModule(gw, (GATEWAY_MODULES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_modules));

    //Assert
    ASSERT_IS_NULL(handle2);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/*Tests_SRS_GATEWAY_14_012: [ The function shall load the module located at GATEWAY_MODULES_ENTRY's module_path into a MODULE_LIBRARY_HANDLE. ]*/
/*Tests_SRS_GATEWAY_14_013: [ The function shall get the const MODULE_API* from the MODULE_LIBRARY_HANDLE. ]*/
/*Tests_SRS_GATEWAY_17_015: [ The function shall use GATEWAY_PROPERTIES::loader_api->Load and each GATEWAY_PROPERTIES::loader_configuration to get each module's MODULE_LIBRARY_HANDLE. ]*/
/*Tests_SRS_GATEWAY_14_017: [ The function shall attach the module to the GATEWAY_HANDLE_DATA's broker using a call to Broker_AddModule. ]*/
/*Tests_SRS_GATEWAY_14_029: [ The function shall create a new MODULE_DATA containing the MODULE_HANDLE, MODULE_LOADER_API and MODULE_LIBRARY_HANDLE if the module was successfully linked to the message broker. ]*/
/*Tests_SRS_GATEWAY_14_032: [ The function shall add the new MODULE_DATA to GATEWAY_HANDLE_DATA's modules if the module was successfully linked to the message broker. ]*/
/*Tests_SRS_GATEWAY_14_019: [ The function shall return the newly created MODULE_HANDLE only if each API call returns successfully. ]*/
/*Tests_SRS_GATEWAY_14_039: [ The function shall increment the BROKER_HANDLE reference count if the MODULE_HANDLE was successfully linked to the GATEWAY_HANDLE_DATA's broker. ]*/
/*Tests_SRS_GATEWAY_99_011: [ The function shall assign `module_apis` to `MODULE::module_apis`. ]*/
/*Tests_SRS_GATEWAY_17_015: [ The function shall use GATEWAY_PROPERTIES::loader_api->Load and each GATEWAY_PROPERTIES::loader_configuration to get each module's MODULE_LIBRARY_HANDLE. ]*/
/*Tests_SRS_GATEWAY_17_021: [ The function shall construct module configuration from module's entrypoint and module's module_configuration. ]*/
/*Tests_SRS_GATEWAY_17_022: [ The function shall clean up any constructed resources. ]*/
TEST_FUNCTION(Gateway_AddModule_Loads_Module_From_Library_Path)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    mocks.ResetAllCalls();

    //Expectations
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Load(IGNORED_PTR_ARG, dummyLoaderInfo.entrypoint))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_BuildModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_IncRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_back(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, gw, GATEWAY_MODULE_LIST_CHANGED))
        .IgnoreArgument(1);

    //Act
    MODULE_HANDLE handle = Gateway_AddModule(gw, (GATEWAY_MODULES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_modules));

    //Assert
    ASSERT_IS_NOT_NULL(handle);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/*Tests_SRS_GATEWAY_14_031: [ If unsuccessful, the function shall return NULL. ]*/
TEST_FUNCTION(Gateway_AddModule_Malloc_data_Fails)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    mocks.ResetActualCalls();

    //Expectations
    whenShallModuleLoader_Load_fail = 1;
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(nullptr);

    MODULE_HANDLE handle = Gateway_AddModule(gw, (GATEWAY_MODULES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_modules));

    //Assert
    ASSERT_IS_NULL(handle);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/*Tests_SRS_GATEWAY_14_031: [ If unsuccessful, the function shall return NULL. ]*/
TEST_FUNCTION(Gateway_AddModule_Fails)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    mocks.ResetActualCalls();

    //Expectations
    whenShallModuleLoader_Load_fail = 1;
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Load(IGNORED_PTR_ARG, dummyLoaderInfo.entrypoint))
        .IgnoreArgument(1);

    MODULE_HANDLE handle = Gateway_AddModule(gw, (GATEWAY_MODULES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_modules));

    //Assert
    ASSERT_IS_NULL(handle);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/*Tests_SRS_GATEWAY_14_015: [ The function shall use the MODULE_API to create a MODULE_HANDLE using the GATEWAY_MODULES_ENTRY's module_properties. ]*/
/*Tests_SRS_GATEWAY_14_039: [ The function shall increment the BROKER_HANDLE reference count if the MODULE_HANDLE was successfully added to the GATEWAY_HANDLE_DATA's broker. ]*/
TEST_FUNCTION(Gateway_AddModule_Creates_Module_Using_Module_Properties)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    mocks.ResetAllCalls();
    bool* properties = (bool*)malloc(sizeof(bool));
    *properties = true;
    GATEWAY_MODULES_ENTRY entry = {
        "Test module",
		dummyLoaderInfo,
        properties
    };

    //Expectations
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Load(IGNORED_PTR_ARG, dummyLoaderInfo.entrypoint))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_BuildModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
		.IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_IncRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_back(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, gw, GATEWAY_MODULE_LIST_CHANGED))
        .IgnoreArgument(1);

    //Act
    MODULE_HANDLE handle = Gateway_AddModule(gw, &entry);

    //Assert
    ASSERT_IS_NOT_NULL(handle);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
    free(properties);
}

/*Tests_SRS_GATEWAY_14_011: [ If gw, entry, or GATEWAY_MODULES_ENTRY's specified loader or entrypoint is NULL the function shall return NULL. ]*/
TEST_FUNCTION(Gateway_AddModule_fails_on_null_loader_api)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    mocks.ResetAllCalls();
    bool* properties = (bool*)malloc(sizeof(bool));
    *properties = true;
    GATEWAY_MODULES_ENTRY entry = {
        "Test module",
		{ NULL,
		NULL},
        properties
    };
	
    //Expectations

    //Act
    MODULE_HANDLE handle = Gateway_AddModule(gw, &entry);

    //Assert
    ASSERT_IS_NULL(handle);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
    free(properties);
}

/*Tests_SRS_GATEWAY_14_016: [ If the module creation is unsuccessful, the function shall return NULL. ]*/
TEST_FUNCTION(Gateway_AddModule_Module_Create_Fails)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    mocks.ResetAllCalls();
    bool properties = false;
    GATEWAY_MODULES_ENTRY entry = {
        "Test module",
		dummyLoaderInfo,
        &properties
    };

    //Expectations
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Load(IGNORED_PTR_ARG, entry.module_loader_info.entrypoint))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_BuildModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
		.SetFailReturn(nullptr);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Unload(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    //Act
    MODULE_HANDLE handle = Gateway_AddModule(gw, &entry);

    //Assert
    ASSERT_IS_NULL(handle);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/*Tests_SRS_GATEWAY_14_018: [ If the function cannot attach the module to the message broker, the function shall return NULL. ]*/
TEST_FUNCTION(Gateway_AddModule_Broker_AddModule_Fails)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    mocks.ResetAllCalls();
    
    //Expectations
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Load(IGNORED_PTR_ARG, dummyLoaderInfo.entrypoint))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_BuildModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    whenShallBroker_AddModule_fail = 1;
    STRICT_EXPECTED_CALL(mocks, Broker_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Unload(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    

    //Act
    MODULE_HANDLE handle = Gateway_AddModule(gw, (GATEWAY_MODULES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_modules));

    ASSERT_IS_NULL(handle);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/*Tests_SRS_GATEWAY_14_030: [ If any internal API call is unsuccessful after a module is created, the library will be unloaded and the module destroyed. ]*/
/*Tests_SRS_GATEWAY_14_039: [The function shall increment the BROKER_HANDLE reference count if the MODULE_HANDLE was successfully linked to the GATEWAY_HANDLE_DATA's broker. ]*/
TEST_FUNCTION(Gateway_AddModule_Internal_API_Fail_Rollback_Module)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    mocks.ResetAllCalls();

    //Expectations
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Load(IGNORED_PTR_ARG, dummyLoaderInfo.entrypoint))
    .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_BuildModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_IncRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    whenShallVECTOR_push_back_fail = 1;
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(BROKER_ERROR);
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, Broker_DecRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Unload(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    //Act
    MODULE_HANDLE handle = Gateway_AddModule(gw, (GATEWAY_MODULES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_modules));

    ASSERT_IS_NULL(handle);
    ASSERT_ARE_EQUAL(size_t, 0, currentBroker_module_count);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/*Tests_SRS_GATEWAY_14_020: [ If gw or module is NULL the function shall return. ]*/
TEST_FUNCTION(Gateway_RemoveModule_Does_Nothing_If_Gateway_NULL)
{
    //Arrange
    CGatewayLLMocks mocks;
    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    MODULE_HANDLE handle = Gateway_AddModule(gw, (GATEWAY_MODULES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_modules));
    mocks.ResetAllCalls();

    //Act
    Gateway_RemoveModule(NULL, NULL);
    Gateway_RemoveModule(NULL, handle);
    
    //Assert
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/*Tests_SRS_GATEWAY_14_020: [ If gw or module is NULL the function shall return. ]*/
TEST_FUNCTION(Gateway_RemoveModule_Does_Nothing_If_Module_NULL)
{
    //Arrange
    CGatewayLLMocks mocks;
    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    mocks.ResetAllCalls();

    //Expectations
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    //Act
    Gateway_RemoveModule(gw, NULL);

    //Assert
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/*Tests_SRS_GATEWAY_14_023: [ The function shall locate the MODULE_DATA object in GATEWAY_HANDLE_DATA's modules containing module and return if it cannot be found. ]*/
/*Tests_SRS_GATEWAY_14_021: [ The function shall detach module from the GATEWAY_HANDLE_DATA's broker BROKER_HANDLE. ]*/
/*Tests_SRS_GATEWAY_14_024: [ The function shall use the MODULE_DATA's module_library_handle to retrieve the MODULE_API and destroy module. ]*/
/*Tests_SRS_GATEWAY_14_025: [ The function shall unload MODULE_DATA's module_library_handle. ]*/
/*Tests_SRS_GATEWAY_14_026: [ The function shall remove that MODULE_DATA from GATEWAY_HANDLE_DATA's modules. ]*/
/*Tests_SRS_GATEWAY_14_038: [ The function shall decrement the BROKER_HANDLE reference count. ]*/
TEST_FUNCTION(Gateway_RemoveModule_Finds_Module_Data_Success)
{
    //Arrange
    CGatewayLLMocks mocks;
    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    MODULE_HANDLE handle = Gateway_AddModule(gw, (GATEWAY_MODULES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_modules));
    mocks.ResetAllCalls();

    //Expectations
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, Broker_DecRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(handle))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Unload(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, gw, GATEWAY_MODULE_LIST_CHANGED))
        .IgnoreArgument(1);
    

    //Act
    Gateway_RemoveModule(gw, handle);

    //Assert
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/*Tests_SRS_GATEWAY_14_023: [ The function shall locate the MODULE_DATA object in GATEWAY_HANDLE_DATA's modules containing module and return if it cannot be found. ]*/
TEST_FUNCTION(Gateway_RemoveModule_Finds_Module_Data_Failure)
{
    //Arrange
    CGatewayLLMocks mocks;
    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    MODULE_HANDLE handle = Gateway_AddModule(gw, (GATEWAY_MODULES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_modules));
    mocks.ResetAllCalls();

    //Expectations
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, gw))
        .IgnoreAllArguments();

    //Act
    Gateway_RemoveModule(gw, (MODULE_HANDLE)gw);

    //Assert
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/*Tests_SRS_GATEWAY_14_022: [ If GATEWAY_HANDLE_DATA's broker cannot detach module, the function shall log the error and continue unloading the module from the GATEWAY_HANDLE. ]*/
/*Tests_SRS_GATEWAY_14_038: [ The function shall decrement the BROKER_HANDLE reference count. ]*/
TEST_FUNCTION(Gateway_RemoveModule_Broker_RemoveModule_Failure)
{
    //Arrange
    CGatewayLLMocks mocks;
    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    MODULE_HANDLE handle = Gateway_AddModule(gw, (GATEWAY_MODULES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_modules));

    mocks.ResetAllCalls();

    //Expectations
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    whenShallBroker_RemoveModule_fail = 1;
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_DecRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Unload(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    // Well it IS removed from the gateway even if still linked to broker. I think this scenario should report the event
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, gw, GATEWAY_MODULE_LIST_CHANGED))
        .IgnoreArgument(1);

    //Act
    Gateway_RemoveModule(gw, handle);

    //Assert
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/*Tests_SRS_GATEWAY_04_005: [ If gw or entryLink is NULL the function shall return. ]*/
TEST_FUNCTION(Gateway_RemoveLink_Does_Nothing_If_Gateway_NULL)
{
    //Arrange
    CGatewayLLMocks mocks;
    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    MODULE_HANDLE handle = Gateway_AddModule(gw, (GATEWAY_MODULES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_modules));

    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module2",
		dummyLoaderInfo,
        NULL
    };

    GATEWAY_LINK_ENTRY dummyLink = {
        "dummy module",
        "dummy module2"
    };

    MODULE_HANDLE handle2 = Gateway_AddModule(gw, &dummyEntry2);

    Gateway_AddLink(gw, (GATEWAY_LINK_ENTRY*)&dummyLink);

    mocks.ResetAllCalls();

    //Act
    Gateway_RemoveLink(NULL, NULL);
    Gateway_RemoveLink(NULL, &dummyLink);

    //Assert
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/*Tests_SRS_GATEWAY_04_005: [ If gw or entryLink is NULL the function shall return. ]*/
TEST_FUNCTION(Gateway_RemoveLink_Does_Nothing_If_entryLink_NULL)
{
    //Arrange
    CGatewayLLMocks mocks;
    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    mocks.ResetAllCalls();

    //Act
    Gateway_RemoveLink(gw, NULL);

    //Assert
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/*Tests_SRS_GATEWAY_04_006: [ The function shall locate the LINK_DATA object in GATEWAY_HANDLE_DATA's links containing link and return if it cannot be found. ]*/
TEST_FUNCTION(Gateway_RemoveLink_NonExistingSourceModule_Find_Link_Data_Failure)
{
    //Arrange
    CGatewayLLMocks mocks;
    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    MODULE_HANDLE handle = Gateway_AddModule(gw, (GATEWAY_MODULES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_modules));

    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module2",
		dummyLoaderInfo,
        NULL
    };

    GATEWAY_LINK_ENTRY dummyLink = {
        "dummy module",
        "dummy module2"
    };

    GATEWAY_LINK_ENTRY dummyLink2 = {
        "NonExistingLink",
        "dummy module"        
    };

    MODULE_HANDLE handle2 = Gateway_AddModule(gw, &dummyEntry2);

    Gateway_AddLink(gw, (GATEWAY_LINK_ENTRY*)&dummyLink);

    mocks.ResetAllCalls();


    //Expectations
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, gw))
        .IgnoreAllArguments();

    //Act
    Gateway_RemoveLink(gw, &dummyLink2);

    //Assert
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/*Tests_SRS_GATEWAY_04_006: [ The function shall locate the LINK_DATA object in GATEWAY_HANDLE_DATA's links containing link and return if it cannot be found. ]*/
TEST_FUNCTION(Gateway_RemoveLink_NonExistingSinkModule_Find_Link_Data_Failure)
{
    //Arrange
    CGatewayLLMocks mocks;
    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    MODULE_HANDLE handle = Gateway_AddModule(gw, (GATEWAY_MODULES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_modules));

    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module2",
		dummyLoaderInfo,
        NULL
    };

    GATEWAY_LINK_ENTRY dummyLink = {
        "dummy module",
        "dummy module2"
    };

    GATEWAY_LINK_ENTRY dummyLink2 = {
        "dummy module",
        "NonExistingLink"
    };

    MODULE_HANDLE handle2 = Gateway_AddModule(gw, &dummyEntry2);

    Gateway_AddLink(gw, (GATEWAY_LINK_ENTRY*)&dummyLink);

    mocks.ResetAllCalls();


    //Expectations
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    //Act
    Gateway_RemoveLink(gw, &dummyLink2);

    //Assert
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/*Codes_SRS_GATEWAY_04_006: [ The function shall locate the LINK_DATA object in GATEWAY_HANDLE_DATA's links containing link and return if it cannot be found. ]*/
TEST_FUNCTION(Gateway_RemoveLink_NonExistingSinkModule_Find_star_Link_Data_Failure)
{
    //Arrange
    CGatewayLLMocks mocks;
    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    MODULE_HANDLE handle = Gateway_AddModule(gw, (GATEWAY_MODULES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_modules));

    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module2",
		dummyLoaderInfo,
        NULL
    };

    const char * dm2 = "dummy module2";
    GATEWAY_LINK_ENTRY dummyLink = {
        "*",
        dm2
    };

    MODULE_HANDLE handle2 = Gateway_AddModule(gw, &dummyEntry2);

    Gateway_AddLink(gw, (GATEWAY_LINK_ENTRY*)&dummyLink);

    mocks.ResetAllCalls();


    //Expectations
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, &dummyLink))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    whenShallVECTOR_find_if_fail = currentVECTOR_find_if_call + 2;
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, IGNORED_PTR_ARG, GATEWAY_MODULE_LIST_CHANGED))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    //Act
    Gateway_RemoveLink(gw, &dummyLink);

    //Assert
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/*Tests_SRS_GATEWAY_04_006: [ The function shall locate the LINK_DATA object in GATEWAY_HANDLE_DATA's links containing link and return if it cannot be found. ]*/
/*Tests_SRS_GATEWAY_04_007: [ The functional shall remove that LINK_DATA from GATEWAY_HANDLE_DATA's links. ]*/
TEST_FUNCTION(Gateway_RemoveLink_Finds_Link_Data_Success)
{
    //Arrange
    CGatewayLLMocks mocks;
    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    MODULE_HANDLE handle = Gateway_AddModule(gw, (GATEWAY_MODULES_ENTRY*)BASEIMPLEMENTATION::VECTOR_front(dummyProps->gateway_modules));

    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module2",
		dummyLoaderInfo,
        NULL
    };

    GATEWAY_LINK_ENTRY dummyLink = {
        "dummy module",
        "dummy module2"
    };

    GATEWAY_LINK_ENTRY dummyLink2 = {
        "dummy module",
        "dummy module2"
    };

    MODULE_HANDLE handle2 = Gateway_AddModule(gw, &dummyEntry2);

    Gateway_AddLink(gw, (GATEWAY_LINK_ENTRY*)&dummyLink);

    mocks.ResetAllCalls();

    //Expectations
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, &dummyLink2))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, IGNORED_PTR_ARG, GATEWAY_MODULE_LIST_CHANGED))
        .IgnoreArgument(1)
        .IgnoreArgument(2);


    //Act
    Gateway_RemoveLink(gw, &dummyLink2);

    //Assert
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/*Tests_SRS_GATEWAY_04_008: [ If gw , entryLink, entryLink->module_source or entryLink->module_source is NULL the function shall return GATEWAY_ADD_LINK_INVALID_ARG. ]*/
TEST_FUNCTION(Gateway_AddLink_with_Null_Gateway_Fail)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    GATEWAY_LINK_ENTRY dummyLink2 = {
        "dummy module",
        "dummy module2"
    };

    mocks.ResetAllCalls();
    
    //Act
    GATEWAY_ADD_LINK_RESULT result = Gateway_AddLink(NULL, &dummyLink2);

    //Assert
    ASSERT_ARE_EQUAL(GATEWAY_ADD_LINK_RESULT, GATEWAY_ADD_LINK_INVALID_ARG, result);

    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/*Tests_SRS_GATEWAY_04_008: [ If gw , entryLink, entryLink->module_source or entryLink->module_source is NULL the function shall return GATEWAY_ADD_LINK_INVALID_ARG. ]*/
TEST_FUNCTION(Gateway_AddLink_with_Null_Link_Fail)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    GATEWAY_LINK_ENTRY dummyLink2 = {
        "dummy module",
        "dummy module2"
    };

    mocks.ResetAllCalls();

    //Act
    GATEWAY_ADD_LINK_RESULT result = Gateway_AddLink(gw, NULL);

    //Assert
    ASSERT_ARE_EQUAL(GATEWAY_ADD_LINK_RESULT, GATEWAY_ADD_LINK_INVALID_ARG, result);

    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/*Tests_SRS_GATEWAY_04_008: [ If gw , entryLink, entryLink->module_source or entryLink->module_source is NULL the function shall return GATEWAY_ADD_LINK_INVALID_ARG. ]*/
TEST_FUNCTION(Gateway_AddLink_with_Null_Link_Module_Source_Fail)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    GATEWAY_LINK_ENTRY dummyLink2;
    dummyLink2.module_source = NULL;
    dummyLink2.module_sink = "Test";

    mocks.ResetAllCalls();

    //Act
    GATEWAY_ADD_LINK_RESULT result = Gateway_AddLink(gw, &dummyLink2);

    //Assert
    ASSERT_ARE_EQUAL(GATEWAY_ADD_LINK_RESULT, GATEWAY_ADD_LINK_INVALID_ARG, result);

    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/* Tests_SRS_GATEWAY_26_001: [ This function shall initialize attached Event System and report GATEWAY_STARTED event. ]*/
/* Tests_SRS_GATEWAY_26_010: [ This function shall report `GATEWAY_MODULE_LIST_CHANGED` event. ] */
TEST_FUNCTION(Gateway_Event_System_Create_And_Report)
{
    //Arrange
    CGatewayLLMocks mocks;
    
    //Expectations
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Initialize());
	EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, Broker_Create());
    EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG)); //Modules.
    EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG)); //Links
    
    expectEventSystemInit(mocks);

    //Act
    GATEWAY_HANDLE gw = Gateway_Create(NULL);

    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/* Tests_SRS_GATEWAY_26_002: [ If Event System module fails to be initialized the gateway module shall be destroyed and NULL returned with no events reported. ] */
TEST_FUNCTION(Gateway_Event_System_Create_Fail)
{
    // Arrange
    CGatewayLLMocks mocks;

    //Expectations
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Initialize());
	STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());
	EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, Broker_Create());
    EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG)); //Modules.
    EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG)); //Links
    // Fail to create
    EXPECTED_CALL(mocks, EventSystem_Init())
        .SetFailReturn((EVENTSYSTEM_HANDLE)NULL);
    // Note - no EventSystem_Report()!
    // Gateway_destroy called from inside create
    EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)); //For Modules.
    EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)); //For Links.
    EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG)); //Modules
    EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG)); //Links
    EXPECTED_CALL(mocks, Broker_Destroy(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

    //Act
    GATEWAY_HANDLE gw = Gateway_Create(NULL);

    //Assert
    ASSERT_IS_NULL(gw);
    mocks.AssertActualAndExpectedCalls();
}

/* Tests_SRS_GATEWAY_26_003: [ If the Event System module is initialized, this function shall report GATEWAY_DESTROYED event. ] */
/* Tests_SRS_GATEWAY_26_004: [ This function shall destroy the attached Event System. ] */
TEST_FUNCTION(Gateway_Event_System_Report_Destroy)
{
    // Arrange
    CGatewayLLMocks mocks;

    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    mocks.ResetAllCalls();

    // Expectations
    expectEventSystemDestroy(mocks);
    EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)); //For Modules.
    EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)); //For Links
    EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG)); //Modules
    EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG)); //Links
    EXPECTED_CALL(mocks, Broker_Destroy(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
	EXPECTED_CALL(mocks, ModuleLoader_Destroy());

    // Act
    Gateway_Destroy(gw);

    // Assert
    mocks.AssertActualAndExpectedCalls();
}

/* Tests_SRS_GATEWAY_26_006: [** This function shall log a failure and do nothing else when `gw` parameter is NULL. ] */
TEST_FUNCTION(Gateway_AddEventCallback_NULL_Gateway)
{
    // Arrange
    CGatewayLLMocks mocks;

    // Expectations
    // Empty! No callbacks should happen at all

    // Act
    Gateway_AddEventCallback(NULL, GATEWAY_STARTED, NULL, NULL);
    Gateway_AddEventCallback(NULL, GATEWAY_DESTROYED, NULL, NULL);
    Gateway_AddEventCallback(NULL, GATEWAY_STARTED, sampleCallbackFunc, NULL);

    // Assert
    ASSERT_ARE_EQUAL(int, sampleCallbackFuncCallCount, 0);
    mocks.AssertActualAndExpectedCalls();
}

/*Tests_SRS_GATEWAY_26_007: [ This function shall return a snapshot copy of information about current gateway modules. ]*/
TEST_FUNCTION(Gateway_GetModuleList_Basic)
{
    // Arrange
    CGatewayLLMocks mocks;
    GATEWAY_HANDLE gw = Gateway_Create(dummyProps);
    mocks.ResetAllCalls();

    // Expect
    EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

    // Act
    VECTOR_HANDLE modules = Gateway_GetModuleList(gw);

    // Assert
    ASSERT_IS_NOT_NULL(modules);
    ASSERT_ARE_EQUAL(int, BASEIMPLEMENTATION::VECTOR_size(modules), 1); 
    ASSERT_ARE_EQUAL(
        int,
        0,
        strcmp(
            ((GATEWAY_MODULE_INFO*)BASEIMPLEMENTATION::VECTOR_element(modules, 0))->module_name,
            "dummy module")
    );
    mocks.AssertActualAndExpectedCalls();

    // Cleanup
    Gateway_DestroyModuleList(modules);
    Gateway_Destroy(gw);
    mocks.ResetAllCalls();
}

/*Tests_SRS_GATEWAY_26_008: [ If the `gw` parameter is NULL, the function shall return NULL handle and not allocate any data. ] */
TEST_FUNCTION(Gateway_GetModuleList_NULL_Gateway)
{
    // Arrange
    CGatewayLLMocks mocks;

    // Expectations
    // Empty

    // Act
    VECTOR_HANDLE vector = Gateway_GetModuleList(NULL);

    // Assert
    ASSERT_IS_NULL(vector);
    mocks.AssertActualAndExpectedCalls();
}

/*Tests_SRS_GATEWAY_26_009: [ This function shall return a NULL handle should any internal callbacks fail. ]*/
TEST_FUNCTION(Gateway_GetModuleList_Vector_Create_Fail)
{
    // Arrange
    CGatewayLLMocks mocks;
    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    mocks.ResetAllCalls();

    // Expectations
    EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .SetFailReturn((VECTOR_HANDLE)NULL);
    EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG));

    // Act
    VECTOR_HANDLE vector = Gateway_GetModuleList(gw);

    // Assert
    ASSERT_IS_NULL(vector);
    mocks.AssertActualAndExpectedCalls();

    // Cleanup
    Gateway_Destroy(gw);
}

/*Tests_SRS_GATEWAY_26_009: [ This function shall return a NULL handle should any internal callbacks fail. ]*/
TEST_FUNCTION(Gateway_GetModuleList_push_back_fail)
{
    // Arrange
    CGatewayLLMocks mocks;
    GATEWAY_HANDLE gw = Gateway_Create(dummyProps);
    mocks.ResetAllCalls();

    // Expectations
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(GATEWAY_MODULE_INFO)));
    EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .SetFailReturn(1);
    // destroy after fail
    EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(1);

    // Act
    VECTOR_HANDLE vector = Gateway_GetModuleList(gw);

    // Assert
    ASSERT_IS_NULL(vector);
    mocks.AssertActualAndExpectedCalls();

    // Cleanup
    Gateway_Destroy(gw);
}

TEST_FUNCTION(Gateway_AddEventCallback_Forwards)
{
    // Arrange
    CGatewayLLMocks mocks;
    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    mocks.ResetAllCalls();

    // Expectations
    STRICT_EXPECTED_CALL(mocks, EventSystem_AddEventCallback(IGNORED_PTR_ARG, GATEWAY_MODULE_LIST_CHANGED, sampleCallbackFunc, NULL))
        .IgnoreArgument(1);

    // Act
    Gateway_AddEventCallback(gw, GATEWAY_MODULE_LIST_CHANGED, sampleCallbackFunc, NULL);

    // Assert
    mocks.AssertActualAndExpectedCalls();

    // Cleanup
    Gateway_Destroy(gw);
}

/*Tests_SRS_GATEWAY_04_008: [ If gw , entryLink, entryLink->module_source or entryLink->module_source is NULL the function shall return GATEWAY_ADD_LINK_INVALID_ARG. ]*/
TEST_FUNCTION(Gateway_AddLink_with_Null_Link_Module_Sink_Fail)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    GATEWAY_LINK_ENTRY dummyLink2;
    dummyLink2.module_source = "Test";
    dummyLink2.module_sink = NULL;

    mocks.ResetAllCalls();

    //Act
    GATEWAY_ADD_LINK_RESULT result = Gateway_AddLink(gw, &dummyLink2);

    //Assert
    ASSERT_ARE_EQUAL(GATEWAY_ADD_LINK_RESULT, GATEWAY_ADD_LINK_INVALID_ARG, result);

    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}


/*Tests_SRS_GATEWAY_04_010: [ If the entryLink already exists it the function shall return GATEWAY_ADD_LINK_ERROR ] */
/*Tests_SRS_GATEWAY_04_009: [ This function shall check if a given link already exists. ] */
TEST_FUNCTION(Gateway_AddLink_DuplicatedLink_Fail)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Add another entry to the properties
    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module 2",
		dummyLoaderInfo,
        NULL
    };

    GATEWAY_LINK_ENTRY dummyLink = {
        "dummy module",
        "dummy module 2"
    };

    GATEWAY_LINK_ENTRY duplicatedLink = {
        "dummy module",
        "dummy module 2"
    };

    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry2, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_links, &dummyLink, 1);

    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);
    mocks.ResetAllCalls();

    //Act
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, &duplicatedLink))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    GATEWAY_ADD_LINK_RESULT result = Gateway_AddLink(gateway, &duplicatedLink);

    //Assert
    ASSERT_ARE_EQUAL(GATEWAY_ADD_LINK_RESULT, GATEWAY_ADD_LINK_ERROR, result);

    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);
}

/*Tests_SRS_GATEWAY_04_011: [If the module referenced by the entryLink->module_source or entryLink->module_sink doesn't exists this function shall return GATEWAY_ADD_LINK_ERROR ]*/
TEST_FUNCTION(Gateway_AddLink_NonExistingSourceModule_Fail)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Add another entry to the properties
    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module 2",
		dummyLoaderInfo,
        NULL
    };

    GATEWAY_LINK_ENTRY dummyLink = {
        "dummy module",
        "dummy module 2"
    };

    const char* nem = "NonExisting";
    GATEWAY_LINK_ENTRY nonExistingModuleLink = {
        nem,
        "dummy module 2"
    };

    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry2, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_links, &dummyLink, 1);

    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);
    mocks.ResetAllCalls();

    //Act
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, &nonExistingModuleLink))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, nem))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    GATEWAY_ADD_LINK_RESULT result = Gateway_AddLink(gateway, &nonExistingModuleLink);

    //Assert
    ASSERT_ARE_EQUAL(GATEWAY_ADD_LINK_RESULT, GATEWAY_ADD_LINK_ERROR, result);

    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);
}

/*Tests_SRS_GATEWAY_04_011: [If the module referenced by the entryLink->module_source or entryLink->module_sink doesn't exists this function shall return GATEWAY_ADD_LINK_ERROR ]*/
TEST_FUNCTION(Gateway_AddLink_NonExistingSinkModule_Fail)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Add another entry to the properties
    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module 2",
		dummyLoaderInfo,
        NULL
    };

    const char * nem = "NonExisting";
    const char * dm2 = "dummy module 2";
    GATEWAY_LINK_ENTRY dummyLink = {
        "dummy module",
        dm2
    };

    GATEWAY_LINK_ENTRY nonExistingModuleLink = {
        dm2,
        nem
    };

    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry2, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_links, &dummyLink, 1);

    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);
    mocks.ResetAllCalls();

    //Act
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, &nonExistingModuleLink))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, dm2))
        .IgnoreArgument(1)
        .IgnoreArgument(2);//Check Source Module.
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, nem))
        .IgnoreArgument(1)
        .IgnoreArgument(2);//Check Sink Module.

    GATEWAY_ADD_LINK_RESULT result = Gateway_AddLink(gateway, &nonExistingModuleLink);

    //Assert
    ASSERT_ARE_EQUAL(GATEWAY_ADD_LINK_RESULT, GATEWAY_ADD_LINK_ERROR, result);

    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);
}

/*Tests_SRS_GATEWAY_04_012: [ This function shall add the entryLink to the gw->links ] */
/*Tests_SRS_GATEWAY_04_013: [If adding the link succeed this function shall return GATEWAY_ADD_LINK_SUCCESS]*/
TEST_FUNCTION(Gateway_AddLink_Succeeds)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Add another entry to the properties
    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module 2",
		dummyLoaderInfo,
        NULL
    };

    GATEWAY_LINK_ENTRY dummyLink = {
        "dummy module",
        "dummy module 2"
    };

    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry2, 1);

    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);
    mocks.ResetAllCalls();

    //Act
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();//Check link
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();//Check Source Module.
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();//Check Sink Module.
    STRICT_EXPECTED_CALL(mocks, Broker_AddLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, IGNORED_PTR_ARG, GATEWAY_MODULE_LIST_CHANGED))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    GATEWAY_ADD_LINK_RESULT result = Gateway_AddLink(gateway, &dummyLink);

    //Assert
    ASSERT_ARE_EQUAL(GATEWAY_ADD_LINK_RESULT, GATEWAY_ADD_LINK_SUCCESS, result);

    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);
}

TEST_FUNCTION(Gateway_AddLink_pushback_fails)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Add another entry to the properties
    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module 2",
		dummyLoaderInfo,
        NULL
    };

    GATEWAY_LINK_ENTRY dummyLink = {
        "dummy module",
        "dummy module 2"
    };

    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry2, 1);

    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);
    mocks.ResetAllCalls();

    //Act
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();//Check link
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();//Check Source Module.
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();//Check Sink Module.
    STRICT_EXPECTED_CALL(mocks, Broker_AddLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(100);
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    GATEWAY_ADD_LINK_RESULT result = Gateway_AddLink(gateway, &dummyLink);

    //Assert
    ASSERT_ARE_EQUAL(GATEWAY_ADD_LINK_RESULT, GATEWAY_ADD_LINK_ERROR, result);

    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);
}

TEST_FUNCTION(Gateway_AddLink_broker_add_fails)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Add another entry to the properties
    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module 2",
		dummyLoaderInfo,
        NULL
    };

    GATEWAY_LINK_ENTRY dummyLink = {
        "dummy module",
        "dummy module 2"
    };

    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry2, 1);

    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);
    mocks.ResetAllCalls();

    //Act
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();//Check link
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();//Check Source Module.
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();//Check Sink Module.
    STRICT_EXPECTED_CALL(mocks, Broker_AddLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetFailReturn(BROKER_ADD_LINK_ERROR);

     GATEWAY_ADD_LINK_RESULT result = Gateway_AddLink(gateway, &dummyLink);

    //Assert
    ASSERT_ARE_EQUAL(GATEWAY_ADD_LINK_RESULT, GATEWAY_ADD_LINK_ERROR, result);

    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);
}

TEST_FUNCTION(Gateway_AddLink_star_2nd_addbroker_fails)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Add another entry to the properties
    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module 2",
		dummyLoaderInfo,
        NULL
    };
    GATEWAY_MODULES_ENTRY dummyEntry3 = {
        "dummy module 3",
		dummyLoaderInfo,
        NULL
    };

    GATEWAY_LINK_ENTRY dummyLink1 = {
        "*",
        "dummy module 2"
    };
    GATEWAY_LINK_ENTRY dummyLink2 = {
        "*",
        "dummy module 3"
    };

    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry2, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry3, 1);

    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);

    GATEWAY_ADD_LINK_RESULT result = Gateway_AddLink(gateway, &dummyLink1);

    mocks.ResetAllCalls();

    //Act
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();//Check link
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();//Check Sink Module.
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2); // Add link to links vector
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1); // for each module.
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Broker_AddLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, Broker_AddLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetFailReturn(BROKER_ADD_LINK_ERROR);

    //Remove link
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1); // for each module.
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 2))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, VECTOR_back(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    result = Gateway_AddLink(gateway, &dummyLink2);

    //Assert
    ASSERT_ARE_EQUAL(GATEWAY_ADD_LINK_RESULT, GATEWAY_ADD_LINK_ERROR, result);

    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);
}

TEST_FUNCTION(Gateway_AddLink_star_2nd_add_push_fails)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Add another entry to the properties
    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module 2",
		dummyLoaderInfo,
        NULL
    };
    GATEWAY_MODULES_ENTRY dummyEntry3 = {
        "dummy module 3",
		dummyLoaderInfo,
        NULL
    };

    GATEWAY_LINK_ENTRY dummyLink1 = {
        "*",
        "dummy module 2"
    };
    GATEWAY_LINK_ENTRY dummyLink2 = {
        "*",
        "dummy module 3"
    };

    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry2, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry3, 1);

    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);

    GATEWAY_ADD_LINK_RESULT result = Gateway_AddLink(gateway, &dummyLink1);

    mocks.ResetAllCalls();

    //Act
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();//Check link
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();//Check Sink Module.
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(1); // Add link to links vector

    result = Gateway_AddLink(gateway, &dummyLink2);

    //Assert
    ASSERT_ARE_EQUAL(GATEWAY_ADD_LINK_RESULT, GATEWAY_ADD_LINK_ERROR, result);

    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);
}

//Tests_SRS_GATEWAY_17_003: [ The gateway shall treat a source of "*" as link to the sink module from every other module in gateway. ]
//Tests_SRS_GATEWAY_17_005: [ For this link, the sink shall receive all messages publish by other modules. ]
TEST_FUNCTION(Gateway_AddModule_Creates_Module_with_star_links)
{
    //Arrange
    CGatewayLLMocks mocks;


    //Add another entry to the properties
    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module 2",
		dummyLoaderInfo,
        NULL
    };
    GATEWAY_MODULES_ENTRY dummyEntry3 = {
        "dummy module 3",
		dummyLoaderInfo,
        NULL
    };

    GATEWAY_LINK_ENTRY dummyLink1 = {
        "*",
        "dummy module 2"
    };
    GATEWAY_LINK_ENTRY dummyLink2 = {
        "*",
        "dummy module"
    };

    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry2, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_links, &dummyLink1, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_links, &dummyLink2, 1);

    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);
    mocks.ResetAllCalls();

    //Expectations
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Load(IGNORED_PTR_ARG, dummyLoaderInfo.entrypoint))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_BuildModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_IncRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_back(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    // 1st broadcast link
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, Broker_AddLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    // 2nd broadcast link
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, Broker_AddLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, gateway, GATEWAY_MODULE_LIST_CHANGED))
        .IgnoreArgument(1);


    //Act
    MODULE_HANDLE handle = Gateway_AddModule(gateway, &dummyEntry3);

    //Assert
    ASSERT_IS_NOT_NULL(handle);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);
}

//Tests_SRS_GATEWAY_17_003: [ The gateway shall treat a source of "*" as link to the sink module from every other module in gateway. ]
//Tests_SRS_GATEWAY_17_005: [ For this link, the sink shall receive all messages publish by other modules. ]
TEST_FUNCTION(Gateway_AddModule_Creates_Module_star_2nd_addLink_fails)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module 2",
		dummyLoaderInfo,
        NULL
    };
    GATEWAY_MODULES_ENTRY dummyEntry3 = {
        "dummy module 3",
		dummyLoaderInfo,
        NULL
    };

    GATEWAY_LINK_ENTRY dummyLink1 = {
        "*",
        "dummy module 2"
    };
    GATEWAY_LINK_ENTRY dummyLink2 = {
        "*",
        "dummy module"
    };

    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry2, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_links, &dummyLink1, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_links, &dummyLink2, 1);

    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);
    mocks.ResetAllCalls();

    //Expectations
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Load(IGNORED_PTR_ARG, dummyLoaderInfo.entrypoint))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_BuildModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_IncRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_back(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    // 1st broadcast link
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, Broker_AddLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    // 2nd broadcast link
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, Broker_AddLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetFailReturn(BROKER_ADD_LINK_ERROR);

    // tear down the star link for each module
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1); // for each module.
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    // and remove the rest.
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(BROKER_ERROR);
    STRICT_EXPECTED_CALL(mocks, Broker_DecRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_back(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Unload(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    //Act
    MODULE_HANDLE handle = Gateway_AddModule(gateway, &dummyEntry3);

    //Assert
    ASSERT_IS_NULL(handle);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);
}

//Tests_SRS_GATEWAY_17_003: [ The gateway shall treat a source of "*" as link to the sink module from every other module in gateway. ]
//Tests_SRS_GATEWAY_17_005: [ For this link, the sink shall receive all messages publish by other modules. ]
TEST_FUNCTION(Gateway_AddModule_Creates_Module_star_2nd_find_fails)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module 2",
		dummyLoaderInfo,
        NULL
    };
    GATEWAY_MODULES_ENTRY dummyEntry3 = {
        "dummy module 3",
		dummyLoaderInfo,
        NULL
    };

    GATEWAY_LINK_ENTRY dummyLink1 = {
        "*",
        "dummy module 2"
    };
    GATEWAY_LINK_ENTRY dummyLink2 = {
        "*",
        "dummy module"
    };

    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry2, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_links, &dummyLink1, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_links, &dummyLink2, 1);

    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);
    mocks.ResetAllCalls();

    //Expectations
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Load(IGNORED_PTR_ARG, dummyLoaderInfo.entrypoint))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_BuildModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_IncRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_back(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    // 1st broadcast link
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, Broker_AddLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    // 2nd broadcast link
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetFailReturn(nullptr);


    // tear down broadcast link.
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetFailReturn(BROKER_REMOVE_LINK_ERROR);
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    // and remove the rest.
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_DecRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_back(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Unload(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    //Act
    MODULE_HANDLE handle = Gateway_AddModule(gateway, &dummyEntry3);

    //Assert
    ASSERT_IS_NULL(handle);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);
}

//Tests_SRS_GATEWAY_17_002: [ The gateway shall accept a link with a source of "*" and a sink of a valid module. ]
//Tests_SRS_GATEWAY_17_003: [ The gateway shall treat a source of "*" as link to the sink module from every other module in gateway. ]
//Tests_SRS_GATEWAY_17_004: [ The gateway shall accept a link containing "*" as entryLink->module_source, and a valid module name as a entryLink->module_sink. ]
//Tests_SRS_GATEWAY_17_005: [ For this link, the sink shall receive all messages publish by other modules. ]
TEST_FUNCTION(Gateway_AddLink_star_success)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module 2",
		dummyLoaderInfo,
        NULL
    };
    GATEWAY_MODULES_ENTRY dummyEntry3 = {
        "dummy module 3",
		dummyLoaderInfo,
        NULL
    };

    GATEWAY_LINK_ENTRY dummyLink1 = {
        "*",
        "dummy module 2"
    };
    GATEWAY_LINK_ENTRY dummyLink2 = {
        "*",
        "dummy module"
    };

    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry2, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry3, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_links, &dummyLink1, 1);

    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);
    mocks.ResetAllCalls();

    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, &dummyLink2))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "dummy module"))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Broker_AddLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 2))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Broker_AddLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, IGNORED_PTR_ARG, GATEWAY_MODULE_LIST_CHANGED))
        .IgnoreArgument(1)
        .IgnoreArgument(2);


    ///Act
    GATEWAY_ADD_LINK_RESULT result = Gateway_AddLink(gateway, &dummyLink2);

    //Assert
    ASSERT_ARE_EQUAL(GATEWAY_ADD_LINK_RESULT, GATEWAY_ADD_LINK_SUCCESS, result);

    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);
//Tests_SRS_GATEWAY_17_005: [ For this link, the sink shall receive all messages publish by other modules. ]
}

//Tests_SRS_GATEWAY_17_004: [ The gateway shall accept a link containing "*" as entryLink->module_source, and a valid module name as a entryLink->module_sink. ]
TEST_FUNCTION(Gateway_AddLink_star_no_sink)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module 2",
        dummyLoaderInfo,
        NULL
    };
    GATEWAY_MODULES_ENTRY dummyEntry3 = {
        "dummy module 3",
        dummyLoaderInfo,
        NULL
    };

    GATEWAY_LINK_ENTRY dummyLink1 = {
        "*",
        "dummy module 2"
    };
    const char * dm4 = "dummy module 4";
    GATEWAY_LINK_ENTRY dummyLink2 = {
        "*",
        dm4
    };

    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry2, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry3, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_links, &dummyLink1, 1);

    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);
    mocks.ResetAllCalls();

    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, &dummyLink2))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, dm4))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    ///Act
    GATEWAY_ADD_LINK_RESULT result = Gateway_AddLink(gateway, &dummyLink2);

    //Assert
    ASSERT_ARE_EQUAL(GATEWAY_ADD_LINK_RESULT, GATEWAY_ADD_LINK_ERROR, result);

    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);
}

//Tests_SRS_GATEWAY_17_003: [ The gateway shall treat a source of "*" as link to the sink module from every other module in gateway. ]
TEST_FUNCTION(Gateway_AddLink_star_failure_to_add)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module 2",
        dummyLoaderInfo,
        NULL
    };
    GATEWAY_MODULES_ENTRY dummyEntry3 = {
        "dummy module 3",
        dummyLoaderInfo,
        NULL
    };

    GATEWAY_LINK_ENTRY dummyLink1 = {
        "*",
        "dummy module 2"
    };
    GATEWAY_LINK_ENTRY dummyLink2 = {
        "*",
        "dummy module"
    };

    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry2, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry3, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_links, &dummyLink1, 1);

    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);
    mocks.ResetAllCalls();

    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, &dummyLink2))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Broker_AddLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetFailReturn(BROKER_ADD_LINK_ERROR);

    //Remove link
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1); // for each module.
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetFailReturn(BROKER_REMOVE_LINK_ERROR);
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 2))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, VECTOR_back(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    ///Act
    GATEWAY_ADD_LINK_RESULT result = Gateway_AddLink(gateway, &dummyLink2);

    //Assert
    ASSERT_ARE_EQUAL(GATEWAY_ADD_LINK_RESULT, GATEWAY_ADD_LINK_ERROR, result);

    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);
}

//Tests_SRS_GATEWAY_17_003: [ The gateway shall treat a source of "*" as link to the sink module from every other module in gateway. ]
TEST_FUNCTION(Gateway_RemoveModule_with_star_links)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Add another entry to the properties
    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module 2",
        dummyLoaderInfo,
        NULL
    };
    GATEWAY_MODULES_ENTRY dummyEntry3 = {
        "dummy module 3",
        dummyLoaderInfo,
        NULL
    };

    GATEWAY_LINK_ENTRY dummyLink1 = {
        "*",
        "dummy module 2"
    };
    GATEWAY_LINK_ENTRY dummyLink2 = {
        "*",
        "dummy module"
    };

    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry2, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_links, &dummyLink1, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_links, &dummyLink2, 1);

    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);
    auto module_handle = Gateway_AddModule(gateway, &dummyEntry3);

    mocks.ResetAllCalls();

    //Expectations
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, module_handle))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    // 1st broadcast link
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    // 2nd broadcast link
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    // and the rest of the remove...
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, Broker_DecRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(module_handle))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Unload(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, gateway, GATEWAY_MODULE_LIST_CHANGED))
        .IgnoreArgument(1);

    //Act
    Gateway_RemoveModule(gateway, module_handle);

    //Assert
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);
}

//Tests_SRS_GATEWAY_17_003: [ The gateway shall treat a source of "*" as link to the sink module from every other module in gateway. ]
TEST_FUNCTION(Gateway_RemoveModule_with_star_links_has_errors)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Add another entry to the properties
    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module 2",
        dummyLoaderInfo,
        NULL
    };
    GATEWAY_MODULES_ENTRY dummyEntry3 = {
        "dummy module 3",
        dummyLoaderInfo,
        NULL
    };

    GATEWAY_LINK_ENTRY dummyLink1 = {
        "*",
        "dummy module 2"
    };
    GATEWAY_LINK_ENTRY dummyLink2 = {
        "*",
        "dummy module"
    };

    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry2, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_links, &dummyLink1, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_links, &dummyLink2, 1);

    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);
    auto module_handle = Gateway_AddModule(gateway, &dummyEntry3);

    mocks.ResetAllCalls();

    //Expectations
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, module_handle))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    // 1st broadcast link
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetFailReturn(nullptr);
    // 2nd broadcast link
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetFailReturn(BROKER_REMOVE_LINK_ERROR);
    // and the rest of the remove...
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, Broker_DecRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(module_handle))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Unload(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, gateway, GATEWAY_MODULE_LIST_CHANGED))
        .IgnoreArgument(1);

    //Act
    Gateway_RemoveModule(gateway, module_handle);

    //Assert
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);
}

//Tests_SRS_GATEWAY_17_003: [ The gateway shall treat a source of "*" as link to the sink module from every other module in gateway. ]
TEST_FUNCTION(Gateway_RemoveLink_star_link_success)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module 2",
        dummyLoaderInfo,
        NULL
    };
    GATEWAY_MODULES_ENTRY dummyEntry3 = {
        "dummy module 3",
        dummyLoaderInfo,
        NULL
    };

    GATEWAY_LINK_ENTRY dummyLink1 = {
        "*",
        "dummy module 2"
    };
    GATEWAY_LINK_ENTRY dummyLink2 = {
        "*",
        "dummy module"
    };
    GATEWAY_LINK_ENTRY dummyLink3 = {
        "dummy module 2",
        "dummy module"
    };

    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry2, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry3, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_links, &dummyLink3, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_links, &dummyLink2, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_links, &dummyLink1, 1);

    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);
    mocks.ResetAllCalls();

    //Expectations
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, &dummyLink2))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "dummy module"))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    // 1st broadcast link
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 2))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetFailReturn(BROKER_REMOVE_LINK_ERROR);
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, IGNORED_PTR_ARG, GATEWAY_MODULE_LIST_CHANGED))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    //Act
    Gateway_RemoveLink(gateway, &dummyLink2);

    //Assert
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);
}

//Tests_SRS_GATEWAY_17_003: [ The gateway shall treat a source of "*" as link to the sink module from every other module in gateway. ]
TEST_FUNCTION(Gateway_RemoveLink_nostar_link_success)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_MODULES_ENTRY dummyEntry2 = {
        "dummy module 2",
        dummyLoaderInfo,
        NULL
    };
    GATEWAY_MODULES_ENTRY dummyEntry3 = {
        "dummy module 3",
        dummyLoaderInfo,
        NULL
    };

    GATEWAY_LINK_ENTRY dummyLink1 = {
        "*",
        "dummy module 2"
    };
    GATEWAY_LINK_ENTRY dummyLink2 = {
        "*",
        "dummy module"
    };
    GATEWAY_LINK_ENTRY dummyLink3 = {
        "dummy module 2",
        "dummy module"
    };

    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry2, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_modules, &dummyEntry3, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_links, &dummyLink2, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_links, &dummyLink1, 1);
    BASEIMPLEMENTATION::VECTOR_push_back(dummyProps->gateway_links, &dummyLink3, 1);

    GATEWAY_HANDLE gateway = Gateway_Create(dummyProps);
    mocks.ResetAllCalls();

    //Expectations
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, &dummyLink3))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetFailReturn(BROKER_REMOVE_LINK_ERROR);
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, IGNORED_PTR_ARG, GATEWAY_MODULE_LIST_CHANGED))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    //Act
    Gateway_RemoveLink(gateway, &dummyLink3);

    //Assert
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);
}

/*Tests_SRS_GATEWAY_26_011: [ The function shall report `GATEWAY_MODULE_LIST_CHANGED` event after successfully adding the module. ]*/
TEST_FUNCTION(Gateway_AddModule_Reports_On_Success)
{
    // Arrange
    CGatewayLLMocks mocks;

    GATEWAY_MODULES_ENTRY dummyModule = {
        "dummy module",
        dummyLoaderInfo,
        NULL
    };

    GATEWAY_HANDLE gateway = Gateway_Create(NULL);
    mocks.ResetAllCalls();

    //Expectations
    // Implementation doesn't matter as long as the report event is called
    mocks.SetIgnoreUnexpectedCalls(true);
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, gateway, GATEWAY_MODULE_LIST_CHANGED))
        .IgnoreArgument(1);

    //Act
    Gateway_AddModule(gateway, &dummyModule);

    //Cleanup
    Gateway_Destroy(gateway);
}

/*Tests_SRS_GATEWAY_26_012: [ The function shall report `GATEWAY_MODULE_LIST_CHANGED` event after successfully removing the module. ]*/
TEST_FUNCTION(Gateway_RemoveModule_Reports_On_Success)
{
    // Arrange
    CGatewayLLMocks mocks;

    GATEWAY_MODULES_ENTRY dummyModule = {
        "dummy module",
        dummyLoaderInfo,
        NULL
    };

    GATEWAY_HANDLE gateway = Gateway_Create(NULL);
    MODULE_HANDLE module_h = Gateway_AddModule(gateway, &dummyModule);
    mocks.ResetAllCalls();

    //Expectations
    // Implementation doesn't matter as long as the report event is called
    mocks.SetIgnoreUnexpectedCalls(true);
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, gateway, GATEWAY_MODULE_LIST_CHANGED))
        .IgnoreArgument(1);

    //Act
    Gateway_RemoveModule(gateway, module_h);

    //Cleanup
    Gateway_Destroy(gateway);
}

/*Tests_SRS_GATEWAY_26_013: [ For each module returned this function shall provide a snapshot copy vector of link sources for that module. ]*/
TEST_FUNCTION(Gateway_GetModuleList_Links_basic_tree)
{
    //Arrange
    CGatewayLLMocks mocks;
    // Using only for implemenation instead of checking calls
    mocks.SetIgnoreUnexpectedCalls(true);

    const size_t module_count = 3;
    const size_t link_count = 2;

    PREDICATE_FUNCTION search_lambda = [](const void *info, const void *searched) { return strcmp(((const GATEWAY_MODULE_INFO*)info)->module_name, (const char*)searched) == 0; };

    GATEWAY_MODULES_ENTRY module_entries[] = {
        {
            "module_1",
            dummyLoaderInfo,
            NULL
        },
        {
            "module_2",
            dummyLoaderInfo,
            NULL
        },
        {
            "module_3",
            dummyLoaderInfo,
            NULL
        }
    };

    GATEWAY_LINK_ENTRY link_entries[] = {
        {"module_1", "module_2"},
        {"module_1", "module_3"}
    };

    GATEWAY_PROPERTIES props;
    props.gateway_modules = VECTOR_create(sizeof(GATEWAY_MODULES_ENTRY));
    props.gateway_links = VECTOR_create(sizeof(GATEWAY_LINK_ENTRY));
    VECTOR_push_back(props.gateway_modules, module_entries, module_count);
    VECTOR_push_back(props.gateway_links, link_entries, link_count);

    auto gateway = Gateway_Create(&props);

    //Act
    auto modules = Gateway_GetModuleList(gateway);
    auto module_1 = (GATEWAY_MODULE_INFO*)VECTOR_find_if(modules, search_lambda, "module_1");
    auto module_2 = (GATEWAY_MODULE_INFO*)VECTOR_find_if(modules, search_lambda, "module_2");
    auto module_3 = (GATEWAY_MODULE_INFO*)VECTOR_find_if(modules, search_lambda, "module_3");

    // Assert
    ASSERT_ARE_EQUAL(size_t, VECTOR_size(modules), module_count);
    ASSERT_IS_NOT_NULL(module_1);
    ASSERT_IS_NOT_NULL(module_2);
    ASSERT_IS_NOT_NULL(module_3);
    ASSERT_IS_NOT_NULL(module_1->module_sources);
    ASSERT_ARE_EQUAL(size_t, VECTOR_size(module_1->module_sources), 0);
    ASSERT_ARE_EQUAL(size_t, VECTOR_size(module_2->module_sources), 1);
    ASSERT_ARE_EQUAL(size_t, VECTOR_size(module_3->module_sources), 1);
    ASSERT_IS_TRUE(*(GATEWAY_MODULE_INFO**)(VECTOR_element(module_2->module_sources, 0)) == module_1);
    ASSERT_IS_TRUE(*(GATEWAY_MODULE_INFO**)(VECTOR_element(module_3->module_sources, 0)) == module_1);

    // Cleanup
    VECTOR_destroy(props.gateway_modules);
    VECTOR_destroy(props.gateway_links);
    Gateway_DestroyModuleList(modules);
    Gateway_Destroy(gateway);
}

/*Tests_SRS_GATEWAY_26_013: [ For each module returned this function shall provide a snapshot copy vector of link sources for that module. ]*/
TEST_FUNCTION(Gateway_GetModuleList_links_cycle)
{
    //Arrange
    CGatewayLLMocks mocks;
    // Using only for implemenation instead of checking calls
    mocks.SetIgnoreUnexpectedCalls(true);

    const size_t module_count = 3;
    const size_t link_count = 3;

    PREDICATE_FUNCTION search_lambda = [](const void *info, const void *searched) { return strcmp(((const GATEWAY_MODULE_INFO*)info)->module_name, (const char*)searched) == 0; };

    GATEWAY_MODULES_ENTRY module_entries[] = {
        {
            "module_1",
			dummyLoaderInfo,
            NULL
        },
        {
            "module_2",
			dummyLoaderInfo,
            NULL
        },
        {
            "module_3",
			dummyLoaderInfo,
            NULL
        }
    };

    GATEWAY_LINK_ENTRY link_entries[] = {
        { "module_1", "module_2" },
        { "module_2", "module_3" },
        { "module_3", "module_1" }
    };

    GATEWAY_PROPERTIES props;
    props.gateway_modules = VECTOR_create(sizeof(GATEWAY_MODULES_ENTRY));
    props.gateway_links = VECTOR_create(sizeof(GATEWAY_LINK_ENTRY));
    VECTOR_push_back(props.gateway_modules, module_entries, module_count);
    VECTOR_push_back(props.gateway_links, link_entries, link_count);

    auto gateway = Gateway_Create(&props);

    //Act
    auto modules = Gateway_GetModuleList(gateway);
    auto module_1 = (GATEWAY_MODULE_INFO*)VECTOR_find_if(modules, search_lambda, "module_1");
    auto module_2 = (GATEWAY_MODULE_INFO*)VECTOR_find_if(modules, search_lambda, "module_2");
    auto module_3 = (GATEWAY_MODULE_INFO*)VECTOR_find_if(modules, search_lambda, "module_3");

    // Assert
    ASSERT_ARE_EQUAL(size_t, VECTOR_size(modules), module_count);
    ASSERT_IS_NOT_NULL(module_1);
    ASSERT_IS_NOT_NULL(module_2);
    ASSERT_IS_NOT_NULL(module_3);
    ASSERT_IS_NOT_NULL(module_1->module_sources);
    ASSERT_ARE_EQUAL(size_t, VECTOR_size(module_1->module_sources), 1);
    ASSERT_ARE_EQUAL(size_t, VECTOR_size(module_2->module_sources), 1);
    ASSERT_ARE_EQUAL(size_t, VECTOR_size(module_3->module_sources), 1);
    ASSERT_IS_TRUE(*(GATEWAY_MODULE_INFO**)(VECTOR_element(module_2->module_sources, 0)) == module_1);
    ASSERT_IS_TRUE(*(GATEWAY_MODULE_INFO**)(VECTOR_element(module_3->module_sources, 0)) == module_2);
    ASSERT_IS_TRUE(*(GATEWAY_MODULE_INFO**)(VECTOR_element(module_1->module_sources, 0)) == module_3);

    // Cleanup
    VECTOR_destroy(props.gateway_modules);
    VECTOR_destroy(props.gateway_links);
    Gateway_DestroyModuleList(modules);
    Gateway_Destroy(gateway);
}

/*Tests_SRS_GATEWAY_26_013: [ For each module returned this function shall provide a snapshot copy vector of link sources for that module. ]*/
TEST_FUNCTION(Gateway_GetModuleList_links_to_itself)
{
    //Arrange
    CGatewayLLMocks mocks;
    // Using only for implemenation instead of checking calls
    mocks.SetIgnoreUnexpectedCalls(true);

    const size_t module_count = 1;
    const size_t link_count = 1;

    PREDICATE_FUNCTION search_lambda = [](const void *info, const void *searched) { return strcmp(((const GATEWAY_MODULE_INFO*)info)->module_name, (const char*)searched) == 0; };

    GATEWAY_MODULES_ENTRY module_entries[] = {
        {
            "module_1",
			dummyLoaderInfo,
            NULL
        }
    };

    GATEWAY_LINK_ENTRY link_entries[] = {
        { "module_1", "module_1" }
    };

    GATEWAY_PROPERTIES props;
    props.gateway_modules = VECTOR_create(sizeof(GATEWAY_MODULES_ENTRY));
    props.gateway_links = VECTOR_create(sizeof(GATEWAY_LINK_ENTRY));
    VECTOR_push_back(props.gateway_modules, module_entries, module_count);
    VECTOR_push_back(props.gateway_links, link_entries, link_count);

    auto gateway = Gateway_Create(&props);

    //Act
    auto modules = Gateway_GetModuleList(gateway);
    auto module_1 = (GATEWAY_MODULE_INFO*)VECTOR_find_if(modules, search_lambda, "module_1");

    // Assert
    ASSERT_ARE_EQUAL(size_t, VECTOR_size(modules), module_count);
    ASSERT_IS_NOT_NULL(module_1);
    ASSERT_IS_NOT_NULL(module_1->module_sources);
    ASSERT_ARE_EQUAL(size_t, VECTOR_size(module_1->module_sources), 1);
    ASSERT_IS_TRUE(*(GATEWAY_MODULE_INFO**)(VECTOR_element(module_1->module_sources, 0)) == module_1);

    // Cleanup
    VECTOR_destroy(props.gateway_modules);
    VECTOR_destroy(props.gateway_links);
    Gateway_DestroyModuleList(modules);
    Gateway_Destroy(gateway);
}

/*Tests_SRS_GATEWAY_26_013: [ For each module returned this function shall provide a snapshot copy vector of link sources for that module. ]*/
TEST_FUNCTION(Gateway_GetModuleList_no_modules)
{
    //Arrange
    CGatewayLLMocks mocks;
    // Using only for implemenation instead of checking calls
    mocks.SetIgnoreUnexpectedCalls(true);

    auto gateway = Gateway_Create(NULL);

    //Act
    auto modules = Gateway_GetModuleList(gateway);

    // Assert
    ASSERT_IS_NOT_NULL(modules);
    ASSERT_ARE_EQUAL(size_t, VECTOR_size(modules), 0);

    // Cleanup
    Gateway_DestroyModuleList(modules);
    Gateway_Destroy(gateway);
}

/*Tests_SRS_GATEWAY_26_014: [ For each module returned that has '*' as a link source this function shall provide NULL vector pointer as it's sources vector. ]*/
TEST_FUNCTION(Gateway_GetModuleList_links_star)
{
    //Arrange
    CGatewayLLMocks mocks;
    // Using only for implemenation instead of checking calls
    mocks.SetIgnoreUnexpectedCalls(true);

    const size_t module_count = 2;
    const size_t link_count = 2;

    PREDICATE_FUNCTION search_lambda = [](const void *info, const void *searched) { return strcmp(((const GATEWAY_MODULE_INFO*)info)->module_name, (const char*)searched) == 0; };

    GATEWAY_MODULES_ENTRY module_entries[] = {
        {
            "module_1",
			dummyLoaderInfo,
            NULL
        },
        {
            "module_2",
			dummyLoaderInfo,
            NULL
        }
    };

    GATEWAY_LINK_ENTRY link_entries[] = {
        { "*", "module_1" },
        { "module_1", "module_2" }
    };

    GATEWAY_PROPERTIES props;
    props.gateway_modules = VECTOR_create(sizeof(GATEWAY_MODULES_ENTRY));
    props.gateway_links = VECTOR_create(sizeof(GATEWAY_LINK_ENTRY));
    VECTOR_push_back(props.gateway_modules, module_entries, module_count);
    VECTOR_push_back(props.gateway_links, link_entries, link_count);

    auto gateway = Gateway_Create(&props);

    //Act
    auto modules = Gateway_GetModuleList(gateway);
    auto module_1 = (GATEWAY_MODULE_INFO*)VECTOR_find_if(modules, search_lambda, "module_1");
    auto module_2 = (GATEWAY_MODULE_INFO*)VECTOR_find_if(modules, search_lambda, "module_2");

    // Assert
    ASSERT_ARE_EQUAL(size_t, VECTOR_size(modules), module_count);
    ASSERT_IS_NOT_NULL(module_1);
    ASSERT_IS_NOT_NULL(module_2);
    ASSERT_IS_NULL(module_1->module_sources);
    ASSERT_ARE_EQUAL(size_t, VECTOR_size(module_2->module_sources), 1);
    ASSERT_IS_TRUE(*(GATEWAY_MODULE_INFO**)(VECTOR_element(module_2->module_sources, 0)) == module_1);

    // Cleanup
    VECTOR_destroy(props.gateway_modules);
    VECTOR_destroy(props.gateway_links);
    Gateway_DestroyModuleList(modules);
    Gateway_Destroy(gateway);
}

TEST_FUNCTION(Gateway_GetModuleList_calloc_fail)
{
    //Arrange
    CGatewayLLMocks mocks;

    const size_t module_count = 1;
    const size_t link_count = 1;

    GATEWAY_MODULES_ENTRY module_entries[] = {
        {
            "module_1",
			dummyLoaderInfo,
            NULL
        }
    };

    GATEWAY_LINK_ENTRY link_entries[] = {
        { "module_1", "module_1" }
    };

    GATEWAY_PROPERTIES props;
    props.gateway_modules = VECTOR_create(sizeof(GATEWAY_MODULES_ENTRY));
    props.gateway_links = VECTOR_create(sizeof(GATEWAY_LINK_ENTRY));
    VECTOR_push_back(props.gateway_modules, module_entries, module_count);
    VECTOR_push_back(props.gateway_links, link_entries, link_count);

    auto gateway = Gateway_Create(&props);
    mocks.ResetAllCalls();

    // Expect
    EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG))
        .SetFailReturn((void*)NULL);
    EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG));

    //Act
    auto modules = Gateway_GetModuleList(gateway);

    // Assert
    ASSERT_IS_NULL(modules);
    mocks.AssertActualAndExpectedCalls();

    // Cleanup
    VECTOR_destroy(props.gateway_modules);
    VECTOR_destroy(props.gateway_links);
    Gateway_Destroy(gateway);
}

TEST_FUNCTION(Gateway_modules_sources_vector_create_fail)
{
    //Arrange
    CGatewayLLMocks mocks;

    const size_t module_count = 1;
    const size_t link_count = 1;

    GATEWAY_MODULES_ENTRY module_entries[] = {
        {
            "module_1",
			dummyLoaderInfo,
            NULL
        }
    };

    GATEWAY_LINK_ENTRY link_entries[] = {
        { "module_1", "module_1" }
    };

    GATEWAY_PROPERTIES props;
    props.gateway_modules = VECTOR_create(sizeof(GATEWAY_MODULES_ENTRY));
    props.gateway_links = VECTOR_create(sizeof(GATEWAY_LINK_ENTRY));
    VECTOR_push_back(props.gateway_modules, module_entries, module_count);
    VECTOR_push_back(props.gateway_links, link_entries, link_count);

    auto gateway = Gateway_Create(&props);
    mocks.ResetAllCalls();

    // Expect
    EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .ExpectedTimesExactly(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(GATEWAY_MODULE_INFO*)))
        .SetFailReturn((VECTOR_HANDLE)NULL);
    EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

    //Act
    auto modules = Gateway_GetModuleList(gateway);

    // Assert
    ASSERT_IS_NULL(modules);
    mocks.AssertActualAndExpectedCalls();

    // Cleanup
    VECTOR_destroy(props.gateway_modules);
    VECTOR_destroy(props.gateway_links);
    Gateway_Destroy(gateway);
}

TEST_FUNCTION(Gateway_GetModuleList_link_push_back_fail)
{
    //Arrange
    CGatewayLLMocks mocks;

    const size_t module_count = 1;
    const size_t link_count = 1;

    GATEWAY_MODULES_ENTRY module_entries[] = {
        {
            "module_1",
			dummyLoaderInfo,
            NULL
        }
    };

    GATEWAY_LINK_ENTRY link_entries[] = {
        { "module_1", "module_1" }
    };

    GATEWAY_PROPERTIES props;
    props.gateway_modules = VECTOR_create(sizeof(GATEWAY_MODULES_ENTRY));
    props.gateway_links = VECTOR_create(sizeof(GATEWAY_LINK_ENTRY));
    VECTOR_push_back(props.gateway_modules, module_entries, module_count);
    VECTOR_push_back(props.gateway_links, link_entries, link_count);

    auto gateway = Gateway_Create(&props);
    mocks.ResetAllCalls();

    // Expect
    EXPECTED_CALL(mocks, VECTOR_create(IGNORED_NUM_ARG))
        .ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .ExpectedTimesExactly(3);
    EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .SetFailReturn(1);
    EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .ExpectedTimesExactly(2);

    //Act
    auto modules = Gateway_GetModuleList(gateway);

    // Assert
    ASSERT_IS_NULL(modules);
    mocks.AssertActualAndExpectedCalls();

    // Cleanup
    VECTOR_destroy(props.gateway_modules);
    VECTOR_destroy(props.gateway_links);
    Gateway_Destroy(gateway);
}

/*Tests_SRS_GATEWAY_26_011: [ This function shall destroy the `module_sources` list of each `GATEWAY_MODULE_INFO` ]*/
/*Tests_SRS_GATEWAY_26_012: [ This function shall destroy the list of `GATEWAY_MODULE_INFO` ]*/
TEST_FUNCTION(Gateway_DestroyModuleList_basic)
{
    //Arrange
    CGatewayLLMocks mocks;

    const size_t module_count = 2;
    const size_t link_count = 4;

    GATEWAY_MODULES_ENTRY module_entries[] = {
        {
            "module_1",
			dummyLoaderInfo,
            NULL
        },
        {
            "module_2",
            dummyLoaderInfo,
            NULL
        }
    };

    GATEWAY_LINK_ENTRY link_entries[] = {
        { "module_1", "module_1" },
        { "module_2", "module_2" },
        { "module_1", "module_2" },
        { "module_2", "module_1" }
    };

    GATEWAY_PROPERTIES props;
    props.gateway_modules = VECTOR_create(sizeof(GATEWAY_MODULES_ENTRY));
    props.gateway_links = VECTOR_create(sizeof(GATEWAY_LINK_ENTRY));
    VECTOR_push_back(props.gateway_modules, module_entries, module_count);
    VECTOR_push_back(props.gateway_links, link_entries, link_count);

    auto gateway = Gateway_Create(&props);
    auto modules = Gateway_GetModuleList(gateway);
    mocks.ResetAllCalls();

    // Expect
    EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .ExpectedTimesExactly(3);

    // Act
    Gateway_DestroyModuleList(modules);

    // Assert
    mocks.AssertActualAndExpectedCalls();

    // Cleanup
    VECTOR_destroy(props.gateway_modules);
    VECTOR_destroy(props.gateway_links);
    Gateway_Destroy(gateway);
}

TEST_FUNCTION(Gateway_DestroyModuleList_null)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Expect
    STRICT_EXPECTED_CALL(mocks, VECTOR_front(NULL));
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(NULL));
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(NULL));

    //Act
    Gateway_DestroyModuleList(NULL);

    //Assert
    mocks.AssertActualAndExpectedCalls();
}

/* Tests_SRS_GATEWAY_26_015: [ If `gw` or `module_name` is `NULL` the function shall do nothing and return with non - zero result. ] */
TEST_FUNCTION(Gateway_RemoveModuleByName_Null_gw)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Expect
    //Nothing!
    
    //Act
    int result = Gateway_RemoveModuleByName(NULL, "foo");

    //Assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    mocks.AssertActualAndExpectedCalls();
}

/* Tests_SRS_GATEWAY_26_017: [ If module with `module_name` name is not found this function shall return non - zero and do nothing. ] */
/* Tests_SRS_GATEWAY_26_015: [ If `gw` or `module_name` is `NULL` the function shall do nothing and return with non - zero result. ] */
TEST_FUNCTION(Gateway_RemoveModuleByName_Null_name)
{
    //Arrange
    CGatewayLLMocks mocks;
    auto gw = Gateway_Create(NULL);
    mocks.ResetAllCalls();

    //Expect
    //Nothing!

    //Act
    int result = Gateway_RemoveModuleByName(gw, NULL);

    //Assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/* Tests_SRS_GATEWAY_26_017: [ If module with `module_name` name is not found this function shall return non - zero and do nothing. ] */
TEST_FUNCTION(Gateway_RemoveModuleByName_no_modules)
{
    //Arrange
    CGatewayLLMocks mocks;
    auto gw = Gateway_Create(NULL);
    mocks.ResetAllCalls();

    //Expect
    EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //Act
    int result = Gateway_RemoveModuleByName(gw, "foo");

    //Assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/* Tests_SRS_GATEWAY_26_017: [ If module with `module_name` name is not found this function shall return non - zero and do nothing. ] */
TEST_FUNCTION(Gateway_RemoveModuleByName_not_existing)
{
    //Arrange
    CGatewayLLMocks mocks;
    auto gw = Gateway_Create(dummyProps);
    mocks.ResetAllCalls();

    //Expect
    EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //Act
    int result = Gateway_RemoveModuleByName(gw, "foo");

    //Assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/* Tests_SRS_GATEWAY_26_016: [ The function shall return 0 if the module was found. ] */
TEST_FUNCTION(Gateway_RemoveModuleByName_success)
{
    //Arrange
    CGatewayLLMocks mocks;
    auto gw = Gateway_Create(dummyProps);
    mocks.ResetAllCalls();

    //Expect
    EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, Broker_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, Broker_DecRef(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, DynamicModuleLoader_Unload(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, IGNORED_PTR_ARG, GATEWAY_MODULE_LIST_CHANGED));

    //Act
    int result = Gateway_RemoveModuleByName(gw, "dummy module");

    //Assert
    ASSERT_ARE_EQUAL(int, 0, result);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
}

/* Tests_SRS_GATEWAY_26_018: [ This function shall remove any links that contain the removed module either as a source or sink. ] */
TEST_FUNCTION(Gateway_RemoveModule_removes_links)
{
    // Arrange
    CNiceCallComparer<CGatewayLLMocks> mocks;

    GATEWAY_MODULES_ENTRY modules[] = {
        {
            "module1",
			dummyLoaderInfo,
            NULL
        },
        {
            "module2",
			dummyLoaderInfo,
            NULL
        },
        {
            "module3",
			dummyLoaderInfo,
            NULL
        }
    };

    GATEWAY_LINK_ENTRY links[] = {
        {
            "module1",
            "module2"
        },
        {
            "module3",
            "module1"
        },
        {
            "module2",
            "module3"
        }
    };

    GATEWAY_PROPERTIES props;
    props.gateway_modules = VECTOR_create(sizeof(GATEWAY_MODULES_ENTRY));
    props.gateway_links = VECTOR_create(sizeof(GATEWAY_LINK_ENTRY));
    VECTOR_push_back(props.gateway_modules, modules, 3);
    VECTOR_push_back(props.gateway_links, links, 3);

    auto gw = Gateway_Create(&props);
    
    // Expect
    EXPECTED_CALL(mocks, Broker_RemoveLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .ExpectedTimesExactly(2);
    // remove from gw->links + remove module
    EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .ExpectedTimesExactly(3);

    // Act
    int result = Gateway_RemoveModuleByName(gw, "module1");

    // Assert
    ASSERT_ARE_EQUAL(int, result, 0);
    mocks.AssertActualAndExpectedCalls();

    // Cleanup
    VECTOR_destroy(props.gateway_modules);
    VECTOR_destroy(props.gateway_links);
    Gateway_Destroy(gw);
}

/*Tests_SRS_GATEWAY_26_019: [ The function shall report `GATEWAY_MODULE_LIST_CHANGED` event after successfully adding the link. ]*/
TEST_FUNCTION(Gateway_AddLink_reports_on_success)
{
    // Arrange
    CNiceCallComparer<CGatewayLLMocks> mocks;
    auto gateway = Gateway_Create(dummyProps);
    GATEWAY_LINK_ENTRY entry = {
        "dummy module",
        "dummy module"
    };

    // Expect
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, IGNORED_PTR_ARG, GATEWAY_MODULE_LIST_CHANGED))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    // Act
    auto result = Gateway_AddLink(gateway, &entry);

    // Assert
    ASSERT_IS_TRUE(result == GATEWAY_ADD_LINK_SUCCESS);
    mocks.AssertActualAndExpectedCalls();

    // Cleanup
    Gateway_Destroy(gateway);
}

/*Tests_SRS_GATEWAY_26_019: [ The function shall report `GATEWAY_MODULE_LIST_CHANGED` event after successfully adding the link. ]*/
TEST_FUNCTION(Gateway_AddLink_no_report_on_fail)
{
    // Arrange
    CNiceCallComparer<CGatewayLLMocks> mocks;
    auto gateway = Gateway_Create(dummyProps);
    GATEWAY_LINK_ENTRY entry = {
        "dummy module",
        "dummy module"
    };

    // Expect
    EXPECTED_CALL(mocks, Broker_AddLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetFailReturn(BROKER_ADD_LINK_ERROR);
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, IGNORED_PTR_ARG, GATEWAY_MODULE_LIST_CHANGED))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .NeverInvoked();

    // Act
    auto result = Gateway_AddLink(gateway, &entry);

    // Assert
    ASSERT_IS_TRUE(result == GATEWAY_ADD_LINK_ERROR);
    mocks.AssertActualAndExpectedCalls();

    // Cleanup
    Gateway_Destroy(gateway);
}

TEST_FUNCTION(Gateway_RemoveLink_reports)
{
    // Arrange
    CNiceCallComparer<CGatewayLLMocks> mocks;
    auto gateway = Gateway_Create(dummyProps);
    GATEWAY_LINK_ENTRY entry = {
        "dummy module",
        "dummy module"
    };
    Gateway_AddLink(gateway, &entry);
    mocks.ResetAllCalls();

    // Expect
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, IGNORED_PTR_ARG, GATEWAY_MODULE_LIST_CHANGED))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    // Act
    Gateway_RemoveLink(gateway, &entry);

    // Assert
    mocks.AssertActualAndExpectedCalls();

    // Cleanup
    Gateway_Destroy(gateway);
}

/*Tests_SRS_GATEWAY_26_020: [ The function shall make a copy of the name of the module for internal use. ]*/
TEST_FUNCTION(Gateway_AddModule_name_is_copied)
{
    // Arrange
    CNiceCallComparer<CGatewayLLMocks> mocks;
    char *name = (char*)malloc(2);
    name[0] = 'a';
    name[1] = '\0';
    GATEWAY_MODULES_ENTRY module = {
        name,
		dummyLoaderInfo,
        NULL
    };
    GATEWAY_PROPERTIES props;
    props.gateway_modules = VECTOR_create(sizeof(GATEWAY_MODULES_ENTRY));
    props.gateway_links = NULL;
    VECTOR_push_back(props.gateway_modules, &module, 1);

    // Act
    auto gateway = Gateway_Create(&props);
    name[0] = 'X';
    auto module_list = Gateway_GetModuleList(gateway);
    GATEWAY_MODULE_INFO *module_info = (GATEWAY_MODULE_INFO*)VECTOR_element(module_list, 0);

    // Assert
    ASSERT_ARE_EQUAL(size_t, VECTOR_size(module_list), 1);
    ASSERT_ARE_NOT_EQUAL(int, strcmp(module_info->module_name, name), 0);

    // Cleanup
    VECTOR_destroy(props.gateway_modules);
    free(name);
    Gateway_DestroyModuleList(module_list);
    Gateway_Destroy(gateway);
}

/*Tests_SRS_GATEWAY_14_030: [ If any internal API call is unsuccessful after a module is created, the library will be unloaded and the module destroyed. ] */
TEST_FUNCTION(Gateway_AddModule_malloc_name_fail)
{
    // Arrange
    CGatewayLLMocks mocks;
    auto gateway = Gateway_Create(NULL);
    mocks.ResetAllCalls();
    GATEWAY_MODULES_ENTRY module = {
        "asd",
		dummyLoaderInfo,
        NULL
    };

    // Expect
    EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, DynamicModuleLoader_Load(IGNORED_PTR_ARG, dummyLoaderInfo.entrypoint));
    EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_BuildModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    EXPECTED_CALL(mocks, Broker_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetFailReturn(0);
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, DynamicModuleLoader_Unload(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, Broker_RemoveModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // Act
    auto result = Gateway_AddModule(gateway, &module);

    // Assert
    ASSERT_IS_NULL(result);
    mocks.AssertActualAndExpectedCalls();

    // Cleanup
    Gateway_Destroy(gateway);
}

//Tests_SRS_GATEWAY_17_010: [ This function shall call Module_Start for every module which defines the start function. ]
//Tests_SRS_GATEWAY_17_012: [ This function shall report a GATEWAY_STARTED events. ]
//Tests_SRS_GATEWAY_17_013: [ This function shall return GATEWAY_START_SUCCESS upon completion. ]
TEST_FUNCTION(Gateway_Start_starts_stuff)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    bool* properties = (bool*)malloc(sizeof(bool));
    *properties = true;
    GATEWAY_MODULES_ENTRY entry1 = {
        "Test module1",
		dummyLoaderInfo,
        properties
    };
    GATEWAY_MODULES_ENTRY entry2 = {
        "Test module2",
		dummyLoaderInfo,
        properties
    };
    const MODULE_API_1 dummyAPIs_nostart = 
    {
        {MODULE_API_VERSION_1},

        mock_Module_ParseConfigurationFromJson,
		mock_Module_FreeConfiguration,
        mock_Module_Create,
        mock_Module_Destroy,
        mock_Module_Receive,
        NULL
    };
    const MODULE_API * dummy_nostart_api = reinterpret_cast<const MODULE_API*>(&dummyAPIs_nostart);

    MODULE_HANDLE handle1 = Gateway_AddModule(gw, &entry1);
    MODULE_HANDLE handle2 = Gateway_AddModule(gw, &entry2);
    mocks.ResetAllCalls();

    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(dummy_nostart_api);

    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, mock_Module_Start(handle2));

    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, gw, GATEWAY_STARTED))
        .IgnoreArgument(1);

    //Act
    auto result = Gateway_Start(gw);

    //Assert
    ASSERT_ARE_EQUAL(GATEWAY_START_RESULT, result, GATEWAY_START_SUCCESS);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
    free(properties);
}

//Tests_SRS_GATEWAY_17_009: [ This function shall return GATEWAY_START_INVALID_ARGS if a NULL gateway is received. ]
TEST_FUNCTION(Gateway_Start_null_gw_returns_error)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Act
    auto result = Gateway_Start(NULL);

    //Assert
    ASSERT_ARE_EQUAL(GATEWAY_START_RESULT, result, GATEWAY_START_INVALID_ARGS);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
}

//Tests_SRS_GATEWAY_17_008: [ When module is found, if the Module_Start function is defined for this module, the Module_Start function shall be called. ]
TEST_FUNCTION(Gateway_StartModule_starts_module)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    bool* properties = (bool*)malloc(sizeof(bool));
    *properties = true;
    GATEWAY_MODULES_ENTRY entry = {
        "Test module",
		dummyLoaderInfo,
        properties
    };
    MODULE_HANDLE handle = Gateway_AddModule(gw, &entry);
    mocks.ResetAllCalls();

    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, handle))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, mock_Module_Start(handle));

    //Act

    Gateway_StartModule(gw, handle);

    //Assert
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
    free(properties);
}

//Tests_SRS_GATEWAY_17_008: [ When module is found, if the Module_Start function is defined for this module, the Module_Start function shall be called. ]
TEST_FUNCTION(Gateway_StartModule_no_start_for_null_start_func)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    bool* properties = (bool*)malloc(sizeof(bool));
    *properties = true;
    GATEWAY_MODULES_ENTRY entry = {
        "Test module",
		dummyLoaderInfo,
        properties
    };

    const MODULE_API_1 dummyAPIs_nostart = 
    {
        {MODULE_API_VERSION_1},

        mock_Module_ParseConfigurationFromJson,
		mock_Module_FreeConfiguration,
        mock_Module_Create,
        mock_Module_Destroy,
        mock_Module_Receive,
        NULL
    };
    const MODULE_API * dummy_nostart_api = reinterpret_cast<const MODULE_API*>(&dummyAPIs_nostart);

    MODULE_HANDLE handle = Gateway_AddModule(gw, &entry);
    mocks.ResetAllCalls();


    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, handle))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(dummy_nostart_api);

    //Act
    Gateway_StartModule(gw, handle);

    //Assert
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
    free(properties);
}

//Tests_SRS_GATEWAY_17_007: [ If module is not found in the gateway, this function shall do nothing. ]
TEST_FUNCTION(Gateway_StartModule_no_module)
{
    //Arrange
    CGatewayLLMocks mocks;

    GATEWAY_HANDLE gw = Gateway_Create(NULL);
    bool* properties = (bool*)malloc(sizeof(bool));
    *properties = true;
    GATEWAY_MODULES_ENTRY entry = {
        "Test module",
		dummyLoaderInfo,
        properties
    };
    MODULE_HANDLE handle = Gateway_AddModule(gw, &entry);
    mocks.ResetAllCalls();

    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, handle))
        .IgnoreAllArguments();

    //Act

    Gateway_StartModule(gw, NULL);

    //Assert
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gw);
    free(properties);
}

//Tests_SRS_GATEWAY_17_006: [ If gw is NULL, this function shall do nothing. ]
TEST_FUNCTION(Gateway_StartModule_no_gateway)
{
    //Arrange
    CGatewayLLMocks mocks;

    //Act
    Gateway_StartModule(NULL, NULL);

    //Assert
    mocks.AssertActualAndExpectedCalls();

    //Cleanup

}


END_TEST_SUITE(gateway_ut)
