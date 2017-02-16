@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

@setlocal EnableExtensions EnableDelayedExpansion
@echo off

set current-path=%~dp0

REM // remove trailing slash
set current-path=%current-path:~0,-1%

set nuget-output=%current-path%\..\build\dist_pkgs\NuGet
set root=%current-path%\..\

REM Resolve to fully qualified path
for %%i in ("%nuget-output%") do set nuget-output=%%~fi
for %%i in ("%root%") do set root=%%~fi

set build-nuspec=%root%dist_pkgs\NuGet

REM Clear the NuGet build folder if it exists
IF EXIST "%nuget-output%" (
  rmdir /s/q %nuget-output%
  if %errorlevel% neq 0 goto :EOF
)

REM Make the output directory for the NuGet packages
mkdir %nuget-output%
if %errorlevel% neq 0 goto :EOF

REM Ensure nuget.exe exists
where /q nuget.exe
if %errorlevel% equ 1 (
@ECHO Installing nuget.exe from https://www.nuget.org/nuget.exe 
Powershell.exe wget -outf nuget.exe https://nuget.org/nuget.exe
    if not exist .\nuget.exe (
        echo nuget does not exist
        exit /b 1
    )
)

REM Build the NuGet packages
nuget pack %build-nuspec%\Microsoft.Azure.Devices.Gateway.Core.nuspec -OutputDirectory %nuget-output%
if %errorlevel% neq 0 goto :EOF

nuget pack %build-nuspec%\Microsoft.Azure.Devices.Gateway.Module.Net.nuspec -OutputDirectory %nuget-output%
if %errorlevel% neq 0 goto :EOF

