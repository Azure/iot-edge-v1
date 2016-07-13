#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#This is the CMakeLists.txt for gateway_ll_uwp_unittests
cmake_minimum_required(VERSION 2.8.11)

add_definitions(-DUWP_BINDING)

compileAsC99()
set(testSuite gateway_ll_uwp_unittests)
set(${testSuite}_cpp_files
    ${testSuite}.cpp
)

set_source_files_properties(../../src/gateway_ll.c PROPERTIES LANGUAGE CXX) 

set(${testSuite}_c_files
    ../../src/gateway_ll.c
)

set(${testSuite}_h_files
    ../../inc/gateway_ll.h
)

include_directories(${GW_INC})

build_test_artifacts(${testSuite} ON)