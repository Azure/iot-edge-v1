// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "azure_c_shared_utility/gballoc.h"

#include <stddef.h>
#include <azure_c_shared_utility/strings.h>
#include <ctype.h>

#include "azure_c_shared_utility/crt_abstractions.h"
#include "messageproperties.h"
#include "message.h"
#include "broker.h"
#include "functionshttptrigger.h"
#include "azure_c_shared_utility/constmap.h"
#include "azure_c_shared_utility/constbuffer.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/httpapiex.h"
#include "azure_c_shared_utility/base64.h"

typedef struct FUNCTIONS_HTTP_TRIGGER_DATA_TAG
{
	BROKER_HANDLE broker;
	FUNCTIONS_HTTP_TRIGGER_CONFIG *functionsHttpTriggerConfiguration;
} FUNCTIONS_HTTP_TRIGGER_DATA;

#define FUNCTIONS_HTTP_TRIGGER_RESULT_VALUES \
    FUNCTIONS_HTTP_TRIGGER_OK, \
    FUNCTIONS_HTTP_TRIGGER_ERROR, \
    FUNCTIONS_HTTP_TRIGGER_MEMORY

DEFINE_ENUM(FUNCTIONS_HTTP_TRIGGER_RESULT, FUNCTIONS_HTTP_TRIGGER_RESULT_VALUES)

DEFINE_ENUM_STRINGS(BROKER_RESULT, BROKER_RESULT_VALUES);

/*
 * @brief	Create a function http trigger module.
 */
static MODULE_HANDLE FunctionsHttpTrigger_Create(BROKER_HANDLE broker, const void* configuration)
{
	FUNCTIONS_HTTP_TRIGGER_DATA* result;
	if (broker == NULL || configuration == NULL)
	{
		/* Codes_SRS_FUNCHTTPTRIGGER_04_002: [ If the broker is NULL, this function shall fail and return NULL. ] */
		/* Codes_SRS_FUNCHTTPTRIGGER_04_003: [ If the configuration is NULL, this function shall fail and return NULL. ] */
		LogError("invalid parameter (NULL).");
		result = NULL;
	}
	else
	{
		FUNCTIONS_HTTP_TRIGGER_CONFIG* config = (FUNCTIONS_HTTP_TRIGGER_CONFIG*)configuration;
		if (config->hostAddress == NULL || config->relativePath == NULL)
		{
			/* Codes_SRS_FUNCHTTPTRIGGER_04_004: [ If any hostAddress or relativePath are NULL, this function shall fail and return NULL. ] */
			LogError("invalid parameter (NULL). HostAddress or relativePath can't be NULL.");
			result = NULL;
		}
		else
		{
			result = (FUNCTIONS_HTTP_TRIGGER_DATA*)malloc(sizeof(FUNCTIONS_HTTP_TRIGGER_DATA));
			if (result == NULL)
			{
				/* Codes_SRS_FUNCHTTPTRIGGER_04_005: [ If FunctionsHttpTrigger_Create fails to allocate a new FUNCTIONS_HTTP_TRIGGER_DATA structure, then this function shall fail, and return NULL. ] */
				LogError("Could not Allocate Module");
			}
			else
			{
				/* Codes_SRS_FUNCHTTPTRIGGER_04_001: [ Upon success, this function shall return a valid pointer to a MODULE_HANDLE. ] */
				result->functionsHttpTriggerConfiguration = (FUNCTIONS_HTTP_TRIGGER_CONFIG*)malloc(sizeof(FUNCTIONS_HTTP_TRIGGER_CONFIG));
				if (result->functionsHttpTriggerConfiguration == NULL)
				{
					LogError("Could not Allocation Memory for Configuration");
					free(result);
					result = NULL;
				}
				else
				{
					result->functionsHttpTriggerConfiguration->hostAddress = STRING_clone(config->hostAddress);
					if (result->functionsHttpTriggerConfiguration->hostAddress == NULL)
					{
						/* Codes_SRS_FUNCHTTPTRIGGER_04_006: [ If FunctionsHttpTrigger_Create fails to clone STRING for hostAddress, then this function shall fail and return NULL. ] */
						LogError("Error cloning string for hostAddress.");
						free(result->functionsHttpTriggerConfiguration);
						free(result);
						result = NULL;
					}
					else
					{
						result->functionsHttpTriggerConfiguration->relativePath = STRING_clone(config->relativePath);
						if (result->functionsHttpTriggerConfiguration->relativePath == NULL)
						{
							/* Codes_SRS_FUNCHTTPTRIGGER_04_007: [ If FunctionsHttpTrigger_Create fails to clone STRING for relativePath, then this function shall fail and return NULL. ] */
							LogError("Error cloning string for relativePath.");
							STRING_delete(result->functionsHttpTriggerConfiguration->hostAddress);
							free(result->functionsHttpTriggerConfiguration);
							free(result);
							result = NULL;
						}
						else
						{
							/* Codes_SRS_FUNCHTTPTRIGGER_04_001: [ Upon success, this function shall return a valid pointer to a MODULE_HANDLE. ] */
							result->broker = broker;
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
static void FunctionsHttpTrigger_Destroy(MODULE_HANDLE moduleHandle)
{
	/* Codes_SRS_FUNCHTTPTRIGGER_04_008: [ If moduleHandle is NULL, FunctionsHttpTrigger_Destroy shall return. ] */
	if (moduleHandle != NULL)
	{
		/* Codes_SRS_FUNCHTTPTRIGGER_04_009: [ FunctionsHttpTrigger_Destroy shall release all resources allocated for the module. ] */
		FUNCTIONS_HTTP_TRIGGER_DATA * idModule = (FUNCTIONS_HTTP_TRIGGER_DATA*)moduleHandle;
		STRING_delete(idModule->functionsHttpTriggerConfiguration->hostAddress);
		STRING_delete(idModule->functionsHttpTriggerConfiguration->relativePath);

		free(idModule->functionsHttpTriggerConfiguration);
		free(idModule);
	}

}

/*
 * @brief	Receive a message from the message broker.
 */
static void FunctionsHttpTrigger_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
	if (moduleHandle == NULL || messageHandle == NULL)
	{
		/* Codes_SRS_FUNCHTTPTRIGGER_04_010: [If moduleHandle is NULL than FunctionsHttpTrigger_Receive shall fail and return.] */
		/* Codes_SRS_FUNCHTTPTRIGGER_04_011: [ If messageHandle is NULL than FunctionsHttpTrigger_Receive shall fail and return. ] */
		LogError("Received NULL arguments: module = %p, massage = %p", moduleHandle, messageHandle);
	}
	else
	{
		FUNCTIONS_HTTP_TRIGGER_DATA*idModule = (FUNCTIONS_HTTP_TRIGGER_DATA*)moduleHandle;
		/* Codes_SRS_FUNCHTTPTRIGGER_04_012: [ FunctionsHttpTrigger_Receive shall get the message content by calling Message_GetContent, if it fails it shall fail and return. ] */
		const CONSTBUFFER * content = Message_GetContent(messageHandle);
		if (content == NULL)
		{
			LogError("Failed to get Message Content");
		}
		else
		{
			/* Codes_SRS_FUNCHTTPTRIGGER_04_013: [ FunctionsHttpTrigger_Receive shall base64 encode by calling Base64_Encode_Bytes, if it fails it shall fail and return. ] */
			STRING_HANDLE contentAsJSON = Base64_Encode_Bytes(content->buffer, content->size);
			if (contentAsJSON == NULL)
			{
				LogError("Failed to Base64 Encode Message Content.");
			}
			else
			{
				/* Codes_SRS_FUNCHTTPTRIGGER_04_014: [ FunctionsHttpTrigger_Receive shall call HTTPAPIEX_Create, passing hostAddress, it if fails it shall fail and return. ] */
				HTTPAPIEX_HANDLE myHTTPEXHandle = HTTPAPIEX_Create(STRING_c_str(idModule->functionsHttpTriggerConfiguration->hostAddress));
				if (myHTTPEXHandle == NULL)
				{
					LogError("Failed to create HTTPAPIEX handle.");
				}
				else
				{
					unsigned int statuscodeBack;
					/* Codes_SRS_FUNCHTTPTRIGGER_04_015: [ FunctionsHttpTrigger_Receive shall call allocate memory to receive data from HTTPAPI by calling BUFFER_new, if it fail it shall fail and return. ] */
					BUFFER_HANDLE myResponseBuffer = BUFFER_new();

					if (myResponseBuffer == NULL)
					{
						LogError("Failed to create response Buffer.");
					}
					else
					{
						/* Codes_SRS_FUNCHTTPTRIGGER_04_016: [ FunctionsHttpTrigger_Receive shall add name and content parameter to relative path, if it fail it shall fail and return. ] */
						STRING_HANDLE relativePathInfoForRequest = STRING_clone(idModule->functionsHttpTriggerConfiguration->relativePath);

						if (relativePathInfoForRequest == NULL)
						{
							LogError("Error building request String.");
						}
						else
						{
							if ((STRING_concat(relativePathInfoForRequest, "?name=myGatewayDevice") != 0) ||
								(STRING_concat(relativePathInfoForRequest, "&content=") != 0) ||
								(STRING_concat(relativePathInfoForRequest, STRING_c_str(contentAsJSON)) != 0))
							{
								LogError("Error building request String.");
							}
							else
							{
								/* Codes_SRS_FUNCHTTPTRIGGER_04_017: [ FunctionsHttpTrigger_Receive shall HTTPAPIEX_ExecuteRequest to send the HTTP GET to Azure Functions. If it fail it shall fail and return. ] */
								HTTPAPIEX_RESULT requestResult = HTTPAPIEX_ExecuteRequest(myHTTPEXHandle, HTTPAPI_REQUEST_GET, STRING_c_str(relativePathInfoForRequest), NULL, NULL, &statuscodeBack, NULL, myResponseBuffer);

								if (requestResult != HTTPAPIEX_OK)
								{
									LogError("Error Sending REquest. Status Code: %d", statuscodeBack);
								}
								else
								{
									/* Codes_SRS_FUNCHTTPTRIGGER_04_018: [ Upon success FunctionsHttpTrigger_Receive shall log the response from HTTP GET and return. ] */
									LogInfo("Request Sent to Function Succesfully. Response from Functions: %s", BUFFER_u_char(myResponseBuffer));
								}
							}
							/* Codes_SRS_FUNCHTTPTRIGGER_04_019: [ FunctionsHttpTrigger_Receive shall destroy any allocated memory before returning. ] */
							STRING_delete(relativePathInfoForRequest);
						}
						/* Codes_SRS_FUNCHTTPTRIGGER_04_019: [ FunctionsHttpTrigger_Receive shall destroy any allocated memory before returning. ] */
						BUFFER_delete(myResponseBuffer);
					}
					/* Codes_SRS_FUNCHTTPTRIGGER_04_019: [ FunctionsHttpTrigger_Receive shall destroy any allocated memory before returning. ] */
					HTTPAPIEX_Destroy(myHTTPEXHandle);
				}
				/* Codes_SRS_FUNCHTTPTRIGGER_04_019: [ FunctionsHttpTrigger_Receive shall destroy any allocated memory before returning. ] */
				STRING_delete(contentAsJSON);
			}
		}
	}
}


/*
 *	Required for all modules:  the public API and the designated implementation functions.
 */
static const MODULE_APIS FunctionsHttpTrigger_APIS_all =
{
	FunctionsHttpTrigger_Create,
	FunctionsHttpTrigger_Destroy,
	FunctionsHttpTrigger_Receive
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT void MODULE_STATIC_GETAPIS(FUNCTIONSHTTPTRIGGER_MODULE)(MODULE_APIS* apis)
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
		(*apis) = FunctionsHttpTrigger_APIS_all;
	}
}
