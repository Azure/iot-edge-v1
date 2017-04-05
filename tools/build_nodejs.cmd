@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

@setlocal EnableExtensions EnableDelayedExpansion
@echo off

set current-path=%~dp0

rem Remove trailing slash
set current-path=%current-path:~0,-1%

set build-root=%current-path%\..

rem Resolve to fully qualified path
for %%i in ("%build-root%") do set build-root=%%~fi

@powershell -File "%build-root%\tools\build_nodejs.ps1" %*

