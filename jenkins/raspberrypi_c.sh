#!/bin/bash
# Copyright (c) Microsoft. All rights reserved. Licensed under the MIT 
# license. See LICENSE file in the project root for full license 
# information.

# Tested on RPi2 debian verion 7.8

install_root="$HOME" 
build_root=$(cd "$(dirname "$0")/.." && pwd) 
cd $build_root

# ----------------------------------------------------------------------------- 
# -- helper subroutines 
# -----------------------------------------------------------------------------
checkExists() {
    if hash $1 2>/dev/null;
    then
        return 1
    else
        echo "$1" not found. Please make sure that "$1" is installed and available in the path.
        exit 1
    fi
}
# ----------------------------------------------------------------------------- 
# -- Check for environment pre-requisites. This script requires -- that 
# the following programs work: -- curl build-essential(g++,gcc,make) 
# cmake git 
# -----------------------------------------------------------------------------
checkExists curl 
checkExists g++ 
checkExists gcc 
checkExists make 
checkExists cmake 
checkExists git

# ----------------------------------------------------------------------------- 
# -- Check for RPiTools directory.
# -----------------------------------------------------------------------------
if [ ! -d "$install_root/RPiTools" ];
then
   echo ---------- Raspberry Pi tool-chain absent ----------
   exit 1
fi

# ----------------------------------------------------------------------------- 
# -- Set environment variable
# -----------------------------------------------------------------------------
cd $install_root/RPiTools/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/arm-linux-gnueabihf 
export RPI_ROOT=$(pwd)

# ----------------------------------------------------------------------------- 
# -- Create toolchain-rpi.cmake 
# -----------------------------------------------------------------------------
echo ---------- Creating toolchain cmake file ---------- 
FILE="$build_root/toolchain-rpi.cmake" 

/bin/cat <<EOM >$FILE
INCLUDE(CMakeForceCompiler) 

SET(CMAKE_SYSTEM_NAME Linux) # this one is important 
SET(CMAKE_SYSTEM_VERSION 1) # this one not so much

# this is the location of the amd64 toolchain targeting the Raspberry Pi
SET(CMAKE_C_COMPILER ${RPI_ROOT}/../bin/arm-linux-gnueabihf-gcc)
SET(CMAKE_CXX_COMPILER ${RPI_ROOT}/../bin/arm-linux-gnueabihf-g++)

# this is the file system root of the target
SET(CMAKE_FIND_ROOT_PATH ${RPI_ROOT})
SET(CMAKE_SYSROOT ${RPI_ROOT})

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY) 
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY) 
EOM

# -----------------------------------------------------------------------------
# -- Build the SDK
# -----------------------------------------------------------------------------
echo ---------- Building the SDK samples ----------
dependency_install_prefix="-Ddependency_install_prefix=$build_root/install-deps"
cmake_root="$build_root"/build
rm -r -f "$cmake_root"
mkdir -p "$cmake_root"
pushd "$cmake_root"
cmake $dependency_install_prefix -DCMAKE_TOOLCHAIN_FILE=$FILE -DCMAKE_BUILD_TYPE=Debug -Drun_e2e_tests:BOOL=OFF -Drun_unittests:BOOL=OFF -Denable_native_remote_modules:BOOL=OFF -Denable_nodejs_remote_modules:BOOL=OFF -Drun_valgrind:BOOL=OFF -Duse_mqtt:BOOL=OFF "$build_root"
[ $? -eq 0 ] || exit $?

make --jobs=$(nproc)
[ $? -eq 0 ] || exit $?

