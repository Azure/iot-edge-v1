#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for callbacks_sample sample

set(callbacks_sources
	./src/main.c
)

set(callbacks_headers
)

include_directories(./inc ${IOTHUB_CLIENT_INC_FOLDER})
include_directories(${GW_INC})

add_executable(callbacks_sample ${callbacks_headers} ${modules_path_file} ${callbacks_sources})

target_link_libraries(callbacks_sample gateway)
linkSharedUtil(callbacks_sample)

add_dependencies(callbacks_sample hello_world)
install_broker(callbacks_sample ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )

add_sample_to_solution(callbacks_sample)
