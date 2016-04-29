@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

@setlocal EnableExtensions EnableDelayedExpansion
@echo off

set build-root=%~dp0..
cd %build-root%\tools\docs

rem -----------------------------------------------------------------------------
rem -- Check for environment pre-requisites. This script requires
rem -- that the following programs work:
rem --     doxygen
rem -----------------------------------------------------------------------------
call :checkExists git
if not %errorlevel%==0 exit /b %errorlevel%
call :checkExists doxygen
if not %errorlevel%==0 exit /b %errorlevel%

rem -----------------------------------------------------------------------------
rem -- Generate C API docs
rem -----------------------------------------------------------------------------
echo Generating C API docs
call gen_cdocs.cmd

rem -----------------------------------------------------------------------------
rem -- done
rem -----------------------------------------------------------------------------
goto :eof

rem -----------------------------------------------------------------------------
rem -- helper subroutines
rem -----------------------------------------------------------------------------
:checkExists
where %~1 >nul 2>nul
if not %errorlevel%==0 (
    echo "%~1" not found. Please make sure that "%~1" is installed and available in the path.
    exit /b %errorlevel%
)
goto :eof

