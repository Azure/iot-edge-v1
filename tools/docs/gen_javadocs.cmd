@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

@setlocal EnableExtensions EnableDelayedExpansion
@echo off

set build-root=%~dp0..\..

rem -----------------------------------------------------------------------------
rem -- Generate Java binding docs
rem -----------------------------------------------------------------------------
echo Generating Java binding API docs...
cd %build-root%\bindings\java\gateway-java-binding
call mvn -q javadoc:javadoc
if not !ERRORLEVEL!==0 (
    echo Generating java docs for gateway-java-binding failed.
    exit /b !ERRORLEVEL!
)

rem Move the generated docs to %build-root%\doc\api_reference\java
set doc-target-dir=%build-root%\doc\api_reference\java
echo Copying Java binding API docs to %doc-target-dir%
if exist %doc-target-dir% rd /s /q %doc-target-dir%
mkdir %doc-target-dir%
xcopy /q /e /y target\site\apidocs %doc-target-dir%
if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!