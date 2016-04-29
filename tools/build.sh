#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -e

build_clean=
build_root=$(cd "$(dirname "$0")/.." && pwd)
log_dir=$build_root
run_unit_tests=ON
run_e2e_tests=ON
run_valgrind=0
cd "$build_root"
usage ()
{
    echo "build.sh [options]"
    echo "options"
    echo " -x,  --xtrace                 print a trace of each command"
    echo " -c,  --clean                  remove artifacts from previous build be
fore building"
    echo " -cl, --compileoption <value>  specify a compile option to be passed to gcc"
    echo "   Example: -cl -O1 -cl ..."
    echo "-rv, --run_valgrind will execute ctest with valgrind"
    echo " --skip-e2e-tests              skip the running of end-to-end tests (e2e tests are run by default)"
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
        extracloptions="$extracloptions $arg"
        save_next_arg=0
      else
          case "$arg" in
              "-x" | "--xtrace" ) set -x;;
              "-c" | "--clean" ) build_clean=1;;
              "--skip-e2e-tests" ) run_e2e_tests=OFF;;
              "-cl" | "--compileoption" ) save_next_arg=1;;
              "-rv" | "--run_valgrind" ) run_valgrind=1;;
              * ) usage;;
          esac
      fi
    done
}

process_args $*

cmake_root="$build_root"/build
rm -r -f "$cmake_root"
mkdir -p "$cmake_root"
pushd "$cmake_root"
cmake -DCMAKE_BUILD_TYPE=Debug -Drun_e2e_tests:BOOL=$run_e2e_tests -Drun_valgrind:BOOL=$run_valgrind "$build_root"
make --jobs=$(nproc)



if [[ $run_valgrind == 1 ]] ;
then
    #use doctored (-DPURIFY no-asm) openssl
    export LD_LIBRARY_PATH=/usr/local/ssl/lib
    ctest -j $(nproc) --output-on-failure
    export LD_LIBRARY_PATH=
else
    ctest -j $(nproc) -C "Debug" --output-on-failure
fi

popd

[ $? -eq 0 ] || exit $?

