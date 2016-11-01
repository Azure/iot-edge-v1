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

if(${use_amqp})
    target_link_libraries(iothubdeviceping iothub_client_amqp_transport)
    linkUAMQP(iothubdeviceping)
else()
    message(FATAL_ERROR "FATAL_ERROR: use_amqp not set")
endif()

if(${use_mqtt})
    target_link_libraries(iothubdeviceping iothub_client_mqtt_transport)
    linkMqttLibrary(iothubdeviceping)
else()
    message(FATAL_ERROR "FATAL_ERROR: use_mqtt not set")
endif()

if(${use_http})
    target_link_libraries(iothubdeviceping iothub_client_http_transport)
    linkHttp(iothubdeviceping)
else()
    message(FATAL_ERROR "FATAL_ERROR: use_http not set")
endif()

linkSharedUtil(iothubdeviceping)
