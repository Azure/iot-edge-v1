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

typedef struct WRITE_CONTEXT_TAG
{
    BLEIO_GATT_HANDLE_DATA*             handle_data;
    GString*                            object_path;
    bluezcharacteristic*                characteristic;
    GBytes*                             data;
    GIO_ASYNCSEQ_HANDLE                 async_seq;
    ON_BLEIO_GATT_ATTRIB_WRITE_COMPLETE on_write_complete;
    void*                               callback_context;
}WRITE_CONTEXT;

// called when an error occurs in an async call
static void on_sequence_error(GIO_ASYNCSEQ_HANDLE async_seq_handle, const GError* error);

// called when the entire async sequence completes
static void on_sequence_complete(GIO_ASYNCSEQ_HANDLE async_seq_handle, gpointer previous_result);

// async sequence functions
static void create_characteristic(GIO_ASYNCSEQ_HANDLE async_seq_handle, gpointer previous_result, gpointer callback_context, GAsyncReadyCallback async_callback);
static gpointer create_characteristic_finish(GIO_ASYNCSEQ_HANDLE async_seq_handle, GAsyncResult* result, GError** error);

static void write_characteristic(GIO_ASYNCSEQ_HANDLE async_seq_handle, gpointer previous_result, gpointer callback_context, GAsyncReadyCallback async_callback);
static gpointer write_characteristic_finish(GIO_ASYNCSEQ_HANDLE async_seq_handle, GAsyncResult* result, GError** error);

// utilities
static void free_context(WRITE_CONTEXT* context);

int BLEIO_gatt_write_char_by_uuid(
    BLEIO_GATT_HANDLE bleio_gatt_handle,
    const char* ble_uuid,
    const unsigned char* buffer,
    size_t size,
    ON_BLEIO_GATT_ATTRIB_WRITE_COMPLETE on_bleio_gatt_attrib_write_complete,
    void* callback_context
)
{
    int result;
    BLEIO_GATT_HANDLE_DATA* handle_data = (BLEIO_GATT_HANDLE_DATA*)bleio_gatt_handle;

    /*Codes_SRS_BLEIO_GATT_13_036: [ BLEIO_gatt_write_char_by_uuid shall return a non-zero value if bleio_gatt_handle is NULL. ]*/
    /*Codes_SRS_BLEIO_GATT_13_045: [ BLEIO_gatt_write_char_by_uuid shall return a non-zero value if buffer is NULL. ]*/
    /*Codes_SRS_BLEIO_GATT_13_048: [ BLEIO_gatt_write_char_by_uuid shall return a non-zero value if ble_uuid is NULL. ]*/
    /*Codes_SRS_BLEIO_GATT_13_046: [ BLEIO_gatt_write_char_by_uuid shall return a non-zero value if size is equal to 0 (zero). ]*/
    /*Codes_SRS_BLEIO_GATT_13_037: [ BLEIO_gatt_write_char_by_uuid shall return a non-zero value if on_bleio_gatt_attrib_write_complete is NULL. ]*/
    /*Codes_SRS_BLEIO_GATT_13_053: [ BLEIO_gatt_write_char_by_uuid shall return a non-zero value if an active connection to the device does not exist. ]*/
    if (
            bleio_gatt_handle != NULL &&
            ble_uuid != NULL &&
            on_bleio_gatt_attrib_write_complete != NULL &&
            buffer != NULL &&
            size > 0 &&
            handle_data->state == BLEIO_GATT_STATE_CONNECTED
       )
    {
        GString* uuid = g_string_new(ble_uuid);
        if(uuid != NULL)
        {
            GString* object_path = g_tree_lookup(handle_data->char_object_path_map, uuid);
            if(object_path != NULL)
            {
                // now that we have the object path we don't need the UUID anymore
                g_string_free(uuid, TRUE);

                WRITE_CONTEXT *context = (WRITE_CONTEXT *)malloc(sizeof(WRITE_CONTEXT));
                if (context != NULL)
                {
                    context->data = g_bytes_new(buffer, size);
                    if(context->data != NULL)
                    {
                        // create async sequence
                        context->async_seq = GIO_Async_Seq_Create(
                            context,
                            on_sequence_error,
                            on_sequence_complete
                        );
                        if (context->async_seq != NULL)
                        {
                            // setup call sequence
                            GIO_ASYNCSEQ_RESULT seq_result = GIO_Async_Seq_Add(
                                context->async_seq, NULL,

                                // create an instance of the characteristic
                                create_characteristic, create_characteristic_finish,

                                // write the characteristic
                                write_characteristic, write_characteristic_finish,

                                // sentinel value to signal end of sequence
                                NULL
                            );
                            if (seq_result == GIO_ASYNCSEQ_OK)
                            {
                                context->handle_data = handle_data;
                                context->object_path = object_path;
                                context->characteristic = NULL;
                                context->on_write_complete = on_bleio_gatt_attrib_write_complete;
                                context->callback_context = callback_context;

                                /*Codes_SRS_BLEIO_GATT_13_040: [ BLEIO_gatt_write_char_by_uuid shall asynchronously initiate a write characteristic operation using the specified UUID. ]*/
                                // kick-off the sequence
                                if(GIO_Async_Seq_Run_Async(context->async_seq) == GIO_ASYNCSEQ_OK)
                                {
                                    /*Codes_SRS_BLEIO_GATT_13_039: [ BLEIO_gatt_write_char_by_uuid shall return 0 (zero) if the write characteristic operation is successful. ]*/
                                    result = 0;
                                }
                                else
                                {
                                    result = __LINE__;
                                    GIO_Async_Seq_Destroy(context->async_seq);
                                    g_bytes_unref(context->data);
                                    free(context);
                                    LogError("GIO_Async_Seq_Run_Async failed.");
                                }
                            }
                            else
                            {
                                /*Codes_SRS_BLEIO_GATT_13_038: [ BLEIO_gatt_write_char_by_uuid shall return a non-zero value if an underlying platform call fails. ]*/
                                result = __LINE__;
                                GIO_Async_Seq_Destroy(context->async_seq);
                                g_bytes_unref(context->data);
                                free(context);
                                LogError("GIO_Async_Seq_Add failed.");
                            }
                        }
                        else
                        {
                            /*Codes_SRS_BLEIO_GATT_13_038: [ BLEIO_gatt_write_char_by_uuid shall return a non-zero value if an underlying platform call fails. ]*/
                            result = __LINE__;
                            g_bytes_unref(context->data);
                            free(context);
                            LogError("GIO_Async_Seq_Create failed.");
                        }
                    }
                    else
                    {
                        /*Codes_SRS_BLEIO_GATT_13_038: [ BLEIO_gatt_write_char_by_uuid shall return a non-zero value if an underlying platform call fails. ]*/
                        result = __LINE__;
                        free(context);
                        LogError("g_byte_array_sized_new failed.");
                    }
                }
                else
                {
                    /*Codes_SRS_BLEIO_GATT_13_038: [ BLEIO_gatt_write_char_by_uuid shall return a non-zero value if an underlying platform call fails. ]*/
                    result = __LINE__;
                    LogError("malloc failed.");
                }
            }
            else
            {
                /*Codes_SRS_BLEIO_GATT_13_038: [ BLEIO_gatt_write_char_by_uuid shall return a non-zero value if an underlying platform call fails. ]*/
                result = __LINE__;
                g_string_free(uuid, TRUE);
                LogError("g_tree_lookup() failed.");
            }
        }
        else
        {
            /*Codes_SRS_BLEIO_GATT_13_038: [ BLEIO_gatt_write_char_by_uuid shall return a non-zero value if an underlying platform call fails. ]*/
            result = __LINE__;
            LogError("g_string_new() failed.");
        }
    }
    else
    {
        result = __LINE__;
        LogError("Invalid args or the state of the object is unexpected.");
    }

    return result;
}

static void create_characteristic(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    gpointer previous_result,
    gpointer callback_context,
    GAsyncReadyCallback async_callback
)
{
    WRITE_CONTEXT* context = (WRITE_CONTEXT*)GIO_Async_Seq_GetContext(async_seq_handle);
    bluez_characteristic__proxy_new(
        context->handle_data->bus,
        G_DBUS_PROXY_FLAGS_NONE,
        "org.bluez",
        context->object_path->str,
        NULL,
        async_callback,
        async_seq_handle
    );
}

static gpointer create_characteristic_finish(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    GAsyncResult* result,
    GError** error
)
{
    return bluez_characteristic__proxy_new_finish(result, error);
}

static void write_characteristic(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    gpointer previous_result,
    gpointer callback_context,
    GAsyncReadyCallback async_callback
)
{
    WRITE_CONTEXT* context = (WRITE_CONTEXT*)GIO_Async_Seq_GetContext(async_seq_handle);
    context->characteristic = (bluezcharacteristic*)previous_result;

    // we don't check if this fails; d-bus will raise an error if no params
    // are passed (which is what NULL would mean) for the "WriteValue"
    // method when it expects one
    gsize size_data;
    gconstpointer data = g_bytes_get_data(context->data, &size_data);
    GVariant* var = g_variant_new_fixed_array(
        G_VARIANT_TYPE_BYTE,
        data,
        size_data,
        sizeof(unsigned char)
    );

    g_dbus_proxy_call(
        G_DBUS_PROXY(context->characteristic),
        "WriteValue",
        g_variant_new_tuple(&var, 1),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        async_callback,
        async_seq_handle
    );
}

static gpointer write_characteristic_finish(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    GAsyncResult* result,
    GError** error
)
{
    WRITE_CONTEXT* context = (WRITE_CONTEXT*)GIO_Async_Seq_GetContext(async_seq_handle);
    
    gboolean ret = bluez_characteristic__call_write_value_finish(
        context->characteristic,
        result,
        error
    );
    
    return (gpointer)(uintptr_t)ret;
}

static void on_sequence_complete(GIO_ASYNCSEQ_HANDLE async_seq_handle, gpointer previous_result)
{
    WRITE_CONTEXT* context = (WRITE_CONTEXT*)GIO_Async_Seq_GetContext(async_seq_handle);

    /*Codes_SRS_BLEIO_GATT_13_041: [ BLEIO_gatt_write_char_by_uuid shall invoke on_bleio_gatt_attrib_write_complete when the write operation completes. ]*/
    /*Codes_SRS_BLEIO_GATT_13_042: [ BLEIO_gatt_write_char_by_uuid shall pass the value of the callback_context parameter to on_bleio_gatt_attrib_write_complete as the context parameter when it is invoked. ]*/
    /*Codes_SRS_BLEIO_GATT_13_043: [ BLEIO_gatt_write_char_by_uuid, when successful, shall supply the value BLEIO_GATT_OK for the result parameter. ]*/
    context->on_write_complete(
        (BLEIO_GATT_HANDLE)context->handle_data,
        context->callback_context,
        BLEIO_GATT_OK
    );

    free_context(context);
}

static void on_sequence_error(GIO_ASYNCSEQ_HANDLE async_seq_handle, const GError* error)
{
    WRITE_CONTEXT* context = (WRITE_CONTEXT*)GIO_Async_Seq_GetContext(async_seq_handle);

    if (error != NULL)
    {
        LogError("Write characteristic failed with - %s", error->message);
    }

    /*Codes_SRS_BLEIO_GATT_13_041: [ BLEIO_gatt_write_char_by_uuid shall invoke on_bleio_gatt_attrib_write_complete when the write operation completes. ]*/
    /*Codes_SRS_BLEIO_GATT_13_042: [ BLEIO_gatt_write_char_by_uuid shall pass the value of the callback_context parameter to on_bleio_gatt_attrib_write_complete as the context parameter when it is invoked. ]*/
    /*Codes_SRS_BLEIO_GATT_13_044: [ When an error occurs asynchronously, the value BLEIO_GATT_ERROR shall be passed for the result parameter of the on_bleio_gatt_attrib_write_complete callback. ]*/
    context->on_write_complete(
        (BLEIO_GATT_HANDLE)context->handle_data,
        context->callback_context,
        BLEIO_GATT_ERROR
    );

    free_context(context);
}

static void free_context(WRITE_CONTEXT* context)
{
    // we don't free context->object_path because that string lives in
    // the uuid->object_path map in the BLEIO_GATT_HANDLE and will be
    // reused for future reads

    if (context->characteristic != NULL)
    {
        g_object_unref(context->characteristic);
    }

    if (context->data != NULL)
    {
        g_bytes_unref(context->data);
    }

    GIO_Async_Seq_Destroy(context->async_seq);
    free(context);
}

