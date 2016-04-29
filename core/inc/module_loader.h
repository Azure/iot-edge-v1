// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MODULE_LOADER_H
#define MODULE_LOADER_H

#include "module.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct MODULE_LIBRARY_HANDLE_DATA_TAG* MODULE_LIBRARY_HANDLE;

extern MODULE_LIBRARY_HANDLE ModuleLoader_Load(const char* moduleLibraryFileName);
extern const MODULE_APIS* ModuleLoader_GetModuleAPIs(MODULE_LIBRARY_HANDLE moduleLibraryHandle);
extern void ModuleLoader_Unload(MODULE_LIBRARY_HANDLE moduleLibraryHandle);

#ifdef __cplusplus
}
#endif

#endif // MODULE_LOADER_H
