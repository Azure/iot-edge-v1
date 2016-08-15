#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

compileAsC99()
set(theseTestsName nodejs_hl_ut)

set(${theseTestsName}_cpp_files
    ${theseTestsName}.cpp
)

set(nodejs_binding_hl_test_sources
    ../../src/nodejs_hl.cpp
)

set(nodejs_binding_hl_test_headers
    ../../inc/nodejs_hl.h
)

include_directories(
    ../../inc
    ${GW_SRC}
    ${GW_INC}
)

set(${theseTestsName}_c_files
    ${nodejs_binding_hl_test_sources}
)

set(${theseTestsName}_h_files
    ${nodejs_binding_hl_test_headers}
)

build_test_artifacts(${theseTestsName} ON ${LIBS})
