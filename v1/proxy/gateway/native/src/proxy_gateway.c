// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "proxy_gateway.h"
#include "broker.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <nanomsg/nn.h>
#include <nanomsg/pair.h>

#include <azure_c_shared_utility/gballoc.h>
#include <azure_c_shared_utility/lock.h>
#include <azure_c_shared_utility/threadapi.h>
#include <azure_c_shared_utility/xlogging.h>

#include "control_message.h"
#include "gateway.h"
#include "message.h"

typedef enum REMOTE_MODULE_RESULT_TAG {
    REMOTE_MODULE_DETACH = -1,
    REMOTE_MODULE_OK,
    REMOTE_MODULE_GATEWAY_CONNECTION_ERROR,
    REMOTE_MODULE_MODULE_CREATION_ERROR,
} REMOTE_MODULE_RESULT;

typedef struct MESSAGE_THREAD_TAG * MESSAGE_THREAD_HANDLE;

int
connect_to_message_channel (
    REMOTE_MODULE_HANDLE remote_module,
    const MESSAGE_URI * channel_uri
);

void
disconnect_from_message_channel (
    REMOTE_MODULE_HANDLE remote_module
);

int
invoke_add_module_procedure (
    REMOTE_MODULE_HANDLE remote_module,
    const char * json_config
);

int
process_module_create_message (
    REMOTE_MODULE_HANDLE remote_module,
    const CONTROL_MESSAGE_MODULE_CREATE * message
);

int
send_control_reply (
    REMOTE_MODULE_HANDLE remote_module,
    uint8_t response
);

int
worker_thread(
    void * thread_arg
);

typedef struct MESSAGE_THREAD_TAG {
    bool halt;
    LOCK_HANDLE mutex;
    THREAD_HANDLE thread;
} MESSAGE_THREAD;

typedef struct REMOTE_MODULE_TAG {
	int control_endpoint;
	int control_socket;
    int message_endpoint;
    int message_socket;
    MESSAGE_THREAD_HANDLE message_thread;
    MODULE module;
} REMOTE_MODULE;

static size_t strnlen_(const char* s, size_t max)
{
    if (!s) return 0;

    size_t i;
    for (i = 0; i < max && s[i] != 0; ++i) {}
    return i;
}

REMOTE_MODULE_HANDLE
ProxyGateway_Attach (
    const MODULE_API * module_apis,
    const char * connection_id
) {
    REMOTE_MODULE_HANDLE remote_module;
    if (NULL == module_apis) {
        /* Codes_SRS_PROXY_GATEWAY_027_000: [Prerequisite Check - If the `module_apis` parameter is `NULL`, then `ProxyGateway_Attach` shall do nothing and return `NULL`] */
        LogError("%s: NULL parameter - module_apis", __FUNCTION__);
        remote_module = NULL;
    } else if (MODULE_API_VERSION_1 < (int)module_apis->version) {
        /* Codes_SRS_PROXY_GATEWAY_027_001: [Prerequisite Check - If the `module_apis` version is beyond `MODULE_API_VERSION_1`, then `ProxyGateway_Attach` shall do nothing and return `NULL`] */
        LogError("%s: Incompatible API version: %d!", __FUNCTION__, (1 + (int)module_apis->version));
        remote_module = NULL;
    } else if (NULL == ((MODULE_API_1 *)module_apis)->Module_Create) {
        /* Codes_SRS_PROXY_GATEWAY_027_002: [Prerequisite Check - If the `module_apis` interface fails to provide `Module_Create`, then `ProxyGateway_Attach` shall do nothing and return `NULL`] */
        LogError("%s: Required interface not met - module_apis->Module_Create", __FUNCTION__);
        remote_module = NULL;
    } else if (NULL == ((MODULE_API_1 *)module_apis)->Module_Destroy) {
        /* Codes_SRS_PROXY_GATEWAY_027_003: [Prerequisite Check - If the `module_apis` interface fails to provide `Module_Destroy`, then `ProxyGateway_Attach` shall do nothing and return `NULL`] */
        LogError("%s: Required interface not met - module_apis->Module_Destroy", __FUNCTION__);
        remote_module = NULL;
    } else if (NULL == ((MODULE_API_1 *)module_apis)->Module_Receive) {
        /* Codes_SRS_PROXY_GATEWAY_027_004: [Prerequisite Check - If the `module_apis` interface fails to provide `Module_Receive`, then `ProxyGateway_Attach` shall do nothing and return `NULL`] */
        LogError("%s: Required interface not met - module_apis->Module_Receive", __FUNCTION__);
        remote_module = NULL;
    } else if (NULL == connection_id) {
        /* Codes_SRS_PROXY_GATEWAY_027_005: [Prerequisite Check - If the `connection_id` parameter is `NULL`, then `ProxyGateway_Attach` shall do nothing and return `NULL`] */
        LogError("%s: NULL parameter - connection_id", __FUNCTION__);
        remote_module = NULL;
    } else if (GATEWAY_CONNECTION_ID_MAX + 1 == strnlen_(connection_id, GATEWAY_CONNECTION_ID_MAX + 1)) {
        /* Codes_SRS_PROXY_GATEWAY_027_006: [Prerequisite Check - If the `connection_id` parameter is longer than `GATEWAY_CONNECTION_ID_MAX`, then `ProxyGateway_Attach` shall do nothing and return `NULL`] */
        LogError("%s: connection_id exceeds GATEWAY_CONNECTION_ID_MAX in length", __FUNCTION__);
        remote_module = NULL;
    /* Codes_SRS_PROXY_GATEWAY_027_007: [`ProxyGateway_Attach` shall allocate the memory required to support its instance data] */
    } else if (NULL == (remote_module = (REMOTE_MODULE_HANDLE)calloc(1, sizeof(REMOTE_MODULE)))) {
        /* Codes_SRS_PROXY_GATEWAY_027_008: [If memory allocation fails for the instance data, then `ProxyGateway_Attach` shall return `NULL`] */
        LogError("%s: Unable to allocate memory!", __FUNCTION__);
    } else {
        static const size_t ENDPOINT_DECORATION_SIZE = sizeof("ipc://") - 1;

        const size_t control_channel_uri_size = strlen(connection_id) + ENDPOINT_DECORATION_SIZE + 1;
        char * control_channel_uri;

        // Transform the connection id into a nanomsg URI
        /* Codes_SRS_PROXY_GATEWAY_027_009: [`ProxyGateway_Attach` shall allocate the memory required to formulate the connection string to the Azure IoT Gateway] */
        if (NULL == (control_channel_uri = (char *)malloc(control_channel_uri_size))) {
            /* Codes_SRS_PROXY_GATEWAY_027_010: [If memory allocation fails for the connection string, then `ProxyGateway_Attach` shall free any previously allocated memory and return `NULL`] */
            LogError("%s: Unable to allocate memory!", __FUNCTION__);
            free(remote_module);
            remote_module = NULL;
        } else if (NULL == strcpy(control_channel_uri, "ipc://")) {
            LogError("%s: Unable to compose channel uri prefix!", __FUNCTION__);
            free(remote_module);
            remote_module = NULL;
        } else if (NULL == strncat(control_channel_uri, connection_id, GATEWAY_CONNECTION_ID_MAX + 1)) {
            LogError("%s: Unable to compose channel uri body!", __FUNCTION__);
            free(remote_module);
            remote_module = NULL;
        } else {
            /* Codes_SRS_PROXY_GATEWAY_027_011: [`ProxyGateway_Attach` shall create a socket for the Azure IoT Gateway control channel by calling `int nn_socket(int domain, int protocol)` with `AF_SP` as the `domain` and `NN_PAIR` as the `protocol`] */
            if (-1 == (remote_module->control_socket = nn_socket(AF_SP, NN_PAIR))) {
                /* Codes_SRS_PROXY_GATEWAY_027_012: [If the call to `nn_socket` returns -1, then `ProxyGateway_Attach` shall free any previously allocated memory and return `NULL`] */
                LogError("%s: Unable to create the gateway socket!", __FUNCTION__);
                free(remote_module);
                remote_module = NULL;
            /* Codes_SRS_PROXY_GATEWAY_027_013: [`ProxyGateway_Attach` shall bind to the Azure IoT Gateway control channel by calling `int nn_bind(int s, const char * addr)` with the newly created socket as `s` and the newly formulated connection string as `addr`] */
            } else if (0 > (remote_module->control_endpoint = nn_bind(remote_module->control_socket, control_channel_uri))) {
                /* Codes_SRS_PROXY_GATEWAY_027_014: [If the call to `nn_bind` returns a negative value, then `ProxyGateway_Attach` shall close the socket, free any previously allocated memory and return `NULL`] */
                LogError("%s: Unable to connect to the gateway control channel!", __FUNCTION__);
				nn_close(remote_module->control_socket);
                free(remote_module);
                remote_module = NULL;
            } else {
                // Save the module API
                remote_module->module.module_apis = module_apis;

                // Initialize remaining fields
                remote_module->message_socket = -1;
                remote_module->message_endpoint = -1;
            }
        }
        /* Codes_SRS_PROXY_GATEWAY_027_015: [`ProxyGateway_Attach` shall release the memory required to formulate the connection string] */
        free(control_channel_uri);
    }

    /* Codes_SRS_PROXY_GATEWAY_027_016: [If no errors are encountered, then `ProxyGateway_Attach` return a handle to the OopModule instance] */
    return remote_module;
}


void
ProxyGateway_Detach (
    REMOTE_MODULE_HANDLE remote_module
) {
    if (NULL == remote_module) {
        /* Codes_SRS_PROXY_GATEWAY_027_058: [Prerequisite Check - If the `remote_module` parameter is `NULL`, then `ProxyGateway_Detach` shall do nothing] */
        LogError("%s: NULL parameter - remote_module!", __FUNCTION__);
    } else {
        // Ensure message thread handle is NULL
        if (NULL != remote_module->message_thread) {
            /* Codes_SRS_PROXY_GATEWAY_027_059: [If the worker thread is active, then `ProxyGateway_Detach` shall attempt to halt the worker thread] */
            (void)ProxyGateway_HaltWorkerThread(remote_module);
            if (NULL != remote_module->message_thread) {
                /* Codes_SRS_PROXY_GATEWAY_027_060: [If unable to halt the worker thread, `ProxyGateway_Detach` shall forcibly free the memory allocated to the worker thread] */
                LogError("%s: Unable to gracefully halt worker thread!", __FUNCTION__);
                free(remote_module->message_thread);
                remote_module->message_thread = NULL;
            }
        }

        /* Codes_SRS_PROXY_GATEWAY_027_061: [`ProxyGateway_Detach` shall attempt to notify the Azure IoT Gateway of the detachment] */
        (void)send_control_reply(remote_module, (uint8_t)REMOTE_MODULE_DETACH);
		ThreadAPI_Sleep(1000);

        /* Codes_SRS_PROXY_GATEWAY_027_062: [`ProxyGateway_Detach` shall disconnect from the Azure IoT Gateway message channels] */
        disconnect_from_message_channel(remote_module);
        /* Codes_SRS_PROXY_GATEWAY_027_063: [`ProxyGateway_Detach` shall shutdown the Azure IoT Gateway control channel by calling `int nn_shutdown(int s, int how)`] */
        (void)nn_shutdown(remote_module->control_socket, remote_module->control_endpoint);
        remote_module->control_endpoint = 0;
        /* Codes_SRS_PROXY_GATEWAY_027_064: [`ProxyGateway_Detach` shall close the Azure IoT Gateway control socket by calling `int nn_close(int s)`] */
        (void)nn_close(remote_module->control_socket);
        remote_module->control_socket = 0;
        /* Codes_SRS_PROXY_GATEWAY_027_065: [`ProxyGateway_Detach` shall free the remaining memory dedicated to its instance data] */
        free(remote_module);
        remote_module = NULL;
    }

    return;
}


void
ProxyGateway_DoWork (
    REMOTE_MODULE_HANDLE remote_module
) {
    if (NULL == remote_module) {
        /* Codes_SRS_PROXY_GATEWAY_027_026: [Prerequisite Check - If the `remote_module` parameter is `NULL`, then `ProxyGateway_DoWork` shall do nothing] */
        LogError("%s: NULL parameter - remote_module!", __FUNCTION__);
    } else {
        int32_t bytes_received;
        void * control_message = NULL;

        /* Codes_SRS_PROXY_GATEWAY_027_027: [Control Channel - `ProxyGateway_DoWork` shall poll the gateway control channel by calling `int nn_recv(int s, void * buf, size_t len, int flags)` with the control socket for `s`, `NULL` for `buf`, `NN_MSG` for `len` and NN_DONTWAIT for `flags`] */
        if (0 > (bytes_received = nn_recv(remote_module->control_socket, &control_message, NN_MSG, NN_DONTWAIT))) {
            if (EAGAIN == nn_errno()) {
                /* Codes_SRS_PROXY_GATEWAY_027_028: [Control Channel - If no message is available, then `ProxyGateway_DoWork` shall abandon the control channel request] */
            } else {
                /* Codes_SRS_PROXY_GATEWAY_027_066: [Control Channel - If an error occurred when polling the gateway, then `ProxyGateway_DoWork` shall signal the gateway abandon the control channel request] */
                LogError("%s: Unexpected error received from the control channel!", __FUNCTION__);
                (void)send_control_reply(remote_module, (uint8_t)REMOTE_MODULE_GATEWAY_CONNECTION_ERROR);
            }
        } else {
            CONTROL_MESSAGE * structured_control_message;

            /* Codes_SRS_PROXY_GATEWAY_027_029: [Control Channel - If a control message was received, then `ProxyGateway_DoWork` will parse that message by calling `CONTROL_MESSAGE * ControlMessage_CreateFromByteArray(const unsigned char * source, size_t size)` with the buffer received from `nn_recv` as `source` and return value from `nn_recv` as `size`] */
            if (NULL == (structured_control_message = ControlMessage_CreateFromByteArray((const unsigned char *)control_message, bytes_received))) {
                /* Codes_SRS_PROXY_GATEWAY_027_030: [Control Channel - If unable to parse the control message, then `ProxyGateway_DoWork` shall signal the gateway, free any previously allocated memory and abandon the control channel request] */
                LogError("%s: Unable to parse control message!", __FUNCTION__);
                (void)send_control_reply(remote_module, (uint8_t)REMOTE_MODULE_GATEWAY_CONNECTION_ERROR);
            } else {
                // Route control channel messages to appropriate functions
                switch (structured_control_message->type) {
                  case CONTROL_MESSAGE_TYPE_MODULE_CREATE:
                    /* Codes_SRS_PROXY_GATEWAY_027_031: [Control Channel - If the message type is CONTROL_MESSAGE_TYPE_MODULE_CREATE, then `ProxyGateway_DoWork` shall process the create message] */
                    if (0 != process_module_create_message(remote_module, (const CONTROL_MESSAGE_MODULE_CREATE *)structured_control_message)) {
                        LogError("%s: Unable to process create message!", __FUNCTION__);
                    }
                    break;
                  case CONTROL_MESSAGE_TYPE_MODULE_START:
                    /* Codes_SRS_PROXY_GATEWAY_027_032: [Control Channel - If the message type is CONTROL_MESSAGE_TYPE_MODULE_START and `Module_Start` was provided, then `ProxyGateway_DoWork` shall call `void Module_Start(MODULE_HANDLE moduleHandle)`] */
                    if (((MODULE_API_1 *)remote_module->module.module_apis)->Module_Start) {
                        ((MODULE_API_1 *)remote_module->module.module_apis)->Module_Start(remote_module->module.module_handle);
                    }
                    break;
                  case CONTROL_MESSAGE_TYPE_MODULE_DESTROY:
                    /* Codes_SRS_PROXY_GATEWAY_027_033: [Control Channel - If the message type is CONTROL_MESSAGE_TYPE_MODULE_DESTROY, then `ProxyGateway_DoWork` shall call `void Module_Destroy(MODULE_HANDLE moduleHandle)`] */
                    ((MODULE_API_1 *)remote_module->module.module_apis)->Module_Destroy(remote_module->module.module_handle);
                    remote_module->module.module_handle = NULL;
                    /* Codes_SRS_PROXY_GATEWAY_027_034: [Control Channel - If the message type is CONTROL_MESSAGE_TYPE_MODULE_DESTROY, then `ProxyGateway_DoWork` shall disconnect from the message channel] */
                    disconnect_from_message_channel(remote_module);
                    break;
                  default: LogError("ERROR: REMOTE_MODULE - Received unsupported message type! [%d]\n", structured_control_message->type); break;
                }
                /* Codes_SRS_PROXY_GATEWAY_027_035: [Control Channel - `ProxyGateway_DoWork` shall free the resources held by the parsed control message by calling `void ControlMessage_Destroy(CONTROL_MESSAGE * message)` using the parsed control message as `message`] */
                ControlMessage_Destroy(structured_control_message);
            }
            /* Codes_SRS_PROXY_GATEWAY_027_036: [Control Channel - `ProxyGateway_DoWork` shall free the resources held by the gateway message by calling `int nn_freemsg(void * msg)` with the resulting buffer from the previous call to `nn_recv`] */
            (void)nn_freemsg(control_message);
        }

        /* Codes_SRS_PROXY_GATEWAY_027_037: [Message Channel - `ProxyGateway_DoWork` shall not check for messages, if the message socket is not available] */
        if ( 0 > remote_module->message_socket ) {
            // not connected to message channel
        } else {
            void * module_message = NULL;

            /* Codes_SRS_PROXY_GATEWAY_027_038: [Message Channel - `ProxyGateway_DoWork` shall poll the gateway message channel by calling `int nn_recv(int s, void * buf, size_t len, int flags)` with each message socket for `s`, `NULL` for `buf`, `NN_MSG` for `len` and NN_DONTWAIT for `flags`] */
            if (0 > (bytes_received = nn_recv(remote_module->message_socket, &module_message, NN_MSG, NN_DONTWAIT))) {
                /* Codes_SRS_PROXY_GATEWAY_027_039: [Message Channel - If no message is available or an error occurred, then `ProxyGateway_DoWork` shall abandon the message channel request] */
                if (EAGAIN == nn_errno()) {
                    // no messages available at this time
                } else {
                    LogError("%s: Unexpected error received from the message channel!", __FUNCTION__);
                }
            } else {
                MESSAGE_HANDLE structured_module_message;

                /* Codes_SRS_PROXY_GATEWAY_027_040: [Message Channel - If a module message was received, then `ProxyGateway_DoWork` will parse that message by calling `MESSAGE_HANDLE Message_CreateFromByteArray(const unsigned char * source, int32_t size)` with the buffer received from `nn_recv` as `source` and return value from `nn_recv` as `size`] */
                if (NULL == (structured_module_message = Message_CreateFromByteArray((const unsigned char *)module_message, bytes_received))) {
                    /* Codes_SRS_PROXY_GATEWAY_027_041: [Message Channel - If unable to parse the module message, then `ProxyGateway_DoWork` shall free any previously allocated memory and abandon the message channel request] */
                    LogError("%s: Unable to parse control message!", __FUNCTION__);
                } else {
                    /* Codes_SRS_PROXY_GATEWAY_027_042: [Message Channel - `ProxyGateway_DoWork` shall pass the structured message to the module by calling `void Module_Receive(MODULE_HANDLE moduleHandle)` using the parsed message as `moduleHandle`] */
                    ((MODULE_API_1 *)remote_module->module.module_apis)->Module_Receive(remote_module->module.module_handle, structured_module_message);
                    /* Codes_SRS_PROXY_GATEWAY_027_043: [Message Channel - `ProxyGateway_DoWork` shall free the resources held by the parsed module message by calling `void Message_Destroy(MESSAGE_HANDLE * message)` using the parsed module message as `message`] */
                    Message_Destroy(structured_module_message);
                }
                /* Codes_SRS_PROXY_GATEWAY_027_044: [Message Channel - `ProxyGateway_DoWork` shall free the resources held by the gateway message by calling `int nn_freemsg(void * msg)` with the resulting buffer from the previous call to `nn_recv`] */
                (void)nn_freemsg(module_message);
            }
        }
    }

    return;
}


int
ProxyGateway_HaltWorkerThread (
    REMOTE_MODULE_HANDLE remote_module
) {
    int result;

    /* Codes_SRS_PROXY_GATEWAY_027_045: [Prerequisite Check - If the `remote_module` parameter is `NULL`, then `ProxyGateway_HaltWorkerThread` shall return a non-zero value] */
    if (NULL == remote_module) {
        LogError("%s: NULL parameter - remote_module!", __FUNCTION__);
        result = __LINE__;
    // Check to confirm message thread is running
    } else if (NULL == remote_module->message_thread) {
        /* Codes_SRS_PROXY_GATEWAY_027_046: [Prerequisite Check - If a worker thread does not exist, then `ProxyGateway_HaltWorkerThread` shall return a non-zero value] */
        LogInfo("%s: Worker thread has not been initialized.", __FUNCTION__);
        result = __LINE__;
    /* Codes_SRS_PROXY_GATEWAY_027_047: [`ProxyGateway_HaltWorkerThread` shall obtain the thread mutex in order to signal the thread by calling `LOCK_RESULT Lock(LOCK_HANDLE handle)`] */
    } else if (LOCK_OK != Lock(remote_module->message_thread->mutex)) {
        /* Codes_SRS_PROXY_GATEWAY_027_048: [If unable to obtain the mutex, then `ProxyGateway_HaltWorkerThread` shall return a non-zero value] */
        LogError("%s: Unable to acquire mutex!", __FUNCTION__);
        result = __LINE__;
    } else {
        int thread_exit_result = -1;
        // Signal the message thread
        remote_module->message_thread->halt = true;
        
        /* Codes_SRS_PROXY_GATEWAY_027_049: [`ProxyGateway_HaltWorkerThread` shall release the thread mutex upon signalling by calling `LOCK_RESULT Unlock(LOCK_HANDLE handle)`] */
        if (LOCK_OK != Unlock(remote_module->message_thread->mutex)) {
            /* Codes_SRS_PROXY_GATEWAY_027_050: [If unable to release the mutex, then `ProxyGateway_HaltWorkerThread` shall return a non-zero value] */
            LogError("%s: Unable to release mutex!", __FUNCTION__);
            result = __LINE__;
        /* Codes_SRS_PROXY_GATEWAY_027_051: [`ProxyGateway_HaltWorkerThread` shall halt the thread by calling `THREADAPI_RESULT ThreadAPI_Join(THREAD_HANDLE handle, int * res)`] */
        } else if (THREADAPI_OK != ThreadAPI_Join(remote_module->message_thread->thread, &thread_exit_result)) {
            /* Codes_SRS_PROXY_GATEWAY_027_052: [If unable to join the thread, then `ProxyGateway_HaltWorkerThread` shall return a non-zero value] */
            LogError("%s: Unable to halt message thread!", __FUNCTION__);
            result = __LINE__;
        } else {
            /* Codes_SRS_PROXY_GATEWAY_027_053: [`ProxyGateway_HaltWorkerThread` shall free the thread mutex by calling `LOCK_RESULT Lock_Deinit(LOCK_HANDLE handle)`] */
            /* Codes_SRS_PROXY_GATEWAY_027_054: [If unable to free the thread mutex, then `ProxyGateway_HaltWorkerThread` shall ignore the result and continue processing] */
            (void)Lock_Deinit(remote_module->message_thread->mutex);
            /* Codes_SRS_PROXY_GATEWAY_027_055: [`ProxyGateway_HaltWorkerThread` shall free the memory allocated to the thread details] */
            free(remote_module->message_thread);
            remote_module->message_thread = NULL;
            
            if (0 != thread_exit_result) {
                /* Codes_SRS_PROXY_GATEWAY_027_056: [If an error is returned from the worker thread, then `ProxyGateway_HaltWorkerThread` shall return the worker thread's error code] */
                LogError("%s: Thread exited with fail code <%d>!", __FUNCTION__, thread_exit_result);
                result = thread_exit_result;
            } else {
                /* Codes_SRS_PROXY_GATEWAY_027_057: [If no errors are encountered, then `ProxyGateway_HaltWorkerThread` shall return zero] */
                result = 0;
            }
        }
    }
    return result;
}


int
ProxyGateway_StartWorkerThread (
	REMOTE_MODULE_HANDLE remote_module
) {
    int result;

    if (NULL == remote_module) {
        /* Codes_SRS_PROXY_GATEWAY_027_017: [Prerequisite Check - If the `remote_module` parameter is `NULL`, then `ProxyGateway_StartWorkerThread` shall do nothing and return a non-zero value] */
        LogError("%s: NULL parameter - remote_module!", __FUNCTION__);
        result = __LINE__;
    } else if (NULL != remote_module->message_thread) {
        /* Codes_SRS_PROXY_GATEWAY_027_018: [Prerequisite Check - If a work thread already exist for the given handle, then `ProxyGateway_StartWorkerThread` shall do nothing and return zero] */
        LogInfo("%s: Worker thread has already been initialized.", __FUNCTION__);
        result = 0;
    /* Codes_SRS_PROXY_GATEWAY_027_019: [`ProxyGateway_StartWorkerThread` shall allocate the memory required to support the worker thread] */
    } else if (NULL == (remote_module->message_thread = (MESSAGE_THREAD_HANDLE)calloc(1, sizeof(MESSAGE_THREAD)))) {
        /* Codes_SRS_PROXY_GATEWAY_027_020: [If memory allocation fails for the worker thread data, then `ProxyGateway_StartWorkerThread` shall return a non-zero value] */
        LogError("%s: Unable to allocate memory!", __FUNCTION__);
        result = __LINE__;
    /* Codes_SRS_PROXY_GATEWAY_027_021: [`ProxyGateway_StartWorkerThread` shall create a mutex by calling `LOCK_HANDLE Lock_Init(void)`] */
    } else if (NULL == (remote_module->message_thread->mutex = Lock_Init())) {
        /* Codes_SRS_PROXY_GATEWAY_027_022: [If a mutex is unable to be created, then `ProxyGateway_StartWorkerThread` shall free any previously allocated memory and return a non-zero value] */
        LogError("%s: Unable to create mutex!", __FUNCTION__);
        result = __LINE__;
        free(remote_module->message_thread);
        remote_module->message_thread = (MESSAGE_THREAD_HANDLE)NULL;
    /* Codes_SRS_PROXY_GATEWAY_027_023: [`ProxyGateway_StartWorkerThread` shall start a worker thread by calling `THREADAPI_RESULT ThreadAPI_Create(&THREAD_HANDLE threadHandle, THREAD_START_FUNC func, void * arg)` with an empty thread handle for `threadHandle`, a function that loops polling the messages for `func`, and `remote_module` for `arg`] */
    } else if (THREADAPI_OK != ThreadAPI_Create(&remote_module->message_thread->thread, worker_thread, remote_module)) {
        /* Codes_SRS_PROXY_GATEWAY_027_024: [If the worker thread failed to start, then `ProxyGateway_StartWorkerThread` shall free any previously allocated memory and return a non-zero value] */
        LogError("%s: Unable to create worker thread!", __FUNCTION__);
        result = __LINE__;
        (void)Lock_Deinit(remote_module->message_thread->mutex);
        free(remote_module->message_thread);
        remote_module->message_thread = (MESSAGE_THREAD_HANDLE)NULL;
    } else {
        /* Codes_SRS_PROXY_GATEWAY_027_025: [If no errors are encountered, then `ProxyGateway_StartWorkerThread` shall return zero] */
        result = 0;
    }

	return result;
}


/* Codes_SRS_BROKER_17_022: [ N/A - Broker_Publish shall Lock the modules lock. ] */
/* Codes_SRS_BROKER_17_023: [ N/A - Broker_Publish shall Unlock the modules lock. ] */
/* Codes_SRS_BROKER_17_026: [ N/A - Broker_Publish shall copy source into the beginning of the nanomsg buffer. ] */
BROKER_RESULT
Broker_Publish (
    BROKER_HANDLE broker,
    MODULE_HANDLE source,
    MESSAGE_HANDLE message
) {
    (void)source;
    REMOTE_MODULE_HANDLE remote_module = (REMOTE_MODULE_HANDLE)broker;
    BROKER_RESULT result;

    /* Codes_SRS_BROKER_13_030: [If broker or message is NULL the function shall return BROKER_INVALIDARG.] */
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
        /* Codes_SRS_BROKER_17_007: [ Broker_Publish shall clone the message. ] */
        MESSAGE_HANDLE msg = Message_Clone(message);
        /* Codes_SRS_BROKER_17_008: [ Broker_Publish shall serialize the message. ] */
        msg_size = Message_ToByteArray(message, NULL, 0);
        if (msg_size < 0)
        {
            /* Codes_SRS_BROKER_13_037: [ This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise. ] */
            LogError("unable to serialize a message [%p]", msg);
            Message_Destroy(msg);
            result = BROKER_ERROR;
        }
        else
        {
            /* Codes_SRS_BROKER_17_025: [ Broker_Publish shall allocate a nanomsg buffer the size of the serialized message + sizeof(MODULE_HANDLE). ] */
            buf_size = msg_size;
            void* nn_msg = nn_allocmsg(buf_size, 0);
            if (nn_msg == NULL)
            {
                /* Codes_SRS_BROKER_13_037: [ This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise. ] */
                LogError("unable to serialize a message [%p]", msg);
                result = BROKER_ERROR;
            }
            else
            {
                unsigned char *nn_msg_bytes = (unsigned char *)nn_msg;
                /* Codes_SRS_BROKER_17_027: [ Broker_Publish shall serialize the message into the remainder of the nanomsg buffer. ] */
                Message_ToByteArray(message, nn_msg_bytes, msg_size);

                /* Codes_SRS_BROKER_17_010: [ Broker_Publish shall send a message on the publish_socket. ] */
                int nbytes = nn_send(remote_module->message_socket, &nn_msg, NN_MSG, 0);
                if (nbytes != buf_size)
                {
                    /* Codes_SRS_BROKER_13_037: [ This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise. ] */
                    LogError("unable to send a message [%p]", msg);
                    /* Codes_SRS_BROKER_17_012: [ Broker_Publish shall free the message. ] */
                    nn_freemsg(nn_msg);
                    result = BROKER_ERROR;
                }
                else
                {
                    result = BROKER_OK;
                }
            }
            /* Codes_SRS_BROKER_17_012: [ Broker_Publish shall free the message. ] */
            Message_Destroy(msg);
            /* Codes_SRS_BROKER_17_011: [ Broker_Publish shall free the serialized message data. ] */
        }

    }

    /* Codes_SRS_BROKER_13_037: [ This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise. ] */
    return result;
}


int
connect_to_message_channel (
    REMOTE_MODULE_HANDLE remote_module,
    const MESSAGE_URI * channel_uri
) {
    int result;

    /* SRS_PROXY_GATEWAY_027_0xx: [`connect_to_message_channel` shall create a socket for the Azure IoT Gateway message channel by calling `int nn_socket(int domain, int protocol)` with `AF_SP` as `domain` and `MESSAGE_URI::uri_type` as `protocol`] */
    if (-1 == (remote_module->message_socket = nn_socket(AF_SP, channel_uri->uri_type))) {
        /* SRS_PROXY_GATEWAY_027_0xx: [If a call to `nn_socket` returns -1, then `connect_to_message_channel` shall free any previously allocated memory, abandon the control message and prepare for the next create message] */
        LogError("%s: Unable to create the gateway socket!", __FUNCTION__);
        result = __LINE__;
    /* SRS_PROXY_GATEWAY_027_0xx: [`connect_to_message_channel` shall bind to the Azure IoT Gateway message channel by calling `int nn_bind(int s, const char * addr)` with the newly created socket as `s` and `MESSAGE_URI::uri` as `addr`] */
    } else if (0 > (remote_module->message_endpoint = nn_bind(remote_module->message_socket, channel_uri->uri))) {
        /* SRS_PROXY_GATEWAY_027_0xx: [If a call to `nn_connect` returns a negative value, then `connect_to_message_channel` shall free any previously allocated memory, abandon the control message and prepare for the next create message] */
        LogError("%s: Unable to connect to the gateway message channel!", __FUNCTION__);
        result = __LINE__;
        (void)nn_close(remote_module->message_socket);
        remote_module->message_socket = -1;
    } else {
        /* SRS_PROXY_GATEWAY_027_0xx: [If no errors are encountered, then `connect_to_message_channel` shall return zero] */
        result = 0;
    }

    return result;
}


void
disconnect_from_message_channel (
    REMOTE_MODULE_HANDLE remote_module
) {
    /* SRS_PROXY_GATEWAY_027_0xx: [`disconnect_from_message_channel` shall shutdown the Azure IoT Gateway message channel by calling `int nn_shutdown(int s, int how)`] */
    (void)nn_shutdown(remote_module->message_socket, remote_module->message_endpoint);
    remote_module->message_endpoint = -1;
    /* SRS_PROXY_GATEWAY_027_0xx: [`disconnect_from_message_channel` shall close the Azure IoT Gateway message socket by calling `int nn_close(int s)`] */
    (void)nn_close(remote_module->message_socket);
    remote_module->message_socket = -1;

    return;
}


int
invoke_add_module_procedure (
    REMOTE_MODULE_HANDLE remote_module,
    const char * json_config
) {
    int result;
    void * create_parameters = NULL;

    /* SRS_PROXY_GATEWAY_027_0xx: [If `Module_ParseConfigurationFromJson` was provided, `invoke_add_module_procedure` shall parse the configuration by calling `void * Module_ParseConfigurationFromJson(const char * configuration)` using the `CONTROL_MESSAGE_MODULE_CREATE::args` as `configuration`] */
    if (((MODULE_API_1 *)remote_module->module.module_apis)->Module_ParseConfigurationFromJson) {
        create_parameters = ((MODULE_API_1 *)remote_module->module.module_apis)->Module_ParseConfigurationFromJson(json_config);
    } else {
        create_parameters = (void *)json_config;
    }

    /* SRS_PROXY_GATEWAY_027_0xx: [`invoke_add_module_procedure` shall create the remote module by calling `MODULE_HANDLE Module_Create(BROKER_HANDLE broker, const void * configuration)` using the `remote_module_handle` as `broker` and the parsed configuration value or `CONTROL_MESSAGE_MODULE_CREATE::args` as `configuration`] */
    if (NULL == (remote_module->module.module_handle = ((MODULE_API_1 *)remote_module->module.module_apis)->Module_Create((BROKER_HANDLE)remote_module, create_parameters))) {
        result = __LINE__;
    } else {
        /* SRS_PROXY_GATEWAY_027_0xx: [If no errors are encountered, `invoke_add_module_procedure` shall return zero] */
        result = 0;
    }
    
    // Call Module_FreeConfiguration function
    if (((MODULE_API_1 *)remote_module->module.module_apis)->Module_FreeConfiguration && ((MODULE_API_1 *)remote_module->module.module_apis)->Module_ParseConfigurationFromJson) {
        /* SRS_PROXY_GATEWAY_027_0xx: [If unable to create the remote module, `invoke_add_module_procedure` shall free the configuration (if applicable) and return a non-zero value] */
        /* SRS_PROXY_GATEWAY_027_0xx: [If both `Module_FreeConfiguration` and `Module_ParseConfigurationFromJson` were provided, `invoke_add_module_procedure` shall free the configuration by calling `void Module_FreeConfiguration(void * configuration)` using the value returned from `Module_ParseConfigurationFromJson` as `configuration`] */
        ((MODULE_API_1 *)remote_module->module.module_apis)->Module_FreeConfiguration(create_parameters);
    }

    return result;
}


int
process_module_create_message (
    REMOTE_MODULE_HANDLE remote_module,
    const CONTROL_MESSAGE_MODULE_CREATE * message
) {
    int result;

    /* SRS_PROXY_GATEWAY_027_0xx: [Prerequisite Check - If the `gateway_message_version` is greater than 1, then `process_module_create_message` shall do nothing and return a non-zero value] */
    if (1 < message->gateway_message_version) {
        LogError("%s: Incompatible create message version: %u!", __FUNCTION__, message->gateway_message_version);
        result = __LINE__;
        (void)send_control_reply(remote_module, (uint8_t)REMOTE_MODULE_GATEWAY_CONNECTION_ERROR);
    } else {
        // Check to see if create has already been called
        if (NULL != remote_module->module.module_handle) {
            /* SRS_PROXY_GATEWAY_027_0xx: [Special Condition - If the creation process has already occurred, `process_module_create_message` shall destroy the module and disconnect from the message channel and continue processing the creation message] */
            ((MODULE_API_1 *)remote_module->module.module_apis)->Module_Destroy(remote_module->module.module_handle);
            remote_module->module.module_handle = NULL;
            disconnect_from_message_channel(remote_module);
        }

        /* SRS_PROXY_GATEWAY_027_0xx: [`process_module_create_message` shall connect to the message channels] */
        if (0 != connect_to_message_channel(remote_module, &message->uri)) {
            /* SRS_PROXY_GATEWAY_027_0xx: [If unable to connect to the message channels, `process_module_create_message` shall attempt to reply to the gateway with a connection error status and return a non-zero value] */
            LogError("%s: Cannot connect to message channels!", __FUNCTION__);
            result = __LINE__;
            (void)send_control_reply(remote_module, (uint8_t)REMOTE_MODULE_GATEWAY_CONNECTION_ERROR);
        /* SRS_PROXY_GATEWAY_027_0xx: [`process_module_create_message` shall invoke the "add module" process] */
        } else if (0 != invoke_add_module_procedure(remote_module, message->args)) {
            /* SRS_PROXY_GATEWAY_027_0xx: [If unable to complete the "add module" process, `process_module_create_message` shall disconnect from the message channels, attempt to reply to the gateway with a module creation error status and return a non-zero value] */
            LogError("%s: Cannot create module!", __FUNCTION__);
            result = __LINE__;
            disconnect_from_message_channel(remote_module);
            (void)send_control_reply(remote_module, (uint8_t)REMOTE_MODULE_MODULE_CREATION_ERROR);
        /* SRS_PROXY_GATEWAY_027_0xx: [`process_module_create_message` shall reply to the gateway with a success status] */
        } else if (0 != send_control_reply(remote_module, (uint8_t)REMOTE_MODULE_OK)) {
            /* SRS_PROXY_GATEWAY_027_0xx: [If unable to contact the gateway, `process_module_create_message` disconnect from the message channels and return a non-zero value] */
            LogError("%s: Unable to contact gateway!", __FUNCTION__);
            result = __LINE__;
            disconnect_from_message_channel(remote_module);
        } else {
            /* SRS_PROXY_GATEWAY_027_0xx: [If no errors are encountered, `process_module_create_message` shall return zero] */
            result = 0;
        }
    }

    return result;
}


int
send_control_reply (
    REMOTE_MODULE_HANDLE remote_module,
    uint8_t response
) {
    int result;
    CONTROL_MESSAGE_MODULE_REPLY reply = {
        .base = {
            .type = CONTROL_MESSAGE_TYPE_MODULE_REPLY,
            .version = CONTROL_MESSAGE_VERSION_1,
        },
        .status = response,
    };
    unsigned char * message_buffer = NULL;
    int32_t message_size;

    /* SRS_PROXY_GATEWAY_027_0xx: [`send_control_reply` shall calculate the serialized message size by calling `size_t ControlMessage_ToByteArray(CONTROL MESSAGE * message, unsigned char * buf, size_t size)`] */
    if (0 > (message_size = ControlMessage_ToByteArray((CONTROL_MESSAGE *)&reply, message_buffer, 0))) {
        /* SRS_PROXY_GATEWAY_027_0xx: [If unable to calculate the serialized message size, `send_control_reply` shall return a non-zero value] */
        LogError("%s: Unable to calculate serialized message size!", __FUNCTION__);
        result = __LINE__;
    } else {
        /* SRS_PROXY_GATEWAY_027_0xx: [`send_control_reply` allocate the necessary space for the nano message, by calling `void * nn_allocmsg(size_t size, int type)` using the previously acquired message size for `size` and `0` for `type`] */
        if (NULL == (message_buffer = nn_allocmsg(message_size, 0))) {
            /* SRS_PROXY_GATEWAY_027_0xx: [If unable to allocate memory, `send_control_reply` shall return a non-zero value] */
            LogError("%s: Unable to allocate message!", __FUNCTION__);
            result = __LINE__;
        /* SRS_PROXY_GATEWAY_027_0xx: [`send_control_reply` shall serialize a creation reply indicating the creation status by calling `size_t ControlMessage_ToByteArray(CONTROL MESSAGE * message, unsigned char * buf, size_t size)`] */
        } else if (0 > ControlMessage_ToByteArray((CONTROL_MESSAGE *)&reply, message_buffer, message_size)) {
            /* SRS_PROXY_GATEWAY_027_0xx: [If unable to serialize the creation message reply, `send_control_reply` shall return a non-zero value] */
            LogError("%s: Unable to serialize message!", __FUNCTION__);
            result = __LINE__;
        /* SRS_PROXY_GATEWAY_027_0xx: [`send_control_reply` shall send the serialized message by calling `int nn_send(int s, const void * buf, size_t len, int flags)` using the serialized message as the `buf` parameter and the value returned from `ControlMessage_ToByteArray` as `len`] */
        } else if (0 > (message_size = nn_send(remote_module->control_socket, &message_buffer, NN_MSG, NN_DONTWAIT))) {
            /* SRS_PROXY_GATEWAY_027_0xx: [If unable to send the serialized message, `send_control_reply` shall release the nano message by calling `int nn_freemsg(void * msg)` using the previously acquired nano message pointer as `msg` and return a non-zero value] */
            LogError("%s: Unable to send message to gateway process!", __FUNCTION__);
            result = __LINE__;
            nn_freemsg(message_buffer);
        } else {
            /* SRS_PROXY_GATEWAY_027_0xx: [If no errors are encountered, `send_control_reply` shall return zero] */
            result = 0;
        }
    }

    return result;
}


/* SRS_PROXY_GATEWAY_027_0xx: [`worker_thread` shall obtain the thread mutex in order to initialize the thread by calling `LOCK_RESULT Lock(LOCK_HANDLE handle)`] */
/* SRS_PROXY_GATEWAY_027_0xx: [If unable to obtain the mutex, then `worker_thread` shall return a non-zero value] */
/* SRS_PROXY_GATEWAY_027_0xx: [`worker_thread` shall release the thread mutex upon entering the loop by calling `LOCK_RESULT Unlock(LOCK_HANDLE handle)`] */
/* SRS_PROXY_GATEWAY_027_0xx: [If unable to release the mutex, then `worker_thread` shall exit the thread and return a non-zero value] */
/* SRS_PROXY_GATEWAY_027_0xx: [`worker_thread` shall invoke asynchronous processing by calling `void ProxyGateway_DoWork(REMOTE_MODULE_HANDLE remote_module)`] */
/* SRS_PROXY_GATEWAY_027_0xx: [`worker_thread` shall yield its remaining quantum by calling `void THREADAPI_Sleep(unsigned int milliseconds)`] */
/* SRS_PROXY_GATEWAY_027_0xx: [`worker_thread` shall obtain the thread mutex in order to check for a halt signal by calling `LOCK_RESULT Lock(LOCK_HANDLE handle)`] */
/* SRS_PROXY_GATEWAY_027_0xx: [If unable to obtain the mutex, then `worker_thread` shall exit the thread return a non-zero value] */
/* SRS_PROXY_GATEWAY_027_0xx: [If unable to obtain the mutex, then `worker_thread` shall exit the thread return a non-zero value] */
/* SRS_PROXY_GATEWAY_027_0xx: [Once the halt signal is received, `worker_thread` shall release the thread mutex by calling `LOCK_RESULT Unlock(LOCK_HANDLE handle)`] */
/* SRS_PROXY_GATEWAY_027_0xx: [If unable to release the mutex, then `worker_thread` shall return a non-zero value] */
/* SRS_PROXY_GATEWAY_027_0xx: [If no errors are encountered, `worker_thread` shall return zero] */
/* SRS_PROXY_GATEWAY_027_0xx: [`worker_thread` shall pass its return value to the caller by calling `void ThreadAPI_Exit(int res)`] */
int
worker_thread (
    void * thread_arg
) {
    int result;
    REMOTE_MODULE_HANDLE remote_module = (REMOTE_MODULE_HANDLE)thread_arg;

    if (LOCK_ERROR == Lock(remote_module->message_thread->mutex)) {
        LogError("%s: Failed to obtain mutex!", __FUNCTION__);
        result = __LINE__;
    }
    else {
        result = __LINE__;
        remote_module->message_thread->halt = false;
        for (; !remote_module->message_thread->halt;) {
            if (LOCK_ERROR == Unlock(remote_module->message_thread->mutex)) {
                LogError("%s: Failed to release mutex!", __FUNCTION__);
                result = __LINE__;
                break;
            }
            else {
                ProxyGateway_DoWork(remote_module);
                ThreadAPI_Sleep(0);  // Release the CPU
                if (LOCK_ERROR == Lock(remote_module->message_thread->mutex)) {
                    LogError("%s: Failed to obtain mutex!", __FUNCTION__);
                    result = __LINE__;
                    break;
                }
            }
        }
        if (LOCK_ERROR == Unlock(remote_module->message_thread->mutex)) {
            LogError("%s: Failed to release mutex!", __FUNCTION__);
            result = __LINE__;
        }
        else {
            result = 0;
        }
    }
    ThreadAPI_Exit(result);

    return result;
}

