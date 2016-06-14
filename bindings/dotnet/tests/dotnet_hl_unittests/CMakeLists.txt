#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

compileAsC99()
set(theseTestsName dotnet_hl_unittests)

set(${theseTestsName}_cpp_files
    ${theseTestsName}.cpp
)

set(dotnet_hl_test_sources
    ../../src/dotnet_hl.c
)
set(dotnet_hl_test_headers
    ../../inc/dotnet_hl.h
)

include_directories(
    ../../inc
    ${GW_SRC}
    ${GW_INC}
)

set(${theseTestsName}_c_files
    ${dotnet_hl_test_sources}
)

set(${theseTestsName}_h_files
    ${dotnet_hl_test_headers}
)

build_test_artifacts(${theseTestsName} ON ${LIBS})
