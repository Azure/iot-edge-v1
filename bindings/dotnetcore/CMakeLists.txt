#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

set(dotnet_core_sources
    ./src/modules_manager.cpp
)

set(dotnet_core_headers
    ./inc/dotnetcore.h
    ./inc/dotnetcore_common.h
    ./inc/modules_manager.h
)

include_directories(./inc)
include_directories(${GW_INC})
include_directories(${IOTHUB_CLIENT_INC_FOLDER})

#this builds the dotnet core dynamic library
add_library(dotnetcore MODULE ${dotnet_core_sources} ${dotnet_core_headers})
target_link_libraries(dotnetcore gateway)

#this builds the dotnet core static library
add_library(dotnet_core_static STATIC ${dotnet_core_sources} ${dotnet_core_headers})
target_compile_definitions(dotnet_core_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(dotnet_core_static gateway)

linkSharedUtil(dotnetcore)
linkSharedUtil(dotnet_core_static)

if(${run_unittests})
	add_subdirectory(tests)
endif()

add_binding_to_solution(dotnetcore)
add_binding_to_solution(dotnet_core_static)

if(install_modules)
    install(TARGETS dotnetcore LIBRARY DESTINATION "${LIB_INSTALL_DIR}/modules")
endif()