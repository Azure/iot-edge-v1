// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <glib.h>
#include <gio/gio.h>

#include "bluez_device.h"
#include "bluez_characteristic.h"

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"

#include "gio_async_seq.h"
#include "ble_gatt_io.h"
#include "ble_gatt_io_linux_common.h"

static gint g_string_cmp(gconstpointer s1, gconstpointer s2, gpointer user_data);
static void tree_key_value_destroy(gpointer data);

BLEIO_GATT_HANDLE BLEIO_gatt_create(
    const BLE_DEVICE_CONFIG* config
)
{
    BLEIO_GATT_HANDLE_DATA* result;
    if (config != NULL)
    {
        result = malloc(sizeof(BLEIO_GATT_HANDLE_DATA));
        if (result != NULL)
        {
            // create a map that maps characteristic UUIDs to
            // d-bus object maps; both the key and value are
            // expected to be GString strings
            result->char_object_path_map = g_tree_new_full(
                g_string_cmp,
                NULL,
                tree_key_value_destroy,
                tree_key_value_destroy
            );
            if(result->char_object_path_map != NULL)
            {
                // save the device's MAC address
                memcpy(
                    &(result->device_addr),
                    &(config->device_addr),
                    sizeof(BLE_MAC_ADDRESS)
                );
                result->bus = NULL;
                result->object_manager = NULL;
                result->device = NULL;
                result->state = BLEIO_GATT_STATE_DISCONNECTED;
                result->ble_controller_index = config->ble_controller_index;

                /*Codes_SRS_BLEIO_GATT_13_001: [ BLEIO_gatt_create shall return a non-NULL handle on successful execution. ]*/
            }
            else
            {
                LogError("g_tree_new failed");
                free(result);
                result = NULL;
            }
        }
        else
        {
            /*Codes_SRS_BLEIO_GATT_13_002: [ BLEIO_gatt_create shall return NULL when any of the underlying platform calls fail. ]*/
            LogError("malloc failed");
            /* Fall through. 'result' is NULL. */
        }
    }
    else
    {
        /* Codes_SRS_BLEIO_GATT_13_003: [ BLEIO_gatt_create shall return NULL if config is NULL. ] */
        LogError("config is NULL");
        result = NULL;
    }

    return (BLEIO_GATT_HANDLE)result;
}

void BLEIO_gatt_destroy(
    BLEIO_GATT_HANDLE bleio_gatt_handle
)
{
    /*Codes_SRS_BLEIO_GATT_13_005: [ If bleio_gatt_handle is NULL BLEIO_gatt_destroy shall do nothing. ]*/
    if (bleio_gatt_handle != NULL)
    {
        /*Codes_SRS_BLEIO_GATT_13_004: [ BLEIO_gatt_destroy shall free all resources associated with the handle. ]*/
        BLEIO_GATT_HANDLE_DATA* handle_data = (BLEIO_GATT_HANDLE_DATA*)bleio_gatt_handle;

        if (handle_data->bus != NULL)
        {
            g_object_unref(handle_data->bus);
        }
        if (handle_data->object_manager != NULL)
        {
            g_object_unref(handle_data->object_manager);
        }
        if (handle_data->device != NULL)
        {
            g_object_unref(handle_data->device);
        }
        if (handle_data->char_object_path_map != NULL)
        {
            g_tree_unref(handle_data->char_object_path_map);
        }

        free(handle_data);
    }
}

static gint g_string_cmp(gconstpointer s1, gconstpointer s2, gpointer user_data)
{
    (void)user_data;

    gint result;

    if (s1 == NULL && s2 != NULL)
    {
        result = -1;
    }
    else if (s2 == NULL && s1 != NULL)
    {
        result = 1;
    }
    else if (s1 == NULL && s2 == NULL)
    {
        result = 0;
    }
    else
    {
        GString* gs1 = (GString*)s1;
        GString* gs2 = (GString*)s2;

        result = g_ascii_strncasecmp
        (
            gs1->str,
            gs2->str,
            gs1->len < gs2->len ? gs1->len : gs2->len
        );
    }

    return result;
}

static void tree_key_value_destroy(gpointer data)
{
    if (data != NULL)
    {
        g_string_free((GString*)data, TRUE);
    }
}
