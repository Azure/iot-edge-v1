#!/bin/bash
# Copyright (c) Microsoft. All rights reserved. Licensed under the MIT 
# license. See LICENSE file in the project root for full license 
# information.

toolchain_root="/opt/windriver/wrlinux/7.0-intel-baytrail-64"
build_root=$(cd "$(dirname "$0")/.." && pwd)
local_install=$build_root/install-deps
target=corei7-64-wrs-linux
native=x86_64-wrlinuxsdk-linux
OECORE_TARGET_SYSROOT=${toolchain_root}/sysroots/${target}
OECORE_NATIVE_SYSROOT=${toolchain_root}/sysroots/${native}
cd $build_root

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
        return 1
    else
        echo "$1" not found. Please make sure that "$1" is installed and available in the path.
        exit 1
    fi
}

# -----------------------------------------------------------------------------
# -- Check for environment pre-requisites.
# -- This script requires that the following programs work:
#    -- curl build-essential(g++,gcc,make) cmake git
# -----------------------------------------------------------------------------
checkExists curl
checkExists g++
checkExists gcc
checkExists make
checkExists cmake
checkExists git

# -----------------------------------------------------------------------------
# -- Check for existence of tool-chain.
# -----------------------------------------------------------------------------
if [ ! -d $toolchain_root ];
then
   echo ---------- Wind River linux tool-chain absent ----------
   exit 1
fi

# -----------------------------------------------------------------------------
# -- Set environment variables
# -----------------------------------------------------------------------------
#source $toolchain_root/env.sh

FILE="$build_root/toolchain-wrl.cmake" 

/bin/cat <<EOM >$FILE
SET(CMAKE_SYSTEM_NAME Linux) 

SET(CMAKE_C_COMPILER ${OECORE_NATIVE_SYSROOT}/usr/bin/x86_64-wrs-linux/x86_64-wrs-linux-gcc)
SET(CMAKE_CXX_COMPILER ${OECORE_NATIVE_SYSROOT}/usr/bin/x86_64-wrs-linux/x86_64-wrs-linux-g++)

# this is the file system root of the target
SET(CMAKE_FIND_ROOT_PATH ${OECORE_TARGET_SYSROOT} ${OECORE_NATIVE_SYSROOT})
SET(CMAKE_SYSROOT ${OECORE_TARGET_SYSROOT})

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY) 
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY) 
list(APPEND CMAKE_MODULE_PATH "${build_root}/tools/toolchain/windriver/cmake")

#set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS ON)
#set( CMAKE_SYSTEM_PROCESSOR corei7)
EOM
echo "---------- toolchain file --------------"

cat $FILE
echo "---------- toolchain file --------------"
TOOLCHAIN_OPTION="-DCMAKE_TOOLCHAIN_FILE=$FILE"
# -----------------------------------------------------------------------------
# -- After the environment is set up, we can run cmake.
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# -- Build the SDK
# -----------------------------------------------------------------------------
echo ---------- Building the SDK samples ----------
cmake_root="$build_root"/build
rm -r -f "$cmake_root"
mkdir -p "$cmake_root"
pushd "$cmake_root"
cmake $TOOLCHAIN_OPTION -Ddependency_install_prefix=$local_install -DCMAKE_BUILD_TYPE=Debug -Drun_unittests:BOOL=OFF -Drun_e2e_tests:BOOL=OFF -Denable_native_remote_modules:BOOL=OFF -Denable_nodejs_remote_modules:BOOL=OFF -Drun_valgrind:BOOL=OFF "$build_root"
[ $? -eq 0 ] || exit $?

make --jobs=$(nproc)
[ $? -eq 0 ] || exit $?
