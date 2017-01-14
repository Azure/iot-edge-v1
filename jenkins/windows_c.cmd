@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

setlocal

set build-root=%~dp0..
rem // resolve to fully qualified path
for %%i in ("%build-root%") do set build-root=%%~fi

set build-platform=Win32
set java-option=
set all-args=%*

:args-loop
if "%1" equ "" goto args-done
if "%1" equ "--platform" goto arg-build-platform
shift

:arg-build-platform
shift
if "%1" equ "" call :usage && exit /b 1
set build-platform=%1
goto args-continue

:args-continue
shift
goto args-loop

:args-done

REM -- set JAVA_HOME based on platform 
set "JAVA_SAVE=%JAVA_HOME%"
if %build-platform% == x64 (
    set "JAVA_HOME=%JAVA_8_64%"
) else (
    set "JAVA_HOME=%JAVA_8_32%"
)

REM -- C --
cd %build-root%\tools
set "PATH=%JAVA_HOME%\bin;%JAVA_HOME%\jre\bin\server;%PATH%;%NODE_LIB%"

REM -- Build first dotnet binding for End2End Test.
call build_dotnet.cmd %*
if errorlevel 1 goto :reset-java

call build.cmd --run-unittests --run-e2e-tests --enable-nodejs-binding --enable-dotnet-binding --enable-java-binding %*
if errorlevel 1 goto :reset-java
cd %build-root%

:reset-java
set "JAVA_HOME=%JAVA_SAVE%"
