#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

include("gatewayFunctions.cmake")

###############################################################################
###################Find/Install/Build azure_c_shared_utility###################
###############################################################################
findAndInstall(azure_c_shared_utility ${PROJECT_SOURCE_DIR}/deps/c-utility ${PROJECT_SOURCE_DIR}/deps/c-utility -Duse_installed_dependencies=ON)
set(SHARED_UTIL_INC_FOLDER ${AZURE_C_SHARED_UTILITY_INCLUDE_DIR} CACHE INTERNAL "this is what needs to be included if using sharedLib lib" FORCE)
set(SHARED_UTIL_LIB_FOLDER ${AZURE_C_SHARED_LIBRARY_DIR} CACHE INTERNAL "this is what needs to be included if using sharedLib lib" FORCE)
set(SHARED_UTIL_LIB aziotsharedutil CACHE INTERNAL "this is what needs to be included if using sharedLib lib" FORCE)
set(SHARED_UTIL_SRC_FOLDER "${CMAKE_CURRENT_LIST_DIR}/deps/c-utility/src" CACHE INTERNAL "")
set(SHARED_UTIL_ADAPTER_FOLDER "${CMAKE_CURRENT_LIST_DIR}/deps/c-utility/adapters" CACHE INTERNAL "")
set_platform_files("${CMAKE_CURRENT_LIST_DIR}/deps/c-utility")

###############################################################################
##########################Find/Install/Build nanomsg###########################
###############################################################################
if(WIN32)
    findAndInstallNonFindPkg(nanomsg ${PROJECT_SOURCE_DIR}/deps/nanomsg ${PROJECT_SOURCE_DIR}/deps/nanomsg)
    set(nanomsg_target_dll "${CMAKE_INSTALL_PREFIX}/../nanomsg/bin/nanomsg.dll" CACHE INTERNAL "The location of the nanomsg.dll (windows)" FORCE)
    add_library(nanomsg STATIC IMPORTED)
    set_target_properties(nanomsg PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/../nanomsg/include"
        IMPORTED_LOCATION "${CMAKE_INSTALL_PREFIX}/../nanomsg/lib/nanomsg.lib"
    )
    set(NANOMSG_INCLUDES "${CMAKE_INSTALL_PREFIX}/../nanomsg/include" CACHE INTERNAL "")
else()
    find_package(PkgConfig REQUIRED)
    pkg_search_module(NANOMSG QUIET nanomsg)
    if(NOT NANOMSG_FOUND)
        findAndInstallNonFindPkg(nanomsg ${PROJECT_SOURCE_DIR}/deps/nanomsg ${PROJECT_SOURCE_DIR}/deps/nanomsg)
        pkg_search_module(NANOMSG REQUIRED nanomsg)
    endif()
    set(NANOMSG_INCLUDES "${NANOMSG_INCLUDEDIR}"  CACHE INTERNAL "")
endif()