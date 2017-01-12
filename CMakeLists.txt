#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)
project(azure_iot_gateway_sdk)

set(GATEWAY_VERSION 1.0.1 CACHE INTERNAL "")
set(COMPANY_NAME "Microsoft")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

include(GNUInstallDirs)

if(WIN32)
    set(VERSION_PREFIX "${GATEWAY_VERSION}" CACHE PATH "Version Prefix")
else()
    set(VERSION_PREFIX "${PROJECT_NAME}-${GATEWAY_VERSION}" CACHE PATH "Version Prefix")
endif()

if(WIN32)
    if(NOT DEFINED CMAKE_INSTALL_PREFIX_SET)
      set(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}-${VERSION_PREFIX}/" CACHE INTERNAL "CMake install prefix")
      set(CMAKE_INSTALL_PREFIX_SET TRUE CACHE INTERNAL "")
    endif()
endif()

message(STATUS "CMake Install Prefix is ${CMAKE_INSTALL_PREFIX}")

set(${dependency_install_prefix} ${dependency_install_prefix} CACHE INTERNAL "")

if(DEFINED ${dependency_install_prefix})
  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${dependency_install_prefix})
  set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_RPATH} ${dependency_install_prefix}/${CMAKE_INSTALL_LIBDIR})
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
else()
  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${CMAKE_INSTALL_PREFIX})
  set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_RPATH} ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

# tests and samples should use pre-install path to gateway.so
set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_RPATH} ${CMAKE_CURRENT_BINARY_DIR}/core/)

#the following variables are project-wide and can be used with cmake-gui
option(run_unittests "set run_unittests to ON to run unittests (default is OFF)" OFF)
option(run_e2e_tests "set run_e2e_tests to ON to run e2e tests (default is OFF) " OFF)
option(nuget_e2e_tests "" OFF)
option(install_executables "should cmake run cmake's install function (that includes dynamic link libraries) [it does for yocto]" OFF)
option(install_modules "should cmake install the default gateway modules" OFF)
option(enable_java_binding "set enable_java_binding to ON to enable building of Java binding (default is OFF)" OFF)
option(enable_nodejs_binding "set enable_nodejs_binding to ON to enable building of Node JS binding (default is OFF)" OFF)
option(run_as_a_service "Flags that we have the goal of running gateway as a service for samples and OS that supports it." OFF)
option(enable_dotnet_binding "set enable_dotnet_binding to ON to build dotnet binding host binaries (default is OFF)" OFF)
option(enable_ble_module "set enable_ble_module to OFF to remove ble module from gateway build (default is ON)" ON)
option(use_amqp "set use_amqp to ON if amqp is to be used, set to OFF to not use amqp" ON)
option(use_http "set use_http to ON if http is to be used, set to OFF to not use http" ON)
option(use_mqtt "set use_mqtt to ON if mqtt is to be used, set to OFF to not use mqtt" ON)


SET(use_condition ON CACHE BOOL "Build C shared utility with condition code" FORCE)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#making a nice global variable to know if we are on linux or not.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(LINUX TRUE)
  find_program( MEMORYCHECK_COMMAND valgrind )
  set( MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full --suppressions=${CMAKE_CURRENT_LIST_DIR}/core/valgrind_suppressions.txt --error-exitcode=1" )
endif()

include (CTest)

include(CheckSymbolExists)
function(detect_architecture symbol arch) 
    if (NOT DEFINED ARCHITECTURE OR ARCHITECTURE STREQUAL "")
        set(CMAKE_REQUIRED_QUIET 1)
        check_symbol_exists("${symbol}" "" ARCHITECTURE_${arch}) 
        unset(CMAKE_REQUIRED_QUIET) 
  
        # The output variable needs to be unique across invocations otherwise 
        # CMake's crazy scope rules will keep it defined 
        if (ARCHITECTURE_${arch}) 
            set(ARCHITECTURE "${arch}" PARENT_SCOPE) 
            set(ARCHITECTURE_${arch} 1 PARENT_SCOPE) 
            add_definitions(-DARCHITECTURE_${arch}=1) 
        endif()
    endif()
endfunction()
if (MSVC)
    detect_architecture("_M_AMD64" x86_64)
    detect_architecture("_M_IX86" x86)
    detect_architecture("_M_ARM" ARM)
else() 
    detect_architecture("__x86_64__" x86_64)
    detect_architecture("__i386__" x86)
    detect_architecture("__arm__" ARM)
endif() 
if (NOT DEFINED ARCHITECTURE OR ARCHITECTURE STREQUAL "") 
    set(ARCHITECTURE "GENERIC") 
endif() 
message(STATUS "AIG architecture: ${ARCHITECTURE}")

#setting #defines
if(WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  add_compile_options(/guard:cf)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /guard:cf")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /guard:cf")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /guard:cf")
endif()

if(LINUX)
  set (CMAKE_C_FLAGS "-fPIC ${CMAKE_C_FLAGS}")
  set (CMAKE_CXX_FLAGS "-fPIC ${CMAKE_CXX_FLAGS}")
endif()

function(add_module_to_solution undecoratedModuleName)
  set_target_properties(${undecoratedModuleName}
    PROPERTIES FOLDER "Modules/${undecoratedModuleName}")
  set_target_properties(${undecoratedModuleName}_static
    PROPERTIES FOLDER "Modules/${undecoratedModuleName}")
endfunction()

function(add_binding_to_solution bindingLanguage)
    set_target_properties(${bindingLanguage} PROPERTIES FOLDER "Bindings")
endfunction()
    
function(add_sample_to_solution sampleName)
  set_target_properties(${sampleName} PROPERTIES FOLDER "Samples")
  set_target_properties(${sampleName} PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
  )
endfunction()


macro(compileAsC99)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_C_FLAGS "--std=c99 ${CMAKE_C_FLAGS}")
      set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
    endif()
  else()
    set (CMAKE_C_STANDARD 99)
    set (CMAKE_CXX_STANDARD 11)
  endif()
endmacro(compileAsC99)

macro(compileAsC11)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_C_FLAGS "--std=c11 ${CMAKE_C_FLAGS}")
      set (CMAKE_C_FLAGS "-D_POSIX_C_SOURCE=200112L ${CMAKE_C_FLAGS}")
      set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
    endif()
  else()
    set (CMAKE_C_STANDARD 11)
    set (CMAKE_CXX_STANDARD 11)
  endif()
endmacro(compileAsC11)

enable_testing()
compileAsC99()

#this adds all test dependencies
include(dependencies-test.cmake)

#this adds all dependencies
include(dependencies.cmake)

function(linkSharedUtil whatIsBuilding)
  target_link_libraries(${whatIsBuilding} aziotsharedutil)
endfunction(linkSharedUtil)

#this adds parson
include_directories(./deps/parson)

function(link_broker whatIsBuilding)

target_link_libraries(${whatIsBuilding} nanomsg )
  
endfunction(link_broker)

function(install_broker whatIsBuilding whatIsBuildingLocation)
  if(WIN32)
  add_custom_command(TARGET ${whatIsBuilding} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${nanomsg_target_dll}
          ${whatIsBuildingLocation})
  endif()
endfunction(install_broker)

set(preinstall_gateway_library_dll ${CMAKE_CURRENT_BINARY_DIR}/core/$(Configuration)/gateway.dll)

function(copy_gateway_dll whatIsBuilding whatIsBuildingLocation)
  if(WIN32)
    add_custom_command(TARGET ${whatIsBuilding} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${preinstall_gateway_library_dll}
          ${whatIsBuildingLocation})
    if(EXISTS "${azure_c_shared_utility_DIR}/../bin/${SHARED_UTIL_LIB}.dll")
        add_custom_command(TARGET ${whatIsBuilding} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${azure_c_shared_utility_DIR}/../bin/${SHARED_UTIL_LIB}.dll"
          ${whatIsBuildingLocation})
    endif()


  endif()
endfunction(copy_gateway_dll)

function(copy_iothub_client_dll whatIsBuilding whatIsBuildingLocation)
  if(WIN32)
    if(EXISTS "${azure_iot_sdks_DIR}/../bin/iothub_client.dll")
      add_custom_command(TARGET ${whatIsBuilding} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${azure_iot_sdks_DIR}/../bin/iothub_client.dll"
        ${whatIsBuildingLocation})
    endif()
  endif()
endfunction(copy_iothub_client_dll)

function(copy_iothub_service_dll whatIsBuilding whatIsBuildingLocation)
  if(WIN32)
    if(EXISTS "${azure_iot_sdks_DIR}/../bin/iothub_service_client.dll")
      add_custom_command(TARGET ${whatIsBuilding} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${azure_iot_sdks_DIR}/../bin/iothub_service_client.dll"
        ${whatIsBuildingLocation})
    endif()
  endif()
endfunction(copy_iothub_service_dll)

if(${ARCHITECTURE} STREQUAL "x86_64")
   set(dotnet_managed_binding_dll ${CMAKE_CURRENT_BINARY_DIR}/../bindings/dotnet/dotnet-binding/Microsoft.Azure.IoT.Gateway/bin/x64/$(Configuration)/Microsoft.Azure.IoT.Gateway.dll CACHE INTERNAL "The location of the Microsoft.Azure.IoT.Gateway.dll (windows)" FORCE)

   set(dotnet_e2etest_module_dll ${CMAKE_CURRENT_BINARY_DIR}/../bindings/dotnet/dotnet-binding/E2ETestModule/bin/x64/$(Configuration)/E2ETestModule.dll CACHE INTERNAL "The location of the E2ETestModule.dll (windows)" FORCE)

   set(dotnet_sensor_module_dll ${CMAKE_CURRENT_BINARY_DIR}/../bindings/dotnet/dotnet-binding/SensorModule/bin/x64/$(Configuration)/SensorModule.dll CACHE INTERNAL "The location of the SensorModule.dll (windows)" FORCE)

   set(dotnet_printer_module_dll ${CMAKE_CURRENT_BINARY_DIR}/../bindings/dotnet/dotnet-binding/PrinterModule/bin/x64/$(Configuration)/PrinterModule.dll CACHE INTERNAL "The location of the PrinterModule.dll (windows)" FORCE)

elseif(${ARCHITECTURE} STREQUAL "x86")
   set(dotnet_managed_binding_dll ${CMAKE_CURRENT_BINARY_DIR}/../bindings/dotnet/dotnet-binding/Microsoft.Azure.IoT.Gateway/bin/x86/$(Configuration)/Microsoft.Azure.IoT.Gateway.dll CACHE INTERNAL "The location of the Microsoft.Azure.IoT.Gateway.dll (windows)" FORCE)

   set(dotnet_e2etest_module_dll ${CMAKE_CURRENT_BINARY_DIR}/../bindings/dotnet/dotnet-binding/E2ETestModule/bin/x86/$(Configuration)/E2ETestModule.dll CACHE INTERNAL "The location of the E2ETestModule.dll (windows)" FORCE)

   set(dotnet_sensor_module_dll ${CMAKE_CURRENT_BINARY_DIR}/../bindings/dotnet/dotnet-binding/SensorModule/bin/x86/$(Configuration)/SensorModule.dll CACHE INTERNAL "The location of the SensorModule.dll (windows)" FORCE)

   set(dotnet_printer_module_dll ${CMAKE_CURRENT_BINARY_DIR}/../bindings/dotnet/dotnet-binding/PrinterModule/bin/x86/$(Configuration)/PrinterModule.dll CACHE INTERNAL "The location of the PrinterModule.dll (windows)" FORCE)
endif()

set(dotnet_host_binding_dll ${CMAKE_CURRENT_BINARY_DIR}/bindings/dotnet/$(Configuration)/dotnet.dll CACHE INTERNAL "The location of dotnet.dll" FORCE)

function(install_binaries whatIsBuilding whatIsBuildingLocation whatIsBinaryLocation)
  add_custom_command(TARGET ${whatIsBuilding} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${whatIsBinaryLocation}
          ${whatIsBuildingLocation})
endfunction(install_binaries)


add_subdirectory(./core)

include_directories(${GW_INC})

add_subdirectory(modules)

add_subdirectory(bindings)

add_subdirectory(samples)
