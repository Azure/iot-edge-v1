#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

set(PROXY_MODULES_DIR ${CMAKE_CURRENT_LIST_DIR} CACHE INTERNAL "Proxy Modules include directory" FORCE)

if(${enable_native_remote_modules})
    add_subdirectory(native_module_host)
endif()
