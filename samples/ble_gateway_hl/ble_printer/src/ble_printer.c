// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include <glib.h>

#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/constmap.h"
#include "azure_c_shared_utility/constbuffer.h"

#include "module.h"
#include "message.h"
#include "message_bus.h"
#include "ble.h"
#include "ble_printer.h"
#include "messageproperties.h"

typedef void(*MESSAGE_PRINTER)(const char* name, const char* timestamp, const CONSTBUFFER* buffer);

static void print_string(const char* name, const char* timestamp, const CONSTBUFFER* buffer);
static void print_temperature(const char* name, const char* timestamp, const CONSTBUFFER* buffer);
static void print_default(const char* name, const char* timestamp, const CONSTBUFFER* buffer);

typedef struct DIPATCH_ENTRY_TAG
{
    const char* name;
    const char* characteristic_uuid;
    MESSAGE_PRINTER message_printer;
}DIPATCH_ENTRY;

// setup the message printers
DIPATCH_ENTRY g_dispatch_entries[] =
{
    {
        "Model Number",
        "00002A24-0000-1000-8000-00805F9B34FB",
        print_string
    },
    {
        "Serial Number",
        "00002A25-0000-1000-8000-00805F9B34FB",
        print_string
    },
    {
        "Serial Number",
        "00002A25-0000-1000-8000-00805F9B34FB",
        print_string
    },
    {
        "Firmware Revision Number",
        "00002A26-0000-1000-8000-00805F9B34FB",
        print_string
    },
    {
        "Hardware Revision Number",
        "00002A27-0000-1000-8000-00805F9B34FB",
        print_string
    },
    {
        "Software Revision Number",
        "00002A28-0000-1000-8000-00805F9B34FB",
        print_string
    },
    {
        "Manufacturer",
        "00002A29-0000-1000-8000-00805F9B34FB",
        print_string
    },
    {
        "Temperature",
        "F000AA01-0451-4000-B000-000000000000",
        print_temperature
    }
};

size_t g_dispatch_entries_length = sizeof(g_dispatch_entries) / sizeof(g_dispatch_entries[0]);

MODULE_HANDLE BLEPrinter_Create(MESSAGE_BUS_HANDLE bus, const void* configuration)
{
    return (MODULE_HANDLE)0x42;
}

void BLEPrinter_Receive(MODULE_HANDLE module, MESSAGE_HANDLE message)
{
    if (message != NULL)
    {
        CONSTMAP_HANDLE props = Message_GetProperties(message);
        if (props != NULL)
        {
            const char* source = ConstMap_GetValue(props, GW_SOURCE_PROPERTY);
            if (source != NULL && strcmp(source, GW_SOURCE_BLE_TELEMETRY) == 0)
            {
                const char* ble_controller_id = ConstMap_GetValue(props, GW_BLE_CONTROLLER_INDEX_PROPERTY);
                const char* mac_address_str = ConstMap_GetValue(props, GW_MAC_ADDRESS_PROPERTY);
                const char* timestamp = ConstMap_GetValue(props, GW_TIMESTAMP_PROPERTY);
                const char* characteristic_uuid = ConstMap_GetValue(props, GW_CHARACTERISTIC_UUID_PROPERTY);
                const CONSTBUFFER* buffer = Message_GetContent(message);
                if (buffer != NULL && characteristic_uuid != NULL)
                {
                    // dispatch the message based on the characteristic uuid
                    size_t i;
                    for (i = 0; i < g_dispatch_entries_length; i++)
                    {
                        if (g_ascii_strcasecmp(
                                characteristic_uuid,
                                g_dispatch_entries[i].characteristic_uuid
                            ) == 0)
                        {
                            g_dispatch_entries[i].message_printer(
                                g_dispatch_entries[i].name,
                                timestamp,
                                buffer
                            );
                            break;
                        }
                    }

                    if (i == g_dispatch_entries_length)
                    {
                        // dispatch to default printer
                        print_default(characteristic_uuid, timestamp, buffer);
                    }
                }
                else
                {
                    LogError("Message is invalid. Nothing to print.");
                }
            }
        }
        else
        {
            LogError("Message_GetProperties for the message returned NULL");
        }
    }
    else
    {
        LogError("message is NULL");
    }
}

void BLEPrinter_Destroy(MODULE_HANDLE module)
{
    // Nothing to do here
}

static const MODULE_APIS Module_GetAPIS_Impl =
{
    BLEPrinter_Create,
    BLEPrinter_Destroy,
    BLEPrinter_Receive
};

const MODULE_APIS* Module_GetAPIS(void)
{
    return &Module_GetAPIS_Impl;
}

static void print_string(const char* name, const char* timestamp, const CONSTBUFFER* buffer)
{
    printf("[%s] %s: %.*s\r\n", timestamp, name, (int)buffer->size, buffer->buffer);
}

/**
 * Code taken from:
 *    http://processors.wiki.ti.com/index.php/CC2650_SensorTag_User's_Guide#Data
 */
static void sensortag_temp_convert(
    uint16_t rawAmbTemp,
    uint16_t rawObjTemp,
    float *tAmb,
    float *tObj
)
{
    const float SCALE_LSB = 0.03125;
    float t;
    int it;

    it = (int)((rawObjTemp) >> 2);
    t = ((float)(it)) * SCALE_LSB;
    *tObj = t;

    it = (int)((rawAmbTemp) >> 2);
    t = (float)it;
    *tAmb = t * SCALE_LSB;
}

static void print_temperature(const char* name, const char* timestamp, const CONSTBUFFER* buffer)
{
    if (buffer->size == 4)
    {
        uint16_t* temps = (uint16_t *)buffer->buffer;
        float ambient, object;
        sensortag_temp_convert(temps[0], temps[1], &ambient, &object);
        printf("[%s] %s: (%f, %f)\r\n",
            timestamp,
            name,
            ambient,
            object
        );
    }
}

static void print_default(const char* name, const char* timestamp, const CONSTBUFFER* buffer)
{
    printf("[%s] %s: ", timestamp, name);
    for (size_t i = 0; i < buffer->size; ++i)
    {
        printf("%02X ", buffer->buffer[i]);
    }
    printf("\r\n");
}