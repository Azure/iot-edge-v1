#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

################################################
# Include GIO headers/libs
################################################
find_package(PkgConfig REQUIRED)
pkg_search_module(GWGIOUNIX REQUIRED gio-unix-2.0)

include_directories(${GWGIOUNIX_INCLUDE_DIRS})
set(LIBS ${GWGIOUNIX_LIBRARIES})

set(filter_sources
    ./src/filter.c
)
set(filter_headers
    ./inc/filter.h
)

include_directories(./inc)
include_directories(%{GW_INC})

set(LIBS ${LIBS} gateway m)

add_library(filter MODULE ${filter_sources} ${filter_headers})
target_link_libraries(filter ${LIBS})
linkSharedUtil(filter)
