@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

C:\traceability_tool\traceabilitytool.exe -i "%WORKSPACE%\v1" -buildcheck -e "%WORKSPACE%\v1\deps;%WORKSPACE%\v1\bindings\nodejs;%WORKSPACE%\v1\bindings\dotnetcore\dotnet-core-binding"
if errorlevel 1 goto :eof
