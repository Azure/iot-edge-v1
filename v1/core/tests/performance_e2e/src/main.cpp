// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <iostream>
#include <string>

#include "gateway.h"
#include "azure_c_shared_utility/threadapi.h"
#include <nanomsg/nn.h>

int main(int argc, char** argv)
{
    int sleep_in_ms = 5000;
    GATEWAY_HANDLE gateway;
    if (argc != 2 && argc != 3)
    {
        std::cout 
            << "usage: performance_sample configFile [duration]" << std::endl 
            << "where configFile is the name of the file that contains the gateway configuration" << std::endl
            << "where duration is the length of time in seconds for the test to run" << std::endl;
    }
    else
    {
        if (argc==3)
        {
            sleep_in_ms = std::stoi(argv[2]) * 1000;
        }
        
        if ((gateway = Gateway_CreateFromJson(argv[1])) == NULL)
        {
            std::cout << "failed to create the gateway from JSON" << std::endl;
        }
        else
        {
            
            std::cout << "gateway successfully created from JSON" << std::endl;
            std::cout << "gateway shall run for " << sleep_in_ms/1000 << " seconds" << std::endl;
            ThreadAPI_Sleep(sleep_in_ms);
            
            Gateway_Destroy(gateway);
        }
    }
    return 0;
}
