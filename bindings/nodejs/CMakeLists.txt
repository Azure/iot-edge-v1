#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

# Check if the environment variables NODE_INCLUDE and NODE_LIB exist
if(
    (NOT (DEFINED ENV{NODE_INCLUDE}))
    OR
    (NOT (DEFINED ENV{NODE_LIB}))
  )
  message(FATAL_ERROR "Environment variables NODE_INCLUDE and NODE_LIB are not "
                      "defined. Please define NODE_INCLUDE to point to the "
                      "location where the Node JS include files reside and "
                      "NODE_LIB to point to the location where the Node JS "
                      "library files reside. If you haven't built Node JS yet, "
                      "then please run tools/build_nodejs.sh or tools/build_nodejs.cmd "
                      "as appropriate.")
endif()

# add the node include/lib folders
include_directories($ENV{NODE_INCLUDE})
link_directories($ENV{NODE_LIB})

if(${LINUX})
    set(node_libs
        v8_base
        pthread
        dl
        uv
        v8_nosnapshot
        openssl
    )
endif()

if(WIN32)
    set(node_libs
    )
endif()

set(node_libs
    ${node_libs}
    node
    v8_libbase
    v8_libplatform
)

# app sources
include_directories(../../modules/common)
include_directories(./inc)

# Node JS binding DLL/SO sources and headers
set(nodejs_sources
    ./src/nodejs.cpp
    ./src/modules_manager.cpp
)
set(nodejs_headers
    ./inc/lock.h
    ./inc/nodejs_common.h
    ./inc/nodejs.h
    ./inc/modules_manager.h
)

# Node JS binding static lib sources and headers
set(nodejs_static_sources ${nodejs_sources})
set(nodejs_static_headers ${nodejs_headers})

# Node JS binding HL DLL/SO sources and headers
set(nodejs_hl_sources
    ./src/nodejs_hl.cpp
)
set(nodejs_hl_headers
    ./inc/nodejs_hl.h
)

# Node JS binding HL static lib sources and headers
set(nodejs_hl_static_sources
    ${nodejs_hl_sources}
)
set(nodejs_hl_static_headers
    ${nodejs_hl_headers}
)

if(${LINUX})
    set(LIBS ${node_libs})
endif()

if(WIN32)
    # prefix the lib names with the path to debug and release bits
    set(LIBS "")
    foreach(lib ${node_libs})
        list(APPEND LIBS ${lib}.lib)
    endforeach()

    set(LIBS
        ${LIBS}
        Ws2_32.lib
        Iphlpapi.lib
        Psapi.lib
        winmm.lib
        userenv.lib
    )
endif()

set(LIBS ${LIBS} gateway)

# build nodejs_binding module as a dynamic library
add_library(nodejs_binding MODULE ${nodejs_sources} ${nodejs_headers})
target_link_libraries(nodejs_binding ${LIBS})
linkSharedUtil(nodejs_binding)

# build nodejs_binding as a static library
add_library(nodejs_binding_static ${nodejs_static_sources} ${nodejs_static_headers})
target_compile_definitions(nodejs_binding_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(nodejs_binding_static ${LIBS})
linkSharedUtil(nodejs_binding_static)

#this builds the nodejs_binding_hl dynamic library
add_library(nodejs_binding_hl MODULE ${nodejs_hl_sources} ${nodejs_hl_headers})
target_link_libraries(nodejs_binding_hl nodejs_binding_static ${LIBS})
linkSharedUtil(nodejs_binding_hl)

#this builds the nodejs_binding_hl static library
add_library(nodejs_binding_hl_static ${nodejs_hl_static_sources} ${nodejs_hl_static_headers})
target_compile_definitions(nodejs_binding_hl_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(nodejs_binding_hl_static nodejs_binding_static ${LIBS})
linkSharedUtil(nodejs_binding_hl_static)

add_module_to_solution(nodejs_binding)
