#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -e

build_root=$(cd "$(dirname "$0")/.." && pwd)
local_install=$build_root/install-deps
log_dir=$build_root
CORES=0
rebuild_deps=OFF
run_unittests=OFF
run_e2e_tests=OFF
run_valgrind=0
enable_java_binding=OFF
enable_dotnet_core_binding=OFF
enable_nodejs_binding=OFF
enable_native_remote_modules=ON
enable_nodejs_remote_modules=OFF
enable_java_remote_modules=OFF
toolchainfile=
dependency_install_prefix="-Ddependency_install_prefix=$local_install"
build_config=Debug
use_xplat_uuid=OFF
if [[ $(uname -s) == Darwin ]]
then
    # Don't build BLE for macOS, even if the caller doesn't pass `--disable-ble-module`
    enable_ble_module=OFF
else
    enable_ble_module=ON
fi

usage ()
{
    echo "build.sh [options]"
    echo "options"
    echo " -cl, --compileoption <val>      Specify a gcc compile option"
    echo "   Example: -cl -O1 -cl ..."
    echo " -f,  --config <value>           Build configuration (e.g. [Debug], Release)"
    echo " --disable-ble-module            Do not build the BLE module"
    echo " --enable-dotnet-core-binding    Build the .NET Core binding"
    echo " --enable-java-binding           Build Java binding"
    echo "                                 (JAVA_HOME must be defined in your environment)"
    echo " --enable-nodejs-binding         Build Node.js binding"
    echo "                                 (NODE_INCLUDE, NODE_LIB must be defined)"
    echo " --disable-native-remote-modules Do not build the infrastructure"
    echo "                                 required to support native remote modules"
    echo " --enable-nodejs-remote-modules  Build the infrastructure required to support"
    echo "                                 Node.js apps as remote modules"
    echo " --enable-java-remote-modules    Build Java Remote Module SDK"
    echo "                                 (JAVA_HOME must be defined in your environment)"
    echo " --rebuild-deps                  Force rebuild of dependencies"
    echo " --run-e2e-tests                 Build/run end-to-end tests"
    echo " --run-unittests                 Build/run unit tests"
    echo " -rv,  --run-valgrind            Execute ctest with valgrind"
    echo " --system-deps-path              Search for dependencies in a system-level location,"
    echo "                                 e.g. /usr/local, and install if not found. When this"
    echo "                                 option is omitted the path is $local_install."
    echo " --toolchain-file <file>         Pass CMake a toolchain file for cross-compiling"
    echo " --use-xplat-uuid                Use SDK's platform-independent UUID implementation"
    echo " -x,  --xtrace                   Print a trace of each command"
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
        extracloptions="$arg $extracloptions"
        save_next_arg=0
      elif [ $save_next_arg == 2 ]
      then
        # save arg to pass as toolchain file
        toolchainfile="$arg"
        save_next_arg=0
      elif [ $save_next_arg == 3 ]
      then
        # save build configuration
        build_config="$arg"
        save_next_arg=0
      else
          case "$arg" in
              "-x" | "--xtrace" ) set -x;;
              "--run-unittests" ) run_unittests=ON;;
              "--run-e2e-tests" ) run_e2e_tests=ON;;
              "--rebuild-deps" ) rebuild_deps=ON;;
              "-cl" | "--compileoption" ) save_next_arg=1;;
              "-rv" | "--run-valgrind" ) run_valgrind=1;;
              "--enable-java-binding" ) enable_java_binding=ON;;
              "--enable-dotnet-core-binding" ) enable_dotnet_core_binding=ON;;
              "--enable-nodejs-binding" ) enable_nodejs_binding=ON;;
              "--disable-native-remote-modules" ) enable_native_remote_modules=OFF;;
              "--enable-nodejs-remote-modules" ) enable_nodejs_remote_modules=ON;;
              "--enable-java-remote-modules" ) enable_java_remote_modules=ON;;
              "--disable-ble-module" ) enable_ble_module=OFF;;
              "--toolchain-file" ) save_next_arg=2;;
              "--system-deps-path" ) dependency_install_prefix=;;
              "-f" | "--config" ) save_next_arg=3;;
              "--use-xplat-uuid" ) use_xplat_uuid=ON;;
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

get_cores ()
{
    CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)

    # The following logic works on Linux, doesn't work on macOS. For now, we'll assume macOS
    # devices have enough memory per core to run the build, rather than porting this logic.
    if [[ $(uname -s) != Darwin ]]
    then
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
            CORES2=$(expr ${MEMAR[0]} / 500000) || true

            # Clamp the cores to use to be between 1 and $CORES (inclusive)
            CORES2=$([ $CORES2 -le 0 ] && echo 1 || echo $CORES2)
            CORES=$([ $CORES -le $CORES2 ] && echo $CORES || echo $CORES2)
        fi
    fi
}

process_args $*

if [[ $enable_java_binding == ON ]]
then
    "$build_root"/tools/build_java.sh
    [ $? -eq 0 ] || exit $?
fi

if [[ $enable_java_remote_modules == ON ]]
then 
    if [[ $enable_java_binding == OFF ]]
    then
        "$build_root"/tools/build_java.sh
        [ $? -eq 0 ] || exit $?
    fi

     "$build_root"/tools/build_java_oop.sh
    [ $? -eq 0 ] || exit $?
fi

if [[ $enable_nodejs_remote_modules == ON ]]
then 
    pushd "$build_root/samples/nodejs_remote_sample"
    [ $? -eq 0 ] || exit $?
    npm install
    [ $? -eq 0 ] || exit $?
    npm run lint
    [ $? -eq 0 ] || exit $?
    popd

    pushd "$build_root/proxy/gateway/nodejs"
    [ $? -eq 0 ] || exit $?
    npm install
    [ $? -eq 0 ] || exit $?
    npm run lint
    [ $? -eq 0 ] || exit $?
    npm test
    [ $? -eq 0 ] || exit $?
    popd
fi

if [[ $enable_dotnet_core_binding == ON ]]
then
    "$build_root"/tools/build_dotnet_core.sh --config $build_config
    [ $? -eq 0 ] || exit $?
fi

get_cores

cd "$build_root"

cmake_root="$build_root/build"
rm -r -f "$cmake_root"
git submodule foreach --recursive --quiet "rm -r -f build/"
mkdir -p "$cmake_root"
pushd "$cmake_root"

cmake $toolchainfile \
      $dependency_install_prefix \
      -DcompileOption_C:STRING="$extracloptions" \
      -DCMAKE_BUILD_TYPE="$build_config" \
      -Drun_unittests:BOOL=$run_unittests \
      -Drun_e2e_tests:BOOL=$run_e2e_tests \
      -Denable_java_binding:BOOL=$enable_java_binding \
      -Denable_dotnet_core_binding:BOOL=$enable_dotnet_core_binding \
      -Denable_nodejs_binding:BOOL=$enable_nodejs_binding \
      -Denable_native_remote_modules:BOOL=$enable_native_remote_modules \
      -Denable_nodejs_remote_modules:BOOL=$enable_nodejs_remote_modules \
      -Denable_java_remote_modules:BOOL=$enable_java_remote_modules \
      -Denable_ble_module:BOOL=$enable_ble_module \
      -Drun_valgrind:BOOL=$run_valgrind \
      -Dbuild_cores=$CORES \
      -Drebuild_deps:BOOL=$rebuild_deps \
      -Duse_xplat_uuid:BOOL=$use_xplat_uuid \
      "$build_root"

make --jobs=$CORES

if [[ "$run_unittests" == "ON" || "$run_e2e_tests" == "ON" ]]
then
    if [[ $run_valgrind == 1 ]]
    then
        #use doctored (-DPURIFY no-asm) openssl
        export LD_LIBRARY_PATH=/usr/local/ssl/lib
        ctest -V -j $CORES --output-on-failure
        export LD_LIBRARY_PATH=
    else
        ctest -V -j $CORES -C $build_config --output-on-failure
    fi
fi

popd

[ $? -eq 0 ] || exit $?

