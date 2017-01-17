# control message Requirements

## Overview
This is the API to create a gateway out of process control message.
Control messages have a base structure in C, optionally followed by a block of 
data, the content is determined by the type of control message.
Control messages are serialized for transmission out of process. The details of 
both the C structures and the corresponding serialized structure is given in 
[Control messages in out process modules](out-process-control-messages.md).


## References

[On out process gateway modules](on-out-process-gateway-modules.md)

[Control messages in out process modules](out-process-control-messages.md)

## Exposed API
```C
#define CONTROL_MESSAGE_VERSION_1           0x10
#define CONTROL_MESSAGE_VERSION_CURRENT     CONTROL_MESSAGE_VERSION_1

#define CONTROL_MESSAGE_TYPE_VALUES      \
    CONTROL_MESSAGE_TYPE_ERROR,           \
    CONTROL_MESSAGE_TYPE_MODULE_CREATE,          \
    CONTROL_MESSAGE_TYPE_MODULE_REPLY,    \
    CONTROL_MESSAGE_TYPE_MODULE_START,           \
    CONTROL_MESSAGE_TYPE_MODULE_DESTROY

DEFINE_ENUM(CONTROL_MESSAGE_TYPE, CONTROL_MESSAGE_TYPE_VALUES);

typedef struct CONTROL_MESSAGE_TAG
{
    uint8_t  version;
    CONTROL_MESSAGE_TYPE  type;
}CONTROL_MESSAGE;

typedef struct MESSAGE_URI_TAG
{
    uint8_t  uri_type;
    char*  uri;
}MESSAGE_URI;

typedef struct CONTROL_MESSAGE_MODULE_CREATE_TAG
{
	CONTROL_MESSAGE base;
    uint8_t gateway_message_version;
    MESSAGE_URI uri;
    uint32_t args_size;
    char* args;
}CONTROL_MESSAGE_MODULE_CREATE;

typedef struct CONTROL_MESSAGE_MODULE_REPLY_TAG
{
    CONTROL_MESSAGE base;
    uint8_t create_status;
}CONTROL_MESSAGE_MODULE_REPLY;

GATEWAY_EXPORT CONTROL_MESSAGE * ControlMessage_CreateFromByteArray(const unsigned char* source, int32_t size);

GATEWAY_EXPORT void ControlMessage_Destroy(CONTROL_MESSAGE * message, bool destroy_args);

GATEWAY_EXPORT int32_t ControlMessage_ToByteArray(CONTROL_MESSAGE * message, unsigned char* buf, int32_t size);

```

## ControlMessage_CreateFromByteArray
```C
GATEWAY_EXPORT CONTROL_MESSAGE * ControlMessage_CreateFromByteArray(const unsigned char* source, int32_t size);
```

`ControlMessage_CreateFromByteArray` creates a `CONTROL_MESSAGE` from a byte array. 

**SRS_CONTROL_MESSAGE_17_001: [** If `source` is NULL, then this function shall return `NULL`. **]**

**SRS_CONTROL_MESSAGE_17_002: [** If `size` is smaller than 8, then this function shall return `NULL`. **]**

**SRS_CONTROL_MESSAGE_17_003: [** If the first two bytes of `source` are not 0xA1 0x6C then this function shall fail and return NULL. **]**

**SRS_CONTROL_MESSAGE_17_004: [** If the version is not equal to `CONTROL_MESSAGE_VERSION_CURRENT`, then this 
function shall return `NULL`. **]**
NOTE: This is the intial version of the control message, only the initial 
version is recognized.

**SRS_CONTROL_MESSAGE_17_005: [** This function shall read the version, type and size from the byte stream. **]**

**SRS_CONTROL_MESSAGE_17_006: [** If the size embedded in the message is not the same as `size` parameter then this function shall fail and return `NULL`. **]**

**SRS_CONTROL_MESSAGE_17_007: [** This function shall return `NULL` if the type is not a valid enum value of 
`CONTROL_MESSAGE_TYPE` or `CONTROL_MESSAGE_TYPE_ERROR`. **]**

### If message type is `CONTROL_MESSAGE_TYPE_MODULE_CREATE`:

**SRS_CONTROL_MESSAGE_17_008: [** This function shall allocate a `CONTROL_MESSAGE_CREATE_MSG` structure. **]**

**SRS_CONTROL_MESSAGE_17_009: [** If the total message size is not at least 20 bytes, then this function shall 
return `NULL` **]**

**SRS_CONTROL_MESSAGE_17_037: [** This function shall read the `gateway_message_version`. **]**

**SRS_CONTROL_MESSAGE_17_012: [** This function shall read the `uri_type`, `uri_size`, and the `uri`. **]**

**SRS_CONTROL_MESSAGE_17_013: [** This function shall allocate `uri_size` bytes for the `uri`. **]**

**SRS_CONTROL_MESSAGE_17_014: [** This function shall read the `args_size` and `args` from the byte stream. **]**

**SRS_CONTROL_MESSAGE_17_015: [** This function shall allocate `args_size` bytes for the `args` array. **]**

**SRS_CONTROL_MESSAGE_17_018: [** Reading past the end of the byte array shall cause this function to fail and return `NULL`. **]**

### If message type is `CONTROL_MESSAGE_TYPE_MODULE_REPLY`:

**SRS_CONTROL_MESSAGE_17_019: [** This function shall allocate a `CONTROL_MESSAGE_MODULE_REPLY` structure. **]**

**SRS_CONTROL_MESSAGE_17_020: [** If the total message size is not at least 9 bytes, then this function shall 
fail and return `NULL`. **]**

**SRS_CONTROL_MESSAGE_17_021: [** This function shall read the `create_status` from the byte stream. **]**



### If the message type is `CONTROL_MESSAGE_TYPE_START` or `CONTROL_MESSAGE_TYPE_DESTROY`:

**SRS_CONTROL_MESSAGE_17_023: [** This function shall allocate a `CONTROL_MESSAGE` structure. **]**

### For all valid messages

**SRS_CONTROL_MESSAGE_17_022: [** This function shall release all allocated memory upon failure. **]**

**SRS_CONTROL_MESSAGE_17_036: [** This function shall return NULL upon failure. **]**

**SRS_CONTROL_MESSAGE_17_024: [** Upon valid reading of the byte stream, this function shall assign the message version and type into the `CONTROL_MESSAGE` base structure. **]**

**SRS_CONTROL_MESSAGE_17_025: [** Upon success, this function shall return a valid pointer to the `CONTROL_MESSAGE` base. **]**

## ControlMessage_Destroy
```c
GATEWAY_EXPORT void ControlMessage_Destroy(CONTROL_MESSAGE * message, bool destroy_args);
```

**SRS_CONTROL_MESSAGE_17_026: [** If `message` is `NULL` this function shall do nothing. **]**

**SRS_CONTROL_MESSAGE_17_027: [** This function shall release all memory allocated in this structure. **]**

**SRS_CONTROL_MESSAGE_17_028: [** For a `CONTROL_MESSAGE_CREATE_MSG` message type, all memory allocated shall be defined as all the memory allocated by `ControlMessage_CreateFromByteArray`. **]**

**SRS_CONTROL_MESSAGE_17_030: [** This function shall release `message` for all message types. **]**


## ControlMessage_ToByteArray
```c
GATEWAY_EXPORT int32_t ControlMessage_ToByteArray(CONTROL_MESSAGE * message, unsigned char* buf, int32_t size);
```

This function will serialize a control message into a transmittable byte array.

**SRS_CONTROL_MESSAGE_17_031: [** If `message` is `NULL`, then this function shall return -1. **]**

**SRS_CONTROL_MESSAGE_17_032: [** If `buf` is `NULL`, but `size` is not zero, then this function shall return -1. **]**

**SRS_CONTROL_MESSAGE_17_033: [** This function shall populate the memory with values as indicated in 
[control messages in out process modules](out-process-control-messages.md). **]**

**SRS_CONTROL_MESSAGE_17_034: [** If any of the above steps fails then this function shall fail and return -1. **]**

**SRS_CONTROL_MESSAGE_17_035: [** Upon success this function shall return the byte array size. **]**

