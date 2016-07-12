@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

setlocal

set build-root=%~dp0..
rem // resolve to fully qualified path
for %%i in ("%build-root%") do set build-root=%%~fi

REM -- Build Java Binding --
cd %build-root%\bindings\java\gateway-java-binding
call mvn clean install
if errorlevel 1 goto :eof
cd %build-root%