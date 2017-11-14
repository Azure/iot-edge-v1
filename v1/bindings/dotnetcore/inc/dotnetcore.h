// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DOTNETCORE_H
#define DOTNETCORE_H

#include "module.h"
#include "dynamic_library.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct DOTNET_CORE_CLR_OPTIONS_TAG
{
    const char* coreClrPath;
    const char* trustedPlatformAssembliesLocation;
}DOTNET_CORE_CLR_OPTIONS;

typedef struct DOTNET_CORE_HOST_CONFIG_TAG
{
    const char* assemblyName;
    const char* entryType;
    const char* moduleArgs;
    DOTNET_CORE_CLR_OPTIONS* clrOptions;
}DOTNET_CORE_HOST_CONFIG;

MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gatewayApiVersion);

MODULE_EXPORT bool Module_DotNetCoreHost_PublishMessage(BROKER_HANDLE broker, MODULE_HANDLE sourceModule, const unsigned char* message, int32_t size);

MODULE_EXPORT void Module_DotNetCoreHost_SetBindingDelegates(intptr_t createAddress, intptr_t receiveAddress, intptr_t destroyAddress, intptr_t startAddress);

#ifdef __cplusplus
}
#endif

#endif /*DOTNETCORE_H*/