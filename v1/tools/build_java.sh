#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

build_root=$(cd "$(dirname "$0")/.." && pwd)

# -- Java Device Client --
cd $build_root/bindings/java/gateway-java-binding
mvn clean install
[ $? -eq 0 ] || exit $?