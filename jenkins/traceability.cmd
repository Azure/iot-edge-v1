@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

C:\traceability_tool\traceabilitytool.exe -i "%WORKSPACE%" -buildcheck -e "%WORKSPACE%\deps;%WORKSPACE%\bindings\nodejs"
if errorlevel 1 goto :eof
