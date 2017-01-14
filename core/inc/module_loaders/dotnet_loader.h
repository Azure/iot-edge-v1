// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DOTNET_LOADER_H
#define DOTNET_LOADER_H

#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/umock_c_prod.h"

#include "module.h"
#include "module_loader.h"
#include "gateway_export.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define DOTNET_LOADER_NAME            "dotnet"

#if WIN32
#define DOTNET_BINDING_MODULE_NAME    "dotnet.dll"
#else
#define DOTNET_BINDING_MODULE_NAME    "libdotnet.so"
#endif

/** @brief Structure to load a dotnet module */
typedef struct DOTNET_LOADER_ENTRYPOINT_TAG
{
    STRING_HANDLE dotnetModulePath;

    STRING_HANDLE dotnetModuleEntryClass;
} DOTNET_LOADER_ENTRYPOINT;

MOCKABLE_FUNCTION(, GATEWAY_EXPORT const MODULE_LOADER*, DotnetLoader_Get);

#ifdef __cplusplus
}
#endif

#endif // DOTNET_LOADER_H
