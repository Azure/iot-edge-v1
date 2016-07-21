@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

@setlocal EnableExtensions EnableDelayedExpansion
@echo off

cd %build-root%\tools\docs\dotnet

rem ---------------------------------------------------------------------------
rem -- Check directory
rem ---------------------------------------------------------------------------
echo Generating .NET binding API docs...
set docs_dir=%build-root%\doc\api_reference\dotnet
if exist %docs_dir% rd /s /q %docs_dir%
if !ERRORLEVEL! neq 0 exit /b !ERRORLEVEL!
mkdir %docs_dir%

rem ---------------------------------------------------------------------------
rem -- Generate C API docs
rem ---------------------------------------------------------------------------
doxygen