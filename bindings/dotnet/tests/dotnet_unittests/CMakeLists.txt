#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

compileAsC99()
set(theseTestsName dotnet_unittests)

set(${theseTestsName}_cpp_files
    ${theseTestsName}.cpp
)

set(dotnet_test_sources
    ../../src/dotnet.cpp
)
set(dotnet_test_headers
    ../../inc/dotnet.h
)

include_directories(
    ../../inc
    ${GW_SRC}
    ${GW_INC}
)

set(${theseTestsName}_c_files
    ${dotnet_test_sources}
)

set(${theseTestsName}_h_files
    ${dotnet_test_headers}
)

add_definitions(-DUNDER_TEST)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /IGNORE:4217")
set(CMAKE_SHARED_LINKER_FLAGS "$(CMAKE_SHARED_LINKER_FLAGS) /IGNORE:4217")

build_test_artifacts(${theseTestsName} ON ${LIBS})
