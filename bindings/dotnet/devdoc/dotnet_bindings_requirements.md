.NET bindings for Azure IoT Gateway Modules
===========================================

Overview
--------


This document specifies the requirements for the gateway module that enables
interoperability between the gateway itself and modules written in .NET
running on hosted .NET CLR. The [high level design](./dotnet_bindings_hld.md) contains
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
    MESSAGE_BUS_HANDLE          bus;
    ICLRMetaHost                *pMetaHost;
    ICLRRuntimeInfo             *pRuntimeInfo;
    ICorRuntimeHost             *pCorRuntimeHost;
}DOTNET_HOST_HANDLE_DATA;
```

DotNET_Create
-------------
```c
MODULE_HANDLE DotNET_Create(MESSAGE_BUS_HANDLE bus, const void* configuration);
```
Creates a new .NET module instance. The parameter `configuration` is a
pointer to a `DOTNET_HOST_CONFIG` object.


**SRS_DOTNET_04_001: [** `DotNET_Create` shall return `NULL` if `bus` is `NULL`. **]**

**SRS_DOTNET_04_002: [** `DotNET_Create` shall return `NULL` if `configuration` is `NULL`. **]**

**SRS_DOTNET_04_003: [** `DotNET_Create` shall return `NULL` if `configuration->dotnet_module_path` is `NULL`. **]**

**SRS_DOTNET_04_004: [** `DotNET_Create` shall return `NULL` if `configuration->dotnet_module_entry_class` is `NULL`. **]**

**SRS_DOTNET_04_005: [** `DotNET_Create` shall return `NULL` if `configuration->dotnet_module_args` is `NULL`. **]**

**SRS_DOTNET_04_006: [** `DotNET_Create` shall return `NULL` if an underlying API call fails. **]**

**SRS_DOTNET_04_007: [** `DotNET_Create` shall return a non-`NULL` `MODULE_HANDLE` when successful. **]**

**SRS_DOTNET_04_008: [** `DotNET_Create` shall allocate memory for an instance of the `DOTNET_HOST_HANDLE_DATA` structure and use that as the backing structure for the module handle. **]**

**SRS_DOTNET_04_012: [** `DotNET_Create` shall get the 3 CLR Host Interfaces (CLRMetaHost, CLRRuntimeInfo and CorRuntimeHost) and save it on `DOTNET_HOST_HANDLE_DATA`. **]**

**SRS_DOTNET_04_009: [** `DotNET_Create` shall create an instance of .NET client Module and save it on `DOTNET_HOST_HANDLE_DATA`. **]**

**SRS_DOTNET_04_010: [** `DotNET_Create` shall save Client module Type and Azure IoT Gateway Assembly on `DOTNET_HOST_HANDLE_DATA`. **]**

**SRS_DOTNET_04_013: [** A .NET Object conforming to the `MessageBus` interface defined shall be created: **]**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ C#
    
    namespace Microsoft.Azure.IoT.Gateway
    {
        /// <summary> Object that represents the bus, to where a messsage is going to be published </summary>
        public class MessageBus
        {
            /// <summary>
            ///     Publish a message into the gateway message bus. 
            /// </summary>
            /// <param name="message">Object representing the message to be published into the bus.</param>
            /// <returns></returns>
            public void Publish(Message message);
        }        
    }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**SRS_DOTNET_04_014: [** `DotNET_Create` shall call `Create` C# method, implemented from IGatewayModule, passing the `MessageBus` object created and `configuration->dotnet_module_args`. **]**


DotNET_Receive
---------------
```c
void DotNET_Receive(MODULE_HANDLE module, MESSAGE_HANDLE message);
```
**SRS_DOTNET_04_015: [** `DotNET_Receive` shall do nothing if `module` is `NULL`. **]**

**SRS_DOTNET_04_016: [** `DotNET_Receive` shall do nothing if `message` is `NULL`. **]**

**SRS_DOTNET_04_017: [** `DotNET_Receive` shall construct an instance of the `Message` interface as defined below: **]**

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ C#
    
    namespace Microsoft.Azure.IoT.Gateway
    {
        /// <summary> Object that represents a message on the message bus. </summary>
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

**SRS_DOTNET_04_018: [** `DotNET_Create` shall call `Receive` C# method passing the `Message` object created with the content of `message` serialized into `Message` object. **]**


DotNET_Destroy
--------------
```c
void DotNET_Destroy(MODULE_HANDLE module);
```
**SRS_DOTNET_04_019: [** `DotNET_Destroy` shall do nothing if `module` is `NULL`. **]**

**SRS_DOTNET_04_022: [** `DotNET_Destroy` shall call the `Destroy` C# method. **]**

**SRS_DOTNET_04_020: [** `DotNET_Destroy` shall free all resources associated with the given module.. **]**

Module_GetAPIs
--------------
```c
extern const MODULE_APIS* Module_GetAPIS(void);
```
**SRS_DOTNET_04_021: [** `Module_GetAPIS` shall return a non-NULL pointer to a structure of type MODULE_APIS that has all fields initialized to non-NULL values. **]**


Module_DotNetHost_PublishMessage
--------------------------------
```c
bool Module_DotNetHost_PublishMessage(MESSAGE_BUS_HANDLE bus, MODULE_HANDLE sourceModule, const unsigned char* source, int32_t size)
```

**SRS_DOTNET_04_022: [** `Module_DotNetHost_PublishMessage` shall return false if `bus` is NULL. **]**

**SRS_DOTNET_04_029: [** `Module_DotNetHost_PublishMessage` shall return false if `sourceModule` is NULL.  **]**

**SRS_DOTNET_04_023: [** `Module_DotNetHost_PublishMessage` shall return false if `source` is NULL, or size if lower than 0. **]**

**SRS_DOTNET_04_024: [** `Module_DotNetHost_PublishMessage` shall create a message from `source` and size by invoking  `Message_CreateFromByteArray`. **]**

**SRS_DOTNET_04_025: [** If `Message_CreateFromByteArray` fails, `Module_DotNetHost_PublishMessage` shall fail. **]**

**SRS_DOTNET_04_026: [** `Module_DotNetHost_PublishMessage` shall call `MessageBus_Publish` passing bus, sourceModule, byte array and msgHandle. **]**

**SRS_DOTNET_04_027: [** If `MessageBus_Publish` fail `Module_DotNetHost_PublishMessage` shall fail.  **]**

**SRS_DOTNET_04_028: [** If `MessageBus_Publish` succeed `Module_DotNetHost_PublishMessage` shall succeed. **]**