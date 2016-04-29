Message Bus
===========

High level design
-----------------

### Overview

The message bus (or just the "bus") is central to the gateway. The message bus plays the role of a message broker - a central agent responsible for receiving and broadcasting messages between interested parties. In case of the gateway, the interested parties will be *modules*. This document describes the high level design of the message bus along with descriptions of flow of control.

### Design Goals

The message bus essentially has two underlying guiding principles that influence its design:

- The message bus APIs shall be thread-safe, i.e., any piece of code running on any arbitrary thread is allowed to call any API on the message bus at any time concurrently or otherwise.
- When the bus delivers messages to modules on the bus it shall ensure that it does so in a serial fashion for any given module, i.e., at no point shall it *concurrently* invoke the message receive function on a module. This allows modules to not have to concern themselves with ensuring synchronization of access to shared state on the module.

### The Message Bus Data

An instance of the message bus is represented in code via an opaque `MESSAGE_BUS_HANDLE` value and is a reference counted object. The `MESSAGE_BUS_HANDLE` is backed by a data structure called `MESSAGE_BUS_HANDLE_DATA` which looks like this:

```C
typedef struct MESSAGE_BUS_HANDLE_DATA_TAG
{
    VECTOR_HANDLE           modules;
    LOCK_HANDLE             modules_lock;
}MESSAGE_BUS_HANDLE_DATA;
```

Here's a description of the members of `MESSAGE_BUS_HANDLE_DATA`.

>| Field       | Description                                                                  |
>|-------------|------------------------------------------------------------------------------|
>| modules       | Vector of modules where each element is an instance of `MODULE_INFO`.      |
>| modules_lock  | A mutex used to synchronize access to the `modules` field.                 |

### The Module Info
    
Each module that is connected to the message bus is represented using a structure of type `MODULE_INFO` which looks like so:

```C
typedef struct MODULE_INFO_TAG
{
    MODULE_HANDLE           module;
    MODULE_APIS             module_apis;
    THREAD_HANDLE           thread;
    VECTOR_HANDLE           mq;
    COND_HANDLE             mq_cond;
    LOCK_HANDLE             mq_lock;
    sig_atomic_t            quit_worker;
}MODULE_INFO;
```

A description of the fields in the `MODULE_INFO` structure follows:

>| Field       | Description                                                          |
>|-------------|----------------------------------------------------------------------|
>| module        | Reference to the module.                                           |
>| module_apis   | The function dispatch table for this module.                       |
>| thread      | Handle to the thread on which this module's message loop is running. |
>| mq          | A queue of messages that are due for delivery to this module.        |
>| mq_cond     | A condition variable that is signaled when there are new messages.   |
>| mq_lock     | A mutex used to synchronize access to the `mq` field.                |
>| quit_worker | Message publish worker will keep running while this is `0`.          |

### Adding A Module To The Message Bus

Whenever a new module is added to the message bus a new thread is created and launched whose responsibility it is to process messages that are delivered for that module by invoking the module's message callback function. The worker thread will wait on the condition variable `mq_cond` and continuously deliver messages to the module whenever `mq_cond` is signalled. If `quit_worker` is equal to `1` then the worker thread will quit and return.

### Publishing A Message

Publishing a message to the bus essentially involves two discrete phases:

1. Queue the message to each module's message queue (the `mq` field in the module's `MODULE_INFO` structure).
2. Signal the `mq_cond` condition variable to cause the worker thread associated with the module to wake up and process the message in the queue. 

Here are the sequence of steps the publish function performs:

**Code Segment 1**
```C
01: MESSAGE_BUS_HANDLE_DATA message_bus_data = message_bus_handle
02: Lock message_bus_data.modules_lock
```

> **NOTE**: Acquiring a lock on the entire `modules` vector is a tad too coarse grained; we could consider enhancing this in the future to use a linked list and use more fine grained locking strategies - for e.g. a sliding window lock where we lock access only to the current, previous and next modules for each module in the list.
> 
> Another alternative might be to consider using lock-free collections using atomic operations. This must however be adopted only if we have:
> 
> - a performance problem due to excessive lock contention, and
> - a way of measuring performance post switching to a lock-free alternative so that we have a reliable mechanism for ascertaining that the alternative made things better.

```C
03: for each module in message_bus_data.modules
04: {
05:     Lock module.mq_lock
06:     MESSAGE_HANDLE msg = Clone input_msg (increment ref count)
07:     Append msg to module.mq
08:     Unlock module.mq_lock
09:     Signal module.mq_cond
10: }
11: Unlock message_bus_data.modules_lock
```

### Module Publish Worker

The `module_publish_worker` function is passed in a pointer to the relevant `MODULE_INFO` object as it's thread context parameter. The function's job is to basically wait on the `mq_cond` condition variable and process messages in `module.mq` when the condition is signalled. Here's the pseudo-code implementation of what it does:

**Code Segment 2**
```C
01: MODULE_INFO module_info = context
02: while(module_info.quit_worker == 0)
03: {
04:     Lock module_info.mq_lock
05:     Wait on module_info.mq_cond, module.mq_lock
06:     while(
07:             module_info.quit_worker == 0 &&
08:             module_info.mq is not empty
09:          )
10:     {
11:         MESSAGE_HANDLE msg = Dequeue module_info.mq
12:         Unlock module_info.mq_lock
13:         Deliver msg to module_info.module
14:         Destroy msg (decrement ref count)
15:         Lock module_info.mq_lock
16:     }
17: }
18: Unlock module_info.mq_lock
```

In other words, this function keeps delivering messages to the module while the module's message queue is not empty. Note that new messages can potentially be concurrently en-queued to the module's message queue while line **14** is being executed.
