#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for java_sample sample

set(java_sample_sources
	./src/main.c
)
if(WIN32)
    set(java_sample_sources 
        ${java_sample_sources}
        ./src/java_sample_win.json
    )
    set_source_files_properties(./src/java_sample_win.json PROPERTIES HEADER_FILE_ONLY ON)
else()
    set(java_sample_sources 
        ${java_sample_sources}
        ./src/java_sample_lin.json
    )
    set_source_files_properties(./src/java_sample_lin.json PROPERTIES HEADER_FILE_ONLY ON)
endif()

set(java_sample_headers
)

include_directories(./inc ${IOTHUB_CLIENT_INC_FOLDER})
include_directories(${GW_INC})
include_directories(../../modules/common)

add_executable(java_sample ${java_sample_headers} ${modules_path_file} ${java_sample_sources})

target_link_libraries(java_sample gateway)
linkSharedUtil(java_sample)
install_broker(java_sample ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )


add_sample_to_solution(java_sample)