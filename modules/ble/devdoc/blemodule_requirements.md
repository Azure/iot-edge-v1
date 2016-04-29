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
    MESSAGE_BUS_HANDLE  bus;
    BLE_DEVICE_CONFIG   device_config;
    BLEIO_GATT_HANDLE   bleio_gatt;
    BLEIO_SEQ_HANDLE    bleio_seq;
}BLE_HANDLE_DATA;
```

## BLE_Create
```c
MODULE_HANDLE BLE_Create(MESSAGE_BUS_HANDLE bus, const void* configuration);
```

Creates a new BLE module instance. The parameter `configuration` is a pointer to a `BLE_CONFIG` object.

**SRS_BLE_13_001: [** `BLE_Create` shall return `NULL` if `bus` is `NULL`. **]**

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

**SRS_BLE_13_019: [** `BLE_Create` shall handle the `ON_BLEIO_SEQ_READ_COMPLETE` callback on the BLE I/O sequence. If the call is successful then a new message shall be published on the message bus with the buffer that was read as the content of the message along with the following properties:

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

## Module_GetAPIs
```c
extern const MODULE_APIS* Module_GetAPIS(void);
```

**SRS_BLE_13_007: [** `Module_GetAPIS` shall return a non-`NULL` pointer to a structure of type `MODULE_APIS` that has all fields initialized to non-`NULL` values. **]**
