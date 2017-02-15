#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

set(gw_sources
  ./src/main.c
)
set(gw_headers
)

include_directories(
  ../../core/inc
  ${GW_INC}
)

set(LIBS ${LIBS} gateway)

add_executable(gw ${gw_headers} ${gw_sources})
target_link_libraries(gw ${LIBS})

# make gw depend on other modules
add_dependencies(gw nodejs_binding dotnet_binding java_binding)

linkSharedUtil(gw)
install_broker(gw ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration))
