#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for hello_world sample

set(hello_world_sources
	./src/main.c
)
if(WIN32)
    set(hello_world_sources 
        ${hello_world_sources}
        ./src/hello_world_win.json
    )
    set_source_files_properties(./src/hello_world_win.json PROPERTIES HEADER_FILE_ONLY ON)
else()
    set(hello_world_sources 
        ${hello_world_sources}
        ./src/hello_world_lin.json
    )
    set_source_files_properties(./src/hello_world_lin.json PROPERTIES HEADER_FILE_ONLY ON)
endif()

set(hello_world_headers
)

include_directories(./inc ${IOTHUB_CLIENT_INC_FOLDER})
include_directories(${GW_INC})
include_directories(../../modules/common)

add_executable(hello_world_sample ${hello_world_headers} ${modules_path_file} ${hello_world_sources})

add_dependencies(hello_world_sample logger_hl)

target_link_libraries(hello_world_sample gateway nanomsg)
linkSharedUtil(hello_world_sample)
install_broker(hello_world_sample ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )

add_sample_to_solution(hello_world_sample)