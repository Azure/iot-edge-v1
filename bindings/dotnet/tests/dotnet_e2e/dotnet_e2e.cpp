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
#include "gateway_ll.h"
#include "messageproperties.h"
#include "azure_c_shared_utility/threadapi.h"
#include "internal/event_system.h"


DEFINE_MICROMOCK_ENUM_TO_STRING(BROKER_RESULT, BROKER_RESULT_VALUES);

static MICROMOCK_MUTEX_HANDLE g_testByTest;
static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

static bool messageRequestReceived = false;
static bool messageReplyReceived = false;

typedef struct GATEWAY_HANDLE_DATA_TAG {
	/** @brief Vector of MODULE_DATA modules that the Gateway must track */
	VECTOR_HANDLE modules;

	/** @brief The message broker contained within this Gateway */
	BROKER_HANDLE broker;

	/** @brief Handle for callback event system coupled with this Gateway */
	EVENTSYSTEM_HANDLE event_system;

	/** @brief Vector of LINK_DATA links that the Gateway must track */
	VECTOR_HANDLE links;
} GATEWAY_HANDLE_DATA;

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
static const MODULE_APIS E2E_APIS_all =
{
	E2EModule_Create,
	E2EModule_Destroy,
	E2EModule_Receive
};

MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void)
{
	return &E2E_APIS_all;
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
	///////act	
	GATEWAY_HANDLE e2eGatewayInstance;


	//Add Managed Module 1
	GATEWAY_MODULES_ENTRY managedModuleSender;

	managedModuleSender.module_name = "Sender";
	managedModuleSender.module_path = "..\\..\\..\\Debug\\dotnet_hl.dll";
	managedModuleSender.module_configuration = "{\"dotnet_module_path\":\"E2ETestModule\",\"dotnet_module_entry_class\":\"E2ETestModule.DotNetE2ETestModule\",\"dotnet_module_args\":\"Sender\"}";
	e2eGatewayInstance = Gateway_LL_Create(NULL);

	MODULE_HANDLE managedModuleSenderHandle = Gateway_LL_AddModule(e2eGatewayInstance, &managedModuleSender);

	//Add Managed Module 2
	GATEWAY_MODULES_ENTRY managedModuleReceiver;

	managedModuleReceiver.module_name = "Receiver";
	managedModuleReceiver.module_path = "..\\..\\..\\Debug\\dotnet_hl.dll";
	managedModuleReceiver.module_configuration = "{\"dotnet_module_path\":\"E2ETestModule\",\"dotnet_module_entry_class\":\"E2ETestModule.DotNetE2ETestModule\",\"dotnet_module_args\":\"Receiver\"}";

	MODULE_HANDLE managedModuleReceiverHandle = Gateway_LL_AddModule(e2eGatewayInstance, &managedModuleReceiver);


	//Set the test probe module.
	GATEWAY_HANDLE_DATA* gatewayHandleData = (GATEWAY_HANDLE_DATA*)e2eGatewayInstance;

	MODULE myProbeTestModule;

	myProbeTestModule.module_apis = &E2E_APIS_all;
	myProbeTestModule.module_handle = E2EModule_Create(gatewayHandleData->broker, NULL);

	BROKER_RESULT myBrokerResult = Broker_AddModule(gatewayHandleData->broker, &myProbeTestModule);

	ASSERT_ARE_EQUAL(BROKER_RESULT, BROKER_OK, myBrokerResult);

	//Now sets the links. From Sender -> Receiver and from both to Test probe
	BROKER_LINK_DATA fromSenderToTestProbe;
	fromSenderToTestProbe.module_source_handle = managedModuleSenderHandle;
	fromSenderToTestProbe.module_sink_handle = myProbeTestModule.module_handle;

	myBrokerResult = Broker_AddLink(gatewayHandleData->broker, &fromSenderToTestProbe);
	ASSERT_ARE_EQUAL(BROKER_RESULT, BROKER_OK, myBrokerResult);

	BROKER_LINK_DATA fromSenderToReceiver;
	fromSenderToReceiver.module_source_handle = managedModuleSenderHandle;
	fromSenderToReceiver.module_sink_handle = managedModuleReceiverHandle;

	myBrokerResult = Broker_AddLink(gatewayHandleData->broker, &fromSenderToReceiver);
	ASSERT_ARE_EQUAL(BROKER_RESULT, BROKER_OK, myBrokerResult);

	BROKER_LINK_DATA fromReceiverToTestProbe;
	fromReceiverToTestProbe.module_source_handle = managedModuleReceiverHandle;
	fromReceiverToTestProbe.module_sink_handle = myProbeTestModule.module_handle;

	myBrokerResult = Broker_AddLink(gatewayHandleData->broker, &fromReceiverToTestProbe);
	ASSERT_ARE_EQUAL(BROKER_RESULT, BROKER_OK, myBrokerResult);

	/////assert
	ASSERT_IS_NOT_NULL(e2eGatewayInstance);
	ASSERT_IS_NOT_NULL(managedModuleReceiverHandle);
	ASSERT_IS_NOT_NULL(managedModuleSenderHandle);


	ThreadAPI_Sleep(5000); //Modules are configured to send a message every second, so waitig 5 seconds has to be enough.

	ASSERT_IS_TRUE(messageRequestReceived);
	ASSERT_IS_TRUE(messageReplyReceived);


	//Cleanup
	Broker_RemoveModule(gatewayHandleData->broker, &myProbeTestModule);

	Gateway_LL_Destroy(e2eGatewayInstance);

}
END_TEST_SUITE(dotnet_e2e)
