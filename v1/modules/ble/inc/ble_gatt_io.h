// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef BLE_GATT_IO_H
#define BLE_GATT_IO_H

#include "azure_c_shared_utility/macro_utils.h"

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
extern "C" {
#else
#include <stddef.h>
#include <stdint.h>
#endif /* __cplusplus */

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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BLE_GATT_IO_H */
