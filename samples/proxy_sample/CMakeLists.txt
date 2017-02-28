#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

#remote process
set(proxy_sample_sources
    ./src/remote_main.c
)

include_directories(./inc ../../proxy/gateway/native/inc ${IOTHUB_CLIENT_INC_FOLDER})
include_directories(${GW_INC})
include_directories(../../modules/common)

add_executable(proxy_sample_remote ${proxy_sample_sources})

target_link_libraries(proxy_sample_remote nanomsg)
target_link_libraries(proxy_sample_remote proxy_gateway )
linkSharedUtil(proxy_sample_remote)
install_broker(proxy_sample_remote ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )

#gateway process.

set(proxy_sample_main_sources
    ./src/main.c
)

if(WIN32)
    set(proxy_sample_main_sources 
        ${proxy_sample_main_sources}
        ./src/proxy_sample_win.json
    )
    set_source_files_properties(./src/proxy_sample_win.json PROPERTIES HEADER_FILE_ONLY ON)
else()
    set(proxy_sample_main_sources 
        ${proxy_sample_main_sources}
        ./src/proxy_sample_lin.json
    )
    set_source_files_properties(./src/proxy_sample_lin.json PROPERTIES HEADER_FILE_ONLY ON)
endif()

add_executable(proxy_sample_main ${proxy_sample_main_sources})

target_link_libraries(proxy_sample_main gateway nanomsg)
linkSharedUtil(proxy_sample_main)
install_broker(proxy_sample_main ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )
copy_gateway_dll(proxy_sample_main ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )

add_sample_to_solution(proxy_sample_remote)
add_sample_to_solution(proxy_sample_main)