#!/bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file
# in the project root for full license information.

set -e

libuv_config=$1
libuv_root=$(cd "$(dirname "$0")/../deps/libuv" && pwd)

$libuv_root/gyp_uv.py -f xcode
xcodebuild -ARCHS="x86_64" -project $libuv_root/uv.xcodeproj -configuration $libuv_config -target libuv