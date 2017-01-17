#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)
if(POLICY CMP0054)
    cmake_policy(SET CMP0054 OLD)
endif()
cmake_policy(SET CMP0022 NEW)

add_subdirectory(deps)

#this sets a global var that stores where the includes for GW are
set(GW_INC ${CMAKE_CURRENT_LIST_DIR}/inc CACHE INTERNAL "Needs to be included for gateway includes" FORCE)
set(GW_SRC ${CMAKE_CURRENT_LIST_DIR}/src CACHE INTERNAL "Needs to be included for gateway sources" FORCE)

#setting the dynamic_loader file based on OS that it is used
if(WIN32)
    include_directories(${GW_INC}/windows )
elseif(LINUX)
    include_directories(${GW_INC}/linux)
endif()

#setting the dynamic_loader file based on OS that it is used
if(WIN32)
    set(dynamic_library_c_file ./adapters/dynamic_library_windows.c ./adapters/gb_library_windows.c)
elseif(LINUX)
    set(dynamic_library_c_file ./adapters/dynamic_library_linux.c ./adapters/gb_library_linux.c )
endif()

#setting specific libraries to be loaded based on OS (for example, Linux needs "-ldl", windows does not)
if(LINUX)
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
    ./src/message.c
    ./src/internal/event_system.c
    ./src/gateway_internal.c
    ./src/gateway.c
    ./src/gateway_createfromjson.c
    ./src/control_message.c
    ./src/broker.c
    ./src/module_loader.c
    ${dynamic_library_c_file}
)


set(gateway_h_sources
    ./inc/message.h
    ./inc/broker.h
    ./inc/module.h
    ./inc/module_access.h
    ./inc/experimental/event_system.h
    ./inc/gateway.h
    ./inc/gateway_export.h
    ./inc/gateway_version.h
    ./inc/control_message.h
    ./inc/module_loader.h
    ./inc/dynamic_library.h
    ./src/gateway_internal.h
    ../deps/parson/parson.h
)

# Add the module loaders
set(gateway_c_sources
    ${gateway_c_sources}
    ./src/module_loaders/dynamic_loader.c
    ./src/module_loaders/outprocess_loader.c
    ./src/module_loaders/outprocess_module.c
)
set(gateway_h_sources
    ${gateway_h_sources}
    ./inc/module_loaders/dynamic_loader.h
    ./inc/module_loaders/outprocess_loader.h
    ./inc/module_loaders/outprocess_module.h
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

include_directories(./inc)

add_library(gateway SHARED
    ${gateway_c_sources}
    ${gateway_h_sources}
)

add_library(gateway_static STATIC
    ${gateway_c_sources}
    ${gateway_h_sources}
)

if(WIN32)
    target_link_libraries(gateway parson nanomsg aziotsharedutil ${dynamic_loader_library})
    target_link_libraries(gateway_static parson nanomsg aziotsharedutil ${dynamic_loader_library})
else()
    target_link_libraries(gateway parson nanomsg aziotsharedutil ${dynamic_loader_library})

    target_link_libraries(gateway ${NN_REQUIRED_LIBRARIES})
    target_link_libraries(gateway m)

    target_link_libraries(gateway_static parson nanomsg aziotsharedutil ${dynamic_loader_library})

    target_link_libraries(gateway_static ${NN_REQUIRED_LIBRARIES})
    target_link_libraries(gateway_static m)
endif()

if(NOT ${use_xplat_uuid})
    if(WIN32)
        target_link_libraries(gateway rpcrt4.lib)
        target_link_libraries(gateway_static rpcrt4.lib)
    else()
        find_package(PkgConfig REQUIRED)
        pkg_search_module(UUID REQUIRED uuid)
        target_link_libraries(gateway ${UUID_LIBRARIES})
        target_link_libraries(gateway_static ${UUID_LIBRARIES})
    endif()
endif()

#this adds the tests to the build process
if(${run_unittests})
    add_subdirectory(tests)
endif()

#############################################################
########################INSTALL STUFF########################
#############################################################

#installs the gateway libs and headers
if (CMAKE_INSTALL_LIBDIR)
    set (LIB_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}")
else ()
    set (LIB_INSTALL_DIR "lib")
endif ()

# (re)set the include and lib dirs for the install based on platform
if(WIN32)
    set(AIG_HEADER_INSTALL_PREFIX "${CMAKE_INSTALL_INCLUDEDIR}")
    set (LIB_INSTALL_DIR "${LIB_INSTALL_DIR}" CACHE INTERNAL "Library install dir")
    set(package_location "cmake")
else()
    set(AIG_HEADER_INSTALL_PREFIX "${CMAKE_INSTALL_INCLUDEDIR}/${VERSION_PREFIX}")
    set (LIB_INSTALL_DIR "${LIB_INSTALL_DIR}/${VERSION_PREFIX}" CACHE INTERNAL "Library install dir")
    set(package_location "${LIB_INSTALL_DIR}/cmake")
endif()

# Set the include dirs for the gatewayTargets includes destination
if(WIN32)
    set(AIG_INCLUDE_DIRS ${AIG_HEADER_INSTALL_PREFIX} ${SHARED_UTIL_INC_FOLDER})
else()
    set(AIG_INCLUDE_DIRS ${AIG_HEADER_INSTALL_PREFIX})
endif()

install(TARGETS gateway gateway_static EXPORT gatewayTargets
    LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
    RUNTIME DESTINATION ${LIB_INSTALL_DIR}/../bin
    INCLUDES DESTINATION ${AIG_INCLUDE_DIRS}
)
install(FILES ${gateway_h_sources} DESTINATION ${AIG_HEADER_INSTALL_PREFIX})

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
