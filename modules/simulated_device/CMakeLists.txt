#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

set(simulated_device_sources
    ./src/simulated_device.c
)

set(simulated_device_headers
    ./inc/simulated_device.h
)

include_directories(./inc)
include_directories(${GW_INC})

#this builds the simulated_device dynamic library
add_library(simulated_device MODULE ${simulated_device_sources}  ${simulated_device_headers})
target_link_libraries(simulated_device gateway)

#this builds the simulated_device static library
add_library(simulated_device_static STATIC ${simulated_device_sources} ${simulated_device_headers})
target_compile_definitions(simulated_device_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(simulated_device_static gateway)

linkSharedUtil(simulated_device)
linkSharedUtil(simulated_device_static)

add_module_to_solution(simulated_device)

if(install_modules)
    install(TARGETS simulated_device LIBRARY DESTINATION "${LIB_INSTALL_DIR}/modules") 
endif()

