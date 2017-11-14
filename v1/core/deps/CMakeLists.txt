#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
cmake_policy(SET CMP0022 NEW)

remove_definitions(-D_CRT_SECURE_NO_WARNINGS)

set(parson_c_sources
    ../../deps/parson/parson.c
)

set(parson_h_sources
    ../../deps/parson/parson.h
)

include_directories(../../deps/parson)

remove_definitions(-D_CRT_SECURE_NO_WARNINGS) 

add_library(parson
    ${parson_c_sources}
    ${parson_h_sources}
)

install(TARGETS parson EXPORT gatewayTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/../bin
)