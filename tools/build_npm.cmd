@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

@setlocal EnableExtensions EnableDelayedExpansion
@echo off

set current-path=%~dp0

rem // remove trailing slash
set current-path=%current-path:~0,-1%

set build-root=%current-path%\..\build_packages
set root=%current-path%\..\

rem Resolve to fully qualified path
for %%i in ("%build-root%") do set build-root=%%~fi
for %%i in ("%root%") do set root=%%~fi

rem Clear the nodejs build folder so we have a fresh build
rmdir /s/q %build-root%
mkdir %build-root%

pushd %build-root%

rem Copy package definition and README files into npm folder.
mkdir npm
xcopy %root%\pkgs\npm npm /S /Q

rem copy binary files for azure iot gateway core runtime.
mkdir npm\az-iot-gw-core\core
copy %root%\build\core\Debug\gateway.dll npm\az-iot-gw-core\core
copy %root%\install-deps\bin\aziotsharedutil.dll npm\az-iot-gw-core\core
copy %root%\install-deps\bin\\nanomsg.dll npm\az-iot-gw-core\core
copy %root%\build_nodejs\node\Release\node.dll npm\az-iot-gw-core\core
copy %root%\build\bindings\nodejs\Debug\nodejs_binding.dll npm\az-iot-gw-core\core

popd

goto :eof