#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 3.8.0)
set(CPPREST_LIBRARIES)
if (WIN32)
    set(CMAKE_TOOLCHAIN_FILE c:/vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake)
    find_library(CPPREST_LIB cpprest)
    message(STATUS "CPPREST_LIB : " ${CPPREST_LIB})
    set (VCPKG_ROOT c:/vcpkg/vcpkg/installed/x86-windows/lib)
    set(CPPREST_LIBRARIES
        ${VCPKG_ROOT}/cpprest_2_9.lib
#        ${VCPKG_ROOT}/boost_atomic-vc141-mt-1_64.dll
#        ${VCPKG_ROOT}/boost_chrono-vc141-mt-1_64.dll
#        ${VCPKG_ROOT}/boost_date_time-vc141-mt-1_64.lib
#        ${VCPKG_ROOT}/boost_filesystem-vc141-mt-1_64.dll
#        ${VCPKG_ROOT}/boost_random-vc141-mt-1_64.dll
#        ${VCPKG_ROOT}/boost_regex-vc141-mt-1_64.dll
#        ${VCPKG_ROOT}/boost_system-vc141-mt-1_64.dll
#        ${VCPKG_ROOT}/boost_thread-vc141-mt-1_64.dll
        ${VCPKG_ROOT}/LIBEAY32.lib
        ${VCPKG_ROOT}/SSLEAY32.lib
    )
    add_definitions(-DBOOST_ALL_DYN_LINK)
    message(STATUS "CMAKE_MODULE_PATH : " ${CMAKE_MODULE_PATH})
else()
endif()

set(httprestapi_sources
    ./src/httprestapi.c
)

set(httprestapi_headers
    ./inc/httprestapi.h
)

include_directories(./inc)
include_directories(${GW_INC})

#this builds the httprestapi dynamic dllrary
add_library(httprestapi MODULE ${httprestapi_sources}  ${httprestapi_headers})
target_link_libraries(httprestapi httprestserver gateway ${CPPREST_LIBRARIES})

#this builds the httprestapi static library
add_library(httprestapi_static STATIC ${httprestapi_sources} ${httprestapi_headers})
target_compile_definitions(httprestapi_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(httprestapi_static httprestserver gateway)

linkSharedUtil(httprestapi)
linkSharedUtil(httprestapi_static)

add_module_to_solution(httprestapi)

if(install_modules)
    install(TARGETS httprestapi LIBRARY DESTINATION "${LIB_INSTALL_DIR}/modules") 
endif()

add_subdirectory(httprestserver)
# For Test
# add_subdirectory(cpptest)
