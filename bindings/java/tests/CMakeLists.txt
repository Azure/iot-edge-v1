#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for logger tests folder

add_definitions(-DUNDER_TEST)

add_subdirectory(java_module_host_unittests)
add_subdirectory(java_module_host_hl_unittests)
add_subdirectory(java_module_host_manager_unittests)
