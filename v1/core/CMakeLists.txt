#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)
if(POLICY CMP0054)
    cmake_policy(SET CMP0054 OLD)
endif()
if(POLICY CMP0022)
    cmake_policy(SET CMP0022 NEW)
endif()
if(POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif()

add_subdirectory(deps)

#this sets a global var that stores where the includes for GW are
set(GW_INC ${CMAKE_CURRENT_LIST_DIR}/inc CACHE INTERNAL "Needs to be included for gateway includes" FORCE)
set(GW_SRC ${CMAKE_CURRENT_LIST_DIR}/src CACHE INTERNAL "Needs to be included for gateway sources" FORCE)

#setting the dynamic_loader file based on OS that it is used
if(WIN32)
    include_directories(${GW_INC}/windows )
elseif(UNIX) # LINUX or APPLE
    include_directories(${GW_INC}/linux)
endif()

#setting the dynamic_loader file based on OS that it is used
if(WIN32)
    set(dynamic_library_c_file ./adapters/dynamic_library_windows.c ./adapters/gb_library_windows.c)
elseif(UNIX) # LINUX or APPLE
    set(dynamic_library_c_file ./adapters/dynamic_library_linux.c ./adapters/gb_library_linux.c )
endif()

# Build libuv with an OS-appropriate script
if (${enable_core_remote_module_support})
    if(WIN32)
        if(8 EQUAL CMAKE_SIZEOF_VOID_P)
            message(STATUS "Building libuv for 64-bit Windows...")
            set(build_libuv build_libuv.cmd x64)
        else()
            message(STATUS "Building libuv for 32-bit Windows...")
            set(build_libuv build_libuv.cmd)
        endif()
    elseif(LINUX)
        message(STATUS "Building libuv for Linux...")
        set(build_libuv ./build_libuv.sh)
    elseif(APPLE)
        message(STATUS "Building libuv for macOS...")
        set(build_libuv ./build_libuv_macos.sh)
    endif()

    execute_process (
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tools/
        COMMAND ${build_libuv}
    )
endif()

#setting specific libraries to be loaded based on OS (for example, Linux needs "-ldl", windows does not)
if(UNIX) # LINUX or APPLE
    set(dynamic_loader_library dl)
endif()

function(linkHttp whatExecutableIsBuilding)
    includeHttp()
    if(WIN32)
        if(WINCE)
            target_link_libraries(${whatExecutableIsBuilding} crypt32.lib)
            target_link_libraries(${whatExecutableIsBuilding} ws2.lib)
        else()
            target_link_libraries(${whatExecutableIsBuilding} winhttp.lib)
        endif()
    else()
        target_link_libraries(${whatExecutableIsBuilding} curl)
    endif()
endfunction(linkHttp)

function(includeHttp)
endfunction(includeHttp)

#nanomsg lib check macro (temporary)
macro (nn_check_lib LIB SYM DEF)
    check_library_exists (${LIB} ${SYM} "" ${DEF})
    if (${DEF})
        add_definitions (-D${DEF}=1)
        set(NN_REQUIRED_LIBRARIES ${NN_REQUIRED_LIBRARIES} ${LIB})
    endif ()
endmacro (nn_check_lib)

set(gateway_c_sources
    ${dynamic_library_c_file}
    ./src/message.c
    ./src/message_queue.c
    ./src/module_loader.c
)

set(gateway_h_sources
    ./inc/message.h
    ./inc/module.h
    ./inc/module_access.h
    ./inc/module_loader.h
    ./inc/dynamic_library.h
    ../deps/parson/parson.h
    ./inc/experimental/event_system.h
    ./inc/gateway.h
    ./inc/gateway_export.h
    ./inc/gateway_version.h
    ./src/gateway_internal.h
    ./inc/message_queue.h
    ./inc/broker.h
)

# Add the module loaders
set(gateway_c_sources
    ${gateway_c_sources}
    ./src/module_loaders/dynamic_loader.c
)
set(gateway_h_sources
    ${gateway_h_sources}
    ./inc/module_loaders/dynamic_loader.h
)

if(${enable_dotnet_binding})
    set(gateway_c_sources
        ${gateway_c_sources}
        ./src/module_loaders/dotnet_loader.c
    )
    set(gateway_h_sources
        ${gateway_h_sources}
        ./inc/module_loaders/dotnet_loader.h
    )
    # This symbol needs to be defined to cause this module loader to be
    # available for the gateway.
    add_definitions(-DDOTNET_BINDING_ENABLED)
    include_directories(../bindings/dotnet/inc)
endif()

if(${enable_dotnet_core_binding})
    set(gateway_c_sources
        ${gateway_c_sources}
        ./src/module_loaders/dotnet_core_loader.c
    )
    set(gateway_h_sources
        ${gateway_h_sources}
        ./inc/module_loaders/dotnet_core_loader.h
    )
    # This symbol needs to be defined to cause this module loader to be
    # available for the gateway.
    add_definitions(-DDOTNET_CORE_BINDING_ENABLED)
    include_directories(../bindings/dotnetcore/inc)
endif()

if(${enable_java_binding})
    set(gateway_c_sources
        ${gateway_c_sources}
        ./src/module_loaders/java_loader.c
    )
    set(gateway_h_sources
        ${gateway_h_sources}
        ./inc/module_loaders/java_loader.h
    )
    # This symbol needs to be defined to cause this module loader to be
    # available for the gateway.
    add_definitions(-DJAVA_BINDING_ENABLED)
    include_directories(../bindings/java/inc)
endif()

if(${enable_nodejs_binding})
    set(gateway_c_sources
        ${gateway_c_sources}
        ./src/module_loaders/node_loader.c
    )
    set(gateway_h_sources
        ${gateway_h_sources}
        ./inc/module_loaders/node_loader.h
    )
    # This symbol needs to be defined to cause this module loader to be
    # available for the gateway.
    add_definitions(-DNODE_BINDING_ENABLED)
    include_directories(../bindings/nodejs/inc)
endif()

if(${enable_core_remote_module_support})
    set(gateway_c_sources
        ${gateway_c_sources}
        ../proxy/message/src/control_message.c
        ../proxy/outprocess/src/module_loaders/outprocess_loader.c
        ../proxy/outprocess/src/module_loaders/outprocess_module.c
        )

    set(gateway_h_sources
        ${gateway_h_sources}
        ../proxy/message/inc/control_message.h
        ../proxy/outprocess/inc/module_loaders/outprocess_loader.h
        ../proxy/outprocess/inc/module_loaders/outprocess_module.h
    )

    add_definitions(-DOUTPROCESS_ENABLED)
    include_directories( ../proxy/outprocess/inc)
    include_directories( ../proxy/message/inc)

    include_directories(${CMAKE_SOURCE_DIR}/build_libuv/dist/include)
    link_directories(${CMAKE_SOURCE_DIR}/build_libuv/dist/lib)
endif()

set(module_host_sources
    ${gateway_c_sources}
)

if (${enable_event_system})
    set( event_system_sources ./src/internal/event_system.c )
else()
    set( event_system_sources ./src/internal/event_stub.c )
endif()

set(gateway_c_sources
    ${gateway_c_sources}
    ${event_system_sources}
    ./src/gateway_internal.c
    ./src/gateway.c
    ./src/gateway_createfromjson.c
    ./src/broker.c
)

include_directories(./inc)

add_library(module_host_static STATIC
    ${module_host_sources}
    ${gateway_h_sources}
)

add_library(gateway SHARED
    ${gateway_c_sources}
    ${gateway_h_sources}
)

add_library(gateway_static STATIC
    ${gateway_c_sources}
    ${gateway_h_sources}
)

if(${enable_core_remote_module_support})
    if(WIN32)
        target_link_libraries(gateway ${CMAKE_SOURCE_DIR}/build_libuv/dist/lib/libuv.lib Psapi.lib Iphlpapi.lib Userenv.lib)
        target_link_libraries(gateway_static ${CMAKE_SOURCE_DIR}/build_libuv/dist/lib/libuv.lib Psapi.lib Iphlpapi.lib Userenv.lib)
        target_link_libraries(module_host_static ${CMAKE_SOURCE_DIR}/build_libuv/dist/lib/libuv.lib Psapi.lib Iphlpapi.lib Userenv.lib)
    else()
        target_link_libraries(gateway ${CMAKE_SOURCE_DIR}/build_libuv/dist/lib/libuv.a)
        target_link_libraries(gateway_static ${CMAKE_SOURCE_DIR}/build_libuv/dist/lib/libuv.a)
        target_link_libraries(module_host_static ${CMAKE_SOURCE_DIR}/build_libuv/dist/lib/libuv.a)
    endif()
endif()

target_link_libraries(gateway parson nanomsg aziotsharedutil ${dynamic_loader_library})
target_link_libraries(gateway_static parson nanomsg aziotsharedutil ${dynamic_loader_library})
target_link_libraries(module_host_static parson nanomsg aziotsharedutil ${dynamic_loader_library})

if(NOT WIN32)
    target_link_libraries(gateway m ${NN_REQUIRED_LIBRARIES})
    target_link_libraries(module_host_static m ${NN_REQUIRED_LIBRARIES})
endif()

if(NOT ${use_xplat_uuid})
    if(WIN32)
        target_link_libraries(gateway rpcrt4.lib)
        target_link_libraries(gateway_static rpcrt4.lib)
        target_link_libraries(module_host_static rpcrt4.lib)
    else()
        find_package(PkgConfig REQUIRED)
        pkg_search_module(UUID REQUIRED uuid)
        link_directories(${UUID_LIBRARY_DIRS})

        if(APPLE)
            target_link_libraries(gateway -L${UUID_LIBRARY_DIRS} ${UUID_LIBRARIES})
            target_link_libraries(gateway_static -L${UUID_LIBRARY_DIRS} ${UUID_LIBRARIES})
            target_link_libraries(module_host_static -L${UUID_LIBRARY_DIRS} ${UUID_LIBRARIES})
        elseif(LINUX)
            target_link_libraries(gateway ${UUID_LIBRARIES})
            target_link_libraries(gateway_static ${UUID_LIBRARIES})
            target_link_libraries(module_host_static ${UUID_LIBRARIES})
        endif()
    endif()
endif()

#this adds the tests to the build process
if(${run_unittests})
    add_subdirectory(tests)
endif()

#############################################################
####################### INSTALL STUFF #######################
#############################################################

#installs the gateway libs and headers
if (CMAKE_INSTALL_LIBDIR)
    set (LIB_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}")
else ()
    set (LIB_INSTALL_DIR "lib")
endif ()

# (re)set the include and lib dirs for the install based on platform
if(WIN32)
    set(AIG_HEADER_INSTALL_PREFIX "${CMAKE_INSTALL_INCLUDEDIR}" CACHE INTERNAL "Needs to be included to share across projects" FORCE)
    set (LIB_INSTALL_DIR "${LIB_INSTALL_DIR}" CACHE INTERNAL "Library install dir")
    set(package_location "cmake")
else()
    set(AIG_HEADER_INSTALL_PREFIX "${CMAKE_INSTALL_INCLUDEDIR}/${VERSION_PREFIX}" CACHE INTERNAL "Needs to be included to share across projects" FORCE)
    set (LIB_INSTALL_DIR "${LIB_INSTALL_DIR}/${VERSION_PREFIX}" CACHE INTERNAL "Library install dir")
    set(package_location "${LIB_INSTALL_DIR}/cmake")
endif()

# Set the include dirs for the gatewayTargets includes destination
if(WIN32)
    set(AIG_INCLUDE_DIRS ${AIG_HEADER_INSTALL_PREFIX} ${SHARED_UTIL_INC_FOLDER})
else()
    set(AIG_INCLUDE_DIRS ${AIG_HEADER_INSTALL_PREFIX})
endif()

install(TARGETS gateway gateway_static module_host_static EXPORT gatewayTargets
    LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
    RUNTIME DESTINATION ${LIB_INSTALL_DIR}/../bin
    INCLUDES DESTINATION ${AIG_INCLUDE_DIRS}
)
install(FILES ${gateway_h_sources} ${gateway_h_sources} DESTINATION ${AIG_HEADER_INSTALL_PREFIX})

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${GATEWAY_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_file("${PROJECT_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}Config.cmake"
    COPYONLY
)

install(EXPORT gatewayTargets
    FILE
        "${PROJECT_NAME}Targets.cmake"
    DESTINATION
        ${package_location}
    EXPORT_LINK_INTERFACE_LIBRARIES
)
install(
    FILES
        "${PROJECT_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION
        ${package_location}
)
