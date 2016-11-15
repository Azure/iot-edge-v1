#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

compileAsC99()
set(testSuiteName dynamic_library_ut)

#setting the dynamic_loader file based on OS that it is used
if(WIN32)
    set(${testSuiteName}_cpp_files
        ${testSuiteName}_windows.cpp)
    set(${testSuiteName}_c_files
        ../../adapters/dynamic_library_windows.c)
elseif(LINUX)
    set(${testSuiteName}_cpp_files
        ${testSuiteName}_linux.cpp)
    set(${testSuiteName}_c_files
        ../../adapters/dynamic_library_linux.c)
endif()

set(${testSuiteName}_h_files
    ../../inc/dynamic_library.h)

include_directories(${GW_INC})

add_definitions(-DGB_LIBRARY_INTERCEPT)

build_test_artifacts(${testSuiteName} ON)