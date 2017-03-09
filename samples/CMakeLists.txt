#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

add_subdirectory(hello_world)
add_subdirectory(simulated_device_cloud_upload)
add_subdirectory(experimental/events_sample)
add_subdirectory(azure_functions_sample)

if(${enable_dotnet_binding})
    add_subdirectory(dotnet_binding_sample)
endif()

if(${enable_dotnet_core_binding})
    add_subdirectory(dotnet_core_module_sample)
endif()

if(${enable_java_binding})
    add_subdirectory(java_sample)
endif()

if(${enable_nodejs_binding})
    add_subdirectory(nodejs_simple_sample)
endif()

if (${enable_native_remote_modules})
    add_subdirectory(proxy_sample)
endif()

if(${enable_ble_module} AND LINUX)
    add_subdirectory(ble_gateway)
endif()
