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

set(ble_printer_sources
    ./src/ble_printer.c
)
set(ble_printer_headers
    ./inc/ble_printer.h
)

include_directories(
    ../../../modules/ble/inc
    ../../../modules/common
    ./inc
    ${GW_INC}
)
set(LIBS ${LIBS} gateway m)

add_library(ble_printer MODULE ${ble_printer_sources} ${ble_printer_headers})
target_link_libraries(ble_printer ${LIBS})
linkSharedUtil(ble_printer)
