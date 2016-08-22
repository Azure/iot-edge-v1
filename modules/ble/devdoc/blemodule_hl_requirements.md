# Bluetooth Low Energy High Level Module

## References

[BLE Module Requirements](./blemodule_requirements.md)

## Overview

This module is a *passthrough* implementation that simply wraps the BLE Module code by providing an easy to use JSON based configuration interface. It de-serializes the JSON into a `BLE_CONFIG` instance and passes control to the underlying implementation.

## BLE_HL_Create
```c
MODULE_HANDLE BLE_HL_Create(BROKER_HANDLE broker, const void* configuration)
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

**SRS_BLE_HL_13_001: [** `BLE_HL_Create` shall return `NULL` if the `broker` or `configuration` parameters are `NULL`. **]**

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

**SRS_BLE_HL_13_024: [** `BLE_HL_Receive` shall pass the received parameters to the underlying BLE module's receive function. **]**

## Module_GetAPIs
```c
extern const MODULE_APIS* Module_GetAPIS(void);
```

**SRS_BLE_HL_13_019: [** `Module_GetAPIS` shall return a non-`NULL` pointer to a structure of type `MODULE_APIS` that has all fields initialized to non-`NULL` values. **]**
