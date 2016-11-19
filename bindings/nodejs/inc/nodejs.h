// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef NODEJS_H
#define NODEJS_H

#include "module.h"
#include "azure_c_shared_utility/strings.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct NODEJS_MODULE_CONFIG_TAG
{
    STRING_HANDLE main_path;
    STRING_HANDLE configuration_json;
}NODEJS_MODULE_CONFIG;

MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(NODEJS_MODULE)(MODULE_API_VERSION gateway_api_version);

#ifdef __cplusplus
}
#endif

#endif /*NODEJS_H*/
