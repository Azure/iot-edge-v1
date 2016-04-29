#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for the iothubhttp module

set(iothubhttp_sources
	./src/iothubhttp.c
)

set(iothubhttp_headers
	./inc/iothubhttp.h
)

set(iothubhttp_hl_sources
	./src/iothubhttp_hl.c
)

set(iothubhttp_hl_headers
	./inc/iothubhttp_hl.h
)

include_directories(./inc)
include_directories(${GW_INC})
include_directories(${IOTHUB_CLIENT_INC_FOLDER})

#this builds the IoTHubHTTP dynamic library
add_library(iothubhttp MODULE
    ${iothubhttp_sources}
    ${iothubhttp_headers}
)
target_link_libraries(iothubhttp
    gateway
    iothub_client
    iothub_client_http_transport
)

#this builds the IoTHubHTTP static library
add_library(iothubhttp_static
    ${iothubhttp_sources}
    ${iothubhttp_headers}
)
target_compile_definitions(iothubhttp_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(iothubhttp_static
    gateway
    iothub_client
    iothub_client_http_transport
)

#this builds the IoTHubHTTP_HL dynamic library (by default it uses IoTHub Module linked statically)
add_library(iothubhttp_hl MODULE
    ${iothubhttp_hl_sources}
    ${iothubhttp_hl_headers}
)
target_link_libraries(iothubhttp_hl
    iothubhttp_static
)

#this builds the IoTHubHTTP_HL static library (by default it uses IoTHub Module linked statically)
add_library(iothubhttp_hl_static
    ${iothubhttp_hl_sources}
    ${iothubhttp_hl_headers}
)
target_compile_definitions(iothubhttp_hl_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(iothubhttp_hl_static
    iothubhttp_static
)

linkSharedUtil(iothubhttp)
linkSharedUtil(iothubhttp_static)
linkSharedUtil(iothubhttp_hl)
linkSharedUtil(iothubhttp_hl_static)

linkHttp(iothubhttp)
linkHttp(iothubhttp_static)
linkHttp(iothubhttp_hl)
linkHttp(iothubhttp_hl_static)

add_module_to_solution(iothubhttp)

add_subdirectory(tests)

if(install_executables)
	install(TARGETS iothubhttp LIBRARY DESTINATION lib) 
    install(TARGETS iothubhttp_hl LIBRARY DESTINATION lib) 
endif()
