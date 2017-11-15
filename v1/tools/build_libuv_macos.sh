#!/bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file
# in the project root for full license information.

set -e

build_root=$(cd "$(dirname "$0")/.." && pwd)
build_root=$build_root/build_libuv

# clear the libuv build folder so we have a fresh build
rm -rf $build_root
mkdir -p $build_root

# build libuv
pushd $build_root

git clone https://github.com/libuv/libuv.git
cd libuv
git checkout -b v1.11.0 tags/v1.11.0

./gyp_uv.py -f xcode
xcodebuild -ARCHS="x86_64" -project uv.xcodeproj -configuration Release -target libuv

# Create a 'dist' folder where the includes/libs live
mkdir -p ../dist/include
cp include/*.h ../dist/include/

mkdir -p ../dist/lib
cp build/Release/libuv.a ../dist/lib/

popd
