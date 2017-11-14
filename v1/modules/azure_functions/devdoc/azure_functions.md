# Azure Functions Module Requirements

## Overview
This document describes the Azure Functions module. This module gets messages received from other modules and sends as an http POST, triggering an Azure Function.

#### Http GET
This module sends an HTTP POST to https://<hostAddress>/<relativepath>?name=myGatewayDevice. It adds the content of all messages received on the body of the POST (Content-Type: application/json) and also
adds an HTTP HEADER for key/code credential (if key configurations present).


## References
[module.h](../../../core/devdoc/module.md)

[Azure Functions Module](azure_functions.md)

[httpapiex.h](../../../deps/c-utility/inc/azure_c_shared_utility/httpapiex.h)

[Introduction to Azure Functions](https://azure.microsoft.com/en-us/blog/introducing-azure-functions/)

## Exposed API
```c

typedef struct AZURE_FUNCTIONS_CONFIG_TAG
{
    STRING_HANDLE hostAddress;
    STRING_HANDLE relativePath;
    STRING_HANDLE securityKey;
} AZURE_FUNCTIONS_CONFIG;

MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version)

```

## Module_GetApi

This is the primary public interface for the module.  It returns a pointer to
the `MODULE_API` structure containing the implementation functions for this module.
The following functions are the implementation of those APIs.

**SRS_AZUREFUNCTIONS_04_020: [** `Module_GetApi` shall return the `MODULE_API` structure. **]**

## AzureFunctions_ParseConfigurationFromJson
```C
void* AzureFunctions_ParseConfigurationFromJson(const void* configuration);
```
This function creates the configuration data for the Azure Functions module.
This function expects a string representing a JSON object with two
values--hostAddress and relativePath--which together form the URL to an Azure
Function.

**SRS_AZUREFUNCTIONS_05_003: [** If `configuration` is NULL then
 `AzureFunctions_ParseConfigurationFromJson` shall fail and return NULL. **]**

**SRS_AZUREFUNCTIONS_05_004: [** If `configuration` is not a JSON string, then `AzureFunctions_ParseConfigurationFromJson` shall fail and return NULL. **]**

**SRS_AZUREFUNCTIONS_05_005: [** `AzureFunctions_ParseConfigurationFromJson` shall parse the
`configuration` as a JSON array of strings. **]**

**SRS_AZUREFUNCTIONS_05_006: [** If the array object does not contain a value
named "hostAddress" then `AzureFunctions_ParseConfigurationFromJson` shall fail and return
NULL. **]**

**SRS_AZUREFUNCTIONS_05_007: [** If the array object does not contain a value
named "relativePath" then `AzureFunctions_ParseConfigurationFromJson` shall fail and return
NULL. **]**

**SRS_AZUREFUNCTIONS_05_019: [** If the array object contains a value named
"key" then `AzureFunctions_ParseConfigurationFromJson` shall create a securityKey based on
input key **]**

**SRS_AZUREFUNCTIONS_05_008: [** `AzureFunctions_ParseConfigurationFromJson` shall call
STRING_construct to create hostAddress based on input host address. **]**

**SRS_AZUREFUNCTIONS_05_009: [** `AzureFunctions_ParseConfigurationFromJson` shall call
STRING_construct to create relativePath based on input host address. **]**

**SRS_AZUREFUNCTIONS_05_010: [** If creating the strings fails, then
`AzureFunctions_ParseConfigurationFromJson` shall fail and return NULL. **]**

**SRS_AZUREFUNCTIONS_17_001: [** `AzureFunctions_ParseConfigurationFromJson` shall allocate an `AZURE_FUNCTIONS_CONFIG` structure. **]**

**SRS_AZUREFUNCTIONS_17_002: [** `AzureFunctions_ParseConfigurationFromJson` shall fill the structure with the constructed strings and return it upon success. **]**

**SRS_AZUREFUNCTIONS_17_003: [** `AzureFunctions_ParseConfigurationFromJson` shall return `NULL` on failure. **]**


**SRS_AZUREFUNCTIONS_05_014: [** `AzureFunctions_ParseConfigurationFromJson` shall release
all data it allocated. **]**

## AzureFunctions_FreeConfiguration
```c
static void AzureFunctions_FreeConfiguration(void * configuration)
```

This function releases any resources allocated by `AzureFunctions_ParseConfigurationFromJson`.

**SRS_AZUREFUNCTIONS_17_004: [** `AzureFunctions_FreeConfiguration` shall do nothing if `configuration` is `NULL`. **]**

**SRS_AZUREFUNCTIONS_17_005: [** `AzureFunctions_FreeConfiguration` shall release all allocated resources if `configuration` is not `NULL`. **]**

## AzureFunctions_Create
```C
MODULE_HANDLE AzureFunctions_Create(BROKER_HANDLE broker, const void* configuration);
```

This function creates the Azure Functions module. This function expects a
`AZURE_FUNCTIONS_CONFIG`, which contains three strings--hostAddress,
relativePath, and securityKey (optional)--which together form the URL to an
Azure Function.

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


01: Retrieve the content of the message received

02: Create an HTTPAPIEX handle

03: Clone the relative path handle

04: Add to the `name` parameter together with an ID (Hard Coded) to the relative path

05: Add the content of the message as a body of the HTTP POST, content type application/json

06: Add securityKey in HTTP Header, if security key string exists

07: Call HTTPAPIEX_ExecuteRequest to send the message

08: Log the reply back by the Azure Functions


**SRS_AZUREFUNCTIONS_04_010: [** If `moduleHandle` is NULL then `AzureFunctions_Receive` shall fail and return. **]**

**SRS_AZUREFUNCTIONS_04_011: [** If `messageHandle` is NULL then `AzureFunctions_Receive` shall fail and return. **]**

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
