// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "oop_module.h"
#include "broker.h"

#include "stdlib.h"
#include "string.h"

#include "nanomsg/nn.h"
#include "nanomsg/pair.h"

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"

#include "control_message.h"
#include "gateway.h"
#include "message.h"

typedef enum OOP_MODULE_STATUS_TAG {
    OOP_MODULE_DETACH = -1,
    OOP_MODULE_OK,
    OOP_MODULE_GATEWAY_CONNECTION_ERROR,
    OOP_MODULE_MODULE_CREATION_ERROR,
} OOP_MODULE_STATUS;

typedef struct MESSAGE_THREAD_TAG * MESSAGE_THREAD_HANDLE;

int
connect_to_message_channels (
    OOP_MODULE_HANDLE oop_module,
    const MESSAGE_URI * channel_uris,
    const size_t channel_count
);

void
disconnect_from_message_channels (
    OOP_MODULE_HANDLE oop_module
);

int
invoke_add_module_process (
    OOP_MODULE_HANDLE oop_module,
    const char * json_config
);

int
worker_thread (
    void * thread_arg
);

int
process_module_create_message (
    OOP_MODULE_HANDLE oop_module,
    CONTROL_MESSAGE_MODULE_CREATE * message
);

int
send_control_reply (
    OOP_MODULE_HANDLE oop_module,
    uint8_t response
);

typedef struct MESSAGE_THREAD_TAG {
    bool halt;
    LOCK_HANDLE mutex;
    THREAD_HANDLE thread;
} MESSAGE_THREAD;

typedef struct OOP_MODULE_TAG {
	int control_endpoint;
	int control_socket;
    size_t message_channel_count;
    int * message_endpoints;
    int * message_sockets;
    MESSAGE_THREAD_HANDLE message_thread;
    MODULE module;
} OOP_MODULE;


/* Codes_SRS_OOP_MODULE_027_000: [Prerequisite Check - If the `module_apis` parameter is `NULL`, then `OopModule_Attach` shall do nothing and return `NULL`] */
/* Codes_SRS_OOP_MODULE_027_001: [Prerequisite Check - If the `module_apis` version is greater than 1, then `OopModule_Attach` shall do nothing and return `NULL`] */
/* Codes_SRS_OOP_MODULE_027_002: [Prerequisite Check - If the `module_apis` interface fails to provide `Module_Create`, then `OopModule_Attach` shall do nothing and return `NULL`] */
/* Codes_SRS_OOP_MODULE_027_003: [Prerequisite Check - If the `module_apis` interface fails to provide `Module_Destroy`, then `OopModule_Attach` shall do nothing and return `NULL`] */
/* Codes_SRS_OOP_MODULE_027_004: [Prerequisite Check - If the `module_apis` interface fails to provide `Module_Receive`, then `OopModule_Attach` shall do nothing and return `NULL`] */
/* Codes_SRS_OOP_MODULE_027_005: [Prerequisite Check - If the `connection_id` parameter is `NULL`, then `OopModule_Attach` shall do nothing and return `NULL`] */
/* Codes_SRS_OOP_MODULE_027_006: [Prerequisite Check - If the `connection_id` parameter is longer than `GATEWAY_CONNECTION_ID_MAX`, then `OopModule_Attach` shall do nothing and return `NULL`] */
/* Codes_SRS_OOP_MODULE_027_007: [`OopModule_Attach` shall allocate the memory required to support its instance data] */
/* Codes_SRS_OOP_MODULE_027_008: [If memory allocation fails for the instance data, then `OopModule_Attach` shall return `NULL`] */
/* Codes_SRS_OOP_MODULE_027_009: [`OopModule_Attach` shall allocate the memory required to formulate the connection string to the Azure IoT Gateway] */
/* Codes_SRS_OOP_MODULE_027_010: [If memory allocation fails for the connection string, then `OopModule_Attach` shall free any previously allocated memory and return `NULL`] */
/* Codes_SRS_OOP_MODULE_027_011: [`OopModule_Attach` shall create a socket for the Azure IoT Gateway control channel by calling `int nn_socket(int domain, int protocol)` with `AF_SP` as the `domain` and `NN_PAIR` as the `protocol`] */
/* Codes_SRS_OOP_MODULE_027_012: [If the call to `nn_socket` returns -1, then `OopModule_Attach` shall free any previously allocated memory and return `NULL`] */
/* Codes_SRS_OOP_MODULE_027_013: [`OopModule_Attach` shall bind to the Azure IoT Gateway control channel by calling `int nn_bind(int s, const char * addr)` with the newly created socket as `s` and the newly formulated connection string as `addr`] */
/* Codes_SRS_OOP_MODULE_027_014: [If the call to `nn_bind` returns a negative value, then `OopModule_Attach` shall free any previously allocated memory and return `NULL`] */
/* Codes_SRS_OOP_MODULE_027_015: [If no errors are encountered, then `OopModule_Attach` return a handle to the OopModule instance] */
OOP_MODULE_HANDLE
OopModule_Attach (
    const MODULE_API * module_apis,
    const char * const connection_id
) {
    OOP_MODULE_HANDLE oop_module;

    if (NULL == module_apis) {
        LogError("%s: NULL parameter - module_apis", __FUNCTION__);
        oop_module = NULL;
    } else if (MODULE_API_VERSION_1 > (int)module_apis->version) {
        LogError("%s: Incompatible API version - module_apis->version", __FUNCTION__);
        oop_module = NULL;
    } else if (NULL == ((MODULE_API_1 *)module_apis)->Module_Create) {
        LogError("%s: Required interface not met - module_apis->Module_Create", __FUNCTION__);
        oop_module = NULL;
    } else if (NULL == ((MODULE_API_1 *)module_apis)->Module_Destroy) {
        LogError("%s: Required interface not met - module_apis->Module_Destroy", __FUNCTION__);
        oop_module = NULL;
    } else if (NULL == ((MODULE_API_1 *)module_apis)->Module_Receive) {
        LogError("%s: Required interface not met - module_apis->Module_Receive", __FUNCTION__);
        oop_module = NULL;
    } else if (NULL == connection_id) {
        LogError("%s: NULL parameter - connection_id", __FUNCTION__);
        oop_module = NULL;
    } else if (GATEWAY_CONNECTION_ID_MAX > strnlen(connection_id, (GATEWAY_CONNECTION_ID_MAX + 1)) ) {
        LogError("%s: connection_id exceeds GATEWAY_CONNECTION_ID_MAX in length", __FUNCTION__);
        oop_module = NULL;
    // Allocate and initialize the out-of-process module data structure
    } else if (NULL == (oop_module = (OOP_MODULE_HANDLE)calloc(1, sizeof(OOP_MODULE)))) {
        LogError("%s: Unable to allocate memory!", __FUNCTION__);
    } else {
        static const size_t ENDPOINT_LENGTH_MAX = (sizeof("ipc://") + NN_SOCKADDR_MAX);

        char * control_channel_uri;

        if (NULL == (control_channel_uri = (char *)malloc(strnlen(connection_id, ENDPOINT_LENGTH_MAX)))) {
            LogError("%s: Unable to allocate memory!", __FUNCTION__);
            free(oop_module);
            oop_module = NULL;
        } else {
            // Transform the connection id into a nanomsg URI
            (void)strcpy(control_channel_uri, "ipc://");
            (void)strncat(control_channel_uri, connection_id, NN_SOCKADDR_MAX + 1);

            // Create the socket
            if (-1 == (oop_module->control_socket = nn_socket(AF_SP, NN_PAIR))) {
                LogError("%s: Unable to create the gateway socket!", __FUNCTION__);
                free(oop_module);
                oop_module = NULL;
            // Connect to the control channel URI
            } else if ( 0 > (oop_module->control_endpoint = nn_bind(oop_module->control_socket, control_channel_uri))) {
                LogError("%s: Unable to connect to the gateway control channel!", __FUNCTION__);
                free(oop_module);
                oop_module = NULL;
            } else {
                // Save the module API
                oop_module->module.module_apis = module_apis;
            }
            free(control_channel_uri);
        }
    }

    return oop_module;
}

/* SRS_OOP_MODULE_027_056: [Prerequisite Check - If the `oop_module` parameter is `NULL`, then `OopModule_Detach` shall do nothing] */
/* SRS_OOP_MODULE_027_057: [If the worker thread is active, then `OopModule_Detach` shall attempt to halt the worker thread] */
/* SRS_OOP_MODULE_027_058: [`OopModule_Detach` shall create the out-of-process module by calling `MODULE_HANDLE Module_Create(BROKER_HANDLE broker, const void * configuration)` using the `oop_module_handle` as `broker` and the parsed configuration value or `CONTROL_MESSAGE_MODULE_CREATE::args` as `configuration`] */
/* SRS_OOP_MODULE_027_059: [If unable to halt the worker thread, `OopModule_Detach` shall forcibly free the memory allocated to the worker thread] */
/* SRS_OOP_MODULE_027_060: [`OopModule_Detach` shall attempt to notify the Azure IoT Gateway of the detachment] */
/* SRS_OOP_MODULE_027_061: [`OopModule_Detach` shall disconnect from the Azure IoT Gateway message channels] */
/* SRS_OOP_MODULE_027_062: [`OopModule_Detach` shall shutdown the Azure IoT Gateway control channel by calling `int nn_shutdown(int s, int how)`] */
/* SRS_OOP_MODULE_027_063: [`OopModule_Detach` shall close the Azure IoT Gateway control socket by calling `int nn_close(int s)`] */
/* SRS_OOP_MODULE_027_064: [`OopModule_Detach` shall free the remaining memory dedicated to its instance data] */
void
OopModule_Detach (
    OOP_MODULE_HANDLE oop_module
) {
    if (NULL == oop_module) {
        LogError("%s: NULL parameter - oop_module!", __FUNCTION__);
    } else {
        // Ensure message thread handle is NULL
        if (NULL != oop_module->message_thread) {
            (void)OopModule_HaltWorkerThread(oop_module);
            if (NULL != oop_module->message_thread) {
                LogError("%s: Unable to gracefully halt worker thread!", __FUNCTION__);
                free(oop_module->message_thread);
                oop_module->message_thread = NULL;
            }
        }

        // Send termination signal
        (void)send_control_reply(oop_module, (uint8_t)OOP_MODULE_DETACH);

        // Free nanomsg sockets and endpoints
        disconnect_from_message_channels(oop_module);
        (void)nn_shutdown(oop_module->control_socket, oop_module->control_endpoint);
        oop_module->control_endpoint = 0;
        (void)nn_close(oop_module->control_socket);
        oop_module->control_socket = 0;

        free(oop_module);
        oop_module = NULL;
    }

    return;
}


/* SRS_OOP_MODULE_027_025: [Prerequisite Check - If the `oop_module` parameter is `NULL`, then `OopModule_DoWork` shall do nothing] */
/* SRS_OOP_MODULE_027_026: [Control Channel - `OopModule_DoWork` shall poll the gateway control channel by calling `int nn_recv(int s, void * buf, size_t len, int flags)` with the control socket for `s`, `NULL` for `buf`, `NN_MSG` for `len` and NN_DONTWAIT for `flags`] */
/* SRS_OOP_MODULE_027_027: [Control Channel - If no message is available or an error occurred, then `OopModule_DoWork` shall abandon the control channel request] */
/* SRS_OOP_MODULE_027_028: [Control Channel - If a control message was received, then `OopModule_DoWork` will parse that message by calling `CONTROL_MESSAGE * ControlMessage_CreateFromByteArray(const unsigned char * source, size_t size)` with the buffer received from `nn_recv` as `source` and return value from `nn_recv` as `size`] */
/* SRS_OOP_MODULE_027_029: [Control Channel - If unable to parse the control message, then `OopModule_DoWork` shall free any previously allocated memory and abandon the control channel request] */
/* SRS_OOP_MODULE_027_030: [Control Channel - If the message type is CONTROL_MESSAGE_TYPE_MODULE_CREATE, then `OopModule_DoWork` shall process the create message] */
/* SRS_OOP_MODULE_027_031: [Control Channel - If unable process the create message, `OopModule_DoWork` shall return a non-zero value] */
/* SRS_OOP_MODULE_027_032: [Control Channel - If the message type is CONTROL_MESSAGE_TYPE_MODULE_START and `Module_Start` was provided, then `OopModule_DoWork` shall call `void Module_Start(MODULE_HANDLE moduleHandle)`] */
/* SRS_OOP_MODULE_027_033: [Control Channel - If the message type is CONTROL_MESSAGE_TYPE_MODULE_DESTROY, then `OopModule_DoWork` shall call `void Module_Destroy(MODULE_HANDLE moduleHandle)`] */
/* SRS_OOP_MODULE_027_034: [Control Channel - `OopModule_DoWork` shall free the resources held by the parsed control message by calling `void ControlMessage_Destroy(CONTROL_MESSAGE * message)` using the parsed control message as `message`] */
/* SRS_OOP_MODULE_027_035: [Control Channel - `OopModule_DoWork` shall free the resources held by the gateway message by calling `int nn_freemsg(void * msg)` with the resulting buffer from the previous call to `nn_recv`] */
/* SRS_OOP_MODULE_027_036: [Message Channel - `OopModule_DoWork` shall poll each gateway message channel by calling `int nn_recv(int s, void * buf, size_t len, int flags)` with each message socket for `s`, `NULL` for `buf`, `NN_MSG` for `len` and NN_DONTWAIT for `flags`] */
/* SRS_OOP_MODULE_027_037: [Message Channel - If no message is available or an error occurred, then `OopModule_DoWork` shall abandon the message channel request] */
/* SRS_OOP_MODULE_027_038: [Message Channel - If a module message was received, then `OopModule_DoWork` will parse that message by calling `MESSAGE_HANDLE Message_CreateFromByteArray(const unsigned char * source, int32_t size)` with the buffer received from `nn_recv` as `source` and return value from `nn_recv` as `size`] */
/* SRS_OOP_MODULE_027_039: [Message Channel - If unable to parse the module message, then `OopModule_DoWork` shall free any previously allocated memory and abandon the message channel request] */
/* SRS_OOP_MODULE_027_040: [Message Channel - `OopModule_DoWork` shall pass the structured message to the module by calling `void Module_Receive(MODULE_HANDLE moduleHandle)` using the parsed message as `moduleHandle`] */
/* SRS_OOP_MODULE_027_041: [Message Channel - `OopModule_DoWork` shall free the resources held by the parsed module message by calling `void Message_Destroy(MESSAGE_HANDLE * message)` using the parsed module message as `message`] */
/* SRS_OOP_MODULE_027_042: [Message Channel - `OopModule_DoWork` shall free the resources held by the gateway message by calling `int nn_freemsg(void * msg)` with the resulting buffer from the previous call to `nn_recv`] */
void
OopModule_DoWork (
    OOP_MODULE_HANDLE oop_module
) {
    if (NULL == oop_module) {
        LogError("%s: NULL parameter - oop_module!", __FUNCTION__);
    } else {
        int bytes_received;
        void * control_message = NULL;

        // Check for message on control channel
        bytes_received = nn_recv(oop_module->control_socket, &control_message, NN_MSG, NN_DONTWAIT);
        if (EAGAIN == bytes_received) {
            // no messages available at this time    
        } else if (0 > bytes_received) {
            LogError("%s: Unexpected error received from the control channel!", __FUNCTION__);
        } else {
            CONTROL_MESSAGE * structured_control_message;

            // Parse message body
            if (NULL == (structured_control_message = ControlMessage_CreateFromByteArray((const unsigned char *)control_message, bytes_received))) {
                LogError("%s: Unable to parse control message!", __FUNCTION__);
            } else {
                // Route control channel messages to appropriate functions
                switch (structured_control_message->type) {
                  case CONTROL_MESSAGE_TYPE_MODULE_CREATE:
                    if (0 != process_module_create_message(oop_module, (CONTROL_MESSAGE_MODULE_CREATE *)structured_control_message)) {
                        LogError("%s: Unable to process create message!", __FUNCTION__);
                    }
                    break;
                  case CONTROL_MESSAGE_TYPE_MODULE_START: if (((MODULE_API_1 *)oop_module->module.module_apis)->Module_Start) { ((MODULE_API_1 *)oop_module->module.module_apis)->Module_Start(oop_module->module.module_handle); } break;
                  case CONTROL_MESSAGE_TYPE_MODULE_DESTROY: ((MODULE_API_1 *)oop_module->module.module_apis)->Module_Destroy(oop_module->module.module_handle); break;
                  default: LogError("ERROR: OOP_MODULE - Received unsupported message type! [%d]\n", structured_control_message->type); break;
                }
                ControlMessage_Destroy(structured_control_message);
            }
            (void)nn_freemsg(control_message);
        }

        // Check first message on each message channel
        for (int i = 0; i < oop_module->message_channel_count ; ++i) {
            void * module_message = NULL;

            bytes_received = nn_recv(oop_module->message_sockets[i], &module_message, NN_MSG, NN_DONTWAIT);
            if (bytes_received < 0 && EAGAIN != bytes_received) {
                // unexpected error
            } else if (EAGAIN == bytes_received) {
                // no messages available at this time
            } else {
                MESSAGE_HANDLE structured_module_message;

                // Parse message body
                if (NULL == (structured_module_message = Message_CreateFromByteArray((const unsigned char *)module_message, bytes_received))) {
                    LogError("%s: Unable to parse control message!", __FUNCTION__);
                } else {
                    // Forward the gateway message
                    ((MODULE_API_1 *)oop_module->module.module_apis)->Module_Receive(oop_module->module.module_handle, structured_module_message);
                    Message_Destroy(structured_module_message);
                }
            }
            (void)nn_freemsg(module_message);
        }
    }

    return;
}


/* Codes_SRS_OOP_MODULE_027_043: [Prerequisite Check - If the `oop_module` parameter is `NULL`, then `OopModule_HaltWorkerThread` shall return a non-zero value] */
/* Codes_SRS_OOP_MODULE_027_044: [Prerequisite Check - If a worker thread does not exist, then `OopModule_HaltWorkerThread` shall return a non-zero value] */
/* Codes_SRS_OOP_MODULE_027_045: [`OopModule_HaltWorkerThread` shall obtain the thread mutex in order to signal the thread by calling `LOCK_RESULT Lock(LOCK_HANDLE handle)`] */
/* Codes_SRS_OOP_MODULE_027_046: [If unable to obtain the mutex, then `OopModule_HaltWorkerThread` shall return a non-zero value] */
/* Codes_SRS_OOP_MODULE_027_047: [`OopModule_HaltWorkerThread` shall release the thread mutex upon signalling by calling `LOCK_RESULT Unlock(LOCK_HANDLE handle)`] */
/* Codes_SRS_OOP_MODULE_027_048: [If unable to release the mutex, then `OopModule_HaltWorkerThread` shall return a non-zero value] */
/* Codes_SRS_OOP_MODULE_027_049: [`OopModule_HaltWorkerThread` shall halt the thread by calling `THREADAPI_RESULT ThreadAPI_Join(THREAD_HANDLE handle, int * res)`] */
/* Codes_SRS_OOP_MODULE_027_050: [If unable to join the thread, then `OopModule_HaltWorkerThread` shall return a non-zero value] */
/* Codes_SRS_OOP_MODULE_027_051: [`OopModule_HaltWorkerThread` shall free the thread mutex by calling `LOCK_RESULT Lock_Deinit(LOCK_HANDLE handle)`] */
/* Codes_SRS_OOP_MODULE_027_052: [If unable to free the thread mutex, then `OopModule_HaltWorkerThread` shall ignore the result and continue processing] */
/* Codes_SRS_OOP_MODULE_027_053: [`OopModule_HaltWorkerThread` shall free the memory allocated to the thread details] */
/* Codes_SRS_OOP_MODULE_027_054: [If an error is returned from the worker thread, then `OopModule_HaltWorkerThread` shall return the worker thread's error code] */
/* Codes_SRS_OOP_MODULE_027_055: [If no errors are encountered, then `OopModule_HaltWorkerThread` shall return zero] */
int
OopModule_HaltWorkerThread (
    OOP_MODULE_HANDLE oop_module
) {
    int result;
    int thread_exit_result;
    
    // Check if message thread handle is NULL
    if (NULL == oop_module) {
        LogError("%s: NULL parameter - oop_module!", __FUNCTION__);
        result = __LINE__;
    // Check to confirm message thread is running
    } else if (NULL == oop_module->message_thread) {
        LogInfo("%s: Worker thread has not been initialized.", __FUNCTION__);
    // Acquire mutex
    } else if (LOCK_OK != Lock(oop_module->message_thread->mutex)) {
        LogError("%s: Unable to acquire mutex!", __FUNCTION__);
        result = __LINE__;
    } else {
        // Signal the message thread
        oop_module->message_thread->halt = true;

        // Release mutex
        if (LOCK_OK != Unlock(oop_module->message_thread->mutex)) {
            LogError("%s: Unable to release mutex!", __FUNCTION__);
            result = __LINE__;
        // Halt the message thread
        } else if (THREADAPI_OK != ThreadAPI_Join(oop_module->message_thread, &thread_exit_result)) {
            LogError("%s: Unable to halt message thread!", __FUNCTION__);
            result = __LINE__;
        } else {
            // Free memory allocated for the thread
            (void)Lock_Deinit(oop_module->message_thread->mutex);
            free(oop_module->message_thread);
            oop_module->message_thread = NULL;

            if (0 != thread_exit_result) {
                LogError("%s: Thread exited with fail code <%d>!", __FUNCTION__, thread_exit_result);
                result = thread_exit_result;
            } else {
                result = 0;
            }
        }
    }

    return result;
}


/* Codes_SRS_OOP_MODULE_027_016: [Prerequisite Check - If the `oop_module` parameter is `NULL`, then `OopModule_StartWorkerThread` shall do nothing and return a non-zero value] */
/* Codes_SRS_OOP_MODULE_027_017: [Prerequisite Check - If a work thread already exist for the given handle, then `OopModule_StartWorkerThread` shall do nothing and return zero] */
/* Codes_SRS_OOP_MODULE_027_018: [`OopModule_StartWorkerThread` shall allocate the memory required to support the worker thread] */
/* Codes_SRS_OOP_MODULE_027_019: [If memory allocation fails for the worker thread data, then `OopModule_StartWorkerThread` shall return a non-zero value] */
/* Codes_SRS_OOP_MODULE_027_020: [`OopModule_StartWorkerThread` shall create a mutex by calling `LOCK_HANDLE Lock_Init(void)`] */
/* Codes_SRS_OOP_MODULE_027_021: [If a mutex is unable to be created, then `OopModule_StartWorkerThread` shall free any previously allocated memory and return a non-zero value] */
/* Codes_SRS_OOP_MODULE_027_022: [`OopModule_StartWorkerThread` shall start a worker thread by calling `THREADAPI_RESULT ThreadAPI_Create(&THREAD_HANDLE threadHandle, THREAD_START_FUNC func, void * arg)` with an empty thread handle for `threadHandle`, a function that loops polling the messages for `func`, and `oop_module` for `arg`] */
/* Codes_SRS_OOP_MODULE_027_023: [If the worker thread failed to start, then `OopModule_StartWorkerThread` shall free any previously allocated memory and return a non-zero value] */
/* Codes_SRS_OOP_MODULE_027_024: [If no errors are encountered, then `OopModule_StartWorkerThread` shall return zero] */
int
OopModule_StartWorkerThread (
	OOP_MODULE_HANDLE oop_module
) {
    int result;

    if (NULL == oop_module) {
        LogError("%s: NULL parameter - oop_module!", __FUNCTION__);
        result = __LINE__;
    // Check to confirm message thread is not running
    } else if (NULL != oop_module->message_thread) {
        LogInfo("%s: Worker thread has already been initialized.", __FUNCTION__);
	// Allocate and initialize the message thread data structure
    } else if (	oop_module->message_thread = (MESSAGE_THREAD_HANDLE)calloc(1, sizeof(MESSAGE_THREAD))) {
        LogError("%s: Unable to allocate memory!", __FUNCTION__);
        result = __LINE__;
	// Create a mutex for context shared between threads
    } else if (NULL == (oop_module->message_thread->mutex = Lock_Init())) {
        LogError("%s: Unable to create mutex!", __FUNCTION__);
        result = __LINE__;
	// Start the message pump thread
    } else if (THREADAPI_OK != ThreadAPI_Create(&oop_module->message_thread->thread, worker_thread, oop_module)) {
        LogError("%s: Unable to create worker thread!", __FUNCTION__);
        result = __LINE__;
    } else {
        result = 0;
    }

	return result;
}


/*Codes_SRS_BROKER_17_022: [ N/A - Broker_Publish shall Lock the modules lock. ]*/
/*Codes_SRS_BROKER_17_023: [ N/A - Broker_Publish shall Unlock the modules lock. ]*/
BROKER_RESULT
Broker_Publish (
    BROKER_HANDLE broker,
    MODULE_HANDLE source,
    MESSAGE_HANDLE message
) {
    (void)source;
    OOP_MODULE_HANDLE oop_module = (OOP_MODULE_HANDLE)broker;
    BROKER_RESULT result;

    /*Codes_SRS_BROKER_13_030: [If broker or message is NULL the function shall return BROKER_INVALIDARG.]*/
    if (broker == NULL || message == NULL)
    {
        result = BROKER_INVALIDARG;
        LogError("Broker handle and/or message handle is NULL");
    }
    else
    {
        // Send message_ to nanomsg
        int32_t msg_size;
        int32_t buf_size;
        /*Codes_SRS_BROKER_17_007: [ Broker_Publish shall clone the message. ]*/
        MESSAGE_HANDLE msg = Message_Clone(message);
        /*Codes_SRS_BROKER_17_008: [ Broker_Publish shall serialize the message. ]*/
        msg_size = Message_ToByteArray(message, NULL, 0);
        if (msg_size < 0)
        {
            /*Codes_SRS_BROKER_13_053: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]*/
            LogError("unable to serialize a message [%p]", msg);
            Message_Destroy(msg);
            result = BROKER_ERROR;
        }
        else
        {
            /*Codes_SRS_BROKER_17_025: [ Broker_Publish shall allocate a nanomsg buffer the size of the serialized message + sizeof(MODULE_HANDLE). ]*/
            buf_size = msg_size + sizeof(MODULE_HANDLE);
            void* nn_msg = nn_allocmsg(buf_size, 0);
            if (nn_msg == NULL)
            {
                /*Codes_SRS_BROKER_13_053: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]*/
                LogError("unable to serialize a message [%p]", msg);
                result = BROKER_ERROR;
            }
            else
            {
                /*Codes_SRS_BROKER_17_026: [ Broker_Publish shall copy source into the beginning of the nanomsg buffer. ]*/
                unsigned char *nn_msg_bytes = (unsigned char *)nn_msg;
                memcpy(nn_msg_bytes, &oop_module->module.module_handle, sizeof(MODULE_HANDLE));
                /*Codes_SRS_BROKER_17_027: [ Broker_Publish shall serialize the message into the remainder of the nanomsg buffer. ]*/
                nn_msg_bytes += sizeof(MODULE_HANDLE);
                Message_ToByteArray(message, nn_msg_bytes, msg_size);

                /*Codes_SRS_BROKER_17_010: [ Broker_Publish shall send a message on the publish_socket. ]*/
                int nbytes = nn_send(oop_module->message_sockets[0], &nn_msg, NN_MSG, 0);
                if (nbytes != buf_size)
                {
                    /*Codes_SRS_BROKER_13_053: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]*/
                    LogError("unable to send a message [%p]", msg);
                    /*Codes_SRS_BROKER_17_012: [ Broker_Publish shall free the message. ]*/
                    nn_freemsg(nn_msg);
                    result = BROKER_ERROR;
                }
                else
                {
                    result = BROKER_OK;
                }
            }
            /*Codes_SRS_BROKER_17_012: [ Broker_Publish shall free the message. ]*/
            Message_Destroy(msg);
            /*Codes_SRS_BROKER_17_011: [ Broker_Publish shall free the serialized message data. ]*/
        }

    }

    /*Codes_SRS_BROKER_13_037: [ This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise. ]*/
    return result;
}


/* SRS_OOP_MODULE_027_0xx: [`connect_to_message_channels` shall allocate memory to store the message sockets] */
/* SRS_OOP_MODULE_027_0xx: [If unable to allocate memory for the message sockets, then `connect_to_message_channels` shall free any previously allocated memory, abandon the control message and prepare for the next create message] */
/* SRS_OOP_MODULE_027_0xx: [`connect_to_message_channels` shall allocate memory to store the message channels] */
/* SRS_OOP_MODULE_027_0xx: [If unable to allocate memory for the message channels, then `connect_to_message_channels` shall free any previously allocated memory, abandon the control message and prepare for the next create message] */
/* SRS_OOP_MODULE_027_0xx: [`connect_to_message_channels` shall create a socket for each of the Azure IoT Gateway message channels by calling `int nn_socket(int domain, int protocol)` with `AF_SP` as `domain` and `MESSAGE_URI::uri_type` as `protocol`] */
/* SRS_OOP_MODULE_027_0xx: [If a call to `nn_socket` returns -1, then `connect_to_message_channels` shall free any previously allocated memory, abandon the control message and prepare for the next create message] */
/* SRS_OOP_MODULE_027_0xx: [`connect_to_message_channels` shall connect to each of the Azure IoT Gateway message channels by calling `int nn_connect(int s, const char * addr)` with the newly created socket as `s` and `MESSAGE_URI::uri` as `addr`] */
/* SRS_OOP_MODULE_027_0xx: [If a call to `nn_connect` returns a negative value, then `connect_to_message_channels` shall free any previously allocated memory, abandon the control message and prepare for the next create message] */
/* SRS_OOP_MODULE_027_0xx: [If no errors are encountered, then `connect_to_message_channels` shall return zero] */
int
connect_to_message_channels (
    OOP_MODULE_HANDLE oop_module,
    const MESSAGE_URI * channel_uris,
    const size_t channel_count
) {
    int result;

    if (NULL == (oop_module->message_sockets = (int *)malloc(channel_count, sizeof(int)))) {
        LogError("%s: Unable to allocate memory!", __FUNCTION__);
        result = __LINE__;
    } else if (NULL == (oop_module->message_endpoints = (int *)malloc(channel_count, sizeof(int)))) {
        free(oop_module->message_sockets);
        LogError("%s: Unable to allocate memory!", __FUNCTION__);
        result = __LINE__;
    } else {
        result = 0;

        // Connect to message channel(s)
        oop_module->message_channel_count = 0;
        for (size_t i = 0; i < channel_count; ++i) {
            if (-1 == (oop_module->message_sockets[i] = nn_socket(AF_SP, channel_uris[i].uri_type))) {
                LogError("%s: Unable to create the gateway socket!", __FUNCTION__);
                result = __LINE__;
                disconnect_from_message_channels(oop_module);
            } else if (0 > (oop_module->message_endpoints[i] = nn_connect(oop_module->message_sockets[i], channel_uris[i].uri))) {
                LogError("%s: Unable to connect to the gateway message channel!", __FUNCTION__);
                result = __LINE__;
                (void)nn_close(oop_module->message_sockets[i]);
                disconnect_from_message_channels(oop_module);
            } else {
                oop_module->message_channel_count = (i + 1);
                result = 0;
            }
        }
    }

    return result;
}


/* SRS_OOP_MODULE_027_0xx: [`disconnect_from_message_channels` shall shutdown each of the Azure IoT Gateway message channels by calling `int nn_shutdown(int s, int how)`] */
/* SRS_OOP_MODULE_027_0xx: [`disconnect_from_message_channels` shall close each of the Azure IoT Gateway message sockets by calling `int nn_close(int s)`] */
/* SRS_OOP_MODULE_027_0xx: [`disconnect_from_message_channels` shall free the memory allocated to store the message channels] */
/* SRS_OOP_MODULE_027_0xx: [`disconnect_from_message_channels` shall free the memory allocated to store the message sockets] */
void
disconnect_from_message_channels (
    OOP_MODULE_HANDLE oop_module
) {
    for (size_t i = (oop_module->message_channel_count - 1); i >= 0; --i) {
        (void)nn_shutdown(oop_module->message_sockets[i], oop_module->message_endpoints[i]);
        (void)nn_close(oop_module->message_sockets[i]);
        oop_module->message_channel_count = i;
    }
    free(oop_module->message_endpoints);
    oop_module->message_endpoints = NULL;
    free(oop_module->message_sockets);
    oop_module->message_sockets = NULL;
    if (0 != oop_module->message_channel_count) {
        LogError("%s: Not all message channels have been closed", __FUNCTION__);
        oop_module->message_channel_count = 0;
    }

    return;
}


/* SRS_OOP_MODULE_027_0xx: [If `Module_ParseConfigurationFromJson` was provided, `invoke_add_module_process` shall parse the configuration by calling `void * Module_ParseConfigurationFromJson(const char * configuration)` using the `CONTROL_MESSAGE_MODULE_CREATE::args` as `configuration`] */
/* SRS_OOP_MODULE_027_0xx: [`invoke_add_module_process` shall create the out-of-process module by calling `MODULE_HANDLE Module_Create(BROKER_HANDLE broker, const void * configuration)` using the `oop_module_handle` as `broker` and the parsed configuration value or `CONTROL_MESSAGE_MODULE_CREATE::args` as `configuration`] */
/* SRS_OOP_MODULE_027_0xx: [If unable to create the out-of-process module, `invoke_add_module_process` shall free the configuration (if applicable) and return a non-zero value] */
/* SRS_OOP_MODULE_027_0xx: [If both `Module_FreeConfiguration` and `Module_ParseConfigurationFromJson` were provided, `invoke_add_module_process` shall free the configuration by calling `void Module_FreeConfiguration(void * configuration)` using the value returned from `Module_ParseConfigurationFromJson` as `configuration`] */
/* SRS_OOP_MODULE_027_0xx: [If no errors are encountered, `invoke_add_module_process` shall return zero] */
int
invoke_add_module_process (
    OOP_MODULE_HANDLE oop_module,
    const char * json_config
) {
    void * create_parameters = NULL;
    int result;

    // Call Module_ParseConfigurationFromJson function
    if (((MODULE_API_1 *)oop_module->module.module_apis)->Module_ParseConfigurationFromJson) {
        create_parameters = ((MODULE_API_1 *)oop_module->module.module_apis)->Module_ParseConfigurationFromJson(json_config);
    } else {
        create_parameters = json_config;
    }

    // Call Module_Create function (with out-of-process module handle supplied as broker)
    if (NULL == (oop_module->module.module_handle = ((MODULE_API_1 *)oop_module->module.module_apis)->Module_Create((BROKER_HANDLE)oop_module, create_parameters)) ) {
        result = __LINE__;
    } else {
        result = 0;
    }
    
    // Call Module_FreeConfiguration function
    if (((MODULE_API_1 *)oop_module->module.module_apis)->Module_FreeConfiguration && ((MODULE_API_1 *)oop_module->module.module_apis)->Module_ParseConfigurationFromJson) {
        ((MODULE_API_1 *)oop_module->module.module_apis)->Module_FreeConfiguration(create_parameters);
    }

    return result;
}


/* SRS_OOP_MODULE_027_0xx: [`worker_thread` shall obtain the thread mutex in order to initialize the thread by calling `LOCK_RESULT Lock(LOCK_HANDLE handle)`] */
/* SRS_OOP_MODULE_027_0xx: [If unable to obtain the mutex, then `worker_thread` shall return a non-zero value] */
/* SRS_OOP_MODULE_027_0xx: [`worker_thread` shall release the thread mutex upon entering the loop by calling `LOCK_RESULT Unlock(LOCK_HANDLE handle)`] */
/* SRS_OOP_MODULE_027_0xx: [If unable to release the mutex, then `worker_thread` shall exit the thread and return a non-zero value] */
/* SRS_OOP_MODULE_027_0xx: [`worker_thread` shall invoke asynchronous processing by calling `void OopModule_DoWork(OOP_MODULE_HANDLE oop_module)`] */
/* SRS_OOP_MODULE_027_0xx: [`worker_thread` shall yield its remaining quantum by calling `void THREADAPI_Sleep(unsigned int milliseconds)`] */
/* SRS_OOP_MODULE_027_0xx: [`worker_thread` shall obtain the thread mutex in order to check for a halt signal by calling `LOCK_RESULT Lock(LOCK_HANDLE handle)`] */
/* SRS_OOP_MODULE_027_0xx: [If unable to obtain the mutex, then `worker_thread` shall exit the thread return a non-zero value] */
/* SRS_OOP_MODULE_027_0xx: [If unable to obtain the mutex, then `worker_thread` shall exit the thread return a non-zero value] */
/* SRS_OOP_MODULE_027_0xx: [Once the halt signal is received, `worker_thread` shall release the thread mutex by calling `LOCK_RESULT Unlock(LOCK_HANDLE handle)`] */
/* SRS_OOP_MODULE_027_0xx: [If unable to release the mutex, then `worker_thread` shall return a non-zero value] */
/* SRS_OOP_MODULE_027_0xx: [If no errors are encountered, `worker_thread` shall return zero] */
/* SRS_OOP_MODULE_027_0xx: [`worker_thread` shall pass its return value to the caller by calling `void ThreadAPI_Exit(int res)`] */
int
worker_thread (
    void * thread_arg
) {
    int result;
    OOP_MODULE_HANDLE oop_module = (OOP_MODULE_HANDLE)thread_arg;

    if (LOCK_ERROR == Lock(oop_module->message_thread->mutex)) {
        LogError("%s: Failed to obtain mutex!", __FUNCTION__);
        result = __LINE__;
    } else {
        result = __LINE__;
        oop_module->message_thread->halt = false;
        for (;!oop_module->message_thread->halt;) {
            if (LOCK_ERROR == Unlock(oop_module->message_thread->mutex)) {
                LogError("%s: Failed to release mutex!", __FUNCTION__);
                result = __LINE__;
                break;
            } else {
                OopModule_DoWork(oop_module);
                ThreadAPI_Sleep(0);  // Release the CPU
                if (LOCK_ERROR == Lock(oop_module->message_thread->mutex)) {
                    LogError("%s: Failed to obtain mutex!", __FUNCTION__);
                    result = __LINE__;
                    break;
                }
            }
        }
        if (LOCK_ERROR == Unlock(oop_module->message_thread->mutex)) {
            LogError("%s: Failed to release mutex!", __FUNCTION__);
            result = __LINE__;
        }
    }
    ThreadAPI_Exit(result);

    return result;
}


/* SRS_OOP_MODULE_027_0xx: [If the creation process has already occurred, `process_module_create_message` shall do nothing and return zero] */
/* SRS_OOP_MODULE_027_0xx: [`process_module_create_message` shall connect to the message channels] */
/* SRS_OOP_MODULE_027_0xx: [If unable to connect to the message channels, `process_module_create_message` shall attempt to reply to the gateway with a connection error status and return a non-zero value] */
/* SRS_OOP_MODULE_027_0xx: [`process_module_create_message` shall invoke the "add module" process] */
/* SRS_OOP_MODULE_027_0xx: [If unable to complete the "add module" process, `process_module_create_message` shall disconnect from the message channels, attempt to reply to the gateway with a module creation error status and return a non-zero value] */
/* SRS_OOP_MODULE_027_0xx: [`process_module_create_message` shall reply to the gateway with a success status] */
/* SRS_OOP_MODULE_027_0xx: [If unable to contact the gateway, `process_module_create_message` disconnect from the message channels and return a non-zero value] */
/* SRS_OOP_MODULE_027_0xx: [If no errors are encountered, `process_module_create_message` shall return zero] */
int
process_module_create_message (
    OOP_MODULE_HANDLE oop_module,
    CONTROL_MESSAGE_MODULE_CREATE * message
) {
    int result;

    // Check to see if create has already been called
    if (0 != oop_module->message_channel_count) {
        // do nothing, create has already been processed
        result = 0;
    } else if (0 != connect_to_message_channels(oop_module, message->uris, message->uris_count)) {
        LogError("%s: Cannot connect to message channels!", __FUNCTION__);
        result = __LINE__;
        (void)send_control_reply(oop_module, (uint8_t)OOP_MODULE_GATEWAY_CONNECTION_ERROR);
    } else if (0 != invoke_add_module_process(oop_module, message->args)) {
        LogError("%s: Cannot create module!", __FUNCTION__);
        result = __LINE__;
        disconnect_from_message_channels(oop_module);
        (void)send_control_reply(oop_module, (uint8_t)OOP_MODULE_MODULE_CREATION_ERROR);
    } else if (0 != send_control_reply(oop_module, (uint8_t)OOP_MODULE_OK)) {
        LogError("%s: Unable to contact gateway!", __FUNCTION__);
        result = __LINE__;
        disconnect_from_message_channels(oop_module);
    } else {
        result = 0;
    }

    return result;
}


/* SRS_OOP_MODULE_027_0xx: [`send_control_reply` shall serialize a creation reply indicating the creation status by calling `size_t ControlMessage_ToByteArray(CONTROL MESSAGE * message, unsigned char * buf, size_t size)`] */
/* SRS_OOP_MODULE_027_0xx: [If unable to serialize the creation message reply, `send_control_reply` shall return a non-zero value] */
/* SRS_OOP_MODULE_027_0xx: [`send_control_reply` shall send the serialized message by calling `int nn_send(int s, const void * buf, size_t len, int flags)` using the serialized message as the `buf` parameter and the value returned from `ControlMessage_ToByteArray` as `len`] */
/* SRS_OOP_MODULE_027_0xx: [If unable to send the serialized message, `send_control_reply` shall free the serialized message and return a non-zero value] */
/* SRS_OOP_MODULE_027_0xx: [`send_control_reply` shall free the serialized message] */
/* SRS_OOP_MODULE_027_0xx: [If no errors are encountered, `send_control_reply` shall return zero] */
int
send_control_reply (
    OOP_MODULE_HANDLE oop_module,
    uint8_t response
) {
    int result;
    CONTROL_MESSAGE_MODULE_REPLY reply = {
        .base = {
            .type = CONTROL_MESSAGE_TYPE_MODULE_REPLY,
            .version = MODULE_API_VERSION_1,
        },
        .status = response,
    };
    unsigned char * message_buffer = NULL;
    size_t message_size;

    if (0 > (message_size = ControlMessage_ToByteArray(&reply, message_buffer, 0))) {
        LogError("%s: Unable to serialize message!", __FUNCTION__);
        result = __LINE__;
    } else {
        if (0 > (message_size = nn_send(oop_module->control_socket, message_buffer, message_size, NN_DONTWAIT))) {
            LogError("%s: Unable to send message to gateway process!", __FUNCTION__);
            result = __LINE__;
        } else {
            result = 0;
        }
        free(message_buffer);
    }

    return result;
}

