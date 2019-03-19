#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

include("../../../gatewayFunctions.cmake")

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
findAndInstall(azure_iot_sdks 1.1.5 ${PROJECT_SOURCE_DIR}/deps/iot-sdk-c ${PROJECT_SOURCE_DIR}/deps/iot-sdk-c -Duse_installed_dependencies=ON -Dbuild_as_dynamic=ON -Duse_openssl=OFF -G "${CMAKE_GENERATOR}")
