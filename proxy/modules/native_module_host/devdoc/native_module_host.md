MODULE HOST FOR OUT OF PROCESS
==============================

Overview
--------

This is the module host for pre-build gateway modules  - modules in the form of a DLL or so, like the [BLE module](../../ble/devdoc/blemodule_requirements.md). These modules need to be loaded as a shared library during runtime, so they require additional functionality not present in the proxy gateway.

The module host requires the following format in the gateway JSON configuration:
```JSON
{ 
    "name" : "outprocess_module",
    "loader" : {
        "name" : "outprocess",
        "entrypoint" : {
            "activation.type" : "none",
            "control.id" : "outprocess_module_control",
        }
    },
    "args" : {
        "outprocess.loaders" : [
            // optional array of loader configurations.
        ],
        "outprocess.loader" : { // a valid loader entry for module
        },
        "module.args" : { // module arguments expected for module
        }
    }
}
```

An example of a correctly declared out of process module using the module host.

```JSON
{ 
    "name" : "azure_function_outprocess",
    "loader" : {
        "name" : "outprocess",
        "entrypoint" : {
            "activation.type" : "none",
            "control.id" : "outprocess_module_control",
        }
    },
    "args" : {
        "outprocess.loader" : {
            "name": "native",
            "entrypoint": {
                "module.path": "modules\\azure_functions\\Debug\\azure_functions.dll"
            }
        },
        "module.arguments" : {
            "hostname": "<YourHostNameHere from Functions Portal>",
            "relativePath": "api/<YourFunctionName>",
            "key": "<your Api Key Here>"
        }
    }
}
```

"args" section for native module host
-------------------------------------

The "args" section for an out of process native module host must contain the information needed to load the gateway module.  It has three sections, "outprocess.loaders", "outprocess.loader", and "module.arguments".

### "outprocess.loaders"

This is the equivalent of the "loaders" section in the gateway JSON configuration. This may be used to configure module loaders on the remote process. It is an array of loader entries, which may configure or extend loader capability. See the [Module Loaders description](../../../../core/devdoc/module_loaders.md) for more details.

- **"type"** The loader type, one of the preconfigured module loader types.

- **"name"** The loader name. If this is a name of a preconfigured loader type, the configuration of the default loader will updated.  If this does not match a a preconfigured loader type, then a new module loader entry will be created for it.

- **"configuration"** Information specific to the module loader type, such as runtime options.

### "outprocess.loader"

This is the equivalent to the "loader" statement in a module entry of the gateway JSON. This tells the native module host how to load the remote module.

### "module.arguments"

This is equivalent to the "args" statement in a module entry of the gateway JSON. This provides the configuration for the module loaded by the native module host.


Exposed API
-----------

```c
#define OOP_MODULE_LOADERS_ARRAY_KEY "outprocess.loaders"
#define OOP_MODULE_LOADER_KEY "outprocess.loader"
#define OOP_MODULE_ARGS_KEY "module.args"

#ifdef __cplusplus
extern "C"
{
#endif

MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(OOP_MODULE_HOST)(MODULE_API_VERSION gateway_api_version);

#ifdef __cplusplus
}
#endif
```

References
----------

[Outprocess High-Level Design](outprocess_hld.md)


Module\_GetApi
--------------
```c
const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version)
```

**SRS_NATIVEMODULEHOST_17_001: [** `Module_GetApi` shall return a valid pointer to a MODULE_API structure. **]**

**SRS_NATIVEMODULEHOST_17_002: [** The `MODULE_API` structure returned shall have valid pointers for all pointers in the strcuture. **]**

NativeModuleHost\_ParseConfigurationFromJson
--------------
```c 
void* NativeModuleHost_ParseConfigurationFromJson(const char* configuration)
```

**SRS_NATIVEMODULEHOST_17_003: [** `NativeModuleHost_ParseConfigurationFromJson` shall return NULL if `configuration` is `NULL`. **]**

**SRS_NATIVEMODULEHOST_17_004: [** On success, `NativeModuleHost_ParseConfigurationFromJson` shall return a valid copy of the configuration string given. **]**

**SRS_NATIVEMODULEHOST_17_005: [** `NativeModuleHost_ParseConfigurationFromJson` shall return `NULL` if any system call fails. **]**

NativeModuleHost\_FreeConfiguration
--------------
```c
void NativeModuleHost_FreeConfiguration(void* configuration)
```

**SRS_NATIVEMODULEHOST_17_006: [** `NativeModuleHost_FreeConfiguration` shall do nothing if passed a `NULL` `configuration`. **]**

**SRS_NATIVEMODULEHOST_17_007: [** `NativeModuleHost_FreeConfiguration` shall free `configuration`. **]**

NativeModuleHost\_Create
--------------
```c
MODULE_HANDLE NativeModuleHost_Create(BROKER_HANDLE broker, const void* configuration)
```

**SRS_NATIVEMODULEHOST_17_008: [** `NativeModuleHost_Create` shall return `NULL` if `broker` is `NULL`. **]**

**SRS_NATIVEMODULEHOST_17_009: [** `NativeModuleHost_Create` shall return `NULL` if `configuration` does not contain valid JSON. **]**

**SRS_NATIVEMODULEHOST_17_011: [** `NativeModuleHost_Create` shall parse the configuration JSON. **]**

**SRS_NATIVEMODULEHOST_17_010: [** `NativeModuleHost_Create` shall intialize the `Module_Loader`. **]**

**SRS_NATIVEMODULEHOST_17_035: [** If the "outprocess.loaders" array exists in the configuration JSON, `NativeModuleHost_Create` shall initialize the `Module_Loader` from this array. **]**

**SRS_NATIVEMODULEHOST_17_012: [** `NativeModuleHost_Create` shall get the "outprocess.loader" object from the configuration JSON. **]**

**SRS_NATIVEMODULEHOST_17_013: [** `NativeModuleHost_Create` shall get "name" string from the "outprocess.loader" object. **]**

**SRS_NATIVEMODULEHOST_17_014: [** If the loader name string is not present, then `NativeModuleHost_Create` shall assume the loader is the native dynamic library loader name. **]**

**SRS_NATIVEMODULEHOST_17_015: [** `NativeModuleHost_Create` shall find the module loader by name. **]**

**SRS_NATIVEMODULEHOST_17_016: [** `NativeModuleHost_Create` shall get "entrypoint" string from the "outprocess.loader" object. **]**

**SRS_NATIVEMODULEHOST_17_017: [** `NativeModuleHost_Create` shall parse the entrypoint string. **]**

**SRS_NATIVEMODULEHOST_17_018: [** `NativeModuleHost_Create` shall get the "module.args" object from the configuration JSON. **]**

**SRS_NATIVEMODULEHOST_17_019: [** `NativeModuleHost_Create` shall load the module library with the pasred entrypoint. **]**

**SRS_NATIVEMODULEHOST_17_020: [** `NativeModuleHost_Create` shall get the MODULE_API pointer from the loader. **]**

**SRS_NATIVEMODULEHOST_17_021: [** `NativeModuleHost_Create` shall call the module's \_ParseConfigurationFromJson on the "module.args" object. **]**

**SRS_NATIVEMODULEHOST_17_022: [** `NativeModuleHost_Create` shall build the module confguration from the parsed entrypoint and parsed module arguments. **]**

**SRS_NATIVEMODULEHOST_17_023: [** `NativeModuleHost_Create` shall call the module's \_Create on the built module configuration. **]**

**SRS_NATIVEMODULEHOST_17_024: [** `NativeModuleHost_Create` shall free all resources used during module loading. **]**

**SRS_NATIVEMODULEHOST_17_025: [** `NativeModuleHost_Create` shall return a non-null pointer to a MODULE_HANDLE on success. **]**

**SRS_NATIVEMODULEHOST_17_026: [** If any step above fails, then `NativeModuleHost_Create` shall free all resources allocated and return `NULL`. **]**

NativeModuleHost\_Destroy
--------------
```c
void NativeModuleHost_Destroy(MODULE_HANDLE moduleHandle)
```

**SRS_NATIVEMODULEHOST_17_027: [** `NativeModuleHost_Destroy` shall always destroy the module loader. **]**

**SRS_NATIVEMODULEHOST_17_028: [** `NativeModuleHost_Destroy` shall free all remaining allocated resources if moduleHandle is not `NULL`. **]**

NativeModuleHost\_Receive
--------------
```c
void NativeModuleHost_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
```

**SRS_NATIVEMODULEHOST_17_029: [** `NativeModuleHost_Receive` shall do nothing if `moduleHandle` is `NULL`. **]**

**SRS_NATIVEMODULEHOST_17_030: [** `NativeModuleHost_Receive` shall get the loaded module's `MODULE_API` pointer. **]**

**SRS_NATIVEMODULEHOST_17_031: [** `NativeModuleHost_Receive` shall call the loaded module's \_Receive function, passing the messageHandle along. **]**


NativeModuleHost\_Start
--------------
```c 
void NativeModuleHost_Start(MODULE_HANDLE moduleHandle)
```

**SRS_NATIVEMODULEHOST_17_032: [** `NativeModuleHost_Start` shall do nothing if `moduleHandle` is `NULL`. **]**

**SRS_NATIVEMODULEHOST_17_033: [** `NativeModuleHost_Start` shall get the loaded module's `MODULE_API` pointer. **]**

**SRS_NATIVEMODULEHOST_17_034: [** `NativeModuleHost_Start` shall call the loaded module's \_Start function, if defined. **]**
