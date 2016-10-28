// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>

#include "simulated_device_cloud_upload_config_service.h"
#include "gateway.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/platform.h"


int main(int argc, char** argv)
{
    GATEWAY_HANDLE deviceCloudUploadGateway;

    if (argc != 2)
    {
        printf("usage: simulated_device_cloud_upload gatewayConfigFile\n");
        printf("gatewayConfigFile is a JSON configuration file \n");
    }
    else
    {
        if (platform_init() == 0)
        {
            if (configureAsAService() != 0)
            {
                LogError("Could not configure gateway as a service.");
            }
            else
            {
                deviceCloudUploadGateway = Gateway_CreateFromJson(argv[1]);
                if (deviceCloudUploadGateway == NULL)
                {
                    LogError("Failed to create gateway");
                }
                else
                {
                    /* Wait for user input to quit. */
                    waitForUserInput();
                    Gateway_Destroy(deviceCloudUploadGateway);
                }
            }
            platform_deinit();
        }
        else
        {
            LogError("Failed to initialize the platform.");
        }
    }
    return 0;
}
