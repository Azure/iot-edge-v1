// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "iothubtest.h"
#include "iothub_account.h"
#include "azure_c_shared_utility/platform.h"

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"
#include "azure_c_shared_utility/map.h"
#include "azure_c_shared_utility/lock.h"
#include "gateway_ll.h"
#include "iothubhttp.h"
#include "e2e_module.h"
#include "messageproperties.h"
#include "identitymap.h"
#include "azure_c_shared_utility/threadapi.h"
#include "module_config_resources.h"

static MICROMOCK_MUTEX_HANDLE g_testByTest;
static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

static size_t g_iotHubTestId = 0;
static IOTHUB_ACCOUNT_INFO_HANDLE g_iothubAcctInfo = NULL;

#define MAX_CLOUD_TRAVEL_TIME           60
#define SEND_DATA_LENGTH                50
const char* TEST_EVENT_DATA_FMT = "{\"data\":\"%.24s\",\"id\":\"%d\"}";

typedef struct EXPECTED_SEND_DATA_TAG
{
	const char* expectedString;
	bool wasFound;
} EXPECTED_SEND_DATA;

static EXPECTED_SEND_DATA* EventData_Create(void)
{
	EXPECTED_SEND_DATA* result = (EXPECTED_SEND_DATA*)malloc(sizeof(EXPECTED_SEND_DATA));
	if (result != NULL)
	{
		char* temp = (char*)malloc(SEND_DATA_LENGTH);
		
		if (temp == NULL)
		{
			free(result);
			result = NULL;
		}
		else
		{
			char* tempString;
			time_t t = time(NULL);
			(void)sprintf_s(temp, SEND_DATA_LENGTH, TEST_EVENT_DATA_FMT, ctime(&t), g_iotHubTestId);
			if ((tempString = (char*)malloc(strlen(temp) + 1)) == NULL)
			{
				free(result);
				result = NULL;
			}
			else
			{
				strcpy(tempString, temp);
				result->expectedString = tempString;
				result->wasFound = false;
				free(temp);
			}
		}
	}
	return result;
}


static void EventData_Destroy(EXPECTED_SEND_DATA* data)
{

	if (data != NULL)
	{
		free((char*)data->expectedString);
		free(data);
	}
}
static int IoTHubCallback(void* context, const char* data, size_t size)
{
	int result = 0; // 0 means "keep processing"

	EXPECTED_SEND_DATA* expectedData = (EXPECTED_SEND_DATA*)context;
	if (expectedData != NULL)
	{
		if (
			(strlen(expectedData->expectedString) == size) &&
			(memcmp(expectedData->expectedString, data, size) == 0)
			)
		{
			expectedData->wasFound = true;
			result = 1;
		}
	}
	return result;
}

BEGIN_TEST_SUITE(gw_e2etests)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
		TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
		g_testByTest = MicroMockCreateMutex();
		ASSERT_IS_NOT_NULL(g_testByTest);
		platform_init();
		g_iothubAcctInfo = IoTHubAccount_Init(true);
		ASSERT_IS_NOT_NULL(g_iothubAcctInfo);
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
		IoTHubAccount_deinit(g_iothubAcctInfo);
		platform_deinit();

		MicroMockDestroyMutex(g_testByTest);
		TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
    }

    TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
    {
		if (!MicroMockAcquireMutex(g_testByTest))
		{
			ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
		}
		g_iotHubTestId++;
    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
		if (!MicroMockReleaseMutex(g_testByTest))
		{
			ASSERT_FAIL("failure in test framework at ReleaseMutex");
		}
    }

    TEST_FUNCTION(GW_e2eModule_Sending_Data)
    {
        ///arrange
		GATEWAY_HANDLE e2eGatewayInstance;

		/* Setup: data for IoT Hub Module */
		IOTHUBHTTP_CONFIG iotHubConfig;

		iotHubConfig.IoTHubName = IoTHubAccount_GetIoTHubName(g_iothubAcctInfo);
		iotHubConfig.IoTHubSuffix = IoTHubAccount_GetIoTHubSuffix(g_iothubAcctInfo);


		E2EMODULE_CONFIG e2eModuleConfiguration;
		EXPECTED_SEND_DATA* sendData = EventData_Create();
		ASSERT_IS_NOT_NULL_WITH_MSG(sendData, "Failure creating data to be sent");

		e2eModuleConfiguration.macAddress = "01:01:01:01:01:01";
		e2eModuleConfiguration.sendData = sendData->expectedString;

		/* Setup: data for identity map module */
		IDENTITY_MAP_CONFIG e2eModuleMapping[1];
		e2eModuleMapping[0].deviceId = IoTHubAccount_GetDeviceId(g_iothubAcctInfo);
		e2eModuleMapping[0].deviceKey = IoTHubAccount_GetDeviceKey(g_iothubAcctInfo);
		e2eModuleMapping[0].macAddress = "01:01:01:01:01:01";

		VECTOR_HANDLE e2eModuleMappingVector = VECTOR_create(sizeof(IDENTITY_MAP_CONFIG));
		if (e2eModuleMappingVector == NULL)
		{
			ASSERT_FAIL("Could not create vector for identity map configuration.");
		}
		else
		{
			if (VECTOR_push_back(e2eModuleMappingVector, &e2eModuleMapping, 1) != 0)
			{
				ASSERT_FAIL("Could not push data into vector for identity map configuration.");
			}
		}
		
		GATEWAY_PROPERTIES_ENTRY modules[3];

		modules[0].module_configuration = &iotHubConfig;
		modules[0].module_name = "IoTHub";
		modules[0].module_path = iothubhttp_module_path();

		
		modules[1].module_configuration = e2eModuleMappingVector;
		modules[1].module_name = GW_IDMAP_MODULE;
		modules[1].module_path = identity_map_module_path();

		modules[2].module_configuration = &e2eModuleConfiguration;
		modules[2].module_name = "E2ETest";
		modules[2].module_path = e2e_module_path();
		
		
		GATEWAY_PROPERTIES m6GatewayProperties;
		VECTOR_HANDLE gatewayProps = VECTOR_create(sizeof(GATEWAY_PROPERTIES_ENTRY));

		VECTOR_push_back(gatewayProps, &modules, 3);


		///act
		m6GatewayProperties.gateway_properties_entries = gatewayProps;
		e2eGatewayInstance = Gateway_LL_Create(&m6GatewayProperties);



        ///assert
		ASSERT_IS_NOT_NULL(e2eGatewayInstance);

		ThreadAPI_Sleep(MAX_CLOUD_TRAVEL_TIME * 1000);

		Gateway_LL_Destroy(e2eGatewayInstance);

		VECTOR_destroy(e2eModuleMappingVector);
		VECTOR_destroy(gatewayProps);

		platform_deinit();
		platform_init();
		//step3: get the data from the other side
		{
			IOTHUB_TEST_HANDLE iotHubTestHandle = IoTHubTest_Initialize(IoTHubAccount_GetEventHubConnectionString(g_iothubAcctInfo), IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo), IoTHubAccount_GetDeviceId(g_iothubAcctInfo), IoTHubAccount_GetDeviceKey(g_iothubAcctInfo), IoTHubAccount_GetEventhubListenName(g_iothubAcctInfo), IoTHubAccount_GetEventhubAccessKey(g_iothubAcctInfo), IoTHubAccount_GetSharedAccessSignature(g_iothubAcctInfo), IoTHubAccount_GetEventhubConsumerGroup(g_iothubAcctInfo));
			ASSERT_IS_NOT_NULL(iotHubTestHandle);

			IOTHUB_TEST_CLIENT_RESULT result = IoTHubTest_ListenForEventForMaxDrainTime(iotHubTestHandle, IoTHubCallback, IoTHubAccount_GetIoTHubPartitionCount(g_iothubAcctInfo), sendData);
			IoTHubTest_Deinit(iotHubTestHandle);
		}

		ASSERT_IS_TRUE_WITH_MSG(sendData->wasFound, "Failure receiving data from eventhub");
		///cleanup

		EventData_Destroy(sendData);
    }
END_TEST_SUITE(gw_e2etests)
