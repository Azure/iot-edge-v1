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

if [ "$build_config" = "0" ]
then
    build_config=
elif [ "$build_config" == "1" ]
then
    exit 1
fi

binding_path="$build_root/bindings/dotnetcore/dotnet-core-binding"
samples_path="$build_root/samples"
sample_modules_path="$samples_path/dotnet_core_module_sample/modules"

projects_to_build="
$binding_path/Microsoft.Azure.Devices.Gateway/Microsoft.Azure.Devices.Gateway.csproj
$binding_path/E2ETestModule/E2ETestModule.csproj
$sample_modules_path/PrinterModule/PrinterModule.csproj
$sample_modules_path/SensorModule/SensorModule.csproj
$samples_path/dotnet_core_managed_gateway/dotnet_core_managed_gateway.csproj"

projects_to_test="$binding_path/Microsoft.Azure.Devices.Gateway.Tests/Microsoft.Azure.Devices.Gateway.Tests.csproj"

# ------------------------------------------------------------------------------
# -- restore
# ------------------------------------------------------------------------------

for project in $projects_to_build $projects_to_test
do
    dotnet restore $project
    [ $? -eq 0 ] || exit $?
done

# ------------------------------------------------------------------------------
# -- build
# ------------------------------------------------------------------------------

for project in $projects_to_build
do
    dotnet build $build_config $project
    [ $? -eq 0 ] || exit $?
done

# ------------------------------------------------------------------------------
# -- test
# ------------------------------------------------------------------------------

for project in $projects_to_test
do
    dotnet test $project
    [ $? -eq 0 ] || exit $?
done