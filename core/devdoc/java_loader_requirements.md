Java Module Loader Requirements
==================================

Overview
--------

This document specifies the requirements to be implemented by the module loader
for Java binding.

References
----------
[Module loader design](./module_loaders.md)

Exposed API
-----------
```C
#define JAVA_LOADER_NAME            "java"

#if WIN32
#define JAVA_BINDING_MODULE_NAME    "java_module_host.dll"
#else
#define JAVA_BINDING_MODULE_NAME    "libjava_module_host.so"
#endif

typedef struct JAVA_LOADER_CONFIGURATION_TAG
{
    MODULE_LOADER_BASE_CONFIGURATION* base;
    JVM_OPTIONS* options;
} JAVA_LOADER_CONFIGURATION;

typedef struct JAVA_LOADER_ENTRYPOINT_TAG
{
    STRING_HANDLE className;
    STRING_HANDLE classPath;
} JAVA_LOADER_ENTRYPOINT;

const MODULE_LOADER* JavaLoader_Get(void);

MODULE_LIBRARY_HANDLE JavaModuleLoader_Load(const MODULE_LOADER* loader, const void* entrypoint);

const MODULE_API* JavaModuleLoader_GetModuleApi(const MODULE_LOADER* loader, MODULE_LIBRARY_HANDLE moduleLibraryHandle);

void JavaModuleLoader_Unload(const MODULE_LOADER* loader, MODULE_LIBRARY_HANDLE moduleLibraryHandle);

void* JavaModuleLoader_ParseEntrypointFromJson(const MODULE_LOADER* loader, const JSON_Value* json);

void JavaModuleLoader_FreeEntrypoint(const MODULE_LOADER* loader, void* entrypoint);

MODULE_LOADER_BASE_CONFIGURATION* JavaModuleLoader_ParseConfigurationFromJson(const MODULE_LOADER* loader, const JSON_Value* json);

void JavaModuleLoader_FreeConfiguration(const MODULE_LOADER* loader, MODULE_LOADER_BASE_CONFIGURATION* configuration);

void* JavaModuleLoader_BuildModuleConfiguration(
    const MODULE_LOADER* loader,
    const void* entrypoint,
    const void* module_configuration
);

void JavaModuleLoader_FreeModuleConfiguration(const MODULE_LOADER* loader, const void* module_configuration);
```

JavaModuleLoader_Load
---------------------
```C
MODULE_LIBRARY_HANDLE JavaModuleLoader_Load(const MODULE_LOADER* loader, const void* entrypoint);
```

Loads the Java binding module into memory.

**SRS_JAVA_MODULE_LOADER_14_001: [** `JavaModuleLoader_Load` shall return `NULL` if `loader` is `NULL`. **]**

**SRS_JAVA_MODULE_LOADER_14_002: [** `JavaModuleLoader_Load` shall return `NULL` if `loader->type` is not `JAVA`. **]**

**SRS_JAVA_MODULE_LOADER_14_003: [** `JavaModuleLoader_Load` shall return `NULL` if an underlying platform call fails. **]**

**SRS_JAVA_MODULE_LOADER_14_004: [** `JavaModuleLoader_Load` shall load the binding module library into memory by calling `DynamicLibrary_LoadLibrary`. **]**

**SRS_JAVA_MODULE_LOADER_14_005: [** `JavaModuleLoader_Load` shall use the binding module path given in `loader->configuration->binding_path` if `loader->configuration` is not `NULL`. **]**

**SRS_JAVA_MODULE_LOADER_14_006: [** `JavaModuleLoader_Load` shall call `DynamicLibrary_FindSymbol` on the binding module handle with the symbol name `Module_GetApi` to acquire the module's API table. **]**

**SRS_JAVA_MODULE_LOADER_14_007: [** `JavaModuleLoader_Load` shall return a non-`NULL` pointer of type `MODULE_LIBRARY_HANDLE` when successful. **]**

**SRS_JAVA_MODULE_LOADER_14_008: [** `JavaModuleLoader_Load` shall return `NULL` if `MODULE_API` returned by the binding module is `NULL`. **]**

**SRS_JAVA_MODULE_LOADER_14_009: [** `JavaModuleLoader_Load` shall return `NULL` if `MODULE_API::version` is great than `Module_ApiGatewayVersion`. **]**

**SRS_JAVA_MODULE_LOADER_14_010: [** `JavaModuleLoader_Load` shall return `NULL` if the `Module_Create` function in `MODULE_API` is `NULL`. **]**

**SRS_JAVA_MODULE_LOADER_14_011: [** `JavaModuleLoader_Load` shall return `NULL` if the `Module_Receive` function in `MODULE_API` is `NULL`. **]**

**SRS_JAVA_MODULE_LOADER_14_012: [** `JavaModuleLoader_Load` shall return `NULL` if the `Module_Destroy` function in `MODULE_API` is `NULL`. **]**

JavaModuleLoader_GetModuleApi
-----------------------------
```C
const MODULE_API* JavaModuleLoader_GetModuleApi(const MODULE_LOADER* loader, MODULE_LIBRARY_HANDLE moduleLibraryHandle);
```

Returns the `MODULE_API` struct for the binding module.

**SRS_JAVA_MODULE_LOADER_14_013: [** `JavaModuleLoader_GetModuleApi` shall return `NULL` if `moduleLibraryHandle` is `NULL`. **]**

**SRS_JAVA_MODULE_LOADER_14_014: [** `JavaModuleLoader_GetModuleApi` shall return a non-`NULL` `MODULE_API` pointer when successful. **]**

JavaModuleLoader_Unload
-----------------------
```C
void JavaModuleLoader_Unload(const MODULE_LOADER* loader, MODULE_LIBRARY_HANDLE moduleLibraryHandle)
```

Unloads the binding module from memory.

**SRS_JAVA_MODULE_LOADER_14_015: [** `JavaModuleLoader_Unload` shall do nothing if `moduleLibraryHandle` is `NULL`. **]**

**SRS_JAVA_MODULE_LOADER_14_016: [** `JavaModuleLoader_Unload` shall unload the binding module from memory by calling `DynamicLibrary_UnloadLibrary`. **]**

**SRS_JAVA_MODULE_LOADER_14_017: [** `JavaModuleLoader_Unload` shall free resources allocated when loading the binding module. **]**

JavaModuleLoader_ParseEntrypointFromJson
----------------------------------------
```C
void* JavaModuleLoader_ParseEntrypointFromJson(const MODULE_LOADER* loader, const JSON_Value* json);
```

Parses entrypoint JSON as it applies to a Java module and returns a pointer
to the parsed data.

**SRS_JAVA_MODULE_LOADER_14_043: [** `JavaModuleLoader_ParseEntrypointFromJson` shall return `NULL` if `loader` is `NULL`. **]**

**SRS_JAVA_MODULE_LOADER_14_018: [** `JavaModuleLoader_ParseEntrypointFromJson` shall return `NULL` if `json` is `NULL`. **]**

**SRS_JAVA_MODULE_LOADER_14_019: [** `JavaModuleLoader_ParseEntrypointFromJson` shall return `NULL` if the root json entity is not an object. **]**

**SRS_JAVA_MODULE_LOADER_14_058: [** `JavaModuleLoader_ParseEntrypointFromJson` shall return `NULL` if either `class.name` or `class.path` is non-existent. **]**

**SRS_JAVA_MODULE_LOADER_14_059: [** `JavaModuleLoader_ParseEntrypointFromJson` shall return `NULL` if `loader->configuration`. **]**

**SRS_JAVA_MODULE_LOADER_14_060: [** `JavaModuleLoader_ParseEntrypointFromJson` shall return `NULL` if `loader->configuration->options` is `NULL`. **]**

**SRS_JAVA_MODULE_LOADER_14_020: [** `JavaModuleLoader_ParseEntrypointFromJson` shall return `NULL` if an underlying platform call fails. **]**

**SRS_JAVA_MODULE_LOADER_14_021: [** `JavaModuleLoader_ParseEntrypointFromJson` shall retreive the fully qualified class name by reading the value of the attribute `class.name`. **]**

**SRS_JAVA_MODULE_LOADER_14_022: [** `JavaModuleLoader_ParseEntrypointFromJson` shall retreive the full class path containing the class definition for the module by reading the value of the attribute `class.path`. **]**

**SRS_JAVA_MODULE_LOADER_14_023: [** `JavaModuleLoader_ParseEntrypointFromJson` shall return a non-`NULL` pointer to the parsed representation of the entrypoint when successful. **]**

**SRS_JAVA_MODULE_LOADER_14_044: [** `JavaModuleLoader_ParseEntrypointFromJson` shall append the classpath to the `loader`'s configuration `JVM_OPTIONS` `class_path` member. **]**

JavaModuleLoader_FreeEntrypoint
-------------------------------
```C
void JavaModuleLoader_FreeEntrypoint(const MODULE_LOADER* loader, void* entrypoint)
```

Frees entrypoint data allocated by `JavaModuleLoader_ParseEntrypointFromJson`.

**SRS_JAVA_MODULE_LOADER_14_024: [** `JavaModuleLoader_FreeEntrypoint` shall do nothing if `entrypoint` is `NULL`. **]**

**SRS_JAVA_MODULE_LOADER_14_025: [** `JavaModuleLoader_FreeEntrypoint` shall free resources allocated during `JavaModuleLoader_ParseEntrypointFromJson`. **]**

JavaModuleLoader_ParseConfigurationFromJson
-------------------------------------------
```C
MODULE_LOADER_BASE_CONFIGURATION* JavaModuleLoader_ParseConfigurationFromJson(const MODULE_LOADER* loader, const JSON_Value* json);
```

Parses loader configuration JSON and returns a pointer to the parsed data.

Parses the module loader configuration from the given JSON and returns a `JAVA_LOADER_CONFIGURATION` pointer.

The JSON object will be the `"jvm.options"` and `"binding.path"` sections of the Gateway JSON `"java"` loader configuration and should be formatted as follows:
```json
{
    "loaders": [
        {
            "type": "java",
            "name": "java",
            "jvm.options":{
                ... JVM options here ...
            },
            "binding.path": "/optional/path/to/java_binding/library"
        }
    ],
    "modules": [
        ... modules listing goes here ...
    ]
}
```

Applicable keys within the `"jvm.options"` object are:

### "library.path"
Path to the Java module host native library. In most cases this should be the same as the `"binding.path"` if one is given.

_Default: default binding path_

### "version"
The major Java version for the JVM

_Default: 4_

### "debug"
Boolean value indicating whether this module should be in debug mode

_Default: false_

### "debug.port"
The remote debugging port.

_Default: 9876_

### "verbose"
Boolean value indicating whether to provide verbose output. The "-verbose:class" option will be set.
To change the verbose setting, provide it in the "additional.options" section.

### "additional.options"
An array of any additional applicable JVM options listed as an array of strings.

_Default: null_

**SRS_JAVA_MODULE_LOADER_14_026: [** `JavaModuleLoader_ParseConfigurationFromJson` shall return `NULL` if `json` is `NULL`. **]**

**SRS_JAVA_MODULE_LOADER_14_027: [** `JavaModuleLoader_ParseConfigurationFromJson` shall return `NULL` if `json` is not a valid JSON object. **]**

**SRS_JAVA_MODULE_LOADER_14_028: [** `JavaModuleLoader_ParseConfigurationFromJson` shall return `NULL` if any underlying platform call fails. **]**

**SRS_JAVA_MODULE_LOADER_14_029: [** `JavaModuleLoader_ParseConfigurationFromJson` shall set any missing field to NULL, false, or 0. **]**

**SRS_JAVA_MODULE_LOADER_14_030: [** `JavaModuleLoader_ParseConfigurationFromJson` shall parse the `jvm.options` JSON object and initialize a new `JAVA_LOADER_CONFIGURATION`. **]**

**SRS_JAVA_MODULE_LOADER_14_031: [** `JavaModuleLoader_ParseConfigurationFromJson` shall parse the `jvm.options.library.path`. **]**

**SRS_JAVA_MODULE_LOADER_14_032: [** `JavaModuleLoader_ParseConfigurationFromJson` shall parse the `jvm.options.version`. **]**

**SRS_JAVA_MODULE_LOADER_14_033: [** `JavaModuleLoader_ParseConfigurationFromJson` shall parse the `jvm.options.debug`. **]**

**SRS_JAVA_MODULE_LOADER_14_034: [** `JavaModuleLoader_ParseConfigurationFromJson` shall parse the `jvm.options.debug.port`. **]**

**SRS_JAVA_MODULE_LOADER_14_035: [** `JavaModuleLoader_ParseConfigurationFromJson` shall parse the `jvm.options.additional.options` object and create a new `STRING_HANDLE` for each. **]**

**SRS_JAVA_MODULE_LOADER_14_036: [** `JavaModuleLoader_ParseConfigurationFromJson` shall return `NULL` if any present field cannot be parsed. **]**

**SRS_JAVA_MODULE_LOADER_14_037: [** `JavaModuleLoader_ParseConfigurationFromJson` shall return a non-`NULL` `JAVA_LOADER_CONFIGURATION` containing all user-specified values. **]**

**SRS_JAVA_MODULE_LOADER_14_038: [** `JavaModuleLoader_ParseConfigurationFromJson` shall set the `options` member of the `JAVA_LOADER_CONFIGURATION` to the parsed `JVM_OPTIONS` structure. **]**

**SRS_JAVA_MODULE_LOADER_14_039: [** `JavaModuleLoader_ParseConfigurationFromJson` shall set the `base` member of the `JAVA_LOADER_CONFIGURATION` by calling to the base module loader. **]**

JavaModuleLoader_FreeConfiguration
----------------------------------
```C
void JavaModuleLoader_FreeConfiguration(const MODULE_LOADER* loader, MODULE_LOADER_BASE_CONFIGURATION* configuration);
```

Frees loader configuration data as allocated by `JavaModuleLoader_ParseConfigurationFromJson`.

**SRS_JAVA_MODULE_LOADER_14_040: [** `JavaModuleLoader_FreeConfiguration` shall do nothing if `configuration` is `NULL`. **]**

**SRS_JAVA_MODULE_LOADER_14_041: [** `JavaModuleLoader_FreeConfiguration` shall call `ModuleLoader_FreeBaseConfiguration` to free resources allocated by `ModuleLoader_ParseBaseConfigurationJson`. **]**

**SRS_JAVA_MODULE_LOADER_14_042: [** `JavaModuleLoader_FreeConfiguration` shall free resources allocated by `JavaModuleLoader_ParseConfigurationFromJson`. **]**

JavaModuleLoader_BuildModuleConfiguration
-----------------------------------------
```C
void* JavaModuleLoader_BuildModuleConfiguration(
    const MODULE_LOADER* loader,
    const void* entrypoint,
    const void* module_configuration
);
```

Builds a `JAVA_MODULE_HOST_CONFIG` object by merging information from `entrypoint` and `module_configuration`.

**SRS_JAVA_MODULE_LOADER_14_045: [** `JavaModuleLoader_BuildModuleConfiguration` shall return `NULL` if `entrypoint` is `NULL`. **]**

**SRS_JAVA_MODULE_LOADER_14_046: [** `JavaModuleLoader_BuildModuleConfiguration` shall return `NULL` if `entrypoint->className` is `NULL`. **]**

**SRS_JAVA_MODULE_LOADER_14_047: [** `JavaModuleLoader_BuildModuleConfiguration` shall return `NULL` if `loader` is `NULL`. **]**

**SRS_JAVA_MODULE_LOADER_14_048: [** `JavaModuleLoader_BuildModuleConfiguration` shall return `NULL` if `loader->options` is `NULL`. **]**

**SRS_JAVA_MODULE_LOADER_14_049: [** `JavaModuleLoader_BuildModuleConfiguration` shall return `NULL` if an underlying platform call fails. **]**

**SRS_JAVA_MODULE_LOADER_14_050: [** `JavaModuleLoader_BuildModuleConfiguration` shall build a `JAVA_MODULE_HOST_CONFIG` object by copying information from `entrypoint`, `module_configuration`, and `loader->options` and return a non-`NULL` pointer. **]**

JavaModuleLoader_FreeModuleConfiguration
----------------------------------------
```C
void JavaModuleLoader_FreeModuleConfiguration(const MODULE_LOADER* loader, const void* module_configuration);
```

Frees the `JAVA_MODULE_HOST_CONFIG` object allocated by `JavaModuleLoader_BuildModuleConfiguration`.

**SRS_JAVA_MODULE_LOADER_14_051: [** `JavaModuleLoader_FreeModuleConfiguration` shall do nothing if `module_configuration` is `NULL`. **]**

**SRS_JAVA_MODULE_LOADER_14_052: [** `JavaModuleLoader_FreeModuleConfiguration` shall free the `JAVA_MODULE_HOST_CONFIG` object. **]**

JavaLoader_Get
--------------
```C
const MODULE_LOADER* JavaLoader_Get(void)
```

**SRS_JAVA_MODULE_LOADER_14_053: [** `JavaLoader_Get` shall return a non-`NULL` pointer to a `MODULE_LOADER` struct. **]**

**SRS_JAVA_MODULE_LOADER_14_054: [** `MODULE_LOADER::type` shall by `JAVA`. **]**

**SRS_JAVA_MODULE_LOADER_14_055: [** `MODULE_LOADER::name` shall be the string `java`. **]**

**SRS_JAVA_MODULE_LOADER_14_056: [** `JavaLoader_Get` shall set the `loader->configuration` to a default `JAVA_LOADER_CONFIGURATION` by setting it to default values. **]**

**SRS_JAVA_MODULE_LOADER_14_057: [** `JavaLoader_Get` shall return `NULL` if any underlying call fails while attempting to set the default `JAVA_LOADER_CONFIGURATION`. **]**