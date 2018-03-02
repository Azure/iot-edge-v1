#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.
#

build_root=$(cd "$(dirname "$0")/.." && pwd)
cd $build_root

OPENSSL_ROOT_DIR=/usr/local/opt/openssl \
tools/build.sh \
    --disable-ble-module \
    --enable-java-binding \
    --enable-dotnet-core-binding \
    --enable-java-remote-modules \
    --enable-nodejs-remote-modules \
    --run-unittests \
    "$@" #-x
[ $? -eq 0 ] || exit $?
