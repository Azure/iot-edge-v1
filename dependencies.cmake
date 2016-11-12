#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(POLICY CMP0054)
    cmake_policy(SET CMP0054 OLD)
endif()

include("gatewayFunctions.cmake")

###############################################################################
###################Find/Install/Build azure_c_shared_utility###################
###############################################################################
findAndInstall(azure_c_shared_utility ${PROJECT_SOURCE_DIR}/deps/c-utility ${PROJECT_SOURCE_DIR}/deps/c-utility -Duse_installed_dependencies=ON -G "${CMAKE_GENERATOR}")
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
    findAndInstallNonFindPkg(nanomsg ${PROJECT_SOURCE_DIR}/deps/nanomsg ${PROJECT_SOURCE_DIR}/deps/nanomsg -G "${CMAKE_GENERATOR}")
    add_library(nanomsg STATIC IMPORTED)

    if(DEFINED ${dependency_install_prefix})
        set(nanomsg_target_dll "${dependency_install_prefix}/bin/nanomsg.dll" CACHE INTERNAL "The location of the nanomsg.dll (windows)" FORCE)
        set_target_properties(nanomsg PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${dependency_install_prefix}/include"
            IMPORTED_LOCATION "${dependency_install_prefix}/lib/nanomsg.lib"
        )
        set(NANOMSG_INCLUDES "${dependency_install_prefix}/include" CACHE INTERNAL "")
    else()
        set(nanomsg_target_dll "${CMAKE_INSTALL_PREFIX}/../nanomsg/bin/nanomsg.dll" CACHE INTERNAL "The location of the nanomsg.dll (windows)" FORCE)
        set_target_properties(nanomsg PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/../nanomsg/include"
            IMPORTED_LOCATION "${CMAKE_INSTALL_PREFIX}/../nanomsg/lib/nanomsg.lib"
        )
        set(NANOMSG_INCLUDES "${CMAKE_INSTALL_PREFIX}/../nanomsg/include" CACHE INTERNAL "")
    endif()
else()
    include(FindPkgConfig)
    find_package(PkgConfig REQUIRED)

    #If using a custom install prefix, tell find pkg to use it instead of defaults
    if(DEFINED ${dependency_install_prefix})
        set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH TRUE)
    endif()

    pkg_search_module(NANOMSG QUIET nanomsg)
    if(NOT NANOMSG_FOUND)
        findAndInstallNonFindPkg(nanomsg ${PROJECT_SOURCE_DIR}/deps/nanomsg ${PROJECT_SOURCE_DIR}/deps/nanomsg -G "${CMAKE_GENERATOR}")
    endif()

    #If earlier cmake
    if("${CMAKE_VERSION}" VERSION_GREATER 3.0.2)
        pkg_search_module(NANOMSG REQUIRED nanomsg)
    else()
        if(DEFINED ${dependency_install_prefix})
            set(NANOMSG_INCLUDEDIR "${dependency_install_prefix}/include")
            set(NANOMSG_LIBRARIES nanomsg)
            set(NANOMSG_LIBRARY_DIRS "${dependency_install_prefix}/${CMAKE_INSTALL_LIBDIR}")
            set(NANOMSG_LDFLAGS "-L${NANOMSG_LIBRARY_DIRS};-l${NANOMSG_LIBRARIES}")
        else()
            pkg_search_module(NANOMSG REQUIRED nanomsg)
            set(NANOMSG_LIBRARY_DIRS "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
        endif()
    endif()

    add_library(nanomsg STATIC IMPORTED)
    set(NANOMSG_INCLUDES "${NANOMSG_INCLUDEDIR}"  CACHE INTERNAL "")
    message(STATUS "NANOMSG LIBRARIES: ${NANOMSG_LIBRARIES}")
    message(STATUS "NANOMSG LDFLAGS: ${NANOMSG_LDFLAGS}")
    message(STATUS "NANOMSG CFLAGS: ${NANOMSG_CFLAGS}")
    set_target_properties(nanomsg PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${NANOMSG_INCLUDEDIR}"
            INTERFACE_LINK_LIBRARIES "${NANOMSG_LIBRARIES}"
            INTERFACE_COMPILE_OPTIONS "${NANOMSG_LDFLAGS}"
            IMPORTED_LOCATION "${NANOMSG_LIBRARY_DIRS}/lib${NANOMSG_LIBRARIES}.so"
        )
endif()