#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

# add the proxy_gateway include/lib folders
include_directories(./inc)
include_directories(../../message/inc)
include_directories(${GW_INC})

# proxy_gateway sources and headers
set(proxy_gateway_sources
    ./src/proxy_gateway.c
    ../../../core/src/message.c
    ../../message/src/control_message.c
)
set(proxy_gateway_headers
    ./inc/proxy_gateway.h
    ../../../core/inc/message.h
    ../../message/inc/control_message.h
)

# this builds the proxy_gateway dynamic library
add_library(proxy_gateway ${proxy_gateway_sources} ${proxy_gateway_headers})
link_broker(proxy_gateway)
linkSharedUtil(proxy_gateway)

set_target_properties(proxy_gateway PROPERTIES FOLDER "Proxy/Gateway")

#install(FILES ./inc/proxy_gateway.h DESTINATION ${AIG_HEADER_INSTALL_PREFIX})

#if(${run_unittests})
#    add_subdirectory(tests)
#endif()
