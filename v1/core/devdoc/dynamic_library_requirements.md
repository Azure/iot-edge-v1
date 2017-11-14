# dynamic_library Requirements


 
## Overview
dynamic_library is a wrapper for OS system calls for loading, unloading and 
finding a symbol in a dynamically linked library.

## References
none

## Exposed API
```C
typedef void*  DYNAMIC_LIBRARY_HANDLE;

extern DYNAMIC_LIBRARY_HANDLE DynamicLibrary_LoadLibrary(const char* dynamicLibraryFileName);
extern void  DynamicLibrary_UnloadLibrary(DYNAMIC_LIBRARY_HANDLE libraryHandle);
extern void* DynamicLibrary_FindSymbol(DYNAMIC_LIBRARY_HANDLE libraryHandle, const char* symbolName);
```

### DynamicLibrary_LoadLibrary
```C
extern DYNAMIC_LIBRARY_HANDLE DynamicLibrary_LoadLibrary(const char* dynamicLibraryFileName);
```

**SRS_DYNAMIC_LIBRARY_17_001: [**`DynamicLibrary_LoadLibrary` shall make the OS system call to load the named library, returning an opaque pointer as a library reference.**]**

In Linux, this will be "dlopen" and in Windows, this will be "LoadLibrary." 

### DynamicLibrary_UnloadLibrary
```C
extern void  DynamicLibrary_UnloadLibrary(DYNAMIC_LIBRARY_HANDLE libraryHandle);
```
 
**SRS_DYNAMIC_LIBRARY_17_002: [**`DynamicLibrary_UnloadLibrary` shall make the OS system call to unload the library referenced by libraryHandle.**]**

In Linux, this will be "dlclose" and in Windows, this will be "FreeLibrary." 

### DynamicLibrary_FindSymbol
```C
extern void* DynamicLibrary_FindSymbol(DYNAMIC_LIBRARY_HANDLE libraryHandle, const char* symbolName);
```

**SRS_DYNAMIC_LIBRARY_17_003: [**`DynamicLibrary_FindSymbol` shall make the OS system call to look up symbolName in the library referenced by libraryHandle.**]**

In Linux, this will be "dlsym" and in Windows, this will be "GetProcAddress."
 