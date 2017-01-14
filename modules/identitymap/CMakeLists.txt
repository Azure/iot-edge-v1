#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

set(identity_map_sources
    ./src/identitymap.c
)

set(identity_map_headers
    ./inc/identitymap.h
)

include_directories(./inc)
include_directories(${GW_INC})

#this builds the identity_map dynamic library
add_library(identity_map MODULE ${identity_map_sources}  ${identity_map_headers})
target_link_libraries(identity_map gateway)

#this builds the identity_map static library
add_library(identity_map_static STATIC ${identity_map_sources} ${identity_map_headers})
target_compile_definitions(identity_map_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(identity_map_static gateway)

linkSharedUtil(identity_map)
linkSharedUtil(identity_map_static)

add_module_to_solution(identity_map)

if(install_modules)
    install(TARGETS identity_map LIBRARY DESTINATION "${LIB_INSTALL_DIR}/modules") 
endif()

if(${run_unittests})
	add_subdirectory(tests)
endif()
