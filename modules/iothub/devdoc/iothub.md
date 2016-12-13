IOTHUB MODULE
=============

High level design
-----------------

### Overview

#### Publishing events to IoT Hub
This module provides a connection to IoT Hub for all mapped devices known to the gateway. When a device wants to publish events to IoT Hub, it must
send a message with the following properties:

>| PropertyName | Description                                                                  |
>|--------------|------------------------------------------------------------------------------|
>| source       | The module only processes messages that have source set to "mapping".        |
>| deviceName   | The device ID registered with IoT Hub                                        |
>| deviceKey    | The device key registered with IoT Hub                                       |

The body of the message will be sent to IoT Hub on behalf of the given device.

The IotHub module dynamically creates per-device instances of IoTHubClient. It can be configured to use any of the protocols supported by 
IoTHubClient. Note that the AMQP and HTTP transports will share one TCP connection for all devices; the MQTT transport will create a new
TCP connection for each device.

#### Receiving messages from IoT Hub 
Upon reception of a message from IoT Hub, this module will publish a message to the broker with the following properties:

>| PropertyName  | Description                                                                         |
>| ------------- | ----------------------------------------------------------------------------------- |
>| source        | "iothub"                                                                            |
>| deviceName    | The receiver's device ID, as registered with IoT Hub                                |

The body of the received message, as well as any additional properties, will be added to the published message.

#### Module configuration
This module connects to the IoT hub described by the following data structure, using the given transport:

```C
typedef struct IOTHUB_CONFIG_TAG
{
    const char* IoTHubName;   /*the name of the IoT hub*/
    const char* IoTHubSuffix; /*the suffix used in generating the host name*/
    IOTHUB_CLIENT_TRANSPORT_PROVIDER transportProvider;
}IOTHUB_CONFIG; /*this needs to be passed to the Module_Create function*/
```

### IotHub_ParseConfigurationFromJson
```C
void* IotHub_ParseConfigurationFromJson(const char* configuration);
```
Deserializes a JSON string into an IOTHUB_CONFIG structure suitable for passing into `IotHub_Create`. `configuration` is a pointer to a null-terminated string containing the JSON object given to `Gateway_CreateFromJson`.

### Expected module arguments
```json
{
    "IoTHubName" : "<the name of the IoTHub>",
    "IoTHubSuffix" : "<the suffix used in generating the host name>",
    "Transport" : "HTTP" | "http" | "AMQP" | "amqp" | "MQTT" | "mqtt"
}
```

**SRS_IOTHUBMODULE_05_002: [** If `configuration` is NULL then `IotHub_ParseConfigurationFromJson` shall fail and return NULL. **]**
**SRS_IOTHUBMODULE_05_004: [** `IotHub_ParseConfigurationFromJson` shall parse `configuration` as a JSON string. **]**
**SRS_IOTHUBMODULE_05_005: [** If parsing of `configuration` fails, `IotHub_ParseConfigurationFromJson` shall fail and return NULL. **]**
**SRS_IOTHUBMODULE_05_006: [** If the JSON object does not contain a value named "IoTHubName" then `IotHub_ParseConfigurationFromJson` shall fail and return NULL. **]**
**SRS_IOTHUBMODULE_05_007: [** If the JSON object does not contain a value named "IoTHubSuffix" then `IotHub_ParseConfigurationFromJson` shall fail and return NULL. **]**
**SRS_IOTHUBMODULE_05_011: [** If the JSON object does not contain a value named "Transport" then `IotHub_ParseConfigurationFromJson` shall fail and return NULL. **]**
**SRS_IOTHUBMODULE_05_012: [** If the value of "Transport" is not one of "HTTP", "AMQP", or "MQTT" (case-insensitive) then `IotHub_ParseConfigurationFromJson` shall fail and return NULL. **]**

### IotHub_FreeConfiguration
```C
void IotHub_FreeConfiguration(void* configuration);
```
Deallocates the structure that was returned from `IotHub_ParseConfigurationFromJson`.

**SRS_IOTHUBMODULE_05_014: [** If `configuration` is NULL then `IotHub_FreeConfiguration` shall do nothing. **]**
**SRS_IOTHUBMODULE_05_015: [** `IotHub_FreeConfiguration` shall free the strings referenced by the `IoTHubName` and `IoTHubSuffix` data members, and then free the `IOTHUB_CONFIG` structure itself. **]**

### IotHub_Create
```C
MODULE_HANDLE IotHub_Create(BROKER_HANDLE broker, const void* configuration);
```
Creates a new IotHub module instance. `configuration` is a pointer to structure of type `IOTHUB_CONFIG`.

**SRS_IOTHUBMODULE_02_001: [** If `broker` is `NULL` then `IotHub_Create` shall fail and return `NULL`. **]**
**SRS_IOTHUBMODULE_02_002: [** If `configuration` is `NULL` then `IotHub_Create` shall fail and return `NULL`. **]**
**SRS_IOTHUBMODULE_02_003: [** If `configuration->IoTHubName` is `NULL` then `IotHub_Create` shall and return `NULL`. **]**
**SRS_IOTHUBMODULE_02_004: [** If `configuration->IoTHubSuffix` is `NULL` then `IotHub_Create` shall fail and return `NULL`. **]**
**SRS_IOTHUBMODULE_17_001: [** If `configuration->transportProvider` is `HTTP_Protocol` or `AMQP_Protocol`, `IotHub_Create` shall create a shared transport by calling `IoTHubTransport_Create`. **]**
**SRS_IOTHUBMODULE_17_002: [** If creating the shared transport fails, `IotHub_Create` shall fail and return `NULL`. **]**

Each {device ID, device key, IoTHubClient handle} triplet is referred to as a "personality".  

**SRS_IOTHUBMODULE_02_006: [** `IotHub_Create` shall create an empty `VECTOR` containing pointers to `PERSONALITY`s. **]**
**SRS_IOTHUBMODULE_02_007: [** If creating the personality vector fails then `IotHub_Create` shall fail and return `NULL`. **]**
**SRS_IOTHUBMODULE_02_028: [** `IotHub_Create` shall create a copy of `configuration->IoTHubName`. **]**
**SRS_IOTHUBMODULE_02_029: [** `IotHub_Create` shall create a copy of `configuration->IoTHubSuffix`. **]**
**SRS_IOTHUBMODULE_17_004: [** `IotHub_Create` shall store the broker. **]**
**SRS_IOTHUBMODULE_02_027: [** When `IotHub_Create` encounters an internal failure it shall fail and return `NULL`. **]**
**SRS_IOTHUBMODULE_02_008: [** Otherwise, `IotHub_Create` shall return a non-`NULL` handle. **]**

###IotHub_Receive
```C
void IoTHub_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle);
```
**SRS_IOTHUBMODULE_02_009: [** If `moduleHandle` or `messageHandle` is `NULL` then `IotHub_Receive` shall do nothing. **]**
**SRS_IOTHUBMODULE_02_010: [** If message properties do not contain a property called "source" having the value set to "mapping" then `IotHub_Receive` shall do nothing. **]**
**SRS_IOTHUBMODULE_02_011: [** If message properties do not contain a property called "deviceName" having a non-`NULL` value then `IotHub_Receive` shall do nothing. **]**
**SRS_IOTHUBMODULE_02_012: [** If message properties do not contain a property called "deviceKey" having a non-`NULL` value then `IotHub_Receive` shall do nothing. **]**

**SRS_IOTHUBMODULE_02_013: [** If no personality exists with a device ID equal to the value of the `deviceName` property of the message, then `IotHub_Receive` shall create a new `PERSONALITY` with the ID and key values from the message. **]**
**SRS_IOTHUBMODULE_02_017: [** Otherwise `IotHub_Receive` shall not create a new personality. **]**
**SRS_IOTHUBMODULE_05_013: [** If a new personality is created and the module's transport has already been created (in `IotHub_Create`), an `IOTHUB_CLIENT_HANDLE` will be added to the personality by a call to `IoTHubClient_CreateWithTransport`. **]**
**SRS_IOTHUBMODULE_05_003: [** If a new personality is created and the module's transport has not already been created, an `IOTHUB_CLIENT_HANDLE` will be added to the personality by a call to `IoTHubClient_Create` with the corresponding transport provider. **]**
**SRS_IOTHUBMODULE_17_003: [** If a new personality is created, then the associated IoTHubClient will be set to receive messages by calling `IoTHubClient_SetMessageCallback` with callback function `IotHub_ReceiveMessageCallback`, and the personality as context. **]**
**SRS_IOTHUBMODULE_02_014: [** If creating the personality fails then `IotHub_Receive` shall return. **]**
**SRS_IOTHUBMODULE_02_016: [** If adding a new personality to the vector fails, then `IoTHub_Receive` shall return. **]**
**SRS_IOTHUBMODULE_02_018: [** `IotHub_Receive` shall create a new IOTHUB_MESSAGE_HANDLE having the same content as `messageHandle`, and the same properties with the exception of `deviceName` and `deviceKey`. **]**
**SRS_IOTHUBMODULE_02_019: [** If creating the IOTHUB_MESSAGE_HANDLE fails, then `IotHub_Receive` shall return. **]**
**SRS_IOTHUBMODULE_02_020: [** `IotHub_Receive` shall call IoTHubClient_SendEventAsync passing the IOTHUB_MESSAGE_HANDLE. **]**
**SRS_IOTHUBMODULE_02_021: [** If `IoTHubClient_SendEventAsync` fails then `IotHub_Receive` shall return. **]**
**SRS_IOTHUBMODULE_02_022: [** If `IoTHubClient_SendEventAsync` succeeds then `IotHub_Receive` shall return. **]**


### IotHub_ReceiveMessageCallback
```C
IOTHUBMESSAGE_DISPOSITION_RESULT IotHub_ReceiveMessageCallback(IOTHUB_MESSAGE_HANDLE msg, void* userContextCallback)
```

**SRS_IOTHUBMODULE_17_005: [** If `userContextCallback` is `NULL`, then `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ABANDONED`. **]**
**SRS_IOTHUBMODULE_17_006: [** If Message Content type is `IOTHUBMESSAGE_UNKNOWN`, then `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ABANDONED`. **]**
**SRS_IOTHUBMODULE_17_007: [** `IotHub_ReceiveMessageCallback` shall get properties from message by calling `IoTHubMessage_Properties`. **]**
**SRS_IOTHUBMODULE_17_008: [** If message properties are `NULL`, `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ABANDONED`. **]**
**SRS_IOTHUBMODULE_17_009: [** `IotHub_ReceiveMessageCallback` shall define a property "source" as "iothub". **]**
**SRS_IOTHUBMODULE_17_010: [** `IotHub_ReceiveMessageCallback` shall define a property "deviceName" as the `PERSONALITY`'s deviceName. **]**
**SRS_IOTHUBMODULE_17_011: [** `IotHub_ReceiveMessageCallback` shall combine message properties with the "source" and "deviceName" properties. **]**
**SRS_IOTHUBMODULE_17_022: [** If message properties fail to combine, `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ABANDONED`. **]**
**SRS_IOTHUBMODULE_17_013: [** If Message content type is `IOTHUBMESSAGE_BYTEARRAY`, `IotHub_ReceiveMessageCallback` shall get the size and buffer from the  results of `IoTHubMessage_GetByteArray`. **]**
**SRS_IOTHUBMODULE_17_023: [** If `IoTHubMessage_GetByteArray` fails, `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ABANDONED`. **]**
**SRS_IOTHUBMODULE_17_014: [** If Message content type is `IOTHUBMESSAGE_STRING`, `IotHub_ReceiveMessageCallback` shall get the buffer from results of `IoTHubMessage_GetString`. **]**
**SRS_IOTHUBMODULE_17_015: [** If Message content type is `IOTHUBMESSAGE_STRING`, `IotHub_ReceiveMessageCallback` shall get the buffer size from the string length. **]**
**SRS_IOTHUBMODULE_17_016: [** `IotHub_ReceiveMessageCallback` shall create a new message from combined properties, the size and buffer. **]**
**SRS_IOTHUBMODULE_17_017: [** If the message fails to create, `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_REJECTED`. **]**
**SRS_IOTHUBMODULE_17_018: [** `IotHub_ReceiveMessageCallback` shall call `Broker_Publish` with the new message, this module's handle, and the `broker`. **]**
**SRS_IOTHUBMODULE_17_019: [** If the message fails to publish, `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_REJECTED`. **]**
**SRS_IOTHUBMODULE_17_020: [** `IotHub_ReceiveMessageCallback` shall destroy all resources it creates. **]**
**SRS_IOTHUBMODULE_17_021: [** Upon success, `IotHub_ReceiveMessageCallback` shall return `IOTHUBMESSAGE_ACCEPTED`. **]**

### IotHub_Destroy
```C
void IotHub_Destroy(MODULE_HANDLE moduleHandle);
```
**SRS_IOTHUBMODULE_02_023: [** If `moduleHandle` is `NULL` then `IotHub_Destroy` shall return. **]**
**SRS_IOTHUBMODULE_02_024: [** Otherwise `IotHub_Destroy` shall free all used resources. **]**

### Module_GetApi
```C
MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version)
```

**SRS_IOTHUBMODULE_26_001: [** `Module_GetApi` shall return a pointer to a `MODULE_API` structure with the required function pointers. **]**
