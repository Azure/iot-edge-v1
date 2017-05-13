# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

[CmdletBinding(PositionalBinding=$false)]
Param(
    # Version of Node.js to build
    [Alias("node-version")]
    [string]
    $NodeVersion="v6.10.1",

    # Other params to be passed to Node.js build script
    [Parameter(ValueFromRemainingArguments=$true)]
    [string[]]
    $BuildParams
)

# Get source root folder.
$buildRoot = [System.IO.Path]::GetFullPath(
    [System.IO.Path]::Combine($PSScriptRoot, "..\build_nodejs")
)

Write-Output "Building Node.js version $NodeVersion."

# Clear the nodejs build folder so we have a fresh build.
if(Test-Path -Path $buildRoot) {
    Write-Output "Deleting previous build."
    Remove-Item -Recurse -Force $buildRoot
}
New-Item -ItemType Directory -Force -Path $buildRoot > $null

Push-Location -Path $buildRoot

# Clone Node.js
Write-Output "Cloning Node.js version $NodeVersion".
git clone -b $NodeVersion --depth 1 https://github.com/nodejs/node.git

# Build Node.js
Write-Output "Building Node.js version $NodeVersion with build params $BuildParams."
Push-Location -Path (Join-Path -Path $buildRoot -ChildPath node)
cmd.exe /C "vcbuild.bat release nosign dll $BuildParams"
if(-not $?) {
    Write-Output "Node.js build script failed."
    Pop-Location
    Return
}
Pop-Location

# Create a 'dist' folder where the includes/libs live
Write-Output "Copying build to dist folder."
New-Item -ItemType Directory -Path dist\inc\libplatform > $null
Copy-Item node\src\env.h dist\inc
Copy-Item node\src\env-inl.h dist\inc
Copy-Item node\src\node.h dist\inc
Copy-Item node\src\node_version.h dist\inc
Copy-Item node\deps\uv\include\*.h dist\inc
Copy-Item node\deps\v8\include\*.h dist\inc
Copy-Item node\deps\v8\include\libplatform\*.h dist\inc\libplatform

New-Item -ItemType Directory -Path dist\lib > $null
Copy-Item node\Release\node.dll dist\lib
Copy-Item node\Release\node.lib dist\lib
Copy-Item node\Release\node.pdb dist\lib
Copy-Item node\Release\lib\*.lib dist\lib
Copy-Item node\build\Release\lib\*.lib dist\lib

Pop-Location # Undo Push-Location -Path $buildRoot

Write-Output "`nNode JS has been built and the includes and library files are in:`n"
Write-Output "`t$buildRoot\dist"
Write-Output "`nSet the following variables so that the Azure IoT Gateway SDK build scripts can find these files."
Write-Output "`n`tset `"NODE_INCLUDE=$buildRoot\dist\inc`""
Write-Output "`tset `"NODE_LIB=$buildRoot\dist\lib`"`n"
