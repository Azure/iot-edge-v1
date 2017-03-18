#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

if(POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif()

set(native_module_host_sources
    ./src/native_module_host.c
)

set(native_module_host_headers
    ./inc/native_module_host.h
)

set(native_module_host_static_sources
    ${native_module_host_sources}
)

set(native_module_host_static_headers
    ${native_module_host_headers}
)

include_directories(./inc)
include_directories(${GW_INC})

#this builds the native_module_host dynamic library
add_library(native_module_host SHARED ${native_module_host_sources}  ${native_module_host_headers})
target_link_libraries(native_module_host parson aziotsharedutil module_host_static proxy_gateway)

#this builds the native_module_host static library
add_library(native_module_host_static STATIC ${native_module_host_static_sources} ${native_module_host_static_headers})
target_compile_definitions(native_module_host_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(native_module_host_static parson aziotsharedutil module_host_static proxy_gateway)

linkSharedUtil(native_module_host)
linkSharedUtil(native_module_host_static)

add_module_to_solution(native_module_host)

if(${run_unittests})
	add_subdirectory(tests)
endif()

if(install_modules)
    install(TARGETS native_module_host LIBRARY DESTINATION "${LIB_INSTALL_DIR}/modules") 
endif()

