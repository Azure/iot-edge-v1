Node.js Module Loader Requirements
==================================

Overview
--------

This document specifies the requirements to be implemented by the module loader
for Node.js binding.

References
----------
[Module loader design](./module_loaders.md)

Exposed API
-----------
```C
#define NODE_LOADER_NAME            "node"

#if WIN32
#define NODE_BINDING_MODULE_NAME    "nodejs_binding.dll"
#else
#define NODE_BINDING_MODULE_NAME    "libnodejs_binding.so"
#endif

typedef struct NODE_LOADER_ENTRYPOINT_TAG
{
    STRING_HANDLE mainPath;
} NODE_LOADER_ENTRYPOINT;

const MODULE_LOADER* NodeLoader_Get(void);

MODULE_LIBRARY_HANDLE NodeModuleLoader_Load(const MODULE_LOADER* loader, const void* entrypoint);

const MODULE_API* NodeModuleLoader_GetModuleApi(const MODULE_LOADER* loader, MODULE_LIBRARY_HANDLE moduleLibraryHandle);

void NodeModuleLoader_Unload(const MODULE_LOADER* loader, MODULE_LIBRARY_HANDLE moduleLibraryHandle);

void* NodeModuleLoader_ParseEntrypointFromJson(const MODULE_LOADER* loader, const JSON_Value* json);

void NodeModuleLoader_FreeEntrypoint(const MODULE_LOADER* loader, void* entrypoint);

MODULE_LOADER_BASE_CONFIGURATION* NodeModuleLoader_ParseConfigurationFromJson(const MODULE_LOADER* loader, const JSON_Value* json);

void NodeModuleLoader_FreeConfiguration(const MODULE_LOADER* loader, MODULE_LOADER_BASE_CONFIGURATION* configuration);

void* NodeModuleLoader_BuildModuleConfiguration(
    const MODULE_LOADER* loader,
    const void* entrypoint,
    const void* module_configuration
);

void NodeModuleLoader_FreeModuleConfiguration(const MODULE_LOADER* loader, const void* module_configuration);
```

NodeModuleLoader_Load
---------------------
```C
MODULE_LIBRARY_HANDLE NodeModuleLoader_Load(const MODULE_LOADER* loader, const void* entrypoint);
```

Loads the Node.js binding module into memory.

**SRS_NODE_MODULE_LOADER_13_001: [** `NodeModuleLoader_Load` shall return `NULL` if `loader` is `NULL`. **]**

**SRS_NODE_MODULE_LOADER_13_002: [** `NodeModuleLoader_Load` shall return `NULL` if `loader->type` is not `NODEJS`. **]**

**SRS_NODE_MODULE_LOADER_13_003: [** `NodeModuleLoader_Load` shall return `NULL` if an underlying platform call fails. **]**

**SRS_NODE_MODULE_LOADER_13_004: [** `NodeModuleLoader_Load` shall load the binding module library into memory by calling `DynamicLibrary_LoadLibrary`. **]**

**SRS_NODE_MODULE_LOADER_13_032: [** `NodeModuleLoader_Load` shall use the the binding module path given in `loader->configuration->binding_path` if `loader->configuration` is not `NULL`. **]**

**SRS_NODE_MODULE_LOADER_13_033: [** `NodeModuleLoader_Load` shall call `DynamicLibrary_FindSymbol` on the binding module handle with the symbol name `Module_GetApi` to acquire the function that returns the module's API table. **]**

**SRS_NODE_MODULE_LOADER_13_005: [** `NodeModuleLoader_Load` shall return a non-`NULL` pointer of type `MODULE_LIBRARY_HANDLE` when successful. **]**

**SRS_NODE_MODULE_LOADER_13_034: [** `NodeModuleLoader_Load` shall return `NULL` if the `MODULE_API` pointer returned by the binding module is `NULL`. **]**

**SRS_NODE_MODULE_LOADER_13_035: [** `NodeModuleLoader_Load` shall return `NULL` if `MODULE_API::version` is greater than `Module_ApiGatewayVersion`. **]**

**SRS_NODE_MODULE_LOADER_13_036: [** `NodeModuleLoader_Load` shall return `NULL` if the `Module_Create` function in `MODULE_API` is `NULL`. **]**

**SRS_NODE_MODULE_LOADER_13_037: [** `NodeModuleLoader_Load` shall return `NULL` if the `Module_Receive` function in `MODULE_API` is `NULL`. **]**

**SRS_NODE_MODULE_LOADER_13_038: [** `NodeModuleLoader_Load` shall return `NULL` if the `Module_Destroy` function in `MODULE_API` is `NULL`. **]**

NodeModuleLoader_GetModuleApi
-----------------------------
```C
const MODULE_API* NodeModuleLoader_GetModuleApi(const MODULE_LOADER* loader, MODULE_LIBRARY_HANDLE moduleLibraryHandle);
```

Returns the `MODULE_API` struct for the binding module.

**SRS_NODE_MODULE_LOADER_13_006: [** `NodeModuleLoader_GetModuleApi` shall return `NULL` if `moduleLibraryHandle` is `NULL`. **]**

**SRS_NODE_MODULE_LOADER_13_007: [** `NodeModuleLoader_GetModuleApi` shall return a non-`NULL` `MODULE_API` pointer when successful. **]**

NodeModuleLoader_Unload
-----------------------
```C
void NodeModuleLoader_Unload(const MODULE_LOADER* loader, MODULE_LIBRARY_HANDLE moduleLibraryHandle)
```

Unloads the binding module from memory.

**SRS_NODE_MODULE_LOADER_13_008: [** `NodeModuleLoader_Unload` shall do nothing if `moduleLibraryHandle` is `NULL`. **]**

**SRS_NODE_MODULE_LOADER_13_009: [** `NodeModuleLoader_Unload` shall unload the binding module from memory by calling `DynamicLibrary_UnloadLibrary`. **]**

**SRS_NODE_MODULE_LOADER_13_010: [** `NodeModuleLoader_Unload` shall free resources allocated when loading the binding module. **]**

NodeModuleLoader_ParseEntrypointFromJson
----------------------------------------
```C
void* NodeModuleLoader_ParseEntrypointFromJson(const MODULE_LOADER* loader, const JSON_Value* json);
```

Parses entrypoint JSON as it applies to a Node.js module and returns a pointer
to the parsed data.

**SRS_NODE_MODULE_LOADER_13_011: [** `NodeModuleLoader_ParseEntrypointFromJson` shall return `NULL` if `json` is `NULL`. **]**

**SRS_NODE_MODULE_LOADER_13_012: [** `NodeModuleLoader_ParseEntrypointFromJson` shall return `NULL` if the root json entity is not an object. **]**

**SRS_NODE_MODULE_LOADER_13_013: [** `NodeModuleLoader_ParseEntrypointFromJson` shall return `NULL` if an underlying platform call fails. **]**

**SRS_NODE_MODULE_LOADER_13_014: [** `NodeModuleLoader_ParseEntrypointFromJson` shall retrieve the path to the main JS file by reading the value of the attribute `main.path`. **]**

**SRS_NODE_MODULE_LOADER_13_039: [** `NodeModuleLoader_ParseEntrypointFromJson` shall return `NULL` if `main.path` does not exist. **]**

**SRS_NODE_MODULE_LOADER_13_015: [** `NodeModuleLoader_ParseEntrypointFromJson` shall return a non-`NULL` pointer to the parsed representation of the entrypoint when successful. **]**

NodeModuleLoader_FreeEntrypoint
-------------------------------
```C
void NodeModuleLoader_FreeEntrypoint(const MODULE_LOADER* loader, void* entrypoint)
```

Frees entrypoint data allocated by `NodeModuleLoader_ParseEntrypointFromJson`.

**SRS_NODE_MODULE_LOADER_13_016: [** `NodeModuleLoader_FreeEntrypoint` shall do nothing if `entrypoint` is `NULL`. **]**

**SRS_NODE_MODULE_LOADER_13_017: [** `NodeModuleLoader_FreeEntrypoint` shall free resources allocated during `NodeModuleLoader_ParseEntrypointFromJson`. **]**

NodeModuleLoader_ParseConfigurationFromJson
-------------------------------------------
```C
MODULE_LOADER_BASE_CONFIGURATION* NodeModuleLoader_ParseConfigurationFromJson(const MODULE_LOADER* loader, const JSON_Value* json);
```

Parses loader configuration JSON and returns a pointer to the parsed data.

Parses the module loader configuration from the given JSON and returns a `MODULE_LOADER_BASE_CONFIGURATION` pointer.

**SRS_NODE_MODULE_LOADER_13_018: [** `NodeModuleLoader_ParseConfigurationFromJson` shall return `NULL` if an underlying platform call fails. **]**

**SRS_NODE_MODULE_LOADER_13_019: [** `NodeModuleLoader_ParseConfigurationFromJson` shall call `ModuleLoader_ParseBaseConfigurationFromJson` to parse the loader configuration and return the result. **]**

NodeModuleLoader_FreeConfiguration
----------------------------------
```C
void NodeModuleLoader_FreeConfiguration(const MODULE_LOADER* loader, MODULE_LOADER_BASE_CONFIGURATION* configuration);
```

Frees loader configuration data as allocated by `NodeModuleLoader_ParseConfigurationFromJson`.

**SRS_NODE_MODULE_LOADER_13_020: [** `NodeModuleLoader_FreeConfiguration` shall do nothing if `configuration` is `NULL`. **]**

**SRS_NODE_MODULE_LOADER_13_021: [** `NodeModuleLoader_FreeConfiguration` shall call `ModuleLoader_FreeBaseConfiguration` to free resources allocated by `ModuleLoader_ParseBaseConfigurationFromJson`. **]**

**SRS_NODE_MODULE_LOADER_13_022: [** `NodeModuleLoader_FreeConfiguration` shall free resources allocated by `NodeModuleLoader_ParseConfigurationFromJson`. **]**

NodeModuleLoader_BuildModuleConfiguration
-----------------------------------------
```C
void* NodeModuleLoader_BuildModuleConfiguration(
    const MODULE_LOADER* loader,
    const void* entrypoint,
    const void* module_configuration
);
```

Builds a `NODEJS_MODULE_CONFIG` object by merging information from `entrypoint` and `module_configuration`.

**SRS_NODE_MODULE_LOADER_13_023: [** `NodeModuleLoader_BuildModuleConfiguration` shall return `NULL` if `entrypoint` is `NULL`. **]**

**SRS_NODE_MODULE_LOADER_13_024: [** `NodeModuleLoader_BuildModuleConfiguration` shall return `NULL` if `entrypoint->mainPath` is `NULL`. **]**

**SRS_NODE_MODULE_LOADER_13_025: [** `NodeModuleLoader_BuildModuleConfiguration` shall return `NULL` if an underlying platform call fails. **]**

**SRS_NODE_MODULE_LOADER_13_026: [** `NodeModuleLoader_BuildModuleConfiguration` shall build a `NODEJS_MODULE_CONFIG` object by copying information from `entrypoint` and `module_configuration` and return a non-`NULL` pointer. **]**

NodeModuleLoader_FreeModuleConfiguration
----------------------------------------
```C
void NodeModuleLoader_FreeModuleConfiguration(const MODULE_LOADER* loader, const void* module_configuration);
```

Frees the `NODEJS_MODULE_CONFIG` object allocated by `NodeModuleLoader_BuildModuleConfiguration`.

**SRS_NODE_MODULE_LOADER_13_027: [** `NodeModuleLoader_FreeModuleConfiguration` shall do nothing if `module_configuration` is `NULL`. **]**

**SRS_NODE_MODULE_LOADER_13_028: [** `NodeModuleLoader_FreeModuleConfiguration` shall free the `NODEJS_MODULE_CONFIG` object. **]**

NodeLoader_Get
--------------
```C
const MODULE_LOADER* NodeLoader_Get(void)
```

**SRS_NODE_MODULE_LOADER_13_029: [** `NodeLoader_Get` shall return a non-`NULL` pointer to a `MODULE_LOADER` struct. **]**

**SRS_NODE_MODULE_LOADER_13_030: [** `MODULE_LOADER::type` shall be `NODEJS`. **]**

**SRS_NODE_MODULE_LOADER_13_031: [** `MODULE_LOADER::name` shall be the string 'node'. **]**
