#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

# unit tests should always pretend nanomsg is statically linked.
add_definitions (-DNN_STATIC_LIB)

compileAsC99()
set(theseTestsName broker_ut)
set(${theseTestsName}_cpp_files
${theseTestsName}.cpp
)

set(${theseTestsName}_c_files
    ../../src/broker.c
)

set(${theseTestsName}_h_files
)

include_directories(${GW_INC})
include_directories(${NANOMSG_INCLUDES})

build_test_artifacts(${theseTestsName} ON)
