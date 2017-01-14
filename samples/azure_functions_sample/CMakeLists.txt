#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

set(azure_functions_sources
    ./src/main.c
)
if(WIN32)
    set(azure_functions_sources 
        ${azure_functions_sources}
        ./src/azure_functions_win.json
    )
    set_source_files_properties(./src/azure_functions_win.json PROPERTIES HEADER_FILE_ONLY ON)
else()
    set(azure_functions_sources 
        ${azure_functions_sources}
        ./src/azure_functions_lin.json
    )
    set_source_files_properties(./src/azure_functions_lin.json PROPERTIES HEADER_FILE_ONLY ON)
endif()

set(azure_functions_headers
)

include_directories(./inc ${IOTHUB_CLIENT_INC_FOLDER})
include_directories(${GW_INC})
include_directories(../../modules/common)

add_executable(azure_functions_sample ${azure_functions_headers} ${modules_path_file} ${azure_functions_sources})

add_dependencies(azure_functions_sample hello_world azure_functions) 

target_link_libraries(azure_functions_sample gateway)
linkSharedUtil(azure_functions_sample)
install_broker(azure_functions_sample ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )
copy_gateway_dll(azure_functions_sample ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )
copy_iothub_client_dll(azure_functions_sample ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )

add_sample_to_solution(azure_functions_sample)