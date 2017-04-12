#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.
#

set -e

script_dir=$(cd "$(dirname "$0")" && pwd)
build_root=$(cd "${script_dir}/.." && pwd)
build_folder=$build_root"/build"

CORES=$(sysctl -n hw.ncpu)

# Java binding
pushd $build_root/bindings/java/gateway-java-binding
mvn clean install
popd

# Java proxy gateway
pushd $build_root/proxy/gateway/java/gateway-remote-module
mvn clean install
popd

rm -rf $build_folder
mkdir -p $build_folder
pushd $build_folder

cmake \
    -Ddependency_install_prefix:PATH=$build_root/install-deps \
    -DOPENSSL_ROOT_DIR:PATH=/usr/local/opt/openssl \
    -Dbuild_cores=$CORES \
    -Denable_ble_module:BOOL=OFF \
    -Denable_java_binding=ON \
    -Denable_java_remote_modules:BOOL=ON \
    -Drun_unittests:BOOL=ON \
    -Drun_e2e_tests:BOOL=ON \
    ..

cmake --build . -- --jobs=$CORES
ctest -C "debug" -V
popd
