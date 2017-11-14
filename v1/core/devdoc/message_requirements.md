# message Requirements

## Overview
This is the API to create a gateway message.
Messages, once created, should be considered immutable by the consuming user code.
Modules create messages and publish them to the message broker.
The message broker routes messages to other modules.
Messages have a bag of properties (name, value) and an opaque array of bytes that is the message content.

The creation of the message is considered finished at the moment when the message is transferred from the producer to the consumer.

## References

[constmap.h](../../deps/c-utility/devdoc/constmap_requirements.md)

[constbuffer.h](../../deps/c-utility/devdoc/constbuffer_requirements.md)

## Exposed API
```C
#define GATEWAY_MESSAGE_VERSION_1           0x01
#define GATEWAY_MESSAGE_VERSION_CURRENT     GATEWAY_MESSAGE_VERSION_1

typedef struct MESSAGE_HANDLE_DATA_TAG* MESSAGE_HANDLE;

typedef struct MESSAGE_CONFIG_TAG
{
    size_t size;
    const unsigned char* source;
    MAP_HANDLE sourceProperties;
}MESSAGE_CONFIG;

typedef struct MESSAGE_BUFFER_CONFIG_TAG
{
    CONSTBUFFER_HANDLE sourceContent;
    MAP_HANDLE sourceProperties;
}MESSAGE_BUFFER_CONFIG;

extern MESSAGE_HANDLE Message_Create(const MESSAGE_CONFIG* cfg);
extern MESSAGE_HANDLE Message_CreateFromByteArray(const unsigned char* source, int32_t size);
extern int32_t Message_ToByteArray(MESSAGE_HANDLE messageHandle, unsigned char* buf, int32_t size);
extern MESSAGE_HANDLE Message_CreateFromBuffer(const MESSAGE_BUFFER_CONFIG* cfg);
extern MESSAGE_HANDLE Message_Clone(MESSAGE_HANDLE message);
extern CONSTMAP_HANDLE Message_GetProperties(MESSAGE_HANDLE message);
extern const CONSTBUFFER* Message_GetContent(MESSAGE_HANDLE message);
extern CONSTBUFFER_HANDLE Message_GetContentHandle(MESSAGE_HANDLE message);
extern void Message_Destroy(MESSAGE_HANDLE message);
```

## Message_Create
```C
extern MESSAGE_HANDLE Message_Create(const MESSAGE_CONFIG* cfg);
```
Message_Create creates a new message from the cfg parameter.
**SRS_MESSAGE_02_002: [**If `cfg` is `NULL` then `Message_Create` shall return `NULL`.**]**
**SRS_MESSAGE_02_003: [**If field `source` of cfg is `NULL` and size is not zero, then `Message_Create` shall fail and return `NULL`.**]**
**SRS_MESSAGE_02_004: [**Mesages shall be allowed to be created from zero-size content.**]**
**SRS_MESSAGE_02_005: [**If `Message_Create` encounters an error while building the internal structures of the message, then it shall return `NULL`.**]**
**SRS_MESSAGE_02_019: [**`Message_Create` shall copy the `sourceProperties` to a readonly CONSTMAP.**]**
**SRS_MESSAGE_17_003: [**`Message_Create` shall copy the `source` to a readonly CONSTBUFFER.**]**
**SRS_MESSAGE_02_006: [**Otherwise, `Message_Create` shall return a non-`NULL` handle and shall set the internal ref count to "1".**]**

 ## Message_CreateFromBuffer
 ```C
 extern MESSAGE_HANDLE Message_CreateFromBuffer(const MESSAGE_BUFFER_CONFIG* cfg);
 ```
 `Message_CreateFromBuffer` creates a new message from a CONSTBUFFER source.

**SRS_MESSAGE_17_008: [** If `cfg` is `NULL` then `Message_CreateFromBuffer` shall return `NULL`.**]**
 **SRS_MESSAGE_17_009: [**If field `sourceContent` of cfg is `NULL`, then `Message_CreateFromBuffer` shall fail and return `NULL`.**]**
 **SRS_MESSAGE_17_010: [**If field `sourceProperties` of cfg is `NULL`, then `Message_CreateFromBuffer` shall fail and return `NULL`.**]**
 **SRS_MESSAGE_17_011: [**If `Message_CreateFromBuffer` encounters an error while building the internal structures of the message, then it shall return `NULL`.**]**
 **SRS_MESSAGE_17_012: [**`Message_CreateFromBuffer` shall copy the `sourceProperties` to a readonly CONSTMAP.**]**
 **SRS_MESSAGE_17_013: [**`Message_CreateFromBuffer` shall clone the CONSTBUFFER `sourceBuffer`.**]**
 **SRS_MESSAGE_17_014: [**On success, `Message_CreateFromBuffer` shall return a non-`NULL` handle and set the internal ref count to "1".**]**

 ## Message_CreateFromByteArray
 ```c
 MESSAGE_HANDLE Message_CreateFromByteArray(const unsigned char* source, int32_t size)
 ```
 Message_CreateFromByteArray creates a `MESSAGE_HANDLE` from a byte array.

 ### Implementation details
 the structure of the byte array shall be as follows:
 a header formed of the following hex characters in this order: 0xA1 0x60
 1 byte representing the message version.
 4 bytes in MSB order representing the total size of the byte array.
 4 bytes in MSB order representing the number of properties
 for every property, 2 arrays of null terminated characters representing the name of the property and the value.
 4 bytes in MSB order representing the number of bytes in the message content array
 n bytes of message content follows.

 The smallests message that can be composed has size:
    - 2 (0xA1 0x60) = fixed header
    - 1 (0x01) = message version (default value is 0x01)
    - 4 (0x00 0x00 0x00 0x0F) = array size [15 bytes in total]
    - 4 (0x00 0x00 0x00 0x00) = 0 properties that follow
    - 4 (0x00 0x00 0x00 0x00) = 0 bytes of message content


 **SRS_MESSAGE_02_022: [** If `source` is NULL then `Message_CreateFromByteArray` shall fail and return NULL. **]**

 **SRS_MESSAGE_02_023: [** If `source` is not NULL and and `size` parameter is smaller than 15 then `Message_CreateFromByteArray` shall fail and return NULL. **]**

 **SRS_MESSAGE_02_024: [** If the first two bytes of `source` are not 0xA1 0x60 then `Message_CreateFromByteArray` shall fail and return NULL. **]**

 **SRS_MESSAGE_02_037: [** If the size embedded in the message is not the same as `size` parameter then `Message_CreateFromByteArray` shall fail and return NULL. **]**
 
 **SRS_MESSAGE_02_025: [** If while parsing the message content, a read would occur past the end of the array (as indicated by `size`) then `Message_CreateFromByteArray` shall fail and return NULL. **]**

 The MESSAGE_HANDLE shall be constructed as follows:
   **SRS_MESSAGE_02_026: [** A MAP_HANDLE shall be created. **]**
   **SRS_MESSAGE_02_027: [** All the properties of the byte array shall be added to the MAP_HANDLE. **]**
   **SRS_MESSAGE_02_028: [** A structure of type `MESSAGE_CONFIG` shall be populated with the MAP_HANDLE previously constructed and the message content **]**
   **SRS_MESSAGE_02_029: [** A `MESSAGE_HANDLE` shall be constructed from the `MESSAGE_CONFIG`. **]**

 **SRS_MESSAGE_02_030: [** If any of the above steps fails, then `Message_CreateFromByteArray` shall fail and return NULL. **]**

 **SRS_MESSAGE_02_031: [** Otherwise `Message_CreateFromByteArray` shall succeed and return a non-NULL handle. **]**

## Message_ToByteArray
```c
extern const unsigned char* Message_ToByteArray(MESSAGE_HANDLE messageHandle, int32_t *size);
extern int32_t Message_ToByteArray(MESSAGE_HANDLE messageHandle, unsigned char* buf, int32_t size);

```
Creates a byte array from a `MESSAGE_HANDLE`.

**SRS_MESSAGE_02_032: [** If `messageHandle` is NULL then `Message_ToByteArray` shall fail and return -1. **]**

**SRS_MESSAGE_02_033: [** `Message_ToByteArray` shall precompute the needed memory size. **]**

**SRS_MESSAGE_17_015: [** if `buf` is NULL and `size` is not equal to zero, `Message_ToByteArray` shall return -1; **]**

**SRS_MESSAGE_17_016: [** If `buf` is NULL and `size` is equal to zero,  `Message_ToByteArray` shall return the needed memory size. **]**

**SRS_MESSAGE_17_017: [** If `buf` is not NULL and `size` is less than the needed memory size,  `Message_ToByteArray` shall return -1; **]**

**SRS_MESSAGE_02_034: [** `Message_ToByteArray` shall populate the memory with values as indicated in the implementation details. **]**

**SRS_MESSAGE_02_035: [** If any of the above steps fails then `Message_ToByteArray` shall fail and return -1. **]**

**SRS_MESSAGE_02_036: [** Otherwise `Message_ToByteArray` shall succeed, and return the byte array size. **]**

## Message_Clone
```C
extern MESSAGE_HANDLE Message_Clone(MESSAGE_HANDLE messageHandle);
```
Message_Clone creates a clone of the messageHandle. Notice: messages once created are immutable.

**SRS_MESSAGE_02_007: [**If messageHandle is `NULL` then `Message_Clone` shall return `NULL`.**]**
**SRS_MESSAGE_02_008: [**Otherwise, `Message_Clone` shall increment the internal ref count.**]**
**SRS_MESSAGE_17_001: [**`Message_Clone` shall clone the CONSTMAP handle.**]**
**SRS_MESSAGE_17_004: [**`Message_Clone` shall clone the CONSTBUFFER handle**]**
**SRS_MESSAGE_02_010: [**Message_Clone shall return messageHandle.**]**

## Message_GetProperties
```C
extern CONSTMAP_HANDLE Message_GetProperties(MESSAGE_HANDLE message);
```
Message_GetProperties returns a CONSTMAP handle that can be used to access the properties of the message.  This handle should be destroyed when no longer needed.

**SRS_MESSAGE_02_011: [**If message is `NULL` then Message_GetProperties shall return `NULL`.**]**
**SRS_MESSAGE_02_012: [**Otherwise, `Message_GetProperties` shall shall clone and return the CONSTMAP handle representing the properties of the message.**]**

## Message_GetContent
```C
extern const MESSAGE_CONTENT* Message_GetContent(MESSAGE_HANDLE message)
```
**SRS_MESSAGE_02_013: [**If message is `NULL` then Message_GetContent shall return `NULL`.**]**
**SRS_MESSAGE_02_014: [**Otherwise, Message_GetContent shall return a non-`NULL` const pointer to a structure of type CONSTBUFFER.**]**
**SRS_MESSAGE_02_015: [**The CONSTBUFFER's field `size` shall have the same value as the cfg's field `size`.**]**
**SRS_MESSAGE_02_016: [**The CONSTBUFFER's field `buffer` shall compare equal byte-by-byte to the cfg's field `source`.**]**
The return of this function needs no free.

## Message_GetContentHandle
```C
extern CONSTBUFFER_HANDLE Message_GetContentHandle(MESSAGE_HANDLE message);
```

This function returns a CONSTBUFFER handle that can be used to access the content. This handle should be destroyed when no longer needed.

**SRS_MESSAGE_17_006: [**If message is `NULL` then `Message_GetContentHandle` shall return `NULL`.**]**
**SRS_MESSAGE_17_007: [**Otherwise, `Message_GetContentHandle` shall shall clone and return the CONSTBUFFER_HANDLE representing the message content.**]**

## Message_Destroy(MESSAGE_HANDLE message)
```C
extern void Message_Destroy(MESSAGE_HANDLE message);
```
**SRS_MESSAGE_02_017: [**If message is `NULL` then `Message_Destroy` shall do nothing.**]**
**SRS_MESSAGE_02_020: [**Otherwise, `Message_Destroy` shall decrement the internal ref count of the message.**]**
**SRS_MESSAGE_17_002: [**`Message_Destroy` shall destroy the CONSTMAP properties.**]**
**SRS_MESSAGE_17_005: [**`Message_Destroy` shall destroy the CONSTBUFFER.**]**
**SRS_MESSAGE_02_021: [**If the ref count is zero then the allocated resources are freed.**]**
