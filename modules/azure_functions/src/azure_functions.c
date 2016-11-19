// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "azure_c_shared_utility/gballoc.h"

#include <stddef.h>
#include "azure_c_shared_utility/strings.h"
#include <ctype.h>

#include "azure_c_shared_utility/crt_abstractions.h"
#include "messageproperties.h"
#include "message.h"
#include "broker.h"
#include "azure_functions.h"
#include "azure_c_shared_utility/constmap.h"
#include "azure_c_shared_utility/constbuffer.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/httpapiex.h"
#include "azure_c_shared_utility/base64.h"

#include <parson.h>

typedef struct AZURE_FUNCTIONS_DATA_TAG
{
    BROKER_HANDLE broker;
    AZURE_FUNCTIONS_CONFIG *azureFunctionsConfiguration;
} AZURE_FUNCTIONS_DATA;

#define AZURE_FUNCTIONS_RESULT_VALUES \
    AZURE_FUNCTIONS_OK, \
    AZURE_FUNCTIONS_ERROR, \
    AZURE_FUNCTIONS_MEMORY

DEFINE_ENUM(AZURE_FUNCTIONS_RESULT, AZURE_FUNCTIONS_RESULT_VALUES)

DEFINE_ENUM_STRINGS(BROKER_RESULT, BROKER_RESULT_VALUES);

/*
 * @brief    Create an Azure Functions module.
 */
static MODULE_HANDLE AzureFunctions_Create(BROKER_HANDLE broker, const void* configuration)
{
    AZURE_FUNCTIONS_DATA* result;
    if (broker == NULL || configuration == NULL)
    {
        /* Codes_SRS_AZUREFUNCTIONS_04_002: [ If the broker is NULL, this function shall fail and return NULL. ] */
        /* Codes_SRS_AZUREFUNCTIONS_04_003: [ If the configuration is NULL, this function shall fail and return NULL. ] */
        LogError("Received NULL arguments: broker = %p, configuration = %p", broker, configuration);
        result = NULL;
    }
    else
    {
        AZURE_FUNCTIONS_CONFIG* config = (AZURE_FUNCTIONS_CONFIG*)configuration;
        if (config->hostAddress == NULL || config->relativePath == NULL)
        {
            /* Codes_SRS_AZUREFUNCTIONS_04_004: [ If any hostAddress or relativePath are NULL, this function shall fail and return NULL. ] */
            LogError("invalid parameter (NULL). HostAddress or relativePath can't be NULL.");
            result = NULL;
        }
        else
        {
            result = (AZURE_FUNCTIONS_DATA*)malloc(sizeof(AZURE_FUNCTIONS_DATA));
            if (result == NULL)
            {
                /* Codes_SRS_AZUREFUNCTIONS_04_005: [ If azureFunctions_Create fails to allocate a new AZURE_FUNCTIONS_DATA structure, then this function shall fail, and return NULL. ] */
                LogError("Could not Allocate Module");
            }
            else
            {
                /* Codes_SRS_AZUREFUNCTIONS_04_001: [ Upon success, this function shall return a valid pointer to a MODULE_HANDLE. ] */
                result->azureFunctionsConfiguration = (AZURE_FUNCTIONS_CONFIG*)malloc(sizeof(AZURE_FUNCTIONS_CONFIG));
                if (result->azureFunctionsConfiguration == NULL)
                {
                    LogError("Could not Allocation Memory for Configuration");
                    free(result);
                    result = NULL;
                }
                else
                {
                    result->azureFunctionsConfiguration->hostAddress = STRING_clone(config->hostAddress);
                    if (result->azureFunctionsConfiguration->hostAddress == NULL)
                    {
                        /* Codes_SRS_AZUREFUNCTIONS_04_006: [ If azureFunctions_Create fails to clone STRING for hostAddress, then this function shall fail and return NULL. ] */
                        LogError("Error cloning string for hostAddress.");
                        free(result->azureFunctionsConfiguration);
                        free(result);
                        result = NULL;
                    }
                    else
                    {
                        result->azureFunctionsConfiguration->relativePath = STRING_clone(config->relativePath);
                        if (result->azureFunctionsConfiguration->relativePath == NULL)
                        {
                            /* Codes_SRS_AZUREFUNCTIONS_04_007: [ If azureFunctions_Create fails to clone STRING for relativePath, then this function shall fail and return NULL. ] */
                            LogError("Error cloning string for relativePath.");
                            STRING_delete(result->azureFunctionsConfiguration->hostAddress);
                            free(result->azureFunctionsConfiguration);
                            free(result);
                            result = NULL;
                        }
                        else
                        {
                            /* Codes_SRS_AZUREFUNCTIONS_04_022: [ if securityKey STRING is NULL AzureFunctions_Create shall do nothing, since this STRING is optional. ] */
                            if (config->securityKey != NULL)
                            {
                                result->azureFunctionsConfiguration->securityKey = STRING_clone(config->securityKey);
                                if (result->azureFunctionsConfiguration->securityKey == NULL)
                                {
                                    /* Codes_SRS_AZUREFUNCTIONS_04_021: [ If AzureFunctions_Create fails to clone STRING for securityKey, then this function shall fail and return NULL. ] */
                                    LogError("Error cloning string for securityKey.");
                                    STRING_delete(result->azureFunctionsConfiguration->relativePath);
                                    STRING_delete(result->azureFunctionsConfiguration->hostAddress);                                    
                                    free(result->azureFunctionsConfiguration);
                                    free(result);
                                    result = NULL;
                                }
                                else
                                {
                                    /* Codes_SRS_AZUREFUNCTIONS_04_001: [ Upon success, this function shall return a valid pointer to a MODULE_HANDLE. ] */
                                    result->broker = broker;
                                }
                            }
                            else
                            {
                                result->azureFunctionsConfiguration->securityKey = NULL;
                                /* Codes_SRS_AZUREFUNCTIONS_04_001: [ Upon success, this function shall return a valid pointer to a MODULE_HANDLE. ] */
                                result->broker = broker;
                            }
                        }
                    }
                }
            }
        }
    }
    return result;
}

/*
* @brief    Create an Azure Functions configuration from a JSON configuration file.
*/
static void * AzureFunctions_ParseConfigurationFromJson(const char* configuration)
{
    AZURE_FUNCTIONS_CONFIG* result;
    if (configuration == NULL)
    {
        /* Codes_SRS_AZUREFUNCTIONS_05_003: [ If configuration is NULL then Azure_Functions_CreateFromJson shall fail and return NULL. ] */
        LogError("NULL parameter detected");
        result = NULL;
    }
    else
    {
        /* Codes_SRS_AZUREFUNCTIONS_05_005: [ Azure_Functions_CreateFromJson shall parse the configuration as a JSON array of strings. ] */
        JSON_Value* json = json_parse_string((const char*)configuration);
        if (json == NULL)
        {
            /* Codes_SRS_AZUREFUNCTIONS_05_004: [ If configuration is not a JSON string, then Azure_Functions_CreateFromJson shall fail and return NULL. ] */
            LogError("unable to json_parse_string");
            result = NULL;
        }
        else
        {
            JSON_Object* obj = json_value_get_object(json);
            if (obj == NULL)
            {
                /* Codes_SRS_AZUREFUNCTIONS_05_004: [ If configuration is not a JSON string, then Azure_Functions_CreateFromJson shall fail and return NULL. ] */
                LogError("unable to json_value_get_object");
                result = NULL;
            }
            else
            {
                /* Codes_SRS_AZUREFUNCTIONS_05_006: [If the array object does not contain a value named "hostAddress" then Azure_Functions_CreateFromJson shall fail and return NULL.] */
                const char* hostAddress = json_object_get_string(obj, "hostname");
                if (hostAddress == NULL)
                {
                    LogError("json_object_get_string failed. hostname");
                    result = NULL;
                }
                else
                {
                    /* Codes_SRS_AZUREFUNCTIONS_05_007: [ If the array object does not contain a value named "relativePath" then Azure_Functions_CreateFromJson shall fail and return NULL. ] */
                    const char* relativePath = json_object_get_string(obj, "relativePath");
                    if (relativePath == NULL)
                    {
                        LogError("json_object_get_string failed. relativePath.");
                        result = NULL;
                    }
                    else
                    {
                        const char* key = json_object_get_string(obj, "key");

                        AZURE_FUNCTIONS_CONFIG config;
                        /* Codes_SRS_AZUREFUNCTIONS_05_019: [ If the array object contains a value named "key" then AzureFunctions_CreateFromJson shall create a securityKey based on input key ] */
                        config.securityKey = STRING_construct(key); //Doesn't need to test key. If key is null will mean we are sending an anonymous request.
                        config.relativePath = NULL;
                        /* Codes_SRS_AZUREFUNCTIONS_05_008: [ Azure_Functions_CreateFromJson shall call STRING_construct to create hostAddress based on input host address. ] */
                        config.hostAddress = STRING_construct(hostAddress);

                        if (config.hostAddress == NULL)
                        {
                            /* Codes_SRS_AZUREFUNCTIONS_05_010: [ If creating the strings fails, then Azure_Functions_CreateFromJson shall fail and return NULL. ] */
                            LogError("error buliding hostAddress String.");
                            result = NULL;
                        }
                        else
                        {
                            /* Codes_SRS_AZUREFUNCTIONS_05_009: [ Azure_Functions_CreateFromJson shall call STRING_construct to create relativePath based on input host address. ] */
                            config.relativePath = STRING_construct(relativePath);
                            if (config.relativePath == NULL)
                            {
                                /* Codes_SRS_AZUREFUNCTIONS_05_010: [ If creating the strings fails, then Azure_Functions_CreateFromJson shall fail and return NULL. ] */
                                LogError("error buliding relative path String.");
                                result = NULL;
                            }
                            else
                            {
                                /* Codes_SRS_AZUREFUNCTIONS_17_001: [ AzureFunctions_ParseConfigurationFromJson shall allocate an AZURE_FUNCTIONS_CONFIG structure. ]*/
								result = malloc(sizeof(AZURE_FUNCTIONS_CONFIG));
								if (result == NULL)
								{
                                    /*Codes_SRS_AZUREFUNCTIONS_17_003: [ AzureFunctions_ParseConfigurationFromJson shall return NULL on failure. ]*/
									LogError("could not allocate AZURE_FUNCTIONS_CONFIG");
								}
								else
								{
                                    /*Codes_SRS_AZUREFUNCTIONS_17_002: [ AzureFunctions_ParseConfigurationFromJson shall fill the structure with the constructed strings and return it upon success. ]*/
									*result = config;
								}
                            }
                        }
						if (result == NULL)
						{
                            /* Codes_SRS_AZUREFUNCTIONS_05_014: [ Azure_Functions_CreateFromJson shall release all data it allocated. ] */
							STRING_delete(config.hostAddress);
							STRING_delete(config.relativePath);
							STRING_delete(config.securityKey);
						}
                    }
                }
            }
            /* Codes_SRS_AZUREFUNCTIONS_05_014: [ Azure_Functions_CreateFromJson shall release all data it allocated. ] */
            json_value_free(json);
        }
    }

    return result;
}

static void AzureFunctions_FreeConfiguration(void * configuration)
{
    /*Codes_SRS_AZUREFUNCTIONS_17_004: [ AzureFunctions_FreeConfiguration shall do nothing if configuration is NULL. ]*/
    if (configuration != NULL)
    {
        /*Codes_SRS_AZUREFUNCTIONS_17_005: [ AzureFunctions_FreeConfiguration shall release all allocated resources if configuration is not NULL. ]*/
		AZURE_FUNCTIONS_CONFIG * azure_config = (AZURE_FUNCTIONS_CONFIG*)configuration;
        STRING_delete(azure_config->hostAddress);
        STRING_delete(azure_config->relativePath);
        STRING_delete(azure_config->securityKey);
        free(configuration);        
    }

}
/*
* @brief    Destroy an identity map module.
*/
static void AzureFunctions_Destroy(MODULE_HANDLE moduleHandle)
{
    /* Codes_SRS_AZUREFUNCTIONS_04_008: [ If moduleHandle is NULL, azureFunctions_Destroy shall return. ] */
    if (moduleHandle != NULL)
    {
        /* Codes_SRS_AZUREFUNCTIONS_04_009: [ azureFunctions_Destroy shall release all resources allocated for the module. ] */
        AZURE_FUNCTIONS_DATA * moduleData = (AZURE_FUNCTIONS_DATA*)moduleHandle;
        STRING_delete(moduleData->azureFunctionsConfiguration->hostAddress);
        STRING_delete(moduleData->azureFunctionsConfiguration->relativePath);
        STRING_delete(moduleData->azureFunctionsConfiguration->securityKey);

        free(moduleData->azureFunctionsConfiguration);
        free(moduleData);
    }

}

/*
 * @brief    Receive a message from the message broker.
 */
static void AzureFunctions_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    if (moduleHandle == NULL || messageHandle == NULL)
    {
        /* Codes_SRS_AZUREFUNCTIONS_04_010: [If moduleHandle is NULL than azureFunctions_Receive shall fail and return.] */
        /* Codes_SRS_AZUREFUNCTIONS_04_011: [ If messageHandle is NULL than azureFunctions_Receive shall fail and return. ] */
        LogError("Received NULL arguments: module = %p, massage = %p", moduleHandle, messageHandle);
    }
    else
    {
        AZURE_FUNCTIONS_DATA*module_data = (AZURE_FUNCTIONS_DATA*)moduleHandle;

        /* Codes_SRS_AZUREFUNCTIONS_04_012: [ azureFunctions_Receive shall get the message content by calling Message_GetContent, if it fails it shall fail and return. ] */
        const CONSTBUFFER* content = Message_GetContent(messageHandle);

        if (content == NULL)
        {
            LogError("unable to get message content.");
        }
        else
        {
            /* Codes_SRS_AZUREFUNCTIONS_04_013: [ AzureFunctions_Receive shall base64 encode by calling Base64_Encode_Bytes, if it fails it shall fail and return. ] */
            STRING_HANDLE contentAsJSON = Base64_Encode_Bytes(content->buffer, content->size);
            if (contentAsJSON == NULL)
            {
                LogError("unable to Base64_Encode_Bytes");
            }
            else
            {
                STRING_HANDLE jsonToBeAppended = STRING_construct("{");
                if (jsonToBeAppended == NULL)
                {
                    LogError("unable to STRING_construct");
                }
                else
                {
                    /* Codes_SRS_AZUREFUNCTIONS_04_024: [ AzureFunctions_Receive shall create a JSON STRING with the content of the message received. If it fails it shall fail and return. ] */
                    if (!((STRING_concat(jsonToBeAppended, "\"content\":\"") == 0) &&
                        (STRING_concat_with_STRING(jsonToBeAppended, contentAsJSON) == 0) &&
                        (STRING_concat(jsonToBeAppended, "\"}") == 0)
                        ))
                    {
                        LogError("STRING concatenation error");
                    }
                    else
                    {
                        /* Codes_SRS_AZUREFUNCTIONS_04_014: [ azureFunctions_Receive shall call HTTPAPIEX_Create, passing hostAddress, it if fails it shall fail and return. ] */
                        HTTPAPIEX_HANDLE myHTTPEXHandle = HTTPAPIEX_Create(STRING_c_str(module_data->azureFunctionsConfiguration->hostAddress));
                        if (myHTTPEXHandle == NULL)
                        {
                            LogError("Failed to create HTTPAPIEX handle.");
                        }
                        else
                        {
                            /* Codes_SRS_AZUREFUNCTIONS_04_015: [ azureFunctions_Receive shall call allocate memory to receive data from HTTPAPI by calling BUFFER_new, if it fail it shall fail and return. ] */
                            BUFFER_HANDLE myResponseBuffer = BUFFER_new();

                            if (myResponseBuffer == NULL)
                            {
                                LogError("Failed to create response Buffer.");
                            }
                            else
                            {
                                /* Codes_SRS_AZUREFUNCTIONS_04_016: [ azureFunctions_Receive shall add name to relative path, if it fail it shall fail and return. ] */
                                STRING_HANDLE relativePathInfoForRequest = STRING_clone(module_data->azureFunctionsConfiguration->relativePath);

                                if (relativePathInfoForRequest == NULL)
                                {
                                    LogError("Error building request String.");
                                }
                                else
                                {
                                    if ((STRING_concat(relativePathInfoForRequest, "?name=myGatewayDevice") != 0))
                                    {
                                        LogError("Error building request String.");
                                    }
                                    else
                                    {
                                        //Adding HTTP HEaders here. 
                                        HTTP_HEADERS_HANDLE httpHeaders = HTTPHeaders_Alloc();

                                        if (httpHeaders == NULL)
                                        {
                                            LogError("Error creating HttpHeaders");
                                        }
                                        else
                                        {
                                            /* Codes_SRS_AZUREFUNCTIONS_04_025: [ AzureFunctions_Receive shall add 2 HTTP Headers to POST Request. Content-Type:application/json and, if securityKey exists x-functions-key:securityKey. If it fails it shall fail and return. ] */
                                            if (HTTPHeaders_AddHeaderNameValuePair(httpHeaders, "Content-Type", "application/json") != HTTP_HEADERS_OK)
                                            {
                                                LogError("Error Adding Content-Type header.");
                                            }
                                            else if (module_data->azureFunctionsConfiguration->securityKey != NULL &&
                                                (HTTPHeaders_AddHeaderNameValuePair(httpHeaders, "x-functions-key", STRING_c_str(module_data->azureFunctionsConfiguration->securityKey)) != HTTP_HEADERS_OK))
                                            {
                                                LogError("Error Adding Content-Type.");
                                            }
                                            else
                                            {

                                                //Add here the content on the body.
                                                BUFFER_HANDLE postContent = BUFFER_create(STRING_c_str(jsonToBeAppended), STRING_length(jsonToBeAppended));

                                                if (postContent == NULL)
                                                {
                                                    LogError("Error building post content.");
                                                }
                                                else
                                                {
                                                    unsigned int statuscodeBack = 0;
                                                    /* Codes_SRS_AZUREFUNCTIONS_04_017: [ azureFunctions_Receive shall HTTPAPIEX_ExecuteRequest to send the HTTP POST to Azure Functions. If it fail it shall fail and return. ] */
                                                    HTTPAPIEX_RESULT requestResult = HTTPAPIEX_ExecuteRequest(myHTTPEXHandle, HTTPAPI_REQUEST_POST, STRING_c_str(relativePathInfoForRequest), httpHeaders, postContent, &statuscodeBack, NULL, myResponseBuffer);

                                                    if (requestResult != HTTPAPIEX_OK || statuscodeBack != 200)
                                                    {
                                                        LogError("Error Sending Request. Status Code: %d", statuscodeBack);
                                                    }
                                                    else
                                                    {
                                                        /* Codes_SRS_AZUREFUNCTIONS_04_018: [ Upon success azureFunctions_Receive shall log the response from HTTP POST and return. ] */
                                                        LogInfo("Request Sent to Function Succesfully. Response from Functions: %s", BUFFER_u_char(myResponseBuffer));
                                                    }
                                                    BUFFER_delete(postContent);
                                                }
                                            }
                                            /* Codes_SRS_AZUREFUNCTIONS_04_019: [ azureFunctions_Receive shall destroy any allocated memory before returning. ] */
                                            HTTPHeaders_Free(httpHeaders);
                                        }
                                    }
                                    /* Codes_SRS_AZUREFUNCTIONS_04_019: [ azureFunctions_Receive shall destroy any allocated memory before returning. ] */
                                    STRING_delete(relativePathInfoForRequest);
                                }
                                /* Codes_SRS_AZUREFUNCTIONS_04_019: [ azureFunctions_Receive shall destroy any allocated memory before returning. ] */
                                BUFFER_delete(myResponseBuffer);
                            }
                            /* Codes_SRS_AZUREFUNCTIONS_04_019: [ azureFunctions_Receive shall destroy any allocated memory before returning. ] */
                            HTTPAPIEX_Destroy(myHTTPEXHandle);
                        }
                    }
                    /* Codes_SRS_AZUREFUNCTIONS_04_019: [ azureFunctions_Receive shall destroy any allocated memory before returning. ] */
                    STRING_delete(jsonToBeAppended);
                }
                /* Codes_SRS_AZUREFUNCTIONS_04_019: [ azureFunctions_Receive shall destroy any allocated memory before returning. ] */
                STRING_delete(contentAsJSON);
            }
        }
    }
}


/*
 *    Required for all modules:  the public API and the designated implementation functions.
 */
static const MODULE_API_1 AzureFunctions_APIS_all =
{
    {MODULE_API_VERSION_1},
    AzureFunctions_ParseConfigurationFromJson,
    AzureFunctions_FreeConfiguration,
    AzureFunctions_Create,
    AzureFunctions_Destroy,
    AzureFunctions_Receive,
    NULL  /* No Start, this module acts on received messages. */
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(AZUREFUNCTIONS_MODULE)(MODULE_API_VERSION gateway_api_version)
#else
MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version)
#endif
{
    const MODULE_API* result;
    if (gateway_api_version >= AzureFunctions_APIS_all.base.version)
    {
        /* Codes_SRS_AZUREFUNCTIONS_04_020: [ Module_GetApi shall return the MODULE_API structure. ] */
        result = (const MODULE_API*)&AzureFunctions_APIS_all;
    }
    else
    {
        result = NULL;
    }
    return result;
}
