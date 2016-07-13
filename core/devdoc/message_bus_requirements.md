# Message Bus

## Overview

The message bus (or just the "bus") is central to the gateway. The bus plays the role of a message broker - a central agent responsible for receiving and broadcasting messages between interested parties. In case of the gateway, the interested parties will be modules. This document describes the API for the message bus and what some of the threading implications are.

## References

* [Message Bus High Level Design](bus_hld.md)
* `module.h` - [Module API requirements](module.md)
* [Message API requirements](message_requirements.md)

## Tracking Modules

The message bus implementation shall use the following structure definition to track each module that's connected to the bus:

```C
typedef struct MESSAGE_BUS_MODULEINFO_TAG
{
    /**
     * Handle to the module that's connected to the message bus.
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
}MESSAGE_BUS_MODULEINFO;
```

## Message Bus API

```C
typedef struct MESSAGE_BUS_HANDLE_DATA_TAG* MESSAGE_BUS_HANDLE;

#define MESSAGE_BUS_RESULT_VALUES \
    MESSAGE_BUS_OK, \
    MESSAGE_BUS_ERROR, \
    MESSAGE_BUS_INVALIDARG

DEFINE_ENUM(MESSAGE_BUS_RESULT, MESSAGE_BUS_RESULT_VALUES);

extern MESSAGE_BUS_HANDLE MESSAGE_extern MESSAGE_BUS_HANDLE MessageBus_Create(void);
extern void MessageBus_IncRef(MESSAGE_BUS_HANDLE bus);
extern void MessageBus_DecRef(BUS_HANDLE bus);
extern MESSAGE_BUS_RESULT MessageBus_Publish(MESSAGE_BUS_HANDLE bus, MODULE_HANDLE source, MESSAGE_HANDLE message);
extern MESSAGE_BUS_RESULT MessageBus_AddModule(MESSAGE_BUS_HANDLE bus, const MODULE* module);
extern MESSAGE_BUS_RESULT MessageBus_RemoveModule(MESSAGE_BUS_HANDLE bus, const MODULE* module);
extern void MessageBus_Destroy(MESSAGE_BUS_HANDLE bus);
```

## MessageBus_Create
```C
MESSAGE_BUS_HANDLE MessageBus_Create(void)
```

**SRS_MESSAGE_BUS_13_001: [** This API shall yield a `MESSAGE_BUS_HANDLE` representing the newly created message bus. This handle value shall not be equal to `NULL` when the API call is successful. **]**

**SRS_MESSAGE_BUS_13_003: [** This function shall return `NULL` if an underlying API call to the platform causes an error. **]**

The message bus implementation shall use the following definition as the backing structure for the message bus handle:

```C
typedef struct MESSAGE_BUS_HANDLE_DATA_TAG
{
    /**
     * List of modules that are attached to this message bus. Each element in this
     * vector is an instance of MESSAGE_BUS_MODULEINFO.
     */
    VECTOR_HANDLE           modules;
    
    /**
     * Lock used to synchronize access to the 'modules' field.
     */
    LOCK_HANDLE             modules_lock;
}MESSAGE_BUS_HANDLE_DATA;
```

**SRS_MESSAGE_BUS_13_067: [** `MessageBus_Create` shall `malloc` a new instance of `BUS_HANDLE_DATA` and return `NULL` if it fails. **]**

**SRS_MESSAGE_BUS_13_007: [** `MessageBus_Create` shall initialize `BUS_HANDLE_DATA::modules` with a valid `VECTOR_HANDLE`. **]**

**SRS_MESSAGE_BUS_13_023: [** `MessageBus_Create` shall initialize `BUS_HANDLE_DATA::modules_lock` with a valid `LOCK_HANDLE`. **]**

## MessageBus_IncRef

```C
void MessageBus_IncRef(MESSAGE_BUS_HANDLE bus);
```
MessageBus_Clone creates a clone of the message bus handle.

**SRS_MESSAGE_BUS_13_108: [** If `bus` is `NULL` then `MessageBus_IncRef` shall do nothing. **]**

**SRS_MESSAGE_BUS_13_109: [** Otherwise, `MessageBus_IncRef` shall increment the internal ref count. **]**

## module_publish_worker

```C
static void module_publish_worker(void* user_data)
```

**SRS_MESSAGE_BUS_13_026: [** This function shall assign `user_data` to a local variable called `module_info` of type `MESSAGE_BUS_MODULEINFO*`. **]**

**SRS_MESSAGE_BUS_13_089: [** This function shall acquire the lock on `module_info->mq_lock`. **]**

**SRS_MESSAGE_BUS_02_004: [** If acquiring the lock fails, then module_publish_worker shall return. **]**

**SRS_MESSAGE_BUS_13_068: [** This function shall run a loop that keeps running while `module_info->quit_worker` is equal to `0`. **]**

**SRS_MESSAGE_BUS_04_001: [** This function shall immediately start processing messages when `module->mq` is not empty without waiting on `module->mq_cond`. **]**

**SRS_MESSAGE_BUS_13_071: [** For every iteration of the loop the function will first wait on `module_info->mq_cond` using `module_info->mq_lock` as the corresponding mutex to be used by the condition variable. **]**

**SRS_MESSAGE_BUS_13_090: [** When `module_info->mq_cond` has been signaled this function shall kick off another loop predicated on `module_info->quit_worker` being equal to `0` and `module_info->mq` not being empty. This thread has the lock on `module_info->mq_lock` at this point. **]**

**SRS_MESSAGE_BUS_13_069: [** The function shall dequeue a message from the module's message queue. **]**

**SRS_MESSAGE_BUS_13_091: [** The function shall unlock `module_info->mq_lock`. **]**

**SRS_MESSAGE_BUS_13_092: [** The function shall deliver the message to the module's callback function via `module_info->module_apis`. **]**

**SRS_MESSAGE_BUS_13_093: [** The function shall destroy the message that was dequeued by calling `Message_Destroy`. **]**

**SRS_MESSAGE_BUS_13_094: [** The function shall re-acquire the lock on `module_info->mq_lock`. **]**

**SRS_MESSAGE_BUS_13_095: [** When the function exits the outer loop predicated on `module_info->quit_worker` being `0` it shall unlock `module_info->mq_lock` before exiting from the function. **]**

**SRS_MESSAGE_BUS_99_012: [** The function shall deliver the message to the module's Receive function via the `IInternalGatewayModule` interface. **]**

## MessageBus_Publish

```C
MESSAGE_BUS_RESULT MessageBus_Publish(
    MESSAGE_BUS_HANDLE bus,
    MODULE_HANDLE source, 
    MESSAGE_HANDLE message
);
```

**SRS_MESSAGE_BUS_13_030: [** If `bus` or `message` is `NULL` the function shall return `MESSAGE_BUS_INVALIDARG`. **]**

**SRS_MESSAGE_BUS_13_031: [** `MessageBus_Publish` shall acquire the lock `MESSAGE_BUS_HANDLE_DATA::modules_lock`. **]**

**SRS_MESSAGE_BUS_13_032: [** `MessageBus_Publish` shall start a processing loop for every module in `MESSAGE_BUS_HANDLE_DATA::modules`.  **]**

**SRS_MESSAGE_BUS_17_002: [** If `source` is not NULL, `MessageBus_Publish` shall not publish the message to the `MESSAGE_BUS_MODULEINFO::module` which matches `source`. **]**

**SRS_MESSAGE_BUS_13_033: [** In the loop, the function shall first acquire the lock on `MESSAGE_BUS_MODULEINFO::mq_lock`. **]**

**SRS_MESSAGE_BUS_13_034: [** The function shall then append `message` to `MESSAGE_BUS_MODULEINFO::mq` by calling `Message_Clone` and `VECTOR_push_back`. **]**

**SRS_MESSAGE_BUS_13_035: [** The function shall then release `MESSAGE_BUS_MODULEINFO::mq_lock`. **]**

**SRS_MESSAGE_BUS_13_096: [** The function shall then signal `MESSAGE_BUS_MODULEINFO::mq_cond`. **]**

**SRS_MESSAGE_BUS_13_040: [** `MessageBus_Publish` shall release the lock `MESSAGE_BUS_HANDLE_DATA::modules_lock` after the loop. **]**

**SRS_MESSAGE_BUS_13_037: [** This function shall return `MESSAGE_BUS_ERROR` if an underlying API call to the platform causes an error or `MESSAGE_BUS_OK` otherwise. **]**

## MessageBus_AddModule

```C
MESSAGE_BUS_RESULT MessageBus_AddModule(MESSAGE_BUS_HANDLE bus, const MODULE* module)
```

**SRS_MESSAGE_BUS_99_013: [** If `bus` or `module` is `NULL` the function shall return `MESSAGE_BUS_INVALIDARG`. **]**

**SRS_MESSAGE_BUS_13_107: [** The function shall assign the `module` handle to `MESSAGE_BUS_MODULEINFO::module`. **]**

**SRS_MESSAGE_BUS_13_098: [** The function shall initialize `MESSAGE_BUS_MODULEINFO::mq` with a valid vector handle. **]**

**SRS_MESSAGE_BUS_13_099: [** The function shall initialize `MESSAGE_BUS_MODULEINFO::mq_lock` with a valid lock handle. **]**

**SRS_MESSAGE_BUS_13_100: [** The function shall initialize `MESSAGE_BUS_MODULEINFO::mq_cond` with a valid condition handle. **]**

**SRS_MESSAGE_BUS_13_101: [** The function shall assign `0` to `MESSAGE_BUS_MODULEINFO::quit_worker`. **]**

**SRS_MESSAGE_BUS_13_102: [** The function shall create a new thread for the module by calling `ThreadAPI_Create` using `module_publish_worker` as the thread callback and using the newly allocated `MESSAGE_BUS_MODULEINFO` object as the thread context. **]**

**SRS_MESSAGE_BUS_13_039: [** This function shall acquire the lock on `MESSAGE_BUS_HANDLE_DATA::modules_lock`. **]**

**SRS_MESSAGE_BUS_13_045: [** `MessageBus_AddModule` shall append the new instance of `MESSAGE_BUS_MODULEINFO` to `MESSAGE_BUS_HANDLE_DATA::modules`. **]**

**SRS_MESSAGE_BUS_13_046: [** This function shall release the lock on `MESSAGE_BUS_HANDLE_DATA::modules_lock`. **]**

**SRS_MESSAGE_BUS_13_047: [** This function shall return `MESSAGE_BUS_ERROR` if an underlying API call to the platform causes an error or `MESSAGE_BUS_OK` otherwise. **]**

**SRS_MESSAGE_BUS_99_014: [** If `module_handle` or `module_apis` are `NULL` the function shall return `MESSAGE_BUS_INVALIDARG`. **]**

**SRS_MESSAGE_BUS_99_015: [** If `module_instance` is `NULL` the function shall return `MESSAGE_BUS_INVALIDARG`. **]**


## MessageBus_RemoveModule

```C
MESSAGE_BUS_RESULT MessageBus_RemoveModule(MESSAGE_BUS_HANDLE bus, const MODULE* module)
```

**SRS_MESSAGE_BUS_13_048: [** If `bus` or `module` is `NULL` the function shall return `MESSAGE_BUS_INVALIDARG`. **]**

**SRS_MESSAGE_BUS_13_088: [** This function shall acquire the lock on `MESSAGE_BUS_HANDLE_DATA::modules_lock`. **]**

**SRS_MESSAGE_BUS_13_049: [** `MessageBus_RemoveModule` shall perform a linear search for `module` in `MESSAGE_BUS_HANDLE_DATA::modules`. **]**

**SRS_MESSAGE_BUS_13_050: [** `MessageBus_RemoveModule` shall unlock `MESSAGE_BUS_HANDLE_DATA::modules_lock` and return `MESSAGE_BUS_ERROR` if the module is not found in `MESSAGE_BUS_HANDLE_DATA::modules`. **]**

**SRS_MESSAGE_BUS_13_052: [** The function shall remove the module from `MESSAGE_BUS_HANDLE_DATA::modules`. **]**

**SRS_MESSAGE_BUS_13_054: [** This function shall release the lock on `MESSAGE_BUS_HANDLE_DATA::modules_lock`. **]**

**SRS_MESSAGE_BUS_02_001: [** MessageBus_RemoveModule shall lock `MESSAGE_BUS_MODULEINFO::mq_lock`. **]** 

**SRS_MESSAGE_BUS_02_002: [** If locking fails, then terminating the thread shall not be attempted (signalling the condition and joining the thread). **]**

**SRS_MESSAGE_BUS_13_103: [** The function shall assign `1` to `MESSAGE_BUS_MODULEINFO::quit_worker`. **]**

**SRS_MESSAGE_BUS_17_001: [**The function shall signal `MESSAGE_BUS_MODULEINFO::mq_cond` to release module from waiting.**]**

**SRS_MESSAGE_BUS_02_003: [** After signaling the condition, MessageBus_RemoveModule shall unlock `MESSAGE_BUS_MODULEINFO::mq_lock`. **]**

**SRS_MESSAGE_BUS_13_104: [** The function shall wait for the module's thread to exit by joining `MESSAGE_BUS_MODULEINFO::thread` via `ThreadAPI_Join`. **]**

**SRS_MESSAGE_BUS_13_056: [** If `MESSAGE_BUS_MODULEINFO::mq` is not empty then this function shall call `Message_Destroy` on every message still left in the collection. **]**

**SRS_MESSAGE_BUS_13_057: [** The function shall free all members of the `MESSAGE_BUS_MODULEINFO` object. **]**

**SRS_MESSAGE_BUS_13_053: [** This function shall return `MESSAGE_BUS_ERROR` if an underlying API call to the platform causes an error or `MESSAGE_BUS_OK` otherwise. **]**

## MessageBus_Destroy

```C
void MessageBus_Destroy(MESSAGE_BUS_HANDLE bus)
```

**SRS_MESSAGE_BUS_13_058: [** If `bus` is `NULL` the function shall do nothing. **]**

**SRS_MESSAGE_BUS_13_111: [** Otherwise, MessageBus_Destroy shall decrement the internal ref count of the message. **]**

**SRS_MESSAGE_BUS_13_112: [** If the ref count is zero then the allocated resources are freed. **]**

## MesageBus_DecRef

```C
void MessageBus_DecRef(MESSAGE_BUS_HANDLE bus)
```

**SRS_MESSAGE_BUS_13_113: [** This function shall implement all the requirements of the `MessageBus_Destroy` API. **]**
