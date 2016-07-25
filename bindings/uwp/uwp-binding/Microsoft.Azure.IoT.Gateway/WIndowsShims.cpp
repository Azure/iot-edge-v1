// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pch.h"

HMODULE WINAPI LoadLibraryA(LPCSTR lpFileName)
{
	std::string filename = lpFileName;
	std::wstring wfilename(filename.begin(), filename.end());
	return LoadPackagedLibrary(wfilename.c_str(), NULL);
}

DWORD WINAPI GetCurrentDirectoryA(DWORD  nBufferLength,LPSTR lpBuffer)
{
	return NULL;
}
