#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

# this is CMakeLists for the gio_async_seq_unittests
cmake_minimum_required(VERSION 2.8.11)

compileAsC99()
set(theseTestsName gio_async_seq_unittests)

set(${theseTestsName}_cpp_files
    ${theseTestsName}.cpp
)

# Include GIO headers/libs
include_directories(${GWGIOUNIX_INCLUDE_DIRS})
set(LIBS ${GWGIOUNIX_LIBRARIES})

include_directories(
    ../../inc
    ${GW_INC}
)

set(${theseTestsName}_c_files
    ../../src/gio_async_seq.c
)

set(${theseTestsName}_h_files
    ../../inc/gio_async_seq.h
)

if(WIN32)
    build_test_artifacts(${theseTestsName} ON)
else()
    build_test_artifacts(${theseTestsName} ON ADDITIONAL_LIBS ${LIBS})
endif()
