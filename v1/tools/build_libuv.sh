#!/bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file
# in the project root for full license information.

set -e

libuv_config=$1
libuv_install=$2
libuv_root=$(cd "$(dirname "$0")/../deps/libuv" && pwd)
make_cores=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)

cd $libuv_root

./autogen.sh
./configure --prefix="$libuv_install" CFLAGS='-fPIC' CXXFLAGS='-fPIC'
make -j $make_cores
make install