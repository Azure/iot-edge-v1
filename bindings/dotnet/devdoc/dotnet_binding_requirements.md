.NET binding for Azure IoT Gateway Modules
===========================================

Overview
--------


This document specifies the requirements for the gateway module that enables
interoperability between the gateway itself and modules written in .NET
running on hosted .NET CLR. The [high level design](./dotnet_binding_hld.md) contains
information on the general approach and might be a more useful read in order to
understand how the binding works.

Types
-----
```c
typedef struct DOTNET_HOST_CONFIG_TAG
{
    const char* dotnet_module_path;
    const char* dotnet_module_entry_class;
    const char* dotnet_module_args;
}DOTNET_HOST_CONFIG;

typedef struct DOTNET_HOST_HANDLE_DATA_TAG
{
    BROKER_HANDLE               broker;
    ICLRMetaHost                *pMetaHost;
    ICLRRuntimeInfo             *pRuntimeInfo;
    ICorRuntimeHost             *pCorRuntimeHost;
}DOTNET_HOST_HANDLE_DATA;
```


DotNet_CreateFromJson
---------------------
```c
MODULE_HANDLE DotNet_CreateFromJson(BROKER_HANDLE broker, const char* configuration);
```
Creates a new .NET module host instance. `configuration` is a pointer to a const char* that contains a json object as supplied by `Gateway_CreateFromJson`.
The json object should conform to the following structure: 
```json
{
    "dotnet_module_path": "/path/to/dotnet_module.dll",
    "dotnet_module_entry_class": "mydotnetmodule.classname",
    "dotnet_module_args": "module configuration"
}
``` 
Example:
The following Gateway config file describes a module called "csharp_hello_world" which uses the .NET binding. It instructs the binding to load a .NET module from `/path/to/csharp_module.dll`
and call the entry class `mycsharpmodule.classname` passing as configuration the serialized string `"module configuration"`.
```json
{
    "modules": [
        {
            "module name": "csharp_hello_world",
            "module path": "/path/to/dotnet.dll",
            "args": {
                "dotnet_module_path": "/path/to/csharp_module.dll",
                "dotnet_module_entry_class": "mycsharpmodule.classname",
                "dotnet_module_args": "module configuration"
            }
        }
    ]
}```


**SRS_DOTNET_05_001: [** If `broker` is NULL then `DotNet_CreateFromJson` shall fail and return NULL. **]**

**SRS_DOTNET_05_002: [** If `configuration` is NULL then `DotNet_CreateFromJson` shall fail and return NULL. **]**

**SRS_DOTNET_05_003: [** If configuration is not a JSON object, then `DotNet_CreateFromJson` shall fail and return NULL. **]**

**SRS_DOTNET_05_004: [** If the JSON object does not contain a value named "dotnet_module_path" then `DotNet_CreateFromJson` shall fail and return NULL. **]**

**SRS_DOTNET_05_005: [** If the JSON object does not contain a value named "dotnet_module_entry_class" then `DotNet_CreateFromJson` shall fail and return NULL. **]**

**SRS_DOTNET_05_006: [** If the JSON object does not contain a value named "dotnet_module_args" then `DotNet_CreateFromJson` shall fail and return NULL. **]**

**SRS_DOTNET_05_007: [** `DotNet_CreateFromJson` shall pass `broker` and `const char* configuration` ( with `DOTNET_HOST_CONFIG`) to `DotNet_Create`. **]**

**SRS_DOTNET_05_008: [** If `DotNet_Create` succeeds then `DotNet_CreateFromJson` shall succeed and return a non-NULL value. **]**

**SRS_DOTNET_05_009: [** If `DotNet_Create` fails then `DotNet_CreateFromJson` shall fail and return NULL. **]**


DotNet_Create
-------------
```c
MODULE_HANDLE DotNet_Create(BROKER_HANDLE broker, const void* configuration);
```
Creates a new .NET module instance. The parameter `configuration` is a
pointer to a `DOTNET_HOST_CONFIG` object.


**SRS_DOTNET_04_001: [** `DotNet_Create` shall return `NULL` if `broker` is `NULL`. **]**

**SRS_DOTNET_04_002: [** `DotNet_Create` shall return `NULL` if `configuration` is `NULL`. **]**

**SRS_DOTNET_04_003: [** `DotNet_Create` shall return `NULL` if `configuration->dotnet_module_path` is `NULL`. **]**

**SRS_DOTNET_04_004: [** `DotNet_Create` shall return `NULL` if `configuration->dotnet_module_entry_class` is `NULL`. **]**

**SRS_DOTNET_04_005: [** `DotNet_Create` shall return `NULL` if `configuration->dotnet_module_args` is `NULL`. **]**

**SRS_DOTNET_04_006: [** `DotNet_Create` shall return `NULL` if an underlying API call fails. **]**

**SRS_DOTNET_04_007: [** `DotNet_Create` shall return a non-`NULL` `MODULE_HANDLE` when successful. **]**

**SRS_DOTNET_04_008: [** `DotNet_Create` shall allocate memory for an instance of the `DOTNET_HOST_HANDLE_DATA` structure and use that as the backing structure for the module handle. **]**

**SRS_DOTNET_04_012: [** `DotNet_Create` shall get the 3 CLR Host Interfaces (CLRMetaHost, CLRRuntimeInfo and CorRuntimeHost) and save it on `DOTNET_HOST_HANDLE_DATA`. **]**

**SRS_DOTNET_04_009: [** `DotNet_Create` shall create an instance of .NET client Module and save it on `DOTNET_HOST_HANDLE_DATA`. **]**

**SRS_DOTNET_04_010: [** `DotNet_Create` shall save Client module Type and Azure IoT Gateway Assembly on `DOTNET_HOST_HANDLE_DATA`. **]**

**SRS_DOTNET_04_013: [** A .NET Object conforming to the `Broker` interface defined shall be created: **]**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ C#
    
    namespace Microsoft.Azure.IoT.Gateway
    {
        /// <summary> Object that represents the message broker, to which messsages will be published. </summary>
        public class Broker
        {
            /// <summary>
            ///     Publish a message to the message broker. 
            /// </summary>
            /// <param name="message">Object representing the message to be published to the broker.</param>
            /// <returns></returns>
            public void Publish(Message message);
        }        
    }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**SRS_DOTNET_04_014: [** `DotNet_Create` shall call `Create` C# method, implemented from IGatewayModule, passing the `Broker` object created and `configuration->dotnet_module_args`. **]**

DotNet_Start
------------
```c
void DotNet_Start(MODULE_HANDLE module);
```

**SRS_DOTNET_17_001: [** `DotNet_Start` shall do nothing if `module` is NULL. **]**

**SRS_DOTNET_17_002: [** `DotNet_Start` shall attempt to get the "IGatewayModuleStart" type interface. **]**

**SRS_DOTNET_17_003: [** If the "IGatewayModuleStart" type interface exists, `DotNet_Start` shall call the `Start` C# method. **]**

DotNet_Receive
---------------
```c
void DotNet_Receive(MODULE_HANDLE module, MESSAGE_HANDLE message);
```
**SRS_DOTNET_04_015: [** `DotNet_Receive` shall do nothing if `module` is `NULL`. **]**

**SRS_DOTNET_04_016: [** `DotNet_Receive` shall do nothing if `message` is `NULL`. **]**

**SRS_DOTNET_04_017: [** `DotNet_Receive` shall construct an instance of the `Message` interface as defined below: **]**

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ C#
    
    namespace Microsoft.Azure.IoT.Gateway
    {
        /// <summary> Object that represents a message passed between modules. </summary>
        public class Message
        {
            public Buffer Content { set; get; };
            
            public Dictionary<string,string> Properties  { set; get; };

            public Message();
            
            public Message(Buffer content, Dictionary<string, string> properties); 
            
            public Message(Message message);
        }        
    }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**SRS_DOTNET_04_018: [** `DotNet_Create` shall call `Receive` C# method passing the `Message` object created with the content of `message` serialized into `Message` object. **]**


DotNet_Destroy
--------------
```c
void DotNet_Destroy(MODULE_HANDLE module);
```
**SRS_DOTNET_04_019: [** `DotNet_Destroy` shall do nothing if `module` is `NULL`. **]**

**SRS_DOTNET_04_022: [** `DotNet_Destroy` shall call the `Destroy` C# method. **]**

**SRS_DOTNET_04_020: [** `DotNet_Destroy` shall free all resources associated with the given module.. **]**

Module_GetAPIs
--------------
```c
extern const MODULE_APIS* Module_GetAPIS(void);
```

**SRS_DOTNET_26_001: [** `Module_GetAPIS` shall fill out the provided `MODULES_API` structure with required module's APIs functions. **]**

Module_DotNetHost_PublishMessage
--------------------------------
```c
bool Module_DotNetHost_PublishMessage(BROKER_HANDLE , MODULE_HANDLE sourceModule, const unsigned char* message, int32_t size)
```

**SRS_DOTNET_04_022: [** `Module_DotNetHost_PublishMessage` shall return false if `broker` is NULL. **]**

**SRS_DOTNET_04_029: [** `Module_DotNetHost_PublishMessage` shall return false if `sourceModule` is NULL.  **]**

**SRS_DOTNET_04_023: [** `Module_DotNetHost_PublishMessage` shall return false if `message` is NULL, or size if lower than 0. **]**

**SRS_DOTNET_04_024: [** `Module_DotNetHost_PublishMessage` shall create a message from `message` and size by invoking  `Message_CreateFromByteArray`. **]**

**SRS_DOTNET_04_025: [** If `Message_CreateFromByteArray` fails, `Module_DotNetHost_PublishMessage` shall fail. **]**

**SRS_DOTNET_04_026: [** `Module_DotNetHost_PublishMessage` shall call `Broker_Publish` passing broker, sourceModule, message and size. **]**

**SRS_DOTNET_04_027: [** If `Broker_Publish` fails `Module_DotNetHost_PublishMessage` shall fail.  **]**

**SRS_DOTNET_04_028: [** If `Broker_Publish` succeeds `Module_DotNetHost_PublishMessage` shall succeed. **]**
