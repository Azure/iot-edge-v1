@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

@setlocal EnableExtensions EnableDelayedExpansion
@echo off

set current-path=%~dp0

rem // remove trailing slash
set current-path=%current-path:~0,-1%

set build-root=%current-path%\..\build\dist_pkgs\npm
set root=%current-path%\..\

rem Resolve to fully qualified path
for %%i in ("%build-root%") do set build-root=%%~fi
for %%i in ("%root%") do set root=%%~fi

rem Clear the nodejs build folder so we have a fresh build
rmdir /s/q %build-root%
mkdir %build-root%

pushd %build-root%

rem Copy package definition and README files into npm folder.
xcopy %root%\dist_pkgs\npm .\ /S /Q
copy %root%\LICENSE.txt az-iot-gw\LICENSE
copy %root%\LICENSE.txt az-iot-gw-module-js\LICENSE

rem copy binary files for azure iot gateway.
mkdir az-iot-gw\bin\win
copy %root%\build\core\Debug\gateway.dll az-iot-gw\bin\win
copy %root%\install-deps\bin\aziotsharedutil.dll az-iot-gw\bin\win
copy %root%\install-deps\bin\\nanomsg.dll az-iot-gw\bin\win
copy %root%\build_nodejs\node\Release\node.dll az-iot-gw\bin\win
copy %root%\build\dist_pkgs\gw\Debug\gw.exe az-iot-gw\bin\win


rem copy files for azure iot gateway module development.
mkdir az-iot-gw-module-js\bindings\win
copy %root%\build\bindings\nodejs\Debug\nodejs_binding.dll az-iot-gw-module-js\bindings\win
mkdir az-iot-gw-module-js\modules\native\win
copy %root%\build\modules\logger\Debug\logger.dll az-iot-gw-module-js\modules\native\win
mkdir az-iot-gw-module-js\samples\simple\modules
xcopy %root%\samples\nodejs_simple_sample\nodejs_modules az-iot-gw-module-js\samples\simple\modules /S /Q


popd

goto :eof
