#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

# Check if the environment variable JAVA_HOME exists
if(
    (NOT (DEFINED ENV{JAVA_HOME}))
  )
  message(FATAL_ERROR "Environment variable JAVA_HOME is not defined. Please define "
                      "JAVA_HOME to point to the location of the JDK.")
endif()

# Check if the local or environment variable JDK_ARCH exists and if not set it to 'amd64' by default
if(
    (NOT (DEFINED JDK_ARCH))
  )
  if( DEFINED ENV{JDK_ARCH} )
    set( JDK_ARCH $ENV{JDK_ARCH} )
  else()
    set( JDK_ARCH amd64 )
  endif()
endif()

set(java_module_host_manager_sources
    ./src/java_module_host_manager.c
)

set(java_module_host_manager_headers
    ./inc/java_module_host_manager.h
)

set(java_module_host_sources
    ./src/java_module_host.c
    ${java_module_host_manager_sources}
)

set(java_module_host_headers
    ./inc/broker_proxy.h
    ./inc/java_module_host_common.h
    ./inc/java_module_host.h
    ${java_module_host_manager_headers}
)

if(WIN32)
    set(java_include_dirs
        $ENV{JAVA_HOME}/include 
        $ENV{JAVA_HOME}/include/win32
    )
    set(java_link_dirs
        $ENV{JAVA_HOME}/lib
        $ENV{JAVA_HOME}/jre/bin/server
    )
    set(java_libs
        $ENV{JAVA_HOME}/lib/jvm.lib
    )
elseif(LINUX)
    set(java_include_dirs
        $ENV{JAVA_HOME}/include 
        $ENV{JAVA_HOME}/include/linux
    )
    set(java_link_dirs
        $ENV{JAVA_HOME}/jre/lib/${JDK_ARCH}/server
    )
    set(java_libs
        $ENV{JAVA_HOME}/jre/lib/${JDK_ARCH}/server/libjvm.so
    )
endif()

set(LIBS ${java_libs} gateway)

include_directories(./inc ${IOTHUB_CLIENT_INC_FOLDER})
include_directories(${GW_INC})
include_directories(${java_include_dirs})
link_directories(${java_link_dirs})

#this builds the java_module_host dynamic library
add_library(java_module_host MODULE ${java_module_host_headers} ${java_module_host_sources})
target_link_libraries(java_module_host ${LIBS})

#this build the java_module_host static library
add_library(java_module_host_static STATIC ${java_module_host_headers} ${java_module_host_sources})
target_compile_definitions(java_module_host_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(java_module_host_static ${LIBS})

linkSharedUtil(java_module_host)
linkSharedUtil(java_module_host_static)

add_binding_to_solution(java_module_host)
add_binding_to_solution(java_module_host_static)

if(${run_unittests})
    add_subdirectory(tests)
endif()

if(install_modules)
    install(TARGETS java_module_host LIBRARY DESTINATION "${LIB_INSTALL_DIR}/modules")
endif()
