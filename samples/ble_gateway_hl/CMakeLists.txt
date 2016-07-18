#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

################################################
# Include GIO headers/libs
################################################
find_package(PkgConfig REQUIRED)
pkg_search_module(GWGIOUNIX REQUIRED gio-unix-2.0)

include_directories(${GWGIOUNIX_INCLUDE_DIRS})
set(LIBS ${GWGIOUNIX_LIBRARIES})

set(ble_gateway_hl_sources
  ./src/main.c
)
set(ble_gateway_hl_headers
)

include_directories(
  ../../inc
  ${GW_INC}
)
set(LIBS ${LIBS} gateway)

# Raspberry Pi 3 doesn't see 'signal' types unless this symbol is defined
add_definitions(-D_POSIX_C_SOURCE=1)

add_executable(ble_gateway_hl ${ble_gateway_hl_headers} ${ble_gateway_hl_sources})
target_link_libraries(ble_gateway_hl ${LIBS})

# make ble_gateway_hl depend on the ble_hl module; since this module
# is a runtime dependency, changes to code in ble_hl will not get built
# when building the 'ble_gateway_hl' target; this explicit dependency
# declaration will cause a build 
add_dependencies(ble_gateway_hl ble_hl ble_printer identity_map_hl logger_hl iothubhttp_hl)

linkSharedUtil(ble_gateway_hl)
install_broker(ble_gateway_hl ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )


add_sample_to_solution(ble_gateway_hl)

if(LINUX)
  add_subdirectory(ble_printer)
endif()
