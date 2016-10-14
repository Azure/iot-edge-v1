#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

include("gatewayFunctions.cmake")

###############################################################################
############################Find/Install/Build ctest###########################
###############################################################################
findAndInstall(ctest ${PROJECT_SOURCE_DIR}/deps/ctest ${PROJECT_SOURCE_DIR}/deps/ctest)

###############################################################################
#########################Find/Install/Build testrunner#########################
###############################################################################
findAndInstall(testrunnerswitcher ${PROJECT_SOURCE_DIR}/deps/testrunner ${PROJECT_SOURCE_DIR}/deps/testrunner)

###############################################################################
###########################Find/Install/Build umock############################
###############################################################################
findAndInstall(umock_c ${PROJECT_SOURCE_DIR}/deps/umock-c ${PROJECT_SOURCE_DIR}/deps/umock-c -Duse_installed_dependencies=ON -Dskip_unittests=ON)