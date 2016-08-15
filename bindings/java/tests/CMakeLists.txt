#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for logger tests folder

add_definitions(-DUNDER_TEST)

add_subdirectory(host_ut)
add_subdirectory(host_hl_ut)
add_subdirectory(host_manager_ut)
