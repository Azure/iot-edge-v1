# Bluetooth Low Energy GATT I/O

## Overview

The Bluetooth Low Energy (BLE) Generic Attribute Profile (GATT) input/output (I/O) API is designed to be a platform abstraction layer (PAL) API that abstracts away the low level details of communicating with BLE devices on a given platform. 

## References

* [Generic Attribute Profile](https://developer.bluetooth.org/TechnologyOverview/Pages/GATT.aspx)

## Additional data types

```c
/**
* The 6 byte mac address of the BLE device.
*/
typedef struct BLE_MAC_ADDRESS_TAG
{
    uint8_t address[6];
}BLE_MAC_ADDRESS;

typedef struct BLE_DEVICE_CONFIG_TAG
{
    /**
     * MAC address of the BLE device.
     */
    BLE_MAC_ADDRESS device_addr;

    /**
     * Zero based index of the bluetooth controller to be
     * used.
     */
    uint8_t ble_controller_index;
}BLE_DEVICE_CONFIG;

typedef struct BLEIO_GATT_INSTANCE_TAG* BLEIO_GATT_HANDLE;

#define BLEIO_GATT_RESULT_VALUES \
    BLEIO_GATT_OK, \
    BLEIO_GATT_ERROR

DEFINE_ENUM(BLEIO_GATT_RESULT, BLEIO_GATT_RESULT_VALUES);

#define BLEIO_GATT_STATE_VALUES \
    BLEIO_GATT_STATE_DISCONNECTED, \
    BLEIO_GATT_STATE_CONNECTING, \
    BLEIO_GATT_STATE_CONNECTED, \
    BLEIO_GATT_STATE_ERROR

DEFINE_ENUM(BLEIO_GATT_STATE, BLEIO_GATT_STATE_VALUES);

#define BLEIO_GATT_CONNECT_RESULT_VALUES \
    BLEIO_GATT_CONNECT_OK, \
    BLEIO_GATT_CONNECT_ERROR

DEFINE_ENUM(BLEIO_GATT_CONNECT_RESULT, BLEIO_GATT_CONNECT_RESULT_VALUES);

typedef void(*ON_BLEIO_GATT_CONNECT_COMPLETE)(BLEIO_GATT_HANDLE bleio_gatt_handle, void* context, BLEIO_GATT_CONNECT_RESULT connect_result);
typedef void(*ON_BLEIO_GATT_DISCONNECT_COMPLETE)(BLEIO_GATT_HANDLE bleio_gatt_handle, void* context);
typedef void(*ON_BLEIO_GATT_ATTRIB_READ_COMPLETE)(BLEIO_GATT_HANDLE bleio_gatt_handle, void* context, BLEIO_GATT_RESULT result, const unsigned char* buffer, size_t size);
typedef void(*ON_BLEIO_GATT_ATTRIB_WRITE_COMPLETE)(BLEIO_GATT_HANDLE bleio_gatt_handle, void* context, BLEIO_GATT_RESULT result);

extern BLEIO_GATT_HANDLE BLEIO_gatt_create(
    const BLE_DEVICE_CONFIG* config
);

extern void BLEIO_gatt_destroy(
    BLEIO_GATT_HANDLE bleio_gatt_handle
);

extern int BLEIO_gatt_connect(
    BLEIO_GATT_HANDLE bleio_gatt_handle,
    ON_BLEIO_GATT_CONNECT_COMPLETE on_bleio_gatt_connect_complete,
    void* callback_context
);

extern void BLEIO_gatt_disconnect(
    BLEIO_GATT_HANDLE bleio_gatt_handle,
    ON_BLEIO_GATT_DISCONNECT_COMPLETE on_bleio_gatt_disconnect_complete,
    void* callback_context
);

extern int BLEIO_gatt_read_char_by_uuid(
    BLEIO_GATT_HANDLE bleio_gatt_handle,
    const char* ble_uuid,
    ON_BLEIO_GATT_ATTRIB_READ_COMPLETE on_bleio_gatt_attrib_read_complete,
    void* callback_context
);

extern int BLEIO_gatt_write_char_by_uuid(
    BLEIO_GATT_HANDLE bleio_gatt_handle,
    const char* ble_uuid,
    const unsigned char* buffer,
    size_t size,
    ON_BLEIO_GATT_ATTRIB_WRITE_COMPLETE on_bleio_gatt_attrib_write_complete,
    void* callback_context
);
```

## BLEIO_gatt_create
```c
extern BLEIO_GATT_HANDLE BLEIO_gatt_create(
    const BLE_DEVICE_CONFIG* config
);
```

**SRS_BLEIO_GATT_13_001: [** `BLEIO_gatt_create` shall return a non-`NULL` handle on successful execution. **]**

**SRS_BLEIO_GATT_13_002: [** `BLEIO_gatt_create` shall return `NULL` when any of the underlying platform calls fail. **]**

**SRS_BLEIO_GATT_13_003: [** `BLEIO_gatt_create` shall return `NULL` if `config` is `NULL`. **]**

## BLEIO_gatt_destroy
```c
extern void BLEIO_gatt_destroy(
    BLEIO_GATT_HANDLE bleio_gatt_handle
);
```

> If there are pending I/O operations in progress when this API is called, the semantics of completion/cancellation of those requests is left to the underlying platform. GLIB's IO channels for instance (which is used internally by *bluez*) allow the caller to specify that pending data that is to be written should be flushed before the channel is shutdown. When the platform provides such capabilities this API should attempt to use them to cancel the pending requests.

**SRS_BLEIO_GATT_13_004: [** `BLEIO_gatt_destroy` shall free all resources associated with the handle. **]**

**SRS_BLEIO_GATT_13_005: [** If `bleio_gatt_handle` is `NULL` `BLEIO_gatt_destroy` shall do nothing. **]**

## BLEIO_gatt_connect
```c
extern int BLEIO_gatt_connect(
    BLEIO_GATT_HANDLE bleio_gatt_handle,
    ON_BLEIO_GATT_CONNECT_COMPLETE on_bleio_gatt_connect_complete,
    void* callback_context
);
```

**SRS_BLEIO_GATT_13_007: [** `BLEIO_gatt_connect` shall asynchronously attempt to open a connection with the BLE device. **]**

**SRS_BLEIO_GATT_13_008: [** On initiating the connect successfully, `BLEIO_gatt_connect` shall return `0` (zero). **]**

**SRS_BLEIO_GATT_13_009: [** If any of the underlying platform calls fail, `BLEIO_gatt_connect` shall return a non-zero value. **]**

**SRS_BLEIO_GATT_13_010: [** If `bleio_gatt_handle` or `on_bleio_gatt_connect_complete` are `NULL` then `BLEIO_gatt_connect` shall return a non-zero value. **]**

**SRS_BLEIO_GATT_13_011: [** When the connect operation to the device has been completed, the callback function pointed at by `on_bleio_gatt_connect_complete` shall be invoked. **]**

**SRS_BLEIO_GATT_13_012: [** When `on_bleio_gatt_connect_complete` is invoked the value passed in `callback_context` to `BLEIO_gatt_connect` shall be passed along to `on_bleio_gatt_connect_complete`. **]**

**SRS_BLEIO_GATT_13_013: [** The `connect_result` parameter of the `on_bleio_gatt_connect_complete` callback shall indicate the status of the connect operation. **]**

**SRS_BLEIO_GATT_13_047: [** If when `BLEIO_gatt_connect` is called, there's another connection request already in progress or if an open connection already exists, then this API shall return a non-zero error code. **]**

## BLEIO_gatt_disconnect
```c
extern void BLEIO_gatt_disconnect(
    BLEIO_GATT_HANDLE bleio_gatt_handle,
    ON_BLEIO_GATT_DISCONNECT_COMPLETE on_bleio_gatt_disconnect_complete,
    void* callback_context
);
```

> If there are pending I/O operations in progress when this API is called, the semantics of completion/cancellation of those requests is left to the underlying platform. GLIB's IO channels for instance (which is used internally by *bluez*) allow the caller to specify that pending data that is to be written should be flushed before the channel is shutdown. When the platform provides such capabilities this API should attempt to use them to cancel the pending requests.

**SRS_BLEIO_GATT_13_014: [** `BLEIO_gatt_disconnect` shall asynchronously disconnect from the BLE device if an open connection exists. **]**

**SRS_BLEIO_GATT_13_050: [** When the disconnect operation has been completed, the callback function pointed at by `on_bleio_gatt_disconnect_complete` shall be invoked if it is not `NULL`. **]**

**SRS_BLEIO_GATT_13_049: [** When `on_bleio_gatt_disconnect_complete` is invoked the value passed in `callback_context` to `BLEIO_gatt_disconnect` shall be passed along to `on_bleio_gatt_disconnect_complete`. **]**

**SRS_BLEIO_GATT_13_015: [** `BLEIO_gatt_disconnect` shall do nothing if an open connection does not exist when it is called. **]**

**SRS_BLEIO_GATT_13_016: [** `BLEIO_gatt_disconnect` shall do nothing if `bleio_gatt_handle` is `NULL`. **]**

**SRS_BLEIO_GATT_13_054: [** `BLEIO_gatt_disconnect` shall do nothing if an underlying platform call fails. **]**

## BLEIO_gatt_read_char_by_uuid

```c
extern int BLEIO_gatt_read_char_by_uuid(
    BLEIO_GATT_HANDLE bleio_gatt_handle,
    const BLE_UUID* ble_uuid,
    ON_BLEIO_GATT_ATTRIB_READ_COMPLETE on_bleio_gatt_attrib_read_complete,
    void* callback_context
);
```
**SRS_BLEIO_GATT_13_027: [** `BLEIO_gatt_read_char_by_uuid` shall return a non-zero value if `bleio_gatt_handle` is `NULL`. **]**

**SRS_BLEIO_GATT_13_028: [** `BLEIO_gatt_read_char_by_uuid` shall return a non-zero value if `on_bleio_gatt_attrib_read_complete` is `NULL`. **]**

**SRS_BLEIO_GATT_13_051: [** `BLEIO_gatt_read_char_by_uuid` shall return a non-zero value if `ble_uuid` is `NULL`. **]**

**SRS_BLEIO_GATT_13_052: [** `BLEIO_gatt_read_char_by_uuid` shall return a non-zero value if the object is not in a *connected* state. **]**

**SRS_BLEIO_GATT_13_029: [** `BLEIO_gatt_read_char_by_uuid` shall return a non-zero value if an underlying platform call fails. **]**

**SRS_BLEIO_GATT_13_030: [** `BLEIO_gatt_read_char_by_uuid` shall return 0 (zero) if the *read characteristic* operation is successful. **]**

**SRS_BLEIO_GATT_13_031: [** `BLEIO_gatt_read_char_by_uuid` shall asynchronously initiate a *read characteristic* operation using the specified UUID. **]**

**SRS_BLEIO_GATT_13_032: [** `BLEIO_gatt_read_char_by_uuid` shall invoke `on_bleio_gatt_attrib_read_complete` when the read operation completes. **]**

**SRS_BLEIO_GATT_13_033: [** `BLEIO_gatt_read_char_by_uuid` shall pass the value of the `callback_context` parameter to `on_bleio_gatt_attrib_read_complete` as the `context` parameter when it is invoked. **]**

**SRS_BLEIO_GATT_13_034: [** `BLEIO_gatt_read_char_by_uuid`, when successful, shall supply the data that has been read to the `on_bleio_gatt_attrib_read_complete` callback along with the value `BLEIO_GATT_OK` for the `result` parameter. **]**

**SRS_BLEIO_GATT_13_035: [** When an error occurs asynchronously, the value `BLEIO_GATT_ERROR` shall be passed for the `result` parameter of the `on_bleio_gatt_attrib_read_complete` callback. **]**

## BLEIO_gatt_write_char_by_uuid

```c
extern int BLEIO_gatt_write_char_by_uuid(
    BLEIO_GATT_HANDLE bleio_gatt_handle,
    const char* ble_uuid,
    const unsigned char* buffer,
    size_t size,
    ON_BLEIO_GATT_ATTRIB_WRITE_COMPLETE on_bleio_gatt_attrib_write_complete,
    void* callback_context
);
```
**SRS_BLEIO_GATT_13_036: [** `BLEIO_gatt_write_char_by_uuid` shall return a non-zero value if `bleio_gatt_handle` is `NULL`. **]**

**SRS_BLEIO_GATT_13_045: [** `BLEIO_gatt_write_char_by_uuid` shall return a non-zero value if `buffer` is `NULL`. **]**

**SRS_BLEIO_GATT_13_048: [** `BLEIO_gatt_write_char_by_uuid` shall return a non-zero value if `ble_uuid` is `NULL`. **]**

**SRS_BLEIO_GATT_13_046: [** `BLEIO_gatt_write_char_by_uuid` shall return a non-zero value if `size` is equal to `0` (zero). **]**

**SRS_BLEIO_GATT_13_053: [** `BLEIO_gatt_write_char_by_uuid` shall return a non-zero value if an active connection to the device does not exist. **]**

**SRS_BLEIO_GATT_13_037: [** `BLEIO_gatt_write_char_by_uuid` shall return a non-zero value if `on_bleio_gatt_attrib_write_complete` is `NULL`. **]**

**SRS_BLEIO_GATT_13_038: [** `BLEIO_gatt_write_char_by_uuid` shall return a non-zero value if an underlying platform call fails. **]**

**SRS_BLEIO_GATT_13_039: [** `BLEIO_gatt_write_char_by_uuid` shall return 0 (zero) if the *write characteristic* operation is successful. **]**

**SRS_BLEIO_GATT_13_040: [** `BLEIO_gatt_write_char_by_uuid` shall asynchronously initiate a *write characteristic* operation using the specified UUID. **]**

**SRS_BLEIO_GATT_13_041: [** `BLEIO_gatt_write_char_by_uuid` shall invoke `on_bleio_gatt_attrib_write_complete` when the write operation completes. **]**

**SRS_BLEIO_GATT_13_042: [** `BLEIO_gatt_write_char_by_uuid` shall pass the value of the `callback_context` parameter to `on_bleio_gatt_attrib_write_complete` as the `context` parameter when it is invoked. **]**

**SRS_BLEIO_GATT_13_043: [** `BLEIO_gatt_write_char_by_uuid`, when successful, shall supply the value `BLEIO_GATT_OK` for the `result` parameter. **]**

**SRS_BLEIO_GATT_13_044: [** When an error occurs asynchronously, the value `BLEIO_GATT_ERROR` shall be passed for the `result` parameter of the `on_bleio_gatt_attrib_write_complete` callback. **]**