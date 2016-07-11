// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DOTNET_H
#define DOTNET_H

#include "module.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct DOTNET_HOST_CONFIG_TAG
{
    const char* dotnet_module_path;
    const char* dotnet_module_entry_class;
    const char* dotnet_module_args;
}DOTNET_HOST_CONFIG;

MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(DOTNET_HOST)(void);

extern __declspec(dllexport) bool Module_DotNetHost_PublishMessage(MESSAGE_BUS_HANDLE bus, MODULE_HANDLE sourceModule, const unsigned char* source, int32_t size);

#ifdef __cplusplus
}
#endif

#endif /*DOTNET_H*/