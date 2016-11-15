#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

compileAsC11()

set(theseTestsName module_loader_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/module_loader.c
    ./real_strings.c
    ./real_vector.c
)

set(${theseTestsName}_h_files
)

include_directories(${GW_INC})

# Enable all bindings since we are going to use mocks.
add_definitions(-DNODE_BINDING_ENABLED -DJAVA_BINDING_ENABLED -DDOTNET_BINDING_ENABLED)

build_c_test_artifacts(${theseTestsName} ON "tests/UnitTests")
