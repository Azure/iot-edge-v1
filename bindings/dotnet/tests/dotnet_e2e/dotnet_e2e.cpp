// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "azure_c_shared_utility/platform.h"

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"
#include "azure_c_shared_utility/map.h"
#include "azure_c_shared_utility/lock.h"
#include "gateway.h"
#include "messageproperties.h"
#include "azure_c_shared_utility/threadapi.h"
#include "experimental/event_system.h"
#include "dotnet.h"
#include "dynamic_loader.h"
#include "azure_c_shared_utility/vector.h"


DEFINE_MICROMOCK_ENUM_TO_STRING(BROKER_RESULT, BROKER_RESULT_VALUES);

static MICROMOCK_MUTEX_HANDLE g_testByTest;
static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

static bool messageRequestReceived = false;
static bool messageReplyReceived = false;

static MODULE_HANDLE E2EModule_CreateFromJson(BROKER_HANDLE broker, const char* configuration)
{
    return (MODULE_HANDLE)0x4242;
}

static MODULE_HANDLE E2EModule_Create(BROKER_HANDLE broker, const void* configuration)
{
    return (MODULE_HANDLE)0x4242;
}

static void E2EModule_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    if (messageHandle != NULL)
    {
        CONSTMAP_HANDLE properties = Message_GetProperties(messageHandle);

        const char * messageType = ConstMap_GetValue(properties, "MsgType");

        ASSERT_IS_NOT_NULL(messageType);

        if (strcmp(messageType, "Reply") == 0)
        {
            messageReplyReceived = true;
        }
        else if (strcmp(messageType, "Request") == 0)
        {
            messageRequestReceived = true;
        }

        ConstMap_Destroy(properties);
    }
}
static void E2EModule_Destroy(MODULE_HANDLE module)
{

}
static const MODULE_API_1 E2E_APIS_all =
{
    {MODULE_API_VERSION_1},

    E2EModule_CreateFromJson,
    E2EModule_Create,
    E2EModule_Destroy,
    E2EModule_Receive,
    NULL
};

MODULE_EXPORT const MODULE_API* Module_GetApi(const MODULE_API_VERSION gateway_api_version)
{
    return reinterpret_cast<const MODULE_API *>(&E2E_APIS_all);
}

MODULE_LIBRARY_HANDLE E2E_Loader_Load(const void * config)
{
    return (MODULE_LIBRARY_HANDLE)&E2E_APIS_all;
}
void E2E_Loader_Unload(MODULE_LIBRARY_HANDLE handle)
{
    return;
}
const MODULE_API* E2E_Loader_GetApi(MODULE_LIBRARY_HANDLE handle)
{
    const MODULE_API* result;
    if (handle != NULL)
    {
        result = (const MODULE_API*)handle;
    }
    else
    {
        result = NULL;
    }
    return result;
}
const MODULE_LOADER_API E2E_Loader_api =
{
    E2E_Loader_Load,
    E2E_Loader_Unload,
    E2E_Loader_GetApi
};

BEGIN_TEST_SUITE(dotnet_e2e)

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

TEST_FUNCTION(GW_dotnet_binding_e2e_Managed2Managed)
{
    ///arrange
    GATEWAY_MODULES_ENTRY modulesEntryArray[3];

    //Add Managed Module 1
    const DOTNET_HOST_CONFIG senderConfig{
        "E2ETestModule",
        "E2ETestModule.DotNetE2ETestModule",
        "Sender"
    };

    const DYNAMIC_LOADER_CONFIG loader_cfg =
    {
        "..\\..\\..\\Debug\\dotnet.dll"
    };

    const char * nameModuleSender = "Sender";

    modulesEntryArray[0] = {
        nameModuleSender,
        &loader_cfg,
        DynamicLoader_GetApi(),
        &senderConfig
    };

    //Add Managed Module 2
    const DOTNET_HOST_CONFIG receiverConfig{
        "E2ETestModule",
        "E2ETestModule.DotNetE2ETestModule",
        "Receiver"
    };

    const char * nameModuleReceiver = "Receiver";
    modulesEntryArray[1] = {
        nameModuleReceiver,
        &loader_cfg,
        DynamicLoader_GetApi(),
        &receiverConfig
    };

    //Add test probe module

    const char * nameProbeModule = "probe test";
    modulesEntryArray[2] = {
        nameProbeModule,
        NULL,
        &E2E_Loader_api,
        NULL
    };

    //Now set the links. From Sender -> Receiver and from both to Test probe
    GATEWAY_LINK_ENTRY linksArray[3];

    // sender to test probe
    linksArray[0] = {
        nameModuleSender,
        nameProbeModule
    };

    // sender to receiver
    linksArray[1] = {
        nameModuleSender,
        nameModuleReceiver
    };

    // receiver to test probe
    linksArray[2] = {
        nameModuleReceiver,
        nameProbeModule
    };

    ///act    
    GATEWAY_PROPERTIES properties;
    properties.gateway_modules = VECTOR_create(sizeof(GATEWAY_MODULES_ENTRY));
    properties.gateway_links = VECTOR_create(sizeof(GATEWAY_LINK_ENTRY));
    ASSERT_IS_NOT_NULL(properties.gateway_modules);
    ASSERT_IS_NOT_NULL(properties.gateway_links);
    VECTOR_push_back(properties.gateway_modules, modulesEntryArray, 3);
    VECTOR_push_back(properties.gateway_links, linksArray, 3);

    GATEWAY_HANDLE e2eGatewayInstance = Gateway_Create(&properties);

    Gateway_Start(e2eGatewayInstance);

    ///assert
    ASSERT_IS_NOT_NULL(e2eGatewayInstance);

    ThreadAPI_Sleep(5000); //Modules are configured to send a message every second, so waiting 5 seconds has to be enough.

    ASSERT_IS_TRUE(messageRequestReceived);
    ASSERT_IS_TRUE(messageReplyReceived);


    ///cleanup
    Gateway_Destroy(e2eGatewayInstance);

}
END_TEST_SUITE(dotnet_e2e)
