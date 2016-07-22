// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#if __linux__
#include <glib.h>
#endif

#include <string.h>

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/gb_time.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/map.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/threadapi.h"

#include "module.h"
#include "message.h"
#include "message_bus.h"
#include "ble_gatt_io.h"
#include "bleio_seq.h"
#include "messageproperties.h"
#include "ble_utils.h"
#include "ble.h"

DEFINE_ENUM_STRINGS(BLEIO_SEQ_INSTRUCTION_TYPE, BLEIO_SEQ_INSTRUCTION_TYPE_VALUES);

typedef struct BLE_HANDLE_DATA_TAG
{
    MESSAGE_BUS_HANDLE  bus;
    BLE_DEVICE_CONFIG   device_config;
    BLEIO_GATT_HANDLE   bleio_gatt;
    BLEIO_SEQ_HANDLE    bleio_seq;
    bool                is_connected;
    bool                is_destroy_complete;
#if __linux__
    GMainLoop*          main_loop;
    THREAD_HANDLE       event_thread;
#endif
}BLE_HANDLE_DATA;

// how long to wait for a destroy complete callback to be invoked
// in microseconds
#define DESTROY_COMPLETE_TIMEOUT    (1000000 * 5)

static void on_connect_complete(
    BLEIO_GATT_HANDLE bleio_gatt_handle,
    void* context,
    BLEIO_GATT_CONNECT_RESULT connect_result
);

static void on_disconnect_complete(
    BLEIO_GATT_HANDLE bleio_gatt_handle,
    void* context
);

static void on_read_complete(
    BLEIO_SEQ_HANDLE bleio_seq_handle,
    void* context,
    const char* characteristic_uuid,
    BLEIO_SEQ_INSTRUCTION_TYPE type,
    BLEIO_SEQ_RESULT result,
    BUFFER_HANDLE data
);

static void on_write_complete(
    BLEIO_SEQ_HANDLE bleio_seq_handle,
    void* context,
    const char* characteristic_uuid,
    BLEIO_SEQ_INSTRUCTION_TYPE type,
    BLEIO_SEQ_RESULT result
);

static VECTOR_HANDLE ble_instr_to_bleioseq_instr(
    BLE_HANDLE_DATA* module,
    VECTOR_HANDLE source_instructions
);

#if __linux__
// how long to wait for a event dispatcher thread to start up
#define EVENT_DISPATCHER_START_TIMEOUT    (G_USEC_PER_SEC * 5)

static bool init_glib_loop(
    BLE_HANDLE_DATA* handle_data
);

static int event_dispatcher(
    void * user_data
);

static bool terminate_event_dispatcher(
    BLE_HANDLE_DATA* handle_data
);
#endif

static MODULE_HANDLE BLE_Create(MESSAGE_BUS_HANDLE bus, const void* configuration)
{
    BLE_HANDLE_DATA* result;
    BLE_CONFIG* config = (BLE_CONFIG*)configuration;

    /*Codes_SRS_BLE_13_001: [BLE_Create shall return NULL if bus is NULL.]*/
    /*Codes_SRS_BLE_13_002: [BLE_Create shall return NULL if configuration is NULL.]*/
    /*Codes_SRS_BLE_13_003: [BLE_Create shall return NULL if configuration->instructions is NULL.]*/
    /*Codes_SRS_BLE_13_004: [BLE_Create shall return NULL if the configuration->instructions vector is empty(size is zero).]*/
    if (
            bus == NULL ||
            configuration == NULL ||
            config->instructions == NULL ||
            VECTOR_size(config->instructions) == 0
       )
    {
        LogError("Invalid input");
        result = NULL;
    }
    else
    {
        /*Codes_SRS_BLE_13_009: [  BLE_Create  shall allocate memory for an instance of the  BLE_HANDLE_DATA  structure and use that as the backing structure for the module handle. ]*/
        result = (BLE_HANDLE_DATA*)malloc(sizeof(BLE_HANDLE_DATA));
        if (result == NULL)
        {
            /*Codes_SRS_BLE_13_005: [  BLE_Create  shall return  NULL  if an underlying API call fails. ]*/
            LogError("malloc failed");
            /* result is already NULL */
        }
        else
        {
            /*Codes_SRS_BLE_13_008: [  BLE_Create  shall create and initialize the  bleio_gatt  field in the  BLE_HANDLE_DATA  object by calling  BLEIO_gatt_create . ]*/
            result->bleio_gatt = BLEIO_gatt_create(&(config->device_config));
            if (result->bleio_gatt == NULL)
            {
                /*Codes_SRS_BLE_13_005: [  BLE_Create  shall return  NULL  if an underlying API call fails. ]*/
                LogError("BLEIO_gatt_create failed");
                free(result);
                result = NULL;
            }
            else
            {
                // transform BLE_INSTRUCTION objects into BLEIO_SEQ_INSTRUCTION objects
                VECTOR_HANDLE instructions = ble_instr_to_bleioseq_instr(result, config->instructions);
                if (instructions == NULL)
                {
                    /*Codes_SRS_BLE_13_005: [  BLE_Create  shall return  NULL  if an underlying API call fails. ]*/
                    LogError("Converting BLE_INSTRUCTION objects into BLEIO_SEQ_INSTRUCTION objects failed");
                    BLEIO_gatt_destroy(result->bleio_gatt);
                    free(result);
                    result = NULL;
                }
                else
                {
                    /*Codes_SRS_BLE_13_010: [  BLE_Create  shall create and initialize the  bleio_seq  field in the  BLE_HANDLE_DATA  object by calling  BLEIO_Seq_Create . ]*/
                    result->bleio_seq = BLEIO_Seq_Create(
                        result->bleio_gatt,
                        instructions,
                        on_read_complete,
                        on_write_complete
                    );
                    if (result->bleio_seq == NULL)
                    {
                        /*Codes_SRS_BLE_13_005: [  BLE_Create  shall return  NULL  if an underlying API call fails. ]*/
                        LogError("BLEIO_Seq_Create failed");
                        BLEIO_gatt_destroy(result->bleio_gatt);
                        VECTOR_destroy(instructions);
                        free(result);
                        result = NULL;
                    }
                    else
                    {
                        result->bus = bus;
                        memcpy(&(result->device_config), &(config->device_config), sizeof(result->device_config));
                        result->is_destroy_complete = false;

#if __linux__
                        if (init_glib_loop(result) == false)
                        {
                            LogError("init_glib_loop returned false");
                            BLEIO_Seq_Destroy(result->bleio_seq, NULL, NULL);
                            free(result);
                            result = NULL;
                        }
                        else
                        {
#endif
                            /*Codes_SRS_BLE_13_011: [  BLE_Create  shall asynchronously open a connection to the BLE device by calling  BLEIO_gatt_connect . ]*/
                            result->is_connected = false;
                            if (BLEIO_gatt_connect(result->bleio_gatt, on_connect_complete, (void*)result) != 0)
                            {
                                /*Codes_SRS_BLE_13_012: [  BLE_Create  shall return  NULL  if  BLEIO_gatt_connect  returns a non-zero value. ]*/
                                LogError("BLEIO_gatt_connect failed");
#if __linux__
                                if (terminate_event_dispatcher(result) == false)
                                {
                                    LogError("terminate_event_dispatcher returned false");
                                }
#endif
                                BLEIO_Seq_Destroy(result->bleio_seq, NULL, NULL);
                                free(result);
                                result = NULL;

                                /**
                                * We don't destroy handle_data->bleio_gatt because the handle is
                                * 'owned' by the BLEIO sequence object and that will destroy the
                                * GATT I/O object.
                                */
                            }
#if __linux__
                        }
#endif
                    }
                }
            }
        }
    }

    /*Codes_SRS_BLE_13_006: [  BLE_Create  shall return a non- NULL   MODULE_HANDLE  when successful. ]*/
    return (MODULE_HANDLE)result;
}

#if __linux__
static bool init_glib_loop(BLE_HANDLE_DATA* handle_data)
{
    bool result;
    handle_data->main_loop = g_main_loop_new(NULL, FALSE);
    if (handle_data->main_loop == NULL)
    {
        LogError("g_main_loop_new returned NULL");
        result = false;
    }
    else
    {
        // start a thread to pump the message loop
        if (ThreadAPI_Create(
                &(handle_data->event_thread),
                event_dispatcher,
                (void*)handle_data
            ) != THREADAPI_OK)
        {
            LogError("ThreadAPI_Create failed");
            g_main_loop_unref(handle_data->main_loop);
            result = false;
        }
        else
        {
            result = true;
        }
    }

    return result;
}

static int event_dispatcher(void * user_data)
{
    BLE_HANDLE_DATA* handle_data = (BLE_HANDLE_DATA*)user_data;
    g_main_loop_run(handle_data->main_loop);
    g_main_loop_unref(handle_data->main_loop);
    return 0;
}

static bool terminate_event_dispatcher(BLE_HANDLE_DATA* handle_data)
{
    bool result;
    gint64 start_time = g_get_monotonic_time();
    if (handle_data->main_loop != NULL)
    {
        GMainContext* loop_context = g_main_loop_get_context(handle_data->main_loop);
        if (loop_context != NULL)
        {
            while (
                    (g_get_monotonic_time() - start_time) < EVENT_DISPATCHER_START_TIMEOUT
                    &&
                    g_main_loop_is_running(handle_data->main_loop) == FALSE
                  )
            {
                // wait for quarter of a second
                g_usleep(G_USEC_PER_SEC / 4);
            }

            if (g_main_loop_is_running(handle_data->main_loop) == TRUE)
            {
                g_main_loop_quit(handle_data->main_loop);
                result = true;
            }
            else
            {
                LogError("Timed out waiting for event dispatcher thread to initialize.");
                result = false;
            }
        }
        else
        {
            LogError("g_main_loop_get_context returned NULL");
            result = false;
        }
    }
    else
    {
        LogError("No GLIB loop to terminate.");
        result = false;
    }

    return result;
}
#endif

static VECTOR_HANDLE ble_instr_to_bleioseq_instr(BLE_HANDLE_DATA* module, VECTOR_HANDLE source_instructions)
{
    VECTOR_HANDLE result = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
    if (result != NULL)
    {
        size_t i, len = VECTOR_size(source_instructions);
        for (i = 0; i < len; ++i)
        {
            BLE_INSTRUCTION* src_instr = (BLE_INSTRUCTION*)VECTOR_element(source_instructions, i);

            // copy the data
            BLEIO_SEQ_INSTRUCTION instr;
            instr.instruction_type = src_instr->instruction_type;
            instr.characteristic_uuid = src_instr->characteristic_uuid;
            memcpy(&(instr.data), &(src_instr->data), sizeof(instr.data));
            instr.context = (void*)module;

            if (VECTOR_push_back(result, &instr, 1) != 0)
            {
                LogError("VECTOR_push_back failed");
                break;
            }
        }

        // if the loop terminated before all elements were processed,
        // then something went wrong
        if (i < len)
        {
            VECTOR_destroy(result);
            result = NULL;
        }
    }
    else
    {
        LogError("VECTOR_create failed");
    }

    return result;
}

static void on_connect_complete(
    BLEIO_GATT_HANDLE bleio_gatt_handle,
    void* context,
    BLEIO_GATT_CONNECT_RESULT connect_result
)
{
    // this MUST NOT be NULL
    BLE_HANDLE_DATA* handle_data = (BLE_HANDLE_DATA*)context;
    if (connect_result != BLEIO_GATT_CONNECT_OK)
    {
        LogError("BLEIO_gatt_connect asynchronously failed");
    }
    else
    {
        handle_data->is_connected = true;

        /*Codes_SRS_BLE_13_014: [ If the asynchronous call to  BLEIO_gatt_connect  is successful then the  BLEIO_Seq_Run  function shall be called on the  bleio_seq  field from  BLE_HANDLE_DATA . ]*/
        if (BLEIO_Seq_Run(handle_data->bleio_seq) != BLEIO_SEQ_OK)
        {
            LogError("BLEIO_Seq_Run failed");
        }
    }
}

static int format_timestamp(char* dest, size_t dest_size)
{
    int result;
    time_t t1 = time(NULL);
    if (t1 == (time_t)-1)
    {
        LogError("time() failed");
        result = __LINE__;
    }
    else
    {
        struct tm* t2 = localtime(&t1);
        if (t2 == NULL)
        {
            LogError("localtime() failed");
            result = __LINE__;
        }
        else
        {
            /**
             * Note: We record the time only with a granularity of seconds. We
             * may want to increase this to include milliseconds.
             */
            if (strftime(dest, dest_size, "%Y:%m:%dT%H:%M:%S", t2) == 0)
            {
                LogError("strftime() failed");
                result = __LINE__;
            }
            else
            {
                result = 0;
            }
        }
    }

    return result;
}

static void on_read_complete(
    BLEIO_SEQ_HANDLE bleio_seq_handle,
    void* context,
    const char* characteristic_uuid,
    BLEIO_SEQ_INSTRUCTION_TYPE type,
    BLEIO_SEQ_RESULT result,
    BUFFER_HANDLE data
)
{
    // this MUST NOT be NULL
    BLE_HANDLE_DATA* handle_data = (BLE_HANDLE_DATA*)context;
    if (result != BLEIO_SEQ_OK)
    {
        LogError("A read instruction for characteristic %s of type %s failed.",
            characteristic_uuid, ENUM_TO_STRING(BLEIO_SEQ_INSTRUCTION_TYPE, type));
    }
    else
    {
        MAP_HANDLE message_properties = Map_Create(NULL);
        if (message_properties == NULL)
        {
            LogError("Map_Create() failed");
        }
        else
        {
            // format BLE controller index
            char ble_controller_index[80];
            int ret = snprintf(ble_controller_index,
                sizeof(ble_controller_index) / sizeof(ble_controller_index[0]),
                "%d",
                handle_data->device_config.ble_controller_index
            );
            if(ret < 0 || ret >= (sizeof(ble_controller_index) / sizeof(ble_controller_index[0])))
            {
                LogError("snprintf() failed");
            }
            else
            {
                // format MAC address
                char mac_address[18] = "";
                ret = snprintf(
                    mac_address,
                    sizeof(mac_address) / sizeof(mac_address[0]),
                    "%02X:%02X:%02X:%02X:%02X:%02X",
                    handle_data->device_config.device_addr.address[0],
                    handle_data->device_config.device_addr.address[1],
                    handle_data->device_config.device_addr.address[2],
                    handle_data->device_config.device_addr.address[3],
                    handle_data->device_config.device_addr.address[4],
                    handle_data->device_config.device_addr.address[5]
                );
                if (ret < 0 || ret >= (sizeof(mac_address) / sizeof(mac_address[0])))
                {
                    LogError("snprintf() failed");
                }
                else
                {
                    // format timestamp
                    char timestamp[25] = "";
                    if (format_timestamp(timestamp, sizeof(timestamp) / sizeof(timestamp[0])) != 0)
                    {
                        LogError("format_timestamp() failed");
                    }
                    else if (Map_Add(message_properties, GW_BLE_CONTROLLER_INDEX_PROPERTY, ble_controller_index) != MAP_OK)
                    {
                        LogError("Map_Add() failed for property %s", GW_BLE_CONTROLLER_INDEX_PROPERTY);
                    }
                    else if (Map_Add(message_properties, GW_MAC_ADDRESS_PROPERTY, mac_address) != MAP_OK)
                    {
                        LogError("Map_Add() failed for property %s", GW_MAC_ADDRESS_PROPERTY);
                    }
                    else if (Map_Add(message_properties, GW_TIMESTAMP_PROPERTY, timestamp) != MAP_OK)
                    {
                        LogError("Map_Add() failed for property %s", GW_TIMESTAMP_PROPERTY);
                    }
                    else if (Map_Add(message_properties, GW_CHARACTERISTIC_UUID_PROPERTY, characteristic_uuid) != MAP_OK)
                    {
                        LogError("Map_Add() failed for property %s", GW_CHARACTERISTIC_UUID_PROPERTY);
                    }
                    else if (Map_Add(message_properties, GW_SOURCE_PROPERTY, GW_SOURCE_BLE_TELEMETRY) != MAP_OK)
                    {
                        LogError("Map_Add() failed for property %s", GW_SOURCE_PROPERTY);
                    }
                    else
                    {
                        MESSAGE_CONFIG message_config;
                        message_config.sourceProperties = message_properties;
                        message_config.size = BUFFER_length(data); // "data" MUST NOT be NULL here
                        message_config.source = (const unsigned char*)BUFFER_u_char(data);

                        MESSAGE_HANDLE message = Message_Create(&message_config);
                        if (message == NULL)
                        {
                            LogError("Message_Create() failed");
                        }
                        else
                        {
                            /*Codes_SRS_BLE_13_019: [BLE_Create shall handle the ON_BLEIO_SEQ_READ_COMPLETE callback on the BLE I/O sequence. If the call is successful then a new message shall be published on the bus with the buffer that was read as the content of the message along with the following properties:

                                | Property Name           | Description                                                   |
                                |-------------------------|---------------------------------------------------------------|
                                | ble_controller_index    | The index of the bluetooth radio hardware on the device.      |
                                | mac_address             | MAC address of the BLE device from which the data was read.   |
                                | timestamp               | Timestamp indicating when the data was read.                  |
                            ]*/
                            if (MessageBus_Publish(handle_data->bus, (MODULE_HANDLE)handle_data, message) != MESSAGE_BUS_OK)
                            {
                                LogError("MessageBus_Publish() failed");
                            }

                            Message_Destroy(message);
                        }
                    }

                    Map_Destroy(message_properties);
                }
            }
        }
    }

    BUFFER_delete(data);
}

void on_write_complete(
    BLEIO_SEQ_HANDLE bleio_seq_handle,
    void * context,
    const char * characteristic_uuid,
    BLEIO_SEQ_INSTRUCTION_TYPE type,
    BLEIO_SEQ_RESULT result
)
{
    // this MUST NOT be NULL
    BLE_HANDLE_DATA* handle_data = (BLE_HANDLE_DATA*)context;

    if (result != BLEIO_SEQ_OK)
    {
        // format MAC address
        char mac_address[18];
        snprintf(
            mac_address,
            sizeof(mac_address) / sizeof(mac_address[0]),
            "%02X:%02X:%02X:%02X:%02X:%02X",
            handle_data->device_config.device_addr.address[0],
            handle_data->device_config.device_addr.address[1],
            handle_data->device_config.device_addr.address[2],
            handle_data->device_config.device_addr.address[3],
            handle_data->device_config.device_addr.address[4],
            handle_data->device_config.device_addr.address[5]
        );

        LogError(
            "Write instruction of type %s for device %s on BLE controller %d for characteristic %s failed",
            ENUM_TO_STRING(BLEIO_SEQ_INSTRUCTION_TYPE, type),
            mac_address,
            handle_data->device_config.ble_controller_index,
            characteristic_uuid
        );
    }
}

static bool is_message_for_module(const char* mac_address_str, BLE_HANDLE_DATA* handle_data)
{
    bool result;
    BLE_MAC_ADDRESS mac_address;

    result = parse_mac_address(mac_address_str, &mac_address);
    if (result == true)
    {
        /*Codes_SRS_BLE_13_022: [ BLE_Receive shall ignore the message unless the 'macAddress' property matches the MAC address that was passed to this module when it was created. ]*/
        // check if the mac address matches this module's mac address
        if (memcmp(&handle_data->device_config.device_addr, &mac_address, sizeof(BLE_MAC_ADDRESS)) == 0)
        {
            result = true;
        }
        else
        {
            result = false;
        }
    }
    else
    {
        LogError("Invalid mac address");
    }

    return result;
}

static void BLE_Receive(MODULE_HANDLE module, MESSAGE_HANDLE message)
{
    /*Codes_SRS_BLE_13_018: [ BLE_Receive shall do nothing if module is NULL or if message is NULL. ]*/
    if (module != NULL && message != NULL)
    {
        BLE_HANDLE_DATA* handle_data = (BLE_HANDLE_DATA*)module;
        CONSTMAP_HANDLE properties = Message_GetProperties(message);

        /*Codes_SRS_BLE_13_020: [ BLE_Receive shall ignore all messages except those that have the following properties:
            >| Property Name           | Description                                                             |
            >|-------------------------|-------------------------------------------------------------------------|
            >| source                  | This property should have the value "BLE".                              |
            >| macAddress              | MAC address of the BLE device to which the data to should be written.   |
        ]*/
        // fetch the 'source' property
        const char* source = ConstMap_GetValue(properties, GW_SOURCE_PROPERTY);
        if (source != NULL && strcmp(source, GW_SOURCE_BLE_COMMAND) == 0)
        {
            // fetch the 'macAddress' property
            const char* mac_address = ConstMap_GetValue(properties, GW_MAC_ADDRESS_PROPERTY);
            if (mac_address != NULL && is_message_for_module(mac_address, handle_data) == true)
            {
                const CONSTBUFFER* content = Message_GetContent(message);
                if (content != NULL && content->buffer != NULL && content->size > 0)
                {
                    BLE_INSTRUCTION* ble_instruction = (BLE_INSTRUCTION*)content->buffer;

                    // transform BLE_INSTRUCTION into BLEIO_SEQ_INSTRUCTION
                    BLEIO_SEQ_INSTRUCTION ble_seq_instruction;
                    ble_seq_instruction.instruction_type = ble_instruction->instruction_type;
                    ble_seq_instruction.characteristic_uuid = ble_instruction->characteristic_uuid;
                    memcpy(&(ble_seq_instruction.data), &(ble_instruction->data), sizeof(ble_instruction->data));

                    // MUST set this as the context so on_read_complete and on_write_complete get
                    // access to BLE_HANDLE_DATA
                    ble_seq_instruction.context = (void*)module;

                    /*Codes_SRS_BLE_13_021: [ BLE_Receive shall treat the content of the message as a BLE_INSTRUCTION and schedule it for execution by calling BLEIO_Seq_AddInstruction. ]*/
                    if (BLEIO_Seq_AddInstruction(handle_data->bleio_seq, &ble_seq_instruction) != BLEIO_SEQ_OK)
                    {
                        LogError("BLEIO_Seq_AddInstruction failed");
                    }
                }
            }
        }

        ConstMap_Destroy(properties);
    }
    else
    {
        LogError("module and/or message is NULL");
    }
}

static void on_destroy_complete(BLEIO_SEQ_HANDLE bleio_seq_handle, void* context)
{
    BLE_HANDLE_DATA* handle_data = (BLE_HANDLE_DATA*)context;
    if (handle_data != NULL)
    {
        handle_data->is_destroy_complete = true;

        if (handle_data->bleio_gatt != NULL)
        {
            // if the device is connected then first disconnect
            if (handle_data->is_connected)
            {
                BLEIO_gatt_disconnect(handle_data->bleio_gatt, NULL, NULL);
            }

            /**
            * We don't destroy handle_data->bleio_gatt because the handle is
            * 'owned' by the BLEIO sequence object and that will destroy the
            * GATT I/O object.
            */
        }
    }
    else
    {
        LogError("bleio_seq_handle is NULL");
    }
}

static void BLE_Destroy(MODULE_HANDLE module)
{
    /*Codes_SRS_BLE_13_016: [If module is NULL BLE_Destroy shall do nothing.]*/
    if (module != NULL)
    {
        /*Codes_SRS_BLE_13_017: BLE_Destroy shall free all resources. ]*/
        BLE_HANDLE_DATA* handle_data = (BLE_HANDLE_DATA*)module;
        if (handle_data->bleio_seq != NULL)
        {
            BLEIO_Seq_Destroy
            (
                handle_data->bleio_seq,
                on_destroy_complete,
                (void*)handle_data
            );
 #if __linux__
            // wait for on_destroy_complete to be called; bail after 5 seconds
            gint64 start_time = g_get_monotonic_time();
            if (handle_data->main_loop != NULL)
            {
                GMainContext* loop_context = g_main_loop_get_context(handle_data->main_loop);
                if (loop_context != NULL)
                {
                    LogInfo("Waiting for sequence to be destroyed...");
                    while (handle_data->is_destroy_complete == false)
                    {
                        g_main_context_iteration(loop_context, FALSE);
                        if ((g_get_monotonic_time() - start_time) >= DESTROY_COMPLETE_TIMEOUT)
                        {
                            LogError("on_destroy_complete did not get called in time");
                            break;
                        }
                    }
                    LogInfo("Done waiting for sequence to be destroyed.");
                }
                else
                {
                    LogError("g_main_loop_get_context returned NULL");
                }

                // terminate glib loop
                g_main_loop_quit(handle_data->main_loop);

                // wait for thread to exit
                int thread_result;
                if (ThreadAPI_Join(handle_data->event_thread, &thread_result) != THREADAPI_OK)
                {
                    LogError("ThreadAPI_Join() returned an error");
                }
            }
#endif
        }

        free(handle_data);
    }
    else
    {
        LogError("module handle is NULL");
    }
}

static const MODULE_APIS Module_GetAPIS_Impl =
{
    BLE_Create,
    BLE_Destroy,
    BLE_Receive
};

/*Codes_SRS_BLE_13_007: [Module_GetAPIS shall return a non - NULL pointer to a structure of type MODULE_APIS that has all fields initialized to non - NULL values.]*/
#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(BLE_MODULE)(void)
#else
const MODULE_APIS* Module_GetAPIS(void)
#endif
{
    return &Module_GetAPIS_Impl;
}
