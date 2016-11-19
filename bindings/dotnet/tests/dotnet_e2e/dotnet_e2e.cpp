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
#include "module_loaders/dotnet_loader.h"
#include "azure_c_shared_utility/vector.h"


DEFINE_MICROMOCK_ENUM_TO_STRING(BROKER_RESULT, BROKER_RESULT_VALUES);

static MICROMOCK_MUTEX_HANDLE g_testByTest;
static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

static bool messageRequestReceived = false;
static bool messageReplyReceived = false;


static void* E2EModule_ParseConfigurationFromJson(const char* configuration)
{
	return (void*)0x4242;
}

static void E2EModule_FreeConfiguration(void* configuration)
{
	return;
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

	E2EModule_ParseConfigurationFromJson,
	E2EModule_FreeConfiguration,
    E2EModule_Create,
    E2EModule_Destroy,
    E2EModule_Receive,
    NULL
};

MODULE_EXPORT const MODULE_API* Module_GetApi(const struct MODULE_LOADER_TAG* loader, MODULE_API_VERSION gateway_api_version)
{
    return reinterpret_cast<const MODULE_API *>(&E2E_APIS_all);
}

MODULE_LIBRARY_HANDLE E2E_Loader_Load(const struct MODULE_LOADER_TAG* loader, const void * config)
{
    return (MODULE_LIBRARY_HANDLE)&E2E_APIS_all;
}

void E2E_Loader_Unload(const struct MODULE_LOADER_TAG* loader, MODULE_LIBRARY_HANDLE handle)
{
    return;
}

const MODULE_API* E2E_Loader_GetApi(const struct MODULE_LOADER_TAG* loader, MODULE_LIBRARY_HANDLE handle)
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

void* E2E_Loader_BuildModuleConfiguration(const struct MODULE_LOADER_TAG* loader, const void* entrypoint, const void* module_configuration)
{
	return NULL;
}

void E2E_Loader_FreeModuleConfiguration(const struct MODULE_LOADER_TAG* loader, const void* module_configuration)
{

}

// E2E static module loader does not need any JSON parsing.
MODULE_LOADER_API E2E_Loader_api =
{
    E2E_Loader_Load,
    E2E_Loader_Unload,
    E2E_Loader_GetApi,
	NULL,
	NULL,
	NULL,
	NULL,
	E2E_Loader_BuildModuleConfiguration,
	E2E_Loader_FreeModuleConfiguration
};

static MODULE_LOADER E2E_Module_Loader =
{
	NATIVE,
	"E2E",
	NULL,
	&E2E_Loader_api
};

const MODULE_LOADER* E2E_Loader_Get(void)
{
	return &E2E_Module_Loader;
}

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
	GATEWAY_MODULE_LOADER_INFO loaders[3];

    //Add Managed Module 1
	const char * senderModuleArgs = "Sender";

	DOTNET_LOADER_ENTRYPOINT senderEndpoint =
	{
		STRING_construct("E2ETestModule"),
		STRING_construct("E2ETestModule.DotNetE2ETestModule")
	};

    const char * nameModuleSender = "Sender";

	loaders[0] = {
		DotnetLoader_Get(),
		&senderEndpoint
	};

    modulesEntryArray[0] = {
        nameModuleSender,
		loaders[0],
		senderModuleArgs
    };

    //Add Managed Module 2

	const char * receiverModuleArgs = "Receiver";
	DOTNET_LOADER_ENTRYPOINT receiverEndpoint =
	{
		STRING_construct("E2ETestModule"),
		STRING_construct("E2ETestModule.DotNetE2ETestModule")
	};

	const char * nameModuleReceiver = "Receiver";

	loaders[1] = {
		DotnetLoader_Get(),
		&receiverEndpoint
	};

    const DOTNET_HOST_CONFIG receiverConfig{
        "E2ETestModule",
        "E2ETestModule.DotNetE2ETestModule",
        "Receiver"
    };

    modulesEntryArray[1] = {
		nameModuleReceiver,
		loaders[1],
		receiverModuleArgs
    };

    //Add test probe module

    const char * nameProbeModule = "probe test";

	loaders[2] = {
		E2E_Loader_Get(),
		(void*)nameProbeModule
	};

    modulesEntryArray[2] = {
        nameProbeModule,
		loaders[2],
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
	STRING_delete(senderEndpoint.dotnetModuleEntryClass);
	STRING_delete(senderEndpoint.dotnetModulePath);
	STRING_delete(receiverEndpoint.dotnetModuleEntryClass);
	STRING_delete(receiverEndpoint.dotnetModulePath);
	VECTOR_destroy(properties.gateway_modules);
	VECTOR_destroy(properties.gateway_links);

}
END_TEST_SUITE(dotnet_e2e)
