#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for ble_unittests
cmake_minimum_required(VERSION 2.8.11)

compileAsC99()
set(theseTestsName ble_unittests)

if(LINUX)
    set(${theseTestsName}_cpp_files
        ${theseTestsName}_linux.cpp
    )

    # Include GIO headers/libs
    include_directories(${GIOUNIX_INCLUDE_DIRS})
    set(LIBS ${GIOUNIX_LIBRARIES})
    
    # BLE GATT I/O sources
    set(ble_test_sources
        ../../src/ble_utils.c
        ../../src/ble.c
    )
    set(ble_test_headers
       ../../inc/gio_async_seq.h
       ../../inc/ble_utils.h
       ../../inc/ble_gatt_io_linux_common.h
    )
elseif(WIN32)
    set(${theseTestsName}_cpp_files
        ${theseTestsName}_windows.cpp
    )

    set(ble_test_sources
        ../../src/ble.c
        ../../src/ble_utils.c
    )
endif()

set(ble_test_headers
    ${ble_test_headers}
    ../../inc/ble_gatt_io.h
    ../../inc/ble_utils.h
    ../../inc/ble.h
)

include_directories(
    ../../inc
    ${GW_SRC}
    ${GW_INC}
)

set(${theseTestsName}_c_files
    ${ble_test_sources}
)

set(${theseTestsName}_h_files
    ${ble_test_headers}
)

add_definitions(-DGB_TIME_INTERCEPT -DNO_LOGGING)



build_test_artifacts(${theseTestsName} ON ${LIBS})

if(WIN32)
	if(TARGET ${theseTestsName}_dll)
		target_link_libraries(${theseTestsName}_dll
			gateway
			rpcrt4
		)
	endif()
	
	if(TARGET ${theseTestsName}_exe)
		target_link_libraries(${theseTestsName}_exe
			gateway
			rpcrt4
		)	
        install_broker(${theseTestsName}_exe ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )
	endif()
endif()
