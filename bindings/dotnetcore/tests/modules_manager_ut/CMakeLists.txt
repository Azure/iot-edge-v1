#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

compileAsC99()
set(theseTestsName modules_manager_ut)

set(${theseTestsName}_cpp_files
    ${theseTestsName}.cpp
)

set(modules_manager_test_sources
    ../../src/modules_manager.cpp
)
set(modules_manager_test_headers
    ../../inc/modules_manager.h
)

include_directories(
    ../../inc
    ${GW_SRC}
    ${GW_INC}
)

set(${theseTestsName}_c_files
    ${modules_manager_test_sources}
)

set(${theseTestsName}_h_files
    ${modules_manager_test_headers}
)

add_definitions(-DUNDER_TEST)

build_test_artifacts(${theseTestsName} ON ${LIBS})
