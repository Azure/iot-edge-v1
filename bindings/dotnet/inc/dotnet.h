// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DOTNET_H
#define DOTNET_H

/**
 * Including module.h dictates that:
 *
 *      MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void);
 *
 * is implemented by the module.
 */
#include "module.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct DOTNET_MODULE_CONFIG_TAG
{
    const char* dotnet_module_path;
    const char* dotnet_entry_class;
    const char* configuration;
}DOTNET_MODULE_CONFIG;

MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(DOTNET_MODULE)(void);

#ifdef __cplusplus
}
#endif

#endif /*DOTNET_H*/