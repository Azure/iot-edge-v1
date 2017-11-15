# Bluetooth Low Energy Module

## Overview

The Bluetooth Low Energy (BLE) module is a general purpose Gateway module that can be used to execute *read* and *write* I/O operations on a BLE device. The I/O operations are expressed in the form of *instructions* that are to be executed. Each instruction has a *type* associated with it and allows for the following use cases:

  - Read some data once
  - Write some data once
  - Read some data at a specified frequency
  - Write some data when the module starts running
  - Write some data when the module terminates

The I/O operations are specified in the context of a [GATT](https://learn.adafruit.com/introduction-to-bluetooth-low-energy/gatt) characteristic, i.e., the reads and writes are from/to GATT characteristics.

## Types

```c
typedef struct BLE_INSTRUCTION_TAG
{
    /**
    * The type of instruction this is from the BLEIO_SEQ_INSTRUCTION_TYPE enum.
    */
    BLEIO_SEQ_INSTRUCTION_TYPE  instruction_type;

    /**
    * The GATT characteristic to read from/write to.
    */
    STRING_HANDLE               characteristic_uuid;

    union
    {
        /**
        * If 'instruction_type' is equal to READ_PERIODIC then this
        * value indicates the polling interval in milliseconds.
        */
        uint32_t                interval_in_ms;

        /**
        * If 'instruction_type' is equal to WRITE_AT_INIT or WRITE_AT_EXIT
        * then this is the buffer that is to be written.
        */
        BUFFER_HANDLE           buffer;
    }data;
}BLE_INSTRUCTION;

typedef struct BLE_CONFIG_TAG
{
    BLE_DEVICE_CONFIG   device_config;  // BLE device information
    VECTOR_HANDLE       instructions;   // array of BLE_INSTRUCTION objects to be executed
}BLE_CONFIG;

typedef struct BLE_HANDLE_DATA_TAG
{
    BROKER_HANDLE       broker;
    BLE_DEVICE_CONFIG   device_config;
    BLEIO_GATT_HANDLE   bleio_gatt;
    BLEIO_SEQ_HANDLE    bleio_seq;
}BLE_HANDLE_DATA;
```

## BLE_ParseConfigurationFromJson
```c
BLE_CONFIG * BLE_ParseConfigurationFromJson(const char* configuration)
```

Creates a new BLE_CONFIG structure from JSON input. `configuration` is a `const char*` that contains a JSON string, as typically passed through the Gateway SDK function `Gateway_CreateFromJson`. The JSON object should conform to the following structure:

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

**SRS_BLE_05_001: [** `BLE_ParseConfigurationFromJson` shall return `NULL` if the `configuration` parameter is `NULL`. **]**

**SRS_BLE_05_002: [** `BLE_ParseConfigurationFromJson` shall return `NULL` if any of the underlying platform calls fail. **]**

**SRS_BLE_05_003: [** `BLE_ParseConfigurationFromJson` shall return `NULL` if the JSON does not start with an `object`. **]**

**SRS_BLE_05_004: [** `BLE_ParseConfigurationFromJson` shall return `NULL` if there is no `device_mac_address` property in the JSON. **]**

**SRS_BLE_05_013: [** `BLE_ParseConfigurationFromJson` shall return `NULL` if the `device_mac_address` property's value is not a well-formed MAC address. **]**

**SRS_BLE_05_005: [** `BLE_ParseConfigurationFromJson` shall return `NULL` if the `controller_index` value in the JSON is less than zero. **]**

**SRS_BLE_05_006: [** `BLE_ParseConfigurationFromJson` shall return `NULL` if the `instructions` array does not exist in the JSON. **]**

**SRS_BLE_05_020: [** `BLE_ParseConfigurationFromJson` shall return `NULL` if the `instructions` array length is equal to zero. **]**

**SRS_BLE_05_007: [** `BLE_ParseConfigurationFromJson` shall return `NULL` if each instruction is not an `object`. **]**

**SRS_BLE_05_008: [** `BLE_ParseConfigurationFromJson` shall return `NULL` if a given instruction does not have a `type` property. **]**

**SRS_BLE_05_021: [** `BLE_ParseConfigurationFromJson` shall return `NULL` if a given instruction's `type` property is unrecognized. **]**

**SRS_BLE_05_009: [** `BLE_ParseConfigurationFromJson` shall return `NULL` if a given instruction does not have a `characteristic_uuid` property. **]**

**SRS_BLE_05_010: [** `BLE_ParseConfigurationFromJson` shall return `NULL` if the `interval_in_ms` value for a `read_periodic` instruction isn't greater than zero. **]**

**SRS_BLE_05_011: [** `BLE_ParseConfigurationFromJson` shall return `NULL` if an instruction of type `write_at_init` or `write_at_exit` does not have a `data` property. **]**

**SRS_BLE_05_012: [** `BLE_ParseConfigurationFromJson` shall return `NULL` if an instruction of type `write_at_init` or `write_at_exit` has a `data` property whose value does not decode successfully from base 64. **]**

**SRS_BLE_17_001: [** `BLE_ParseConfigurationFromJson` shall allocate a new `BLE_CONFIG` structure containing BLE instructions and configuration as parsed from the JSON input.  **]**

**SRS_BLE_05_023: [** `BLE_ParseConfigurationFromJson` shall return a non-`NULL` pointer to the `BLE_CONFIG` struct allocated if successful. **]**

## BLE_FreeConfiguration
```c
void BLE_FreeConfiguration(void* configuration)
```
Frees all resources allocated by `BLE_ParseConfigurationFromJson`.

**SRS_BLE_17_002: [** `BLE_FreeConfiguration` shall do nothing if `configuration` is `NULL`. **]**

**SRS_BLE_17_003: [** `BLE_FreeConfiguration` shall release all resources allocated in the `BLE_CONFIG` structure and release `configuration`. **]**


## BLE_Create
```c
MODULE_HANDLE BLE_Create(BROKER_HANDLE broker, const void* configuration);
```

Creates a new BLE module instance. The parameter `configuration` is a pointer to a `BLE_CONFIG` object.

**SRS_BLE_13_001: [** `BLE_Create` shall return `NULL` if `broker` is `NULL`. **]**

**SRS_BLE_13_002: [** `BLE_Create` shall return `NULL` if `configuration` is `NULL`. **]**

**SRS_BLE_13_003: [** `BLE_Create` shall return `NULL` if `configuration->instructions` is `NULL`. **]**

**SRS_BLE_13_004: [** `BLE_Create` shall return `NULL` if the `configuration->instructions` vector is empty (size is zero). **]**

**SRS_BLE_13_005: [** `BLE_Create` shall return `NULL` if an underlying API call fails. **]**

**SRS_BLE_13_006: [** `BLE_Create` shall return a non-`NULL` `MODULE_HANDLE` when successful. **]**

**SRS_BLE_13_009: [** `BLE_Create` shall allocate memory for an instance of the `BLE_HANDLE_DATA` structure and use that as the backing structure for the module handle. **]**

**SRS_BLE_13_008: [** `BLE_Create` shall create and initialize the `bleio_gatt` field in the `BLE_HANDLE_DATA` object by calling `BLEIO_gatt_create`. **]**

**SRS_BLE_13_010: [** `BLE_Create` shall create and initialize the `bleio_seq` field in the `BLE_HANDLE_DATA` object by calling `BLEIO_Seq_Create`. **]**

**SRS_BLE_13_011: [** `BLE_Create` shall asynchronously open a connection to the BLE device by calling `BLEIO_gatt_connect`. **]**

**SRS_BLE_13_012: [** `BLE_Create` shall return `NULL` if `BLEIO_gatt_connect` returns a non-zero value. **]**

**SRS_BLE_13_014: [** If the asynchronous call to `BLEIO_gatt_connect` is successful then the `BLEIO_Seq_Run` function shall be called on the `bleio_seq` field from `BLE_HANDLE_DATA`. **]**

**SRS_BLE_13_019: [** `BLE_Create` shall handle the `ON_BLEIO_SEQ_READ_COMPLETE` callback on the BLE I/O sequence. If the call is successful then a new message shall be published on the message broker with the buffer that was read as the content of the message along with the following properties:

>| Property Name           | Description                                                   |
>|-------------------------|---------------------------------------------------------------|
>| ble_controller_index    | The index of the bluetooth radio hardware on the device.      |
>| mac_address             | MAC address of the BLE device from which the data was read.   |
>| timestamp               | Timestamp indicating when the data was read.                  |
>| source                  | This property will always have the value `bleTelemetry`.      |

**]**

## BLE_Receive
```c
void BLE_Receive(MODULE_HANDLE module, MESSAGE_HANDLE message);
```

**SRS_BLE_13_018: [** `BLE_Receive` shall do nothing if `module` is `NULL` or if `message` is `NULL`. **]**

**SRS_BLE_13_020: [** `BLE_Receive` shall ignore all messages except those that have the following properties:

>| Property Name           | Description                                                             |
>|-------------------------|-------------------------------------------------------------------------|
>| source                  | This property should have the value `bleCommand`.                       |
>| macAddress              | MAC address of the BLE device to which the data to should be written.   |

**]**

**SRS_BLE_13_022: [** `BLE_Receive` shall ignore the message unless the 'macAddress' property matches the MAC address that was passed to this module when it was created. **]**

**SRS_BLE_13_021: [** `BLE_Receive` shall treat the content of the message as a `BLE_INSTRUCTION` and schedule it for execution by calling `BLEIO_Seq_AddInstruction`. **]**

## BLE_Destroy
```c
void BLE_Destroy(MODULE_HANDLE module);
```

**SRS_BLE_13_016: [** If `module` is `NULL` `BLE_Destroy` shall do nothing. **]**

**SRS_BLE_13_017: [** `BLE_Destroy` shall free all resources. **]**

## Module_GetApi
```c
MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version);
```

**SRS_BLE_26_001: [** `Module_GetApi` return a pointer to a `MODULE_API` structure. **]**
