#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)
project(azure_iot_gateway_sdk)

set(GATEWAY_VERSION 1.0.7 CACHE INTERNAL "")
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

# Enable PDB generation for all release build configurations.
# VC++ compiler and linker options taken from this article:
#   http://www.wintellect.com/devcenter/jrobbins/correctly-creating-native-c-release-build-pdbs
if(WIN32)
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Zi")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")

  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
  set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /DEBUG /OPT:REF /OPT:ICF")
  set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} /DEBUG /OPT:REF /OPT:ICF")

  set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
  set(CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO} /DEBUG /OPT:REF /OPT:ICF")
  set(CMAKE_MODULE_LINKER_FLAGS_MINSIZEREL "${CMAKE_MODULE_LINKER_FLAGS_MINSIZEREL} /DEBUG /OPT:REF /OPT:ICF")

  set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
  set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO} /DEBUG /OPT:REF /OPT:ICF")
  set(CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL "${CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL} /DEBUG /OPT:REF /OPT:ICF")
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

# apps need to be able to load native_module_host
set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_RPATH} ${CMAKE_CURRENT_BINARY_DIR}/proxy/modules/native_module_host/)

# tests and samples should use pre-install path to gateway.so
set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_RPATH} ${CMAKE_CURRENT_BINARY_DIR}/core/)

#the following variables are project-wide and can be used with cmake-gui
option(run_unittests "set run_unittests to ON to run unittests (default is OFF)" OFF)
option(rebuild_deps "set rebuild_deps to ON to rebuild dependencies (default is OFF)" OFF)
option(run_e2e_tests "set run_e2e_tests to ON to run e2e tests (default is OFF) " OFF)
option(nuget_e2e_tests "" OFF)
option(install_executables "should cmake run cmake's install function (that includes dynamic link libraries) [it does for yocto]" OFF)
option(install_modules "should cmake install the default gateway modules" OFF)
option(enable_java_binding "set enable_java_binding to ON to enable building of Java binding (default is OFF)" OFF)
option(enable_nodejs_binding "set enable_nodejs_binding to ON to enable building of Node JS binding (default is OFF)" OFF)
option(run_as_a_service "Flags that we have the goal of running gateway as a service for samples and OS that supports it." OFF)
option(enable_dotnet_binding "set enable_dotnet_binding to ON to build dotnet binding host binaries (default is OFF)" OFF)
option(enable_dotnet_core_binding "set enable_dotnet_core_binding to ON to build dotnet core binding host binaries (default is OFF)" OFF)
option(enable_ble_module "set enable_ble_module to OFF to remove ble module from gateway build (default is ON)" ON)
option(enable_native_remote_modules "Build the infrastructure required to support native remote modules (default is ON)" ON)
option(enable_java_remote_modules "Build the infrastructure required to support java remote modules (default is OFF)" OFF)
option(enable_nodejs_remote_modules "Build the infrastructure required to support Node.js apps as remote modules (default is OFF)" OFF)
option(use_amqp "set use_amqp to ON if amqp is to be used, set to OFF to not use amqp" ON)
option(use_http "set use_http to ON if http is to be used, set to OFF to not use http" ON)
option(use_mqtt "set use_mqtt to ON if mqtt is to be used, set to OFF to not use mqtt" ON)
option(use_xplat_uuid "use the SDK's platform-independent UUID implementation (default is OFF)" OFF)

if(WIN32)
    # C-SDK requires this to be set for some functions we use.
    set(use_etw OFF)
endif()

option(enable_event_system "Build event system (default is ON)" ON)

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
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wformat=2 -Wformat-security ")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wformat=2 -Wformat-security ")
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,-z,noexecheap -pie")
  if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wtrampolines -fstack-protector-all -Wstack-protector --param ssp-buffer-size=4 ")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wtrampolines -fstack-protector-all -Wstack-protector --param ssp-buffer-size=4" )
  endif()
elseif(APPLE)
  # Warnings as errors
  set (CMAKE_C_FLAGS "-Werror ${CMAKE_C_FLAGS} -fstack-protector-all -Wstack-protector --param ssp-buffer-size=4 ")
  set (CMAKE_CXX_FLAGS "-Werror ${CMAKE_CXX_FLAGS} -fstack-protector-all -Wstack-protector --param ssp-buffer-size=4 ")
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
  target_link_libraries(${whatIsBuilding} nanomsg ${NN_REQUIRED_LIBRARIES})
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

set(preinstall_native_module_host_dll ${CMAKE_CURRENT_BINARY_DIR}/proxy/modules/native_module_host/$(Configuration)/native_module_host.dll)

function(copy_module_host_dll whatIsBuilding whatIsBuildingLocation)
  if(WIN32)
    add_custom_command(TARGET ${whatIsBuilding} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${preinstall_native_module_host_dll}
          ${whatIsBuildingLocation})
    if(EXISTS "${azure_c_shared_utility_DIR}/../bin/${SHARED_UTIL_LIB}.dll")
        add_custom_command(TARGET ${whatIsBuilding} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${azure_c_shared_utility_DIR}/../bin/${SHARED_UTIL_LIB}.dll"
          ${whatIsBuildingLocation})
    endif()

  endif()
endfunction(copy_module_host_dll)

function(copy_iothub_client_dll whatIsBuilding whatIsBuildingLocation)
  if(WIN32)
    if(EXISTS "${azure_iot_sdks_DIR}/../bin/iothub_client_dll.dll")
      add_custom_command(TARGET ${whatIsBuilding} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${azure_iot_sdks_DIR}/../bin/iothub_client_dll.dll"
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

set(dotnet_managed_binding_dll "${CMAKE_CURRENT_BINARY_DIR}/../bindings/dotnet/dotnet-binding/Microsoft.Azure.IoT.Gateway/bin/Any CPU/$(Configuration)/Microsoft.Azure.IoT.Gateway.dll" CACHE INTERNAL "The location of the Microsoft.Azure.IoT.Gateway.dll (windows)" FORCE)
set(dotnet_e2etest_module_dll "${CMAKE_CURRENT_BINARY_DIR}/../bindings/dotnet/dotnet-binding/E2ETestModule/bin/Any CPU/$(Configuration)/E2ETestModule.dll" CACHE INTERNAL "The location of the E2ETestModule.dll (windows)" FORCE)
set(dotnet_sensor_module_dll "${CMAKE_CURRENT_BINARY_DIR}/../bindings/dotnet/dotnet-binding/SensorModule/bin/Any CPU/$(Configuration)/SensorModule.dll" CACHE INTERNAL "The location of the SensorModule.dll (windows)" FORCE)
set(dotnet_printer_module_dll "${CMAKE_CURRENT_BINARY_DIR}/../bindings/dotnet/dotnet-binding/PrinterModule/bin/Any CPU/$(Configuration)/PrinterModule.dll" CACHE INTERNAL "The location of the PrinterModule.dll (windows)" FORCE)
set(dotnet_host_binding_dll ${CMAKE_CURRENT_BINARY_DIR}/bindings/dotnet/${CMAKE_BUILD_TYPE}/dotnet.dll CACHE INTERNAL "The location of dotnet.dll" FORCE)

set(dotnet_core_host_binding_dll ${CMAKE_CURRENT_BINARY_DIR}/bindings/dotnetcore/$(Configuration)/dotnetcore.dll CACHE INTERNAL "The location of dotnetcore.dll" FORCE)
set(dotnet_core_host_binding_so ${CMAKE_CURRENT_BINARY_DIR}/bindings/dotnetcore/libdotnetcore.so CACHE INTERNAL "The location of libdotnetcore.so" FORCE)

set(dotnet_core_managed_binding_dll ${CMAKE_CURRENT_BINARY_DIR}/../bindings/dotnetcore/dotnet-core-binding/Microsoft.Azure.Devices.Gateway/bin/${CMAKE_BUILD_TYPE}/netstandard1.3/Microsoft.Azure.Devices.Gateway.dll CACHE INTERNAL "The location of the Microsoft.Azure.Devices.Gateway.dll" FORCE)
set(dotnet_core_e2etest_module_dll ${CMAKE_CURRENT_BINARY_DIR}/../bindings/dotnetcore/dotnet-core-binding/E2ETestModule/bin/${CMAKE_BUILD_TYPE}/netstandard1.3/E2ETestModule.dll CACHE INTERNAL "The location of the E2ETestModule.dll" FORCE)
set(dotnet_core_sensor_module_dll ${CMAKE_CURRENT_BINARY_DIR}/../samples/dotnet_core_module_sample/modules/SensorModule/bin/${CMAKE_BUILD_TYPE}/netstandard1.3/SensorModule.dll CACHE INTERNAL "The location of the SensorModule.dll" FORCE)
set(dotnet_core_printer_module_dll ${CMAKE_CURRENT_BINARY_DIR}/../samples/dotnet_core_module_sample/modules/PrinterModule/bin/${CMAKE_BUILD_TYPE}/netstandard1.3/PrinterModule.dll CACHE INTERNAL "The location of the PrinterModule.dll" FORCE)

function(install_binaries whatIsBuilding whatIsBuildingLocation whatIsBinaryLocation)
  add_custom_command(TARGET ${whatIsBuilding} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${whatIsBinaryLocation}
          ${whatIsBuildingLocation})
endfunction(install_binaries)


if (${enable_native_remote_modules} OR ${enable_java_remote_modules} OR ${enable_nodejs_remote_modules})
    set(enable_core_remote_module_support TRUE CACHE INTERNAL "")
endif()

add_subdirectory(core)

include_directories(${GW_INC}) # GW_INC defined in core

if (${enable_core_remote_module_support})
  cmake_minimum_required(VERSION 3.2.2)
  add_subdirectory(proxy)
  if(LINUX)
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ldl -lrt -pthread")
  endif()
endif()

add_subdirectory(modules)

add_subdirectory(bindings)

add_subdirectory(samples)
