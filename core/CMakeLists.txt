#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

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
    ./src/module_loader.c
    ./src/internal/event_system.c
	./src/gateway_internal.c
    ./src/gateway_ll.c
    ./src/gateway.c
    ./src/broker.c
    ${dynamic_library_c_file}
)


set(gateway_h_sources
    ./inc/message.h
    ./inc/broker.h
    ./inc/module.h
    ./inc/internal/event_system.h
    ./inc/gateway_ll.h
    ./inc/gateway.h
    ./inc/module_loader.h
    ./inc/dynamic_library.h
)

include_directories(./inc)

add_library(gateway
    ${gateway_c_sources}
    ${gateway_h_sources}
)

add_library(gateway_install
    ${gateway_c_sources}
    ${gateway_h_sources}
)

set_target_properties(gateway_install PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD 1)

if(WIN32)
    target_link_libraries(gateway_install debug "${CMAKE_CURRENT_BINARY_DIR}/../deps/c-utility/Debug/aziotsharedutil.lib" debug "${CMAKE_CURRENT_BINARY_DIR}/../deps/nanomsg/Debug/nanomsg.lib")
    target_link_libraries(gateway_install optimized "${CMAKE_CURRENT_BINARY_DIR}/../deps/c-utility/Release/aziotsharedutil.lib" optimized "${CMAKE_CURRENT_BINARY_DIR}/../deps/nanomsg/Release/nanomsg.lib")
    target_link_libraries(gateway_install parson ${dynamic_loader_library})
    
    target_link_libraries(gateway aziotsharedutil nanomsg parson ${dynamic_loader_library})
else()
    target_link_libraries(gateway_install "${CMAKE_CURRENT_BINARY_DIR}/../deps/c-utility/libaziotsharedutil.a" "${CMAKE_CURRENT_BINARY_DIR}/../deps/nanomsg/libnanomsg.a" parson ${dynamic_loader_library})
    target_link_libraries(gateway aziotsharedutil nanomsg parson ${dynamic_loader_library})
    
    nn_check_lib (anl getaddrinfo_a NN_HAVE_GETADDRINFO_A)
    nn_check_lib (rt clock_gettime  NN_HAVE_CLOCK_GETTIME)
    nn_check_lib (rt sem_wait NN_HAVE_SEMAPHORE_RT)
    nn_check_lib (pthread sem_wait  NN_HAVE_SEMAPHORE_PTHREAD)
    nn_check_lib (nsl gethostbyname NN_HAVE_LIBNSL)
    nn_check_lib (socket socket NN_HAVE_LIBSOCKET)

    target_link_libraries(gateway_install ${NN_REQUIRED_LIBRARIES})
    target_link_libraries(gateway_install m)

    target_link_libraries(gateway ${NN_REQUIRED_LIBRARIES})
    target_link_libraries(gateway m)
endif()

if(WIN32)
    target_link_libraries(gateway_install rpcrt4.lib)
    target_link_libraries(gateway rpcrt4.lib)
else()
    find_package(PkgConfig REQUIRED)
    pkg_search_module(UIDLINUX REQUIRED uuid)

    target_link_libraries(gateway_install uuid)
    target_link_libraries(gateway uuid)
endif()

set_target_properties(gateway PROPERTIES FOLDER "Gateway")

add_subdirectory(tests)

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
    set(AIG_INCLUDE_DIRS ${AIG_HEADER_INSTALL_PREFIX} ${AIG_HEADER_INSTALL_PREFIX}/azureiot)
else()
    set(AIG_INCLUDE_DIRS ${AIG_HEADER_INSTALL_PREFIX} ${AIG_HEADER_INSTALL_PREFIX}/../azureiot)
endif()

install(TARGETS gateway_install EXPORT gatewayTargets
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
    COMPATIBILITY AnyNewerVersion
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
)
install(
    FILES
        "${PROJECT_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION
        ${package_location}
)