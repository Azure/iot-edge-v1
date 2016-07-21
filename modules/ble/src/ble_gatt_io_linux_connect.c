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

typedef struct CONNECT_CONTEXT_TAG
{
    BLEIO_GATT_HANDLE_DATA*         handle_data;
    GString*                        device_path;
    GIO_ASYNCSEQ_HANDLE             async_seq;
    ON_BLEIO_GATT_CONNECT_COMPLETE  on_connect_complete;
    void*                           callback_context;
}CONNECT_CONTEXT;

// called when an error occurs in an async call
static void on_sequence_error(GIO_ASYNCSEQ_HANDLE async_seq_handle, const GError* error);

// called when the entire async sequence completes
static void on_sequence_complete(GIO_ASYNCSEQ_HANDLE async_seq_handle, gpointer previous_result);

// async sequence functions
static void connect_system_bus(GIO_ASYNCSEQ_HANDLE async_seq_handle, gpointer previous_result, gpointer callback_context, GAsyncReadyCallback async_callback);
static gpointer connect_system_bus_finish(GIO_ASYNCSEQ_HANDLE async_seq_handle, GAsyncResult* result, GError** error);

static void create_object_manager(GIO_ASYNCSEQ_HANDLE async_seq_handle, gpointer previous_result, gpointer callback_context, GAsyncReadyCallback async_callback);
static gpointer create_object_manager_finish(GIO_ASYNCSEQ_HANDLE async_seq_handle, GAsyncResult* result, GError** error);

static void create_device_proxy(GIO_ASYNCSEQ_HANDLE async_seq_handle, gpointer previous_result, gpointer callback_context, GAsyncReadyCallback async_callback);
static gpointer create_device_proxy_finish(GIO_ASYNCSEQ_HANDLE async_seq_handle, GAsyncResult* result, GError** error);

static void connect_device(GIO_ASYNCSEQ_HANDLE async_seq_handle, gpointer previous_result, gpointer callback_context, GAsyncReadyCallback async_callback);
static gpointer connect_device_finish(GIO_ASYNCSEQ_HANDLE async_seq_handle, GAsyncResult* result, GError** error);

// utilities
static void free_context(CONNECT_CONTEXT* context);

// The connect API essentially performs the following asynchronous
// operations in sequence. There has to be an active GLIB loop
// running in order for all this to work.
//
//  [*] Get a connection to the system d-bus
//  [*] Get a reference to the d-bus object manager for the
//      org.bluez root object
//  [*] Get a device proxy object for the given mac address
//  [*] Open a connection to the device
//  [*] Load up all characteristics on the device and map
//      d-bus object paths to GATT characteristic UUIDs

int BLEIO_gatt_connect(
    BLEIO_GATT_HANDLE bleio_gatt_handle,
    ON_BLEIO_GATT_CONNECT_COMPLETE on_bleio_gatt_connect_complete,
    void* callback_context
)
{
    int result;
    BLEIO_GATT_HANDLE_DATA* handle_data = (BLEIO_GATT_HANDLE_DATA*)bleio_gatt_handle;

    /*Codes_SRS_BLEIO_GATT_13_047: [ If when BLEIO_gatt_connect is called, there's another connection request already in progress or if an open connection already exists, then this API shall return a non-zero error code. ]*/
    if (
            bleio_gatt_handle != NULL &&
            on_bleio_gatt_connect_complete != NULL &&
            (
                handle_data->state == BLEIO_GATT_STATE_DISCONNECTED ||
                handle_data->state == BLEIO_GATT_STATE_ERROR
            )
       )
    {
        CONNECT_CONTEXT *context = (CONNECT_CONTEXT *)malloc(sizeof(CONNECT_CONTEXT));
        if (context == NULL)
        {
            /*Codes_SRS_BLEIO_GATT_13_009: [ If any of the underlying platform calls fail, BLEIO_gatt_connect shall return a non-zero value. ]*/
            result = __LINE__;
            LogError("malloc failed.");
        }
        else
        {
            // create async sequence
            context->async_seq = GIO_Async_Seq_Create(
                context,
                on_sequence_error,
                on_sequence_complete
            );
            if (context->async_seq != NULL)
            {
                /*Codes_SRS_BLEIO_GATT_13_007: [ BLEIO_gatt_connect shall asynchronously attempt to open a connection with the BLE device. ]*/
                // setup call sequence
                GIO_ASYNCSEQ_RESULT seq_result = GIO_Async_Seq_Add(
                    context->async_seq, NULL,

                    // connect to the system bus
                    connect_system_bus, connect_system_bus_finish,

                    // create an instance of the d-bus object manager
                    create_object_manager, create_object_manager_finish,

                    // create an instance of a device proxy object
                    create_device_proxy, create_device_proxy_finish,

                    // connect to the device
                    connect_device, connect_device_finish,

                    // sentinel value to signal end of sequence
                    NULL
                );
                if (seq_result == GIO_ASYNCSEQ_OK)
                {
                    context->device_path = NULL;
                    context->handle_data = handle_data;
                    context->on_connect_complete = on_bleio_gatt_connect_complete;

                    /*Codes_SRS_BLEIO_GATT_13_012: [ When on_bleio_gatt_connect_complete is invoked the value passed in callback_context to BLEIO_gatt_connect shall be passed along to on_bleio_gatt_connect_complete. ]*/
                    context->callback_context = callback_context;

                    // kick-off the sequence
                    handle_data->state = BLEIO_GATT_STATE_CONNECTING;
                    if(GIO_Async_Seq_Run_Async(context->async_seq) == GIO_ASYNCSEQ_OK)
                    {
                        /*Codes_SRS_BLEIO_GATT_13_008: [ On initiating the connect successfully, BLEIO_gatt_connect shall return 0 (zero). ]*/
                        result = 0;
                    }
                    else
                    {
                        result = __LINE__;
                        GIO_Async_Seq_Destroy(context->async_seq);
                        free(context);
                        LogError("GIO_Async_Seq_Run failed.");
                    }
                }
                else
                {
                    /*Codes_SRS_BLEIO_GATT_13_009: [ If any of the underlying platform calls fail, BLEIO_gatt_connect shall return a non-zero value. ]*/
                    result = __LINE__;
                    GIO_Async_Seq_Destroy(context->async_seq);
                    free(context);
                    LogError("GIO_Async_Seq_Add failed.");
                }
            }
            else
            {
                /*Codes_SRS_BLEIO_GATT_13_009: [ If any of the underlying platform calls fail, BLEIO_gatt_connect shall return a non-zero value. ]*/
                result = __LINE__;
                free(context);
                LogError("GIO_Async_Seq_Create failed.");
            }
        }
    }
    else
    {
        /*Codes_SRS_BLEIO_GATT_13_010: [ If bleio_gatt_handle or on_bleio_gatt_connect_complete are NULL then BLEIO_gatt_connect shall return a non-zero value. ]*/
        result = __LINE__;
        LogError("Invalid args or the state of the object is unexpected.");
    }

    return result;
}

static void connect_system_bus(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    gpointer previous_result,
    gpointer callback_context,
    GAsyncReadyCallback async_callback
)
{
    g_bus_get(
        G_BUS_TYPE_SYSTEM,
        NULL,               // not cancellable
        async_callback,
        async_seq_handle
    );
}

static gpointer connect_system_bus_finish(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    GAsyncResult* result,
    GError** error
)
{
    return g_bus_get_finish(result, error);
}

static void create_object_manager(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    gpointer previous_result,
    gpointer callback_context,
    GAsyncReadyCallback async_callback
)
{
    CONNECT_CONTEXT* context = (CONNECT_CONTEXT*)GIO_Async_Seq_GetContext(async_seq_handle);
    context->handle_data->bus = (GDBusConnection*)previous_result;

    g_dbus_object_manager_client_new(
        context->handle_data->bus,
        G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
        "org.bluez",
        "/",
        NULL, NULL, NULL, // no custom proxies
        NULL,             // not cancellable
        async_callback,
        async_seq_handle
    );
}

static gpointer create_object_manager_finish(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    GAsyncResult* result,
    GError** error
)
{
    return g_dbus_object_manager_client_new_finish(result, error);
}

static void create_device_proxy(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    gpointer previous_result,
    gpointer callback_context,
    GAsyncReadyCallback async_callback
)
{
    CONNECT_CONTEXT* context = (CONNECT_CONTEXT*)GIO_Async_Seq_GetContext(async_seq_handle);
    context->handle_data->object_manager = (GDBusObjectManagerClient*)previous_result;

    // convert BLE MAC address to an object path
    context->device_path = g_string_new(NULL);
    g_string_printf(
        context->device_path,
        "/org/bluez/hci%d/dev_%02X_%02X_%02X_%02X_%02X_%02X",
        context->handle_data->ble_controller_index,
        context->handle_data->device_addr.address[0],
        context->handle_data->device_addr.address[1],
        context->handle_data->device_addr.address[2],
        context->handle_data->device_addr.address[3],
        context->handle_data->device_addr.address[4],
        context->handle_data->device_addr.address[5]
    );

    bluez_device__proxy_new(
        context->handle_data->bus,
        G_DBUS_PROXY_FLAGS_NONE,
        "org.bluez",
        context->device_path->str,
        NULL,                                       // not cancellable
        async_callback,
        async_seq_handle
    );
}

static gpointer create_device_proxy_finish(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    GAsyncResult* result,
    GError** error
)
{
    return bluez_device__proxy_new_finish(result, error);
}

static void connect_device(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    gpointer previous_result,
    gpointer callback_context,
    GAsyncReadyCallback async_callback
)
{
    CONNECT_CONTEXT* context = (CONNECT_CONTEXT*)GIO_Async_Seq_GetContext(async_seq_handle);
    context->handle_data->device = (bluezdevice*)previous_result;

    /*Codes_SRS_BLEIO_GATT_13_007: [ BLEIO_gatt_connect shall asynchronously attempt to open a connection with the BLE device. ]*/
    bluez_device__call_connect(
        context->handle_data->device,
        NULL,
        async_callback,
        async_seq_handle
    );
}

static gpointer connect_device_finish(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    GAsyncResult* result,
    GError** error
)
{
    CONNECT_CONTEXT* context = (CONNECT_CONTEXT*)GIO_Async_Seq_GetContext(async_seq_handle);
    return (gpointer)(uintptr_t)bluez_device__call_connect_finish(
        context->handle_data->device, result, error
    );
}

static gint is_interface_type(gconstpointer data, gconstpointer user_data)
{
    GDBusProxy* proxy = (GDBusProxy*)data;
    const gchar* interface_name = g_dbus_proxy_get_interface_name(proxy);
    return g_strcmp0(interface_name, (const gchar*)user_data);
}

static void unref_object(gpointer data)
{
    g_object_unref(data);
}

static void unref_list_object(gpointer data, gpointer user_data)
{
    unref_object(data);
}

static GString* get_characteristic_uuid(GDBusObject* object)
{
    // 'object' is a GATT characteristic if one of the interfaces
    // that this object supports is 'org.bluez.GattCharacteristic1'

    GString* result;
    GList* interfaces = g_dbus_object_get_interfaces(object);
    if (interfaces != NULL)
    {
        GList* characteristic_item = g_list_find_custom(
            interfaces,
            (gconstpointer)"org.bluez.GattCharacteristic1",
            is_interface_type
        );
        if (characteristic_item != NULL)
        {
            GVariant* var = g_dbus_proxy_get_cached_property(
                (GDBusProxy*)(characteristic_item->data),
                "UUID"
            );
            if (var != NULL)
            {
                result = g_string_new(g_variant_get_string(var, NULL));
                g_variant_unref(var);
            }
            else
            {
                result = NULL;
            }
        }
        else
        {
            result = NULL;
        }

        g_list_foreach(interfaces, unref_list_object, NULL);
        g_list_free(interfaces);
    }
    else
    {
        result = NULL;
    }

    return result;
}

static void process_object(gpointer data, gpointer user_data)
{
    CONNECT_CONTEXT* context = (CONNECT_CONTEXT*)user_data;
    GDBusObject* object = (GDBusObject*)data;

    // get device and object paths
    const gchar* device_path = g_dbus_proxy_get_object_path(
        (GDBusProxy*)context->handle_data->device
    );
    const gchar* object_path = g_dbus_object_get_object_path(object);

    // if "device_path" is a prefix of "object_path" then
    // "object" is a child of "device"
    if (
            device_path != NULL &&
            object_path != NULL &&
            g_strcmp0(object_path, device_path) != 0 &&
            g_str_has_prefix(object_path, device_path) == TRUE
       )
    {
        // add UUID of object to map if this is a characteristic
        GString* uuid = get_characteristic_uuid(object);
        if(uuid != NULL)
        {
            GString* opath = g_string_new(object_path);
            if(opath != NULL)
            {
                g_tree_insert(
                    context->handle_data->char_object_path_map,
                    uuid,
                    opath
                );
            }
            else
            {
                g_string_free(uuid, TRUE);
            }
        }
    }

    g_object_unref(object);
}

static void on_sequence_complete(GIO_ASYNCSEQ_HANDLE async_seq_handle, gpointer previous_result)
{
    CONNECT_CONTEXT* context = (CONNECT_CONTEXT*)GIO_Async_Seq_GetContext(async_seq_handle);
    context->handle_data->state = BLEIO_GATT_STATE_CONNECTED;

    // fetch list of objects on the object manager
    GList* objects_list = g_dbus_object_manager_get_objects(
        (GDBusObjectManager*)context->handle_data->object_manager
    );
    if (objects_list == NULL)
    {
        on_sequence_error(async_seq_handle, NULL);
    }
    else
    {
        // iterate through each object and add each characteristic
        // that is a child of the device object path to the map
        g_list_foreach(objects_list, process_object, context);
        g_list_free(objects_list);

        /*Codes_SRS_BLEIO_GATT_13_011: [ When the connect operation to the device has been completed, the callback function pointed at by on_bleio_gatt_connect_complete shall be invoked. ]*/
        /*Codes_SRS_BLEIO_GATT_13_012: [ When on_bleio_gatt_connect_complete is invoked the value passed in callback_context to BLEIO_gatt_connect shall be passed along to on_bleio_gatt_connect_complete. ]*/
        /*Codes_SRS_BLEIO_GATT_13_013: [ The connect_result parameter of the on_bleio_gatt_connect_complete callback shall indicate the status of the connect operation. ]*/
        // invoke the user's callback
        context->on_connect_complete(
            (BLEIO_GATT_HANDLE)context->handle_data,
            context->callback_context,
            BLEIO_GATT_CONNECT_OK
        );

        free_context(context);
    }
}

static void on_sequence_error(GIO_ASYNCSEQ_HANDLE async_seq_handle, const GError* error)
{
    LogError("Connect failed with - %s",
        (error != NULL && error->message != NULL) ? error->message : "[unknown error]");

    CONNECT_CONTEXT* context = (CONNECT_CONTEXT*)GIO_Async_Seq_GetContext(async_seq_handle);
    context->handle_data->state = BLEIO_GATT_STATE_ERROR;

    /*Codes_SRS_BLEIO_GATT_13_011: [ When the connect operation to the device has been completed, the callback function pointed at by on_bleio_gatt_connect_complete shall be invoked. ]*/
    /*Codes_SRS_BLEIO_GATT_13_012: [ When on_bleio_gatt_connect_complete is invoked the value passed in callback_context to BLEIO_gatt_connect shall be passed along to on_bleio_gatt_connect_complete. ]*/
    /*Codes_SRS_BLEIO_GATT_13_013: [ The connect_result parameter of the on_bleio_gatt_connect_complete callback shall indicate the status of the connect operation. ]*/
    // invoke the user's callback with an error status
    context->on_connect_complete(
        (BLEIO_GATT_HANDLE)context->handle_data,
        context->callback_context,
        BLEIO_GATT_CONNECT_ERROR
    );
    free_context(context);
}

static void free_context(CONNECT_CONTEXT* context)
{
    if (context->device_path != NULL)
    {
        g_string_free(context->device_path, TRUE);
    }

    GIO_Async_Seq_Destroy(context->async_seq);
    free(context);
}
