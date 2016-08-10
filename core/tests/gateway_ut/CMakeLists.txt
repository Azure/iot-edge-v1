#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

compileAsC99()
set(testSuite gateway_ut)
set(${testSuite}_cpp_files
    ${testSuite}.cpp
)

set(${testSuite}_c_files
    ../../src/gateway.c
)

set(${testSuite}_h_files
    ../../inc/gateway.h
)

include_directories(
    ${GW_INC}
    ../../parson/
)

build_test_artifacts(${testSuite} ON)