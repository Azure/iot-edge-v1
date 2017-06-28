@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

setlocal

set build-root=%~dp0..
rem // resolve to fully qualified path
for %%i in ("%build-root%") do set build-root=%%~fi

set "JAVA_HOME=%JAVA_8_64%"
set "PATH=%JAVA_HOME%\bin;%JAVA_HOME%\jre\bin\server;%PATH%;%NODE_LIB%"

cd %build-root%\tools
call build.cmd --run-unittests --run-e2e-tests --enable-nodejs-binding --enable-dotnet-binding --enable-dotnet-core-binding --enable-java-binding %*
if errorlevel 1 exit /b 1
