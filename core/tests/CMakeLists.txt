#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

add_subdirectory(broker_ut)
add_subdirectory(dynamic_library_ut)
add_subdirectory(outprocess_loader_ut)
add_subdirectory(outprocess_module_ut)
add_subdirectory(event_system_ut)
add_subdirectory(gateway_ut)
add_subdirectory(gateway_createfromjson_ut)
add_subdirectory(gwmessage_ut)
add_subdirectory(control_msg_ut)
add_subdirectory(dynamic_loader_ut)
add_subdirectory(module_loader_ut)
if(${enable_java_binding})
    add_subdirectory(java_loader_ut)
endif()
if(${enable_dotnet_binding})
    add_subdirectory(dotnet_loader_ut)
endif()

if(${enable_dotnet_core_binding})
    add_subdirectory(dotnet_core_loader_ut)
endif()

if(${enable_nodejs_binding})
    add_subdirectory(node_loader_ut)
endif()

if(${run_e2e_tests})
    add_subdirectory(gateway_e2e)
endif()

