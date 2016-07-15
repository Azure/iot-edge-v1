#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

build_root=$(cd "$(dirname "$0")/../.." && pwd)

# -----------------------------------------------------------------------------
# -- Generate Java binding docs
# -----------------------------------------------------------------------------
echo Generating Java Device SDK docs... 
cd $build_root/bindings/java/gateway-java-binding
mvn -q javadoc:javadoc
if [ $? -ne 0 ]
then
    echo Generating java docs for gateway-java-binding failed.
    exit $?
fi

# Move the generated docs to $build_root/doc/api_reference/java
doc_target_dir=$build_root/doc/api_reference/java
echo Copying Java binding API docs to $doc_target_dir
if [ -d $doc_target_dir ]
then
	rm -rf $doc_target_dir
fi
mkdir -p $doc_target_dir
cp -r target/site/apidocs/* $doc_target_dir
