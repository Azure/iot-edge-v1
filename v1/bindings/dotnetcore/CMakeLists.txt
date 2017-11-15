#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

if(WIN32)
    set(dotnetcore_utils_file ./src/adapters/dotnetcore_utils_windows.cpp)
elseif(LINUX)
    set(dotnetcore_utils_file ./src/adapters/dotnetcore_utils_linux.cpp)
endif()


set(dotnet_core_sources
    ./src/dotnetcore.cpp
    ${dotnetcore_utils_file}
)

set(dotnet_core_headers
    ./inc/dotnetcore.h
    ./inc/dotnetcore_common.h
    ./inc/dotnetcore_utils.h
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