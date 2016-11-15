// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IDENTITYMAP_H
#define IDENTITYMAP_H

#include "module.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct IDENTITY_MAP_CONFIG_TAG
{
    const char* macAddress;
    const char* deviceId;
    const char* deviceKey;
} IDENTITY_MAP_CONFIG;

MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(IDENTITYMAP_MODULE)(MODULE_API_VERSION gateway_api_version);

#ifdef __cplusplus
}
#endif

#endif /*IDENTITYMAP_H*/
