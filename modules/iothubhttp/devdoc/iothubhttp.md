IOTHUB HTTP MODULE
===========

High level design
-----------------

### Overview

#### Publishing events
This module ensured IoTHub connectivity for all the mapped devices on the bus. The module identifies the messages that it needs to process
by the following properties that must exist:

>| PropertyName | Description                                                                  |
>|--------------|------------------------------------------------------------------------------|
>| source       | The module only processes message that have source set to "mapping".         |
>| deviceName   | The deviceName as registered with IoTHub                                     |
>| deviceKey    | The key as registered with IoTHub                                            |

The module shall dynamically create instances of IoTHubClient (one per each device). The module shall use HTTP 
protocol for connections.

#### Receiving IoT Hub messages
Upon reception of a message from the IoTHub, this module will publish a message to the bus.  The published message will have the following properties:
>| PropertyName  | Description                                                                         |
>| ------------- | ----------------------------------------------------------------------------------- |
>| source        | "IoTHubHTTP"                                                                    |
>| deviceName    | The receiver's deviceName, as registered with IoTHub                                |
>| * (all other) | All other properties of the received message will be added to the published message |

The body of the published message will be the body of the received message.


####Additional data types
```C
typedef struct IOTHUBHTTP_CONFIG_TAG
{
	const char* IoTHubName; /*the name of the IoTHub*/
	const char* IoTHubSuffix; /*the suffix used in generating the host name*/
}IOTHUBHTTP_CONFIG; /*this needs to be passed to the Module_Create function*/
```

###IoTHubHttp_Create
```C
MODULE_HANDLE IoTHubHttp_Create(MESSAGE_BUS_HANDLE busHandle, const void* configuration);
```
Creates a new IoTHubHttp instance. configuration is a pointer to a `IOTHUBHTTP_CONFIG`.

**SRS_IOTHUBHTTP_02_001: [**If `busHandle` is NULL then `IoTHubHttp_Create` shall fail and return NULL.**]**
**SRS_IOTHUBHTTP_02_002: [**If `configuration` is NULL then `IoTHubHttp_Create` shall fail and return NULL.**]**
**SRS_IOTHUBHTTP_02_003: [**If `configuration->IoTHubName` is NULL then `IoTHubHttp_Create` shall and return NULL.**]**
**SRS_IOTHUBHTTP_02_004: [**If `configuration->IoTHubSuffix` is NULL then `IoTHubHttp_Create` shall fail and return NULL.**]**

IoTHubHttp shall maintain a shared transport for all devices.

**SRS_IOTHUBHTTP_17_001: [** `IoTHubHttp_Create` shall create a shared transport by calling `IoTHubTransport_Create`. **]**

**SRS_IOTHUBHTTP_17_002: [** If creating the shared transport fails, `IoTHubHttp_Create` shall fail and return `NULL`. **]**

IoTHubHttp shall name the triplet of deviceName, deviceKey and IOTHUB_CLIENT_HANDLE `PERSONALITY`.  

**SRS_IOTHUBHTTP_02_006: [**`IoTHubHttp_Create` shall create an empty `VECTOR` containing pointers to `PERSONALITY`s.**]** 

**SRS_IOTHUBHTTP_02_007: [**If creating the `VECTOR` fails
then `IoTHubHttp_Create` shall fail and return NULL.**]**
**SRS_IOTHUBHTTP_02_028: [**`IoTHubHttp_Create` shall create a copy of `configuration->IoTHubName`.**]**
**SRS_IOTHUBHTTP_02_029: [**`IoTHubHttp_Create` shall create a copy of `configuration->IoTHubSuffix`.**]**
**SRS_IOTHUBHTTP_17_004: [** `IoTHubHttp_Create` shall store the busHandle. **]**
**SRS_IOTHUBHTTP_02_027: [**When `IoTHubHttp_Create` encounters an internal failure it shall fail and return NULL.**]**

**SRS_IOTHUBHTTP_02_008: [**Otherwise, `IoTHubHttp_Create` shall return a non-NULL handle.**]**

###IoTHubHttp_Receive
```C
MODULE_HANDLE IoTHubHttp_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle);
```
**SRS_IOTHUBHTTP_02_009: [**If `moduleHandle` or `messageHandle` is NULL then `IoTHubHttp_Receive` shall do nothing.**]**
**SRS_IOTHUBHTTP_02_010: [**If message properties do not contain a property called "source" having the value set to "mapping" then `IoTHubHttp_Receive` shall do nothing.**]**
**SRS_IOTHUBHTTP_02_011: [**If message properties do not contain a property called "deviceName" having a non-NULL value then `IoTHubHttp_Receive` shall do nothing.**]**
**SRS_IOTHUBHTTP_02_012: [**If message properties do not contain a property called "deviceKey" having a non-NULL value then `IoTHubHttp_Receive` shall do nothing.**]**

**SRS_IOTHUBHTTP_02_013: [**If the deviceName does not exist in the `PERSONALITY` collection then `IoTHubHttp_Receive` shall create a new 
`PERSONALITY` containing `deviceName`, `deviceKey` and an `IOTHUB_CLIENT_HANDLE` (by a call to `IoTHubClient_CreateWithTransport`).**]**
**SRS_IOTHUBHTTP_17_003: [** If a new `PERSONALITY` is created, then the IoTHubClient will be set to receive messages, by calling `IoTHubClient_SetMessageCallback` with callback function `IoTHubHttp_ReceiveMessageCallback` and `PERSONALITY` as context.**]**   
**SRS_IOTHUBHTTP_02_014: [**If creating the `PERSONALITY` fails then `IoTHubHttp_Receive` shall return.**]** 
**SRS_IOTHUBHTTP_02_016: [**If adding the new triplet fails, then `IoTHubClient_Create` shall return.**]** 
**SRS_IOTHUBHTTP_02_017: [**If the deviceName exists in the `PERSONALITY` collection then `IoTHubHttp_Receive` shall not 
create a new IOTHUB_CLIENT_HANDLE.**]**

**SRS_IOTHUBHTTP_02_018: [**`IoTHubHttp_Receive` shall create a new IOTHUB_MESSAGE_HANDLE having the same content as the `messageHandle` and
same properties with the exception of deviceName and deviceKey properties.**]**

**SRS_IOTHUBHTTP_02_019: [**If creating the IOTHUB_MESSAGE_HANDLE fails, then `IoTHubHttp_Receive` shall return.**]**

**SRS_IOTHUBHTTP_02_020: [**`IoTHubHttp_Receive` shall call IoTHubClient_SendEventAsync passing the IOTHUB_MESSAGE_HANDLE.**]**
**SRS_IOTHUBHTTP_02_021: [**If `IoTHubClient_SendEventAsync` fails then `IoTHubHttp_Receive` shall return.**]**

**SRS_IOTHUBHTTP_02_022: [**`IoTHubHttp_Receive` shall return.**]**


### IoTHubHttp_ReceiveMessageCallback
```c
IOTHUBMESSAGE_DISPOSITION_RESULT IoTHubHttp_ReceiveMessageCallback(IOTHUB_MESSAGE_HANDLE msg, void* userContextCallback)
```

**SRS_IOTHUBHTTP_17_005: [** If `userContextCallback` is `NULL`, then `IoTHubHttp_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ABANDONED`. **]**

**SRS_IOTHUBHTTP_17_006: [** If Message Content type is `IOTHUBMESSAGE_UNKNOWN`, then `IoTHubHttp_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ABANDONED`. **]**

**SRS_IOTHUBHTTP_17_007: [** `IoTHubHttp_ReceiveMessageCallback` shall get properties from message by calling `IoTHubMessage_Properties`. **]**

**SRS_IOTHUBHTTP_17_008: [** If message properties are `NULL`, `IoTHubHttp_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ABANDONED`. **]**

**SRS_IOTHUBHTTP_17_009: [** `IoTHubHttp_ReceiveMessageCallback` shall define a property "source" as "IoTHubHTTP". **]**

**SRS_IOTHUBHTTP_17_010: [** `IoTHubHttp_ReceiveMessageCallback` shall define a property "deviceName" as the `PERSONALITY`'s deviceName. **]**

**SRS_IOTHUBHTTP_17_011: [** `IoTHubHttp_ReceiveMessageCallback` shall combine message properties with the "source" and "deviceName" properties. **]**

**SRS_IOTHUBHTTP_17_022: [** If message properties fail to combine, `IoTHubHttp_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ABANDONED`. **]**

**SRS_IOTHUBHTTP_17_013: [** If Message content type is `IOTHUBMESSAGE_BYTEARRAY`, `IoTHubHttp_ReceiveMessageCallback` shall get the size and buffer from the  results of `IoTHubMessage_GetByteArray`. **]**

**SRS_IOTHUBHTTP_17_023: [** If `IoTHubMessage_GetByteArray` fails, `IoTHubHttp_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ABANDONED`. **]**

**SRS_IOTHUBHTTP_17_014: [** If Message content type is `IOTHUBMESSAGE_STRING`, `IoTHubHttp_ReceiveMessageCallback` shall get the buffer from results of `IoTHubMessage_GetString`. **]**

**SRS_IOTHUBHTTP_17_015: [** If Message content type is `IOTHUBMESSAGE_STRING`, `IoTHubHttp_ReceiveMessageCallback` shall get the buffer size from the string length. **]**

**SRS_IOTHUBHTTP_17_016: [** `IoTHubHttp_ReceiveMessageCallback` shall create a new message from combined properties, the size and buffer. **]**

**SRS_IOTHUBHTTP_17_017: [** If the message fails to create, `IoTHubHttp_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_REJECTED`. **]**

**SRS_IOTHUBHTTP_17_018: [** `IoTHubHttp_ReceiveMessageCallback` shall call `Bus_Publish` with the new message and the `busHandle`. **]**

**SRS_IOTHUBHTTP_17_019: [** If the message fails to publish, `IoTHubHttp_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_REJECTED`. **]**

**SRS_IOTHUBHTTP_17_020: [** `IoTHubHttp_ReceiveMessageCallback` shall destroy all resources it creates. **]**

**SRS_IOTHUBHTTP_17_021: [** Upon success, `IoTHubHttp_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ACCEPTED`. **]**


###IoTHubHttp_Destroy
```C
void IoTHubHttp_Destroy(MODULE_HANDLE moduleHandle);
```
**SRS_IOTHUBHTTP_02_023: [**If `moduleHandle` is NULL then `IoTHubHttp_Destroy` shall return.**]**
**SRS_IOTHUBHTTP_02_024: [**Otherwise `IoTHubHttp_Destroy` shall free all used resources.**]**

###Module_GetAPIs
```C
extern const MODULE_APIS* Module_GetAPIS(void);
```

**SRS_IOTHUBHTTP_02_025: [**`Module_GetAPIS` shall return a non-NULL pointer.**]** 
**SRS_IOTHUBHTTP_02_026: [**The MODULE_APIS structure shall have non-NULL `Module_Create`, `Module_Destroy`, and `Module_Receive` fields.**]**