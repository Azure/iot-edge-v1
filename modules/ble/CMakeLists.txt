#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)
if(POLICY CMP0054)
    cmake_policy(SET CMP0054 OLD)
endif()

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
        ./src/ble_instr_utils.c
        ./src/ble_utils.c
        ./src/ble.c
    )
    set(ble_headers
       ${bluez_headers}
       ./inc/gio_async_seq.h
       ./inc/bleio_seq.h
       ./inc/ble_gatt_io_linux_common.h
       ./inc/bleio_seq_linux_common.h
    )
elseif(WIN32)
    set(ble_sources
        ./src/ble_gatt_io_windows.c
        ./src/bleio_seq_windows.c
        ./src/ble_instr_utils.c
        ./src/ble_utils.c
        ./src/ble.c
    )
endif()

set(ble_headers
    ${ble_headers}
    ./inc/ble.h
    ./inc/ble_gatt_io.h
    ./inc/ble_utils.h
    ../common/messageproperties.h
)

set(ble_c2d_sources
    ./src/ble_c2d.c
    ./src/ble_instr_utils.c
)

set(ble_c2d_headers
    ./inc/ble_c2d.h
    ./inc/ble_instr_utils.h
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
add_library(ble_static STATIC ${ble_sources} ${ble_headers})
target_compile_definitions(ble_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(ble_static ${LIBS} iothub_service_client)
linkSharedUtil(ble_static)

# build ble C2D as a dynamic library
add_library(ble_c2d MODULE ${ble_c2d_sources} ${ble_c2d_headers})
target_link_libraries(ble_c2d ${LIBS})
linkSharedUtil(ble_c2d)

# build ble C2D as a static library
add_library(ble_c2d_static STATIC ${ble_c2d_sources} ${ble_c2d_headers})
target_compile_definitions(ble_c2d_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(ble_c2d_static ${LIBS} iothub_service_client)
linkSharedUtil(ble_c2d_static)

add_module_to_solution(ble)
add_module_to_solution(ble_c2d)

if(${run_unittests})
    add_subdirectory(tests)
endif()

if(install_modules)
    install(TARGETS ble LIBRARY DESTINATION "${LIB_INSTALL_DIR}/modules") 
endif()
