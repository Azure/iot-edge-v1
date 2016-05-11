@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

@setlocal EnableExtensions EnableDelayedExpansion
@echo off

set current-path=%~dp0

rem Remove trailing slash
set current-path=%current-path:~0,-1%

set build-root=%current-path%\..\build_nodejs

rem Resolve to fully qualified path
for %%i in ("%build-root%") do set build-root=%%~fi

rem Clear the nodejs build folder so we have a fresh build
rmdir /s/q %build-root%
mkdir %build-root%

pushd %build-root%

rem Clone Node.js
git clone -b enable-shared-build https://github.com/avranju/node.git
pushd node

rem Build Node.js
call vcbuild.bat release nosign enable-shared
popd

rem Create a 'dist' folder where the includes/libs live
mkdir dist\inc\libplatform
copy node\src\env.h dist\inc
copy node\src\env-inl.h dist\inc
copy node\src\node.h dist\inc
copy node\src\node_version.h dist\inc
copy node\deps\uv\include\*.h dist\inc
copy node\deps\v8\include\*.h dist\inc
copy node\deps\v8\include\libplatform\*.h dist\inc\libplatform

mkdir dist\lib
copy node\Release\node.dll dist\lib
copy node\Release\node.lib dist\lib
copy node\Release\node.pdb dist\lib
copy node\Release\lib\*.lib dist\lib
copy node\build\Release\lib\*.lib dist\lib

popd

rem Export environment variables for where the include/lib files can be found
set "NODE_INCLUDE=%build-root%\dist\inc"
set "NODE_LIB=%build-root%\dist\lib"

goto :eof
