#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for logger_unittests
cmake_minimum_required(VERSION 2.8.11)

compileAsC99()
set(theseTestsName logger_unittests)
set(${theseTestsName}_cpp_files
${theseTestsName}.cpp
)

set(${theseTestsName}_c_files
	../../src/logger.c
)

set(${theseTestsName}_h_files
)

include_directories(${GW_INC} ../../inc)

add_definitions(-DGB_STDIO_INTERCEPT -DGB_TIME_INTERCEPT -DNO_LOGGING)

build_test_artifacts(${theseTestsName} ON)
