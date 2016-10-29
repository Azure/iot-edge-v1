#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

build_root=$(cd "$(dirname "$0")/../.." && pwd)
cd $build_root

# -----------------------------------------------------------------------------
# -- Generate Node.js binding docs
# -----------------------------------------------------------------------------
doc_target_dir=$build_root/doc/api_reference/node
if [ -d $doc_target_dir ]
then
	rm -rf $doc_target_dir
fi
mkdir -p $doc_target_dir
jsdoc -c ../build/docs/jsdoc-device.conf.json -d $doc_target_dir
