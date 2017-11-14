// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "module.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct SIMULATOR_MODULE_CONFIG_TAG
{
    char * device_id;
    size_t message_delay;
    size_t properties_count;
    size_t properties_size;
    size_t message_size;
} SIMULATOR_MODULE_CONFIG;


MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(SIMULATOR_MODULE)(MODULE_API_VERSION gateway_api_version);

#ifdef __cplusplus
}
#endif

#endif /*SIMULATOR_H*/
