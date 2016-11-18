Module Loader Requirements
==========================

Overview
--------

This document specifies the requirements to be implemented by the module loading
system in the gateway.

References
----------
[Module loader design](./module_loaders.md)

Exposed API
-----------
```C
#define MODULE_LOADER_RESULT_VALUES \
    MODULE_LOADER_SUCCESS, \
    MODULE_LOADER_ERROR

DEFINE_ENUM(MODULE_LOADER_RESULT, MODULE_LOADER_RESULT_VALUES);

MODULE_LOADER_RESULT ModuleLoader_Initialize(void);

MODULE_LOADER_RESULT ModuleLoader_ParseBaseConfigurationFromJson(
    MODULE_LOADER_BASE_CONFIGURATION* configuration,
    const JSON_Value* json
);

void ModuleLoader_FreeBaseConfiguration(MODULE_LOADER_BASE_CONFIGURATION* configuration);

void ModuleLoader_Destroy(void);

MODULE_LOADER_RESULT ModuleLoader_Add(const MODULE_LOADER* loader);

MODULE_LOADER_RESULT ModuleLoader_UpdateConfiguration(
    MODULE_LOADER* loader,
    MODULE_LOADER_BASE_CONFIGURATION* configuration
);

MODULE_LOADER* ModuleLoader_FindByName(const char* name);

MODULE_LOADER* ModuleLoader_GetDefaultLoaderForType(MODULE_LOADER_TYPE type);

MODULE_LOADER_TYPE ModuleLoader_ParseType(const char* type);

bool ModuleLoader_IsDefaultLoader(const char* name);

MODULE_LOADER_RESULT ModuleLoader_InitializeFromJson(const JSON_Value* loaders);
```

ModuleLoader_Initialize
-----------------------
```C
MODULE_LOADER_RESULT ModuleLoader_Initialize(void);
```

Initializes the gateway with the default module loaders. The following global
struct shall be used to keep track of the list of modules.

```C
static struct
{
    // List of module loaders that we support.
    VECTOR_HANDLE module_loaders;

    // Lock used to protect this instance.
    LOCK_HANDLE   lock;

} g_module_loaders = { 0 };
```

**SRS_MODULE_LOADER_13_001: [** `ModuleLoader_Initialize` shall initialize `g_module_loaders.lock`. **]**

**SRS_MODULE_LOADER_13_002: [** `ModuleLoader_Initialize` shall return `MODULE_LOADER_ERROR` if an underlying platform call fails. **]**

**SRS_MODULE_LOADER_13_003: [** `ModuleLoader_Initialize` shall acquire the lock on `g_module_loaders.lock`. **]**

**SRS_MODULE_LOADER_13_004: [** `ModuleLoader_Initialize` shall initialize `g_module.module_loaders` by calling `VECTOR_create`. **]**

**SRS_MODULE_LOADER_13_005: [** `ModuleLoader_Initialize` shall add the default support module loaders to `g_module.module_loaders`. **]**

**SRS_MODULE_LOADER_13_007: [** `ModuleLoader_Initialize` shall unlock `g_module.lock`. **]**

**SRS_MODULE_LOADER_13_006: [** `ModuleLoader_Initialize` shall return `MODULE_LOADER_SUCCESS` once all the default loaders have been added successfully. **]**

ModuleLoader_Add
----------------
```C
MODULE_LOADER_RESULT ModuleLoader_Add(const MODULE_LOADER* loader);
```

Adds a new module loader to the gateway's list of module loaders.

**SRS_MODULE_LOADER_13_008: [** `ModuleLoader_Add` shall return `MODULE_LOADER_ERROR` if `loader` is `NULL`. **]**

**SRS_MODULE_LOADER_13_011: [** `ModuleLoader_Add` shall return `MODULE_LOADER_ERROR` if `loader->api` is `NULL` **]**

**SRS_MODULE_LOADER_13_012: [** `ModuleLoader_Add` shall return `MODULE_LOADER_ERROR` if `loader->name` is `NULL` **]**

**SRS_MODULE_LOADER_13_013: [** `ModuleLoader_Add` shall return `MODULE_LOADER_ERROR` if `loader->type` is `UNKNOWN` **]**

**SRS_MODULE_LOADER_13_014: [** `ModuleLoader_Add` shall return `MODULE_LOADER_ERROR` if `loader->api->BuildModuleConfiguration` is `NULL` **]**

**SRS_MODULE_LOADER_13_015: [** `ModuleLoader_Add` shall return `MODULE_LOADER_ERROR` if `loader->api->FreeConfiguration` is `NULL` **]**

**SRS_MODULE_LOADER_13_016: [** `ModuleLoader_Add` shall return `MODULE_LOADER_ERROR` if `loader->api->FreeEntrypoint` is `NULL` **]**

**SRS_MODULE_LOADER_13_017: [** `ModuleLoader_Add` shall return `MODULE_LOADER_ERROR` if `loader->api->FreeModuleConfiguration` is `NULL` **]**

**SRS_MODULE_LOADER_13_018: [** `ModuleLoader_Add` shall return `MODULE_LOADER_ERROR` if `loader->api->GetApi` is `NULL` **]**

**SRS_MODULE_LOADER_13_019: [** `ModuleLoader_Add` shall return `MODULE_LOADER_ERROR` if `loader->api->Load` is `NULL` **]**

**SRS_MODULE_LOADER_13_020: [** `ModuleLoader_Add` shall return `MODULE_LOADER_ERROR` if `loader->api->ParseConfigurationFromJson` is `NULL` **]**

**SRS_MODULE_LOADER_13_021: [** `ModuleLoader_Add` shall return `MODULE_LOADER_ERROR` if `loader->api->ParseEntrypointFromJson` is `NULL` **]**

**SRS_MODULE_LOADER_13_022: [** `ModuleLoader_Add` shall return `MODULE_LOADER_ERROR` if `loader->api->Unload` is `NULL` **]**

**SRS_MODULE_LOADER_13_009: [** `ModuleLoader_Add` shall return `MODULE_LOADER_ERROR` if `g_module_loaders.lock` is `NULL`. **]**

**SRS_MODULE_LOADER_13_010: [** `ModuleLoader_Add` shall return `MODULE_LOADER_ERROR` if `g_module_loaders.module_loaders` is `NULL`. **]**

**SRS_MODULE_LOADER_13_023: [** `ModuleLoader_Add` shall return `MODULE_LOADER_ERROR` if an underlying platform call fails. **]**

**SRS_MODULE_LOADER_13_027: [** `ModuleLoader_Add` shall lock `g_module_loaders.lock`. **]**

**SRS_MODULE_LOADER_13_024: [** `ModuleLoader_Add` shall add the new module to the global module loaders list. **]**

**SRS_MODULE_LOADER_13_025: [** `ModuleLoader_Add` shall unlock `g_module_loaders.lock`. **]**

**SRS_MODULE_LOADER_13_026: [** `ModuleLoader_Add` shall return `MODULE_LOADER_SUCCESS` if the loader has been added successfully. **]**

ModuleLoader_UpdateConfiguration
---------------------------------
```C
MODULE_LOADER_RESULT ModuleLoader_UpdateConfiguration(
    MODULE_LOADER* loader,
    MODULE_LOADER_BASE_CONFIGURATION* configuration
);
```

Replaces the module loader configuration for the given loader in a thread-safe manner.

**SRS_MODULE_LOADER_13_028: [** `ModuleLoader_UpdateConfiguration` shall return `MODULE_LOADER_ERROR` if `loader` is `NULL`. **]**

**SRS_MODULE_LOADER_13_029: [** `ModuleLoader_UpdateConfiguration` shall return `MODULE_LOADER_ERROR` if `g_module_loaders.lock` is `NULL`. **]**

**SRS_MODULE_LOADER_13_030: [** `ModuleLoader_UpdateConfiguration` shall return `MODULE_LOADER_ERROR` if `g_module_loaders.module_loaders` is `NULL`. **]**

**SRS_MODULE_LOADER_13_031: [** `ModuleLoader_UpdateConfiguration` shall return `MODULE_LOADER_ERROR` if an underlying platform call fails. **]**

**SRS_MODULE_LOADER_13_032: [** `ModuleLoader_UpdateConfiguration` shall lock `g_module_loaders.lock` **]**

**SRS_MODULE_LOADER_13_074: [** If the existing configuration on the loader is not `NULL` `ModuleLoader_UpdateConfiguration` shall call `FreeConfiguration` on the configuration pointer. **]**

**SRS_MODULE_LOADER_13_033: [** `ModuleLoader_UpdateConfiguration` shall assign `configuration` to the module loader. **]**

**SRS_MODULE_LOADER_13_034: [** `ModuleLoader_UpdateConfiguration` shall unlock `g_module_loaders.lock`. **]**

**SRS_MODULE_LOADER_13_035: [** `ModuleLoader_UpdateConfiguration` shall return `MODULE_LOADER_SUCCESS` if the loader has been updated successfully. **]**

ModuleLoader_FindByName
-----------------------
```C
MODULE_LOADER* ModuleLoader_FindByName(const char* name);
```

Searches the module loader collection given the loader's name.

**SRS_MODULE_LOADER_13_036: [** `ModuleLoader_FindByName` shall return `NULL` if `name` is `NULL`. **]**

**SRS_MODULE_LOADER_13_037: [** `ModuleLoader_FindByName` shall return `NULL` if `g_module_loaders.lock` is `NULL`. **]**

**SRS_MODULE_LOADER_13_038: [** `ModuleLoader_FindByName` shall return `NULL` if `g_module_loaders.module_loaders` is `NULL`.**]**

**SRS_MODULE_LOADER_13_039: [** `ModuleLoader_FindByName` shall return `NULL` if an underlying platform call fails. **]**

**SRS_MODULE_LOADER_13_040: [** `ModuleLoader_FindByName` shall lock `g_module_loaders.lock`. **]**

**SRS_MODULE_LOADER_13_041: [** `ModuleLoader_FindByName` shall search for a module loader whose name is equal to `name`. The comparison is case sensitive. **]**

**SRS_MODULE_LOADER_13_042: [** `ModuleLoader_FindByName` shall return `NULL` if a matching module loader is not found. **]**

**SRS_MODULE_LOADER_13_043: [** `ModuleLoader_FindByName` shall return a pointer to the `MODULE_LOADER` if a matching entry is found. **]**

**SRS_MODULE_LOADER_13_044: [** `ModuleLoader_FindByName` shall unlock `g_module_loaders.lock`. **]**

ModuleLoader_Destroy
--------------------
```C
void ModuleLoader_Destroy(void);
```

**SRS_MODULE_LOADER_13_045: [** `ModuleLoader_Destroy` shall free `g_module_loaders.lock` if it is not NULL. **]**

**SRS_MODULE_LOADER_13_046: [** `ModuleLoader_Destroy` shall invoke `FreeConfiguration` on every module loader's `configuration` field. **]**

**SRS_MODULE_LOADER_13_047: [** `ModuleLoader_Destroy` shall free the loader's `name` and the loader itself if it is not a default loader. **]**

**SRS_MODULE_LOADER_13_048: [** `ModuleLoader_Destroy` shall destroy the loaders vector. **]**

ModuleLoader_ParseBaseConfigurationFromJson
-------------------------------------------
```C
MODULE_LOADER_RESULT ModuleLoader_ParseBaseConfigurationFromJson(
    MODULE_LOADER_BASE_CONFIGURATION* configuration,
    const JSON_Value* json
)
```

**SRS_MODULE_LOADER_13_049: [** `ModuleLoader_ParseBaseConfigurationFromJson` shall return `MODULE_LOADER_ERROR` if `configuration` is `NULL`. **]**

**SRS_MODULE_LOADER_13_050: [** `ModuleLoader_ParseBaseConfigurationFromJson` shall return `MODULE_LOADER_ERROR` if `json` is `NULL`. **]**

**SRS_MODULE_LOADER_13_051: [** `ModuleLoader_ParseBaseConfigurationFromJson` shall return `MODULE_LOADER_ERROR` if `json` is not a JSON object. **]**

**SRS_MODULE_LOADER_13_052: [** `ModuleLoader_ParseBaseConfigurationFromJson` shall return `MODULE_LOADER_ERROR` if an underlying platform call fails. **]**

**SRS_MODULE_LOADER_13_053: [** `ModuleLoader_ParseBaseConfigurationFromJson` shall read the value of the string attribute `binding.path` from the JSON object and assign to `configuration->binding_path`. **]**

**SRS_MODULE_LOADER_13_054: [** `ModuleLoader_ParseBaseConfigurationFromJson` shall return `MODULE_LOADER_SUCCESS` if the parsing is successful. **]**

ModuleLoader_FreeBaseConfiguration
----------------------------------
```C
void ModuleLoader_FreeBaseConfiguration(MODULE_LOADER_BASE_CONFIGURATION* configuration);
```

**SRS_MODULE_LOADER_13_055: [** `ModuleLoader_FreeBaseConfiguration` shall do nothing if `configuration` is `NULL`. **]**

**SRS_MODULE_LOADER_13_056: [** `ModuleLoader_FreeBaseConfiguration` shall free `configuration->binding_path`. **]**

ModuleLoader_GetDefaultLoaderForType
------------------------------------
```C
MODULE_LOADER* ModuleLoader_GetDefaultLoaderForType(MODULE_LOADER_TYPE type);
```

**SRS_MODULE_LOADER_13_057: [** `ModuleLoader_GetDefaultLoaderForType` shall return `NULL` if `type` is not a recongized loader type. **]**

**SRS_MODULE_LOADER_13_058: [** `ModuleLoader_GetDefaultLoaderForType` shall return a non-`NULL` `MODULE_LOADER` pointer when the loader type is a recongized type. **]**

ModuleLoader_ParseType
----------------------
```C
MODULE_LOADER_TYPE ModuleLoader_ParseType(const char* type);
```

**SRS_MODULE_LOADER_13_059: [** `ModuleLoader_ParseType` shall return `UNKNOWN` if `type` is not a recognized module loader type string. **]**

**SRS_MODULE_LOADER_13_060: [** `ModuleLoader_ParseType` shall return a valid `MODULE_LOADER_TYPE` if `type` is a recognized module loader type string. **]**

ModuleLoader_IsDefaultLoader
----------------------------
```C
bool ModuleLoader_IsDefaultLoader(const char* name);
```

**SRS_MODULE_LOADER_13_061: [** `ModuleLoader_IsDefaultLoader` shall return `true` if `name` is the name of a default module loader and `false` otherwise. The default module loader names are 'native', 'node', 'java' and 'dotnet'. **]**

ModuleLoader_InitializeFromJson
-------------------------------
```C
MODULE_LOADER_RESULT ModuleLoader_InitializeFromJson(const JSON_Value* loaders);
```

**SRS_MODULE_LOADER_13_062: [** `ModuleLoader_InitializeFromJson` shall return `MODULE_LOADER_ERROR` if `loaders` is `NULL`. **]**

**SRS_MODULE_LOADER_13_063: [** `ModuleLoader_InitializeFromJson` shall return `MODULE_LOADER_ERROR` if `loaders` is not a JSON array. **]**

**SRS_MODULE_LOADER_13_064: [** `ModuleLoader_InitializeFromJson` shall return `MODULE_LOADER_ERROR` if an underlying platform call fails. **]**

**SRS_MODULE_LOADER_13_065: [** `ModuleLoader_InitializeFromJson` shall return `MODULE_LOADER_ERROR` if an entry in the `loaders` array is not a JSON object. **]**

**SRS_MODULE_LOADER_13_066: [** `ModuleLoader_InitializeFromJson` shall return `MODULE_LOADER_ERROR` if a loader entry's `name` or `type` fields are `NULL` or are empty strings. **]**

**SRS_MODULE_LOADER_13_067: [** `ModuleLoader_InitializeFromJson` shall return `MODULE_LOADER_ERROR` if a loader entry's `type` field has an unknown value. **]**

**SRS_MODULE_LOADER_13_068: [** `ModuleLoader_InitializeFromJson` shall return `MODULE_LOADER_ERROR` if no default loader could be located for a loader entry. **]**

**SRS_MODULE_LOADER_13_069: [** `ModuleLoader_InitializeFromJson` shall invoke `ParseConfigurationFromJson` to parse the loader entry's configuration JSON. **]**

**SRS_MODULE_LOADER_13_070: [** `ModuleLoader_InitializeFromJson` shall allocate a `MODULE_LOADER` and add the loader to the gateway by calling `ModuleLoader_Add` if the loader entry is not for a default loader. **]**

**SRS_MODULE_LOADER_13_071: [** `ModuleLoader_InitializeFromJson` shall return `MODULE_LOADER_ERROR` if `ModuleLoader_Add` fails. **]**

**SRS_MODULE_LOADER_13_072: [** `ModuleLoader_InitializeFromJson` shall update the configuration on the default loader if the entry is for a default loader by calling `ModuleLoader_UpdateConfiguration`. **]**

**SRS_MODULE_LOADER_13_073: [** `ModuleLoader_InitializeFromJson` shall return `MODULE_LOADER_SUCCESS` if the the JSON has been processed successfully. **]**
