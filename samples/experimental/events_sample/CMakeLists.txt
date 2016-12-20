#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

set(events_sources
    ./src/main.c
)

set(events_headers
)

include_directories(${IOTHUB_CLIENT_INC_FOLDER} ${GW_INC} ${GW_INC}/experimental)

add_executable(events_sample ${events_headers} ${events_sources})

#use static library because functions aren't public.
target_link_libraries(events_sample gateway_static)
linkSharedUtil(events_sample)

add_dependencies(events_sample hello_world)
install_broker(events_sample ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )
copy_gateway_dll(events_sample ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )
copy_iothub_client_dll(events_sample ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )

# add events_sample to non-standard "Experimental" VS project folder for now 
#add_sample_to_solution(events_sample)
set_target_properties(events_sample PROPERTIES FOLDER "Samples/Experimental")
