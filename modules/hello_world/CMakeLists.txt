#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

set(hello_world_sources
    ./src/hello_world.c
)

set(hello_world_headers
    ./inc/hello_world.h
)

include_directories(./inc)
include_directories(${GW_INC})

#this builds the hello_world dynamic library
add_library(hello_world MODULE ${hello_world_sources} ${hello_world_headers})
target_link_libraries(hello_world gateway)

#this builds the hello_world static library
add_library(hello_world_static STATIC ${hello_world_sources} ${hello_world_headers})
target_compile_definitions(hello_world_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(hello_world_static gateway)

linkSharedUtil(hello_world)
linkSharedUtil(hello_world_static)

add_module_to_solution(hello_world)

if(install_modules)
    install(TARGETS hello_world LIBRARY DESTINATION "${LIB_INSTALL_DIR}/modules") 
endif()

