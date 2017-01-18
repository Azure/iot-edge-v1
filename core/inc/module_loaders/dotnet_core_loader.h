// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DOTNET_CORE_LOADER_H
#define DOTNET_CORE_LOADER_H

#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/umock_c_prod.h"

#include "module.h"
#include "module_loader.h"
#include "gateway_export.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define DOTNET_CORE_LOADER_NAME            "dotnetcore"

#if WIN32
#define DOTNET_CORE_BINDING_MODULE_NAME    "dotnetcore.dll"
#else
#define DOTNET_CORE_BINDING_MODULE_NAME    "libdotnetcore.so"
#endif

/** @brief Structure to load a dotnet module */
typedef struct DOTNET_CORE_LOADER_ENTRYPOINT_TAG
{
    STRING_HANDLE dotnetCoreModulePath;

    STRING_HANDLE dotnetCoreModuleEntryClass;
} DOTNET_CORE_LOADER_ENTRYPOINT;

MOCKABLE_FUNCTION(, GATEWAY_EXPORT const MODULE_LOADER*, DotnetCoreLoader_Get);

#ifdef __cplusplus
}
#endif

#endif // DOTNET_CORE_LOADER_H