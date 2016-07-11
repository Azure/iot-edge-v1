#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for bleio_seq_unittests
cmake_minimum_required(VERSION 2.8.11)

compileAsC99()
set(theseTestsName bleio_seq_unittests)

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
    set(bleio_seq_test_sources
        ../../src/bleio_seq_linux.c
        ../../src/bleio_seq_linux_schedule_periodic.c
        ../../src/bleio_seq_linux_schedule_read.c
        ../../src/bleio_seq_linux_schedule_write.c
    )
    set(bleio_seq_test_headers
       ${bluez_headers}
       ../../inc/gio_async_seq.h
       ../../inc/bleio_seq.h
       ../../inc/bleio_seq_linux_common.h
    )
elseif(WIN32)
    set(${theseTestsName}_cpp_files
        ${theseTestsName}_windows.cpp
    )

    set(bleio_seq_test_sources
        ../../src/bleio_seq_windows.c
    )
endif()

set(bleio_seq_test_headers
    ${bleio_seq_test_headers}
    ../../inc/ble_gatt_io.h
)

include_directories(
    ../../inc
    ${GW_INC}
)

set(${theseTestsName}_c_files
    ${bleio_seq_test_sources}
)

set(${theseTestsName}_h_files
    ${bleio_seq_test_headers}
)

if(WIN32)
    build_test_artifacts(${theseTestsName} ON)
else()
    build_test_artifacts(${theseTestsName} ON ADDITIONAL_LIBS ${LIBS})
endif()

