#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for the core tests folder

add_subdirectory(broadcast_bus_ut)
add_subdirectory(broker_ut)
add_subdirectory(dynamic_library_ut)
add_subdirectory(event_system_ut)
add_subdirectory(gateway_ll_ut)
add_subdirectory(gateway_ut)
add_subdirectory(gwmessage_ut)
add_subdirectory(module_loader_ut)

if(WIN32)
    add_subdirectory(broker_uwp_ut)
    add_subdirectory(gateway_ll_uwp_ut)
endif()

if(${run_e2e_tests})
    add_subdirectory(gateway_e2e)
endif()

