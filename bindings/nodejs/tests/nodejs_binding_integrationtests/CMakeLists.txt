#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for logger_unittests
cmake_minimum_required(VERSION 2.8.11)

set(theseTestsName nodejs_binding_integrationtests)

set(${theseTestsName}_cpp_files
    ${theseTestsName}.cpp
    ../../src/nodejs.cpp
    ../../src/nodejs_utils.cpp
    ../../src/nodejs_idle.cpp
    ../../src/modules_manager.cpp
)

set(${theseTestsName}_c_files)

set(${theseTestsName}_h_files
    ../../inc/lock.h
    ../../inc/nodejs_common.h
    ../../inc/nodejs_utils.h
    ../../inc/nodejs_idle.h
    ../../inc/nodejs.h
    ../../inc/modules_manager.h
)

build_test_artifacts(${theseTestsName} ON)

target_include_directories(
    ${theseTestsName}_exe PUBLIC
    ${GW_INC}
    ../../inc
    ../../../modules/common
    $ENV{NODE_INCLUDE}
)
target_link_libraries(${theseTestsName}_exe ${LIBS})
target_include_directories(nodejs_binding_static PUBLIC $ENV{NODE_INCLUDE})
install_broker(${theseTestsName}_exe ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )

if(WIN32)
    target_include_directories(
        ${theseTestsName}_dll PUBLIC
        ${GW_INC}
        ../../inc
        ../../../modules/common
        $ENV{NODE_INCLUDE}
    )
    target_link_libraries(${theseTestsName}_dll ${LIBS})
endif()