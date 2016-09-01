IOTHUB HL MODULE
================

High level design
-----------------

### References
[IOTHUB MODULE](./iothub.md)

[json](http://www.json.org)

[gateway](../../../../devdoc/gateway_requirements.md)

### Overview

This module adapts the IotHub module for use with the Gateway HL library. It is passthrough to the existing module,
with a specialized create function to interpret the serialized JSON module arguments.

### Expected module arguments
```json
{
    "IoTHubName" : "<the name of the IoTHub>",
    "IoTHubSuffix" : "<the suffix used in generating the host name>",
    "Transport" : "HTTP" | "http" | "AMQP" | "amqp" | "MQTT" | "mqtt"
}
```


### IotHub_HL_Create
```C
MODULE_HANDLE IotHub_HL_Create(BROKER_HANDLE broker, const void* configuration);
```
Creates a new module instance. `configuration` is a pointer to a null-terminated string containing the JSON object 
given to `Gateway_Create_From_JSON`.

**SRS_IOTHUBMODULE_HL_17_001: [**If `broker` is NULL then `IotHub_HL_Create` shall fail and return NULL.**]**
**SRS_IOTHUBMODULE_HL_17_002: [** If `configuration` is NULL then `IotHub_HL_Create` shall fail and return NULL. **]**
**SRS_IOTHUBMODULE_HL_17_003: [** If `configuration` is not a JSON string, then `IotHub_HL_Create` shall fail and return NULL. **]**
**SRS_IOTHUBMODULE_HL_17_004: [** `IotHub_HL_Create` shall parse `configuration` as a JSON string. **]**
**SRS_IOTHUBMODULE_HL_17_005: [** If parsing of `configuration` fails, `IotHub_HL_Create` shall fail and return NULL. **]**
**SRS_IOTHUBMODULE_HL_17_006: [** If the JSON object does not contain a value named "IoTHubName" then `IotHub_HL_Create` shall fail and return NULL. **]**
**SRS_IOTHUBMODULE_HL_17_007: [** If the JSON object does not contain a value named "IoTHubSuffix" then `IotHub_HL_Create` shall fail and return NULL. **]**
**SRS_IOTHUBMODULE_HL_05_001: [** If the JSON object does not contain a value named "Transport" then `IotHub_HL_Create` shall fail and return NULL. **]**
**SRS_IOTHUBMODULE_HL_05_002: [** If the value of "Transport" is not one of "HTTP", "AMQP", or "MQTT" (case-insensitive) then `IotHub_HL_Create` shall fail and return NULL. **]**
**SRS_IOTHUBMODULE_HL_17_008: [** `IotHub_HL_Create` shall invoke the IotHub module's create function, using the broker, IotHubName, IoTHubSuffix, and Transport. **]**
**SRS_IOTHUBMODULE_HL_17_009: [** When the lower layer IotHub module creation succeeds, `IotHub_HL_Create` shall succeed and return a non-NULL value. **]**
**SRS_IOTHUBMODULE_HL_17_010: [** If the lower layer IotHub module creation fails, `IotHub_HL_Create` shall fail and return NULL. **]**

### IotHub_HL_Receive
```C
MODULE_HANDLE IotHub_HL_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle);
```

**SRS_IOTHUBMODULE_HL_17_011: [** `IotHub_HL_Receive` shall pass the received parameters to the underlying IotHub module's receive function. **]**

### IotHub_HL_Destroy
```C
void IotHub_HL_Destroy(MODULE_HANDLE moduleHandle);
```

**SRS_IOTHUBMODULE_HL_17_012: [** `IotHub_HL_Destroy` shall free all used resources. **]**

### Module_GetAPIs
```C
extern const MODULE_APIS* Module_GetAPIS(void);
```

**SRS_IOTHUBMODULE_HL_17_013: [** `Module_GetAPIS` shall return a non-NULL pointer. **]**
**SRS_IOTHUBMODULE_HL_17_014: [** The MODULE_APIS structure shall have non-NULL `Module_Create`, `Module_Destroy`, and `Module_Receive` fields. **]**