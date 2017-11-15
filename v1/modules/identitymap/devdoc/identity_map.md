# Identity Map Module Requirements

##Overview
This document describes the identity map module.  This module maps MAC addresses 
to device id and keys, and device ids to MAC Addresses. This module is 
not multi-threaded, all work will be completed in the Receive callback.
 
#### MAC Address to device name (Device to Cloud)
The module identifies the messages that it needs to process by the following 
properties that must exist:

>| PropertyName | Description                                                                  |
>|--------------|------------------------------------------------------------------------------|
>| macAddress   | The MAC address of a sensor, in canonical form                               |

*and* if any of the following *do not exist*.

>| PropertyName | Description                                                                    |
>|--------------|--------------------------------------------------------------------------------|
>| deviceName   | The deviceName as registered with IoTHub                                       |
>| deviceKey    | The key as registered with IoTHub                                              |
>| source       | Set to "mapping" - this shall ensure the mapping module rejects its own messages |

When this module publishes a message, each message will have the the following properties:
>| PropertyName | Description                                                                  |
>|--------------|------------------------------------------------------------------------------|
>| source       | Source will be set to "mapping".                                             |
>| deviceName   | The deviceName as registered with IoTHub                                     |
>| deviceKey    | The key as registered with IoTHub                                            |

Then the module will remove the following properties:

>| PropertyName | Description                                                                  |
>|--------------|------------------------------------------------------------------------------|
>| macAddress   | The MAC address of a sensor, in canonical form                               |


#### Device Id to MAC Address (Cloud to Device)

The module identifies the messages that it needs to process by the following properties that must exist:
>| PropertyName | Description                                                                  |
>|--------------|------------------------------------------------------------------------------|
>| deviceName   | The deviceName as registered with IoTHub                                     |
>| source       | Set to "iothub"                                                          |

When this module publishes a message, each message will have the following proporties:
>| PropertyName | Description                                                                  |
>|--------------|------------------------------------------------------------------------------|
>| source       | Source will be set to "mapping".                                             |
>| macAddress   | The MAC address of a sensor, in canonical form                               |
      
Then the module will remove the following properties:
>| PropertyName | Description                                                                  |
>|--------------|------------------------------------------------------------------------------|
>| deviceName   | The deviceName as registered with IoTHub                                     |
>| deviceKey    | The key as registered with IoTHub                                            |

The MAC addresses are expected in canonical form. For the purposes of this module, 
the canonical form is "XX:XX:XX:XX:XX:XX", a sequence of six two-digit hexadecimal 
numbers separated by colons, ie "AC:DE:48:12:7B:80".  The MAC address is 
case-insenstive.

##References

[module.h](../../../core/devdoc/module.md)

[vector.h](../../../deps/c-utility/inc/azure_c_shared_utility/vector.h)

IEEE Std 802-2014, section 8.1 (MAC Address canonical form)

##Exposed API
```c

typedef struct IDENTITY_MAP_CONFIG_TAG
{
    const char* macAddress;
    const char* deviceId;
    const char* deviceKey;
} IDENTITY_MAP_CONFIG;

MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version);

```

## Module_GetApi

This is the primary public interface for the module.  It returns a pointer to 
the `MODULE_API` structure containing the implementation functions for this module.  
The following functions are the implementation of those APIs.

**SRS_IDMAP_26_001: [** `Module_GetApi` shall return a pointer to a `MODULE_API` structure. **]**


## IdentityMap_ParseConfigurationFromJson
```C
static void * IdentityMap_ParseConfigurationFromJson(const char* configuration)
;
```
This function parses the JSON configuration for the identity map module. `configuration` is a JSON array
of the following object:
```json
{
    "macAddress" : "<mac address in canonical form>",
    "deviceId"   : "<device name as registered with IoTHub>",
    "deviceKey"  : "<key as registered with IoTHub>"
}
```
### Example Arguments
```json
[
    {
        "macAddress" : "01:01:01:01:01:01",
        "deviceId"   : "sample-device1",
        "deviceKey"  : "<key as registered with IoTHub>"
    },
    {
        "macAddress" : "02:02:02:02:02:02",
        "deviceId"   : "sample-device2",
        "deviceKey"  : "<key as registered with IoTHub>"
    }
]
```

**SRS_IDMAP_05_004: [** If `configuration` is NULL then
 `IdentityMap_ParseConfigurationFromJson` shall fail and return NULL. **]**

**SRS_IDMAP_05_005: [** If `configuration` is not a JSON array of 
JSON objects, then `IdentityMap_ParseConfigurationFromJson` shall fail and return NULL. **]**

**SRS_IDMAP_05_006: [** `IdentityMap_ParseConfigurationFromJson` shall parse the 
`configuration` as a JSON array of objects. **]**

**SRS_IDMAP_05_007: [** `IdentityMap_ParseConfigurationFromJson` shall call 
VECTOR_create to make the identity map module input vector. **]**

**SRS_IDMAP_05_019: [** If creating the vector fails, then 
`IdentityMap_ParseConfigurationFromJson` shall fail and return NULL. **]**

**SRS_IDMAP_05_008: [** `IdentityMap_ParseConfigurationFromJson` shall walk 
through each object of the array. **]**

**SRS_IDMAP_05_009: [** If the array object does not contain a value 
named "macAddress" then `IdentityMap_ParseConfigurationFromJson` shall fail and return 
NULL. **]**

**SRS_IDMAP_05_010: [** If the array object does not contain a value 
named "deviceId" then `IdentityMap_ParseConfigurationFromJson` shall fail and return 
NULL. **]**

**SRS_IDMAP_05_011: [** If the array object does not contain a value 
named "deviceKey" then `IdentityMap_ParseConfigurationFromJson` shall fail and return 
NULL. **]**

**SRS_IDMAP_05_012: [** `IdentityMap_ParseConfigurationFromJson` shall use 
"macAddress", "deviceId", and "deviceKey" values as the fields for an 
IDENTITY_MAP_CONFIG structure and call VECTOR_push_back to add this element 
to the vector. **]**

**SRS_IDMAP_05_020: [** If pushing into the vector is not successful, 
then `IdentityMap_ParseConfigurationFromJson` shall fail and return NULL. **]** 

**SRS_IDMAP_17_060: [** `IdentityMap_ParseConfigurationFromJson` shall allocate memory for the configuration vector. **]**

**SRS_IDMAP_17_061: [** If allocation fails, `IdentityMap_ParseConfigurationFromJson` shall fail and return NULL. **]**

**SRS_IDMAP_17_062: [** `IdentityMap_ParseConfigurationFromJson` shall return the pointer to the configuration vector on success. **]**

## IdentityMap_FreeConfiguration
```c
static void IdentityMap_FreeConfiguration(void * configuration);
```

This function releases any allocated resources creates by `IdentityMap_ParseConfigurationFromJson`.

**SRS_IDMAP_17_059: [** `IdentityMap_FreeConfiguration` shall do nothing if `configuration` is NULL. **]**

**SRS_IDMAP_05_016: [** `IdentityMap_FreeConfiguration` shall release 
all data `IdentityMap_ParseConfigurationFromJson` allocated. **]**


## IdentityMap_Create
```C
MODULE_HANDLE IdentityMap_Create(BROKER_HANDLE broker, const void* configuration);
```

This function creates the identity map module.  This module expects a `VECTOR_HANDLE` 
of `IDENTITY_MAP_CONFIG`, which contains a triplet of canonical form MAC 
address, device ID and device key. The MAC address will be treated as the key for the MAC address to device array, and the deviceName will be treated as the key for the device to MAC address array.

**SRS_IDMAP_17_003: [**Upon success, this function shall return a valid pointer to a `MODULE_HANDLE`.**]**
**SRS_IDMAP_17_004: [**If the `broker` is `NULL`, this function shall fail and return `NULL`.**]**
**SRS_IDMAP_17_005: [**If the configuration is `NULL`, this function shall fail and return `NULL`.**]**
**SRS_IDMAP_17_041: [**If the configuration has no vector elements, this function shall fail and return `NULL`.**]**
**SRS_IDMAP_17_019: [**If any `macAddress`, `deviceId` or `deviceKey` are `NULL`, this function shall fail and return `NULL`.**]**
**SRS_IDMAP_17_006: [**If any `macAddress` string in configuration is **not** a MAC address in canonical form, this function shall fail and return `NULL`.**]**

Note that this module does not confirm the device ID and key are valid to IoT Hub.

The valid module handle will be a pointer to the structure:

```C
typedef struct IDENTITY_MAP_DATA_TAG
{
    BROKER_HANDLE broker;
    size_t mappingSize;
    IDENTITY_MAP_CONFIG * macToDeviceArray;
    IDENTITY_MAP_CONFIG * deviceToMacArray;    
} IDENTITY_MAP_DATA;
```    

Where `broker` is the message broker passed in as input, `mappingSize` is the number of 
elements in the vector `configuration`, and the `macToDeviceArray` and `deviceToMacArray` are the sorted lists 
of mapping triplets. 

**SRS_IDMAP_17_010: [**If `IdentityMap_Create` fails to allocate a new `IDENTITY_MAP_DATA` structure, then this function shall fail, and return `NULL`.**]**
**SRS_IDMAP_17_011: [**If `IdentityMap_Create` fails to create memory for the macToDeviceArray, then this function shall fail and return `NULL`.**]**
**SRS_IDMAP_17_042: [** If `IdentityMap_Create` fails to create memory for the deviceToMacArray, then this function shall fail and return `NULL`. **]**   
**SRS_IDMAP_17_012: [**If `IdentityMap_Create` fails to add a MAC address triplet to the macToDeviceArray, then this function shall fail, release all resources, and return `NULL`.**]**
**SRS_IDMAP_17_043: [** If `IdentityMap_Create` fails to add a MAC address triplet to the deviceToMacArray, then this function shall fail, release all resources, and return `NULL`. **]**


##Module_Destroy
```C
static void IdentityMap_Destroy(MODULE_HANDLE moduleHandle);
```

This function released all resources owned by the module specified by the `moduleHandle`.

**SRS_IDMAP_17_018: [**If `moduleHandle` is `NULL`, `IdentityMap_Destroy` shall return.**]**
**SRS_IDMAP_17_015: [**`IdentityMap_Destroy` shall release all resources allocated for the module.**]**



##IdentityMap_Receive
```C
static void IdentityMap_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle);
```

This function will be the main work of this module. The processing of each 
message in pseudocode is as follows:

```
01: If message properties contain a "macAddress" key and does not contain "source"=="mapping", or both "deviceName" and "deviceKey" keys,
02:     Get MAC address from message properties via the "macAddress" key
03:     Search macToDeviceArray for MAC address
04:     If found, there is a new message to publish
05:         Get deviceId and deviceKey from macToDeviceArray.
06:         Create a new MAP from message properties.
07:         Add or replace "deviceName" with deviceId
08:         Add or replace "deviceKey" with deviceKey
09:         Add or replace "source".
10:         Delete "macAddress"
11: Else if message properties contain a "deviceName" key and does not contain "source"=="mapping" key,
12:     Get deviceId from messages properties via the "deviceName" key
13:     Search deviceToMacArray for deviceId
14:     If found, there is a new message to publish
15:         Get MAC address from deviceToMacArray
16:         Create a new MAP from message properties.
17:         Add or replace "macAddress" with MAC address.
18:         Replace "source".
19:         Delete "deviceName"
20:         Delete "deviceKey" if it exists.
21: If there is a new message to publish,
22:         Clone original mesage content.
23:         Create a new message from MAP and original message content.
24:         Publish new message on broker
25:         Destroy all resources created
```

**SRS_IDMAP_17_020: [**If `moduleHandle` or `messageHandle` is `NULL`, then the function shall return.**]**
#### MAC Address to device name (D2C)
**SRS_IDMAP_17_021: [**If `messageHandle` properties does not contain "macAddress" property, then the message shall not be marked as a D2C message.**]**   
**SRS_IDMAP_17_024: [**If `messageHandle` properties contains properties "deviceName" **and** "deviceKey", then the message shall not be marked as a D2C message.**]**   
**SRS_IDMAP_17_044: [** If messageHandle properties contains a "source" property that is set to "mapping", the message shall not be marked as a D2C message. **]**   
**SRS_IDMAP_17_040: [**If the `macAddress` of the message is not in canonical form, the message shall not be marked as a D2C message.**]**   
**SRS_IDMAP_17_025: [**If the `macAddress` of the message is not found in the `macToDeviceArray` list, the message shall not be marked as a D2C message.**]**   
On a message which passes all checks, the message shall be marked as a D2C message.

**SRS_IDMAP_17_026: [**On a D2C message received, `IdentityMap_Receive` shall call `ConstMap_CloneWriteable` on the message properties.**]**   
**SRS_IDMAP_17_027: [**If `ConstMap_CloneWriteable` fails, `IdentityMap_Receive` shall deallocate any resources and return.**]**   
Upon recognition of a D2C message, the following transformations will be done to create a message to send:
**SRS_IDMAP_17_028: [**`IdentityMap_Receive` shall call `Map_AddOrUpdate` with key of "deviceName" and value of found `deviceId`.**]**   
**SRS_IDMAP_17_029: [**If adding `deviceName` fails,`IdentityMap_Receive` shall deallocate all resources and return.**]**   
**SRS_IDMAP_17_030: [**`IdentityMap_Receive` shall call `Map_AddOrUpdate` with key of "deviceKey" and value of found `deviceKey`.**]**   
**SRS_IDMAP_17_031: [**If adding `deviceKey` fails, `IdentityMap_Receive` shall deallocate all resources and return.**]**   
**SRS_IDMAP_17_053: [** `IdentityMap_Receive` shall call `Map_Delete` to remove the "macAddress" property. **]**   
**SRS_IDMAP_17_054: [** If deleting the MAC Address fails, `IdentityMap_Receive` shall deallocate all resources and return. **]**   

#### Device Id to MAC Address (C2D)
**SRS_IDMAP_17_045: [** If `messageHandle` properties does not contain "deviceName" property, then the message shall not be marked as a C2D message. **]**    
**SRS_IDMAP_17_046: [** If messageHandle properties does not contain a "source" property, then the message shall not be marked as a C2D message. **]**   
**SRS_IDMAP_17_047: [** If messageHandle property "source" is not equal to "iothub", then the message shall not be marked as a C2D message. **]**   
**SRS_IDMAP_17_048: [** If the `deviceName` of the message is not found in deviceToMacArray, then the message shall not be marked as a C2D message. **]**   
On a message which passes all these checks, the message will be marked as a C2D message.

**SRS_IDMAP_17_049: [** On a C2D message received, `IdentityMap_Receive` shall call `ConstMap_CloneWriteable` on the message properties. **]**   
**SRS_IDMAP_17_050: [** If `ConstMap_CloneWriteable` fails, `IdentityMap_Receive` shall deallocate any resources and return. **]**   
Upon recognition of a C2D message, the following transformations will be done to create a message to send:

**SRS_IDMAP_17_051: [** `IdentityMap_Receive` shall call `Map_AddOrUpdate` with key of "macAddress" and value of found `macAddress`. **]**   
**SRS_IDMAP_17_052: [** If adding `macAddress` fails, `IdentityMap_Receive` shall deallocate all resources and return. **]**   
**SRS_IDMAP_17_055: [** `IdentityMap_Receive` shall call `Map_Delete` to remove the "deviceName" property. **]**   
**SRS_IDMAP_17_056: [** If deleting the device name fails, `IdentityMap_Receive` shall deallocate all resources and return. **]**   
**SRS_IDMAP_17_057: [** `IdentityMap_Receive` shall call `Map_Delete` to remove the "deviceKey" property. **]**      
**SRS_IDMAP_17_058: [** If deleting the device key does not return `MAP_OK` or `MAP_KEYNOTFOUND`, `IdentityMap_Receive` shall deallocate all resources and return. **]**    
NOTE: The device key is not required to be present, so `MAP_KEYNOTFOUND` is not considered a failure.   

#### Message to send exists
Upon recognition of a C2D or D2C message, then a new message shall be published.

**SRS_IDMAP_17_032: [**`IdentityMap_Receive` shall call `Map_AddOrUpdate` with key of "source" and value of "mapping".**]**   
**SRS_IDMAP_17_033: [**If adding source fails, `IdentityMap_Receive` shall deallocate all resources and return.**]**   
**SRS_IDMAP_17_034: [**`IdentityMap_Receive` shall clone message content.**]**
**SRS_IDMAP_17_035: [**If cloning message content fails, `IdentityMap_Receive` shall deallocate all resources and return.**]**   
**SRS_IDMAP_17_036: [**`IdentityMap_Receive` shall create a new message by calling `Message_CreateFromBuffer` with new map and cloned content.**]**   
**SRS_IDMAP_17_037: [**If creating new message fails, `IdentityMap_Receive` shall deallocate all resources and return.**]**   
**SRS_IDMAP_17_038: [**`IdentityMap_Receive` shall call `Broker_Publish` with `broker` and new message.**]**   
**SRS_IDMAP_17_039: [**`IdentityMap_Receive` will destroy all resources it created.**]**   
