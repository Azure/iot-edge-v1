@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

@REM Bin place .NET dll's for CodeSign

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

REM Make the source and output directories for the NuGet packages
mkdir %csu-source%
if %errorlevel% neq 0 goto :EOF

mkdir %csu-output%
if %errorlevel% neq 0 goto :EOF

xcopy /q /y /R %root%bindings\dotnet\dotnet-binding\PrinterModule\bin\x86\Release_Delay_Sign\PrinterModule.dll %csu-source%
if %errorlevel% neq 0  goto :EOF

xcopy /q /y /R %root%bindings\dotnet\dotnet-binding\SensorModule\bin\x86\Release_Delay_Sign\SensorModule.dll %csu-source%
if %errorlevel% neq 0  goto :EOF

xcopy /q /y /R %root%bindings\dotnet\dotnet-binding\Microsoft.Azure.IoT.Gateway\bin\x86\Release_Delay_Sign\Microsoft.Azure.IoT.Gateway.dll %csu-source%
if %errorlevel% neq 0  goto :EOF

REM Auto-sign the managed dlls placed in the "tosign" Folder 
csu.exe /s=True /w=True /i=%root%build\ToSign /o=%root%build\Signed /c1=72 /c2=401 /d="SN Signing Azure IoT SDK .NET Gateway binaries"
if %errorlevel% neq 0 goto :EOF

xcopy /q /y /R %csu-output%\PrinterModule.dll %root%bindings\dotnet\dotnet-binding\PrinterModule\bin\x86\Release_Delay_Sign 
if %errorlevel% neq 0  goto :EOF

xcopy /q /y /R %csu-output%\SensorModule.dll %root%bindings\dotnet\dotnet-binding\SensorModule\bin\x86\Release_Delay_Sign
if %errorlevel% neq 0  goto :EOF

xcopy /q /y /R %csu-output%\Microsoft.Azure.IoT.Gateway.dll %root%bindings\dotnet\dotnet-binding\Microsoft.Azure.IoT.Gateway\bin\x86\Release_Delay_Sign
if %errorlevel% neq 0  goto :EOF

REM Clean up the directories
rmdir /s/q %csu-source%
rmdir /s/q %csu-output%