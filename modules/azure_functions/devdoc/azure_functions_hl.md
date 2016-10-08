# Azure Functions Module HL Requirements

## Overview
This document describes the Azure Functions high level module.  This module adapts
the existing lower layer Azure Functions module for use with the Gateway HL library.  
It is mostly a pass through to the existing module, with a specialized create 
function to interpret the serialized JSON module arguments.

The Azure Functions module triggers an Azure Function by sending an http GET to it's trigger endpoint.

## Reference

[module.h](../../../../devdoc/module.md)

[Azure Functions Module](azure_functions.md)

[httpapiex.h](../../../../azure-c-shared-utility/c/inc/httpapiex.h)

[Introduction to Azure Functions](https://azure.microsoft.com/en-us/blog/introducing-azure-functions/)

### Expected Arguments

The arguments to this module is a JSON with the following values:
```json
{
        "hostname": "<YourHosNameHere from Functions Portal>", <== Required
        "relativePath": "api/<YourFunctionName>", <== Required
        "key": "<your security code here>" <== Optional if Authentication requires it. Anonymous Authentication doesn't require key/code.
}
```

## Exposed API
```c
MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void);
```

## Module_GetAPIs

This is the primary public interface for the module.  It returns a pointer to 
the `MODULE_APIS` structure containing the implementation functions for this
module. The following functions are the implementation of those APIs.

**SRS_AZUREFUNCTIONS_HL_04_001: [** `Module_GetAPIS` shall fill the provided `MODULE_APIS` function with the required function pointers. **]**

## Azure_Functions_HL_Create
```C
MODULE_HANDLE Azure_Functions_HL_Create(BROKER_HANDLE broker, const void* configuration);
```
This function creates the Azure Functions HL module. This module reads a JSON 
 and converts this to 2 strings, a hostAddress and a relativePath, as expected by
the Azure Functions module.  Finally, it calls the Azure Functions module create
function.

**SRS_AZUREFUNCTIONS_HL_04_002: [** If `broker` is NULL then
 `Azure_Functions_HL_Create` shall fail and return NULL. **]**

**SRS_AZUREFUNCTIONS_HL_04_003: [** If `configuration` is NULL then
 `Azure_Functions_HL_Create` shall fail and return NULL. **]**

**SRS_AZUREFUNCTIONS_HL_04_004: [** If `configuration` is not a JSON string, then `Azure_Functions_HL_Create` shall fail and return NULL. **]**

**SRS_AZUREFUNCTIONS_HL_04_005: [** `Azure_Functions_HL_Create` shall parse the 
`configuration` as a JSON array of strings. **]**

**SRS_AZUREFUNCTIONS_HL_04_006: [** If the array object does not contain a value 
named "hostAddress" then `Azure_Functions_HL_Create` shall fail and return 
NULL. **]**

**SRS_AZUREFUNCTIONS_HL_04_007: [** If the array object does not contain a value 
named "relativePath" then `Azure_Functions_HL_Create` shall fail and return 
NULL. **]**

**SRS_AZUREFUNCTIONS_HL_04_019: [** If the array object contains a value named "key" then `Azure_Functions_HL_Create` shall create a securityKey based on input key **]**

**SRS_AZUREFUNCTIONS_HL_04_008: [** `Azure_Functions_HL_Create` shall call 
STRING_construct to create hostAddress based on input host address. **]**

**SRS_AZUREFUNCTIONS_HL_04_009: [** `Azure_Functions_HL_Create` shall call 
STRING_construct to create relativePath based on input host address. **]**

**SRS_AZUREFUNCTIONS_HL_04_010: [** If creating the strings fails, then 
`Azure_Functions_HL_Create` shall fail and return NULL. **]**


**SRS_AZUREFUNCTIONS_HL_04_011: [** `Azure_Functions_HL_Create` shall invoke 
Azure Functions module's create, passing in the message broker handle and the `Azure_Functions_CONFIG`. 
**]**

**SRS_AZUREFUNCTIONS_HL_04_012: [** When the lower layer Azure Functions module 
create succeeds, `Azure_Functions_HL_Create` shall succeed and return a 
non-NULL value. **]**

**SRS_AZUREFUNCTIONS_HL_04_013: [** If the lower layer Azure Functions module create 
fails, `Azure_Functions_HL_Create` shall fail and return NULL. **]**

**SRS_AZUREFUNCTIONS_HL_04_014: [** `Azure_Functions_HL_Create` shall release 
all data it allocated. **]**


## Azure_Functions_HL_Destroy
```C
static void Azure_Functions_HL_Destroy(MODULE_HANDLE moduleHandle);
```

**SRS_AZUREFUNCTIONS_HL_04_015: [** `Azure_Functions_HL_Destroy` shall free all 
used resources. **]**

**SRS_AZUREFUNCTIONS_HL_04_017: [** `Azure_Functions_HL_Destroy` shall do nothing if `moduleHandle` is NULL. **]**


## Azure_Functions_HL_Receive
```C
static void Azure_Functions_HL_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle);
```

**SRS_AZUREFUNCTIONS_HL_04_016: [** `Azure_Functions_HL_Receive` shall pass the 
received parameters to the underlying  identity map module receive function. **]**

**SRS_AZUREFUNCTIONS_HL_04_018: [** `Azure_Functions_HL_Receive` shall do nothing if any parameter is NULL. **]**