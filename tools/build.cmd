@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

@setlocal EnableExtensions EnableDelayedExpansion
@echo off

set current-path=%~dp0

rem // remove trailing slash
set current-path=%current-path:~0,-1%

set build-root=%current-path%\..
rem // resolve to fully qualified path
for %%i in ("%build-root%") do set build-root=%%~fi

set local-install=%build-root%\install-deps

rem ----------------------------------------------------------------------------
rem -- parse script arguments
rem ----------------------------------------------------------------------------

rem // default build options
set build-config=Debug
set build-platform=Win32
set rebuild_deps=OFF
set CMAKE_run_unittests=OFF
set CMAKE_run_e2e_tests=OFF
set CMAKE_enable_dotnet_binding=OFF
set CMAKE_enable_dotnet_core_binding=OFF
set enable-java-binding=OFF
set enable_nodejs_binding=OFF
set enable_native_remote_modules=ON
set enable_nodejs_remote_modules=OFF
set enable_java_remote_modules=OFF
set CMAKE_enable_ble_module=ON
set use_xplat_uuid=OFF
set dependency_install_prefix="-Ddependency_install_prefix=%local-install%"

:args-loop
if "%1" equ "" goto args-done
if "%1" equ "--config" goto arg-build-config
if "%1" equ "--rebuild-deps" goto arg-rebuild-deps
if "%1" equ "--platform" goto arg-build-platform
if "%1" equ "--run-unittests" goto arg-run-unittests
if "%1" equ "--run-e2e-tests" goto arg-run-e2e-tests
if "%1" equ "--enable-dotnet-binding" goto arg-enable-dotnet-binding
if "%1" equ "--enable-dotnet-core-binding" goto arg-enable-dotnet-core-binding
if "%1" equ "--enable-java-binding" goto arg-enable-java-binding
if "%1" equ "--enable-nodejs-binding" goto arg-enable_nodejs_binding
if "%1" equ "--disable-native-remote-modules" goto arg-disable_native_remote_modules
if "%1" equ "--enable-nodejs-remote-modules" goto arg-enable_nodejs_remote_modules
if "%1" equ "--enable-java-remote-modules" goto arg-enable-java-remote-modules
if "%1" equ "--disable-ble-module" goto arg-disable_ble_module
if "%1" equ "--system-deps-path" goto arg-system-deps-path
if "%1" equ "--use-xplat-uuid" goto arg-use-xplat-uuid

call :usage && exit /b 1

:arg-build-config
shift
if "%1" equ "" call :usage && exit /b 1
set build-config=%1
goto args-continue

:arg-build-platform
shift
if "%1" equ "" call :usage && exit /b 1
set build-platform=%1
goto args-continue

:arg-rebuild-deps
set rebuild_deps=ON
goto args-continue

:arg-run-unittests
set CMAKE_run_unittests=ON
goto args-continue

:arg-run-e2e-tests
set CMAKE_run_e2e_tests=ON
goto args-continue

:arg-enable-dotnet-binding
set CMAKE_enable_dotnet_binding=ON
if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!
goto args-continue

:arg-enable-dotnet-core-binding
set CMAKE_enable_dotnet_core_binding=ON
if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!
goto args-continue

:arg-enable-java-binding
set enable-java-binding=ON
call %current-path%\build_java.cmd
if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!
goto args-continue

:arg-disable_ble_module
set CMAKE_enable_ble_module=OFF
goto args-continue

:arg-enable_nodejs_binding
set enable_nodejs_binding=ON
goto args-continue

:arg-disable_native_remote_modules
set enable_native_remote_modules=OFF
goto args-continue

:arg-enable_nodejs_remote_modules
set enable_nodejs_remote_modules=ON
goto args-continue

:arg-enable-java-remote-modules
set enable_java_remote_modules=ON

if "%enable-java-binding%" == "OFF" (
    call %current-path%\build_java.cmd
    if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!
)

call %current-path%\build_java_oop.cmd
if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!

goto args-continue

:arg-system-deps-path
set dependency_install_prefix=""
goto args-continue

:arg-use-xplat-uuid
set use_xplat_uuid=ON
goto args-continue

:args-continue
shift
goto args-loop

:args-done

rem -----------------------------------------------------------------------------
rem -- run .NET Scripts
rem -----------------------------------------------------------------------------

if %CMAKE_enable_dotnet_core_binding% == ON (
    call %current-path%\build_dotnet_core.cmd --config %build-config%
    if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!
)

if %CMAKE_enable_dotnet_binding% == ON (
    call %current-path%\build_dotnet.cmd --config %build-config%
    if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!
)

rem -----------------------------------------------------------------------------
rem -- lint/test Node.js proxy gateway
rem -----------------------------------------------------------------------------

if %enable_nodejs_remote_modules% == ON (
    rem lint the sample
    pushd %build-root%\samples\nodejs_remote_sample
    if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!
    call npm install
    if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!
    call npm run lint
    if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!
    popd

    pushd %build-root%\proxy\gateway\nodejs
    if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!
    call npm install
    if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!
    call npm run lint
    if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!
    call npm test
    if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!
    popd
)

rem -----------------------------------------------------------------------------
rem -- build with CMAKE and run tests
rem -----------------------------------------------------------------------------

rem this is setting the cmake path in a quoted way
set "cmake-root=%build-root%\build"

rem Figure out which CMake generator to use
if "%VisualStudioVersion%" == "14.0" (
    set "cmake-generator=Visual Studio 14 2015"
) else if "%VisualStudioVersion%" == "15.0" (
    set "cmake-generator=Visual Studio 15 2017"
)
if NOT DEFINED cmake-generator (
    echo ERROR: VisualStudioVersion not defined, or not one of ^(14.0, 15.0^)
    exit /b 1
)
if "%build-platform%" == "x64" (
    set "cmake-generator=%cmake-generator% Win64"
)
echo **Building for "%cmake-generator%"

echo Cleaning up build artifacts...
rmdir /s/q %cmake-root%
if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!

git submodule foreach --recursive --quiet "rm -rf build/"
if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!

mkdir %cmake-root%
if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!

pushd %cmake-root%
cmake %dependency_install_prefix% -DCMAKE_BUILD_TYPE="%build-config%" -Drun_unittests:BOOL=%CMAKE_run_unittests% -Drun_e2e_tests:BOOL=%CMAKE_run_e2e_tests% -Denable_dotnet_binding:BOOL=%CMAKE_enable_dotnet_binding% -Denable_dotnet_core_binding:BOOL=%CMAKE_enable_dotnet_core_binding% -Denable_java_binding:BOOL=%enable-java-binding% -Denable_nodejs_binding:BOOL=%enable_nodejs_binding% -Denable_native_remote_modules:BOOL=%enable_native_remote_modules% -Denable_java_remote_modules:BOOL=%enable_java_remote_modules% -Denable_nodejs_remote_modules:BOOL=%enable_nodejs_remote_modules% -Denable_ble_module:BOOL=%CMAKE_enable_ble_module% -Drebuild_deps:BOOL=%rebuild_deps% -Duse_xplat_uuid:BOOL=%use_xplat_uuid% -G "%cmake-generator%" "%build-root%"
if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!

msbuild /m /p:Configuration="%build-config%" /p:Platform="%build-platform%" azure_iot_gateway_sdk.sln
if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!

if "%CMAKE_run_unittests%"=="OFF" if "%CMAKE_run_e2e_tests%"=="OFF" goto skip-tests

ctest -C "%build-config%" -V
if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!
:skip-tests

popd
goto :eof

rem -----------------------------------------------------------------------------
rem -- subroutines
rem -----------------------------------------------------------------------------

:usage
echo build.cmd [options]
echo options:
echo  --config value                  Build configuration (e.g. [Debug], Release)
echo  --disable-ble-module            Do not build the BLE module
echo  --enable-dotnet-binding         Build .NET binding
echo  --enable-dotnet-core-binding    Build the .NET Core binding
echo  --enable-java-binding           Build Java binding
echo                                  (JAVA_HOME must be defined in your environment)
echo  --enable-nodejs-binding         Build Node.js binding
echo                                  (NODE_INCLUDE, NODE_LIB must be defined)
echo  --disable-native-remote-modules Do not build the infrastructure required
echo                                  to support native remote modules
echo  --enable-nodejs-remote-modules  Build the infrastructure required to support
echo                                  Node.js apps as remote modules
echo  --enable-java-remote-modules    Build Java Remote modules SDK
echo                                  (JAVA_HOME must be defined in your environment)
echo  --platform value                Build platform (e.g. [Win32], x64, ...)
echo  --rebuild-deps                  Force rebuild of dependencies
echo  --run-e2e-tests                 Build/run end-to-end tests
echo  --run-unittests                 Build/run unit tests
echo  --system-deps-path              Search for dependencies in a system-level location,
echo                                  e.g. "C:\Program Files (x86)", and install if not
echo                                  found. When this option is omitted the path is
echo                                  %local-install%.
echo  --use-xplat-uuid                Use SDK's platform-independent UUID implementation
goto :eof

