#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for device_ping sample

set(device_ping_sources
	./src/main.c
)
if(WIN32)
    set(device_ping_sources
        ${device_ping_sources}
        ./src/device_ping_win.json
    )
    set_source_files_properties(./src/device_ping_win.json PROPERTIES HEADER_FILE_ONLY ON)
else()
    set(device_ping_sources
        ${device_ping_sources}
        ./src/device_ping_lin.json
    )
    set_source_files_properties(./src/device_ping_lin.json PROPERTIES HEADER_FILE_ONLY ON)
endif()

set(device_ping_headers
)

include_directories(./inc ${IOTHUB_CLIENT_INC_FOLDER})
include_directories(${GW_INC})
include_directories(../../modules/common)

add_executable(device_ping ${device_ping_headers} ${modules_path_file} ${device_ping_sources})

add_dependencies(device_ping iothubdeviceping_hl logger_hl iothubdeviceping)

target_link_libraries(device_ping gateway nanomsg)
linkSharedUtil(device_ping)
install_broker(device_ping ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )

add_sample_to_solution(device_ping)

add_subdirectory(iothubdeviceping)