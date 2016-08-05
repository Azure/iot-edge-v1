# Pub/Sub Message Broker

## Overview

The message broker (or just the "broker") is central to the gateway. The broker is responsible for sending and receiving messages between interested parties. In the case of the gateway, the interested parties are modules. This document describes the API for the message broker and what some of the threading implications are.

## References

* [Message Broker High-level Design](broker_hld.md)
* `module.h` - [Module API requirements](module.md)
* [Message API requirements](message_requirements.md)
* [nanomsg](http://nanomsg.org/)

## Tracking Modules

The message broker implementation shall use the following structure definition to track each associated module:

```C
typedef struct MESSAGE_BUS_MODULEINFO_TAG
{
    /**
     * Handle to a module associated with this broker.
     */
    MODULE_HANDLE             module;
    
    /**
     * The function dispatch table for this module.
     */
    CONST MODULE_APIS*        module_apis;
    
    /**
     * Handle to the thread on which this module’s message processing loop is
     * running.
     */
    THREAD_HANDLE           thread;
    
    /**
     * Handle to the queue of messages to be delivered to this module.
     */
    int                     receive_socket;
    
    /**
     * Lock used to synchronize access to nn_recv call.
     */
    LOCK_HANDLE             socket_lock;
    
    /**
     * Message publish worker will keep running until this signal is sent.
     */
    STRING_HANDLE           quit_message_guid;
}MESSAGE_BUS_MODULEINFO;
```

## Message Broker API

```C
typedef struct BROKER_HANDLE_DATA_TAG* BROKER_HANDLE;

#define BROKER_RESULT_VALUES \
    BROKER_OK, \
    BROKER_ERROR, \
    BROKER_INVALIDARG

DEFINE_ENUM(BROKER_RESULT, BROKER_RESULT_VALUES);

extern BROKER_HANDLE MESSAGE_extern BROKER_HANDLE Broker_Create(void);
extern void Broker_IncRef(BROKER_HANDLE bus);
extern void Broker_DecRef(BUS_HANDLE bus);
extern BROKER_RESULT Broker_Publish(BROKER_HANDLE bus, MODULE_HANDLE source, MESSAGE_HANDLE message);
extern BROKER_RESULT Broker_AddModule(BROKER_HANDLE bus, const MODULE* module);
extern BROKER_RESULT Broker_RemoveModule(BROKER_HANDLE bus, const MODULE* module);
extern void Broker_Destroy(BROKER_HANDLE bus);
```

## Broker_Create
```C
BROKER_HANDLE Broker_Create(void)
```

**SRS_MESSAGE_BUS_13_001: [** This API shall yield a `BROKER_HANDLE` representing the newly created message broker. This handle value shall not be equal to `NULL` when the API call is successful. **]**

**SRS_MESSAGE_BUS_13_003: [** This function shall return `NULL` if an underlying API call to the platform causes an error. **]**

The backing structure for a message broker handle is defined as follows:

```C
typedef struct BROKER_HANDLE_DATA_TAG
{
    /**
     * List of modules that are attached to this message broker. Each element in this
     * vector is an instance of MESSAGE_BUS_MODULEINFO.
     */
    VECTOR_HANDLE           modules;
    
    /**
     * Lock used to synchronize access to the 'modules' field.
     */
    LOCK_HANDLE             modules_lock;

    /**
     * Socket to publish messages to broker.
     */
    int                     publish_socket;

    /**
     * URL of message broker binding.
     */
    STRING_HANDLE           url;
}BROKER_HANDLE_DATA;
```

**SRS_MESSAGE_BUS_13_067: [** `Broker_Create` shall `malloc` a new instance of `BUS_HANDLE_DATA` and return `NULL` if it fails. **]**

**SRS_MESSAGE_BUS_13_007: [** `Broker_Create` shall initialize `BUS_HANDLE_DATA::modules` with a valid `VECTOR_HANDLE`. **]**

**SRS_MESSAGE_BUS_13_023: [** `Broker_Create` shall initialize `BUS_HANDLE_DATA::modules_lock` with a valid `LOCK_HANDLE`. **]**

**SRS_MESSAGE_BUS_17_001: [** `Broker_Create` shall initialize a socket for publishing messages. **]**
 
**SRS_MESSAGE_BUS_17_002: [**  `Broker_Create` shall create a unique id. **]**

**SRS_MESSAGE_BUS_17_003: [** `Broker_Create` shall initialize a url consisting of "inproc://" + unique id. **]**

**SRS_MESSAGE_BUS_17_004: [** `Broker_Create` shall bind the socket to the `BROKER_HANDLE_DATA::url`. **]**

## Broker_IncRef

```C
void Broker_IncRef(BROKER_HANDLE bus);
```
MessageBus_Clone creates a clone of the message broker handle.

**SRS_MESSAGE_BUS_13_108: [** If `bus` is `NULL` then `Broker_IncRef` shall do nothing. **]**

**SRS_MESSAGE_BUS_13_109: [** Otherwise, `Broker_IncRef` shall increment the internal ref count. **]**

## module_publish_worker

```C
static void module_publish_worker(void* user_data)
```

**SRS_MESSAGE_BUS_13_026: [** This function shall assign `user_data` to a local variable called `module_info` of type `MESSAGE_BUS_MODULEINFO*`. **]**

**SRS_MESSAGE_BUS_13_089: [** This function shall acquire the lock on `module_info->socket_lock`. **]**

**SRS_MESSAGE_BUS_02_004: [** If acquiring the lock fails, then module_publish_worker shall return. **]**

**SRS_MESSAGE_BUS_13_068: [** This function shall run a loop that keeps running until `module_info->quit_message_guid` is sent to the thread. **]**

**SRS_MESSAGE_BUS_13_091: [** The function shall unlock `module_info->socket_lock`. **]**

**SRS_MESSAGE_BUS_17_016: [** If releasing the lock fails, then module_publish_worker shall return. **]**

**SRS_MESSAGE_BUS_17_005: [** For every iteration of the loop, the function shall wait on the `receive_socket` for messages. **]**

**SRS_MESSAGE_BUS_17_006: [** An error on receiving a message shall terminate the loop. **]**

**SRS_MESSAGE_BUS_17_017: [** The function shall deserialize the message received. **]**

**SRS_MESSAGE_BUS_17_018: [** If the deserialization is not successful, the message loop shall continue. **]**

**SRS_MESSAGE_BUS_13_092: [** The function shall deliver the message to the module's callback function via `module_info->module_apis`. **]**

**SRS_MESSAGE_BUS_13_093: [** The function shall destroy the message that was dequeued by calling `Message_Destroy`. **]**

**SRS_MESSAGE_BUS_17_019: [** The function shall free the buffer received on the `receive_socket`. **]**

**SRS_MESSAGE_BUS_99_012: [** The function shall deliver the message to the module's Receive function via the `IInternalGatewayModule` interface. **]**

## Broker_Publish

```C
BROKER_RESULT Broker_Publish(
    BROKER_HANDLE bus,
    MODULE_HANDLE source, 
    MESSAGE_HANDLE message
);
```

**SRS_MESSAGE_BUS_13_030: [** If `bus` or `message` is `NULL` the function shall return `BROKER_INVALIDARG`. **]**

**SRS_MESSAGE_BUS_17_022: [** `Broker_Publish` shall Lock the modules lock. **]**

**SRS_MESSAGE_BUS_17_007: [** `Broker_Publish` shall clone the `message`. **]**

**SRS_MESSAGE_BUS_17_008: [** `Broker_Publish` shall serialize the `message`. **]**

**SRS_MESSAGE_BUS_17_009: [** `Broker_Publish` shall allocate a nanomsg buffer and copy the serialized message into it. **]**

**SRS_MESSAGE_BUS_17_010: [** `Broker_Publish` shall send a message on the `publish_socket`. **]**

**SRS_MESSAGE_BUS_17_011: [** `Broker_Publish` shall free the serialized `message` data. **]**

**SRS_MESSAGE_BUS_17_012: [** `Broker_Publish` shall free the `message`. **]**

**SRS_MESSAGE_BUS_17_023: [** `Broker_Publish` shall Unlock the modules lock. **]**

**SRS_MESSAGE_BUS_13_037: [** This function shall return `BROKER_ERROR` if an underlying API call to the platform causes an error or `BROKER_OK` otherwise. **]**

## Broker_AddModule

```C
BROKER_RESULT Broker_AddModule(BROKER_HANDLE bus, const MODULE* module)
```

**SRS_MESSAGE_BUS_99_013: [** If `bus` or `module` is `NULL` the function shall return `BROKER_INVALIDARG`. **]**

**SRS_MESSAGE_BUS_13_107: [** The function shall assign the `module` handle to `MESSAGE_BUS_MODULEINFO::module`. **]**

**SRS_MESSAGE_BUS_17_013: [** The function shall create a nanomsg socket for reception. **]**

**SRS_MESSAGE_BUS_17_014: [** The function shall bind the socket to the the `BROKER_HANDLE_DATA::url`. **]**

**SRS_MESSAGE_BUS_13_099: [** The function shall initialize `MESSAGE_BUS_MODULEINFO::socket_lock` with a valid lock handle. **]**

**SRS_MESSAGE_BUS_17_020: [** The function shall create a unique ID used as a quit signal. **]**

**SRS_MESSAGE_BUS_13_102: [** The function shall create a new thread for the module by calling `ThreadAPI_Create` using `module_publish_worker` as the thread callback and using the newly allocated `MESSAGE_BUS_MODULEINFO` object as the thread context. **]**

**SRS_MESSAGE_BUS_13_039: [** This function shall acquire the lock on `BROKER_HANDLE_DATA::modules_lock`. **]**

**SRS_MESSAGE_BUS_13_045: [** `Broker_AddModule` shall append the new instance of `MESSAGE_BUS_MODULEINFO` to `BROKER_HANDLE_DATA::modules`. **]**

**SRS_MESSAGE_BUS_13_046: [** This function shall release the lock on `BROKER_HANDLE_DATA::modules_lock`. **]**

**SRS_MESSAGE_BUS_13_047: [** This function shall return `BROKER_ERROR` if an underlying API call to the platform causes an error or `BROKER_OK` otherwise. **]**

**SRS_MESSAGE_BUS_99_014: [** If `module_handle` or `module_apis` are `NULL` the function shall return `BROKER_INVALIDARG`. **]**

**SRS_MESSAGE_BUS_99_015: [** If `module_instance` is `NULL` the function shall return `BROKER_INVALIDARG`. **]**


## Broker_RemoveModule

```C
BROKER_RESULT Broker_RemoveModule(BROKER_HANDLE bus, const MODULE* module)
```

**SRS_MESSAGE_BUS_13_048: [** If `bus` or `module` is `NULL` the function shall return `BROKER_INVALIDARG`. **]**

**SRS_MESSAGE_BUS_13_088: [** This function shall acquire the lock on `BROKER_HANDLE_DATA::modules_lock`. **]**

**SRS_MESSAGE_BUS_13_049: [** `Broker_RemoveModule` shall perform a linear search for `module` in `BROKER_HANDLE_DATA::modules`. **]**

**SRS_MESSAGE_BUS_13_050: [** `Broker_RemoveModule` shall unlock `BROKER_HANDLE_DATA::modules_lock` and return `BROKER_ERROR` if the module is not found in `BROKER_HANDLE_DATA::modules`. **]**

**SRS_MESSAGE_BUS_13_052: [** The function shall remove the module from `BROKER_HANDLE_DATA::modules`. **]**

**SRS_MESSAGE_BUS_13_054: [** This function shall release the lock on `BROKER_HANDLE_DATA::modules_lock`. **]**

**SRS_MESSAGE_BUS_17_021: [** This function shall send a quit signal to the worker thread by sending `MESSAGE_BUS_MODULEINFO::quit_message_guid` to the publish_socket. **]**

**SRS_MESSAGE_BUS_02_001: [** Broker_RemoveModule shall lock `MESSAGE_BUS_MODULEINFO::socket_lock`. **]** 

**SRS_MESSAGE_BUS_17_015: [** This function shall close the `MESSAGE_BUS_MODULEINFO::receive_socket`. **]** 

**SRS_MESSAGE_BUS_02_003: [** After closing the socket, Broker_RemoveModule shall unlock `MESSAGE_BUS_MODULEINFO::info_lock`. **]**

**SRS_MESSAGE_BUS_13_104: [** The function shall wait for the module's thread to exit by joining `MESSAGE_BUS_MODULEINFO::thread` via `ThreadAPI_Join`. **]**

**SRS_MESSAGE_BUS_13_057: [** The function shall free all members of the `MESSAGE_BUS_MODULEINFO` object. **]**

**SRS_MESSAGE_BUS_13_053: [** This function shall return `BROKER_ERROR` if an underlying API call to the platform causes an error or `BROKER_OK` otherwise. **]**

## Broker_Destroy

```C
void Broker_Destroy(BROKER_HANDLE bus)
```

**SRS_MESSAGE_BUS_13_058: [** If `bus` is `NULL` the function shall do nothing. **]**

**SRS_MESSAGE_BUS_13_111: [** Otherwise, Broker_Destroy shall decrement the internal ref count of the message. **]**

**SRS_MESSAGE_BUS_13_112: [** If the ref count is zero then the allocated resources are freed. **]**

## MesageBus_DecRef

```C
void Broker_DecRef(BROKER_HANDLE bus)
```

**SRS_MESSAGE_BUS_13_113: [** This function shall implement all the requirements of the `Broker_Destroy` API. **]**
