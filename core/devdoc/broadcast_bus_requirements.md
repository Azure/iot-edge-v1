# Broadcast Message Broker

## Overview

The message broker (or just the "broker") is central to the gateway. The broker is responsible for sending and receiving messages between interested parties. In the case of the gateway, the interested parties are modules. This document describes the API for the message broker and what some of the threading implications are.

## References

* [Message Broker High Level Design](broker_hld.md)
* `module.h` - [Module API requirements](module.md)
* [Message API requirements](message_requirements.md)

## Tracking Modules

The message broker implementation shall use the following structure definition to track each module that's connected to the broker:

```C
typedef struct BROKER_MODULEINFO_TAG
{
    /**
     * Handle to the module that's connected to the message broker.
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
    VECTOR_HANDLE           mq;
    
    /**
     * Lock used to synchronize access to the 'mq' field.
     */
    LOCK_HANDLE             mq_lock;
    
    /**
     * A condition variable that is signaled when there are new messages.
     */
    COND_HANDLE             mq_cond;
    
    /**
     * Message publish worker will keep running while this is false.
     */
    sig_atomic_t            quit_worker;
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

**SRS_BCAST_BROKER_13_001: [** This API shall yield a `BROKER_HANDLE` representing the newly created message broker. This handle value shall not be equal to `NULL` when the API call is successful. **]**

**SRS_BCAST_BROKER_13_003: [** This function shall return `NULL` if an underlying API call to the platform causes an error. **]**

The message broker implementation shall use the following definition as the backing structure for the message broker handle:

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
}BROKER_HANDLE_DATA;
```

**SRS_BCAST_BROKER_13_067: [** `Broker_Create` shall `malloc` a new instance of `BROKER_HANDLE_DATA`. **]**

**SRS_BCAST_BROKER_13_007: [** `Broker_Create` shall initialize `BROKER_HANDLE_DATA::modules` with a valid `VECTOR_HANDLE`. **]**

**SRS_BCAST_BROKER_13_023: [** `Broker_Create` shall initialize `BROKER_HANDLE_DATA::modules_lock` with a valid `LOCK_HANDLE`. **]**

## Broker_IncRef

```C
void Broker_IncRef(BROKER_HANDLE broker);
```
Broker_Clone creates a clone of the message broker handle.

**SRS_BCAST_BROKER_13_108: [** If `broker` is `NULL` then `Broker_IncRef` shall do nothing. **]**

**SRS_BCAST_BROKER_13_109: [** Otherwise, `Broker_IncRef` shall increment the internal ref count. **]**

## module_publish_worker

```C
static void module_publish_worker(void* user_data)
```

**SRS_BCAST_BROKER_13_026: [** This function shall assign `user_data` to a local variable called `module_info` of type `BROKER_MODULEINFO*`. **]**

**SRS_BCAST_BROKER_13_089: [** This function shall acquire the lock on `module_info->mq_lock`. **]**

**SRS_BCAST_BROKER_02_004: [** If acquiring the lock fails, then module_publish_worker shall return. **]**

**SRS_BCAST_BROKER_13_068: [** This function shall run a loop that keeps running while `module_info->quit_worker` is equal to `0`. **]**

**SRS_BCAST_BROKER_04_001: [** This function shall immediately start processing messages when `module->mq` is not empty without waiting on `module->mq_cond`. **]**

**SRS_BCAST_BROKER_13_071: [** For every iteration of the loop the function will first wait on `module_info->mq_cond` using `module_info->mq_lock` as the corresponding mutex to be used by the condition variable. **]**

**SRS_BCAST_BROKER_13_090: [** When `module_info->mq_cond` has been signaled this function shall kick off another loop predicated on `module_info->quit_worker` being equal to `0` and `module_info->mq` not being empty. This thread has the lock on `module_info->mq_lock` at this point. **]**

**SRS_BCAST_BROKER_13_069: [** The function shall dequeue a message from the module's message queue. **]**

**SRS_BCAST_BROKER_13_091: [** The function shall unlock `module_info->mq_lock`. **]**

**SRS_BCAST_BROKER_13_092: [** The function shall deliver the message to the module's callback function via `module_info->module_apis`. **]**

**SRS_BCAST_BROKER_13_093: [** The function shall destroy the message that was dequeued by calling `Message_Destroy`. **]**

**SRS_BCAST_BROKER_13_094: [** The function shall re-acquire the lock on `module_info->mq_lock`. **]**

**SRS_BCAST_BROKER_13_095: [** When the function exits the outer loop predicated on `module_info->quit_worker` being `0` it shall unlock `module_info->mq_lock` before exiting from the function. **]**

**SRS_BCAST_BROKER_99_012: [** The function shall deliver the message to the module's Receive function via the `IInternalGatewayModule` interface. **]**

## Broker_Publish

```C
BROKER_RESULT Broker_Publish(
    BROKER_HANDLE broker,
    MODULE_HANDLE source, 
    MESSAGE_HANDLE message
);
```

**SRS_BCAST_BROKER_13_030: [** If `broker` or `message` is `NULL` the function shall return `BROKER_INVALIDARG`. **]**

**SRS_BCAST_BROKER_13_031: [** `Broker_Publish` shall acquire the lock `BROKER_HANDLE_DATA::modules_lock`. **]**

**SRS_BCAST_BROKER_13_032: [** `Broker_Publish` shall start a processing loop for every module in `BROKER_HANDLE_DATA::modules`.  **]**

**SRS_BCAST_BROKER_17_002: [** If `source` is not NULL, `Broker_Publish` shall not publish the message to the `BROKER_MODULEINFO::module` which matches `source`. **]**

**SRS_BCAST_BROKER_13_033: [** In the loop, the function shall first acquire the lock on `BROKER_MODULEINFO::mq_lock`. **]**

**SRS_BCAST_BROKER_13_034: [** The function shall then append `message` to `BROKER_MODULEINFO::mq` by calling `Message_Clone` and `VECTOR_push_back`. **]**

**SRS_BCAST_BROKER_13_035: [** The function shall then release `BROKER_MODULEINFO::mq_lock`. **]**

**SRS_BCAST_BROKER_13_096: [** The function shall then signal `BROKER_MODULEINFO::mq_cond`. **]**

**SRS_BCAST_BROKER_13_040: [** `Broker_Publish` shall release the lock `BROKER_HANDLE_DATA::modules_lock` after the loop. **]**

**SRS_BCAST_BROKER_13_037: [** This function shall return `BROKER_ERROR` if an underlying API call to the platform causes an error or `BROKER_OK` otherwise. **]**

## Broker_AddModule

```C
BROKER_RESULT Broker_AddModule(BROKER_HANDLE broker, const MODULE* module)
```

**SRS_BCAST_BROKER_99_013: [** If `broker` or `module` is `NULL` the function shall return `BROKER_INVALIDARG`. **]**

**SRS_BCAST_BROKER_13_107: [** The function shall assign the `module` handle to `BROKER_MODULEINFO::module`. **]**

**SRS_BCAST_BROKER_13_098: [** The function shall initialize `BROKER_MODULEINFO::mq` with a valid vector handle. **]**

**SRS_BCAST_BROKER_13_099: [** The function shall initialize `BROKER_MODULEINFO::mq_lock` with a valid lock handle. **]**

**SRS_BCAST_BROKER_13_100: [** The function shall initialize `BROKER_MODULEINFO::mq_cond` with a valid condition handle. **]**

**SRS_BCAST_BROKER_13_101: [** The function shall assign `0` to `BROKER_MODULEINFO::quit_worker`. **]**

**SRS_BCAST_BROKER_13_102: [** The function shall create a new thread for the module by calling `ThreadAPI_Create` using `module_publish_worker` as the thread callback and using the newly allocated `BROKER_MODULEINFO` object as the thread context. **]**

**SRS_BCAST_BROKER_13_039: [** This function shall acquire the lock on `BROKER_HANDLE_DATA::modules_lock`. **]**

**SRS_BCAST_BROKER_13_045: [** `Broker_AddModule` shall append the new instance of `BROKER_MODULEINFO` to `BROKER_HANDLE_DATA::modules`. **]**

**SRS_BCAST_BROKER_13_046: [** This function shall release the lock on `BROKER_HANDLE_DATA::modules_lock`. **]**

**SRS_BCAST_BROKER_13_047: [** This function shall return `BROKER_ERROR` if an underlying API call to the platform causes an error or `BROKER_OK` otherwise. **]**

**SRS_BCAST_BROKER_99_014: [** If `module_handle` or `module_apis` are `NULL` the function shall return `BROKER_INVALIDARG`. **]**

**SRS_BCAST_BROKER_99_015: [** If `module_instance` is `NULL` the function shall return `BROKER_INVALIDARG`. **]**


## Broker_RemoveModule

```C
BROKER_RESULT Broker_RemoveModule(BROKER_HANDLE broker, const MODULE* module)
```

**SRS_BCAST_BROKER_13_048: [** If `broker` or `module` is `NULL` the function shall return `BROKER_INVALIDARG`. **]**

**SRS_BCAST_BROKER_13_088: [** This function shall acquire the lock on `BROKER_HANDLE_DATA::modules_lock`. **]**

**SRS_BCAST_BROKER_13_049: [** `Broker_RemoveModule` shall perform a linear search for `module` in `BROKER_HANDLE_DATA::modules`. **]**

**SRS_BCAST_BROKER_13_050: [** `Broker_RemoveModule` shall unlock `BROKER_HANDLE_DATA::modules_lock` and return `BROKER_ERROR` if the module is not found in `BROKER_HANDLE_DATA::modules`. **]**

**SRS_BCAST_BROKER_13_052: [** The function shall remove the module from `BROKER_HANDLE_DATA::modules`. **]**

**SRS_BCAST_BROKER_13_054: [** This function shall release the lock on `BROKER_HANDLE_DATA::modules_lock`. **]**

**SRS_BCAST_BROKER_02_001: [** Broker_RemoveModule shall lock `BROKER_MODULEINFO::mq_lock`. **]** 

**SRS_BCAST_BROKER_02_002: [** If locking fails, then terminating the thread shall not be attempted (signalling the condition and joining the thread). **]**

**SRS_BCAST_BROKER_13_103: [** The function shall assign `1` to `BROKER_MODULEINFO::quit_worker`. **]**

**SRS_BCAST_BROKER_17_001: [**The function shall signal `BROKER_MODULEINFO::mq_cond` to release module from waiting.**]**

**SRS_BCAST_BROKER_02_003: [** After signaling the condition, Broker_RemoveModule shall unlock `BROKER_MODULEINFO::mq_lock`. **]**

**SRS_BCAST_BROKER_13_104: [** The function shall wait for the module's thread to exit by joining `BROKER_MODULEINFO::thread` via `ThreadAPI_Join`. **]**

**SRS_BCAST_BROKER_13_056: [** If `BROKER_MODULEINFO::mq` is not empty then this function shall call `Message_Destroy` on every message still left in the collection. **]**

**SRS_BCAST_BROKER_13_057: [** The function shall free all members of the `BROKER_MODULEINFO` object. **]**

**SRS_BCAST_BROKER_13_053: [** This function shall return `BROKER_ERROR` if an underlying API call to the platform causes an error or `BROKER_OK` otherwise. **]**

## Broker_AddLink
```c
extern BROKER_RESULT Broker_AddLink(BROKER_HANDLE broker, const LINK_DATA* link);
```

Add a router link to the Broker.

**SRS_BCAST_BROKER_17_003: [** `Broker_AddLink` shall return BROKER_OK. **]**


## Broker_RemoveLink
```c
extern BROKER_RESULT Broker_RemoveLink(BROKER_HANDLE broker, const LINK_DATA* link);
```

Remove a router link from the Broker.

**SRS_BCAST_BROKER_17_004: [** `Broker_RemoveLink` shall return BROKER_OK. **]**

## Broker_Destroy

```C
void Broker_Destroy(BROKER_HANDLE broker)
```

**SRS_BCAST_BROKER_13_058: [** If `broker` is `NULL` the function shall do nothing. **]**

**SRS_BCAST_BROKER_13_111: [** Otherwise, Broker_Destroy shall decrement the internal ref count of the message. **]**

**SRS_BCAST_BROKER_13_112: [** If the ref count is zero then the allocated resources are freed. **]**

## Broker_DecRef

```C
void Broker_DecRef(BROKER_HANDLE broker)
```

**SRS_BCAST_BROKER_13_113: [** This function shall implement all the requirements of the `Broker_Destroy` API. **]**
