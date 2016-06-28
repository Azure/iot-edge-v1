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
link_directories($ENV{NODE_LIB})

if(${LINUX})
    set(v8_libs
        v8_base
        v8_nosnapshot
    )
    set(node_libs
        pthread
        dl
        uv
        openssl
    )
endif()

set(v8_libs
    ${v8_libs}
    v8_libbase
    v8_libplatform
)

set(node_libs
    ${node_libs}
    ${v8_libs}
    node
)

# app sources
include_directories(../../modules/common)
include_directories(./inc)

# Node JS binding DLL/SO sources and headers
set(nodejs_sources
    ./src/nodejs.cpp
    ./src/nodejs_idle.cpp
    ./src/nodejs_utils.cpp
    ./src/modules_manager.cpp
)
set(nodejs_headers
    ./inc/lock.h
    ./inc/nodejs_common.h
    ./inc/nodejs.h
    ./inc/nodejs_idle.h
    ./inc/nodejs_utils.h
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
    # suffix the lib names with '.lib'
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

function(add_pic_flag target)
    if(${LINUX})
        # need the -fPIC flag on Linux; see:
        #   https://cmake.org/pipermail/cmake/2007-May/014350.html
        if( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
            set_target_properties(${target} PROPERTIES COMPILE_FLAGS "-fPIC")
        endif( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
    endif()
endfunction()

# build nodejs_binding module as a dynamic library
add_library(nodejs_binding MODULE ${nodejs_sources} ${nodejs_headers})
target_link_libraries(nodejs_binding ${LIBS})
linkSharedUtil(nodejs_binding)
add_pic_flag(nodejs_binding)
target_include_directories(nodejs_binding PUBLIC $ENV{NODE_INCLUDE})

# build nodejs_binding as a static library
add_library(nodejs_binding_static ${nodejs_static_sources} ${nodejs_static_headers})
target_compile_definitions(nodejs_binding_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(nodejs_binding_static ${LIBS})
linkSharedUtil(nodejs_binding_static)
add_pic_flag(nodejs_binding_static)
target_include_directories(nodejs_binding_static PUBLIC $ENV{NODE_INCLUDE})

#this builds the nodejs_binding_hl dynamic library
add_library(nodejs_binding_hl MODULE ${nodejs_hl_sources} ${nodejs_hl_headers})
target_link_libraries(nodejs_binding_hl nodejs_binding_static ${LIBS})
linkSharedUtil(nodejs_binding_hl)
add_pic_flag(nodejs_binding_hl)
target_include_directories(nodejs_binding_hl PUBLIC $ENV{NODE_INCLUDE})

#this builds the nodejs_binding_hl static library
add_library(nodejs_binding_hl_static ${nodejs_hl_static_sources} ${nodejs_hl_static_headers})
target_compile_definitions(nodejs_binding_hl_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(nodejs_binding_hl_static nodejs_binding_static ${LIBS})
linkSharedUtil(nodejs_binding_hl_static)
add_pic_flag(nodejs_binding_hl_static)
target_include_directories(nodejs_binding_hl_static PUBLIC $ENV{NODE_INCLUDE})

add_module_to_solution(nodejs_binding)

add_subdirectory(tests)
