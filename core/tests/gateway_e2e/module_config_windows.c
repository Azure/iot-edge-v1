// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "gateway.h"

const char* e2e_module_path_string = "..\\e2e_module\\Debug\\e2e_module.dll";
const char* iothub_path_string = "..\\..\\..\\..\\modules\\iothub\\Debug\\iothub.dll";
const char* identity_map_path_string = "..\\..\\..\\..\\modules\\identitymap\\Debug\\identity_map.dll";

const char* e2e_module_path()
{
    return e2e_module_path_string;
}

const char* iothub_module_path()
{
    return iothub_path_string;
}

const char* identity_map_module_path()
{
    return identity_map_path_string;
}