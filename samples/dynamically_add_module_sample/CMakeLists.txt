#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

set(dynamically_add_module_sources
    ./src/main.c
)
if(WIN32)
    set(dynamically_add_module_sources 
        ${dynamically_add_module_sources}
        ./src/modules_win.json
        ./src/links_win.json
    )
    set_source_files_properties(./src/modules_win.json PROPERTIES HEADER_FILE_ONLY ON)
    set_source_files_properties(./src/links_win.json PROPERTIES HEADER_FILE_ONLY ON)
else()
    set(dynamically_add_module_sources 
        ${dynamically_add_module_sources}
        ./src/modules_lin.json
        ./src/links_lin.json
    )
    set_source_files_properties(./src/modules_lin.json PROPERTIES HEADER_FILE_ONLY ON)
    set_source_files_properties(./src/links_lin.json PROPERTIES HEADER_FILE_ONLY ON)
endif()

include_directories(./inc ${IOTHUB_CLIENT_INC_FOLDER})
include_directories(${GW_INC})
include_directories(../../modules/common)

add_executable(dynamically_add_module_sample ${dynamically_add_module_sources})

add_dependencies(dynamically_add_module_sample hello_world logger)

target_link_libraries(dynamically_add_module_sample gateway nanomsg)
linkSharedUtil(dynamically_add_module_sample)
install_broker(dynamically_add_module_sample ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )
copy_gateway_dll(dynamically_add_module_sample ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )

add_sample_to_solution(dynamically_add_module_sample)