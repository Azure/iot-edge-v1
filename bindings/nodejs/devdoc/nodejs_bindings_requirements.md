Node JS bindings for Azure IoT Gateway Modules
==============================================

Overview
--------

This document specifies the requirements for the gateway module that enables
interoperability between the gateway itself and modules written in JavaScript
running on Node JS. The [high level design](./node_bindings_hld.md) contains
information on the general approach and might be a more useful read in order to
understand how the binding works.

Types
-----
```c
typedef struct NODEJS_MODULE_CONFIG_TAG
{
    const char* main_path;
    const char* configuration_json;
}NODEJS_MODULE_CONFIG;

typedef struct NODEJS_MODULE_HANDLE_DATA_TAG
{
    MESSAGE_BUS_HANDLE          bus;
    STRING_HANDLE               main_path;
    STRING_HANDLE               configuration_json;
    THREAD_HANDLE               nodejs_thread;
    v8::Persistent<v8::Object>  module_object;
}NODEJS_MODULE_HANDLE_DATA;
```

NodeJS_Create
-------------
```c
MODULE_HANDLE NodeJS_Create(MESSAGE_BUS_HANDLE bus, const void* configuration);
```

Creates a new Node JS module instance. The parameter `configuration` is a
pointer to a `NODEJS_MODULE_CONFIG` object.

**SRS_NODEJS_13_001: [** `NodeJS_Create` shall return `NULL` if `bus` is `NULL`. **]**

**SRS_NODEJS_13_002: [** `NodeJS_Create` shall return `NULL` if `configuration` is `NULL`. **]**

**SRS_NODEJS_13_019: [** `NodeJS_Create` shall return `NULL` if `configuration->configuration_json` is not valid JSON. **]**

**SRS_NODEJS_13_003: [** `NodeJS_Create` shall return `NULL` if `configuration->main_path` is `NULL`. **]**

**SRS_NODEJS_13_013: [** `NodeJS_Create` shall return `NULL` if `configuration->main_path` is an invalid file system path. **]**

**SRS_NODEJS_13_004: [** `NodeJS_Create` shall return `NULL` if an underlying API call fails. **]**

**SRS_NODEJS_13_005: [** `NodeJS_Create` shall return a non-`NULL` `MODULE_HANDLE` when successful. **]**

**SRS_NODEJS_13_006: [** `NodeJS_Create` shall allocate memory for an instance of the `NODEJS_MODULE_HANDLE_DATA` structure and use that as the backing structure for the module handle. **]**

**SRS_NODEJS_13_007: [** `NodeJS_Create` shall construct and initialize the `STRING_HANDLE` member `NODEJS_MODULE_HANDLE_DATA::main_path` from `configuration->main_path`. **]**

**SRS_NODEJS_13_008: [** `NodeJS_Create` shall construct and initialize the `STRING_HANDLE` member `NODEJS_MODULE_HANDLE_DATA::configuration_json` from `configuration->configuration_json`. **]**

**SRS_NODEJS_13_009: [** `NodeJS_Create` shall create a new thread to run the Node JS event loop. **]**

**SRS_NODEJS_13_010: [** The Node JS event thread shall setup a idle handler on the default libuv loop. **]**

**SRS_NODEJS_13_011: [** When the libuv handler is invoked, a new JavaScript object shall be created and added to the v8 global context with the name `gatewayHost`. The object shall implement the following interface (TypeScript syntax used below is just for the purpose of description):
```ts
interface GatewayModuleHost {
    registerModule: (module: GatewayModule) => void;
}
```
**]**

**SRS_NODEJS_13_012: [** The following JavaScript is then executed supplying the contents of `NODEJS_MODULE_HANDLE_DATA::main_path` for the placeholder variable `js_main_path`:

```js
gatewayHost.registerModule(require(js_main_path));
```
**]**

**SRS_NODEJS_13_014: [** When the native implementation of `GatewayModuleHost.registerModule` is invoked it shall do nothing if no parameter is passed to it. **]**

**SRS_NODEJS_13_015: [** When the native implementation of `GatewayModuleHost.registerModule` is invoked it shall do nothing if the parameter passed to it is not a JavaScript object. **]**

**SRS_NODEJS_13_016: [** When the native implementation of `GatewayModuleHost.registerModule` is invoked it shall do nothing if the parameter is an object but does not conform to the following interface:
```ts
interface GatewayModule {
    create: (messageBus: MessageBus, configuration: any) => boolean;
    receive: (message: Message) => void;
    destroy: () => void;
}
```
**]**

**SRS_NODEJS_13_017: [** A JavaScript object conforming to the `MessageBus` interface defined shall be created:
```ts
interface StringMap {
    [key: string]: string;
}

interface Message {
    properties: StringMap;
    content: Uint8Array;
}

interface MessageBus {
    publish: (message: Message) => boolean;
}
```
 **]**
 
 **SRS_NODEJS_13_018: [** The `GatewayModule.create` method shall be invoked passing the `MessageBus` instance and a parsed instance of the configuration JSON string. **]**

NodeJS_Receive
---------------
```c
void NodeJS_Receive(MODULE_HANDLE module, MESSAGE_HANDLE message);
```

**SRS_NODEJS_13_020: [** `NodeJS_Receive` shall do nothing if `module` is `NULL`. **]**

**SRS_NODEJS_13_021: [** `NodeJS_Receive` shall do nothing if `message` is `NULL`. **]**

**SRS_NODEJS_13_022: [** `NodeJS_Receive` shall construct an instance of the `Message` interface as defined below:
```ts
interface StringMap {
    [key: string]: string;
}

interface Message {
    properties: StringMap;
    content: Uint8Array;
}
```
**]**

**SRS_NODEJS_13_023: [** `NodeJS_Receive` shall invoke `GatewayModule.receive` passing the newly constructed `Message` instance. **]**

MessageBus.publish
------------------
```c
void message_bus_publish(const v8::FunctionCallbackInfo<Value>& info);
```

**SRS_NODEJS_13_027: [** `message_bus_publish` shall set the return value to `false` if the arguments count is less than 1. **]**

**SRS_NODEJS_13_028: [** `message_bus_publish` shall set the return value to `false` if the first argument is not a JS object. **]**

**SRS_NODEJS_13_029: [** `message_bus_publish` shall set the return value to `false` if the first argument is an object whose shape does not conform to the following interface:
```ts
interface StringMap {
    [key: string]: string;
}

interface Message {
    properties: StringMap;
    content: Uint8Array;
}
```
**]**

**SRS_NODEJS_13_031: [** `message_bus_publish` shall set the return value to `false` if any underlying platform call fails. **]**

**SRS_NODEJS_13_030: [** `message_bus_publish` shall construct and initialize a `MESSAGE_HANDLE` from the first argument. **]**

**SRS_NODEJS_13_032: [** `message_bus_publish` shall call `MessageBus_Publish` passing the newly constructed `MESSAGE_HANDLE`. **]**

**SRS_NODEJS_13_033: [** `message_bus_publish` shall set the return value to `true` or `false` depending on the status of the `MessageBus_Publish` call. **]**

**SRS_NODEJS_13_034: [** `message_bus_publish` shall destroy the `MESSAGE_HANDLE`. **]**

NodeJS_Destroy
--------------
```c
void NodeJS_Destroy(MODULE_HANDLE module);
```

**SRS_NODEJS_13_024: [** `NodeJS_Destroy` shall do nothing if `module` is `NULL`. **]**

**SRS_NODEJS_13_025: [** `NodeJS_Destroy` shall free all resources. **]**

Module_GetAPIs
--------------
```c
extern const MODULE_APIS* Module_GetAPIS(void);
```

**SRS_NODEJS_13_026: [** `Module_GetAPIS` shall return a non-NULL pointer to a structure of type MODULE_APIS that has all fields initialized to non-NULL values. **]**