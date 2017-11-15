// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef BLE_GATT_IO_LINUX_COMMON_H
#define BLE_GATT_IO_LINUX_COMMON_H

#include <stdint.h>

#include <glib.h>
#include <gio/gio.h>

#include "bluez_device.h"

#include "gio_async_seq.h"
#include "ble_gatt_io.h"

typedef struct BLEIO_GATT_HANDLE_DATA_TAG {
    BLE_MAC_ADDRESS             device_addr;            // MAC address of the BLE device.
    GDBusConnection*            bus;                    // connection to system d-bus
    GDBusObjectManagerClient*   object_manager;         // d-bus object manager for org.bluez
    bluezdevice*                device;                 // the device to do I/O with
    BLEIO_GATT_STATE            state;                  // current connection state
    uint8_t                     ble_controller_index;   // index of the bluetooth controller to be used
    GTree*                      char_object_path_map;   // maps characteristic UUIDs to d-bus object paths
}BLEIO_GATT_HANDLE_DATA;

#endif // BLE_GATT_IO_LINUX_COMMON_H
