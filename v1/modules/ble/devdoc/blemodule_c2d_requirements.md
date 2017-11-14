# Bluetooth Low Energy Cloud To Device Messaging Module

## References

[BLE Module Requirements](./blemodule_requirements.md)

## Overview

This module is responsible for processing cloud-to-device (C2D) messages received as JSON and passing it on to the BLE module for execution. 

### Receiving messages on the message broker
The module identifies the messages that it needs to process by the following 
properties that must exist:

>| PropertyName | Description                                                                  |
>|--------------|------------------------------------------------------------------------------|
>| macAddress   | The MAC address of a sensor, in canonical form                               |
>| source       | The source property shall be set to "mapping"                                |

The message content is a JSON object of the format:
```json
{
    "type": "write_once",
    "characteristic_uuid": "<GATT characteristic to read from/write to>",
    "data": "<base64 value>"
}
```

## BLE_C2D_ParseConfigurationFromJson
```c
void* BLE_C2D_ParseConfigurationFromJson(const char* configuration)
```

Creates configuration for a BLE C2D module. `configuration` is ignored.

**SRS_BLE_CTOD_17_027: [** `BLE_C2D_ParseConfigurationFromJson` shall return `NULL`. **]**

## BLE_C2D_FreeConfiguration
```c
void BLE_C2D_FreeConfiguration(void* configuration);
```
**SRS_BLE_CTOD_17_028: [** `BLE_C2D_FreeConfiguration` shall do nothing. **]**

## BLE_C2D_Create
```c
MODULE_HANDLE BLE_C2D_Create(BROKER_HANDLE broker, const void* configuration)
```

Creates a new BLE C2D module instance. `configuration` is ignored.

**SRS_BLE_CTOD_13_001: [** `BLE_C2D_Create` shall return `NULL` if the `broker` parameter is `NULL`. **]**

**SRS_BLE_CTOD_13_002: [** `BLE_C2D_Create` shall return `NULL` if any of the underlying platform calls fail. **]**

**SRS_BLE_CTOD_13_023: [** `BLE_C2D_Create` shall return a non-`NULL` handle when the function succeeds. **]**

## BLE_C2D_Destroy
```c
void BLE_C2D_Destroy(MODULE_HANDLE module)
```

**SRS_BLE_CTOD_13_017: [** `BLE_C2D_Destroy` shall do nothing if `module` is `NULL`. **]**

**SRS_BLE_CTOD_13_015: [** `BLE_C2D_Destroy` shall destroy all used resources. **]**

## BLE_C2D_Receive
```c
void BLE_C2D_Receive(MODULE_HANDLE module, MESSAGE_HANDLE message_handle)
```

**SRS_BLE_CTOD_13_016: [** `BLE_C2D_Receive` shall do nothing if `module` is `NULL`. **]**

**SRS_BLE_CTOD_17_001: [** `BLE_C2D_Receive` shall do nothing if `message_handle` is `NULL`. **]**



**SRS_BLE_CTOD_17_002: [** If `message_handle` properties does not contain "macAddress" property, then this function shall do nothing. **]**

**SRS_BLE_CTOD_17_004: [** If `message_handle` properties does not contain "source" property, then this function shall do nothing. **]**

**SRS_BLE_CTOD_17_005: [** If the `source` of the message properties is not "mapping", then this function shall do nothing. **]**

**SRS_BLE_CTOD_17_006: [** `BLE_C2D_Receive` shall parse the message contents as a JSON object. **]**

**SRS_BLE_CTOD_17_007: [** If the message contents do not parse, then `BLE_C2D_Receive` shall do nothing. **]**

**SRS_BLE_CTOD_17_008: [** `BLE_C2D_Receive` shall return if the JSON object does not contain the following fields: "type" and "characteristic_uuid". **]**

**SRS_BLE_CTOD_17_014: [** `BLE_C2D_Receive` shall parse the json object to fill in a new `BLE_INSTRUCTION`. **]**

**SRS_BLE_CTOD_17_026: [** If the json object does not parse, `BLE_C2D_Receive` shall return. **]**

**SRS_BLE_CTOD_17_020: [** `BLE_C2D_Receive` shall call add a property with key of "source" and value of "bleCommand". **]**

**SRS_BLE_CTOD_17_023: [** `BLE_C2D_Receive` shall create a new message by calling `Message_Create` with new map and BLE_INSTRUCTION as the buffer. **]**

**SRS_BLE_CTOD_17_024: [** If creating new message fails, `BLE_C2D_Receive` shall de-allocate all resources and return. **]**

**SRS_BLE_CTOD_13_018: [** `BLE_C2D_Receive` shall publish the new message to the broker. **]**

**SRS_BLE_CTOD_13_024: [** `BLE_C2D_Receive` shall do nothing if an underlying API call fails. **]**

## Module_GetApi
```c
MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version);
```

**SRS_BLE_CTOD_26_001: [** `Module_GetApi` shall return a pointer to the `MODULE_API` structure. **]**
