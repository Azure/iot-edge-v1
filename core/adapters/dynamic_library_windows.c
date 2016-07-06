// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include "gb_library.h"

#include "dynamic_library.h"
#include "azure_c_shared_utility/xlogging.h"

#ifdef UWP_BINDING

HMODULE WINAPI LoadLibraryA(LPCSTR lpFileName);
DWORD WINAPI GetCurrentDirectoryA(DWORD  nBufferLength, LPSTR lpBuffer);

#endif // UWP_BINDING

/* Codes_SRS_DYNAMIC_LIBRARY_17_001: [DynamicLibrary_LoadLibrary shall make the OS system call to load the named library, returning an opaque pointer as a library reference.] */
DYNAMIC_LIBRARY_HANDLE DynamicLibrary_LoadLibrary(const char* dynamicLibraryFileName)
{
	DYNAMIC_LIBRARY_HANDLE returnValue = LoadLibraryA(dynamicLibraryFileName);
	if (returnValue == NULL)
	{
    	DWORD error = GetLastError();
		DWORD status;
		char currentPath[MAX_PATH];

		LogError("Error Loading Library. Error code is:  %u", error);

		//This retry was needed because when you run multiple tests together LoadLibraryA failed to find the Library. 
		//So we need to help by building the string with current Path.
		status = GetCurrentDirectoryA(MAX_PATH, currentPath);

		if (status != 0)
		{
			size_t totalAllocationNeeded = 0;
			char* libraryPath;
			totalAllocationNeeded += strlen(currentPath);
			totalAllocationNeeded += strlen(dynamicLibraryFileName);

			//Allocate speace for current Path + \\ + dynamic Library FileName + NULLTerminate.
			libraryPath = (char*)malloc(totalAllocationNeeded + 2);
			
			if (libraryPath != NULL)
			{
				int sprintfReturnCode;

				sprintfReturnCode = sprintf(libraryPath, "%s\\%s", currentPath, dynamicLibraryFileName);

				if (sprintfReturnCode != -1)
				{
					returnValue = LoadLibraryA(libraryPath);
				}
				else
				{
					LogError("CurrentPath + Library name too long and doesn't fit on MAX_PATH.");
					returnValue = NULL;
				}
				free(libraryPath);
			}
			else
			{
				LogError("Failed to allocate memory for library path.");
				returnValue = NULL;
			}
			
		}
		else
		{
			error = GetLastError();
			LogError("Could not retrieve Current Directory. Error: %u", error);
			returnValue = NULL;
		}
	}

	return returnValue;
}

/*Codes_SRS_DYNAMIC_LIBRARY_17_002: [DynamicLibrary_UnloadLibrary shall make the OS system call to unload the library referenced by libraryHandle.] */
void DynamicLibrary_UnloadLibrary(DYNAMIC_LIBRARY_HANDLE libraryHandle)
{
    //"Module" here is not to be confused with modules at the gateway level.
	HMODULE hModule = (HMODULE)libraryHandle;
	FreeLibrary(hModule);
}

/*Codes_SRS_DYNAMIC_LIBRARY_17_003: [DynamicLibrary_FindSymbol shall make the OS system call to look up symbolName in the library referenced by libraryHandle.]*/
void* DynamicLibrary_FindSymbol(DYNAMIC_LIBRARY_HANDLE libraryHandle, const char* symbolName)
{
    //"Module" here is not to be confused with modules at the gateway level.
	HMODULE hModule = (HMODULE)libraryHandle;
	return (void*)GetProcAddress(hModule, symbolName);
}
