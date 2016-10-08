#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -e

build_clean=
build_root=$(cd "$(dirname "$0")/.." && pwd)
log_dir=$build_root
skip_unittests=OFF
run_e2e_tests=ON
run_valgrind=0
enable_java_binding=OFF
enable_nodejs_binding=OFF
toolchainfile=

cd "$build_root"
usage ()
{
    echo "build.sh [options]"
    echo "options"
    echo " -x,  --xtrace                 print a trace of each command"
    echo " -c,  --clean                  remove artifacts from previous build before building"
    echo " -cl, --compileoption <value>  specify a compile option to be passed to gcc"
    echo "   Example: -cl -O1 -cl ..."
    echo " -rv, --run-valgrind           will execute ctest with valgrind"
    echo " --skip-unittests              skip the running of unit tests (unit tests are run by default)"
    echo " --skip-e2e-tests              skip the running of end-to-end tests (e2e tests are run by default)"
    echo " --enable-java-binding         enables building of Java binding; environment variable JAVA_HOME must be defined"
    echo " --enable-nodejs-binding       enables building of Node.js binding; environment variables NODE_INCLUDE and NODE_LIB must be defined"
    echo " --toolchain-file <file>       pass cmake a toolchain file for cross compiling"
    exit 1
}

process_args ()
{
    build_clean=0
    save_next_arg=0
    extracloptions=" "

    for arg in $*
    do
      if [ $save_next_arg == 1 ]
      then
        # save arg to pass to gcc
        extracloptions="$arg $extracloptions"
        save_next_arg=0
      elif [ $save_next_arg == 2 ]
      then
        # save arg to pass as toolchain file
        toolchainfile="$arg"
		save_next_arg=0
      else
          case "$arg" in
              "-x" | "--xtrace" ) set -x;;
              "-c" | "--clean" ) build_clean=1;;
              "--skip-unittests" ) skip_unittests=ON;;
              "--skip-e2e-tests" ) run_e2e_tests=OFF;;
              "-cl" | "--compileoption" ) save_next_arg=1;;
              "-rv" | "--run-valgrind" ) run_valgrind=1;;
              "--enable-java-binding" ) enable_java_binding=ON;;
              "--enable-nodejs-binding" ) enable_nodejs_binding=ON;;
              "--toolchain-file" ) save_next_arg=2;;
              * ) usage;;
          esac
      fi
    done

    if [ -n "$toolchainfile" ]
    then
      toolchainfile=$(readlink -f $toolchainfile)
      toolchainfile="-DCMAKE_TOOLCHAIN_FILE=$toolchainfile"
    fi
}

process_args $*

if [[ $enable_java_binding == ON ]]
then
    "$build_root"/tools/build_java.sh
    [ $? -eq 0 ] || exit $?
fi

cmake_root="$build_root"/build
rm -r -f "$cmake_root"
mkdir -p "$cmake_root"
pushd "$cmake_root"
cmake $toolchainfile \
      -DcompileOption_C:STRING="$extracloptions" \
      -DCMAKE_BUILD_TYPE=Debug \
      -Dskip_unittests:BOOL=$skip_unittests \
      -Drun_e2e_tests:BOOL=$run_e2e_tests \
      -Denable_java_binding:BOOL=$enable_java_binding \
      -Denable_nodejs_binding:BOOL=$enable_nodejs_binding \
      -Drun_valgrind:BOOL=$run_valgrind \
      "$build_root"

CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)

# Make sure there is enough virtual memory on the device to handle more than one job.
# We arbitrarily decide that 500 MB per core is what we need in order to run the build
# in parallel.
MINVSPACE=$(expr 500000 \* $CORES)

# Acquire total memory and total swap space setting them to zero in the event the command fails
MEMAR=( $(sed -n -e 's/^MemTotal:[^0-9]*\([0-9][0-9]*\).*/\1/p' -e 's/^SwapTotal:[^0-9]*\([0-9][0-9]*\).*/\1/p' /proc/meminfo) )
[ -z "${MEMAR[0]##*[!0-9]*}" ] && MEMAR[0]=0
[ -z "${MEMAR[1]##*[!0-9]*}" ] && MEMAR[1]=0

let VSPACE=${MEMAR[0]}+${MEMAR[1]}

if [ "$VSPACE" -lt "$MINVSPACE" ] ; then
  # We think the number of cores to use is a function of available memory divided by 500 MB
  CORES2=$(expr ${MEMAR[0]} / 500000)

  # Clamp the cores to use to be between 1 and $CORES (inclusive)
  CORES2=$([ $CORES2 -le 0 ] && echo 1 || echo $CORES2)
  CORES=$([ $CORES -le $CORES2 ] && echo $CORES || echo $CORES2)
fi

make --jobs=$CORES

if [[ $run_valgrind == 1 ]] ;
then
    #use doctored (-DPURIFY no-asm) openssl
    export LD_LIBRARY_PATH=/usr/local/ssl/lib
    ctest -j $CORES --output-on-failure
    export LD_LIBRARY_PATH=
else
    ctest -j $CORES -C "Debug" --output-on-failure
fi

popd

[ $? -eq 0 ] || exit $?

