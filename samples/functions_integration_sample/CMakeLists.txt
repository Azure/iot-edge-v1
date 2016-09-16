#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

set(functions_integration_sources
	./src/main.c
)
if(WIN32)
    set(functions_integration_sources 
        ${functions_integration_sources}
        ./src/functions_integration_win.json
    )
    set_source_files_properties(./src/functions_integration_win.json PROPERTIES HEADER_FILE_ONLY ON)
else()
    set(functions_integration_sources 
        ${functions_integration_sources}
        ./src/functions_integration_lin.json
    )
    set_source_files_properties(./src/functions_integration_lin.json PROPERTIES HEADER_FILE_ONLY ON)
endif()

set(functions_integration_headers
)

include_directories(./inc ${IOTHUB_CLIENT_INC_FOLDER})
include_directories(${GW_INC})
include_directories(../../modules/common)

add_executable(functions_integration_sample ${functions_integration_headers} ${modules_path_file} ${functions_integration_sources})

add_dependencies(functions_integration_sample hello_world hello_world_hl functions_http_trigger functions_http_trigger_hl) 

target_link_libraries(functions_integration_sample gateway)
linkSharedUtil(functions_integration_sample)
install_broker(functions_integration_sample ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )

add_sample_to_solution(functions_integration_sample)