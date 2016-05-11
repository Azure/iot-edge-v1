Node JS bindings high level API for Azure IoT Gateway Modules
=============================================================

References
----------

  - [High level design document](./nodejs_bindings_hld.md)
  - [Low level API requirements](./nodejs_bindings_requirements.md)

Overview
--------

This module is a *passthrough* implementation that simply wraps the Node JS
module code by providing an easy to use JSON based configuration interface. It
de-serializes the JSON into a `NODEJS_MODULE_CONFIG` instance and passes control
to the underlying implementation.

NODEJS_HL_Create
----------------
```c
MODULE_HANDLE NODEJS_HL_Create(MESSAGE_BUS_HANDLE bus, const void* configuration);
```

Creates a new Node JS module HL instance. `configuration` is a pointer to a
`const char*` that contains a JSON string as supplied by `Gateway_Create_From_JSON`.
The JSON should conform to the following structure:

```json
{
    "modules": [
        {
            "module name": "node_bot",
            "module path": "/path/to/json_module.so|.dll",
            "args": {
                "main_path": "/path/to/main.js",
                "args": "module configuration"
            }
        }
    ]
}
```

**SRS_NODEJS_HL_13_001: [** `NODEJS_HL_Create` shall return `NULL` if `bus` is `NULL`. **]**

**SRS_NODEJS_HL_13_002: [** `NODEJS_HL_Create` shall return `NULL` if `configuration` is `NULL`. **]**

**SRS_NODEJS_HL_13_012: [** `NODEJS_HL_Create` shall parse `configuration` as a JSON string. **]**

**SRS_NODEJS_HL_13_003: [** `NODEJS_HL_Create` shall return `NULL` if `configuration` is not a valid JSON string. **]**

**SRS_NODEJS_HL_13_014: [** `NODEJS_HL_Create` shall return `NULL` if the `configuration` JSON does not start with an object at the root. **]**

**SRS_NODEJS_HL_13_013: [** `NODEJS_HL_Create` shall extract the value of the `main_path` property from the configuration JSON. **]**

**SRS_NODEJS_HL_13_004: [** `NODEJS_HL_Create` shall return `NULL` if the `configuration` JSON does not contain a string property called `main_path`. **]**

**SRS_NODEJS_HL_13_006: [** `NODEJS_HL_Create` shall extract the value of the `args` property from the configuration JSON. **]**

**SRS_NODEJS_HL_13_005: [** `NODEJS_HL_Create` shall populate a `NODEJS_MODULE_CONFIG` object with the values of the `main_path` and `args` properties and invoke `NODEJS_Create` passing the `bus` handle the config object. **]**

**SRS_NODEJS_HL_13_007: [** If `NODEJS_Create` succeeds then a valid `MODULE_HANDLE` shall be returned. **]**

**SRS_NODEJS_HL_13_008: [** If `NODEJS_Create` fail then the value `NULL` shall be returned. **]**

NODEJS_HL_Receive
-----------------
```c
void NODEJS_HL_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
```

**SRS_NODEJS_HL_13_009: [** `NODEJS_HL_Receive` shall pass the received parameters to the underlying module's `_Receive` function. **]**

NODEJS_HL_Destroy
-----------------
```c
void NODEJS_HL_Destroy(MODULE_HANDLE module)
```

**SRS_NODEJS_HL_13_010: [** `NODEJS_HL_Destroy` shall destroy all used resources. **]**

###Module_GetAPIs
```c
extern const MODULE_APIS* Module_GetAPIS(void);
```
**SRS_NODEJS_HL_13_011: [** `Module_GetAPIS` shall return a non-NULL pointer to a structure of type `MODULE_APIS` that has all fields non-`NULL`. **]**
