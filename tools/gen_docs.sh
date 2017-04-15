#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -e

build_root=$(cd "$(dirname "$0")/.." && pwd)
cd $build_root/tools/docs

# -----------------------------------------------------------------------------
# -- helper subroutines
# -----------------------------------------------------------------------------
checkExists() {
    if hash $1 2>/dev/null;
    then
        return 1
    else
        echo "$1" not found. Please make sure that "$1" is installed and available in the path.
        exit 1
    fi
}

# -----------------------------------------------------------------------------
# -- Check for environment pre-requisites. This script requires
# -- that the following programs work:
# --     doxygen
# -----------------------------------------------------------------------------
checkExists git
checkExists node
checkExists npm
checkExists doxygen
checkExists mvn
checkExists javadoc
checkExists jsdoc

# -----------------------------------------------------------------------------
# -- Generating docs
# -----------------------------------------------------------------------------

echo Generating Gateway Core API docs
./gen_cdocs.sh

echo Generating Java Module API docs
./gen_javadocs.sh

echo Generating Node.js Module API docs
./gen_jsdocs.sh

echo Generating .NET Core Module API docs
./gen_dotnetcoredocs.sh
