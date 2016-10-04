# Azure Functions Module Requirements

## Overview
This document describes the Azure Functions module.  This module gets messages received from other modules and sends as an http POST, triggering an Azure Function. 
 
#### Http GET 
This module sends an HTTP POST to https://<hostAddress>/<relativepath>?name=myGatewayDevice. It add the content of all messages received on the Body of the POST (Content-Type: application/json) and also 
adds an HTTP HEADER for key/code credential (if key configurations if present).


## References
[module.h](../../../../devdoc/module.md)

[Azure Functions Module](azure_functions.md)

[httpapiex.h](../../../../azure-c-shared-utility/c/inc/httpapiex.h)

[Introduction to Azure Functions](https://azure.microsoft.com/en-us/blog/introducing-azure-functions/)

## Exposed API
```c

typedef struct AZURE_FUNCTIONS_CONFIG_TAG
{
    STRING_HANDLE hostAddress;
	STRING_HANDLE relativePath;
	STRING_HANDLE securityKey;
} AZURE_FUNCTIONS_CONFIG;

MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void);

```

## Module_GetAPIs

This is the primary public interface for the module.  It returns a pointer to 
the `MODULE_APIS` structure containing the implementation functions for this module.  
The following functions are the implementation of those APIs.

**SRS_AZUREFUNCTIONS_04_020: [** `Module_GetAPIS` shall fill the provided `MODULE_APIS` function with the required function pointers. **]**

## AzureFunctions_Create
```C
MODULE_HANDLE AzureFunctions_Create(BROKER_HANDLE broker, const void* configuration);
```

This function creates the Azure Functions module.  This module expects a `AZURE_FUNCTIONS_CONFIG`, which contains three strings which are hostAddress, relativePath and securityKey (Optional)
 for an HTTP Trigger Azure Function. 

**SRS_AZUREFUNCTIONS_04_001: [** Upon success, this function shall return a valid pointer to a `MODULE_HANDLE`. **]**

**SRS_AZUREFUNCTIONS_04_002: [** If the `broker` is `NULL`, this function shall fail and return `NULL`. **]**

**SRS_AZUREFUNCTIONS_04_003: [** If the configuration is `NULL`, this function shall fail and return `NULL`. **]**

**SRS_AZUREFUNCTIONS_04_004: [** If any `hostAddress` or `relativePath` are `NULL`, this function shall fail and return `NULL`. **]**

The valid module handle will be a pointer to the structure:

```C
typedef struct AZURE_FUNCTIONS_DATA_TAG
{
	BROKER_HANDLE broker;
	AZURE_FUNCTIONS_CONFIG *AzureFunctionsConfiguration;
} AZURE_FUNCTIONS_DATA;
```    

Where `broker` is the message broker passed in as input, `AzureFunctionsConfiguration` is structure with the 3 `STRING_HANDLE` for
`hostAddress`,`relativePath` and `securityKey`.

**SRS_AZUREFUNCTIONS_04_005: [** If `AzureFunctions_Create` fails to allocate a new `AZURE_FUNCTIONS_DATA` structure, then this function shall fail, and return `NULL`. **]**

**SRS_AZUREFUNCTIONS_04_006: [** If `AzureFunctions_Create` fails to clone STRING for `hostAddress`, then this function shall fail and return `NULL`. **]**

**SRS_AZUREFUNCTIONS_04_007: [** If `AzureFunctions_Create` fails to clone STRING for `relativePath`, then this function shall fail and return `NULL`. **]**

**SRS_AZUREFUNCTIONS_04_021: [** If `AzureFunctions_Create` fails to clone STRING for `securityKey`, then this function shall fail and return `NULL`. **]**

**SRS_AZUREFUNCTIONS_04_022: [** if `securityKey` STRING is NULL `AzureFunctions_Create` shall do nothing, since this STRING is optional. **]**

## Module_Destroy
```C
static void AzureFunctions_Destroy(MODULE_HANDLE moduleHandle);
```

This function released all resources owned by the module specified by the `moduleHandle`.

**SRS_AZUREFUNCTIONS_04_008: [** If `moduleHandle` is `NULL`, `AzureFunctions_Destroy` shall return. **]**

**SRS_AZUREFUNCTIONS_04_009: [** `AzureFunctions_Destroy` shall release all resources allocated for the module. **]**



## AzureFunctions_Receive
```C
static void AzureFunctions_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle);
```

This function will be the main work of this module. The processing of each 
message in pseudocode is as follows:


01: Retrieves the content of the message received;

02: Create an HTTPAPIEX handle;

03: Clone the relative path handle

04: Adds to the  `name` parameter together with an ID (Hard Coded) to the relative path;

05: adds the content of the message as a body of the HTTP POST, content type application/json;

06: Adds securityKey as HTTP Header, if security key string exists;

07: call HTTPAPIEX_ExecuteRequest to send the message;

08: logs the reply back by the Azure Functions.


**SRS_AZUREFUNCTIONS_04_010: [** If `moduleHandle` is NULL than `AzureFunctions_Receive` shall fail and return. **]**

**SRS_AZUREFUNCTIONS_04_011: [** If `messageHandle` is NULL than `AzureFunctions_Receive` shall fail and return. **]**

**SRS_AZUREFUNCTIONS_04_012: [** `AzureFunctions_Receive` shall get the message content by calling  `Message_GetContent`, if it fails it shall fail and return. **]**

**SRS_AZUREFUNCTIONS_04_013: [** `AzureFunctions_Receive` shall base64 encode by calling `Base64_Encode_Bytes`, if it fails it shall fail and return. **]**

**SRS_AZUREFUNCTIONS_04_024: [** `AzureFunctions_Receive` shall create a JSON STRING with the content of the message received. If it fails it shall fail and return. **]**

**SRS_AZUREFUNCTIONS_04_014: [** `AzureFunctions_Receive` shall call HTTPAPIEX_Create, passing `hostAddress`, it if fails it shall fail and return.  **]**

**SRS_AZUREFUNCTIONS_04_015: [** `AzureFunctions_Receive` shall call allocate memory to receive data from HTTPAPI by calling `BUFFER_new`, if it fail it shall fail and return.  **]**

**SRS_AZUREFUNCTIONS_04_016: [** `AzureFunctions_Receive` shall add `name` to relative path, if it fail it shall fail and return.  **]**

**SRS_AZUREFUNCTIONS_04_025: [** `AzureFunctions_Receive` shall add 2 HTTP Headers to POST Request. `Content-Type`:`application/json` and, if `securityKey` exists `x-functions-key`:`securityKey`. If it fails it shall fail and return. **]**

**SRS_AZUREFUNCTIONS_04_017: [** `AzureFunctions_Receive` shall `HTTPAPIEX_ExecuteRequest` to send the HTTP POST to Azure Functions. If it fail it shall fail and return.  **]**

**SRS_AZUREFUNCTIONS_04_018: [** Upon success `AzureFunctions_Receive` shall log the response from HTTP POST and return.  **]**

**SRS_AZUREFUNCTIONS_04_019: [** `AzureFunctions_Receive` shall destroy any allocated memory before returning. **]**
