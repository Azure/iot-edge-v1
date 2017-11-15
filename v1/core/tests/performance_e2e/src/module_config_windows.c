// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "gateway.h"

#ifdef _DEBUG
const char* simulator_module_path_string = "Debug\\simulator.dll";
const char* metrics_module_path_string = "Debug\\metrics.dll";
#else
const char* simulator_module_path_string = "Release\\simulator.dll";
const char* metrics_module_path_string = "Release\\metrics.dll";
#endif

const char* simulator_module_path()
{
    return simulator_module_path_string;
}

const char* metrics_module_path()
{
    return metrics_module_path_string;
}