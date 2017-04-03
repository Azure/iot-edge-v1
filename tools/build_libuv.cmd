@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

@setlocal EnableExtensions EnableDelayedExpansion
@echo off

set current-path=%~dp0

rem Remove trailing slash
set current-path=%current-path:~0,-1%

set build-root=%current-path%\..\build_libuv

rem Resolve to fully qualified path
for %%i in ("%build-root%") do set build-root=%%~fi

rem Clear the nodejs build folder so we have a fresh build
rmdir /s/q %build-root%
mkdir %build-root%

pushd %build-root%

rem Clone libuv
git clone https://github.com/libuv/libuv.git
pushd libuv
git checkout -b v1.10.1 tags/v1.10.1

rem The following 2 variables must be un-defined because libuv's
rem vcbuild.bat skips its VS version detection code if these are
rem defined and defaults to using VS 2012.
set "WindowsSDKDir_Value=%WindowsSDKDir%"
set WindowsSDKDir=
set "VCINSTALLDIR=%VCINSTALLDIR%"
set VCINSTALLDIR=

rem Build libuv
call vcbuild.bat release %1
popd

rem Create a 'dist' folder where the includes/libs live
mkdir dist\include
copy libuv\include\*.h dist\include

mkdir dist\lib
copy libuv\Release\lib\libuv.lib dist\lib

popd

rem Export environment variables for where the include/lib files can be found
set "LIBUV_INCLUDE=%build-root%\dist\inc"
set "LIBUV_LIB=%build-root%\dist\lib"

rem Set environment variables for where the include/lib files can be found
@endlocal & set LIBUV_INCLUDE=%build-root%\dist\include\ & set LIBUV_LIB=%build-root%\dist\lib\

goto :eof
