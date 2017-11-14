// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef BLE_UTILS_H
#define BLE_UTILS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "ble_gatt_io.h"

extern bool parse_mac_address(const char* mac_address_str, BLE_MAC_ADDRESS* mac_address);

#ifdef __cplusplus
}
#endif

#endif /*BLE_UTILS_H*/

