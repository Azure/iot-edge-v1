#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

if(WIN32)
set(dotnet_core_module_sample_sources
    ./src/main.c
    ./src/dotnet_core_module_sample_win.json
)
else()
set(dotnet_core_module_sample_sources
    ./src/main.c
    ./src/dotnet_core_module_sample_lin.json
)
endif()

set_source_files_properties(./src/dotnet_core_module_sample.json PROPERTIES HEADER_FILE_ONLY ON)


set(dotnet_core_module_sample_headers
)

include_directories(${GW_INC})
include_directories(../../modules/common)

add_executable(dotnet_core_module_sample ${dotnet_core_module_sample_headers} ${dotnet_core_module_sample_sources})

add_dependencies(dotnet_core_module_sample dotnetcore logger)

target_link_libraries(dotnet_core_module_sample gateway)
linkSharedUtil(dotnet_core_module_sample)
install_broker(dotnet_core_module_sample ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )
copy_gateway_dll(dotnet_core_module_sample ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )

install_binaries(dotnet_core_module_sample ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) ${dotnet_core_managed_binding_dll} )
install_binaries(dotnet_core_module_sample ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) ${dotnet_core_sensor_module_dll} )
install_binaries(dotnet_core_module_sample ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) ${dotnet_core_printer_module_dll} )

if(WIN32)
    install_binaries(dotnet_core_module_sample ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) ${dotnet_core_host_binding_dll} )
else()
    install_binaries(dotnet_core_module_sample ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) ${dotnet_core_host_binding_so} )
endif()

add_sample_to_solution(dotnet_core_module_sample)