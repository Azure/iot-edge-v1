#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for ble_gatt_io_unittests
cmake_minimum_required(VERSION 2.8.11)

compileAsC99()
set(theseTestsName ble_gatt_io_unittests)

if(LINUX)
    set(${theseTestsName}_cpp_files
        ${theseTestsName}_linux.cpp
    )

    # Include GIO headers/libs
    include_directories(${GWGIOUNIX_INCLUDE_DIRS})
    set(LIBS ${GWGIOUNIX_LIBRARIES})
    
    # Blue-z dbus generated sources
    set(bluez_headers
        ../../deps/linux/dbus-bluez/inc/bluez_characteristic.h
        ../../deps/linux/dbus-bluez/inc/bluez_device.h
    )
    include_directories(../../deps/linux/dbus-bluez/inc)
    
    # BLE GATT I/O sources
    set(ble_gatt_io_test_sources
        ../../src/ble_gatt_io_linux.c
        ../../src/ble_gatt_io_linux_connect.c
        ../../src/ble_gatt_io_linux_disconnect.c
        ../../src/ble_gatt_io_linux_read.c
        ../../src/ble_gatt_io_linux_write.c
    )
    set(ble_gatt_io_test_headers
       ${bluez_headers}
       ../../inc/gio_async_seq.h
       ../../inc/ble_gatt_io_linux_common.h
    )
elseif(WIN32)
    set(${theseTestsName}_cpp_files
        ${theseTestsName}_windows.cpp
    )

    set(ble_gatt_io_test_sources
        ../../src/ble_gatt_io_windows.c
    )
endif()

set(ble_gatt_io_test_headers
    ${ble_gatt_io_test_headers}
    ../../inc/ble_gatt_io.h
)

include_directories(
    ../../inc
    ${GW_INC}
)

set(${theseTestsName}_c_files
    ${ble_gatt_io_test_sources}
)

set(${theseTestsName}_h_files
    ${ble_gatt_io_test_headers}
)

if(WIN32)
    build_test_artifacts(${theseTestsName} ON)
else()
    build_test_artifacts(${theseTestsName} ON ADDITIONAL_LIBS ${LIBS} VALGRIND_SUPPRESSIONS_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../core/valgrind_suppressions.txt)
endif()
