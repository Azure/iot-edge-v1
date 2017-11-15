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

rem ----------------------------------------------------------------------------
rem -- parse arguments
rem ----------------------------------------------------------------------------

set build-clean=
set build-config=

:args-loop
if "%1" equ "" goto args-done
if "%1" equ "-c" goto arg-build-clean
if "%1" equ "--clean" goto arg-build-clean
if "%1" equ "--config" goto arg-build-config
call :usage && exit /b 1

:arg-build-clean
set "build-clean=--no-incremental"
goto args-continue

:arg-build-config
shift
if "%1" equ "" call :usage && exit /b 1
set "build-config=--configuration %1"
goto args-continue

:args-continue
shift
goto args-loop

:args-done

set binding-path="%build-root%\bindings\dotnetcore\dotnet-core-binding"
set samples-path="%build-root%\samples"
set sample-modules-path="%samples-path%\dotnet_core_module_sample\modules"

set projects-to-build=^
    "%binding-path%\Microsoft.Azure.Devices.Gateway\Microsoft.Azure.Devices.Gateway.csproj" ^
    "%binding-path%\E2ETestModule\E2ETestModule.csproj" ^
    "%sample-modules-path%\PrinterModule\PrinterModule.csproj" ^
    "%sample-modules-path%\SensorModule\SensorModule.csproj" ^
    "%samples-path%\dotnet_core_managed_gateway\dotnet_core_managed_gateway.csproj"

set projects-to-test=^
    "%binding-path%\Microsoft.Azure.Devices.Gateway.Tests\Microsoft.Azure.Devices.Gateway.Tests.csproj"

rem ----------------------------------------------------------------------------
rem -- restore
rem ----------------------------------------------------------------------------

for %%i in (%projects-to-build% %projects-to-test%) do (
    call dotnet restore %%i
    if not !errorlevel!==0 exit /b !errorlevel!
)

rem ----------------------------------------------------------------------------
rem -- build
rem ----------------------------------------------------------------------------

for %%i in (%projects-to-build%) do (
    call dotnet build %build-clean% %build-config% %%i
    if not !errorlevel!==0 exit /b !errorlevel!
)

rem ----------------------------------------------------------------------------
rem -- test
rem ----------------------------------------------------------------------------

for %%i in (%projects-to-test%) do (
    call dotnet test %%i
    if not !errorlevel!==0 exit /b !errorlevel!
)

goto :eof

rem ----------------------------------------------------------------------------
rem -- helper routines
rem ----------------------------------------------------------------------------

:usage
echo build_dotnet_core.cmd [options]
echo options:
echo  -c, --clean           delete artifacts from previous build before building
echo  --config ^<value^>      [Debug] build configuration (e.g. Debug, Release)
goto :eof
