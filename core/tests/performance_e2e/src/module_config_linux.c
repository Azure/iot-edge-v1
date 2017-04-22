// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

const char* simulator_module_path_string = "libsimulator.so";
const char* metrics_module_path_string = "libmetrics.so";

const char* simulator_module_path()
{
    return simulator_module_path_string;
}

const char* metrics_module_path()
{
    return metrics_module_path_string;
}
