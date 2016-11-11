#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

include("../../../gatewayFunctions.cmake")

###############################################################################
###########################Find/Install/Build uamqp############################
###############################################################################
findAndInstall(uamqp ${PROJECT_SOURCE_DIR}/deps/uamqp ${PROJECT_SOURCE_DIR}/deps/uamqp -Duse_installed_dependencies=ON -Dskip_unittests=ON)

###############################################################################
###########################Find/Install/Build umqtt############################
###############################################################################
findAndInstall(umqtt ${PROJECT_SOURCE_DIR}/deps/umqtt ${PROJECT_SOURCE_DIR}/deps/umqtt -Duse_installed_dependencies=ON -Dskip_unittests=ON)

###############################################################################
#######################Find/Install/Build azure_iot_sdks#######################
###############################################################################
#The azure_iot_sdks repo requires special treatment. Parson submodule must be initialized.
if(NOT EXISTS ${PROJECT_SOURCE_DIR}/deps/iot-sdk/c/parson/README.md)
    execute_process(
        COMMAND git submodule update --init c/parson
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/deps/iot-sdk
        RESULT_VARIABLE res
    )
    if(${res})
        message(FATAL_ERROR "Error pulling parson submodule: ${res}")
    endif()
endif()
findAndInstall(azure_iot_sdks ${PROJECT_SOURCE_DIR}/deps/iot-sdk ${PROJECT_SOURCE_DIR}/deps/iot-sdk/c -Duse_installed_dependencies=ON -Drun_e2e_tests=ON -Duse_openssl=OFF -Dskip_unittests=ON)
