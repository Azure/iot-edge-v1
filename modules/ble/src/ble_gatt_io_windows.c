// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/gballoc.h"

#include "ble_gatt_io.h"

BLEIO_GATT_HANDLE BLEIO_gatt_create(
    const BLE_DEVICE_CONFIG* config
)
{
    LogError("BLEIO_gatt_create not implemented on Windows yet.");
    return NULL;
}

void BLEIO_gatt_destroy(
    BLEIO_GATT_HANDLE bleio_gatt_handle
)
{
    LogError("BLEIO_gatt_destroy not implemented on Windows yet.");
}

int BLEIO_gatt_connect(
    BLEIO_GATT_HANDLE bleio_gatt_handle,
    ON_BLEIO_GATT_CONNECT_COMPLETE on_bleio_gatt_connect_complete,
    void* callback_context
)
{
    LogError("BLEIO_gatt_connect not implemented on Windows yet.");
    return __LINE__;
}

void BLEIO_gatt_disconnect(
    BLEIO_GATT_HANDLE bleio_gatt_handle,
    ON_BLEIO_GATT_DISCONNECT_COMPLETE on_bleio_gatt_disconnect_complete,
    void* callback_context
)
{
    LogError("BLEIO_gatt_disconnect not implemented on Windows yet.");
}

int BLEIO_gatt_read_char_by_uuid(
    BLEIO_GATT_HANDLE bleio_gatt_handle,
    const char* ble_uuid,
    ON_BLEIO_GATT_ATTRIB_READ_COMPLETE on_bleio_gatt_attrib_read_complete,
    void* callback_context
)
{
    LogError("BLEIO_gatt_read_char_by_uuid not implemented on Windows yet.");
    return __LINE__;
}

int BLEIO_gatt_write_char_by_uuid(
    BLEIO_GATT_HANDLE bleio_gatt_handle,
    const char* ble_uuid,
    const unsigned char* buffer,
    size_t size,
    ON_BLEIO_GATT_ATTRIB_WRITE_COMPLETE on_bleio_gatt_attrib_write_complete,
    void* callback_context
)
{
    LogError("BLEIO_gatt_write_char_by_uuid not implemented on Windows yet.");
    return __LINE__;
}
