// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "gateway.h"
#include "module_config_resources.h"
#include "simulator.h"
#include "module_loader.h"
#include "module_loaders/dynamic_loader.h"
#include "azure_c_shared_utility/threadapi.h"

#include "testrunnerswitcher.h"

//=============================================================================
//Globals
//=============================================================================

static TEST_MUTEX_HANDLE g_dllByDll;
static TEST_MUTEX_HANDLE g_testByTest;

BEGIN_TEST_SUITE(Performance_e2e)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    TEST_MUTEX_DESTROY(g_testByTest);
    TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest) != 0)
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

TEST_FUNCTION(Performance_e2e_5_second_run)
{
        ///arrange
        GATEWAY_HANDLE e2eGatewayInstance;

        /* Setup: data for simulator module */
        SIMULATOR_MODULE_CONFIG simulator_config = 
        {
            "device1",
            0,
            2,
            16,
            256
        };

        GATEWAY_MODULES_ENTRY modules[2];
		DYNAMIC_LOADER_ENTRYPOINT loader_info[2];
        GATEWAY_LINK_ENTRY links[1];
		
        // simulator
		modules[0].module_name = "simulator1";
        modules[0].module_configuration = &simulator_config;
		modules[0].module_loader_info.loader = DynamicLoader_Get();
		loader_info[0].moduleLibraryFileName = STRING_construct(simulator_module_path());
		modules[0].module_loader_info.entrypoint = (void*)&(loader_info[0]);

        // metrics
		modules[1].module_name = "metrics1";
		modules[1].module_configuration = NULL;
		modules[1].module_loader_info.loader = DynamicLoader_Get();
		loader_info[1].moduleLibraryFileName = STRING_construct(metrics_module_path());
		modules[1].module_loader_info.entrypoint = (void*)&(loader_info[1]);

        links[0].module_source = "simulator1";
        links[0].module_sink = "metrics1";

        GATEWAY_PROPERTIES performance_gw_properties;
        VECTOR_HANDLE gatewayProps = VECTOR_create(sizeof(GATEWAY_MODULES_ENTRY));
        VECTOR_HANDLE gatewayLinks = VECTOR_create(sizeof(GATEWAY_LINK_ENTRY));

        VECTOR_push_back(gatewayProps, &modules, 2);
        VECTOR_push_back(gatewayLinks, &links, 1);



        ///act
        performance_gw_properties.gateway_modules = gatewayProps;
        performance_gw_properties.gateway_links = gatewayLinks; 
        e2eGatewayInstance = Gateway_Create(&performance_gw_properties);
        GATEWAY_START_RESULT start_result = Gateway_Start(e2eGatewayInstance);

        ///assert
        ASSERT_IS_NOT_NULL(e2eGatewayInstance);
        ASSERT_IS_TRUE((start_result == GATEWAY_START_SUCCESS));


        ThreadAPI_Sleep(5000);

        Gateway_Destroy(e2eGatewayInstance);

        VECTOR_destroy(gatewayProps);
        VECTOR_destroy(gatewayLinks);

		for (int loader = 0; loader < 2; loader++)
		{
			STRING_delete(loader_info[loader].moduleLibraryFileName);
		}
}

TEST_FUNCTION(Performance_e2e_10_second_run)
{
        ///arrange
        GATEWAY_HANDLE e2eGatewayInstance;

        /* Setup: data for simulator module */
        SIMULATOR_MODULE_CONFIG simulator_config = 
        {
            "device1",
            0,
            2,
            16,
            256
        };

        GATEWAY_MODULES_ENTRY modules[2];
		DYNAMIC_LOADER_ENTRYPOINT loader_info[2];
        GATEWAY_LINK_ENTRY links[1];
		
        // simulator
		modules[0].module_name = "simulator1";
        modules[0].module_configuration = &simulator_config;
		modules[0].module_loader_info.loader = DynamicLoader_Get();
		loader_info[0].moduleLibraryFileName = STRING_construct(simulator_module_path());
		modules[0].module_loader_info.entrypoint = (void*)&(loader_info[0]);

        // metrics
		modules[1].module_name = "metrics1";
		modules[1].module_configuration = NULL;
		modules[1].module_loader_info.loader = DynamicLoader_Get();
		loader_info[1].moduleLibraryFileName = STRING_construct(metrics_module_path());
		modules[1].module_loader_info.entrypoint = (void*)&(loader_info[1]);

        links[0].module_source = "simulator1";
        links[0].module_sink = "metrics1";

        GATEWAY_PROPERTIES performance_gw_properties;
        VECTOR_HANDLE gatewayProps = VECTOR_create(sizeof(GATEWAY_MODULES_ENTRY));
        VECTOR_HANDLE gatewayLinks = VECTOR_create(sizeof(GATEWAY_LINK_ENTRY));

        VECTOR_push_back(gatewayProps, &modules, 2);
        VECTOR_push_back(gatewayLinks, &links, 1);



        ///act
        performance_gw_properties.gateway_modules = gatewayProps;
        performance_gw_properties.gateway_links = gatewayLinks; 
        e2eGatewayInstance = Gateway_Create(&performance_gw_properties);
        GATEWAY_START_RESULT start_result = Gateway_Start(e2eGatewayInstance);

        ///assert
        ASSERT_IS_NOT_NULL(e2eGatewayInstance);
        ASSERT_IS_TRUE((start_result == GATEWAY_START_SUCCESS));


        ThreadAPI_Sleep(10000);

        Gateway_Destroy(e2eGatewayInstance);

        VECTOR_destroy(gatewayProps);
        VECTOR_destroy(gatewayLinks);

		for (int loader = 0; loader < 2; loader++)
		{
			STRING_delete(loader_info[loader].moduleLibraryFileName);
		}

}


END_TEST_SUITE(Performance_e2e);
