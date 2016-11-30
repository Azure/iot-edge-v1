#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

set(iothub_sources
    ./src/iothub.c
    ./src/null_protocol.c
)

set(iothub_headers
    ./inc/iothub.h
)

include_directories(./inc)
include_directories(${GW_INC})
include_directories(${IOTHUB_CLIENT_INC_FOLDER})

#this builds the iothub dynamic library
add_library(iothub MODULE
    ${iothub_sources}
    ${iothub_headers}
)
target_link_libraries(iothub
    gateway
    iothub_client
)

#this builds the iothub static library
add_library(iothub_static STATIC
    ${iothub_sources}
    ${iothub_headers}
)
target_compile_definitions(iothub_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(iothub_static
    gateway
    iothub_client
)

if(${use_amqp})
    target_link_libraries(iothub_static iothub_client_amqp_transport)
    target_link_libraries(iothub iothub_client_amqp_transport)
    linkUAMQP(iothub_static)
    linkUAMQP(iothub)
else()
    add_definitions(-DIOTHUBMODULE_NULL_AMQP)
endif()

if(${use_http})
    target_link_libraries(iothub_static iothub_client_http_transport)
    target_link_libraries(iothub iothub_client_http_transport)
    linkHttp(iothub_static)
    linkHttp(iothub)
else()
    add_definitions(-DIOTHUBMODULE_NULL_HTTP)
endif()

if(${use_mqtt})
    target_link_libraries(iothub_static iothub_client_mqtt_transport)
    target_link_libraries(iothub iothub_client_mqtt_transport)
    linkMqttLibrary(iothub_static)
    linkMqttLibrary(iothub)
else()
    add_definitions(-DIOTHUBMODULE_NULL_MQTT)
endif()

linkSharedUtil(iothub)
linkSharedUtil(iothub_static)

add_module_to_solution(iothub)

if(${run_unittests})
	add_subdirectory(tests)
endif()

if(install_modules)
    install(TARGETS iothub LIBRARY DESTINATION "${LIB_INSTALL_DIR}/modules") 
endif()
