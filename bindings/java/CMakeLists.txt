#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for java binding

set(java_binding_sources
	./c_src/message_bus_proxy.c
)

set(java_binding_headers
    ./c_inc/message_bus_proxy.h
)

include_directories(./c_inc ${IOTHUB_CLIENT_INC_FOLDER})
include_directories(${GW_INC})
include_directories($ENV{JAVA_HOME}/include $ENV{JAVA_HOME}/include/win32)

add_library(java_binding SHARED ${java_binding_headers} ${java_binding_sources})

target_link_libraries(java_binding gateway)
linkSharedUtil(java_binding)

add_binding_to_solution(java_binding)