@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

@setlocal EnableExtensions EnableDelayedExpansion
@echo off

set current-path=%~dp0

rem // remove trailing slash
set current-path=%current-path:~0,-1%

set build-root=%current-path%\..\build\dist_pkgs
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
xcopy %root%\dist_pkgs\npm npm /S /Q
copy %root%\LICENSE.txt npm\az-iot-gw\LICENSE
copy %root%\LICENSE.txt npm\az-iot-gw-module-js\LICENSE

rem copy binary files for azure iot gateway.
mkdir npm\az-iot-gw\bin\win
copy %root%\build\core\Debug\gateway.dll npm\az-iot-gw\bin\win
copy %root%\install-deps\bin\aziotsharedutil.dll npm\az-iot-gw\bin\win
copy %root%\install-deps\bin\\nanomsg.dll npm\az-iot-gw\bin\win
copy %root%\build_nodejs\node\Release\node.dll npm\az-iot-gw\bin\win
copy %root%\build\samples\nodejs_simple_sample\Debug\nodejs_simple_sample.exe npm\az-iot-gw\bin\win\gw.exe


rem copy files for azure iot gateway module development.
mkdir npm\az-iot-gw-module-js\bindings\win
copy %root%\build\bindings\nodejs\Debug\nodejs_binding.dll npm\az-iot-gw-module-js\bindings\win
mkdir npm\az-iot-gw-module-js\modules\native\win
copy %root%\build\modules\logger\Debug\logger.dll npm\az-iot-gw-module-js\modules\native\win
mkdir npm\az-iot-gw-module-js\samples\simple\modules
xcopy %root%\samples\nodejs_simple_sample\nodejs_modules npm\az-iot-gw-module-js\samples\simple\modules /S /Q


popd

goto :eof