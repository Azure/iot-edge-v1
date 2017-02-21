#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

build_root=$(cd "$(dirname "$0")/.." && pwd)

cd $build_root/bindings/dotnetcore/dotnet-core-binding

dotnet restore
[ $? -eq 0 ] || exit $?

dotnet build ./Microsoft.Azure.IoT.Gateway ./PrinterModule ./SensorModule ./E2ETestModule
[ $? -eq 0 ] || exit $?

cd $build_root/bindings/dotnetcore/dotnet-core-binding/Microsoft.Azure.IoT.Gateway.Tests

dotnet test
[ $? -eq 0 ] || exit $?

