#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for samples folder

add_subdirectory(hello_world)
add_subdirectory(simulated_device_cloud_upload)

if(LINUX)
    add_subdirectory(ble_gateway_hl)
endif()

if(${enable_dotnet_binding})
    add_subdirectory(dotnet_binding_sample)
endif()