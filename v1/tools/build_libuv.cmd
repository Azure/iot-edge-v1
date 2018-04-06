@rem Copyright (c) Microsoft. All rights reserved.
@rem Licensed under the MIT license. See LICENSE file in the project root for full license information.

@setlocal EnableExtensions EnableDelayedExpansion
@echo off

rem Get path to this script, minus trailing slash
set current-path=%~dp0
set current-path=%current-path:~0,-1%

rem Get fully qualified path to libuv source code
set libuv-root=%current-path%\..\deps\libuv
for %%i in ("%libuv-root%") do set libuv-root=%%~fi

rem Process arguments
if "%~1" == "Debug" (
  set libuv-config=debug
) else if "%~1" == "Release" (
  set libuv-config=release
)

if defined libuv-config (
  shift
) else (
  set libuv-config=debug
)

set "libuv-arch=%~1"

rem Build libuv
if defined VisualStudioVersion if "%VisualStudioVersion%"=="15.0" (
  call vcbuild.bat vs2017 %libuv-config% %libuv-arch%
) else (
  rem The following 2 variables must be un-defined because libuv's
  rem vcbuild.bat skips its VS version detection code if these are
  rem defined and defaults to using VS 2012.
  set WindowsSDKDir=
  set VCINSTALLDIR=
  call %libuv-root%\vcbuild.bat %libuv-config% %libuv-arch%
)