#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

#remote process
set(native_host_sample_sources
    ./src/remote_main.c
)

include_directories(./inc ../../proxy/gateway/native/inc ${IOTHUB_CLIENT_INC_FOLDER})
include_directories(${GW_INC})
include_directories(../../modules/common)
include_directories(${PROXY_MODULES_DIR}/native_module_host/inc)

add_executable(native_host_sample ${native_host_sample_sources})

target_link_libraries(native_host_sample nanomsg)
target_link_libraries(native_host_sample proxy_gateway native_module_host)
linkSharedUtil(native_host_sample)
install_broker(native_host_sample ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )
copy_module_host_dll(native_host_sample ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )

#gateway process.

set(native_gateway_sources
    ./src/main.c
)

if(WIN32)
    set(native_gateway_sources 
        ${native_gateway_sources}
        ./src/native_host_sample_win.json
    )
    set_source_files_properties(./src/native_host_sample_win.json PROPERTIES HEADER_FILE_ONLY ON)
else()
    set(native_gateway_sources 
        ${native_gateway_sources}
        ./src/native_host_sample_lin.json
    )
    set_source_files_properties(./src/native_host_sample_lin.json PROPERTIES HEADER_FILE_ONLY ON)
endif()

add_executable(native_gateway ${native_gateway_sources})

target_link_libraries(native_gateway gateway nanomsg)
linkSharedUtil(native_gateway)
install_broker(native_gateway ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )
copy_gateway_dll(native_gateway ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )

add_sample_to_solution(native_host_sample)
add_sample_to_solution(native_gateway)
