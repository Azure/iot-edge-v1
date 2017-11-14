// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "gateway.h"

const char* e2e_module_path_string = "e2e_module/libe2e_module.so";
const char* iothub_path_string = "../../../modules/iothub/libiothub.so";
const char* identity_map_path_string = "../../../modules/identitymap/libidentity_map.so";

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