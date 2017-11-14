# Message Format

Azure IoT Edge communicates with modules running in another process via _control_ messages. In certain cases, the out-of-process modules respond to the control messages they receive. Additionally, IoT Edge facilitates communication between modules via _module_ messages. This document describes the format of these messages.

---------------------------------

## Control Messages (IoT Edge to Module)

### Create

After an out-of-process module connects to the control channel, it receives this message to establish the session. It is expected to respond with a Create Response message (see below).

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |      0xA1     |      0x6C     |  Control Ver  |  Control Type |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Total Size                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |   Create Ver  |   Msg Ch Type |    Message Channel URI Size   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    Msg Ch URI Size (cont.)    |      Message Channel URI      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         Arguments Size                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                            Arguments                          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

#### Header: 2 bytes
The first two bytes of any control message are 0xA1, 0x6C.

#### Control Version: 1 byte
The version of the control message structure (currently 1).

#### Control Type: 1 byte
The 'Create' control message type identifier (1).

#### Total Size: 4 bytes
The size, in bytes, of the entire message, including this and all preceding control message fields.

#### Create Version: 1 byte
The version of the Create control message structure (currently 1).

#### Message Channel Type: 1 byte
A channel type identifier that is specific to the underlying messaging library. In version 1 of the Create control message structure, this value is equivalent to the symbol NN_PAIR, defined by nanomsg.

#### Message Channel URI Size: 4 bytes
The size in bytes of the Message Channel URI (including the null-terminating char), which follows this field.

#### Message Channel URI: variable (integral # of bytes)
The null-terminated string of UTF8 characters representing the complete URI used to connect to the module message channel. This value should be passed directly to the underlying messaging library (e.g., nanomsg) without modification.

#### Arguments Size: 4 bytes
Immediately follows the terminating null byte of the Message Channel URI. Indicates the size, in bytes, of the module arguments which follow this field.

#### Arguments: variable (integral # of bytes)
An array of bytes which will be passed to the out-of-process module.

### Start

An out-of-process module recieves this message after responding to the Create message (unless the Create Response reported an error). The message is only sent after all links between modules have been established, and therefore signals that it is safe to begin publishing module messages.

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |      0xA1     |      0x6C     |  Control Ver  |  Control Type |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Total Size                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

#### Header: 2 bytes
The first two bytes of any control message are 0xA1, 0x6C.

#### Control Version: 1 byte
The version of the control message structure (currently 1).

#### Control Type: 1 byte
The 'Start' control message type identifier (3).

#### Total Size: 4 bytes
The size, in bytes, of the entire message, including this and all preceding control message fields.

### Destroy

An out-of-process module recieves this message when IoT Edge is shutting down.

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |      0xA1     |      0x6C     |  Control Ver  |  Control Type |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Total Size                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

#### Header: 2 bytes
The first two bytes of any control message are 0xA1, 0x6C.

#### Control Version: 1 byte
The version of the control message structure (currently 1).

#### Control Type: 1 byte
The 'Destroy' control message type identifier (4).

#### Total Size: 4 bytes
The size, in bytes, of the entire message, including this and all preceding control message fields.

---------------------------------

## Control Messages (Module to IoT Edge)

### Create Response

An out-of-process module sends this message to IoT Edge after it has received the Create message and has completed (or attempted) any initialization.

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |      0xA1     |      0x6C     |  Control Ver  |  Control Type |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Total Size                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Create Result |                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

#### Header: 2 bytes
The first two bytes of any control message are 0xA1, 0x6C.

#### Control Version: 1 byte
The version of the control message structure (currently 1).

#### Control Type: 1 byte
The module-to-edge control message type identifier (2).

#### Total Size: 4 bytes
The size, in bytes, of the entire message, including this and all preceding control message fields.

#### Create Result: 1 byte
The result of the 'Create' operation. 0 for success, 1 for error.

### Detach

An out-of-process module sends this message when it needs to detach from the gateway (whether or not IoT Edge sent a Destroy message).

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |      0xA1     |      0x6C     |  Control Ver  |  Control Type |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Total Size                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Detach    |                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

#### Header: 2 bytes
The first two bytes of any control message are 0xA1, 0x6C.

#### Control Version: 1 byte
The version of the control message structure (currently 1).

#### Control Type: 1 byte
The module-to-edge control message type identifier (2).

#### Total Size: 4 bytes
The size, in bytes, of the entire message, including this and all preceding control message fields.

#### Detach: 1 byte
A marker signifying that the module is terminating the connection to IoT Edge (-1).

---------------------------------

## Module Messages

An out-of-process module sends and receives module messages.

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |      0xA1     |      0x60     |           Total Size          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |       Total Size (cont.)      |          # Properties         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |      # Properties (cont.)     |           Properties          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Content Size                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                            Content                            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

#### Header: 2 bytes
The first two bytes of any module message are 0xA1, 0x60.

#### Total Size: 4 bytes
The size, in bytes, of the entire message, including this field and the header bytes.

#### # Properties: 4 bytes
The number of properties encoded immediately after this field

#### Properties: variable (integral # of bytes)
A null-delimited sequence of key/value pairs in UTF8, e.g. `key1\0value1\0key2\0value2\0`.

#### Content Size: 4 bytes
The size, in bytes, of the message body.

#### Content: variable (integral # of bytes)
The message body, as an array of bytes.
