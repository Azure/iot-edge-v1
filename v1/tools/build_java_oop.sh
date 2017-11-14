#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -e

build_root=$(cd "$(dirname "$0")/.." && pwd)

# -- Java Remote Module SDK --
pushd $build_root/proxy/gateway/java/gateway-remote-module

mvn clean install
[ $? -eq 0 ] || exit $?
popd