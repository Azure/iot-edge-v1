@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

@REM Codesign Authenticode SHA2 digital certs for Azure IoT SDK core dll's

@setlocal EnableExtensions EnableDelayedExpansion
@echo off

set current-path=%~dp0

REM // remove trailing slash
set current-path=%current-path:~0,-1%

set csu-source=%current-path%\..\build\ToSign
set csu-output=%current-path%\..\build\Signed
set root=%current-path%\..\

REM Resolve to fully qualified path
for %%i in ("%csu-source%") do set csu-source=%%~fi
for %%i in ("%csu-output%") do set csu-output=%%~fi
for %%i in ("%root%") do set root=%%~fi

REM Clear the source folder if it exists
IF EXIST "%csu-source%" (
  rmdir /s/q %csu-source%
  if %errorlevel% neq 0 goto :EOF
)

IF EXIST "%csu-output%" (
  rmdir /s/q %csu-output%
  if %errorlevel% neq 0 goto :EOF
)

REM Make the source and output directories
mkdir %csu-source%
if %errorlevel% neq 0 goto :EOF

mkdir %csu-output%
if %errorlevel% neq 0 goto :EOF

REM Copy files to be signed to the tosign folder
xcopy /q /y /R %root%build\bindings\dotnet\Release\dotnet.dll %csu-source%
if %errorlevel% neq 0  goto :EOF

xcopy /q /y /R %root%build\bindings\java\Release\java_module_host.dll %csu-source%
if %errorlevel% neq 0  goto :EOF

xcopy /q /y /R %root%build_nodejs\node\Release\node.dll %csu-source%
if %errorlevel% neq 0  goto :EOF

xcopy /q /y /R %root%build\bindings\nodejs\Release\nodejs_binding.dll %csu-source%
if %errorlevel% neq 0  goto :EOF

xcopy /q /y /R %root%build\dist_pkgs\gw\Release\gw.exe %csu-source%
if %errorlevel% neq 0  goto :EOF

xcopy /q /y /R %root%build\core\Release\gateway.dll %csu-source%
if %errorlevel% neq 0  goto :EOF

xcopy /q /y /R %root%install-deps\bin\aziotsharedutil.dll %csu-source% 
if %errorlevel% neq 0  goto :EOF

xcopy /q /y /R %root%install-deps\bin\nanomsg.dll %csu-source%
if %errorlevel% neq 0  goto :EOF

REM Auto-sign the dlls placed in the "tosign" Folder 
call csu.exe /s=True /w=True /i=%root%build\ToSign /o=%root%build\Signed /c1=401 /d="Signing Azure IoT SDK Core Gateway binaries"
if %errorlevel% neq 0 goto :EOF

REM Copy the siged dlls from the signed folder to their original locations
xcopy /q /y /R %csu-output%\dotnet.dll build\bindings\dotnet\Release
if %errorlevel% neq 0  goto :EOF

xcopy /q /y /R %csu-output%\java_module_host.dll %root%build\bindings\java\Release
if %errorlevel% neq 0  goto :EOF

xcopy /q /y /R %csu-output%\node.dll %root%build_nodejs\node\Release
if %errorlevel% neq 0  goto :EOF

xcopy /q /y /R %csu-output%\nodejs_binding.dll %root%build\bindings\nodejs\Release
if %errorlevel% neq 0  goto :EOF

xcopy /q /y /R %csu-output%\gw.exe %root%build\dist_pkgs\gw\Release
if %errorlevel% neq 0  goto :EOF

xcopy /q /y /R %csu-output%\gateway.dll %root%build\core\Release
if %errorlevel% neq 0  goto :EOF

xcopy /q /y /R %csu-output%\aziotsharedutil.dll %root%install-deps\bin 
if %errorlevel% neq 0  goto :EOF

xcopy /q /y /R %csu-output%\nanomsg.dll %root%install-deps\bin
if %errorlevel% neq 0  goto :EOF

REM Clean up the directories
rmdir /s/q %csu-source%
rmdir /s/q %csu-output%
