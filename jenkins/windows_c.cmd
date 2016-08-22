@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

setlocal

set build-root=%~dp0..
rem // resolve to fully qualified path
for %%i in ("%build-root%") do set build-root=%%~fi

REM -- C --
cd %build-root%\tools
set PATH=%PATH%;%NODE_LIB%

REM -- Build first dotnet binding for End2End Test.
call build_dotnet.cmd %*
if errorlevel 1 goto :eof

call build.cmd --run-e2e-tests --enable-nodejs-binding --enable-dotnet-binding %*
if errorlevel 1 goto :eof
cd %build-root%