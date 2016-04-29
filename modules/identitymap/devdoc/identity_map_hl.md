# Identity Map Module HL Requirements

## Overview
This document describes the identity map high level module.  This module adapts
the existing lower layer identity map module for use with the Gateway HL library.  
It is mostly a passthrough to the existing module, with a specialized create 
function to interpret the serialized JSON module arguments.

The identity map module maps MAC addresses to device ids and keys.

## Reference

[module.h](../../../../devdoc/module.md)

[Identity Map Module](identity_map.md)

[vector.h](../../../../azure-c-shared-utility/c/inc/vector.h)

IEEE Std 802-2014, section 8.1 (MAC Address canonical form)

### Expected Arguments

The arguments to this module is a JSON array of the following object:
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

##Exposed API
```c
MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void);
```

## Module_GetAPIs

This is the primary public interface for the module.  It returns a pointer to 
the `MODULE_APIS` structure containing the implementation functions for this
module. The following functions are the implementation of those APIs.

**SRS_IDMAP_HL_17_001: [**`Module_GetAPIs` shall return a non-`NULL` pointer
to a MODULE_APIS structure.**]**

**SRS_IDMAP_HL_17_002: [** The `MODULE_APIS` structure shall have non-`NULL`
`Module_Create`, `Module_Destroy`, and `Module_Receive` fields. **]**

## IdentityMap_HL_Create
```C
MODULE_HANDLE IdentityMap_HL_Create(MESSAGE_BUS_HANDLE busHandle, const void* configuration);
```
This function creates the identity map HL module. This module reads a JSON 
array and converts this to a VECTOR of IDENTITY_MAP_CONFIG, as expected by
the identity map module.  Finally, it calls the identity map module create
function.

**SRS_IDMAP_HL_17_003: [** If `busHandle` is NULL then
 `IdentityMap_HL_Create` shall fail and return NULL. **]**

**SRS_IDMAP_HL_17_004: [** If `configuration` is NULL then
 `IdentityMap_HL_Create` shall fail and return NULL. **]**

**SRS_IDMAP_HL_17_005: [** If `configuration` is not a JSON array of 
JSON objects, then `IdentityMap_HL_Create` shall fail and return NULL. **]**

**SRS_IDMAP_HL_17_006: [** `IdentityMap_HL_Create` shall parse the 
`configuration` as a JSON array of objects. **]**

**SRS_IDMAP_HL_17_007: [** `IdentityMap_HL_Create` shall call 
VECTOR_create to make the identity map module input vector. **]**

**SRS_IDMAP_HL_17_019: [** If creating the vector fails, then 
`IdentityMap_HL_Create` shall fail and return NULL. **]**

**SRS_IDMAP_HL_17_008: [** `IdentityMap_HL_Create` shall walk 
through each object of the array. **]**

**SRS_IDMAP_HL_17_009: [** If the array object does not contain a value 
named "macAddress" then `IdentityMap_HL_Create` shall fail and return 
NULL. **]**

**SRS_IDMAP_HL_17_010: [** If the array object does not contain a value 
named "deviceId" then `IdentityMap_HL_Create` shall fail and return 
NULL. **]**

**SRS_IDMAP_HL_17_011: [** If the array object does not contain a value 
named "deviceKey" then `IdentityMap_HL_Create` shall fail and return 
NULL. **]**

**SRS_IDMAP_HL_17_012: [** `IdentityMap_HL_Create` shall use 
"macAddress", "deviceId", and "deviceKey" values as the fields for an 
IDENTITY_MAP_CONFIG structure and call VECTOR_push_back to add this element 
to the vector. **]**

**SRS_IDMAP_HL_17_020: [** If pushing into the vector is not successful, 
then `IdentityMap_HL_Create` shall fail and return NULL. **]** 

**SRS_IDMAP_HL_17_013: [** `IdentityMap_HL_Create` shall invoke 
identity map module's create, passing in the message bus handle and the input vector. 
**]**

**SRS_IDMAP_HL_17_014: [** When the lower layer identity map module 
create succeeds, `IdentityMap_HL_Create` shall succeed and return a 
non-NULL value. **]**

**SRS_IDMAP_HL_17_015: [** If the lower layer identity map module create 
fails, `IdentityMap_HL_Create` shall fail and return NULL. **]**

**SRS_IDMAP_HL_17_016: [** `IdentityMap_HL_Create` shall release 
all data it allocated. **]**


## IdentityMap_HL_Destroy
```C
static void IdentityMap_HL_Destroy(MODULE_HANDLE moduleHandle);
```

**SRS_IDMAP_HL_17_017: [** `IdentityMap_HL_Destroy` shall free all 
used resources. **]**


## IdentityMap_HL_Receive
```C
static void IdentityMap_HL_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle);
```

**SRS_IDMAP_HL_17_018: [** `IdentityMap_HL_Receive` shall pass the 
received parameters to the underlying  identity map module receive function. **]**
