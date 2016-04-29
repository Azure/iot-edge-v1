# message Requirements


 

##Overview
This is the API to create a gateway message. 
Messages, once created, should be considered immutable by the consuming user code. 
Modules create messages. Modules publish the said messages to the message bus. 
The message bus feeds the messages to the consumers. 
Messages on the message bus have a bag of properties (name, value) and an opaque array of bytes that is the message content.

The creation of the message is considered finished at the moment when the message is transferred from the producer to the consumer.

##References

[constmap.h](../azure-c-shared-utility/c/devdoc/constmap_requirements.md)

[constbuffer.h](../azure-c-shared-utility/c/devdoc/constbuffer_requirements.md)

##Exposed API
```C
#ifndef MESSAGE_H
#define MESSAGE_H

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>
extern "C"
{
#else
#include <stdint.h>
#include <stddef.h>
#endif

#include "macro_utils.h"
#include "azure_c_shared_utility/map.h"
#include "azure_c_shared_utility/constmap.h"
#include "azure_c_shared_utility/constbuffer.h"
#include "azure_c_shared_utility/buffer_.h"

/*this is the interface of any message*/

typedef struct MESSAGE_HANDLE_DATA_TAG* MESSAGE_HANDLE;

/*all messages are constructed from this */
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

/*this creates a new message */
extern MESSAGE_HANDLE Message_Create(const MESSAGE_CONFIG* cfg);

/* this creates a new message from a CONSTBUFFER content */
extern MESSAGE_HANDLE Message_CreateFromBuffer(const MESSAGE_BUFFER_CONFIG* cfg);

/*this clones a message. Since messages are immutable, it would only increment the inner count*/
extern MESSAGE_HANDLE Message_Clone(MESSAGE_HANDLE message);

/*this gets an immutable map (dictionary) of all the properties of the message*/
extern CONSTMAP_HANDLE Message_GetProperties(MESSAGE_HANDLE message);

/*this gets the message content*/
extern const CONSTBUFFER* Message_GetContent(MESSAGE_HANDLE message);

/*this gets the message content handle*/
extern const CONSTBUFFER_HANDLE Message_GetContentHandle(MESSAGE_HANDLE message);

/*this destroys the message*/
extern void Message_Destroy(MESSAGE_HANDLE message);

#ifdef __cplusplus
}
#else
#endif


#endif /*MESSAGE_H*/

```

##Message_Create
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
 
 ##Message_CreateFromBuffer
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
 
##Message_Clone
```C
extern MESSAGE_HANDLE Message_Clone(MESSAGE_HANDLE messageHandle);
```
Message_Clone creates a clone of the messageHandle. Notice: messages once created are immutable.

**SRS_MESSAGE_02_007: [**If messageHandle is `NULL` then `Message_Clone` shall return `NULL`.**]**
**SRS_MESSAGE_02_008: [**Otherwise, `Message_Clone` shall increment the internal ref count.**]**
**SRS_MESSAGE_17_001: [**`Message_Clone` shall clone the CONSTMAP handle.**]**
**SRS_MESSAGE_17_004: [**`Message_Clone` shall clone the CONSTBUFFER handle**]**
**SRS_MESSAGE_02_010: [**Message_Clone shall return messageHandle.**]**

##Message_GetProperties
```C
extern CONSTMAP_HANDLE Message_GetProperties(MESSAGE_HANDLE message);
```
Message_GetProperties returns a CONSTMAP handle that can be used to access the properties of the message.  This handle should be destroyed when no longer needed.

**SRS_MESSAGE_02_011: [**If message is `NULL` then Message_GetProperties shall return `NULL`.**]**
**SRS_MESSAGE_02_012: [**Otherwise, `Message_GetProperties` shall shall clone and return the CONSTMAP handle representing the properties of the message.**]**

##Message_GetContent
```C
extern const MESSAGE_CONTENT* Message_GetContent(MESSAGE_HANDLE message)
```
**SRS_MESSAGE_02_013: [**If message is `NULL` then Message_GetContent shall return `NULL`.**]**
**SRS_MESSAGE_02_014: [**Otherwise, Message_GetContent shall return a non-`NULL` const pointer to a structure of type CONSTBUFFER.**]** 
**SRS_MESSAGE_02_015: [**The CONSTBUFFER's field `size` shall have the same value as the cfg's field `size`.**]**
**SRS_MESSAGE_02_016: [**The CONSTBUFFER's field `buffer` shall compare equal byte-by-byte to the cfg's field `source`.**]**
The return of this function needs no free.

##Message_GetContentHandle
```C
extern CONSTBUFFER_HANDLE Message_GetContentHandle(MESSAGE_HANDLE message);
```

This function returns a CONSTBUFFER handle that can be used to access the content. This handle should be destroyed when no longer needed.

**SRS_MESSAGE_17_006: [**If message is `NULL` then `Message_GetContentHandle` shall return `NULL`.**]**
**SRS_MESSAGE_17_007: [**Otherwise, `Message_GetContentHandle` shall shall clone and return the CONSTBUFFER_HANDLE representing the message content.**]**

##Message_Destroy(MESSAGE_HANDLE message)
```C
extern void Message_Destroy(MESSAGE_HANDLE message);
```
**SRS_MESSAGE_02_017: [**If message is `NULL` then `Message_Destroy` shall do nothing.**]**
**SRS_MESSAGE_02_020: [**Otherwise, `Message_Destroy` shall decrement the internal ref count of the message.**]** 
**SRS_MESSAGE_17_002: [**`Message_Destroy` shall destroy the CONSTMAP properties.**]**
**SRS_MESSAGE_17_005: [**`Message_Destroy` shall destroy the CONSTBUFFER.**]**
**SRS_MESSAGE_02_021: [**If the ref count is zero then the allocated resources are freed.**]**
