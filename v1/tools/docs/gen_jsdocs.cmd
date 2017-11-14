@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

@setlocal EnableExtensions EnableDelayedExpansion
@echo off

cd %~dp0..\..

rem -----------------------------------------------------------------------------
rem -- Generate Node.js binding docs
rem -----------------------------------------------------------------------------
set "doc-target-dir=%~dp0..\..\doc\api_reference\node"
if exist %doc-target-dir% rd /s /q %doc-target-dir%
if not exist %doc-target-dir% mkdir %doc-target-dir% 

call :jsdoc "Microsoft Azure IoT Gateway SDK" "-c tools\docs\jsdoc.conf.json -d %doc-target-dir%"

goto :eof

:jsdoc
cmd /c "set "JSDOC_TITLE=%~1" && jsdoc %~2"
goto :eof
