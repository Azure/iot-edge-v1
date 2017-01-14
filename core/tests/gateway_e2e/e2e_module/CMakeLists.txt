#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

set(e2e_module_sources
    ./src/e2e_module.c
)

set(e2e_module_headers
    ./inc/e2e_module.h
)


include_directories(./inc)
include_directories(${GW_INC})

add_library(e2e_module MODULE ${e2e_module_sources} ${e2e_module_headers})

target_link_libraries(e2e_module gateway)
linkSharedUtil(e2e_module)

