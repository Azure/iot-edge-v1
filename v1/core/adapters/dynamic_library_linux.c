// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "dynamic_library.h"
#include "gb_library.h"

/* Codes_SRS_DYNAMIC_LIBRARY_17_001: [DynamicLibrary_LoadLibrary shall make the OS system call to load the named library, returning an opaque pointer as a library reference.] */
DYNAMIC_LIBRARY_HANDLE DynamicLibrary_LoadLibrary(const char* dynamicLibraryFileName)
{
    return dlopen(dynamicLibraryFileName, RTLD_LAZY);
}

/*Codes_SRS_DYNAMIC_LIBRARY_17_002: [DynamicLibrary_UnloadLibrary shall make the OS system call to unload the library referenced by libraryHandle.] */
void DynamicLibrary_UnloadLibrary(DYNAMIC_LIBRARY_HANDLE libraryHandle)
{
    dlclose(libraryHandle);
}

/*Codes_SRS_DYNAMIC_LIBRARY_17_003: [DynamicLibrary_FindSymbol shall make the OS system call to look up symbolName in the library referenced by libraryHandle.]*/
void* DynamicLibrary_FindSymbol(DYNAMIC_LIBRARY_HANDLE libraryHandle, const char* symbolName)
{
    return dlsym(libraryHandle, symbolName);
}

