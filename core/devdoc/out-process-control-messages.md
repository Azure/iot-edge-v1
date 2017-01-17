Control messages in out process modules
=======================================

When a gateway communicates with a module that is hosted in an external process
there are two communication channels established between the two processes that
are used for:

1.  Passing *control* messages that signal the creation, start and destruction
    of the module. These messages always originate from the gateway and are sent
    to the out-process module.

2.  Exchanging *data* messages between the two processes.

This document specifies the format of the control messages that will be sent
from the gateway for the different kinds of supported messages.

Base control message structure
------------------------------

The basic structure of the control message is simple. It will look like this:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
+----------------------+   --+
| header1: uint8_t     |     |
+----------------------+     |
| header2: uint8_t     |     |
+----------------------+     |
| version: uint8_t     |     |
+----------------------+     |
| type: uint8_t        |     |
+----------------------+     |   Header
|                      |     |
| total_size: uint32_t |     |
|                      |     |
+----------------------+   --+
|                      |     |
| data                 |     |   Body
|                      |     |
+----------------------+   --+
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This structure is always transmitted over the “wire” in network byte order (big
endian). Here’s what each of these fields mean:

-   **header1** - This is an unsigned byte containing 0xA1.

-   **header2** - This is an unsigned byte containing 0x6C.

-   **version** - This is an unsigned byte containing the version number that
    signifies the structure of the rest of the message. As modifications to the
    structure of any of the supported messages or the base message structure are
    made this version number is increased. When a module host process or the
    gateway process receives a message with a version number that it does not
    recognize it is expected to treat that as an error. To begin with the
    version number will have the hexadecimal value `0x01`.

-   **type** - This is an enumeration that indicates the message type. This is
    used to signify whether the message is a *create*, *start* or *destroy*
    message.

-   **total_size** - This unsigned 32-bit number indicates the total message
    size including the size of the header.

-   **data** - The `total_size` field is followed by the actual message itself
    which will change depending on the value of the `type` field.

Represented in C syntax, this is what the struct looks like:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ c
typedef enum CONTROL_MESSAGE_TYPE_TAG
{
    CONTROL_MESSAGE_TYPE_ERROR,
    CONTROL_MESSAGE_TYPE_MODULE_CREATE,
    CONTROL_MESSAGE_TYPE_MODULE_CREATE_REPLY,
    CONTROL_MESSAGE_TYPE_MODULE_START,
    CONTROL_MESSAGE_TYPE_MODULE_DESTROY
}CONTROL_MESSAGE_TYPE;

typedef struct CONTROL_MESSAGE_TAG
{
    uint8_t  version;
    CONTROL_MESSAGE_TYPE  type;
} CONTROL_MESSAGE;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

### Why not use the gateway message API for building & serializing control messages?

The gateway message API is essentially an object that captures a dictionary of
properties along with an array of bytes which works well as a generic message
format for passing messages between modules. When talking about a message format
that is meant to be serialized across process boundaries (or network boundaries)
however the aim is to come up with a format that is compact and efficient.
Encoding a string dictionary into control messages does not provide justifiable
benefit for the cost that is incurred in terms of serialization.

Create message
--------------

The *create* message is sent by the gateway to the module host process to
indicate that the remote module should be loaded and instantiated. The [high
level design document](./on-out-process-gateway-modules.md) specifies what this
message should be composed of. The `type` field will have the value
`CONTROL_MESSAGE_TYPE_MODULE_CREATE` and the body of the message will be a struct that
looks like this (as specified in the high level design document):

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct MESSAGE_URI_TAG
{
    uint8_t   uri_type;
    uint32_t  uri_size;
    uint8_t*  uri;
}MESSAGE_URI;

typedef struct CONTROL_MESSAGE_MODULE_CREATE_TAG
{

    CONTROL_MESSAGE  base;
    uint8_t gateway_message_version;
    MESSAGE_URI   uri;
    uint32_t  args_size;
    char*     args;
}CONTROL_MESSAGE_MODULE_CREATE;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The structure of the serialized message will look like:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
+---------------------------+                           --+
| CONTROL_MESSAGE           |                             |  Header
+---------------------------+                           --+
| gateway_message_version:  |                             |
| uint8_t                   |                             |
+---------------------------+  --+                        |
| uri_type: uint8_t         |    |                        |
+---------------------------+    |                        |
|                           |    |                        |
| uri_size: uint32_t        |    |                        |
|                           |    |  MESSAGE_URI           |
+---------------------------+    |                        |
| uri[0]                    |    |                        |
| [...]                     |    |                        |
| uri[uri_size-2]           |    |                        |
| '\0'                      |    |                        |
+---------------------------+  --+                        |  Body
|                           |    |                        |
| args_size: uint32_t       |    |                        |
|                           |    |                        |
+---------------------------+    |  module args           |
| args[0]                   |    |                        |
| [...]                     |    |                        |
| args[args_size-2]         |    |                        |
| '\0'                      |    |                        |
+---------------------------+  --+                      --+
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Create message reply
--------------------

This message is sent by the module host process to indicate whether the module
creation was successful or not. The message `type` field will have the value
`CONTROL_MESSAGE_TYPE_MODULE_CREATE_REPLY` and the body of the message is a 
single unsigned 8-bit value, with 0 indicating success and any non-zero value 
indicating failure. Here’s what the struct looks like:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct CONTROL_MESSAGE_MODULE_REPLY_TAG
{
    CONTROL_MESSAGE  base;
            uint8_t  status;
}CONTROL_MESSAGE_MODULE_REPLY;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The serialed format of the message is:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
+------------------------+                           --+
| CONTROL_MESSAGE        |                             |  Header
+------------------------+                           --+
| status: uint8_t        |                             |  Body
+------------------------+                           --+
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Start module
------------

This message is sent by the gateway to signal the module host process that the
`Module_Start` API in the remote module should be invoked. There is no message
body for this message. The `type` field is set to the value
`CONTROL_MESSAGE_TYPE_MODULE_START`.

Destroy module
--------------

This message is sent by the gateway to signal the module host process that the
`Module_Destroy` API in the remote module should be invoked and the module
should be unloaded. There is no message body for this message. The `type` field
is set to the value `CONTROL_MESSAGE_TYPE_MODULE_DESTROY`.
