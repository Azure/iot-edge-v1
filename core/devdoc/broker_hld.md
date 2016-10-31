Message Broker (PubSub)
==============

High level design
-----------------

### Overview

The message broker (or just the "broker") is central to the gateway. The broker is responsible for sending and receiving messages between interested parties. In the case of the gateway, the interested parties are *modules*. This document describes the high level design of the broker along with descriptions of flow of control.

### Design Goals

There are two guiding principles that influence the broker's design:

- The broker APIs shall be thread-safe, i.e., any piece of code running on any arbitrary thread is allowed to call any API on the broker at any time, concurrently or otherwise.
- When the broker delivers messages to modules it shall do so serially for a given module, i.e., at no point shall it *concurrently* invoke a module's receive function. Modules do not need to synchronize access to their internal state.

### The Broker's Internal State

An instance of the broker is represented in code via an opaque `BROKER_HANDLE` value and is a reference counted object. The `BROKER_HANDLE` is backed by a data structure called `BROKER_HANDLE_DATA` which looks like this:

```C
typedef struct BROKER_HANDLE_DATA_TAG
{
    VECTOR_HANDLE           modules;
    int                     publish_socket;
    STRING_HANDLE           url;
    LOCK_HANDLE             modules_lock;
}BROKER_HANDLE_DATA;
```

`BROKER_HANDLE_DATA` has the following members:

>| Field          | Description                                                           |
>|----------------|-----------------------------------------------------------------------|
>| modules        | Vector of modules where each element is an instance of `MODULE_INFO`. |
>| publish_socket | The socket used to publish messages to other modules.                 |
>| url            | A URL to uniquely describe the broker.                                |
>| modules_lock   | A mutex used to synchronize access to the `modules` field.            |

Each module that is connected to the broker is represented using a structure of type `MODULE_INFO` which looks like this:

```C
typedef struct MODULE_INFO_TAG
{
    MODULE_HANDLE           module;
    MODULE_API              module_api;
    THREAD_HANDLE           thread;
    int                     receive_socket;
    LOCK_HANDLE             socket_lock;
    STRING_HANDLE           quit_message_guid;
}MODULE_INFO;
```

`MODULE_INFO` has the following members:

>| Field                 | Description                                                          |
>|-----------------------|----------------------------------------------------------------------|
>| module                | Reference to the module.                                             |
>| module_api           | The function dispatch table for this module.                         |
>| thread                | Handle to the thread on which this module's message loop is running. |
>| receive\_socket       | The delivery socket for this module.                                 |
>| socket\_lock          | A mutex used to synchronize access to the nanomsg read function.     |
>| quit\_message\_guid   | A unique ID sent to the worker thread to close it.                   |

### Attaching a Module to the Broker

When a new module is added to the broker a worker thread is created to receive messages for that module. The worker thread will wait on `receive_socket` and deliver messages to the module's receive callback function. If the buffer received is equal to the `quit_message_guid`, then the loop will terminate.

### Publishing A Message

Using a messaging system like nanomsg strongly encourages the broker to pass messages as serialized data, rather than passing messages as pointers or handles.

Nanomsg sockets are considered thread-safe, which means we can avoid locking during a publish unless we need access to critical module data.

In published messages, the topic is always the value of `source` as a `MODULE_HANDLE` type. This will be copied into the message in the platform-specific serialization of the type.

**Message publishing pseudo code**

```c
01: MESSAGE_HANDLE msg = Message_Clone(message)
02: message_size = Message_ToByteArray(msg, NULL, 0)
03: buffer_size = message_size + sizeof(MODULE_HANDLE)
04: void* nn_msg = nn_allocmsg(buffer_size, 0)
05: memcpy (nn_msg, source, sizeof(MODULE_HANDLE))
06: Message_ToByteArray(msg, nn_msg+sizeof(MODULE_HANDLE), message_size)
07: int nbytes = nn_send(broker_data->publish_socket, nn_msg, NN_MSG, 0)
08: free(nn_msg)
09: Message_Destroy(msg)
```

The call to `nn_allocmsg` creates a buffer managed by nanomsg.  This allows for zero copy message passing as well as memory management inside nanomsg. This buffer will be destroyed after a successful call.

### Module Worker

The `module_worker` function is passed in a pointer to the relevant `MODULE_INFO` object as it's thread context parameter. The function's job is to basically wait on the receive socket and process messages when received. Here's the pseudo-code implementation of what it does:

**Code Segment 2**
```c

01: MODULE_INFO module_info = context
02: while(should_continue)
03: {
04:     Lock module_info.socket_lock
05:     Wait on nn_recv(module_info.receive_socket, &buf, NN_MSG, 0)
06:     Unlock module_info.socket_lock
07:     if (nbytes > 0)
08:     {
09:         if (nbytes == BROKER_GUID_SIZE && (message == module_info->quit_message_guid )
10:         { 
11:             should_continue = false
12:         }
13:         else
14:         {   
15:             Strip off topic from received buffer.
16:             MESSAGE_HANDLE msg = Message_CreateFromByteArray(buf, nbytes)
17:             Deliver msg to module_info.module
18:             Destroy msg
19:         }
20:         nn_freemsg(buf)
21:     }
22:     else
23:     {
24:         should_continue = false;
25:     }
26: }
```

Why do we need the `socket_lock`?  Helgrind and drd found a race condition between `nn_recv` and `nn_close` on the internal socket data. The socket lock prevents this race condition.

### Closing the Module Publish Worker

When the Broker adds a module, it creates the `quit_message_guid` as a unique ID to send to the worker thread to signal the thread should close.  This guid is not serialized as a message object.

The following is pseudo-code for stopping the Module Publish Worker thread:

```c
01: nn_send(receive_socket, module_info->quit_message_guid, BROKER_GUID_SIZE, 0)
02: Lock module_info.socket_lock
03: nn_close(module_info->receive_socket)
04: Unlock module_info.socket_lock
05: ThreadAPI_Join(module_info->thread, &thread_result)
```

If for any reason the send fails, closing the socket will guarantee the next read will fail, and the thread will terminate.

### Routing

The broker will receive a series of links, each with a valid source module handle and a valid sink module handle. The link entry specifies that the source will publish a message expected to be consumed by the sink. Therefore, a sink will subscribe to a source.

As described above in *Publishing A Message*, the PubSub Broker will always publish the message using the `MODULE_HANDLE` as the topic. For each link pair sent to the Broker, the sink will subscribe to the source `MODULE_HANDLE`.  

The following is pseudo-code for Broker_AddLink:
```c
01: Lock modules_lock
02: Locate module_info for sink module.
03: nn_setsockopt(sink->receive_socket, NN_SUB, NN_SUB_SUBSCRIBE, &source,sizeof(MODULE_HANDLE));
04: Unlock modules_lock
```

When removing the link, the Broker will unsubscribe to the source `MODULE_HANDLE`.  The following is pseudo-code for Broker_RemoveLink:
```c
01: Lock modules_lock
02: Locate module_info for sink module.
03: nn_setsockopt(sink->receive_socket, NN_SUB, NN_SUB_UNSUBSCRIBE, &source,sizeof(MODULE_HANDLE));
04: Unlock modules_lock
```

The MODULE_HANDLE was chosen over the module name simply because the handle is a fixed size, making it quick and easy to strip off of the received buffer.

