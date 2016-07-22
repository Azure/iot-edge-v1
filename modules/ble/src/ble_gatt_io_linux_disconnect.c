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

typedef struct DISCONNECT_CONTEXT_TAG
{
    BLEIO_GATT_HANDLE_DATA*             handle_data;
    ON_BLEIO_GATT_DISCONNECT_COMPLETE   on_disconnect_complete;
    void*                               callback_context;
}DISCONNECT_CONTEXT;

// called when the device completes disconnection
static void on_device_disconnect(GObject* source_object, GAsyncResult* result, gpointer user_data);

void BLEIO_gatt_disconnect(
    BLEIO_GATT_HANDLE bleio_gatt_handle,
    ON_BLEIO_GATT_DISCONNECT_COMPLETE on_bleio_gatt_disconnect_complete,
    void* callback_context
)
{
    BLEIO_GATT_HANDLE_DATA* handle_data = (BLEIO_GATT_HANDLE_DATA*)bleio_gatt_handle;
    if (bleio_gatt_handle != NULL)
    {
        if (handle_data->state == BLEIO_GATT_STATE_CONNECTED)
        {
            /*Codes_SRS_BLEIO_GATT_13_054: [ BLEIO_gatt_disconnect shall do nothing if an underlying platform call fails. ]*/
            DISCONNECT_CONTEXT* context = (DISCONNECT_CONTEXT*)malloc(sizeof(DISCONNECT_CONTEXT));
            if (context != NULL)
            {
                context->handle_data = handle_data;
                context->on_disconnect_complete = on_bleio_gatt_disconnect_complete;
                context->callback_context = callback_context;

                /*Codes_SRS_BLEIO_GATT_13_014: [ BLEIO_gatt_disconnect shall asynchronously disconnect from the BLE device if an open connection exists. ]*/
                bluez_device__call_disconnect(
                    handle_data->device,
                    NULL,
                    on_device_disconnect,
                    context
                );
            }
            else
            {
                LogError("malloc failed.");
            }
        }
        else
        {
            /*Codes_SRS_BLEIO_GATT_13_015: [ BLEIO_gatt_disconnect shall do nothing if an open connection does not exist when it is called. ]*/
            LogError("Object is already disconnected from the device.");
        }
    }
    else
    {
        /*Codes_SRS_BLEIO_GATT_13_016: [ BLEIO_gatt_disconnect shall do nothing if bleio_gatt_handle is NULL. ]*/
        LogError("Invalid args.");
    }
}

static void on_device_disconnect(
    GObject* source_object,
    GAsyncResult* result,
    gpointer user_data
)
{
    DISCONNECT_CONTEXT* context = (DISCONNECT_CONTEXT*)user_data;
    bluez_device__call_disconnect_finish(context->handle_data->device, result, NULL);

    /*Codes_SRS_BLEIO_GATT_13_050: [ When the disconnect operation has been completed, the callback function pointed at by on_bleio_gatt_disconnect_complete shall be invoked if it is not NULL. ]*/
    /*Codes_SRS_BLEIO_GATT_13_049: [ When on_bleio_gatt_disconnect_complete is invoked the value passed in callback_context to BLEIO_gatt_disconnect shall be passed along to on_bleio_gatt_disconnect_complete. ]*/
    if(context->on_disconnect_complete != NULL)
    {
        context->on_disconnect_complete(
            (BLEIO_GATT_HANDLE)context->handle_data,
            context->callback_context
        );
    }

    free(context);
}
