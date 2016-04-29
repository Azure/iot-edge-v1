#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for the modules

set(MODULES_DIR ${CMAKE_CURRENT_LIST_DIR} CACHE INTERNAL "Modules include directory" FORCE)

function(linkModule moduleName)
	include_directories(${MODULES_DIR}/${moduleName}/inc)
endfunction(linkModule)

include_directories(./common)

add_subdirectory(ble)
add_subdirectory(simulated_device)
add_subdirectory(identitymap)
add_subdirectory(iothubhttp)
add_subdirectory(logger)
add_subdirectory(hello_world)