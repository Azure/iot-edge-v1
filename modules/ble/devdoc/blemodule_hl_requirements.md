# Bluetooth Low Energy High Level Module

## References

[BLE Module Requirements](./blemodule_requirements.md)

## Overview

This module is a *passthrough* implementation that simply wraps the BLE Module code by providing an easy to use JSON based configuration interface. It de-serializes the JSON into a `BLE_CONFIG` instance and passes control to the underlying implementation. This module shall also accept cloud to device messages and transform cloud messages into a structure understood by the BLE module.

### Receiving messages on the message bus
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

## BLE_HL_Create
```c
MODULE_HANDLE BLE_HL_Create(MESSAGE_BUS_HANDLE bus, const void* configuration)
```

Creates a new BLE Module HL instance. `configuration` is a `const char*` that contains a JSON string that's typically passed through via the high level Gateway API `Gateway_Create_From_JSON`. The JSON object should conform to the following structure:

```
{
    /**
     * Index of the BLE controller hardware on the device.
     */
    "controller_index": 0,
    
    /**
     * MAC address of the BLE device to connect to.
     */
    "device_mac_address": "AA:BB:CC:DD:EE:FF",
    
    /**
     * One or more instructions to be sent to the BLE device.
     */
    "instructions": [
        {
            /**
             * The instruction type that maps to the `BLEIO_SEQ_INSTRUCTION_TYPE`
             * enumeration from `bleio_seq.h`. The 'type' property can be one of
             * the following: `read_once`, `read_periodic`, `write_at_init` and
             * `write_at_exit` each mapping respectively to the `READ_ONCE`,
             * `READ_PERIODIC`, `WRITE_AT_INIT` and `WRITE_AT_EXIT` values from
             * the `BLEIO_SEQ_INSTRUCTION_TYPE` enumeration.
             */
            "type": "read_once",
            
            /**
             * The GATT characteristic's UUID string.
             */
            "characteristic_uuid": "00002A24-0000-1000-8000-00805F9B34FB"
        },
        {
            "type": "read_periodic",
            "characteristic_uuid": "F000AA01-0451-4000-B000-000000000000",
            
            /**
             * For a `read_periodic` instruction this value specifies the
             * sampling frequency for the characteristic in question given
             * in milliseconds.
             */
            "interval_in_ms": 1000
        },
        {
            "type": "write_at_init",
            "characteristic_uuid": "F000AA02-0451-4000-B000-000000000000",
            
            /**
             * For `write_at_init` and `write_at_exit` instructions this value
             * provides the data to be written to the device encoded as a Base64
             * string.
             */
            "data": "AQ=="
        },
        {
            "type": "write_at_exit",
            "characteristic_uuid": "F000AA02-0451-4000-B000-000000000000",
            "data": "AA=="
        }
    ]
}
```

**SRS_BLE_HL_13_001: [** `BLE_HL_Create` shall return `NULL` if the `bus` or `configuration` parameters are `NULL`. **]**

**SRS_BLE_HL_13_002: [** `BLE_HL_Create` shall return `NULL` if any of the underlying platform calls fail. **]**

**SRS_BLE_HL_13_003: [** `BLE_HL_Create` shall return `NULL` if the JSON does not start with an `object`. **]**

**SRS_BLE_HL_13_004: [** `BLE_HL_Create` shall return `NULL` if there is no `device_mac_address` property in the JSON. **]**

**SRS_BLE_HL_13_013: [** `BLE_HL_Create` shall return `NULL` if the `device_mac_address` property's value is not a well-formed MAC address. **]**

**SRS_BLE_HL_13_005: [** `BLE_HL_Create` shall return `NULL` if the `controller_index` value in the JSON is less than zero. **]**

**SRS_BLE_HL_13_006: [** `BLE_HL_Create` shall return `NULL` if the `instructions` array does not exist in the JSON. **]**

**SRS_BLE_HL_13_020: [** `BLE_HL_Create` shall return `NULL` if the `instructions` array length is equal to zero. **]**

**SRS_BLE_HL_13_007: [** `BLE_HL_Create` shall return `NULL` if each instruction is not an `object`. **]**

**SRS_BLE_HL_13_008: [** `BLE_HL_Create` shall return `NULL` if a given instruction does not have a `type` property. **]**

**SRS_BLE_HL_13_021: [** `BLE_HL_Create` shall return `NULL` if a given instruction's `type` property is unrecognized. **]**

**SRS_BLE_HL_13_009: [** `BLE_HL_Create` shall return `NULL` if a given instruction does not have a `characteristic_uuid` property. **]**

**SRS_BLE_HL_13_010: [** `BLE_HL_Create` shall return `NULL` if the `interval_in_ms` value for a `read_periodic` instruction isn't greater than zero. **]**

**SRS_BLE_HL_13_011: [** `BLE_HL_Create` shall return `NULL` if an instruction of type `write_at_init` or `write_at_exit` does not have a `data` property. **]**

**SRS_BLE_HL_13_012: [** `BLE_HL_Create` shall return `NULL` if an instruction of type `write_at_init` or `write_at_exit` has a `data` property whose value does not decode successfully from base 64. **]**

**SRS_BLE_HL_13_014: [** `BLE_HL_Create` shall call the underlying module's 'create' function. **]**

**SRS_BLE_HL_13_022: [** `BLE_HL_Create` shall return `NULL` if calling the underlying module's `create` function fails. **]**

**SRS_BLE_HL_13_023: [** `BLE_HL_Create` shall return a non-`NULL` handle if calling the underlying module's `create` function succeeds. **]**

## BLE_HL_Destroy
```c
void BLE_HL_Destroy(MODULE_HANDLE module)
```

**SRS_BLE_HL_13_017: [** `BLE_HL_Destroy` shall do nothing if `module` is `NULL`. **]**

**SRS_BLE_HL_13_015: [** `BLE_HL_Destroy` shall destroy all used resources. **]**

## BLE_HL_Receive
```c
void BLE_HL_Receive(MODULE_HANDLE module, MESSAGE_HANDLE message_handle)
```

**SRS_BLE_HL_13_016: [** `BLE_HL_Receive` shall do nothing if `module` is `NULL`. **]**

**SRS_BLE_HL_17_001: [** `BLE_HL_Receive` shall do nothing if `message_handle` is `NULL`. **]**

**SRS_BLE_HL_17_002: [** If `messageHandle` properties does not contain "macAddress" property, then this function shall return. **]**

**SRS_BLE_HL_17_003: [** If `macAddress` of the message property does not match this module's MAC address, then this function shall return. **]**

**SRS_BLE_HL_17_004: [** If `messageHandle` properties does not contain "source" property, then this function shall return. **]**

**SRS_BLE_HL_17_005: [** If the `source` of the message properties is not "mapping", then this function shall return. **]**

**SRS_BLE_HL_17_006: [** `BLE_HL_Receive` shall parse the message contents as a JSON object. **]**

**SRS_BLE_HL_17_007: [** If the message contents do not parse, then `BLE_HL_Receive` shall return. **]**

**SRS_BLE_HL_17_008: [** `BLE_HL_Receive` shall return if the JSON object does not contain the following fields: "type", "characteristic_uuid", and "data". **]**

**SRS_BLE_HL_17_012: [** `BLE_HL_Receive` shall create a `STRING_HANDLE` from the characteristic_uuid data field. **]**

**SRS_BLE_HL_17_013: [** If the string creation fails, `BLE_HL_Receive` shall return. **]**

**SRS_BLE_HL_17_014: [** `BLE_HL_Receive` shall parse the json object to fill in a new BLE_INSTRUCTION. **]**

**SRS_BLE_HL_17_026: [** If the json object does not parse, `BLE_HL_Receive` shall return. **]**

**SRS_BLE_HL_17_016: [** `BLE_HL_Receive` shall set characteristic_uuid to the created STRING. **]**

**SRS_BLE_HL_17_018: [** `BLE_HL_Receive` shall call `ConstMap_CloneWriteable` on the message properties. **]**

**SRS_BLE_HL_17_019: [** If `ConstMap_CloneWriteable` fails, `BLE_HL_Receive` shall return. **]**

**SRS_BLE_HL_17_020: [** `BLE_HL_Receive` shall call `Map_AddOrUpdate` with key of "source" and value of "BLE". **]**

**SRS_BLE_HL_17_023: [** `BLE_HL_Receive` shall create a new message by calling `Message_Create` with new map and BLE_INSTRUCTION as the buffer. **]**

**SRS_BLE_HL_17_024: [** If creating new message fails, `BLE_HL_Receive` shall deallocate all resources and return. **]**

**SRS_BLE_HL_13_018: [** `BLE_HL_Receive` shall forward new message to the underlying module. **]**

**SRS_BLE_HL_17_025: [** `BLE_HL_Receive` shall free all resources created. **]**

## Module_GetAPIs
```c
extern const MODULE_APIS* Module_GetAPIS(void);
```

**SRS_BLE_HL_13_019: [** `Module_GetAPIS` shall return a non-`NULL` pointer to a structure of type `MODULE_APIS` that has all fields initialized to non-`NULL` values. **]**
