#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

compileAsC99()
set(testSuite gateway_ll_ut)
set(${testSuite}_cpp_files
    ${testSuite}.cpp
)

set(${testSuite}_c_files
    ../../src/gateway_ll.c
)

set(${testSuite}_h_files
    ../../inc/gateway_ll.h
)

include_directories(${GW_INC})

build_test_artifacts(${testSuite} ON)