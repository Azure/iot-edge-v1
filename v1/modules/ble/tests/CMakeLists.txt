#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

add_subdirectory(ble_c2d_ut)
add_subdirectory(ble_ut)
add_subdirectory(bleio_seq_ut)
add_subdirectory(gatt_io_ut)

# add the gio_async_seq unit tests only for Linux
if(LINUX)
    add_subdirectory(gio_async_seq_ut)
endif()