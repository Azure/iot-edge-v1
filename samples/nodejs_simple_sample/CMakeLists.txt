#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

set(nodejs_simple_sample_sources
  ./src/main.c
)
set(nodejs_simple_sample_headers
)

include_directories(
  ../../core/inc
  ${GW_INC}
)
set(LIBS ${LIBS} gateway)

add_executable(nodejs_simple_sample ${nodejs_simple_sample_headers} ${nodejs_simple_sample_sources})
target_link_libraries(nodejs_simple_sample ${LIBS})

# make nodejs_simple_sample depend on other modules
add_dependencies(nodejs_simple_sample nodejs_binding_hl logger_hl)

linkSharedUtil(nodejs_simple_sample)
install_broker(nodejs_simple_sample ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )


add_sample_to_solution(nodejs_simple_sample)
