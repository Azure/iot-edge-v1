#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

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

set(node_target_dll $ENV{NODE_LIB}/node.dll CACHE INTERNAL "The location of node.dll (windows)" FORCE)

function(copy_node_dll whatIsBuilding whatIsBuildingLocation)
  if(WIN32)
  add_custom_command(TARGET ${whatIsBuilding} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${node_target_dll}
          ${whatIsBuildingLocation})
  endif()
endfunction(copy_node_dll)

# build nodejs_binding module as a dynamic library
add_library(nodejs_binding MODULE ${nodejs_sources} ${nodejs_headers})
target_link_libraries(nodejs_binding ${LIBS})
linkSharedUtil(nodejs_binding)
add_pic_flag(nodejs_binding)
target_include_directories(nodejs_binding PUBLIC $ENV{NODE_INCLUDE})

# build nodejs_binding as a static library
add_library(nodejs_binding_static STATIC ${nodejs_static_sources} ${nodejs_static_headers})
target_compile_definitions(nodejs_binding_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(nodejs_binding_static ${LIBS})
linkSharedUtil(nodejs_binding_static)
add_pic_flag(nodejs_binding_static)
target_include_directories(nodejs_binding_static PUBLIC $ENV{NODE_INCLUDE})

add_binding_to_solution(nodejs_binding)
add_binding_to_solution(nodejs_binding_static)

if(${run_unittests})
    add_subdirectory(tests)
endif()

if(install_modules)
    install(TARGETS nodejs_binding LIBRARY DESTINATION "${LIB_INSTALL_DIR}/modules")
endif()
