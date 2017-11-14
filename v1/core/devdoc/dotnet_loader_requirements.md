.NET Module Loader Requirements
===============================

Overview
--------

This document specifies the requirements to be implemented by the module loader
for .NET binding.

References
----------
[Module loader design](./module_loaders.md)

Exposed API
-----------
```C
#define DOTNET_LOADER_NAME "dotnet"

#if WIN32
#define DOTNET_BINDING_MODULE_NAME    "dotnet.dll"
#else
#error Cannot build a .NET language binding module for your platform yet.
#endif

    /** @brief Structure to load a dotnet module */
    typedef struct DOTNET_LOADER_ENTRYPOINT_TAG
    {
        STRING_HANDLE dotnetModulePath;

        STRING_HANDLE dotnetModuleEntryClass;
    } DOTNET_LOADER_ENTRYPOINT;

    /** @brief  The API for the dotnet module loader. */
    extern const MODULE_LOADER* DotnetLoader_Get(void);
```

DotnetModuleLoader_Load
-----------------------
```C
MODULE_LIBRARY_HANDLE DotnetModuleLoader_Load(const MODULE_LOADER* loader, const void* entrypoint);
```

Loads the .NET binding module into memory.

**SRS_DOTNET_MODULE_LOADER_04_001: [** `DotnetModuleLoader_Load` shall return `NULL` if `loader` is `NULL`. **]**

**SRS_DOTNET_MODULE_LOADER_04_030: [** `DotnetModuleLoader_Load` shall return `NULL` if `entrypoint` is `NULL`. **]**

**SRS_DOTNET_MODULE_LOADER_04_002: [** `DotnetModuleLoader_Load` shall return `NULL` if `loader->type` is not `DOTNET`. **]**

**SRS_DOTNET_MODULE_LOADER_04_031: [** `DotnetModuleLoader_Load` shall return `NULL` if `entrypoint->dotnetModuleEntryClass` is `NULL`. **]**

**SRS_DOTNET_MODULE_LOADER_04_032: [** `DotnetModuleLoader_Load` shall return `NULL` if `entrypoint->dotnetModulePath` is `NULL`. **]**

**SRS_DOTNET_MODULE_LOADER_04_003: [** `DotnetModuleLoader_Load` shall return `NULL` if an underlying platform call fails. **]**

**SRS_DOTNET_MODULE_LOADER_04_033: [** `DotnetModuleLoader_Load` shall use the the binding module path given in `loader->configuration->binding_path` if `loader->configuration` is not `NULL`. Otherwise it shall use the default binding path name. **]**

**SRS_DOTNET_MODULE_LOADER_04_034: [** `DotnetModuleLoader_Load` shall verify that the library has an exported function called `Module_GetApi`  **]**

**SRS_DOTNET_MODULE_LOADER_04_035: [** `DotnetModuleLoader_Load` shall verify if api version is lower or equal than `MODULE_API_VERSION_1` and if `MODULE_CREATE`, `MODULE_DESTROY` and `MODULE_RECEIVE` are implemented, otherwise return `NULL` **]**

**SRS_DOTNET_MODULE_LOADER_04_005: [** `DotnetModuleLoader_Load` shall return a non-`NULL` pointer of type `MODULE_LIBRARY_HANDLE` when successful. **]**

DotnetModuleLoader_GetModuleApi
-------------------------------
```C
const MODULE_API* DotnetModuleLoader_GetModuleApi(MODULE_LIBRARY_HANDLE moduleLibraryHandle);
```

Returns the `MODULE_API` struct for the binding module.

**SRS_DOTNET_MODULE_LOADER_04_006: [** `DotnetModuleLoader_GetModuleApi` shall return `NULL` if `moduleLibraryHandle` is `NULL`. **]**

**SRS_DOTNET_MODULE_LOADER_04_007: [** `DotnetModuleLoader_GetModuleApi` shall return a non-`NULL` `MODULE_API` pointer when successful. **]**

DotnetModuleLoader_Unload
-------------------------
```C
void DotnetModuleLoader_Unload(MODULE_LIBRARY_HANDLE moduleLibraryHandle)
```

Unloads the binding module from memory.

**SRS_DOTNET_MODULE_LOADER_04_008: [** `DotnetModuleLoader_Unload` shall do nothing if `moduleLibraryHandle` is `NULL`. **]**

**SRS_DOTNET_MODULE_LOADER_04_009: [** `DotnetModuleLoader_Unload` shall unload the binding module from memory by calling `DynamicLibrary_UnloadLibrary`. **]**

**SRS_DOTNET_MODULE_LOADER_04_010: [** `DotnetModuleLoader_Unload` shall free resources allocated when loading the binding module. **]**

DotnetModuleLoader_ParseEntrypointFromJson
------------------------------------------
```C
void* DotnetModuleLoader_ParseEntrypointFromJson(const JSON_Value* json);
```

Parses entrypoint JSON as it applies to a .NET module and returns a pointer
to the parsed data.

**SRS_DOTNET_MODULE_LOADER_04_011: [** `DotnetModuleLoader_ParseEntrypointFromJson` shall return `NULL` if `json` is `NULL`. **]**

**SRS_DOTNET_MODULE_LOADER_04_012: [** `DotnetModuleLoader_ParseEntrypointFromJson` shall return `NULL` if the root json entity is not an object. **]**

**SRS_DOTNET_MODULE_LOADER_04_013: [** `DotnetModuleLoader_ParseEntrypointFromJson` shall return `NULL` if an underlying platform call fails. **]**

**SRS_DOTNET_MODULE_LOADER_04_014: [** `DotnetModuleLoader_ParseEntrypointFromJson` shall retrieve the `assembly_name` by reading the value of the attribute `assembly.name`. **]**

**SRS_DOTNET_MODULE_LOADER_04_036: [** `DotnetModuleLoader_ParseEntrypointFromJson` shall retrieve the `entry_type` by reading the value of the attribute `entry.type`. **]**

**SRS_DOTNET_MODULE_LOADER_04_015: [** `DotnetModuleLoader_ParseEntrypointFromJson` shall return a non-`NULL` pointer to the parsed representation of the entrypoint when successful. **]**

DotnetModuleLoader_FreeEntrypoint
---------------------------------
```C
void DotnetModuleLoader_FreeEntrypoint(void* entrypoint)
```

Frees entrypoint data allocated by `DotnetModuleLoader_ParseEntrypointFromJson`.

**SRS_DOTNET_MODULE_LOADER_04_016: [** `DotnetModuleLoader_FreeEntrypoint` shall do nothing if `entrypoint` is `NULL`. **]**

**SRS_DOTNET_MODULE_LOADER_04_017: [** `DotnetModuleLoader_FreeEntrypoint` shall free resources allocated during `DotnetModuleLoader_ParseEntrypointFromJson`. **]**

DotnetModuleLoader_ParseConfigurationFromJson
---------------------------------------------
```C
MODULE_LOADER_BASE_CONFIGURATION* DotnetModuleLoader_ParseConfigurationFromJson(const JSON_Value* json);
```
Parses the module loader configuration from the given JSON and returns a `MODULE_LOADER_BASE_CONFIGURATION` pointer.

**SRS_DOTNET_MODULE_LOADER_04_018: [** `DotnetModuleLoader_ParseConfigurationFromJson` shall return `NULL` if an underlying platform call fails. **]**

**SRS_DOTNET_MODULE_LOADER_04_019: [** `DotnetModuleLoader_ParseConfigurationFromJson` shall call `ModuleLoader_ParseBaseConfigurationFromJson` to parse the loader configuration and return the result. **]**

DotnetModuleLoader_FreeConfiguration
------------------------------------
```C
void DotnetModuleLoader_FreeConfiguration(MODULE_LOADER_BASE_CONFIGURATION* configuration);
```

Frees loader configuration data as allocated by `DotnetModuleLoader_ParseConfigurationFromJson`.

**SRS_DOTNET_MODULE_LOADER_04_020: [** `DotnetModuleLoader_FreeConfiguration` shall do nothing if `configuration` is `NULL`. **]**

**SRS_DOTNET_MODULE_LOADER_04_021: [** `DotnetModuleLoader_FreeConfiguration` shall call `ModuleLoader_FreeBaseConfiguration` to free resources allocated by `ModuleLoader_ParseBaseConfigurationFromJson`. **]**

**SRS_DOTNET_MODULE_LOADER_04_022: [** `DotnetModuleLoader_FreeConfiguration` shall free resources allocated by `DotnetModuleLoader_ParseConfigurationFromJson`. **]**

DotnetModuleLoader_BuildModuleConfiguration
-------------------------------------------
```C
void* DotnetModuleLoader_BuildModuleConfiguration(
    const MODULE_LOADER* loader,
    const void* entrypoint,
    const void* module_configuration
);
```

Builds a `DOTNET_HOST_CONFIG` object by merging information from `entrypoint` and `module_configuration`.

**SRS_DOTNET_MODULE_LOADER_04_023: [** `DotnetModuleLoader_BuildModuleConfiguration` shall return `NULL` if `entrypoint` is `NULL`. **]**

**SRS_DOTNET_MODULE_LOADER_04_024: [** `DotnetModuleLoader_BuildModuleConfiguration` shall return `NULL` if `entrypoint->dotnetModulePath` is `NULL`. **]**

**SRS_DOTNET_MODULE_LOADER_04_037: [** `DotnetModuleLoader_BuildModuleConfiguration` shall return `NULL` if `entrypoint->dotnetModuleEntryClass` is `NULL`. **]**

**SRS_DOTNET_MODULE_LOADER_04_025: [** `DotnetModuleLoader_BuildModuleConfiguration` shall return `NULL` if an underlying platform call fails. **]**

**SRS_DOTNET_MODULE_LOADER_04_026: [** `DotnetModuleLoader_BuildModuleConfiguration` shall build a `DOTNET_HOST_CONFIG` object by copying information from `entrypoint` and `module_configuration` and return a non-`NULL` pointer. **]**

DotnetModuleLoader_FreeModuleConfiguration
------------------------------------------
```C
void DotnetModuleLoader_FreeModuleConfiguration(const void* module_configuration);
```

Frees the `DOTNET_HOST_CONFIG` object allocated by `DotnetModuleLoader_BuildModuleConfiguration`.

**SRS_DOTNET_MODULE_LOADER_04_027: [** `DotnetModuleLoader_FreeModuleConfiguration` shall do nothing if `module_configuration` is `NULL`. **]**

**SRS_DOTNET_MODULE_LOADER_04_028: [** `DotnetModuleLoader_FreeModuleConfiguration` shall free the `DOTNET_HOST_CONFIG` object. **]**

DotnetLoader_Get
----------------
```C
const MODULE_LOADER* DotnetLoader_Get(void)
```

**SRS_DOTNET_MODULE_LOADER_04_029: [** `DotnetLoader_Get` shall return a non-`NULL` pointer to a `MODULE_LOADER` struct. **]**

**SRS_DOTNET_MODULE_LOADER_04_038: [** MODULE_LOADER::type shall be DOTNET. **]**

**SRS_DOTNET_MODULE_LOADER_04_039: [** MODULE_LOADER::name shall be the string 'dotnet'. **]**

