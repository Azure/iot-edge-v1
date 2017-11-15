// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MODULE_CONFIG_RESOURCES_H
#define MODULE_CONFIG_RESOURCES_H

extern "C" {
//Add here paths for any new Module. 
const char* e2e_module_path();
const char* iothub_module_path();
const char* identity_map_module_path();
}

#endif /*MODULE_CONFIG_RESOURCES_H*/