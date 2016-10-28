// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include "gb_library.h"

void* gb_LoadLibraryA(const char* dynamicLibraryFileName)
{
    return (void*)LoadLibraryA(dynamicLibraryFileName);
}


int gb_FreeLibrary(void* library)
{
    return (int)FreeLibrary((HMODULE)library);
}


void* gb_GetProcAddress(void* library, const char* symbolName)
{
    return (void*)GetProcAddress((HMODULE)library, symbolName);
}

DWORD gb_GetLastError()
{
    return GetLastError();
}

DWORD gb_GetCurrentDirectoryA(DWORD nBufferLength, char* lpBuffer)
{
    return GetCurrentDirectoryA(nBufferLength, lpBuffer);
}

