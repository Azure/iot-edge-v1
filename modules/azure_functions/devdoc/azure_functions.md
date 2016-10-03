# Azure Functions Module Requirements

## Overview
This document describes the Azure Functions module.  This module gets messages received from other modules and sends as an http GET, triggering an Azure Function. 
 
#### Http GET 
This module sends an HTTP GET request to https://<hostAddress>/<relativepath>?name=myGatewayDevice&content=<MessageContentreceived>


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

This function creates the Azure Functions module.  This module expects a `AZURE_FUNCTIONS_CONFIG`, which contains two strings which are hostAddress and relativePath
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

Where `broker` is the message broker passed in as input, `AzureFunctionsConfiguration` is structure with the 2 `STRING_HANDLE` for
`hostAddress` and `relativePath`.

**SRS_AZUREFUNCTIONS_04_005: [** If `AzureFunctions_Create` fails to allocate a new `AZURE_FUNCTIONS_DATA` structure, then this function shall fail, and return `NULL`. **]**

**SRS_AZUREFUNCTIONS_04_006: [** If `AzureFunctions_Create` fails to clone STRING for `hostAddress`, then this function shall fail and return `NULL`. **]**

**SRS_AZUREFUNCTIONS_04_007: [** If `AzureFunctions_Create` fails to clone STRING for `relativePath`, then this function shall fail and return `NULL`. **]**


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

05: adds the `content` parameter together with the message content to the relative path;

06: call HTTPAPIEX_ExecuteRequest to send the message;

07: logs the reply back by the Azure Functions.


**SRS_AZUREFUNCTIONS_04_010: [** If `moduleHandle` is NULL than `AzureFunctions_Receive` shall fail and return. **]**

**SRS_AZUREFUNCTIONS_04_011: [** If `messageHandle` is NULL than `AzureFunctions_Receive` shall fail and return. **]**

**SRS_AZUREFUNCTIONS_04_012: [** `AzureFunctions_Receive` shall get the message content by calling  `Message_GetContent`, if it fails it shall fail and return. **]**

**SRS_AZUREFUNCTIONS_04_013: [** `AzureFunctions_Receive` shall base64 encode by calling `Base64_Encode_Bytes`, if it fails it shall fail and return. **]**


**SRS_AZUREFUNCTIONS_04_014: [** `AzureFunctions_Receive` shall call HTTPAPIEX_Create, passing `hostAddress`, it if fails it shall fail and return.  **]**

**SRS_AZUREFUNCTIONS_04_015: [** `AzureFunctions_Receive` shall call allocate memory to receive data from HTTPAPI by calling `BUFFER_new`, if it fail it shall fail and return.  **]**

**SRS_AZUREFUNCTIONS_04_016: [** `AzureFunctions_Receive` shall add `name` and `content` parameter to relative path, if it fail it shall fail and return.  **]**

**SRS_AZUREFUNCTIONS_04_017: [** `AzureFunctions_Receive` shall `HTTPAPIEX_ExecuteRequest` to send the HTTP GET to Azure Functions. If it fail it shall fail and return.  **]**

**SRS_AZUREFUNCTIONS_04_018: [** Upon success `AzureFunctions_Receive` shall log the response from HTTP GET and return.  **]**

**SRS_AZUREFUNCTIONS_04_019: [** `AzureFunctions_Receive` shall destroy any allocated memory before returning. **]**
