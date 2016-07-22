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

rem ensure nuget.exe exists
where /q nuget.exe
if not !errorlevel! == 0 (
@Echo Azure IoT Gateway SDK needs to download nuget.exe from https://www.nuget.org/nuget.exe 
@Echo https://www.nuget.org 
choice /C yn /M "Do you want to download and run nuget.exe"
if not !errorlevel!==1 goto :eof
rem if nuget.exe is not found, then ask user
Powershell.exe wget -outf nuget.exe https://nuget.org/nuget.exe
	if not exist .\nuget.exe (
		echo nuget does not exist
		exit /b 1
	)
)

rem -----------------------------------------------------------------------------
rem -- parse script arguments
rem -----------------------------------------------------------------------------

rem // default build options
set build-clean=0
set build-config=Release
set build-platform=x86
set run-unit-tests=1

:args-loop
if "%1" equ "" goto args-done
if "%1" equ "-c" goto arg-build-clean
if "%1" equ "--clean" goto arg-build-clean
if "%1" equ "--config" goto arg-build-config
if "%1" equ "--platform" goto arg-build-platform
if "%1" equ "--skip-unittests" goto arg-skip-unittests
call :usage && exit /b 1

:arg-build-clean
set build-clean=1
goto args-continue

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

:arg-skip-unittests
set run-unit-tests=0
goto args-continue

:args-continue
shift
goto args-loop

:args-done



rem -----------------------------------------------------------------------------
rem -- build csharp language binding project.
rem -----------------------------------------------------------------------------
call nuget restore "%build-root%\bindings\uwp\uwp-binding\Microsoft.Azure.IoT.Gateway.sln"



if %build-clean%==1 (
    call :clean-a-solution "%build-root%\bindings\uwp\uwp-binding\Microsoft.Azure.IoT.Gateway.Test.sln" %build-config% %build-platform%
    if not !errorlevel!==0 exit /b !errorlevel!
)
call :build-a-solution "%build-root%\bindings\uwp\uwp-binding\Microsoft.Azure.IoT.Gateway.Test.sln" %build-config% %build-platform%
if not !errorlevel!==0 exit /b !errorlevel!

if not %run-unit-tests%==1 goto :eof
rem ------------------
rem -- run unit tests

rem RELEASE: azure-iot-gateway-sdk\bindings\uwp\uwp-binding\Microsoft.Azure.IoT.Gateway.Test\AppPackages\Microsoft.Azure.IoT.Gateway.Test_1.0.0.0_x86_Test\Microsoft.Azure.IoT.Gateway.Test_1.0.0.0_x86.appx
rem DEBUG  : azure-iot-gateway-sdk\bindings\uwp\uwp-binding\Microsoft.Azure.IoT.Gateway.Test\AppPackages\Microsoft.Azure.IoT.Gateway.Test_1.0.0.0_x86_Debug_Test\Microsoft.Azure.IoT.Gateway.Test_1.0.0.0_x86_Debug.appx
if %build-config%==Debug (
    set suffix=_Debug
)
if %build-config%==Release (
    set suffix=
)
set appxpath=Microsoft.Azure.IoT.Gateway.Test_1.0.0.0_%build-platform%%suffix%_Test
call echo Certutil.exe -addstore root "%build-root%\bindings\uwp\uwp-binding\Microsoft.Azure.IoT.Gateway.Test\AppPackages\%appxpath%\Microsoft.Azure.IoT.Gateway.Test_1.0.0.0_%build-platform%%suffix%.cer"
call Certutil.exe -addstore root "%build-root%\bindings\uwp\uwp-binding\Microsoft.Azure.IoT.Gateway.Test\AppPackages\%appxpath%\Microsoft.Azure.IoT.Gateway.Test_1.0.0.0_%build-platform%%suffix%.cer"
call echo VSTest.Console.exe "%build-root%\bindings\uwp\uwp-binding\Microsoft.Azure.IoT.Gateway.Test\AppPackages\%appxpath%\Microsoft.Azure.IoT.Gateway.Test_1.0.0.0_%build-platform%%suffix%.appx" /InIsolation
call VSTest.Console.exe "%build-root%\bindings\uwp\uwp-binding\Microsoft.Azure.IoT.Gateway.Test\AppPackages\%appxpath%\Microsoft.Azure.IoT.Gateway.Test_1.0.0.0_%build-platform%%suffix%.appx" /InIsolation
if not !errorlevel!==0 exit /b !errorlevel!
rem ------------------

rem -----------------------------------------------------------------------------
rem -- done
rem -----------------------------------------------------------------------------

goto :eof

rem -----------------------------------------------------------------------------
rem -- subroutines
rem -----------------------------------------------------------------------------

:clean-a-solution
call :_run-msbuild "Clean" %1 %2 %3
echo %errorlevel%
goto :eof

:build-a-solution
call :_run-msbuild "Build" %1 %2 %3
goto :eof

:usage
echo build.cmd [options]
echo options:
echo  -c, --clean           delete artifacts from previous build before building
echo  --skip-unittests      skip running unit tests after build
echo  --config ^<value^>      [Debug] build configuration (e.g. Debug, Release)
echo  --platform ^<value^>    [Win32] build platform (e.g. Win32, x64, ...)
goto :eof

rem -----------------------------------------------------------------------------
rem -- helper subroutines
rem -----------------------------------------------------------------------------

:_run-msbuild
rem // optionally override configuration|platform
setlocal EnableExtensions
set build-target=
if "%~1" neq "Build" set "build-target=/t:%~1"
if "%~3" neq "" set build-config=%~3
if "%~4" neq "" set build-platform=%~4

msbuild /m %build-target% "/p:Configuration=%build-config%;Platform=%build-platform%" %2
if not %errorlevel%==0 exit /b %errorlevel%
goto :eof

