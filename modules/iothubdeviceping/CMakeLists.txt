#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for the iothubdeviceping module

set(iothubdeviceping_sources
	./src/iothubdeviceping.c
)

set(iothubdeviceping_headers
	./inc/iothubdeviceping.h
)

set(iothubdeviceping_hl_sources
	./src/iothubdeviceping_hl.c
)

set(iothubdeviceping_hl_headers
	./inc/iothubdeviceping_hl.h
)

include_directories(./inc)
include_directories(${GW_INC})
include_directories(${IOTHUB_CLIENT_INC_FOLDER} ${UAMQP_INC_FOLDER} ${SHARED_UTIL_INC_FOLDER})

#this builds the IoTHubDevicePing dynamic library
add_library(iothubdeviceping MODULE
    ${iothubdeviceping_sources}
    ${iothubdeviceping_headers}
)
target_link_libraries(iothubdeviceping
    gateway
    iothub_client
)

#this builds the IoTHubDevicePing static library
add_library(iothubdeviceping_static
    ${iothubdeviceping_sources}
    ${iothubdeviceping_headers}
)
target_compile_definitions(iothubdeviceping_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(iothubdeviceping_static
    gateway
    iothub_client
)

#this builds the IoTHubDevicePing_HL dynamic library (by default it uses IoTHub Module linked statically)
add_library(iothubdeviceping_hl MODULE
    ${iothubdeviceping_hl_sources}
    ${iothubdeviceping_hl_headers}
)
target_link_libraries(iothubdeviceping_hl
    iothubdeviceping_static
)

#this builds the IoTHubDevicePing_HL static library (by default it uses IoTHub Module linked statically)
add_library(iothubdeviceping_hl_static
    ${iothubdeviceping_hl_sources}
    ${iothubdeviceping_hl_headers}
)
target_compile_definitions(iothubdeviceping_hl_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(iothubdeviceping_hl_static
    iothubdeviceping_static
)

if(${use_amqp})
	target_link_libraries(iothubdeviceping_static iothub_client_amqp_transport)
	target_link_libraries(iothubdeviceping iothub_client_amqp_transport)
	linkUAMQP(iothubdeviceping_static)
	linkUAMQP(iothubdeviceping)
else()
	add_definitions(-DIOTHUBMODULE_NULL_AMQP)
endif()

if(${use_http})
	target_link_libraries(iothubdeviceping_static iothub_client_http_transport)
	target_link_libraries(iothubdeviceping iothub_client_http_transport)
	linkHttp(iothubdeviceping_static)
	linkHttp(iothubdeviceping)
else()
	add_definitions(-DIOTHUBMODULE_NULL_HTTP)
endif()

if(${use_mqtt})
	target_link_libraries(iothubdeviceping_static iothub_client_mqtt_transport)
	target_link_libraries(iothubdeviceping iothub_client_mqtt_transport)
	linkMqttLibrary(iothubdeviceping_static)
	linkMqttLibrary(iothubdeviceping)
else()
	add_definitions(-DIOTHUBMODULE_NULL_MQTT)
endif()

linkSharedUtil(iothubdeviceping)
linkSharedUtil(iothubdeviceping_static)

add_module_to_solution(iothubdeviceping)

#add_subdirectory(tests)

if(install_executables)
	install(TARGETS iothubdeviceping LIBRARY DESTINATION lib)
    install(TARGETS iothubdeviceping_hl LIBRARY DESTINATION lib)
endif()
