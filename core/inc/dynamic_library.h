// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DYNAMIC_LIBRARY_H
#define DYNAMIC_LIBRARY_H

#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif
typedef void*  DYNAMIC_LIBRARY_HANDLE;

MOCKABLE_FUNCTION(, DYNAMIC_LIBRARY_HANDLE, DynamicLibrary_LoadLibrary, const char*, dynamicLibraryFileName);
MOCKABLE_FUNCTION(, void, DynamicLibrary_UnloadLibrary, DYNAMIC_LIBRARY_HANDLE, libraryHandle);
MOCKABLE_FUNCTION(, void*, DynamicLibrary_FindSymbol, DYNAMIC_LIBRARY_HANDLE, libraryHandle, const char*, symbolName);

#ifdef __cplusplus
}
#endif

#endif // DYNAMIC_LIBRARY_H
