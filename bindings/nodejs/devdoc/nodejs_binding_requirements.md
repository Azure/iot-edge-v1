Node.js bindings for Azure IoT Gateway Modules
==============================================

Overview
--------

This document specifies the requirements for the gateway module that enables
interoperability between the gateway itself and modules written in JavaScript
running on Node.js. The [high level design](./node_bindings_hld.md) contains
information on the general approach and might be a more useful read in order to
understand how the binding works.

Types
-----
```c++
struct NODEJS_MODULE_HANDLE_DATA;

typedef void(*PFNMODULE_START)(NODEJS_MODULE_HANDLE_DATA* handle_data);

struct NODEJS_MODULE_HANDLE_DATA
{
    BROKER_HANDLE               broker;
    std::string                 main_path;
    std::string                 configuration_json;
    v8::Isolate                 *v8_isolate;
    v8::Persistent<v8::Object>  module_object;
    size_t                      module_id;
    PFNMODULE_START             on_module_start;
};
```

NODEJS_CreateFromJson
---------------------
```c
MODULE_HANDLE NODEJS_CreateFromJson(BROKER_HANDLE broker, const char* configuration);
```

Creates a new Node.js module host instance. `configuration` is a pointer to a
`const char*` that contains a JSON string as supplied by `Gateway_CreateFromJson`.
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

**SRS_NODEJS_05_001: [** `NODEJS_CreateFromJson` shall return `NULL` if `broker` is `NULL`. **]**

**SRS_NODEJS_05_002: [** `NODEJS_CreateFromJson` shall return `NULL` if `configuration` is `NULL`. **]**

**SRS_NODEJS_05_012: [** `NODEJS_CreateFromJson` shall parse `configuration` as a JSON string. **]**

**SRS_NODEJS_05_003: [** `NODEJS_CreateFromJson` shall return `NULL` if `configuration` is not a valid JSON string. **]**

**SRS_NODEJS_05_014: [** `NODEJS_CreateFromJson` shall return `NULL` if the `configuration` JSON does not start with an object at the root. **]**

**SRS_NODEJS_05_013: [** `NODEJS_CreateFromJson` shall extract the value of the `main_path` property from the configuration JSON. **]**

**SRS_NODEJS_05_004: [** `NODEJS_CreateFromJson` shall return `NULL` if the `configuration` JSON does not contain a string property called `main_path`. **]**

**SRS_NODEJS_05_006: [** `NODEJS_CreateFromJson` shall extract the value of the `args` property from the configuration JSON. **]**

**SRS_NODEJS_05_005: [** `NODEJS_CreateFromJson` shall populate a `NODEJS_MODULE_CONFIG` object with the values of the `main_path` and `args` properties and invoke `NODEJS_Create` passing the `broker` handle and the config object. **]**

**SRS_NODEJS_05_007: [** If `NODEJS_Create` succeeds then a valid `MODULE_HANDLE` shall be returned. **]**

**SRS_NODEJS_05_008: [** If `NODEJS_Create` fail then the value `NULL` shall be returned. **]**


NODEJS_Create
-------------
```c
MODULE_HANDLE NodeJS_Create(BROKER_HANDLE broker, const void* configuration);
```

Creates a new Node.js module instance. The parameter `configuration` is a
pointer to a `NODEJS_MODULE_CONFIG` object.

**SRS_NODEJS_13_001: [** `NodeJS_Create` shall return `NULL` if `broker` is `NULL`. **]**

**SRS_NODEJS_13_002: [** `NodeJS_Create` shall return `NULL` if `configuration` is `NULL`. **]**

**SRS_NODEJS_13_019: [** `NodeJS_Create` shall return `NULL` if `configuration->configuration_json` is not valid JSON. **]**

**SRS_NODEJS_05_001: [** `NodeJS_Create` shall interpret a `NULL` value for `configuration->configuration_json` as the JSON string `"\"args\": null"`. **]**

**SRS_NODEJS_13_003: [** `NodeJS_Create` shall return `NULL` if `configuration->main_path` is `NULL`. **]**

**SRS_NODEJS_13_013: [** `NodeJS_Create` shall return `NULL` if `configuration->main_path` is an invalid file system path. **]**

**SRS_NODEJS_13_004: [** `NodeJS_Create` shall return `NULL` if an underlying API call fails. **]**

**SRS_NODEJS_13_005: [** `NodeJS_Create` shall return a non-`NULL` `MODULE_HANDLE` when successful. **]**

**SRS_NODEJS_13_035: [** `NodeJS_Create` shall acquire a reference to the singleton `ModulesManager` object. **]**

**SRS_NODEJS_13_036: [** `NodeJS_Create` shall invoke `ModulesManager::AddModule` to add a new module to it's internal list. **]**

**SRS_NODEJS_13_011: [** When the `NODEJS_MODULE_HANDLE_DATA::on_module_start` function is invoked, a new JavaScript object shall be created and added to the v8 global context with the name `gatewayHost`. The object shall implement the following interface (TypeScript syntax used below is just for the purpose of description):
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

**SRS_NODEJS_13_014: [** When the native implementation of `GatewayModuleHost.registerModule` is invoked it shall do nothing if at least 2 parameters have not been passed to it. **]**

**SRS_NODEJS_13_015: [** When the native implementation of `GatewayModuleHost.registerModule` is invoked it shall do nothing if the first parameter passed to it is not a JavaScript object. **]**

**SRS_NODEJS_13_037: [** When the native implementation of `GatewayModuleHost.registerModule` is invoked it shall do nothing if the second parameter passed to it is not a number. **]**

**SRS_NODEJS_13_016: [** When the native implementation of `GatewayModuleHost.registerModule` is invoked it shall do nothing if the parameter is an object but does not conform to the following interface:
```ts
interface GatewayModule {
    create: (broker: Broker, configuration: any) => boolean;
    receive: (message: Message) => void;
    destroy: () => void;
}
```
**]**

**SRS_NODEJS_13_017: [** A JavaScript object conforming to the `Broker` interface defined shall be created:
```ts
interface StringMap {
    [key: string]: string;
}

interface Message {
    properties: StringMap;
    content: Uint8Array;
}

interface Broker {
    publish: (message: Message) => boolean;
}
```
 **]**
 
 **SRS_NODEJS_13_018: [** The `GatewayModule.create` method shall be invoked passing the `Broker` instance and a parsed instance of the configuration JSON string. **]**

NODEJS_Receive
---------------
```c
void NodeJS_Receive(MODULE_HANDLE module, MESSAGE_HANDLE message);
```

**SRS_NODEJS_13_020: [** `NodeJS_Receive` shall do nothing if `module` is `NULL`. **]**

**SRS_NODEJS_13_021: [** `NodeJS_Receive` shall do nothing if `message` is `NULL`. **]**

**SRS_NODEJS_13_038: [** `NodeJS_Receive` shall schedule a callback to be invoked on Node.js's event loop. **]**

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

Broker.publish
------------------
```c
void broker_publish(const v8::FunctionCallbackInfo<Value>& info);
```

**SRS_NODEJS_13_027: [** `broker_publish` shall set the return value to `false` if the arguments count is less than 1. **]**

**SRS_NODEJS_13_028: [** `broker_publish` shall set the return value to `false` if the first argument is not a JS object. **]**

**SRS_NODEJS_13_029: [** `broker_publish` shall set the return value to `false` if the first argument is an object whose shape does not conform to the following interface:
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

**SRS_NODEJS_13_031: [** `broker_publish` shall set the return value to `false` if any underlying platform call fails. **]**

**SRS_NODEJS_13_030: [** `broker_publish` shall construct and initialize a `MESSAGE_HANDLE` from the first argument. **]**

**SRS_NODEJS_13_032: [** `broker_publish` shall call `Broker_Publish` passing the newly constructed `MESSAGE_HANDLE`. **]**

**SRS_NODEJS_13_033: [** `broker_publish` shall set the return value to `true` or `false` depending on the status of the `Broker_Publish` call. **]**

**SRS_NODEJS_13_034: [** `broker_publish` shall destroy the `MESSAGE_HANDLE`. **]**

NODEJS_Destroy
--------------
```c
void NodeJS_Destroy(MODULE_HANDLE module);
```

**SRS_NODEJS_13_024: [** `NodeJS_Destroy` shall do nothing if `module` is `NULL`. **]**

**SRS_NODEJS_13_039: [** `NodeJS_Destroy` shall schedule a callback to be invoked on Node.js's event loop. **]**

**SRS_NODEJS_13_040: [** `NodeJS_Destroy` shall invoke the `destroy` method on module's JS implementation. **]**

**SRS_NODEJS_13_025: [** `NodeJS_Destroy` shall free all resources. **]**

Module_GetAPIs
--------------
```c
extern const MODULE_APIS* Module_GetAPIS(void);
```

**SRS_NODEJS_26_001: [** `Module_GetAPIS` shall fill out the provided `MODULES_API` structure with required module's APIs functions. **]**
