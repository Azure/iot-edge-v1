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
		LogError("invalid parameter (NULL).");
		result = NULL;
	}
	else
	{
		FUNCTIONS_HTTP_TRIGGER_CONFIG* config = (FUNCTIONS_HTTP_TRIGGER_CONFIG*)configuration;
		result = (FUNCTIONS_HTTP_TRIGGER_DATA*)malloc(sizeof(FUNCTIONS_HTTP_TRIGGER_DATA));
		if (result == NULL)
		{
			LogError("Could not Allocate Module");
		}
		else
		{
			result->functionsHttpTriggerConfiguration = (FUNCTIONS_HTTP_TRIGGER_CONFIG*)malloc(sizeof(FUNCTIONS_HTTP_TRIGGER_CONFIG));
			//TODO: MISSING ALL THE CHECK AND CLEAN UP. 
			result->functionsHttpTriggerConfiguration->hostAddress = STRING_clone(config->hostAddress);
			result->functionsHttpTriggerConfiguration->relativePath = STRING_clone(config->relativePath);
			result->broker = broker;

			LogInfo("hostName is: %s", STRING_c_str(result->functionsHttpTriggerConfiguration->hostAddress));
			LogInfo("relative path is: %s", STRING_c_str(result->functionsHttpTriggerConfiguration->relativePath));
		}
	}
	return result;
}

/*
* @brief	Destroy an identity map module.
*/
//TODO: CHECK FOR MEMORY LEAKS.
static void FunctionsHttpTrigger_Destroy(MODULE_HANDLE moduleHandle)
{
	if (moduleHandle != NULL)
	{
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
//TODO: PARSE ADDRESS INPUT, CLEAN UP, BUILD PARAMETERS ON THE HTTP REQUEST.
static void FunctionsHttpTrigger_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
	if (moduleHandle == NULL || messageHandle == NULL)
	{
		LogError("Received NULL arguments: module = %p, massage = %p", moduleHandle, messageHandle);
	}
	else
	{
		FUNCTIONS_HTTP_TRIGGER_DATA*idModule = (FUNCTIONS_HTTP_TRIGGER_DATA*)moduleHandle;
		const CONSTBUFFER * content = Message_GetContent(messageHandle);
		STRING_HANDLE contentAsJSON = Base64_Encode_Bytes(content->buffer, content->size);

		//HTTPAPIEX_HANDLE myHTTPEXHandle = HTTPAPIEX_Create(STRING_c_str(idModule->functionsHttpTriggerConfiguration->httpAddress));
		HTTPAPIEX_HANDLE myHTTPEXHandle = HTTPAPIEX_Create(STRING_c_str(idModule->functionsHttpTriggerConfiguration->hostAddress));
		
		unsigned int statuscodeBack;
		HTTP_HEADERS_HANDLE responseHttpHeadersHandle;
		BUFFER_HANDLE myResponse = BUFFER_new();
		
		STRING_HANDLE relativePathInfoForRequest = STRING_clone(idModule->functionsHttpTriggerConfiguration->relativePath);
		STRING_concat(relativePathInfoForRequest, "?name=myGatewayDevice");
		STRING_concat(relativePathInfoForRequest, "&content=");
		STRING_concat(relativePathInfoForRequest, STRING_c_str(contentAsJSON));


		HTTPAPIEX_RESULT requestResult = HTTPAPIEX_ExecuteRequest(myHTTPEXHandle, HTTPAPI_REQUEST_GET, STRING_c_str(relativePathInfoForRequest), NULL, NULL, &statuscodeBack, NULL, myResponse);

		if (requestResult != HTTPAPIEX_OK)
		{
			LogError("Error Sending REquest. Status Code: %d", statuscodeBack);
		}
		else
		{
			LogInfo("Request Sent to Function Succesfully. Response from Functions: %s", BUFFER_u_char(myResponse));
		}
		
		STRING_delete(relativePathInfoForRequest);
		HTTPAPIEX_Destroy(myHTTPEXHandle);
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
MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(FUNCTIONSHTTPTRIGGER_MODULE)(void)
#else
MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void)
#endif
{
	return &FunctionsHttpTrigger_APIS_all;
}
