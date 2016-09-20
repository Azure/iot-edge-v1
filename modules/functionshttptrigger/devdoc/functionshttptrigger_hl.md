# Functiosn Http Trigger Module HL Requirements

## Overview
This document describes the functions http trigger high level module.  This module adapts
the existing lower layer functions http trigger module for use with the Gateway HL library.  
It is mostly a passthrough to the existing module, with a specialized create 
function to interpret the serialized JSON module arguments.

The functions http trigger module triggers an Azure Functions by senting an http GET to it's trigger endpoint.

## Reference

[module.h](../../../../devdoc/module.md)

[Functions Http Trigger Module](functionshttptrigger.md)

[vector.h](../../../../azure-c-shared-utility/c/inc/vector.h)

[Introduction to Azure Functions](https://azure.microsoft.com/en-us/blog/introducing-azure-functions/)

### Expected Arguments

The arguments to this module is a JSON with the following values:
```json
{
        "hostname": "<YourHosNameHere from Functions Portal>",
        "relativePath": "api/<YourFunctionName>"
}
```

##Exposed API
```c
MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void);
```

## Module_GetAPIs

This is the primary public interface for the module.  It returns a pointer to 
the `MODULE_APIS` structure containing the implementation functions for this
module. The following functions are the implementation of those APIs.

**SRS_FUNCHTTPTRIGGER_HL_04_001: [** `Module_GetAPIS` shall fill the provided `MODULE_APIS` function with the required function pointers. **]**

## Functions_Http_Trigger_HL_Create
```C
MODULE_HANDLE Functions_Http_Trigger_HL_Create(BROKER_HANDLE broker, const void* configuration);
```
This function creates the functions http trigger HL module. This module reads a JSON 
 and converts this to 2 strings, a hostAddress and a relativePath, as expected by
the functions http trigger module.  Finally, it calls the functions http trigger module create
function.

**SRS_FUNCHTTPTRIGGER_HL_04_002: [** If `broker` is NULL then
 `Functions_Http_Trigger_HL_Create` shall fail and return NULL. **]**

**SRS_FUNCHTTPTRIGGER_HL_04_003: [** If `configuration` is NULL then
 `Functions_Http_Trigger_HL_Create` shall fail and return NULL. **]**

**SRS_FUNCHTTPTRIGGER_HL_04_004: [** If `configuration` is not a JSON string, then `Functions_Http_Trigger_HL_Create` shall fail and return NULL. **]**

**SRS_FUNCHTTPTRIGGER_HL_04_005: [** `Functions_Http_Trigger_HL_Create` shall parse the 
`configuration` as a JSON array of strings. **]**

**SRS_FUNCHTTPTRIGGER_HL_04_006: [** `Functions_Http_Trigger_HL_Create` shall call 
STRING_construct to create hostAddress based on input host address. **]**

**SRS_FUNCHTTPTRIGGER_HL_04_007: [** `Functions_Http_Trigger_HL_Create` shall call 
STRING_construct to create relativePath based on input host address. **]**

**SRS_FUNCHTTPTRIGGER_HL_04_008: [** If creating the strings fails, then 
`Functions_Http_Trigger_HL_Create` shall fail and return NULL. **]**

**SRS_FUNCHTTPTRIGGER_HL_17_009: [** If the array object does not contain a value 
named "hostAddress" then `Functions_Http_Trigger_HL_Create` shall fail and return 
NULL. **]**

**SRS_FUNCHTTPTRIGGER_HL_04_010: [** If the array object does not contain a value 
named "relativePath" then `Functions_Http_Trigger_HL_Create` shall fail and return 
NULL. **]**

**SRS_FUNCHTTPTRIGGER_HL_04_011: [** `Functions_Http_Trigger_HL_Create` shall invoke 
functions http trigger module's create, passing in the message broker handle and the `FUNCTIONS_HTTP_TRIGGER_CONFIG`. 
**]**

**SRS_FUNCHTTPTRIGGER_HL_04_012: [** When the lower layer functions http trigger module 
create succeeds, `Functions_Http_Trigger_HL_Create` shall succeed and return a 
non-NULL value. **]**

**SRS_FUNCHTTPTRIGGER_HL_04_013: [** If the lower layer functions http trigger module create 
fails, `Functions_Http_Trigger_HL_Create` shall fail and return NULL. **]**

**SRS_FUNCHTTPTRIGGER_HL_04_014: [** `Functions_Http_Trigger_HL_Create` shall release 
all data it allocated. **]**


## Functions_Http_Trigger_HL_Destroy
```C
static void Functions_Http_Trigger_HL_Destroy(MODULE_HANDLE moduleHandle);
```

**SRS_FUNCHTTPTRIGGER_HL_04_015: [** `Functions_Http_Trigger_HL_Destroy` shall free all 
used resources. **]**


## Functions_Http_Trigger_HL_Receive
```C
static void Functions_Http_Trigger_HL_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle);
```

**SRS_FUNCHTTPTRIGGER_HL_04_016: [** `Functions_Http_Trigger_HL_Receive` shall pass the 
received parameters to the underlying  identity map module receive function. **]**
