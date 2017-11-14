#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

if(NOT IS_DIRECTORY $ENV{NETFXSDKDir})
    message(FATAL_ERROR "Environment variable NETFXSDKDir is not defined.")
endif()

set(NETFXSdkDir $ENV{NETFXSDKDir})

set(dotnet_sources
    ./src/dotnet.cpp
)

set(dotnet_headers
    ./inc/dotnet.h
)

include_directories(./inc)
include_directories(${GW_INC})
include_directories(${IOTHUB_CLIENT_INC_FOLDER})
include_directories(${NETFXSdkDir}Include/um)

#this builds the dotnet dynamic library
add_library(dotnet MODULE ${dotnet_sources} ${dotnet_headers})
target_link_libraries(dotnet gateway mscoree)

#this builds the dotnet static library
add_library(dotnet_static STATIC ${dotnet_sources} ${dotnet_headers})
target_compile_definitions(dotnet_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(dotnet_static gateway mscoree)

linkSharedUtil(dotnet)
linkSharedUtil(dotnet_static)

if(${run_unittests})
	add_subdirectory(tests)
endif()

add_binding_to_solution(dotnet)
add_binding_to_solution(dotnet_static)

if(install_modules)
    install(TARGETS dotnet LIBRARY DESTINATION "${LIB_INSTALL_DIR}/modules")
endif()
