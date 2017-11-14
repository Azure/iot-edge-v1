#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

set(logger_sources
    ./src/logger.c
)

set(logger_headers
    ./inc/logger.h
)

set(logger_static_sources
    ${logger_sources}
)

set(logger_static_headers
    ${logger_headers}
)

include_directories(./inc)
include_directories(${GW_INC})

#this builds the Logger dynamic library
add_library(logger MODULE ${logger_sources}  ${logger_headers})
target_link_libraries(logger gateway)

#this builds the Logger static library
add_library(logger_static STATIC ${logger_static_sources} ${logger_static_headers})
target_compile_definitions(logger_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(logger_static gateway)

linkSharedUtil(logger)
linkSharedUtil(logger_static)

add_module_to_solution(logger)

if(${run_unittests})
	add_subdirectory(tests)
endif()

if(install_modules)
    install(TARGETS logger LIBRARY DESTINATION "${LIB_INSTALL_DIR}/modules") 
endif()

