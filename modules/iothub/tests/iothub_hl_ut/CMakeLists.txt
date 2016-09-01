#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

compileAsC99()
set(theseTestsName iothub_hl_ut)
set(${theseTestsName}_cpp_files
${theseTestsName}.cpp
)

set(${theseTestsName}_c_files
	../../src/iothub_hl.c
)

set(${theseTestsName}_h_files
)

include_directories(${GW_INC} ../../inc)

build_test_artifacts(${theseTestsName} ON)