// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DOTNET_CORE_UTILS_H
#define DOTNET_CORE_UTILS_H

#include "dotnetcore_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

void AddFilesFromDirectoryToTpaList(const std::string& directory, std::string& tpaList);

bool initializeDotNetCoreCLR(coreclr_initialize_ptr coreclrInitialize_ptr, const char* trustedPlatformAssemblyiesLocation, void** hostHandle, unsigned int* domainId);

#ifdef __cplusplus
}
#endif

#endif // DOTNET_CORE_UTILS_H
