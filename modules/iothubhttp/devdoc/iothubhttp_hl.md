IOTHUB HTTP HL MODULE
===========

High level design
-----------------

### References
[IOTHUB HTTP MODULE](./iothubhttpmodule.md)

[json](http://www.json.org)

[gateway](../../../../devdoc/gateway_requirements.md)

### Overview

This module adapts the existing lower layer IoTHubHttp module for use with the Gateway
HL library.  It is mostly a passthrough to the existing module, with a specialized
create function to  interpret the serialized JSON module arguments.

### Expected Arguments
```json
{
    "IoTHubName" : "<the name of the IoTHub>",
    "IoTHubSuffix" : "<the suffix used in generating the host name>"
}
```


###IoTHubHttp_HL_Create
```C
MODULE_HANDLE IoTHubHttp_HL_Create(MESSAGE_BUS_HANDLE busHandle, const void* configuration);
```
Creates a new IoTHubHttp_HL instance. `configuration` is a pointer to a char* 
of the serialzed JSON object, as supplied by `Gateway_Create_From_JSON`.

**SRS_IOTHUBHTTP_HL_17_001: [**If `busHandle` is NULL then `IoTHubHttp_HL_Create` 
shall fail and return NULL.**]**
 
**SRS_IOTHUBHTTP_HL_17_002: [**If `configuration` is NULL then 
`IoTHubHttp_HL_Create` shall fail and return NULL.**]**

**SRS_IOTHUBHTTP_HL_17_003: [** If `configuration` is not a JSON string, then `IoTHubHttp_HL_Create` shall 
fail and return NULL. **]**

**SRS_IOTHUBHTTP_HL_17_004: [** `IoTHubHttp_HL_Create` shall parse the `configuration` as a JSON string. **]**

**SRS_IOTHUBHTTP_HL_17_005: [** If parsing configuration fails, `IoTHubHttp_HL_Create` shall fail and return 
NULL. **]**

**SRS_IOTHUBHTTP_HL_17_006: [** If the JSON object does not contain a value named "IoTHubName" then `IoTHubHttp_HL_Create` shall fail and return NULL. **]**

**SRS_IOTHUBHTTP_HL_17_007: [** If the JSON object does not contain a value named "IoTHubSuffix" then `IoTHubHttp_HL_Create` shall fail and return NULL. **]**

**SRS_IOTHUBHTTP_HL_17_008: [** `IoTHubHttp_HL_Create` shall invoke the IoTHubHttp Module's create, using the busHandle, IotHubName, and IoTHubSuffix. **]**

**SRS_IOTHUBHTTP_HL_17_009: [** When the lower layer IoTHubHttp create succeeds, `IoTHubHttp_HL_Create` shall succeed and return a non-NULL value. **]**

**SRS_IOTHUBHTTP_HL_17_010: [** If the lower layer IoTHubHttp create fails, `IoTHubHttp_HL_Create` shall fail and return NULL. **]**

###IoTHubHttp_HL_Receive
```C
MODULE_HANDLE IoTHubHttp_HL_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle);
```

**SRS_IOTHUBHTTP_HL_17_011: [** `IoTHubHttp_HL_Receive` shall pass the received parameters to the underlying  IoTHubHttp receive function. **]**

###IoTHubHttp_HL_Destroy
```C
void IoTHubHttp_HL_Destroy(MODULE_HANDLE moduleHandle);
```
**SRS_IOTHUBHTTP_HL_17_012: [** `IoTHubHttp_HL_Destroy` shall free all used resources. **]**


###Module_GetAPIs
```C
extern const MODULE_APIS* Module_GetAPIS(void);
```

**SRS_IOTHUBHTTP_HL_17_013: [** `Module_GetAPIS` shall return a non-NULL pointer. **]**
 
**SRS_IOTHUBHTTP_HL_17_014: [** The MODULE_APIS structure shall have non-NULL `Module_Create`, `Module_Destroy`, and `Module_Receive` fields. **]**