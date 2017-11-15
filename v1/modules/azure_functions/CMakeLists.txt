#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

set(azure_functions_sources
    ./src/azure_functions.c
)

set(azure_functions_headers
    ./inc/azure_functions.h
)

include_directories(./inc)
include_directories(${GW_INC})

#this builds the azure_functions dynamic library
add_library(azure_functions MODULE ${azure_functions_sources}  ${azure_functions_headers})
target_link_libraries(azure_functions gateway)

#this builds the azure_functions static library
add_library(azure_functions_static STATIC ${azure_functions_sources} ${azure_functions_headers})
target_compile_definitions(azure_functions_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(azure_functions_static gateway)

linkSharedUtil(azure_functions)
linkSharedUtil(azure_functions_static)

add_module_to_solution(azure_functions)

if(install_executables)
    install(TARGETS azure_functions LIBRARY DESTINATION lib) 
endif()

if(${run_unittests})
	add_subdirectory(tests)
endif()