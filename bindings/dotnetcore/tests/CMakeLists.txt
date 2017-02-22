#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

add_definitions(-DUNDER_TEST)

add_subdirectory(dotnetcore_ut)

#Enable e2e test just on 64bits for now
if(NOT (${ARCHITECTURE} STREQUAL "x86")) 
add_subdirectory(dotnetcore_e2e)
endif()