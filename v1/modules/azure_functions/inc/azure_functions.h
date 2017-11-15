// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef AZUREFUNCTIONS_H
#define AZUREFUNCTIONS_H

#include "module.h"
#include "azure_c_shared_utility/strings.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct AZURE_FUNCTIONS_CONFIG_TAG
{
    STRING_HANDLE hostAddress;
    STRING_HANDLE relativePath;
    STRING_HANDLE securityKey;
} AZURE_FUNCTIONS_CONFIG;

MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(AZUREFUNCTIONS_MODULE)(MODULE_API_VERSION gateway_api_version);

#ifdef __cplusplus
}
#endif

#endif /*AZUREFUNCTIONS_H*/
