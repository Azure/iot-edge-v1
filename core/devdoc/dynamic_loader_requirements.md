Dynamic Module Loader Requirements
==================================

Overview
--------

The dynamic module loader implements loading of gateway modules that are distributed as DLLs or SOs.

## References
[Module loader design](./module_loaders.md)

## Exposed API
```C

#define DYNAMIC_LOADER_NAME "native"

typedef struct DYNAMIC_LOADER_ENTRYPOINT_TAG
{
    STRING_HANDLE moduleLibraryFileName;
} DYNAMIC_LOADER_ENTRYPOINT;

const MODULE_LOADER* DynamicLoader_Get(void);
```

DynamicModuleLoader_Load
------------------------
```C
MODULE_LIBRARY_HANDLE DynamicModuleLoader_Load(const MODULE_LOADER* loader, const void* entrypoint)
```

Loads the module passed in via `entrypoint` into memory. `entrypoint` is a `DYNAMIC_LOADER_ENTRYPOINT` instance.

**SRS_DYNAMIC_MODULE_LOADER_13_001: [** `DynamicModuleLoader_Load` shall return `NULL` if `loader` is `NULL`. **]**

**SRS_DYNAMIC_MODULE_LOADER_13_041: [** `DynamicModuleLoader_Load` shall return `NULL` if `entrypoint` is `NULL`. **]**

**SRS_DYNAMIC_MODULE_LOADER_13_002: [** `DynamicModuleLoader_Load` shall return `NULL` if `loader->type` is not `NATIVE`. **]**

**SRS_DYNAMIC_MODULE_LOADER_13_003: [** `DynamicModuleLoader_Load` shall return `NULL` if an underlying platform call fails. **]**

**SRS_DYNAMIC_MODULE_LOADER_13_039: [** `DynamicModuleLoader_Load` shall return `NULL` if `entrypoint->moduleLibraryFileName` is `NULL`. **]**

**SRS_DYNAMIC_MODULE_LOADER_13_004: [** `DynamicModuleLoader_Load` shall load the module into memory by calling `DynamicLibrary_LoadLibrary`. **]**

**SRS_DYNAMIC_MODULE_LOADER_13_033: [** `DynamicModuleLoader_Load` shall call `DynamicLibrary_FindSymbol` on the module handle with the symbol name `Module_GetApi` to acquire the function that returns the module's API table. **]**

**SRS_DYNAMIC_MODULE_LOADER_13_040: [** `DynamicModuleLoader_Load` shall call the module's `Module_GetAPI` callback to acquire the module API table. **]**

**SRS_DYNAMIC_MODULE_LOADER_13_034: [** `DynamicModuleLoader_Load` shall return `NULL` if the `MODULE_API` pointer returned by the module is `NULL`. **]**

**SRS_DYNAMIC_MODULE_LOADER_13_035: [** `DynamicModuleLoader_Load` shall return `NULL` if `MODULE_API::version` is greater than `Module_ApiGatewayVersion`. **]**

**SRS_DYNAMIC_MODULE_LOADER_13_036: [** `DynamicModuleLoader_Load` shall return `NULL` if the `Module_Create` function in `MODULE_API` is `NULL`. **]**

**SRS_DYNAMIC_MODULE_LOADER_13_037: [** `DynamicModuleLoader_Load` shall return `NULL` if the `Module_Receive` function in `MODULE_API` is `NULL`. **]**

**SRS_DYNAMIC_MODULE_LOADER_13_038: [** `DynamicModuleLoader_Load` shall return `NULL` if the `Module_Destroy` function in `MODULE_API` is `NULL`. **]**

**SRS_DYNAMIC_MODULE_LOADER_13_005: [** `DynamicModuleLoader_Load` shall return a non-`NULL` pointer of type `MODULE_LIBRARY_HANDLE` when successful. **]**

DynamicModuleLoader_GetModuleApi
--------------------------------
```C
extern const MODULE_API* `DynamicModuleLoader_GetModuleApi`(MODULE_LIBRARY_HANDLE moduleLibraryHandle);
```

**SRS_MODULE_LOADER_17_007: [**`DynamicModuleLoader_GetModuleApi` shall return `NULL` if the moduleLibraryHandle is `NULL`.**]**

**SRS_MODULE_LOADER_17_008: [**`DynamicModuleLoader_GetModuleApi` shall return a valid pointer to `MODULE_API` on success.**]**

DynamicModuleLoader_Unload
--------------------------
```C
void DynamicModuleLoader_Unload(MODULE_LIBRARY_HANDLE moduleLibraryHandle);
```

**SRS_MODULE_LOADER_17_009: [**`DynamicModuleLoader_Unload` shall do nothing if the moduleLibraryHandle is `NULL`.**]**

**SRS_MODULE_LOADER_17_010: [**`DynamicModuleLoader_Unload` shall unload the library.**]**

**SRS_MODULE_LOADER_17_011: [**`DynamicModuleLoader_Unload` shall deallocate memory for the structure `MODULE_LIBRARY_HANDLE`.**]**

DynamicModuleLoader_ParseEntrypointFromJson
-------------------------------------------
```C
void* DynamicModuleLoader_ParseEntrypointFromJson(const JSON_Value* json);
```

Parses entrypoint JSON as it applies to a native module and returns a pointer
to the parsed data.

**SRS_DYNAMIC_MODULE_LOADER_13_042: [** `DynamicModuleLoader_ParseEntrypointFromJson` shall return `NULL` if `json` is `NULL`.  **]**

**SRS_DYNAMIC_MODULE_LOADER_13_043: [** `DynamicModuleLoader_ParseEntrypointFromJson` shall return `NULL` if the root json entity is not an object.  **]**

**SRS_DYNAMIC_MODULE_LOADER_13_044: [** `DynamicModuleLoader_ParseEntrypointFromJson` shall return `NULL` if an underlying platform call fails.  **]**

**SRS_DYNAMIC_MODULE_LOADER_13_045: [** `DynamicModuleLoader_ParseEntrypointFromJson` shall retrieve the path to the module library file by reading the value of the attribute `module.path`. **]**

**SRS_DYNAMIC_MODULE_LOADER_13_047: [** `DynamicModuleLoader_ParseEntrypointFromJson` shall return `NULL` if `module.path` does not exist. **]**

**SRS_DYNAMIC_MODULE_LOADER_13_046: [** `DynamicModuleLoader_ParseEntrypointFromJson` shall return a non-`NULL` pointer to the parsed representation of the entrypoint when successful.  **]**

DynamicModuleLoader_FreeEntrypoint
----------------------------------
```C
void DynamicModuleLoader_FreeEntrypoint(void* entrypoint)
```

Frees entrypoint data allocated by `DynamicModuleLoader_ParseEntrypointFromJson`.

**SRS_DYNAMIC_MODULE_LOADER_13_049: [** `DynamicModuleLoader_FreeEntrypoint` shall do nothing if `entrypoint` is `NULL`. **]**

**SRS_DYNAMIC_MODULE_LOADER_13_048: [** `DynamicModuleLoader_FreeEntrypoint` shall free resources allocated during `DynamicModuleLoader_ParseEntrypointFromJson`. **]**

DynamicModuleLoader_ParseConfigurationFromJson
----------------------------------------------
```C
MODULE_LOADER_BASE_CONFIGURATION* DynamicModuleLoader_ParseConfigurationFromJson(const JSON_Value* json);
```

The dynamic loader does not have any configuration. So this method always returns NULL.

**SRS_DYNAMIC_MODULE_LOADER_13_050: [** `DynamicModuleLoader_ParseConfigurationFromJson` shall return `NULL`. **]**

DynamicModuleLoader_FreeConfiguration
-------------------------------------
```C
void DynamicModuleLoader_FreeConfiguration(MODULE_LOADER_BASE_CONFIGURATION* configuration);
```

The dynamic loader does not have any configuration. So there is nothing to free here.

**SRS_DYNAMIC_MODULE_LOADER_13_051: [** `DynamicModuleLoader_FreeConfiguration` shall do nothing. **]**

DynamicModuleLoader_BuildModuleConfiguration
--------------------------------------------
```C
void* DynamicModuleLoader_BuildModuleConfiguration(
    const MODULE_LOADER* loader,
    const void* entrypoint,
    const void* module_configuration
);
```

The native dynamic loader does not need to do any special configuration translation. So this function simply returns `module_configuration`.

**SRS_DYNAMIC_MODULE_LOADER_13_052: [** `DynamicModuleLoader_BuildModuleConfiguration` shall return `module_configuration`. **]**

DynamicModuleLoader_FreeModuleConfiguration
-------------------------------------------
```C
void DynamicModuleLoader_FreeModuleConfiguration(const void* module_configuration);
```

Since the dynamic loader does not do any configuration translation, there is nothing to free.

**SRS_DYNAMIC_MODULE_LOADER_13_053: [** `DynamicModuleLoader_FreeModuleConfiguration` shall do nothing. **]**

DynamicModuleLoader_Get
-----------------------
```C
const MODULE_LOADER* DynamicModuleLoader_Get(void);
```

**SRS_DYNAMIC_MODULE_LOADER_13_054: [** `DynamicModuleLoader_Get` shall return a non-`NULL` pointer to a `MODULE_LOADER` struct. **]**

**SRS_DYNAMIC_MODULE_LOADER_13_055: [** `MODULE_LOADER::type` shall be `NATIVE`. **]**

**SRS_DYNAMIC_MODULE_LOADER_13_056: [** `MODULE_LOADER::name` shall be the string 'native'. **]**
