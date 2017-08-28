#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

include("../gatewayFunctions.cmake")

###############################################################################
###########################Find/Install/Build uamqp############################
###############################################################################
findAndInstall(uamqp 1.0.25 ${PROJECT_SOURCE_DIR}/deps/uamqp ${PROJECT_SOURCE_DIR}/deps/uamqp -Duse_installed_dependencies=ON -G "${CMAKE_GENERATOR}")

###############################################################################
###########################Find/Install/Build umqtt############################
###############################################################################
findAndInstall(umqtt 1.0.25 ${PROJECT_SOURCE_DIR}/deps/umqtt ${PROJECT_SOURCE_DIR}/deps/umqtt -Duse_installed_dependencies=ON -G "${CMAKE_GENERATOR}")

###############################################################################
#######################Find/Install/Build azure_iot_sdks#######################
###############################################################################
#The azure_iot_sdks repo requires special treatment. Parson submodule must be initialized.
if(NOT EXISTS ${PROJECT_SOURCE_DIR}/deps/iot-sdk-c/deps/parson/README.md)
    execute_process(
        COMMAND git submodule update --init ${PROJECT_SOURCE_DIR}/deps/iot-sdk-c
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        RESULT_VARIABLE res
    
    )
    if(${res})
        message(FATAL_ERROR "Error pulling iot-sdk-c submodule: ${res}")
    endif()
    execute_process(
        COMMAND git submodule update --init deps/parson
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/deps/iot-sdk-c
        RESULT_VARIABLE res
    )
    if(${res})
        message(FATAL_ERROR "Error pulling parson submodule: ${res}")
    endif()
endif()
findAndInstall(azure_iot_sdks 1.1.5 ${PROJECT_SOURCE_DIR}/deps/iot-sdk-c ${PROJECT_SOURCE_DIR}/deps/iot-sdk-c -Duse_installed_dependencies=ON -Duse_openssl=OFF -Dbuild_as_dynamic=ON -Dskip_samples=ON -G "${CMAKE_GENERATOR}")
