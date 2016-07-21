@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

@setlocal EnableExtensions EnableDelayedExpansion
@echo off

set build-root=%~dp0..\..\
cd %build-root%

set gh_pages_dir=%temp%\gw_temp_gh_pages

rem ---------------------------------------------------------------------------
rem -- Make sure current git branch is 'develop'. If it isn't check out that
rem -- branch.
rem ---------------------------------------------------------------------------
for /f "delims=" %%i in ('git rev-parse --abbrev-ref HEAD') do set current_git_branch=%%i
if "%current_git_branch%" neq "develop" (
	echo Switching git branch to 'develop'
	git checkout develop
	if !ERRORLEVEL! neq 0 exit /b !ERRORLEVEL!
)

rem ---------------------------------------------------------------------------
rem -- Checking out gh-pages branch to temporary location
rem ---------------------------------------------------------------------------
if exist %gh_pages_dir% rd /s /q %gh_pages_dir%
if !ERRORLEVEL! neq 0 exit /b !ERRORLEVEL!
mkdir %gh_pages_dir%
cd %temp%
git clone -b gh-pages https://github.com/Azure/azure-iot-gateway-sdk %gh_pages_dir%

rem ---------------------------------------------------------------------------
rem -- Put docs into temp directory
rem ---------------------------------------------------------------------------
call %build-root%\tools\gen_docs.cmd
echo robocopy /E "%build-root%doc\api_reference" "%gh_pages_dir%\api_reference"
robocopy /E "%build-root%doc\api_reference" "%gh_pages_dir%\api_reference"
