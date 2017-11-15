# Message Broker

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
typedef struct BROKER_MODULEINFO_TAG
{
    /**
     * Handle to a module associated with this broker.
     */
    MODULE_HANDLE             module;
    
    /**
     * The function dispatch table for this module.
     */
    CONST MODULE_API*        module_api;
    
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
}BROKER_MODULEINFO;
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
extern void Broker_IncRef(BROKER_HANDLE broker);
extern void Broker_DecRef(BROKER_HANDLE broker);
extern BROKER_RESULT Broker_Publish(BROKER_HANDLE broker, MODULE_HANDLE source, MESSAGE_HANDLE message);
extern BROKER_RESULT Broker_AddModule(BROKER_HANDLE broker, const MODULE* module);
extern BROKER_RESULT Broker_RemoveModule(BROKER_HANDLE broker, const MODULE* module);
extern BROKER_RESULT Broker_AddLink(BROKER_HANDLE broker, const LINK_DATA* link);
extern BROKER_RESULT Broker_RemoveLink(BROKER_HANDLE broker, const LINK_DATA* link);
extern void Broker_Destroy(BROKER_HANDLE broker);
```

## Broker_Create
```C
BROKER_HANDLE Broker_Create(void)
```

**SRS_BROKER_13_001: [** This API shall yield a `BROKER_HANDLE` representing the newly created message broker. This handle value shall not be equal to `NULL` when the API call is successful. **]**

**SRS_BROKER_13_003: [** This function shall return `NULL` if an underlying API call to the platform causes an error. **]**

The backing structure for a message broker handle is defined as follows:

```C
typedef struct BROKER_HANDLE_DATA_TAG
{
    /**
     * List of modules that are attached to this message broker. Each element in this
     * vector is an instance of BROKER_MODULEINFO.
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

**SRS_BROKER_13_067: [** `Broker_Create` shall `malloc` a new instance of `BROKER_HANDLE_DATA`. **]**

**SRS_BROKER_13_007: [** `Broker_Create` shall initialize `BROKER_HANDLE_DATA::modules` with a valid `VECTOR_HANDLE`. **]**

**SRS_BROKER_13_023: [** `Broker_Create` shall initialize `BROKER_HANDLE_DATA::modules_lock` with a valid `LOCK_HANDLE`. **]**

**SRS_BROKER_17_001: [** `Broker_Create` shall initialize a socket for publishing messages. **]**
 
**SRS_BROKER_17_002: [**  `Broker_Create` shall create a unique id. **]**

**SRS_BROKER_17_003: [** `Broker_Create` shall initialize a url consisting of "inproc://" + unique id. **]**

**SRS_BROKER_17_004: [** `Broker_Create` shall bind the socket to the `BROKER_HANDLE_DATA::url`. **]**

## Broker_IncRef

```C
void Broker_IncRef(BROKER_HANDLE broker);
```
Broker_Clone creates a clone of the message broker handle.

**SRS_BROKER_13_108: [** If `broker` is `NULL` then `Broker_IncRef` shall do nothing. **]**

**SRS_BROKER_13_109: [** Otherwise, `Broker_IncRef` shall increment the internal ref count. **]**

## module_worker

```C
static void module_worker(void* user_data)
```

**SRS_BROKER_13_026: [** This function shall assign `user_data` to a local variable called `module_info` of type `BROKER_MODULEINFO*`. **]**

**SRS_BROKER_13_089: [** This function shall acquire the lock on `module_info->socket_lock`. **]**

**SRS_BROKER_02_004: [** If acquiring the lock fails, then `module_worker` shall return. **]**

**SRS_BROKER_13_068: [** This function shall run a loop that keeps running until `module_info->quit_message_guid` is sent to the thread. **]**

**SRS_BROKER_13_091: [** The function shall unlock `module_info->socket_lock`. **]**

**SRS_BROKER_17_016: [** If releasing the lock fails, then `module_worker` shall return. **]**

**SRS_BROKER_17_005: [** For every iteration of the loop, the function shall wait on the `receive_socket` for messages. **]**

**SRS_BROKER_17_006: [** An error on receiving a message shall terminate the loop. **]**

**SRS_BROKER_17_024: [** The function shall strip off the topic from the message. **]**

**SRS_BROKER_17_017: [** The function shall deserialize the message received. **]**

**SRS_BROKER_17_018: [** If the deserialization is not successful, the message loop shall continue. **]**

**SRS_BROKER_13_092: [** The function shall deliver the message to the module's callback function via `module_info->module_api`. **]**

**SRS_BROKER_13_093: [** The function shall destroy the message that was dequeued by calling `Message_Destroy`. **]**

**SRS_BROKER_17_019: [** The function shall free the buffer received on the `receive_socket`. **]**

## Broker_Publish

```C
BROKER_RESULT Broker_Publish(
    BROKER_HANDLE broker,
    MODULE_HANDLE source, 
    MESSAGE_HANDLE message
);
```

**SRS_BROKER_13_030: [** If `broker`, `source`, or `message` is `NULL` the function shall return `BROKER_INVALIDARG`. **]**

**SRS_BROKER_17_022: [** `Broker_Publish` shall Lock the modules lock. **]**

**SRS_BROKER_17_007: [** `Broker_Publish` shall clone the `message`. **]**

**SRS_BROKER_17_008: [** `Broker_Publish` shall serialize the `message`. **]**

**SRS_BROKER_17_025: [** `Broker_Publish` shall allocate a nanomsg buffer the size of the serialized message + `sizeof(MODULE_HANDLE)`.  **]**

**SRS_BROKER_17_026: [** `Broker_Publish` shall copy `source` into the beginning of the nanomsg buffer. **]** 

**SRS_BROKER_17_027: [** `Broker_Publish` shall serialize the `message` into the remainder of the nanomsg buffer. **]**

**SRS_BROKER_17_010: [** `Broker_Publish` shall send a message on the `publish_socket`. **]**

**SRS_BROKER_17_011: [** `Broker_Publish` shall free the serialized `message` data. **]**

**SRS_BROKER_17_012: [** `Broker_Publish` shall free the `message`. **]**

**SRS_BROKER_17_023: [** `Broker_Publish` shall Unlock the modules lock. **]**

**SRS_BROKER_13_037: [** This function shall return `BROKER_ERROR` if an underlying API call to the platform causes an error or `BROKER_OK` otherwise. **]**

## Broker_AddModule

```C
BROKER_RESULT Broker_AddModule(BROKER_HANDLE broker, const MODULE* module)
```

**SRS_BROKER_99_013: [** If `broker` or `module` is `NULL` the function shall return `BROKER_INVALIDARG`. **]**

**SRS_BROKER_13_107: [** The function shall assign the `module` handle to `BROKER_MODULEINFO::module`. **]**

**SRS_BROKER_17_013: [** The function shall create a nanomsg socket for reception. **]**

**SRS_BROKER_17_014: [** The function shall bind the socket to the the `BROKER_HANDLE_DATA::url`. **]**

**SRS_BROKER_13_099: [** The function shall initialize `BROKER_MODULEINFO::socket_lock` with a valid lock handle. **]**

**SRS_BROKER_17_020: [** The function shall create a unique ID used as a quit signal. **]**

**SRS_BROKER_17_028: [** The function shall subscribe `BROKER_MODULEINFO::receive_socket` to the quit signal GUID. **]**

**SRS_BROKER_13_102: [** The function shall create a new thread for the module by calling `ThreadAPI_Create` using `module_worker` as the thread callback and using the newly allocated `BROKER_MODULEINFO` object as the thread context. **]**

**SRS_BROKER_13_039: [** This function shall acquire the lock on `BROKER_HANDLE_DATA::modules_lock`. **]**

**SRS_BROKER_13_045: [** `Broker_AddModule` shall append the new instance of `BROKER_MODULEINFO` to `BROKER_HANDLE_DATA::modules`. **]**

**SRS_BROKER_13_046: [** This function shall release the lock on `BROKER_HANDLE_DATA::modules_lock`. **]**

**SRS_BROKER_13_047: [** This function shall return `BROKER_ERROR` if an underlying API call to the platform causes an error or `BROKER_OK` otherwise. **]**

**SRS_BROKER_99_014: [** If `module_handle` or `module_api` are `NULL` the function shall return `BROKER_INVALIDARG`. **]**


## Broker_RemoveModule

```C
BROKER_RESULT Broker_RemoveModule(BROKER_HANDLE broker, const MODULE* module)
```

**SRS_BROKER_13_048: [** If `broker` or `module` is `NULL` the function shall return `BROKER_INVALIDARG`. **]**

**SRS_BROKER_13_088: [** This function shall acquire the lock on `BROKER_HANDLE_DATA::modules_lock`. **]**

**SRS_BROKER_13_049: [** `Broker_RemoveModule` shall perform a linear search for `module` in `BROKER_HANDLE_DATA::modules`. **]**

**SRS_BROKER_13_050: [** `Broker_RemoveModule` shall unlock `BROKER_HANDLE_DATA::modules_lock` and return `BROKER_ERROR` if the module is not found in `BROKER_HANDLE_DATA::modules`. **]**

**SRS_BROKER_13_052: [** The function shall remove the module from `BROKER_HANDLE_DATA::modules`. **]**

**SRS_BROKER_13_054: [** This function shall release the lock on `BROKER_HANDLE_DATA::modules_lock`. **]**

**SRS_BROKER_17_021: [** This function shall send a quit signal to the worker thread by sending `BROKER_MODULEINFO::quit_message_guid` to the publish_socket. **]**

**SRS_BROKER_02_001: [** Broker_RemoveModule shall lock `BROKER_MODULEINFO::socket_lock`. **]** 

**SRS_BROKER_17_015: [** This function shall close the `BROKER_MODULEINFO::receive_socket`. **]** 

**SRS_BROKER_02_003: [** After closing the socket, Broker_RemoveModule shall unlock `BROKER_MODULEINFO::info_lock`. **]**

**SRS_BROKER_13_104: [** The function shall wait for the module's thread to exit by joining `BROKER_MODULEINFO::thread` via `ThreadAPI_Join`. **]**

**SRS_BROKER_13_057: [** The function shall free all members of the `BROKER_MODULEINFO` object. **]**

**SRS_BROKER_13_053: [** This function shall return `BROKER_ERROR` if an underlying API call to the platform causes an error or `BROKER_OK` otherwise. **]**


## Broker_AddLink
```c
extern BROKER_RESULT Broker_AddLink(BROKER_HANDLE broker, const LINK_DATA* link);
```

Add a router link to the Broker.

**SRS_BROKER_17_029: [** If `broker`, `link`, `link->module_source_handle` or `link->module_sink_handle` are NULL, `Broker_AddLink` shall return `BROKER_INVALIDARG`. **]**

**SRS_BROKER_17_030: [** `Broker_AddLink` shall lock the `modules_lock`. **]** 

**SRS_BROKER_17_031: [** `Broker_AddLink` shall find the `BROKER_HANDLE_DATA::module_info` for `link->module_sink_handle`. **]**

**SRS_BROKER_17_041: [** `Broker_AddLink` shall find the `BROKER_HANDLE_DATA::module_info` for `link->module_source_handle`. **]**

**SRS_BROKER_17_032: [** `Broker_AddLink` shall subscribe `module_info->receive_socket` to the `link->module_source_handle` module handle. **]** 

**SRS_BROKER_17_033: [** `Broker_AddLink` shall unlock the `modules_lock`. **]** 

**SRS_BROKER_17_034: [** Upon an error, `Broker_AddLink` shall return `BROKER_ADD_LINK_ERROR` **]** 


## Broker_RemoveLink
```c
extern BROKER_RESULT Broker_RemoveLink(BROKER_HANDLE broker, const LINK_DATA* link);
```

Remove a router link from the Broker.

**SRS_BROKER_17_035: [** If `broker`, `link`, `link->module_source_handle` or `link->module_sink_handle` are NULL, `Broker_RemoveLink` shall return `BROKER_INVALIDARG`. **]** 

**SRS_BROKER_17_036: [** `Broker_RemoveLink` shall lock the `modules_lock`. **]** 

**SRS_BROKER_17_037: [** `Broker_RemoveLink` shall find the `module_info` for `link->module_sink_handle`. **]** 

**SRS_BROKER_17_042: [** `Broker_RemoveLink` shall find the `module_info` for `link->module_source_handle`. **]**

**SRS_BROKER_17_038: [** `Broker_RemoveLink` shall unsubscribe `module_info->receive_socket` from the `link->module_source_handle` module handle. **]** 

**SRS_BROKER_17_039: [** `Broker_RemoveLink` shall unlock the `modules_lock`. **]**

**SRS_BROKER_17_040: [** Upon an error, `Broker_RemoveLink` shall return `BROKER_REMOVE_LINK_ERROR`. **]** 

## Broker_Destroy

```C
void Broker_Destroy(BROKER_HANDLE broker)
```

**SRS_BROKER_13_058: [** If `broker` is `NULL` the function shall do nothing. **]**

**SRS_BROKER_13_111: [** Otherwise, Broker_Destroy shall decrement the internal ref count of the message. **]**

**SRS_BROKER_13_112: [** If the ref count is zero then the allocated resources are freed. **]**

## Broker_DecRef

```C
void Broker_DecRef(BROKER_HANDLE broker)
```

**SRS_BROKER_13_113: [** This function shall implement all the requirements of the `Broker_Destroy` API. **]**
