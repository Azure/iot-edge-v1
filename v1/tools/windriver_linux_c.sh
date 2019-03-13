#!/bin/bash
# Copyright (c) Microsoft. All rights reserved. Licensed under the MIT 
# license. See LICENSE file in the project root for full license 
# information.

build_root=$(cd "$(dirname "$0")/.." && pwd)
toolchain_root="/opt/windriver/wrlinux/7.0-intel-baytrail-64"

usage ()
{
    echo "windriver_linux_c.sh [options]"
    echo "options"
    echo " --toolchain      Set the toolchain directory location"
    exit 1
}

process_args ()
{
    save_next_arg=0
    extracloptions=" "

    for arg in $*
    do
      if [ $save_next_arg == 1 ]
      then
        # save arg to pass to gcc
        toolchain_root="$arg"
        save_next_arg=0
      else
          case "$arg" in
              "--toolchain" ) save_next_arg=1;;
              * ) usage;;
          esac
      fi
    done
}

process_args $*

# -----------------------------------------------------------------------------
# -- How to build Wind River Linux for our project:
# 1.    Acquire the Wind River Linux toolchain installer (licensed software)
# 2.    Make sure the installer is executable and run it from the console to
#       extract the toolchain (example):
# ./wrlinux-7.0.0.13-glibc-x86_64-intel_baytrail_64-wrlinux-image-idp-sdk.sh
# 3.    By default, will install to a directory like:
#           /opt/windriver/wrlinux/7.0-intel-baytrail-64
#           (see $toolchain_root above)
# 4.    Use env.sh to setup your environment: 
#       . /opt/windriver/wrlinux/7.0-intel-baytrail-64/env.sh
# 5.    Now you can navigate to the SDK build directory and use cmake to 
#       compile as normal. You will notice that gcc and libs from the 
#       windriver sysroot are now used.
# -----------------------------------------------------------------------------

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
   echo ---------- Wind River Linux toolchain absent ----------
   exit 1
fi

# -----------------------------------------------------------------------------
# -- Set up environment
# -----------------------------------------------------------------------------
source $toolchain_root/env.sh

# -----------------------------------------------------------------------------
# -- Create toolchain for CMake
# -----------------------------------------------------------------------------
toolchain_file="$build_root/tools/toolchain-windriver.cmake"
cp "$OECORE_NATIVE_SYSROOT/usr/share/cmake/OEToolchainConfig.cmake" $toolchain_file
cat <<EOM >>$toolchain_file
list(INSERT CMAKE_MODULE_PATH 0 "$build_root/tools/toolchain/windriver/cmake")
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
    -Drun_unittests=OFF \
    -Drun_e2e_tests=OFF \
    -Denable_native_remote_modules=OFF \
    -Drun_valgrind=OFF \
    ..
[ $? -eq 0 ] || exit $?

make --jobs=$(nproc)
[ $? -eq 0 ] || exit $?
