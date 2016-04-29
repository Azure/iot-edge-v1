// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <stdio.h>

#include "ble_gatt_io.h"
#include "ble_utils.h"

bool parse_mac_address(const char* mac_address_str, BLE_MAC_ADDRESS* mac_address)
{
    bool result;

    // mac address has to be in the format XX:XX:XX:XX:XX:XX where X is a value
    // in the range 0-F hex base
    if (strlen(mac_address_str) != 17)
    {
        result = false;
    }
    else
    {
        // taken from http://stackoverflow.com/a/20553913
        int values[6];
        char unexpected;
        if (sscanf(
                mac_address_str,
                "%x:%x:%x:%x:%x:%x%c",
                &values[0], &values[1],
                &values[2], &values[3],
                &values[4], &values[5], &unexpected) == 6)
        {
            for (size_t i = 0; i < 6; i++)
            {
                mac_address->address[i] = (uint8_t)values[i];
            }
            result = true;
        }
        else
        {
            result = false;
        }
    }

    return result;
}