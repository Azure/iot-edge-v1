#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

compileAsC99()
set(testSuiteName module_loader_ut)
set(${testSuiteName}_cpp_files
${testSuiteName}.cpp
)

set(${testSuiteName}_c_files
../../src/module_loader.c
)

set(${testSuiteName}_h_files
../../inc/module_loader.h
../../inc/dynamic_library.h

)

include_directories(${GW_INC})

build_test_artifacts(${testSuiteName} ON)