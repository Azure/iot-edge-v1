.NET core binding for Azure IoT Gateway Modules
===============================================

Overview
--------

This document specifies the requirements for the gateway module that enables
interoperability between the gateway itself and modules written in .NET core
running on hosted .NET core CLR. The [high level design](./dotnet_core_binding_hld.md) contains
information on the general approach and might be a more useful read in order to
understand how the binding works.

Types
-----
```c
typedef struct DOTNET_CORE_CORE_HOST_CONFIG_TAG
{
    const char* assembly_name;
    const char* entry_type;
    const char* module_args;
}DOTNET_CORE_CORE_HOST_CONFIG;

struct DOTNET_CORE_CORE_HOST_HANDLE_DATA
{
    DOTNET_CORE_CORE_HOST_HANDLE_DATA();
    DOTNET_CORE_CORE_HOST_HANDLE_DATA(const char* input_assembly_name);
    DOTNET_CORE_CORE_HOST_HANDLE_DATA(BROKER_HANDLE broker, const char* input_assembly_name);
    DOTNET_CORE_CORE_HOST_HANDLE_DATA(DOTNET_CORE_CORE_HOST_HANDLE_DATA&& rhs);
    DOTNET_CORE_CORE_HOST_HANDLE_DATA(const DOTNET_CORE_CORE_HOST_HANDLE_DATA& rhs);
    DOTNET_CORE_CORE_HOST_HANDLE_DATA(const DOTNET_CORE_CORE_HOST_HANDLE_DATA& rhs, size_t module_id);
    size_t module_id;
    BROKER_HANDLE broker;
    STRING_HANDLE assembly_name;
};
```

DotNetCore_ParseConfigurationFromJson
-------------------------------------
```c
static void* DotNetCore_ParseConfigurationFromJson(const char* configuration);
```
This is the configuration of the Client Module. The String is going to be passed as is for .NET Client Module.

DotNetCore_Create
-----------------
```c
MODULE_HANDLE DotNetCore_Create(BROKER_HANDLE broker, const void* configuration);
```
Creates a new .NET core module instance. The parameter `configuration` is a
pointer to a `DOTNET_CORE_HOST_CONFIG` object.


**SRS_DOTNET_CORE_04_001: [** `DotNetCore_Create` shall return `NULL` if `broker` is `NULL`. **]**

**SRS_DOTNET_CORE_04_002: [** `DotNetCore_Create` shall return `NULL` if `configuration` is `NULL`. **]**

**SRS_DOTNET_CORE_04_003: [** `DotNetCore_Create` shall return `NULL` if `configuration->assembly_name` is `NULL`. **]**

**SRS_DOTNET_CORE_04_004: [** `DotNetCore_Create` shall return `NULL` if `configuration->entry_type` is `NULL`. **]**

**SRS_DOTNET_CORE_04_005: [** `DotNetCore_Create` shall return `NULL` if `configuration->module_args` is `NULL`. **]**

**SRS_DOTNET_CORE_04_035: [** `DotNetCore_Create` shall return `NULL` if `configuration->clrOptions` is `NULL`. **]**

**SRS_DOTNET_CORE_04_036: [** `DotNetCore_Create` shall return `NULL` if `configuration->clrOptions->coreclrpath` is `NULL`. **]**

**SRS_DOTNET_CORE_04_037: [** `DotNetCore_Create` shall return `NULL` if `configuration->clrOptions->trustedplatformassemblieslocation` is `NULL`. **]**

**SRS_DOTNET_CORE_04_006: [** `DotNetCore_Create` shall return `NULL` if an underlying API call fails. **]**

**SRS_DOTNET_CORE_04_007: [** `DotNetCore_Create` shall return a non-`NULL` `MODULE_HANDLE` when successful. **]**

**SRS_DOTNET_CORE_04_010: [** `DotNetCore_Create` shall load `coreclr` library, if not loaded yet. **]**

**SRS_DOTNET_CORE_04_011: [** `DotNetCore_Create` shall get address for 3 external methods `coreclr_initialize`, `coreclr_shutdown` and `coreclr_create_delegate` and save it to global reference for Dot Net Core binding. **]**

**SRS_DOTNET_CORE_04_013: [** `DotNetCore_Create` shall call `coreclr_create_delegate` to be able to call `Microsoft.Azure.Devices.Gateway.GatewayDelegatesGateway.Delegates_Create` **]**

**SRS_DOTNET_CORE_04_021: [** `DotNetCore_Create` shall call `coreclr_create_delegate` to be able to call `Microsoft.Azure.Devices.Gateway.GatewayDelegatesGateway.Delegates_Receive` **]**

**SRS_DOTNET_CORE_04_024: [** `DotNetCore_Destroy` shall call `coreclr_create_delegate` to be able to call `Microsoft.Azure.Devices.Gateway.GatewayDelegatesGateway.Delegates_Destroy` **]**

**SRS_DOTNET_CORE_04_014: [** `DotNetCore_Create` shall call `Microsoft.Azure.Devices.Gateway.GatewayDelegatesGateway.Delegates_Create` C# method, implemented on `Microsoft.Azure.Devices.Gateway.dll`. **]**


DotNetCore_Start
----------------
```c
void DotNetCore_Start(MODULE_HANDLE module);
```

**SRS_DOTNET_CORE_004_015: [** `DotNetCore_Start` shall do nothing if `module` is NULL. **]**

**SRS_DOTNET_CORE_004_016: [** `DotNetCore_Start` shall call `coreclr_create_delegate` to be able to call `Microsoft.Azure.Devices.Gateway.GatewayDelegatesGateway.Delegates_Start` **]**

**SRS_DOTNET_CORE_004_017: [** `DotNetCore_Start` shall call `Microsoft.Azure.Devices.Gateway.GatewayDelegatesGateway.Delegates_Start` C# method, implemented on `Microsoft.Azure.Devices.Gateway.dll`. **]**

DotNetCore_Receive
------------------
```c
void DotNetCore_Receive(MODULE_HANDLE module, MESSAGE_HANDLE message);
```
**SRS_DOTNET_CORE_04_018: [** `DotNetCore_Receive` shall do nothing if `module` is `NULL`. **]**

**SRS_DOTNET_CORE_04_019: [** `DotNetCore_Receive` shall do nothing if `message` is `NULL`. **]**

**SRS_DOTNET_CORE_04_020: [** `DotNetCore_Receive` shall call `Message_ToByteArray` to serialize `message`. **]**

**SRS_DOTNET_CORE_04_022: [** `DotNetCore_Receive` shall call `Microsoft.Azure.Devices.Gateway.GatewayDelegatesGateway.Delegates_Receive` C# method, implemented on `Microsoft.Azure.Devices.Gateway.dll`. **]**

DotNetCore_Destroy
------------------
```c
void DotNetCore_Destroy(MODULE_HANDLE module);
```
**SRS_DOTNET_CORE_04_023: [** `DotNetCore_Destroy` shall do nothing if `module` is `NULL`. **]**

**SRS_DOTNET_CORE_04_038: [** `DotNetCore_Destroy` shall release all resources allocated by `DotNetCore_Create`. **]**

**SRS_DOTNET_CORE_04_039: [** `DotNetCore_Destroy` shall verify that there is no module and shall shutdown the dotnet core clr. **]**

**SRS_DOTNET_CORE_04_025: [** `DotNetCore_Destroy` shall call `Microsoft.Azure.Devices.Gateway.GatewayDelegatesGateway.Delegates_Destroy` C# method, implemented on `Microsoft.Azure.Devices.Gateway.dll`. **]**

Module_GetApi
--------------
```c
MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version);
```

**SRS_DOTNET_CORE_04_026:: [** `Module_GetApi` shall return out the provided `MODULES_API` structure with required module's APIs functions. **]**

Module_DotNetCoreHost_PublishMessage
------------------------------------
```c
bool Module_DotNetCoreHost_PublishMessage(BROKER_HANDLE , MODULE_HANDLE sourceModule, const unsigned char* message, int32_t size)
```

**SRS_DOTNET_CORE_04_027: [** `Module_DotNetCoreHost_PublishMessage` shall return false if `broker` is NULL. **]**

**SRS_DOTNET_CORE_04_028: [** `Module_DotNetCoreHost_PublishMessage` shall return false if `sourceModule` is NULL.  **]**

**SRS_DOTNET_CORE_04_029: [** `Module_DotNetCoreHost_PublishMessage` shall return false if `message` is NULL, or size if lower than 0. **]**

**SRS_DOTNET_CORE_04_030: [** `Module_DotNetCoreHost_PublishMessage` shall create a message from `message` and size by invoking  `Message_CreateFromByteArray`. **]**

**SRS_DOTNET_CORE_04_031: [** If `Message_CreateFromByteArray` fails, `Module_DotNetCoreHost_PublishMessage` shall fail. **]**

**SRS_DOTNET_CORE_04_032: [** `Module_DotNetCoreHost_PublishMessage` shall call `Broker_Publish` passing broker, sourceModule, message and size. **]**

**SRS_DOTNET_CORE_04_033: [** If `Broker_Publish` fails `Module_DotNetCoreHost_PublishMessage` shall fail.  **]**

**SRS_DOTNET_CORE_04_034: [** If `Broker_Publish` succeeds `Module_DotNetCoreHost_PublishMessage` shall succeed. **]**


Module_DotNetCoreHost_SetBindingDelegates
-----------------------------------------
```c
void Module_DotNetCoreHost_SetBindingDelegates(intptr_t createAddress, intptr_t receiveAddress, intptr_t destroyAddress, intptr_t startAddress)
```
**SRS_DOTNET_CORE_04_040: [** `Module_DotNetCoreHost_SetBindingDelegates` shall just assign `createAddress` to `GatewayCreateDelegate` **]**

**SRS_DOTNET_CORE_04_041: [** `Module_DotNetCoreHost_SetBindingDelegates` shall just assign `receiveAddress` to `GatewayReceiveDelegate` **]**

**SRS_DOTNET_CORE_04_042: [** `Module_DotNetCoreHost_SetBindingDelegates` shall just assign `destroyAddress` to `GatewayDestroyDelegate` **]**

**SRS_DOTNET_CORE_04_043: [** `Module_DotNetCoreHost_SetBindingDelegates` shall just assign `startAddress` to `GatewayStartDelegate` **]**




DotNetCore_FreeConfiguration
----------------------------
```c
void DotNetCore_FreeConfiguration(void* configuration)
```
There is no need to free configuration, since we don't allocate anything.