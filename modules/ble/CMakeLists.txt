#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

# this is CMakeLists for the ble module
cmake_minimum_required(VERSION 2.8.11)

if(LINUX)
    ################################################
    # Include GIO headers/libs
    ################################################
    find_package(PkgConfig REQUIRED)
    pkg_search_module(GWGIOUNIX REQUIRED gio-unix-2.0)

    include_directories(${GWGIOUNIX_INCLUDE_DIRS})
    set(LIBS ${GWGIOUNIX_LIBRARIES})

    ################################################
    # Blue-z dbus generated sources
    ################################################
    set(bluez_sources
        ./deps/linux/dbus-bluez/src/bluez_characteristic.c
        ./deps/linux/dbus-bluez/src/bluez_device.c
    )
    set(bluez_headers
        ./deps/linux/dbus-bluez/inc/bluez_characteristic.h
        ./deps/linux/dbus-bluez/inc/bluez_device.h
    )
    include_directories(./deps/linux/dbus-bluez/inc)

    ################################################
    # BLE Module sources
    ################################################
    set(ble_sources
        ${bluez_sources}
        ./src/gio_async_seq.c
        ./src/ble_gatt_io_linux.c
        ./src/ble_gatt_io_linux_connect.c
        ./src/ble_gatt_io_linux_disconnect.c
        ./src/ble_gatt_io_linux_read.c
        ./src/ble_gatt_io_linux_write.c
        ./src/bleio_seq_linux.c
        ./src/bleio_seq_linux_schedule_write.c
        ./src/bleio_seq_linux_schedule_read.c
        ./src/bleio_seq_linux_schedule_periodic.c
        ./src/ble_utils.c
        ./src/ble.c
    )
    set(ble_headers
       ${bluez_headers}
       ./inc/gio_async_seq.h
       ./inc/bleio_seq.h
       ./inc/ble_gatt_io_linux_common.h
       ./inc/bleio_seq_linux_common.h
       ./inc/ble.h
    )
elseif(WIN32)
    set(ble_sources
        ./src/ble_gatt_io_windows.c
        ./src/bleio_seq_windows.c
        ./src/ble_utils.c
        ./src/ble.c
    )
endif()

set(ble_headers
    ${ble_headers}
    ./inc/ble_gatt_io.h
    ./inc/ble_utils.h
    ../common/messageproperties.h
)

set(ble_static_sources
    ${ble_sources}
)

set(ble_static_headers
    ${ble_headers}
)

set(ble_hl_sources
    ./src/ble_hl.c
)

set(ble_hl_headers
    ./inc/ble_hl.h
)

set(ble_hl_static_sources
    ${ble_hl_sources}
)

set(ble_hl_static_headers
    ${ble_hl_headers}
)

include_directories(
    ./inc
    ../common
    ${GW_INC}
)
set(LIBS ${LIBS} gateway)

# build ble module as a dynamic library
add_library(ble MODULE ${ble_sources} ${ble_headers})
target_link_libraries(ble ${LIBS})
linkSharedUtil(ble)

# build ble as a static library
add_library(ble_static ${ble_static_sources} ${ble_static_headers})
target_compile_definitions(ble_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(ble_static ${LIBS})
linkSharedUtil(ble_static)

# build ble HL as a dynamic library
add_library(ble_hl MODULE ${ble_hl_sources} ${ble_hl_headers})
target_link_libraries(ble_hl ble_static ${LIBS})
linkSharedUtil(ble_hl)

# build ble HL as a static library
add_library(ble_hl_static ${ble_hl_static_sources} ${ble_hl_static_headers})
target_compile_definitions(ble_hl_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(ble_hl_static ble_static ${LIBS})
linkSharedUtil(ble_hl_static)

add_module_to_solution(ble)

add_subdirectory(tests)

if(install_executables)
    install(TARGETS ble LIBRARY DESTINATION lib) 
endif()
