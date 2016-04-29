#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for ble tests folder

add_subdirectory(ble_gatt_io_unittests)
add_subdirectory(bleio_seq_unittests)
add_subdirectory(ble_unittests)
add_subdirectory(ble_hl_unittests)

# add the gio_async_seq unit tests only for Linux
if(LINUX)
    add_subdirectory(gio_async_seq_unittests)
endif()