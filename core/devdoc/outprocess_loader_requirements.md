Out of Process Module Loader Requirements
=========================================

Overview
--------

The out of process (outprocess) module loader implements loading of gateway modules that are proxies for modules hosted out of process.

## References
[Module loader design](./module_loaders.md)
[Out of Process Module HLD](./on-out-process-gateway-modules.md)
[Out of Process Module requirements](./outprocess_module_requirements.md)

## Exposed API
```C

#define OUTPROCESS_LOADER_ACTIVATION_TYPE_VALUES \
    OUTPROCESS_LOADER_ACTIVATION_NONE

/**
 * @brief Enumeration listing all supported module loaders
 */
DEFINE_ENUM(OUTPROCESS_LOADER_ACTIVATION_TYPE, OUTPROCESS_LOADER_ACTIVATION_TYPE_VALUES);

/** @brief Structure to load an out of process proxy module */
typedef struct OUTPROCESS_LOADER_ENTRYPOINT_TAG
{
    /** 
     * @brief Tells the module and loader how much control the gateway 
     * has over the module host process.
     */
    OUTPROCESS_LOADER_ACTIVATION_TYPE activation_type;
    /** @brief The URI for the module host control channel.*/
    STRING_HANDLE control_id;
    /** @brief The URI for the gateway message channel.*/
    STRING_HANDLE message_id;
} OUTPROCESS_LOADER_ENTRYPOINT;

/** @brief      The API for the out of process proxy module loader. */
MOCKABLE_FUNCTION(, GATEWAY_EXPORT const MODULE_LOADER*, OutprocessLoader_Get);

```

OutprocessModuleLoader_Load
---------------------------
```C
MODULE_LIBRARY_HANDLE OutprocessModuleLoader_Load(const MODULE_LOADER* loader, const void* entrypoint)
```

Loads the outprocess module according to the `entrypoint` given.  The outprocess module is statically linked in the gateway library, so there is no shared library to load.

**SRS_OUTPROCESS_LOADER_17_001: [** If `loader` or `entrypoint` are `NULL`, then this function shall return `NULL`.  **]**

**SRS_OUTPROCESS_LOADER_17_042: [** If the loader type is not `OUTPROCESS`, then this function shall return `NULL`. **]**

**SRS_OUTPROCESS_LOADER_17_002: [** If the entrypoint's `control_id` is `NULL`, then this function shall return `NULL`.  **]**
At Load time, it is permitted for `message_id` to be `NULL`.

**SRS_OUTPROCESS_LOADER_17_003: [** If the entrypoint's `activation_type` is not NONE, then this function shall return `NULL`. **]**

**SRS_OUTPROCESS_LOADER_17_004: [** The loader shall allocate memory for the loader handle. **]**

**SRS_OUTPROCESS_LOADER_17_006: [** The loader shall store a pointer to the `MODULE_API` in the loader handle. **]**

**SRS_OUTPROCESS_LOADER_17_007: [** Upon success, this function shall return a valid pointer to the loader handle. **]**

**SRS_OUTPROCESS_LOADER_17_008: [** If any call in this function fails, this function shall return `NULL`. **]**

OutprocessModuleLoader_GetModuleApi
-----------------------------------
```C
extern const MODULE_API* `OutprocessModuleLoader_GetModuleApi`(const MODULE_LOADER* loader, MODULE_LIBRARY_HANDLE moduleLibraryHandle);
```

The out of process module is linked with the outprocess loader.

**SRS_OUTPROCESS_LOADER_17_009: [** This function shall return a valid pointer to the `MODULE_API` MODULE_API. **]**

OutprocessModuleLoader_Unload
-----------------------------
```C
void OutprocessModuleLoader_Unload(const MODULE_LOADER* loader, MODULE_LIBRARY_HANDLE moduleLibraryHandle);
```

**SRS_OUTPROCESS_LOADER_17_010: [** This function shall do nothing if `moduleLibraryHandle` is `NULL`. **]**

**SRS_OUTPROCESS_LOADER_17_011: [** This function shall release all resources created by this loader. **]**

OutprocessModuleLoader_ParseEntrypointFromJson
----------------------------------------------
```C
void* OutprocessModuleLoader_ParseEntrypointFromJson(const struct MODULE_LOADER_TAG* loader, const JSON_Value* json);
```

Parses entrypoint JSON as it applies to out of process modules and returns a 
pointer to the parsed data.

**SRS_OUTPROCESS_LOADER_17_012: [** This function shall return `NULL` if `json` is `NULL`. **]**

**SRS_OUTPROCESS_LOADER_17_013: [** This function shall return `NULL` if "activation.type" is not present in `json`. **]**

**SRS_OUTPROCESS_LOADER_17_014: [** This function shall return `NULL` if "activation.type is not "NONE". **]**


**SRS_OUTPROCESS_LOADER_17_041: [** This function shall return `NULL` if "control.id" is not present in `json`. **]**

**SRS_OUTPROCESS_LOADER_17_016: [** This function shall allocate a `OUTPROCESS_LOADER_ENTRYPOINT` structure. **]**

**SRS_OUTPROCESS_LOADER_17_017: [** This function shall assign the entrypoint activation_type to NONE. **]**

**SRS_OUTPROCESS_LOADER_17_018: [** This function shall assign the entrypoint `control_id` to the string value of "ipc://" + "control.id" in `json`. **]**

**SRS_OUTPROCESS_LOADER_17_019: [** This function shall assign the entrypoint `message_id` to the string value of "ipc://" + "message.id" in `json`, `NULL` if not present. **]**

**SRS_OUTPROCESS_LOADER_17_021: [** This function shall return `NULL` if any calls fails. **]**

**SRS_OUTPROCESS_LOADER_17_022: [** This function shall return a valid pointer to an `OUTPROCESS_LOADER_ENTRYPOINT` on success. **]**

OutprocessModuleLoader_FreeEntrypoint
-------------------------------------
```C
void OutprocessModuleLoader_FreeEntrypoint(const struct MODULE_LOADER_TAG* loader, void* entrypoint)
```

**SRS_OUTPROCESS_LOADER_17_023: [** This function shall release all resources allocated by `OutprocessModuleLoader_ParseEntrypointFromJson`. **]**

OutprocessModuleLoader_ParseConfigurationFromJson
-------------------------------------------------
```C
MODULE_LOADER_BASE_CONFIGURATION* OutprocessModuleLoader_ParseConfigurationFromJson(const struct MODULE_LOADER_TAG* loader, const JSON_Value* json);
```

**SRS_OUTPROCESS_LOADER_17_024: [** The out of process loader does not have any configuration. So this method shall return `NULL`. **]**



OutprocessModuleLoader_FreeConfiguration
----------------------------------------
```C
void OutprocessModuleLoader_FreeConfiguration(const struct MODULE_LOADER_TAG* loader, MODULE_LOADER_BASE_CONFIGURATION* configuration);
```

The out of process loader does not have any configuration. So there is nothing to free here.

**SRS_OUTPROCESS_LOADER_17_025: [** This function shall move along, nothing to free here. **]**


OutprocessModuleLoader_BuildModuleConfiguration
-----------------------------------------------
```C
void* OutprocessModuleLoader_BuildModuleConfiguration(
    const MODULE_LOADER* loader,
    const void* entrypoint,
    const void* module_configuration
);
```

**SRS_OUTPROCESS_LOADER_17_026: [** This function shall return `NULL` if `loader`, `entrypoint`, `control_id`, or `module_configuration` is `NULL`. **]**  Note: If there is no module configuration, this function expects the configuration will be the string "null".

**SRS_OUTPROCESS_LOADER_17_027: [** This function shall allocate a `OUTPROCESS_MODULE_CONFIG` structure. **]**

**SRS_OUTPROCESS_LOADER_17_029: [** If the entrypoint's `message_id` is `NULL`, then the loader shall construct an IPC uri. **]** 

**SRS_OUTPROCESS_LOADER_17_030: [** The loader shall create a unique id, if needed for URI constrution. **]**

**SRS_OUTPROCESS_LOADER_17_032: [** The message uri shall be composed of "ipc://" + unique id. **]**

**SRS_OUTPROCESS_LOADER_17_033: [** This function shall allocate and copy each string in `OUTPROCESS_LOADER_ENTRYPOINT` and assign them to the corresponding fields in `OUTPROCESS_MODULE_CONFIG`. **]**

**SRS_OUTPROCESS_LOADER_17_034: [** This function shall allocate and copy the `module_configuration` string and assign it the `OUTPROCESS_MODULE_CONFIG::outprocess_module_args` field. **]**

**SRS_OUTPROCESS_LOADER_17_035: [** Upon success, this function shall return a valid pointer to an `OUTPROCESS_MODULE_CONFIG` structure. **]**

**SRS_OUTPROCESS_LOADER_17_036: [** If any call fails, this function shall return `NULL`. **]**


OutprocessModuleLoader_FreeModuleConfiguration
----------------------------------------------
```C
void OutprocessModuleLoader_FreeModuleConfiguration(const struct MODULE_LOADER_TAG* loader, const void* module_configuration);
```

**SRS_OUTPROCESS_LOADER_17_037: [** This function shall release all memory allocated by `OutprocessModuleLoader_BuildModuleConfiguration`. **]**


OutprocessModuleLoader_Get
--------------------------
```C
const MODULE_LOADER* OutprocessModuleLoader_Get(void);
```

**SRS_OUTPROCESS_LOADER_17_038: [** `OutprocessModuleLoader_Get` shall return a non-`NULL` pointer to a `MODULE_LOADER` struct. **]**

**SRS_OUTPROCESS_LOADER_17_039: [** `MODULE_LOADER::type` shall be `OUTPROCESS`. **]**

**SRS_OUTPROCESS_LOADER_17_040: [** `MODULE_LOADER::name` shall be the string 'outprocess'. **]**
