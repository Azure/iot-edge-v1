.NET Host Module High Level 
===========================================



### References
[dotNET Host Module](./dotnet_bindings_requirements.md)

[json](http://www.json.org)


Overview
--------

This module is just a passthrough to .NET Host Module in all aspects except for creation that is done by means of a json value.

DotNET_HL_Create
-------------
```c
MODULE_HANDLE DotNET_HL_Create(MESSAGE_BUS_HANDLE bus, const void* configuration);
```
Creates a new dotNET HOST Module HL instance. `configuration` is a pointer to a const char* that contains a json object as supplied by `Gateway_Create_From_JSON`.
By convention in the json object should contain 
```json
{"dotnet_module_path": "/path/to/dotnet_module.dll",
 "dotnet_module_entry_class": "mydotnetmodule.classname",
 "dotnet_module_args": "module configuration"
}
``` 
Example:
The following Gateway config file will contain a module called "csharp_hello_world" build from `/path/to/dotnet_hl.dll` and instructs the dotNET Host Module to load a .NET Module from `/path/to/csharp_module.dll`
and call the entry class called `mycsharpmodule.classname` and passing as configuration the serialized string `"module configuration"`.
```json
{
    "modules": [
        {
            "module name": "csharp_hello_world",
            "module path": "/path/to/dotnet_hl.dll",
            "args": {
                "dotnet_module_path": "/path/to/csharp_module.dll",
                "dotnet_module_entry_class": "mycsharpmodule.classname",
                "dotnet_module_args": "module configuration"
            }
        }
    ]
}```


**SRS_DOTNET_HL_04_001: [** If `busHandle` is NULL then `DotNET_HL_Create` shall fail and return NULL. **]**

**SRS_DOTNET_HL_04_002: [** If `configuration` is NULL then `DotNET_HL_Create` shall fail and return NULL. **]**

**SRS_DOTNET_HL_04_003: [** If configuration is not a JSON object, then `DotNET_HL_Create` shall fail and return NULL. **]**

**SRS_DOTNET_HL_04_004: [** If the JSON object does not contain a value named "dotnet_module_path" then `DotNET_HL_Create` shall fail and return NULL. **]**

**SRS_DOTNET_HL_04_005: [** If the JSON object does not contain a value named "dotnet_module_entry_class" then `DotNET_HL_Create` shall fail and return NULL. **]**

**SRS_DOTNET_HL_04_006: [** If the JSON object does not contain a value named "dotnet_module_args" then `DotNET_HL_Create` shall fail and return NULL. **]**

**SRS_DOTNET_HL_04_007: [** `DotNET_HL_Create` shall pass `busHandle` and `const void* configuration` ( with `DOTNET_HOST_CONFIG`) to `DotNET_Create`. **]**

**SRS_DOTNET_HL_04_008: [** If `DotNET_Create` succeeds then `DotNET_HL_Create` shall succeed and return a non-NULL value. **]**

**SRS_DOTNET_HL_04_009: [** If `DotNET_Create` fails then `DotNET_HL_Create` shall fail and return NULL. **]**


DotNET_HL_Receive
---------------
```c
void DotNET_HL_Receive(MODULE_HANDLE module, MESSAGE_HANDLE message);
```
**SRS_DOTNET_HL_04_010: [** `DotNET_HL_Receive` shall pass the received parameters to the underlying DotNET Host Module's `_Receive` function. **]**

DotNET_HL_Destroy
--------------
```c
void DotNET_HL_Destroy(MODULE_HANDLE module);
```

**SRS_DOTNET_HL_04_011: [** `DotNET_HL_Destroy` shall destroy all used resources for the associated module. **]**

**SRS_DOTNET_HL_04_013: [** `DotNET_HL_Destroy` shall do nothing if module is NULL. **]**

Module_GetAPIs
--------------
```c
extern const MODULE_APIS* Module_GetAPIS(void);
```
**SRS_DOTNET_HL_04_012: [** `Module_GetAPIS` shall return a non-NULL pointer to a structure of type MODULE_APIS that has all fields non-NULL. **]**