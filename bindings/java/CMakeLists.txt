#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for java binding

# Check if the environment variables NODE_INCLUDE and NODE_LIB exist
if(
    (NOT (DEFINED ENV{JAVA_HOME}))
  )
  message(FATAL_ERROR "Environment variable JAVA_HOME is not defined. Please define "
                      "JAVA_HOME to point to the location of the JDK.")
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
    ./inc/message_bus_proxy.h
    ./inc/java_module_host_common.h
    ./inc/java_module_host.h
    ${java_module_host_manager_headers}
)

set(java_module_host_static_sources
    ${java_module_host_sources}
)

set(java_module_host_static_headers
    ${java_module_host_headers}
)

set(java_module_host_hl_sources
    ./src/java_module_host_hl.c
)

set(java_module_host_hl_headers
    ./inc/message_bus_proxy.h
    ./inc/java_module_host_hl.h
)

set(java_module_host_hl_static_sources
    ${java_module_host_hl_sources}
)

set(java_module_host_hl_static_headers
    ${java_module_host_hl_headers}
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
        $ENV{JAVA_HOME}/jre/lib/amd64/server
    )
    set(java_libs
        $ENV{JAVA_HOME}/jre/lib/amd64/server/libjvm.so
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
add_library(java_module_host_static ${java_module_host_static_headers} ${java_module_host_static_sources})
target_compile_definitions(java_module_host_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(java_module_host_static ${LIBS})

#this builds the java_module_host_hl dynamic library (by default it uses java_module_host linked statically)
add_library(java_module_host_hl MODULE ${java_module_host_hl_headers} ${java_module_host_hl_sources})
target_link_libraries(java_module_host_hl java_module_host_static ${LIBS})

#this builds the java_module_host_hl static library (by default it uses the java_module_host linked statically)
add_library(java_module_host_hl_static ${java_module_host_hl_static_headers} ${java_module_host_hl_static_sources})
target_compile_definitions(java_module_host_hl_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(java_module_host_hl_static java_module_host_static ${LIBS})

linkSharedUtil(java_module_host)
linkSharedUtil(java_module_host_static)
linkSharedUtil(java_module_host_hl)
linkSharedUtil(java_module_host_hl_static)

add_module_to_solution(java_module_host)

add_subdirectory(tests)

if(install_executables)
    install(TARGETS java_module_host LIBRARY DESTINATION lib)
    install(TARGETS java_module_host_hl LIBRARY DESTINATION lib)
endif()