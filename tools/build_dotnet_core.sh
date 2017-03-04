#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

build_root=$(cd "$(dirname "$0")/.." && pwd)

usage ()
{
    echo "build_dotnet_core.sh [options]"
    echo "options"
    echo " --config <value>     Build configuration (e.g. [Debug], Release)"
    exit 1
}

# ------------------------------------------------------------------------------
# parse arguments
# ------------------------------------------------------------------------------

build_config=0

for arg in $*
do
    if [ $build_config == 1 ]
    then
        build_config="--configuration $arg"
    else
        case "$arg" in
            "--config" ) build_config=1;;
            * ) usage;;
        esac
    fi
done

if [ $build_config == 0 ]
then
    build_config=
elif [ $build_config == 1 ]
then
    exit 1
fi

# ------------------------------------------------------------------------------
# build
# ------------------------------------------------------------------------------

dotnet restore \
    $build_root/bindings/dotnetcore/dotnet-core-binding \
    $build_root/samples/dotnet_core_module_sample/modules
[ $? -eq 0 ] || exit $?

dotnet build $build_config \
    $build_root/bindings/dotnetcore/dotnet-core-binding/Microsoft.Azure.Devices.Gateway \
    $build_root/bindings/dotnetcore/dotnet-core-binding/E2ETestModule \
    $build_root/samples/dotnet_core_module_sample/modules/PrinterModule \
    $build_root/samples/dotnet_core_module_sample/modules/SensorModule
[ $? -eq 0 ] || exit $?

# ------------------------------------------------------------------------------
# test
# ------------------------------------------------------------------------------

dotnet test $build_root/bindings/dotnetcore/dotnet-core-binding/Microsoft.Azure.Devices.Gateway.Tests
[ $? -eq 0 ] || exit $?

