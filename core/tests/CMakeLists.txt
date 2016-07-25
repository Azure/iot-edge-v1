#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for the core tests folder

add_subdirectory(gwmessage_unittests)
add_subdirectory(module_loader_unittests)
add_subdirectory(dynamic_library_unittests)
add_subdirectory(message_bus_unittests)
add_subdirectory(gateway_ll_unittests)
add_subdirectory(gateway_unittests)

if(WIN32)
add_subdirectory(message_bus_uwp_unittests)
add_subdirectory(gateway_ll_uwp_unittests)
endif()

if(${run_e2e_tests})
    add_subdirectory(gw_e2etests)
endif()

