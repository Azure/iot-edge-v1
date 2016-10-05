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
 * @brief	Create an Azure Functions module.
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
* @brief	Destroy an identity map module.
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
 * @brief	Receive a message from the message broker.
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
													unsigned int statuscodeBack;
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
 *	Required for all modules:  the public API and the designated implementation functions.
 */
static const MODULE_APIS AzureFunctions_APIS_all =
{
	AzureFunctions_Create,
	AzureFunctions_Destroy,
	AzureFunctions_Receive
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT void MODULE_STATIC_GETAPIS(AZUREFUNCTIONS_MODULE)(MODULE_APIS* apis)
#else
MODULE_EXPORT void Module_GetAPIS(MODULE_APIS* apis)
#endif
{
	if (!apis)
	{
		LogError("NULL passed to Module_GetAPIS");
	}
	else
	{
		/* Codes_SRS_AZUREFUNCTIONS_04_020: [ Module_GetAPIS shall fill the provided MODULE_APIS function with the required function pointers. ] */
		(*apis) = AzureFunctions_APIS_all;
	}
}
