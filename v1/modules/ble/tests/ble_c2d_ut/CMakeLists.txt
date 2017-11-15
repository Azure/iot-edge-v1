#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

compileAsC99()
set(theseTestsName ble_c2d_ut)

set(${theseTestsName}_cpp_files
    ${theseTestsName}.cpp
)

set(ble_c2d_test_sources
    ../../src/ble_utils.c
    ../../src/ble_instr_utils.c
    ../../src/ble_c2d.c
)

if(LINUX)
    # Include GIO headers/libs
    include_directories(${GIOUNIX_INCLUDE_DIRS})
    set(LIBS ${GIOUNIX_LIBRARIES})
endif()

set(ble_c2d_test_headers
    ../../inc/ble_utils.h
    ../../inc/ble_instr_utils.h
    ../../inc/ble_c2d.h
)

include_directories(
    ../../inc
    ${GW_SRC}
    ${GW_INC}
)

set(${theseTestsName}_c_files
    ${ble_c2d_test_sources}
)

set(${theseTestsName}_h_files
    ${ble_c2d_test_headers}
)

build_test_artifacts(${theseTestsName} ON ${LIBS})
