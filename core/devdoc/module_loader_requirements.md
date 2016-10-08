# module_loader Requirements


 
## Overview
module_loader allows a user to dynamically load modules into a gateway.  The module in this case is represented by a file name of a shared library (a DLL or SO file, depending on operating system).  The library name given is expected to implement a gateway module, so it is expected to export the API as defined in module.h.
## References
module.h – defines `MODULE_APIS`, `MODULE_GETAPIS_NAME`, and `Module_GetAPIS` function declaration.
## Exposed API
```C
typedef struct MODULE_LIBRARY_DATA_TAG* MODULE_LIBRARY_HANDLE;

extern MODULE_LIBRARY_HANDLE ModuleLoader_Load(const char* moduleLibraryFileName);
extern const MODULE_APIS* ModuleLoader_GetModuleAPIs(MODULE_LIBRARY_HANDLE moduleLibraryHandle);
extern void ModuleLoader_Unload(MODULE_LIBRARY_HANDLE moduleLibraryHandle);
```

### ModuleLoader_Load
```C
extern MODULE_LIBRARY_HANDLE ModuleLoader_Load(const char* moduleLibraryFileName);
```

**SRS_MODULE_LOADER_17_001: [**`ModuleLoader_Load` shall validate the moduleLibraryFileName, if it is `NULL`, it shall return NULL.**]** 
	
**SRS_MODULE_LOADER_17_002: [**`ModuleLoader_Load` shall load the library as a file, the filename given by the moduleLibraryFileName.**]**
**SRS_MODULE_LOADER_17_012: [**If load library is not successful, the load shall fail, and it shall return `NULL`.**]** 
	
**SRS_MODULE_LOADER_17_003: [**`ModuleLoader_Load` shall locate the function defined by `MODULE_GETAPIS_NAME` in the open library.**]**
**SRS_MODULE_LOADER_17_013: [**If locating the function is not successful, the load shall fail, and it shall return `NULL`.**]**

**SRS_MODULE_LOADER_26_002: [**`ModulerLoader_Load` shall allocate memory for the structure `MODULE_APIS`.**]**
**SRS_MODULE_LOADER_26_003: [**If memory allocation is not successful, the load shall fail, and it shall return `NULL`.**]**

**SRS_MODULE_LOADER_17_004: [**`ModuleLoader_Load` shall call the function defined by `MODULE_GETAPIS_NAME` in the open library.**]**
**SRS_MODULE_LOADER_26_001: [** If the get API call doesn't set required functions, the load shall fail and it shall return `NULL`. **]**

**SRS_MODULE_LOADER_17_005: [**`ModuleLoader_Load` shall allocate memory for the structure `MODULE_LIBRARY_HANDLE`.**]**
**SRS_MODULE_LOADER_17_014: [**If memory allocation is not successful, the load shall fail, and it shall return `NULL`. **]**
 
**SRS_MODULE_LOADER_17_006: [**`ModuleLoader_Load` shall return a non-NULL handle to a `MODULE_LIBRARY_DATA_TAG` upon success.**]**
 
The contents of the structure `MODULE_LIBRARY_DATA_TAG` will be operating system specific.  The structure is expected to have at least one element: an opaque handle to the loaded library.  The structure may also to keep a reference to the `MODULE_APIS` provided by the library to improve performance.

### ModuleLoader_GetModuleAPIs
```C
extern const MODULE_APIS* `ModuleLoader_GetModuleAPIs`(MODULE_LIBRARY_HANDLE moduleLibraryHandle);
```

**SRS_MODULE_LOADER_17_007: [**`ModuleLoader_GetModuleAPIs` shall return `NULL` if the moduleLibraryHandle is `NULL`.**]**
 
**SRS_MODULE_LOADER_17_008: [**`ModuleLoader_GetModuleAPIs` shall return a valid pointer to `MODULE_APIS` on success.**]** 

### ModuleLoader_Unload
```C
extern void ModuleLoader_Unload(MODULE_LIBRARY_HANDLE moduleLibraryHandle);
```

**SRS_MODULE_LOADER_17_009: [**`ModuleLoader_Unload` shall do nothing if the moduleLibraryHandle is `NULL`.**]**
 
**SRS_MODULE_LOADER_17_010: [**`ModuleLoader_Unload` shall unload the library.**]**
 
**SRS_MODULE_LOADER_17_011: [**`ModuleLoader_Unload` shall deallocate memory for the structure `MODULE_LIBRARY_HANDLE`.**]**

**SRS_MODULE_LOADER_26_004: [**`ModulerLoader_Unload` shall deallocate memory for the structure `MODULE_APIS`.**]**
