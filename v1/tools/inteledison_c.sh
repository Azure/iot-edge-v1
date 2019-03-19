#!/bin/bash
# Copyright (c) Microsoft. All rights reserved. Licensed under the MIT 
# license. See LICENSE file in the project root for full license 
# information.

build_root=$(cd "$(dirname "$0")/.." && pwd)
toolchain_root="/opt/poky-edison"

# -----------------------------------------------------------------------------
# -- helper subroutines
# -----------------------------------------------------------------------------
checkExists() {
    if hash $1 2>/dev/null;
    then
        return 0
    else
        echo "$1" not found. Please make sure that "$1" is installed and available in the path.
        exit 1
    fi
}

# -----------------------------------------------------------------------------
# -- Check for environment prerequisites
# -----------------------------------------------------------------------------
checkExists curl
checkExists g++
checkExists gcc
checkExists make
checkExists cmake
checkExists git

# -----------------------------------------------------------------------------
# -- Check for existence of toolchain
# -----------------------------------------------------------------------------
if [ ! -d $toolchain_root ];
then
   echo ---------- Intel Edison toolchain absent ----------
   exit 1
fi

# -----------------------------------------------------------------------------
# -- Set up environment
# -----------------------------------------------------------------------------
toolchain_root="$toolchain_root/$(ls $toolchain_root)"
source $toolchain_root/$(ls $toolchain_root | grep environment)
sysroot="$toolchain_root/sysroots/core2-32-poky-linux"
toolroot="$toolchain_root/sysroots/x86_64-pokysdk-linux"

# -----------------------------------------------------------------------------
# -- Create the toolchain for CMake
# -----------------------------------------------------------------------------
echo ---------- Creating toolchain cmake file ----------
toolchain_file="$build_root/tools/toolchain-inteledison.cmake"

/bin/cat <<EOM >$toolchain_file
INCLUDE(CMakeForceCompiler)

SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)

# this is the location of the amd64 toolchain targeting the Intel Edison
SET(CMAKE_C_COMPILER $toolroot/usr/bin/i586-poky-linux/i586-poky-linux-gcc)
SET(CMAKE_CXX_COMPILER $toolroot/usr/bin/i586-poky-linux/i586-poky-linux-g++)

# this is the file system root of the target
SET(CMAKE_FIND_ROOT_PATH $sysroot)
SET(CMAKE_SYSROOT $sysroot)

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
cd $build_root
rm -rf build/
mkdir -p build
[ $? -eq 0 ] || exit $?
cd build
cmake \
    -DCMAKE_TOOLCHAIN_FILE=$toolchain_file \
    -DCMAKE_BUILD_TYPE=Debug \
    -Ddependency_install_prefix=$build_root/install-deps \
    -Drun_e2e_tests=OFF \
    -Drun_unittests=OFF \
    -Denable_native_remote_modules=OFF \
    -Drun_valgrind=OFF \
    ..
[ $? -eq 0 ] || exit $?

make --jobs=$(nproc)
[ $? -eq 0 ] || exit $?
