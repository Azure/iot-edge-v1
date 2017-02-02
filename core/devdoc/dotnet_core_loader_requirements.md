.NET Core Module Loader Requirements
====================================

Overview
--------

This document specifies the requirements to be implemented by the module loader
for .NET Core binding.

References
----------
[Module loader design](./module_loaders.md)

Exposed API
-----------
```C
#define DOTNET_CORE_LOADER_NAME "dotnetcore"

#if WIN32
#define DOTNET_CORE_BINDING_MODULE_NAME                                "dotnetcore.dll"
#define DOTNET_CORE_CLR_PATH_DEFAULT                                   "C:\\Program Files\\dotnet\\shared\\Microsoft.NETCore.App\\1.0.1\\coreclr.dll"
#define DOTNET_CORE_TRUSTED_PLATFORM_ASSEMBLIES_LOCATION_DEFAULT       "C:\\Program Files\\dotnet\\shared\\Microsoft.NETCore.App\\1.0.1\\"
#else
#define DOTNET_CORE_BINDING_MODULE_NAME                                "./libdotnetcore.so"
#define DOTNET_CORE_CLR_PATH_DEFAULT                                   "/usr/share/dotnet/shared/Microsoft.NETCore.App/1.1.0/libcoreclr.so"
#define DOTNET_CORE_TRUSTED_PLATFORM_ASSEMBLIES_LOCATION_DEFAULT       "/usr/share/dotnet/shared/Microsoft.NETCore.App/1.1.0/"
#endif

#define DOTNET_CORE_CLR_PATH_KEY                                "binding.coreclrpath"
#define DOTNET_CORE_TRUSTED_PLATFORM_ASSEMBLIES_LOCATION_KEY    "binding.trustedplatformassemblieslocation"

/** @brief Module Loader Configuration, including clrOptions Configuration. */
typedef struct DOTNET_CORE_LOADER_CONFIGURATION_TAG
{
    MODULE_LOADER_BASE_CONFIGURATION base;
    DOTNET_CORE_CLR_OPTIONS* clrOptions;
} DOTNET_CORE_LOADER_CONFIGURATION;

/** @brief Structure to load a dotnet module */
typedef struct DOTNET_CORE_LOADER_ENTRYPOINT_TAG
{
    STRING_HANDLE dotnetCoreModulePath;

    STRING_HANDLE dotnetCoreModuleEntryClass;
} DOTNET_CORE_LOADER_ENTRYPOINT;

    /** @brief  The API for the dotnet core module loader. */
    extern const MODULE_LOADER* DotnetCoreLoader_Get(void);
```

DotnetCoreModuleLoader_Load
---------------------------
```C
MODULE_LIBRARY_HANDLE DotnetCoreModuleLoader_Load(const MODULE_LOADER* loader, const void* entrypoint);
```

Loads the .NET core binding module into memory.

**SRS_DOTNET_CORE_MODULE_LOADER_04_001: [** `DotnetCoreModuleLoader_Load` shall return `NULL` if `loader` is `NULL`. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_030: [** `DotnetCoreModuleLoader_Load` shall return `NULL` if `entrypoint` is `NULL`. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_002: [** `DotnetCoreModuleLoader_Load` shall return `NULL` if `loader->type` is not `DOTNETCORE`. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_031: [** `DotnetCoreModuleLoader_Load` shall return `NULL` if `entrypoint->dotnetCoreModuleEntryClass` is `NULL`. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_032: [** `DotnetCoreModuleLoader_Load` shall return `NULL` if `entrypoint->dotnetCoreModulePath` is `NULL`. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_003: [** `DotnetCoreModuleLoader_Load` shall return `NULL` if an underlying platform call fails. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_033: [** `DotnetCoreModuleLoader_Load` shall use the the binding module path given in `loader->configuration->binding_path` if `loader->configuration` is not `NULL`. Otherwise it shall use the default binding path name. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_034: [** `DotnetCoreModuleLoader_Load` shall verify that the library has an exported function called `Module_GetApi`  **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_035: [** `DotnetCoreModuleLoader_Load` shall verify if api version is lower or equal than `MODULE_API_VERSION_1` and if `MODULE_CREATE`, `MODULE_DESTROY` and `MODULE_RECEIVE` are implemented, otherwise return `NULL` **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_005: [** `DotnetCoreModuleLoader_Load` shall return a non-`NULL` pointer of type `MODULE_LIBRARY_HANDLE` when successful. **]**

DotnetCoreModuleLoader_GetModuleApi
-----------------------------------
```C
const MODULE_API* DotnetCoreModuleLoader_GetModuleApi(MODULE_LIBRARY_HANDLE moduleLibraryHandle);
```

Returns the `MODULE_API` struct for the binding module.

**SRS_DOTNET_CORE_MODULE_LOADER_04_006: [** `DotnetCoreModuleLoader_GetModuleApi` shall return `NULL` if `moduleLibraryHandle` is `NULL`. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_007: [** `DotnetCoreModuleLoader_GetModuleApi` shall return a non-`NULL` `MODULE_API` pointer when successful. **]**

DotnetCoreModuleLoader_Unload
-----------------------------
```C
void DotnetCoreModuleLoader_Unload(MODULE_LIBRARY_HANDLE moduleLibraryHandle)
```

Unloads the binding module from memory.

**SRS_DOTNET_CORE_MODULE_LOADER_04_008: [** `DotnetCoreModuleLoader_Unload` shall do nothing if `moduleLibraryHandle` is `NULL`. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_009: [** `DotnetCoreModuleLoader_Unload` shall unload the binding module from memory by calling `DynamicLibrary_UnloadLibrary`. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_010: [** `DotnetCoreModuleLoader_Unload` shall free resources allocated when loading the binding module. **]**

DotnetCoreModuleLoader_ParseEntrypointFromJson
----------------------------------------------
```C
void* DotnetCoreModuleLoader_ParseEntrypointFromJson(const JSON_Value* json);
```

Parses entrypoint JSON as it applies to a .NET core module and returns a pointer
to the parsed data.

**SRS_DOTNET_CORE_MODULE_LOADER_04_011: [** `DotnetCoreModuleLoader_ParseEntrypointFromJson` shall return `NULL` if `json` is `NULL`. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_012: [** `DotnetCoreModuleLoader_ParseEntrypointFromJson` shall return `NULL` if the root json entity is not an object. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_013: [** `DotnetCoreModuleLoader_ParseEntrypointFromJson` shall return `NULL` if an underlying platform call fails. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_014: [** `DotnetCoreModuleLoader_ParseEntrypointFromJson` shall retrieve the `assembly_name` by reading the value of the attribute `assembly.name`. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_036: [** `DotnetCoreModuleLoader_ParseEntrypointFromJson` shall retrieve the `entry_type` by reading the value of the attribute `entry.type`. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_015: [** `DotnetCoreModuleLoader_ParseEntrypointFromJson` shall return a non-`NULL` pointer to the parsed representation of the entrypoint when successful. **]**

DotnetCoreModuleLoader_FreeEntrypoint
-------------------------------------
```C
void DotnetCoreModuleLoader_FreeEntrypoint(void* entrypoint)
```

Frees entrypoint data allocated by `DotnetCoreModuleLoader_ParseEntrypointFromJson`.

**SRS_DOTNET_CORE_MODULE_LOADER_04_016: [** `DotnetCoreModuleLoader_FreeEntrypoint` shall do nothing if `entrypoint` is `NULL`. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_017: [** `DotnetCoreModuleLoader_FreeEntrypoint` shall free resources allocated during `DotnetCoreModuleLoader_ParseEntrypointFromJson`. **]**

DotnetCoreModuleLoader_ParseConfigurationFromJson
-------------------------------------------------
```C
MODULE_LOADER_BASE_CONFIGURATION* DotnetCoreModuleLoader_ParseConfigurationFromJson(const JSON_Value* json);
```
Parses the module loader configuration from the given JSON and returns a `DOTNET_CORE_LOADER_ENTRYPOINT` pointer.

**SRS_DOTNET_CORE_MODULE_LOADER_04_018: [** `DotnetCoreModuleLoader_ParseConfigurationFromJson` shall return `NULL` if an underlying platform call fails. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_040: [** `DotnetCoreModuleLoader_ParseConfigurationFromJson` shall set default paths on `clrOptions`.
 **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_041: [** `DotnetCoreModuleLoader_ParseConfigurationFromJson` shall check if JSON contains `DOTNET_CORE_CLR_PATH_KEY`, and if it has it shall change the value of `clrOptions->coreclrpath`. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_042: [** `DotnetCoreModuleLoader_ParseConfigurationFromJson` shall check if JSON contains `DOTNET_CORE_TRUSTED_PLATFORM_ASSEMBLIES_LOCATION_KEY`, and if it has it shall change the value of `clrOptions->trustedplatformassemblieslocation` . **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_019: [** `DotnetCoreModuleLoader_ParseConfigurationFromJson` shall call `ModuleLoader_ParseBaseConfigurationFromJson` to parse the loader configuration and return the result. **]**

DotnetCoreModuleLoader_FreeConfiguration
----------------------------------------
```C
void DotnetCoreModuleLoader_FreeConfiguration(MODULE_LOADER_BASE_CONFIGURATION* configuration);
```

Frees loader configuration data as allocated by `DotnetCoreModuleLoader_ParseConfigurationFromJson`.

**SRS_DOTNET_CORE_MODULE_LOADER_04_020: [** `DotnetCoreModuleLoader_FreeConfiguration` shall do nothing if `configuration` is `NULL`. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_021: [** `DotnetCoreModuleLoader_FreeConfiguration` shall call `ModuleLoader_FreeBaseConfiguration` to free resources allocated by `ModuleLoader_ParseBaseConfigurationFromJson`. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_022: [** `DotnetCoreModuleLoader_FreeConfiguration` shall free resources allocated by `DotnetCoreModuleLoader_ParseConfigurationFromJson`. **]**

DotnetCoreModuleLoader_BuildModuleConfiguration
-----------------------------------------------
```C
void* DotnetCoreModuleLoader_BuildModuleConfiguration(
    const MODULE_LOADER* loader,
    const void* entrypoint,
    const void* module_configuration
);
```

Builds a `DOTNET_CORE_HOST_CONFIG` object by merging information from `entrypoint` and `module_configuration`.

**SRS_DOTNET_CORE_MODULE_LOADER_04_023: [** `DotnetCoreModuleLoader_BuildModuleConfiguration` shall return `NULL` if `entrypoint` is `NULL`. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_024: [** `DotnetCoreModuleLoader_BuildModuleConfiguration` shall return `NULL` if `entrypoint->dotnetCoreModulePath` is `NULL`. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_037: [** `DotnetCoreModuleLoader_BuildModuleConfiguration` shall return `NULL` if `entrypoint->dotnetCoreModuleEntryClass` is `NULL`. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_025: [** `DotnetCoreModuleLoader_BuildModuleConfiguration` shall return `NULL` if an underlying platform call fails. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_026: [** `DotnetCoreModuleLoader_BuildModuleConfiguration` shall build a `DOTNET_CORE_HOST_CONFIG` object by copying information from `entrypoint` and `module_configuration` and return a non-`NULL` pointer. **]**

DotnetCoreModuleLoader_FreeModuleConfiguration
----------------------------------------------
```C
void DotnetCoreModuleLoader_FreeModuleConfiguration(const void* module_configuration);
```

Frees the `DOTNET_CORE_HOST_CONFIG` object allocated by `DotnetCoreModuleLoader_BuildModuleConfiguration`.

**SRS_DOTNET_CORE_MODULE_LOADER_04_027: [** `DotnetCoreModuleLoader_FreeModuleConfiguration` shall do nothing if `module_configuration` is `NULL`. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_028: [** `DotnetCoreModuleLoader_FreeModuleConfiguration` shall free the `DOTNET_CORE_HOST_CONFIG` object. **]**

DotnetCoreLoader_Get
--------------------
```C
const MODULE_LOADER* DotnetCoreLoader_Get(void)
```

**SRS_DOTNET_CORE_MODULE_LOADER_04_029: [** `DotnetCoreLoader_Get` shall return a non-`NULL` pointer to a `MODULE_LOADER` struct. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_038: [** MODULE_LOADER::type shall be `DOTNETCORE`. **]**

**SRS_DOTNET_CORE_MODULE_LOADER_04_039: [** MODULE_LOADER::name shall be the string `dotnetcore`. **]**