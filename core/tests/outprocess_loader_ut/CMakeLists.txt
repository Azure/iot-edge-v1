#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

compileAsC11()

set(theseTestsName outprocess_loader_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/module_loaders/outprocess_loader.c
    ./real_strings.c
)

set(${theseTestsName}_h_files
    ./real_strings.h
)

include_directories(${GW_INC})

build_c_test_artifacts(${theseTestsName} ON "tests/UnitTests")
