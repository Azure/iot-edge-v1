#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

include("dependencies.cmake")

set(MODULES_DIR ${CMAKE_CURRENT_LIST_DIR} CACHE INTERNAL "Modules include directory" FORCE)

include_directories(./common)

if(${enable_ble_module})
    add_subdirectory(ble)
endif()

add_subdirectory(simulated_device)
add_subdirectory(identitymap)
add_subdirectory(iothub)
add_subdirectory(logger)
add_subdirectory(hello_world)
add_subdirectory(azure_functions)
