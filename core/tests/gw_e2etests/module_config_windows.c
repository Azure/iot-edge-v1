// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "gateway_ll.h"

const char* e2e_module_path_string = "..\\e2e_module\\Debug\\e2e_module.dll";
const char* iothubhttp_path_string = "..\\..\\..\\..\\modules\\iothubhttp\\Debug\\iothubhttp.dll";
const char* identity_map_path_string = "..\\..\\..\\..\\modules\\identitymap\\Debug\\identity_map.dll";

const char* e2e_module_path()
{
	return e2e_module_path_string;
}

const char* iothubhttp_module_path()
{
	return iothubhttp_path_string;
}

const char* identity_map_module_path()
{
	return identity_map_path_string;
}