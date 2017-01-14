#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(NOT azure_c_shared_utility_FOUND)
    find_package(azure_c_shared_utility REQUIRED CONFIG)
endif()

if(WIN32)
    add_library(nanomsg STATIC IMPORTED)
    if(DEFINED ${dependency_install_prefix})
        set_target_properties(nanomsg PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${dependency_install_prefix}/include"
            IMPORTED_LOCATION "${dependency_install_prefix}/lib/nanomsg.lib"
        )
        file(
            INSTALL
                "${dependency_install_prefix}/bin/nanomsg.dll"
            DESTINATION
                "${CMAKE_CURRENT_BINARY_DIR}"
        )
    else()
        set_target_properties(nanomsg PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/../../nanomsg/include/nanomsg"
            IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/../../nanomsg/lib/nanomsg.lib"
        )
        file(
            INSTALL
                "${CMAKE_CURRENT_LIST_DIR}/../../nanomsg/bin/nanomsg.dll"
            DESTINATION
                "${CMAKE_CURRENT_BINARY_DIR}"
        )
    endif()
    set(nanomsg_FOUND 1 CACHE INTERNAL "")
else()
    include(GNUInstallDirs)
    link_directories("${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
endif()

# install required gateway DLLs to current project 
if (WIN32)
    file(
        INSTALL
            "${azure_iot_gateway_sdk_DIR}/../bin/gateway.dll"
        DESTINATION
            "${CMAKE_CURRENT_BINARY_DIR}"
    )

    file(
        INSTALL
            "${azure_c_shared_utility_DIR}/../bin/aziotsharedutil.dll"
        DESTINATION
            "${CMAKE_CURRENT_BINARY_DIR}"
    )
endif()

link_directories("${CMAKE_CURRENT_LIST_DIR}/../lib")

include("${CMAKE_CURRENT_LIST_DIR}/azure_iot_gateway_sdkTargets.cmake")