// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DYNAMIC_LIBRARY_H
#define DYNAMIC_LIBRARY_H


#ifdef __cplusplus
extern "C"
{
#endif
typedef void*  DYNAMIC_LIBRARY_HANDLE;

extern DYNAMIC_LIBRARY_HANDLE DynamicLibrary_LoadLibrary(const char* dynamicLibraryFileName);
extern void  DynamicLibrary_UnloadLibrary(DYNAMIC_LIBRARY_HANDLE libraryHandle);
extern void* DynamicLibrary_FindSymbol(DYNAMIC_LIBRARY_HANDLE libraryHandle, const char* symbolName);

#ifdef __cplusplus
}
#endif

#endif // DYNAMIC_LIBRARY_H
