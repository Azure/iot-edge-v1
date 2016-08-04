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
    int                     publish_socket;
    STRING_HANDLE           url;
    LOCK_HANDLE             modules_lock;
}MESSAGE_BUS_HANDLE_DATA;
```

Here's a description of the members of `MESSAGE_BUS_HANDLE_DATA`.

>| Field       | Description                                                                  |
>|-------------|------------------------------------------------------------------------------|
>| modules        | Vector of modules where each element is an instance of `MODULE_INFO`. |
>| publish_socket | The socket to publish messages to the broadcast bus.                       |
>| url            | A URL to uniquely describe the broadcast bus.        |
>| modules_lock   | A mutex used to synchronize access to the `modules` field.                 |

### The Module Info
    
Each module that is connected to the message bus is represented using a structure of type `MODULE_INFO` which looks like so:

```C
typedef struct MODULE_INFO_TAG
{
    MODULE_HANDLE           module;
    MODULE_APIS             module_apis;
    THREAD_HANDLE           thread;
    int                     receive_socket;
    LOCK_HANDLE				socket_lock;
    STRING_HANDLE			quit_message_guid;
}MODULE_INFO;
```

A description of the fields in the `MODULE_INFO` structure follows:

>| Field       | Description                                                          |
>|-----------------------|----------------------------------------------------------------------|
>| module                | Reference to the module.                                             |
>| module_apis           | The function dispatch table for this module.                         |
>| thread                | Handle to the thread on which this module's message loop is running. |
>| receive\_socket       | The delivery socket for this module.        |
>| socket\_lock          | A mutex used to synchronize access to the nanomsg read function.  |
>| quit\_message\_guid   | A unique ID sent to the worker thread to close it.          |

### Adding A Module To The Message Bus

Whenever a new module is added to the message bus a new thread is created and launched whose responsibility it is to process messages that are delivered for that module by invoking the module's message callback function. The worker thread will wait on receive_socket and deliver messages to the module. If `quit_worker` is equal to `1` then the worker thread will quit and return.

### Publishing A Message

Using a messaging system like nanomsg strongly encourages the message bus to pass messages as serialized data, rather than passing messages as pointers or handles.

Nanomsg sockets are considered thread-safe, which means we can avoid locking during a publish unless we need access to critical module data.

**Message publishing pseudo code**

```c
01: MESSAGE_HANDLE msg = Message_Clone(message)
02: unsigned char* serial_message = Message_ToByteArray( msg, &size)
03: void* nn_msg = nn_allocmsg(size, 0)
04: memcpy(nn_msg, serial_message, size)
05: int nbytes = nn_send(bus_data->publish_socket, nn_msg, NN_MSG, 0)
06: free(serial_message)
07: Message_Destroy(msg)
```

The call to nn_alloc creates a buffer managed by nanomsg.  THis allows for zero copy message passing as well as memory management inside nanomsg. This buffer will be destroyed after a successful call.

### Module Publish Worker

The `module_publish_worker` function is passed in a pointer to the relevant `MODULE_INFO` object as it's thread context parameter. The function's job is to basically wait on the receive socket and process messages when received. Here's the pseudo-code implementation of what it does:

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
09:         if (nbytes == MESSAGE_BUS_GUID_SIZE && (message == module_info->quit_message_guid )
10:         { 
11:             should_continue = false
12:         }
13:         else
14:         {
15:             MESSAGE_HANDLE msg = Message_CreateFromByteArray(buf, nbytes)
16:             Deliver msg to module_info.module
17:             Destroy msg
18:         }
19:         nn_freemsg(buf)
20:     }
21:     else
22:     {
23:         should_continue = false;
24:     }
25: }
```

Why do we need the `socket_lock`?  Helgrind and drd found a race condition between `nn_recv` and `nn_close` on the internal socket data. The socket lock prevents this race condition.

### Closing the Module Publish Worker

When the MessageBus adds a module, it creates the `quit_message_guid` as a unique ID to send to the worker thread to signal the thread should close.  This guid is not serialized as a message object.

The following is pseudo-code for stopping the Module Publish Worker thread:

```c
01: nn_send(receive_socket, module_info->quit_message_guid, MESSAGE_BUS_GUID_SIZE, 0)
02: Lock module_info.socket_lock
03: nn_close(module_info->receive_socket)
04: Unlock module_info.socket_lock
05: ThreadAPI_Join(module_info->thread, &thread_result)
```

If for any reason the send fails, closing the socket will guarantee the next read will fail, and the thread will terminate.
