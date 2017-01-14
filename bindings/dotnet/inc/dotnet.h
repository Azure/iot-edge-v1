// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DOTNET_H
#define DOTNET_H

#include "module.h"
#include "gateway.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct DOTNET_HOST_CONFIG_TAG
{
    const char* assembly_name;
    const char* entry_type;
    const char* module_args;
}DOTNET_HOST_CONFIG;

MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version);

extern __declspec(dllexport) bool Module_DotNetHost_PublishMessage(BROKER_HANDLE broker, MODULE_HANDLE sourceModule, const unsigned char* message, int32_t size);

extern __declspec(dllexport) GATEWAY_HANDLE Module_DotNetHost_Gateway_CreateFromJson(const char* file_path);

extern __declspec(dllexport) void Module_DotNetHost_Gateway_Destroy(GATEWAY_HANDLE handle);

#ifdef __cplusplus
}
#endif

#endif /*DOTNET_H*/