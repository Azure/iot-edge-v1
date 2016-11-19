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

#include "module_loader.h"
#include "experimental/event_system.h"

#include "gateway.h"
#include "../src/gateway_internal.h"
#include <parson.h>

#define DUMMY_JSON_PATH "x.json"
#define MISCONFIG_JSON_PATH "invalid_json.json"
#define MISSING_INFO_JSON_PATH "missing_info_json.json"
#define VALID_JSON_PATH "valid_json.json"
#define VALID_JSON_NULL_ARGS_PATH "valid_json_null.json"

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

static MODULE_API_1 dummyAPIs;
static size_t currentBroker_ref_count;
static MODULE_LOADER_API default_module_loader;
static MODULE_LOADER dummyModuleLoader;
static GATEWAY_MODULE_LOADER_INFO dummyLoaderInfo;

TYPED_MOCK_CLASS(CGatewayMocks, CGlobalMock)
{
public:

    /*Parson Mocks*/
    MOCK_STATIC_METHOD_1(, JSON_Value*, json_parse_file, const char *, filename)
        JSON_Value* value = NULL;
        if (filename != NULL)
        {
            value = (JSON_Value*)malloc(1);
        }
    MOCK_METHOD_END(JSON_Value*, value);

    MOCK_STATIC_METHOD_1(, JSON_Object*, json_value_get_object, const JSON_Value*, value)
        JSON_Object* object = NULL;
        if (value != NULL)
        {
            object = (JSON_Object*)0x42;
        }
    MOCK_METHOD_END(JSON_Object*, object);

    MOCK_STATIC_METHOD_2(, JSON_Array*, json_object_get_array, const JSON_Object*, object, const char*, name)
        JSON_Array* arr = NULL;
        if (object != NULL && name != NULL)
        {
            arr = (JSON_Array*)0x42;
        }
    MOCK_METHOD_END(JSON_Array*, arr);

    MOCK_STATIC_METHOD_1(, size_t, json_array_get_count, const JSON_Array*, arr)
        size_t size = 0;
    MOCK_METHOD_END(size_t, size);

    MOCK_STATIC_METHOD_2(, JSON_Object*, json_array_get_object, const JSON_Array*, arr, size_t, index)
        JSON_Object* object = NULL;
        if (arr != NULL && index >= 0)
        {
            object = (JSON_Object*)0x42;
        }
    MOCK_METHOD_END(JSON_Object*, object);

    MOCK_STATIC_METHOD_2(, const char*, json_object_get_string, const JSON_Object*, object, const char*, name)
        const char* string = NULL;
        if (object != NULL && name != NULL)
        {
            string = name;
        }
    MOCK_METHOD_END(const char*, string);

    MOCK_STATIC_METHOD_2(, JSON_Object*, json_object_get_object, const JSON_Object*, object, const char*, name)
        JSON_Object* object1 = NULL;
        if (object != NULL && name != NULL)
        {
            object1 = (JSON_Object*)0x42;
        }
        MOCK_METHOD_END(JSON_Object*, object1);

    MOCK_STATIC_METHOD_2(, JSON_Value*, json_object_get_value, const JSON_Object*, object, const char*, name)
        JSON_Value* value = NULL;
        if (object != NULL && name != NULL)
        {
            value = (JSON_Value*)0x42;
        }
    MOCK_METHOD_END(JSON_Value*, value);

    MOCK_STATIC_METHOD_1(, char*, json_serialize_to_string, const JSON_Value*, value)
        char* serialized_string = NULL;
        const char* text = "[serialized string]";
        if (value != NULL)
        {
            serialized_string = (char*)malloc(sizeof(char) * strlen(text) + 1);
            strcpy(serialized_string, text);
        }
    MOCK_METHOD_END(char*, serialized_string);

    MOCK_STATIC_METHOD_1(, void, json_value_free, JSON_Value*, value)
        BASEIMPLEMENTATION::gballoc_free(value);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_1(, void, json_free_serialized_string, char*, string)
        BASEIMPLEMENTATION::gballoc_free(string);
    MOCK_VOID_METHOD_END();

    /*Gateway Mocks*/
    MOCK_STATIC_METHOD_1(, GATEWAY_HANDLE, Gateway_Create, const GATEWAY_PROPERTIES*, properties)
        GATEWAY_HANDLE handle = (GATEWAY_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1);
    MOCK_METHOD_END(GATEWAY_HANDLE, handle);

    MOCK_STATIC_METHOD_1(, void, Gateway_Destroy, GATEWAY_HANDLE, gw)
        BASEIMPLEMENTATION::gballoc_free(gw);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_1(, GATEWAY_START_RESULT, Gateway_Start, GATEWAY_HANDLE, gw)
    MOCK_METHOD_END(GATEWAY_START_RESULT, GATEWAY_START_SUCCESS);

    /*Broker Mocks*/
    MOCK_STATIC_METHOD_0(, BROKER_HANDLE, Broker_Create)
        ++currentBroker_ref_count;
        BROKER_HANDLE result1 = (BROKER_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1);
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

    MOCK_STATIC_METHOD_2(, BROKER_RESULT, Broker_AddModule, BROKER_HANDLE, handle, const MODULE*, module)
    MOCK_METHOD_END(BROKER_RESULT, BROKER_OK);

    MOCK_STATIC_METHOD_2(, BROKER_RESULT, Broker_RemoveModule, BROKER_HANDLE, handle, const MODULE*, module)
    MOCK_METHOD_END(BROKER_RESULT, BROKER_OK);

    MOCK_STATIC_METHOD_2(, BROKER_RESULT, Broker_AddLink, BROKER_HANDLE, handle, const BROKER_LINK_DATA*, link)
    MOCK_METHOD_END(BROKER_RESULT, BROKER_OK)

    MOCK_STATIC_METHOD_2(, BROKER_RESULT, Broker_RemoveLink, BROKER_HANDLE, handle, const BROKER_LINK_DATA*, link)
    MOCK_METHOD_END(BROKER_RESULT, BROKER_OK)

    /*ModuleLoader Mocks*/
    MOCK_STATIC_METHOD_0(, const MODULE_LOADER_API*, DynamicLoader_GetApi)
    MOCK_METHOD_END(const MODULE_LOADER_API*, &default_module_loader);

    MOCK_STATIC_METHOD_2(, MODULE_LIBRARY_HANDLE, DynamicModuleLoader_Load, const struct MODULE_LOADER_TAG*, loader, const void*, entrypoint)
    MOCK_METHOD_END(MODULE_LIBRARY_HANDLE, (MODULE_LIBRARY_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1));

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

    MOCK_STATIC_METHOD_1(, MODULE_LOADER_RESULT, ModuleLoader_InitializeFromJson, const JSON_Value*, loaders);
    MOCK_METHOD_END(MODULE_LOADER_RESULT, MODULE_LOADER_SUCCESS);

    MOCK_STATIC_METHOD_0(, void, ModuleLoader_Destroy);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_1(, MODULE_LOADER*, ModuleLoader_FindByName, const char*, name)
    MOCK_METHOD_END(MODULE_LOADER*, &dummyModuleLoader);


    /*EventSystem Mocks*/
    MOCK_STATIC_METHOD_0(, EVENTSYSTEM_HANDLE, EventSystem_Init)
    MOCK_METHOD_END(EVENTSYSTEM_HANDLE, (EVENTSYSTEM_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1));

    MOCK_STATIC_METHOD_4(, void, EventSystem_AddEventCallback, EVENTSYSTEM_HANDLE, event_system, GATEWAY_EVENT, event_type, GATEWAY_CALLBACK, callback, void*, user_param)
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_3(, void, EventSystem_ReportEvent, EVENTSYSTEM_HANDLE, event_system, GATEWAY_HANDLE, gw, GATEWAY_EVENT, event_type)
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_1(, void, EventSystem_Destroy, EVENTSYSTEM_HANDLE, handle)
        BASEIMPLEMENTATION::gballoc_free(handle);
    MOCK_VOID_METHOD_END();

    /*Vector Mocks*/
    MOCK_STATIC_METHOD_1(, VECTOR_HANDLE, VECTOR_create, size_t, elementSize)
        VECTOR_HANDLE vector = BASEIMPLEMENTATION::VECTOR_create(elementSize);
    MOCK_METHOD_END(VECTOR_HANDLE, vector);

    MOCK_STATIC_METHOD_1(, void, VECTOR_destroy, VECTOR_HANDLE, handle)
        BASEIMPLEMENTATION::VECTOR_destroy(handle);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_3(, int, VECTOR_push_back, VECTOR_HANDLE, handle, const void*, elements, size_t, numElements)
    int result1 = BASEIMPLEMENTATION::VECTOR_push_back(handle, elements, numElements);
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
        void* element = BASEIMPLEMENTATION::VECTOR_find_if(handle, pred, value);
    MOCK_METHOD_END(void*, element);

    /*crt_abstractions Mocks*/
    MOCK_STATIC_METHOD_2(, int, mallocAndStrcpy_s, char**, destination, const char*, source)
        (*destination) = (char*)malloc(strlen(source) + 1);
        strcpy(*destination, source);
    MOCK_METHOD_END(int, 0);

    /*gballoc Mocks*/
    MOCK_STATIC_METHOD_1(, void*, gballoc_malloc, size_t, size)
        void* result2 = BASEIMPLEMENTATION::gballoc_malloc(size);
    MOCK_METHOD_END(void*, result2);

    MOCK_STATIC_METHOD_2(, void*, gballoc_realloc, void*, ptr, size_t, size)
    MOCK_METHOD_END(void*, BASEIMPLEMENTATION::gballoc_realloc(ptr, size));

    MOCK_STATIC_METHOD_1(, void, gballoc_free, void*, ptr)
        BASEIMPLEMENTATION::gballoc_free(ptr);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, MODULE_HANDLE, mock_Module_ParseConfigurationFromJson, const char*, configuration)
    MOCK_METHOD_END(MODULE_HANDLE, (MODULE_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1));

    MOCK_STATIC_METHOD_1(, void, mock_Module_FreeConfiguration, void*, configuration)
        BASEIMPLEMENTATION::gballoc_free(configuration);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_2(, MODULE_HANDLE, mock_Module_Create, BROKER_HANDLE, broker, const void*, configuration)
    MOCK_METHOD_END(MODULE_HANDLE, (MODULE_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1));

    MOCK_STATIC_METHOD_1(, void, mock_Module_Destroy, MODULE_HANDLE, moduleHandle)
        BASEIMPLEMENTATION::gballoc_free(moduleHandle);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_2(, void, mock_Module_Receive, MODULE_HANDLE, moduleHandle, MESSAGE_HANDLE, messageHandle)
    MOCK_VOID_METHOD_END();
};

DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , JSON_Value*, json_parse_file, const char *, filename);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , JSON_Object*, json_value_get_object, const JSON_Value*, value);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , JSON_Array*, json_object_get_array, const JSON_Object*, object, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , size_t, json_array_get_count, const JSON_Array*, arr);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , JSON_Object*, json_array_get_object, const JSON_Array*, arr, size_t, index);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , const char*, json_object_get_string, const JSON_Object*, object, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , JSON_Object*, json_object_get_object, const JSON_Object*, object, const char*, name);

DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , JSON_Value*, json_object_get_value, const JSON_Object*, object, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , char*, json_serialize_to_string, const JSON_Value*, value);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , void, json_value_free, JSON_Value*, value);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , void, json_free_serialized_string, char*, string);

DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , GATEWAY_HANDLE, Gateway_Create, const GATEWAY_PROPERTIES*, properties);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , void, Gateway_Destroy, GATEWAY_HANDLE, gw);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , GATEWAY_START_RESULT, Gateway_Start, GATEWAY_HANDLE, gw);

DECLARE_GLOBAL_MOCK_METHOD_0(CGatewayMocks, , BROKER_HANDLE, Broker_Create);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , void, Broker_Destroy, BROKER_HANDLE, broker);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , void, Broker_IncRef, BROKER_HANDLE, broker);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , void, Broker_DecRef, BROKER_HANDLE, broker);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , BROKER_RESULT, Broker_AddModule, BROKER_HANDLE, handle, const MODULE*, module);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , BROKER_RESULT, Broker_RemoveModule, BROKER_HANDLE, handle, const MODULE*, module);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , BROKER_RESULT, Broker_AddLink, BROKER_HANDLE, handle, const BROKER_LINK_DATA*, link);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , BROKER_RESULT, Broker_RemoveLink, BROKER_HANDLE, handle, const BROKER_LINK_DATA*, link);

DECLARE_GLOBAL_MOCK_METHOD_0(CGatewayMocks, , const MODULE_LOADER_API*, DynamicLoader_GetApi);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , MODULE_LIBRARY_HANDLE, DynamicModuleLoader_Load, const struct MODULE_LOADER_TAG*, loader, const void*, entrypoint);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , const MODULE_API*, DynamicModuleLoader_GetModuleApi, const struct MODULE_LOADER_TAG*, loader, MODULE_LIBRARY_HANDLE, module_library_handle);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , void, DynamicModuleLoader_Unload, const struct MODULE_LOADER_TAG*, loader, MODULE_LIBRARY_HANDLE, moduleLibraryHandle);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , void*, DynamicModuleLoader_ParseEntrypointFromJson, const struct MODULE_LOADER_TAG*, loader, const JSON_Value*, json);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , void, DynamicModuleLoader_FreeEntrypoint, const struct MODULE_LOADER_TAG*, loader, void*, entrypoint);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , MODULE_LOADER_BASE_CONFIGURATION*, DynamicModuleLoader_ParseConfigurationFromJson, const struct MODULE_LOADER_TAG*, loader, const JSON_Value*, json);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , void, DynamicModuleLoader_FreeConfiguration, const struct MODULE_LOADER_TAG*, loader, MODULE_LOADER_BASE_CONFIGURATION*, configuration);
DECLARE_GLOBAL_MOCK_METHOD_3(CGatewayMocks, , void*, DynamicModuleLoader_BuildModuleConfiguration, const struct MODULE_LOADER_TAG*, loader, const void*, entrypoint, const void*, module_configuration);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , void, DynamicModuleLoader_FreeModuleConfiguration, const struct MODULE_LOADER_TAG*, loader, const void*, module_configuration);
DECLARE_GLOBAL_MOCK_METHOD_0(CGatewayMocks, , MODULE_LOADER_RESULT, ModuleLoader_Initialize);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , MODULE_LOADER_RESULT, ModuleLoader_InitializeFromJson, const JSON_Value*, loaders);
DECLARE_GLOBAL_MOCK_METHOD_0(CGatewayMocks, , void, ModuleLoader_Destroy);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , MODULE_LOADER*, ModuleLoader_FindByName, const char*, name);


DECLARE_GLOBAL_MOCK_METHOD_0(CGatewayMocks, , EVENTSYSTEM_HANDLE, EventSystem_Init);
DECLARE_GLOBAL_MOCK_METHOD_4(CGatewayMocks, , void, EventSystem_AddEventCallback, EVENTSYSTEM_HANDLE, event_system, GATEWAY_EVENT, event_type, GATEWAY_CALLBACK, callback, void*, user_param);
DECLARE_GLOBAL_MOCK_METHOD_3(CGatewayMocks, , void, EventSystem_ReportEvent, EVENTSYSTEM_HANDLE, event_system, GATEWAY_HANDLE, gw, GATEWAY_EVENT, event_type);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , void, EventSystem_Destroy, EVENTSYSTEM_HANDLE, handle);

DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , VECTOR_HANDLE, VECTOR_create, size_t, elementSize);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , void, VECTOR_destroy, VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_3(CGatewayMocks, , int, VECTOR_push_back, VECTOR_HANDLE, handle, const void*, elements, size_t, numElements);
DECLARE_GLOBAL_MOCK_METHOD_3(CGatewayMocks, , void, VECTOR_erase, VECTOR_HANDLE, handle, void*, elements, size_t, index);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , void*, VECTOR_element, const VECTOR_HANDLE, handle, size_t, index);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , void*, VECTOR_front, const VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , void*, VECTOR_back, const VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , size_t, VECTOR_size, const VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_3(CGatewayMocks, , void*, VECTOR_find_if, const VECTOR_HANDLE, handle, PREDICATE_FUNCTION, pred, const void*, value);

DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , int, mallocAndStrcpy_s, char**, destination, const char*, source);

DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , void*, gballoc_realloc, void*, ptr, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , void, gballoc_free, void*, ptr)

DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , MODULE_HANDLE, mock_Module_ParseConfigurationFromJson, const char*, configuration);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , void, mock_Module_FreeConfiguration, void*, configuration);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , MODULE_HANDLE, mock_Module_Create, BROKER_HANDLE, broker, const void*, configuration);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , void, mock_Module_Destroy, MODULE_HANDLE, moduleHandle);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , void, mock_Module_Receive, MODULE_HANDLE, moduleHandle, MESSAGE_HANDLE, messageHandle);

static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;
static MICROMOCK_MUTEX_HANDLE g_testByTest;


BEGIN_TEST_SUITE(gateway_createfromjson_ut)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    g_testByTest = MicroMockCreateMutex();
    ASSERT_IS_NOT_NULL(g_testByTest);
    currentBroker_ref_count = 0;

    dummyAPIs =
    {
        { MODULE_API_VERSION_1 },

        mock_Module_ParseConfigurationFromJson,
        mock_Module_FreeConfiguration,
        mock_Module_Create,
        mock_Module_Destroy,
        mock_Module_Receive,
        NULL
    };

    default_module_loader =
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
    dummyModuleLoader =
    {
        NATIVE,
        "dummy loader",
        NULL,
        &default_module_loader
    };

    dummyLoaderInfo =
    {
        &dummyModuleLoader,
        (void*)0x42
    };
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

/*Tests_SRS_GATEWAY_JSON_14_001: [If file_path is NULL the function shall return NULL.]*/
TEST_FUNCTION(Gateway_CreateFromJson_Returns_NULL_For_NULL_JSON_Input)
{
    //Act
    GATEWAY_HANDLE gateway = Gateway_CreateFromJson(NULL);

    //Assert
    ASSERT_IS_NULL(gateway);
}

/*Tests_SRS_GATEWAY_JSON_17_012: [ This function shall return NULL if the module list is not initialized. ]*/
TEST_FUNCTION(Gateway_CreateFromJson_Returns_NULL_ModuleLoader_fails)
{
    //Arrange
    CGatewayMocks mocks;

    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Initialize())
        .SetFailReturn(MODULE_LOADER_ERROR);

    //Act
    GATEWAY_HANDLE gateway = Gateway_CreateFromJson(DUMMY_JSON_PATH);

    //Assert
    ASSERT_IS_NULL(gateway);
    mocks.AssertActualAndExpectedCalls();
}

/*Tests_SRS_GATEWAY_JSON_14_003: [The function shall return NULL if the file contents could not be read and / or parsed to a JSON_Value.]*/
/*Tests_SRS_GATEWAY_JSON_17_006: [ Upon failure this function shall destroy the module loader list. ]*/
TEST_FUNCTION(Gateway_CreateFromJson_Returns_NULL_If_File_Not_Exist)
{
    //Arrange
    CGatewayMocks mocks;

    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Initialize());
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());
    STRICT_EXPECTED_CALL(mocks, json_parse_file(DUMMY_JSON_PATH))
        .SetFailReturn((JSON_Value*)NULL);

    //Act
    GATEWAY_HANDLE gateway = Gateway_CreateFromJson(DUMMY_JSON_PATH);

    //Assert
    ASSERT_IS_NULL(gateway);
    mocks.AssertActualAndExpectedCalls();
}

/*Tests_SRS_GATEWAY_JSON_17_007: [ The function shall parse the "loaders" JSON array and initialize new module loaders or update the existing default loaders. ]*/
TEST_FUNCTION(Gateway_CreateFromJson_Returns_NULL_If_ML_init_fromjson_fails)
{
    //Arrange
    CGatewayMocks mocks;

    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Initialize());

    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());
    STRICT_EXPECTED_CALL(mocks, json_parse_file(DUMMY_JSON_PATH));
    STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(GATEWAY_PROPERTIES)));
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_value(IGNORED_PTR_ARG, "loaders"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_InitializeFromJson(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(MODULE_LOADER_ERROR);

    //Act
    GATEWAY_HANDLE gateway = Gateway_CreateFromJson(DUMMY_JSON_PATH);

    //Assert
    ASSERT_IS_NULL(gateway);
    mocks.AssertActualAndExpectedCalls();
}

/*Tests_SRS_GATEWAY_JSON_14_008: [ This function shall return NULL upon any memory allocation failure. ]*/
TEST_FUNCTION(Gateway_CreateFromJson_Returns_NULL_On_Properties_Malloc_Failure)
{
    //Arrange
    CGatewayMocks mocks;

    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Initialize());
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());
    STRICT_EXPECTED_CALL(mocks, json_parse_file(DUMMY_JSON_PATH));
    STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(GATEWAY_PROPERTIES)))
        .SetFailReturn((void*)NULL);


    //Act
    GATEWAY_HANDLE gateway = Gateway_CreateFromJson(DUMMY_JSON_PATH);

    //Assert
    ASSERT_IS_NULL(gateway);
    mocks.AssertActualAndExpectedCalls();

}

static void setup_2module_gw(CGatewayMocks& mocks, char * path)
{
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Initialize());

    STRICT_EXPECTED_CALL(mocks, json_parse_file(path));
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(GATEWAY_PROPERTIES)));
    STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_value(IGNORED_PTR_ARG, "loaders"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_InitializeFromJson(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "modules"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "links"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(GATEWAY_MODULES_ENTRY)));
}

static void setup_parse_modules_entry(CGatewayMocks& mocks, size_t index, const char * modulename, const char* loadername = "loader1")
{
    STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, index))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_object(IGNORED_PTR_ARG, "loader"))
        .IgnoreArgument(1)
        .SetReturn((JSON_Object*)0x42);
    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "name"))
        .IgnoreArgument(1)
        .SetReturn(loadername);
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_FindByName(loadername == NULL ? "native" : "loader1"));
    STRICT_EXPECTED_CALL(mocks, json_object_get_value(IGNORED_PTR_ARG, "entrypoint"))
        .IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_ParseEntrypointFromJson(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "name"))
        .IgnoreArgument(1)
        .SetReturn(modulename);
    STRICT_EXPECTED_CALL(mocks, json_object_get_value(IGNORED_PTR_ARG, "args"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_serialize_to_string(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
}

static void setup_links_entry(CGatewayMocks& mocks, size_t index, const char * source, const char * sink)
{
    STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, index))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "source"))
        .IgnoreArgument(1)
        .SetReturn(source);
    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "sink"))
        .IgnoreArgument(1)
        .SetReturn(sink);
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
}

static void add_a_module(CGatewayMocks& mocks, size_t index)
{
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, index))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(MODULE_DATA)));
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Load(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, mock_Module_ParseConfigurationFromJson("[serialized string]"));
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_BuildModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, mock_Module_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, mock_Module_FreeConfiguration(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_AddModule(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, Broker_IncRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_back(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
}

static void add_a_link(CGatewayMocks& mocks, size_t index)
{
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, index))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(mocks, Broker_AddLink(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
}

/*Tests_SRS_GATEWAY_JSON_14_008: [ This function shall return NULL upon any memory allocation failure. */
TEST_FUNCTION(Gateway_CreateFromJson_Returns_NULL_on_gateway_create_internal_fail)
{
    //Arrange
    CGatewayMocks mocks;

    setup_2module_gw(mocks, (char *)VALID_JSON_PATH);

    // modules array
    setup_parse_modules_entry(mocks, 0, "module1");
    setup_parse_modules_entry(mocks, 1, "module2");

    // links entry
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(GATEWAY_LINK_ENTRY)));
    STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(2);

    setup_links_entry(mocks, 0, "module1", "module2");
    setup_links_entry(mocks, 1, "module2", "module1");

    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(GATEWAY_HANDLE_DATA)))
        .SetFailReturn(nullptr);

    STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_free_serialized_string((char *)"[serialized string]"));
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_free_serialized_string((char *)"[serialized string]"));
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());


    //Act
    GATEWAY_HANDLE gateway = Gateway_CreateFromJson(VALID_JSON_PATH);

    //Assert
    ASSERT_IS_NULL(gateway);
    mocks.AssertActualAndExpectedCalls();

}

/*Tests_SRS_GATEWAY_JSON_14_002: [The function shall use parson to read the file and parse the JSON string to a parson JSON_Value structure.]*/
/*Tests_SRS_GATEWAY_JSON_17_005: [ The function shall parse the "loading args" for "module path" and fill a DYNAMIC_LOADER_CONFIG structure with the module path information. ]*/
/*Tests_SRS_GATEWAY_JSON_14_004: [The function shall traverse the JSON_Value object to initialize a GATEWAY_PROPERTIES instance.]*/
/*Tests_SRS_GATEWAY_JSON_14_005: [The function shall set the value of const void* module_properties in the GATEWAY_PROPERTIES instance to a char* representing the serialized args value for the particular module.]*/
/*Tests_SRS_GATEWAY_JSON_14_007: [The function shall use the GATEWAY_PROPERTIES instance to create and return a GATEWAY_HANDLE using the lower level API.]*/
/*Tests_SRS_GATEWAY_JSON_04_001: [The function shall create a Vector to Store all links to this gateway.] */
/*Tests_SRS_GATEWAY_JSON_04_002: [The function shall add all modules source and sink to GATEWAY_PROPERTIES inside gateway_links.] */
/*Tests_SRS_GATEWAY_JSON_17_004: [ The function shall set the module loader to the default dynamically linked library module loader. ]*/
/*Tests_SRS_GATEWAY_JSON_17_001: [Upon successful creation, this function shall start the gateway.]*/
/*Tests_SRS_GATEWAY_JSON_17_008: [ The function shall parse the "modules" JSON array for each module entry. ]*/
/*Tests_SRS_GATEWAY_JSON_17_009: [ For each module, the function shall call the loader's ParseEntrypointFromJson function to parse the entrypoint JSON. ]*/
/*Tests_SRS_GATEWAY_JSON_17_011: [ The function shall the loader's BuildModuleConfiguration to construct module input from module's "args" and "loader.entrypoint". ]*/
/*Tests_SRS_GATEWAY_JSON_17_013: [ The function shall parse each modules object for "loader.name" and "loader.entrypoint". ]*/
/*Tests_SRS_GATEWAY_JSON_17_014: [ The function shall find the correct loader by "loader.name". ]*/
TEST_FUNCTION(Gateway_CreateFromJson_Parses_Valid_JSON_Configuration_File)
{
    //Arrange
    CGatewayMocks mocks;

    setup_2module_gw(mocks, (char *)VALID_JSON_PATH);

    // modules array
    setup_parse_modules_entry(mocks, 0, "module1");
    setup_parse_modules_entry(mocks, 1, "module2");

    // links entry
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(GATEWAY_LINK_ENTRY)));
    STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(2);

    setup_links_entry(mocks, 0, "module1", "module2");
    setup_links_entry(mocks, 1, "module2", "module1");


    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(GATEWAY_HANDLE_DATA)));
    STRICT_EXPECTED_CALL(mocks, Broker_Create());
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(MODULE_DATA*)));
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(LINK_DATA)));
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //Adding module 1 (Success)
    add_a_module(mocks, 0);
    //Adding module 2 (Success)
    add_a_module(mocks, 1);

    //process the links
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    add_a_link(mocks, 0);
    add_a_link(mocks, 1);


    //Gateway start
       STRICT_EXPECTED_CALL(mocks, EventSystem_Init());
       STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, IGNORED_PTR_ARG, GATEWAY_CREATED))
           .IgnoreArgument(1)
           .IgnoreArgument(2);
       STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, IGNORED_PTR_ARG, GATEWAY_MODULE_LIST_CHANGED))
           .IgnoreArgument(1)
           .IgnoreArgument(2);
       STRICT_EXPECTED_CALL(mocks, Gateway_Start(IGNORED_PTR_ARG))
           .IgnoreArgument(1);
       STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
           .IgnoreArgument(1);
       STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
           .IgnoreArgument(1);
	   STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		   .IgnoreArgument(1)
           .IgnoreArgument(2);
       STRICT_EXPECTED_CALL(mocks, json_free_serialized_string((char*)"[serialized string]"));
       STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1))
           .IgnoreArgument(1);
	   STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		   .IgnoreArgument(1)
           .IgnoreArgument(2);
       STRICT_EXPECTED_CALL(mocks, json_free_serialized_string((char*)"[serialized string]"));
       STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
           .IgnoreArgument(1);
       STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
           .IgnoreArgument(1);
       STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
           .IgnoreArgument(1);
       STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
          .IgnoreArgument(1);

    //Act
    GATEWAY_HANDLE gateway = Gateway_CreateFromJson(VALID_JSON_PATH);

    //Assert
    ASSERT_IS_NOT_NULL(gateway);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    gateway_destroy_internal(gateway);
}

//Tests_SRS_GATEWAY_JSON_17_002: [ This function shall return NULL if starting the gateway fails. ]
TEST_FUNCTION(Gateway_Create_Start_fails_returns_null)
{
    //Arrange
    CGatewayMocks mocks;

    setup_2module_gw(mocks, (char *)VALID_JSON_PATH);

    // modules array
    setup_parse_modules_entry(mocks, 0, "module1");
    setup_parse_modules_entry(mocks, 1, "module2");

    // links entry
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(GATEWAY_LINK_ENTRY)));
    STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(2);

    setup_links_entry(mocks, 0, "module1", "module2");
    setup_links_entry(mocks, 1, "module2", "module1");


    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(GATEWAY_HANDLE_DATA)));
    STRICT_EXPECTED_CALL(mocks, Broker_Create());
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(MODULE_DATA*)));
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(LINK_DATA)));
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //Adding module 1 (Success)
    add_a_module(mocks, 0);
    //Adding module 2 (Success)
    add_a_module(mocks, 1);

    //process the links
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    add_a_link(mocks, 0);
    add_a_link(mocks, 1);

    STRICT_EXPECTED_CALL(mocks, EventSystem_Init());
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, IGNORED_PTR_ARG, GATEWAY_CREATED))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, IGNORED_PTR_ARG, GATEWAY_MODULE_LIST_CHANGED))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Gateway_Start(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn((GATEWAY_START_RESULT)GATEWAY_START_INVALID_ARGS);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG,0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_free_serialized_string((char *)"[serialized string]"));
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG,1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_free_serialized_string((char *)"[serialized string]"));
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //Cleaning up calls
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, IGNORED_PTR_ARG, GATEWAY_DESTROYED))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(mocks, EventSystem_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_front(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveLink(IGNORED_PTR_ARG,IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveLink(IGNORED_PTR_ARG,IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG,IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG,IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG,IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_erase(IGNORED_PTR_ARG,IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(mocks, VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveModule(IGNORED_PTR_ARG,IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_RemoveModule(IGNORED_PTR_ARG,IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Broker_DecRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Broker_DecRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Broker_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, mock_Module_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_GetModuleApi(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Unload(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_Unload(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());

    //Act
    GATEWAY_HANDLE gateway = Gateway_CreateFromJson(VALID_JSON_PATH);

    //Assert
    ASSERT_IS_NULL(gateway);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
}

/*Tests_SRS_GATEWAY_JSON_14_002: [The function shall use parson to read the file and parse the JSON string to a parson JSON_Value structure.]*/
/*Tests_SRS_GATEWAY_JSON_14_004: [The function shall traverse the JSON_Value object to initialize a GATEWAY_PROPERTIES instance.]*/
/*Tests_SRS_GATEWAY_JSON_14_005: [The function shall set the value of const void* module_properties in the GATEWAY_PROPERTIES instance to a char* representing the serialized args value for the particular module.]*/
/*Tests_SRS_GATEWAY_JSON_14_006: [The function shall return NULL if the JSON_Value contains incomplete information.]*/
TEST_FUNCTION(Gateway_CreateFromJson_Traverses_JSON_Push_Back_Fail)
{
    //Arrange
    CGatewayMocks mocks;

    setup_2module_gw(mocks, (char *)VALID_JSON_PATH);

    // modules array
    setup_parse_modules_entry(mocks, 0, "module1");

    STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_object(IGNORED_PTR_ARG, "loader"))
        .IgnoreArgument(1)
        .SetReturn((JSON_Object*)0x42);
    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "name"))
        .IgnoreArgument(1)
        .SetReturn("loader1");
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_FindByName("loader1"));
    STRICT_EXPECTED_CALL(mocks, json_object_get_value(IGNORED_PTR_ARG, "entrypoint"))
        .IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_ParseEntrypointFromJson(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "name"))
        .IgnoreArgument(1)
        .SetReturn("Module2");
    STRICT_EXPECTED_CALL(mocks, json_object_get_value(IGNORED_PTR_ARG, "args"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_serialize_to_string(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1);

    STRICT_EXPECTED_CALL(mocks, json_free_serialized_string(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_free_serialized_string(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());

    //Act
    GATEWAY_HANDLE gateway = Gateway_CreateFromJson(VALID_JSON_PATH);

    //Assert
    ASSERT_IS_NULL(gateway);
    mocks.AssertActualAndExpectedCalls();

}

/*Tests_SRS_GATEWAY_JSON_14_006: [The function shall return NULL if the JSON_Value contains incomplete information.]*/
TEST_FUNCTION(Gateway_CreateFromJson_Traverses_JSON_Value_NULL_Modules_Array)
{
    //Arrange
    CGatewayMocks mocks;

    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Initialize());

    STRICT_EXPECTED_CALL(mocks, json_parse_file(VALID_JSON_PATH));
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(GATEWAY_PROPERTIES)));
    STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_value(IGNORED_PTR_ARG, "loaders"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_InitializeFromJson(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "modules"))
        .IgnoreArgument(1)
        .SetFailReturn((JSON_Array*)NULL);
    STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "links"))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());

    //Act
    GATEWAY_HANDLE gateway = Gateway_CreateFromJson(VALID_JSON_PATH);

    //Assert
    ASSERT_IS_NULL(gateway);
    mocks.AssertActualAndExpectedCalls();
}

/*Tests_SRS_GATEWAY_JSON_14_006: [The function shall return NULL if the JSON_Value contains incomplete information.]*/
TEST_FUNCTION(Gateway_CreateFromJson_Traverses_JSON_Value_NULL_Loaders_Array_success)
{
    //Arrange
    CGatewayMocks mocks;

    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Initialize());

    STRICT_EXPECTED_CALL(mocks, json_parse_file(VALID_JSON_PATH));
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(GATEWAY_PROPERTIES)));
    STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_value(IGNORED_PTR_ARG, "loaders"))
        .IgnoreArgument(1)
        .SetFailReturn(nullptr);
    STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "modules"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "links"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(2);
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(GATEWAY_MODULES_ENTRY)));

    // modules array
    setup_parse_modules_entry(mocks, 0, "module1");
    setup_parse_modules_entry(mocks, 1, "module2");

    // links entry
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(GATEWAY_LINK_ENTRY)));
    STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(2);

    setup_links_entry(mocks, 0, "module1", "module2");
    setup_links_entry(mocks, 1, "module2", "module1");

    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(GATEWAY_HANDLE_DATA)));
    STRICT_EXPECTED_CALL(mocks, Broker_Create());
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(MODULE_DATA*)));
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(LINK_DATA)));
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //Adding module 1 (Success)
    add_a_module(mocks, 0);
    //Adding module 2 (Success)
    add_a_module(mocks, 1);

    //process the links
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    add_a_link(mocks, 0);
    add_a_link(mocks, 1);


    //Gateway start
    STRICT_EXPECTED_CALL(mocks, EventSystem_Init());
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, IGNORED_PTR_ARG, GATEWAY_CREATED))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, IGNORED_PTR_ARG, GATEWAY_MODULE_LIST_CHANGED))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Gateway_Start(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, json_free_serialized_string((char *)"[serialized string]"));
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, json_free_serialized_string((char *)"[serialized string]"));
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);



    //Act
    GATEWAY_HANDLE gateway = Gateway_CreateFromJson(VALID_JSON_PATH);

    //Assert
    ASSERT_IS_NOT_NULL(gateway);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    gateway_destroy_internal(gateway);
}

/*Tests_SRS_GATEWAY_JSON_14_006: [The function shall return NULL if the JSON_Value contains incomplete information.]*/
TEST_FUNCTION(Gateway_CreateFromJson_Traverses_JSON_Value_ML_fail)
{
    //Arrange
    CGatewayMocks mocks;

    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Initialize());

    STRICT_EXPECTED_CALL(mocks, json_parse_file(VALID_JSON_PATH));
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(GATEWAY_PROPERTIES)));
    STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_value(IGNORED_PTR_ARG, "loaders"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_InitializeFromJson(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(MODULE_LOADER_ERROR);

    STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());

    //Act
    GATEWAY_HANDLE gateway = Gateway_CreateFromJson(VALID_JSON_PATH);

    //Assert
    ASSERT_IS_NULL(gateway);
    mocks.AssertActualAndExpectedCalls();
}
/*Tests_SRS_GATEWAY_JSON_14_008: [This function shall return NULL upon any memory allocation failure.]*/
TEST_FUNCTION(Gateway_CreateFromJson_Traverses_JSON_Value_VECTOR_Create_Fail)
{
    //Arrange
    CGatewayMocks mocks;

    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Initialize());

    STRICT_EXPECTED_CALL(mocks, json_parse_file(VALID_JSON_PATH));
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(GATEWAY_PROPERTIES)));
    STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_value(IGNORED_PTR_ARG, "loaders"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_InitializeFromJson(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "modules"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "links"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(GATEWAY_MODULES_ENTRY)))
        .SetFailReturn((VECTOR_HANDLE)NULL);

    STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());

    //Act
    GATEWAY_HANDLE gateway = Gateway_CreateFromJson(VALID_JSON_PATH);

    //Assert
    ASSERT_IS_NULL(gateway);
    mocks.AssertActualAndExpectedCalls();
}

/*Tests_SRS_GATEWAY_JSON_14_008: [This function shall return NULL upon any memory allocation failure.]*/
TEST_FUNCTION(Gateway_CreateFromJson_Traverses_JSON_Value_VECTOR_Create_for_links_Fail)
{
    //Arrange
    CGatewayMocks mocks;

    setup_2module_gw(mocks, (char*)VALID_JSON_PATH);

    setup_parse_modules_entry(mocks, 0, "module0");
    setup_parse_modules_entry(mocks, 1, "module0");

    STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(GATEWAY_LINK_ENTRY)))
        .SetFailReturn((VECTOR_HANDLE)NULL);

    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_free_serialized_string(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_free_serialized_string(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);


    STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());

    //Act
    GATEWAY_HANDLE gateway = Gateway_CreateFromJson(VALID_JSON_PATH);

    //Assert
    ASSERT_IS_NULL(gateway);
    mocks.AssertActualAndExpectedCalls();
}


/*Tests_SRS_GATEWAY_JSON_14_004: [The function shall traverse the JSON_Value object to initialize a GATEWAY_PROPERTIES instance.]*/
/*Tests_SRS_GATEWAY_JSON_14_006: [The function shall return NULL if the JSON_Value contains incomplete information.]*/
TEST_FUNCTION(Gateway_CreateFromJson_Traverses_JSON_Value_NULL_Root_Value_Failure)
{
    //Arrange
    CGatewayMocks mocks;

    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Initialize());

    STRICT_EXPECTED_CALL(mocks, json_parse_file(DUMMY_JSON_PATH));
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(GATEWAY_PROPERTIES)));
    STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn((JSON_Object*)NULL);

    STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());

    //Act
    GATEWAY_HANDLE gateway = Gateway_CreateFromJson(DUMMY_JSON_PATH);

    //Assert
    ASSERT_IS_NULL(gateway);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    Gateway_Destroy(gateway);
}

/*Tests_SRS_GATEWAY_JSON_14_006: [The function shall return NULL if the JSON_Value contains incomplete information.]*/
TEST_FUNCTION(Gateway_CreateFromJson_Fails_For_Missing_Info_In_JSON_Configuration)
{
    //Arrange
    CGatewayMocks mocks;

    setup_2module_gw(mocks, (char*)MISSING_INFO_JSON_PATH);

    STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_object(IGNORED_PTR_ARG, "loader"))
        .IgnoreArgument(1)
        .SetReturn((JSON_Object*)0x42);
    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "name"))
        .IgnoreArgument(1)
        .SetReturn("loader1");
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_FindByName("loader1"));
    STRICT_EXPECTED_CALL(mocks, json_object_get_value(IGNORED_PTR_ARG, "entrypoint"))
        .IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_ParseEntrypointFromJson(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "name"))
        .IgnoreArgument(1)
        .SetReturn((char*)NULL);

    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());

    //Act
    GATEWAY_HANDLE gateway = Gateway_CreateFromJson(MISSING_INFO_JSON_PATH);

    //Assert
    ASSERT_IS_NULL(gateway);
    mocks.AssertActualAndExpectedCalls();
}

//Tests_SRS_GATEWAY_JSON_13_001: [ If loader.name is not found in the JSON then the gateway assumes that the loader name is native. ]
TEST_FUNCTION(Gateway_CreateFromJson_uses_native_loader_when_loader_name_is_missing)
{
    //Arrange
    CGatewayMocks mocks;

    setup_2module_gw(mocks, (char*)VALID_JSON_PATH);

    // modules array
    setup_parse_modules_entry(mocks, 0, "module1", NULL);
    setup_parse_modules_entry(mocks, 1, "module2", NULL);

    // links entry
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(GATEWAY_LINK_ENTRY)));
    STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(2);

    setup_links_entry(mocks, 0, "module1", "module2");
    setup_links_entry(mocks, 1, "module2", "module1");

    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(GATEWAY_HANDLE_DATA)));
    STRICT_EXPECTED_CALL(mocks, Broker_Create());
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(MODULE_DATA*)));
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(LINK_DATA)));
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //Adding module 1 (Success)
    add_a_module(mocks, 0);
    //Adding module 2 (Success)
    add_a_module(mocks, 1);

    //process the links
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    add_a_link(mocks, 0);
    add_a_link(mocks, 1);

    //Gateway start
    STRICT_EXPECTED_CALL(mocks, EventSystem_Init());
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, IGNORED_PTR_ARG, GATEWAY_CREATED))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, EventSystem_ReportEvent(IGNORED_PTR_ARG, IGNORED_PTR_ARG, GATEWAY_MODULE_LIST_CHANGED))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Gateway_Start(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, json_free_serialized_string((char *)"[serialized string]"));
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, json_free_serialized_string((char *)"[serialized string]"));
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //Act
    GATEWAY_HANDLE gateway = Gateway_CreateFromJson(VALID_JSON_PATH);

    //Assert
    ASSERT_IS_NOT_NULL(gateway);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    gateway_destroy_internal(gateway);
}

/*Tests_SRS_GATEWAY_JSON_17_010: [ If the module's loader is not found by name, the the function shall fail and return NULL. ]*/
TEST_FUNCTION(Gateway_CreateFromJson_Fails_For_not_finding_loader)
{
    //Arrange
    CGatewayMocks mocks;

    setup_2module_gw(mocks, (char*)MISSING_INFO_JSON_PATH);

    STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_object(IGNORED_PTR_ARG, "loader"))
        .IgnoreArgument(1)
        .SetReturn((JSON_Object*)0x42);
    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "name"))
        .IgnoreArgument(1)
        .SetReturn("loader1");
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_FindByName("loader1"))
        .SetFailReturn( (MODULE_LOADER*)NULL);


    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());

    //Act
    GATEWAY_HANDLE gateway = Gateway_CreateFromJson(MISSING_INFO_JSON_PATH);

    //Assert
    ASSERT_IS_NULL(gateway);
    mocks.AssertActualAndExpectedCalls();
}

/*Tests_SRS_GATEWAY_JSON_14_006: [The function shall return NULL if the JSON_Value contains incomplete information.]*/
TEST_FUNCTION(Gateway_CreateFromJson_fails_with_no_entry_point)
{
    //Arrange
    CGatewayMocks mocks;

    setup_2module_gw(mocks, (char*)VALID_JSON_PATH);

    // modules array
    STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_object(IGNORED_PTR_ARG, "loader"))
        .IgnoreArgument(1)
        .SetReturn((JSON_Object*)0x42);
    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "name"))
        .IgnoreArgument(1)
        .SetReturn("loader1");
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_FindByName("loader1"));
    STRICT_EXPECTED_CALL(mocks, json_object_get_value(IGNORED_PTR_ARG, "entrypoint"))
        .IgnoreArgument(1)
        .SetFailReturn(nullptr);
    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "name"))
        .IgnoreArgument(1)
        .SetReturn("module1");
    STRICT_EXPECTED_CALL(mocks, json_object_get_value(IGNORED_PTR_ARG, "args"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_serialize_to_string(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    setup_parse_modules_entry(mocks, 1, "module2");

    // links entry
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(GATEWAY_LINK_ENTRY)));
    STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(2);

    setup_links_entry(mocks, 0, "module1", "module2");
    setup_links_entry(mocks, 1, "module2", "module1");

    // Create gateway until 1st module fails immediately
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(GATEWAY_HANDLE_DATA)));
    STRICT_EXPECTED_CALL(mocks, Broker_Create());
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(MODULE_DATA*)));
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(LINK_DATA)));
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //tear down.

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
    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, json_free_serialized_string((char *)"[serialized string]"));
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, json_free_serialized_string((char *)"[serialized string]"));
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());


    //Act
    GATEWAY_HANDLE gateway = Gateway_CreateFromJson(VALID_JSON_PATH);

    //Assert
    ASSERT_IS_NULL(gateway);
    mocks.AssertActualAndExpectedCalls();

    //Cleanup
    gateway_destroy_internal(gateway);
}

/*Tests_SRS_GATEWAY_JSON_14_006: [The function shall return NULL if the JSON_Value contains incomplete information.]*/
TEST_FUNCTION(Gateway_CreateFromJson_Fails_For_not_parsing_loader_entrypoint)
{
    //Arrange
    CGatewayMocks mocks;

    setup_2module_gw(mocks, (char*)MISSING_INFO_JSON_PATH);

    STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_object(IGNORED_PTR_ARG, "loader"))
        .IgnoreArgument(1)
        .SetReturn((JSON_Object*)0x42);
    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "name"))
        .IgnoreArgument(1)
        .SetReturn("loader1");
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_FindByName("loader1"));
    STRICT_EXPECTED_CALL(mocks, json_object_get_value(IGNORED_PTR_ARG, "entrypoint"))
        .IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_ParseEntrypointFromJson(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(nullptr);



    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());

    //Act
    GATEWAY_HANDLE gateway = Gateway_CreateFromJson(MISSING_INFO_JSON_PATH);

    //Assert
    ASSERT_IS_NULL(gateway);
    mocks.AssertActualAndExpectedCalls();
}

/*Tests_SRS_GATEWAY_JSON_14_006: [ The function shall return NULL if the JSON_Value contains incomplete information. ]*/
TEST_FUNCTION(Gateway_CreateFromJson_Fails_links_parsing_no_source)
{
    //Arrange
    CGatewayMocks mocks;

    setup_2module_gw(mocks, (char*)VALID_JSON_PATH);

    // modules array
    setup_parse_modules_entry(mocks, 0, "module1");
    setup_parse_modules_entry(mocks, 1, "module2");

    // links entry
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(GATEWAY_LINK_ENTRY)));
    STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(2);

    setup_links_entry(mocks, 0, "module1", "module2");
    STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "source"))
        .IgnoreArgument(1)
        .SetReturn(nullptr);
    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "sink"))
        .IgnoreArgument(1)
        .SetReturn("module1");

    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, json_free_serialized_string((char *)"[serialized string]"));
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, json_free_serialized_string((char *)"[serialized string]"));
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());

    //Act
    GATEWAY_HANDLE gateway = Gateway_CreateFromJson(VALID_JSON_PATH);

    //Assert
    ASSERT_IS_NULL(gateway);
    mocks.AssertActualAndExpectedCalls();
}

/*Tests_SRS_GATEWAY_JSON_14_008: [ This function shall return NULL upon any memory allocation failure. ]*/
TEST_FUNCTION(Gateway_CreateFromJson_Fails_links_parsing_pushback_fails)
{
    //Arrange
    CGatewayMocks mocks;

    setup_2module_gw(mocks, (char*)VALID_JSON_PATH);

    // modules array
    setup_parse_modules_entry(mocks, 0, "module1");
    setup_parse_modules_entry(mocks, 1, "module2");

    // links entry
    STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(GATEWAY_LINK_ENTRY)));
    STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(2);

    setup_links_entry(mocks, 0, "module1", "module2");
    STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "source"))
        .IgnoreArgument(1)
        .SetReturn("module2");
    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "sink"))
        .IgnoreArgument(1)
        .SetReturn("module1");
    STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(1);

    STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, json_free_serialized_string((char *)"[serialized string]"));
    STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, DynamicModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, json_free_serialized_string((char *)"[serialized string]"));
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, ModuleLoader_Destroy());

    //Act
    GATEWAY_HANDLE gateway = Gateway_CreateFromJson(VALID_JSON_PATH);

    //Assert
    ASSERT_IS_NULL(gateway);
    mocks.AssertActualAndExpectedCalls();
}

END_TEST_SUITE(gateway_createfromjson_ut)
