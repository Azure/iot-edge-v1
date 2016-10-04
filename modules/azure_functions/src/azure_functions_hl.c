// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/strings.h"
#include "message.h"
#include "parson.h"

#include "azure_functions.h"
#include "azure_functions_hl.h"

/*
 * @brief	Create an Azure Functions HL module.
 */
static MODULE_HANDLE Azure_Functions_HL_Create(BROKER_HANDLE broker, const void* configuration)
{
    MODULE_HANDLE result;
    if(
        (broker == NULL) ||
        (configuration == NULL)
    )
    { 
		/* Codes_SRS_AZUREFUNCTIONS_HL_04_002: [ If broker is NULL then Azure_Functions_HL_Create shall fail and return NULL. ] */
		/* Codes_SRS_AZUREFUNCTIONS_HL_04_003: [ If configuration is NULL then Azure_Functions_HL_Create shall fail and return NULL. ] */
        LogError("NULL parameter detected broker=%p configuration=%p", broker, configuration);
        result = NULL;
    }
    else
    {
		/* Codes_SRS_AZUREFUNCTIONS_HL_04_005: [ Azure_Functions_HL_Create shall parse the configuration as a JSON array of strings. ] */
        JSON_Value* json = json_parse_string((const char*)configuration);
        if (json == NULL)
        {
			/* Codes_SRS_AZUREFUNCTIONS_HL_04_004: [ If configuration is not a JSON string, then Azure_Functions_HL_Create shall fail and return NULL. ] */
            LogError("unable to json_parse_string");
            result = NULL;
        }
        else
        {
            JSON_Object* obj = json_value_get_object(json);
            if (obj == NULL)
            {
				/* Codes_SRS_AZUREFUNCTIONS_HL_04_004: [ If configuration is not a JSON string, then Azure_Functions_HL_Create shall fail and return NULL. ] */
                LogError("unable to json_value_get_object");
                result = NULL;
            }
            else
            {
			    /* Codes_SRS_AZUREFUNCTIONS_HL_04_006: [If the array object does not contain a value named "hostAddress" then Azure_Functions_HL_Create shall fail and return NULL.] */
                const char* hostAddress = json_object_get_string(obj, "hostname");
                if (hostAddress == NULL)
                {
                    LogError("json_object_get_string failed. hostname");
                    result = NULL;
                }
				else
				{
					/* Codes_SRS_AZUREFUNCTIONS_HL_04_007: [ If the array object does not contain a value named "relativePath" then Azure_Functions_HL_Create shall fail and return NULL. ] */
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

						/* Codes_SRS_AZUREFUNCTIONS_HL_04_019: [ If the array object contains a value named "key" then Azure_Functions_HL_Create shall create a securityKey based on input key ] */
						config.securityKey = STRING_construct(key); //Doesn't need to test key. If key is null will mean we are sending an anonymous request.
						config.relativePath = NULL;
						/* Codes_SRS_AZUREFUNCTIONS_HL_04_008: [ Azure_Functions_HL_Create shall call STRING_construct to create hostAddress based on input host address. ] */
						config.hostAddress = STRING_construct(hostAddress);
						

						if (config.hostAddress == NULL)
						{
							/* Codes_SRS_AZUREFUNCTIONS_HL_04_010: [ If creating the strings fails, then Azure_Functions_HL_Create shall fail and return NULL. ] */
							LogError("error buliding hostAddress String.");
							result = NULL;
						}
						else
						{
							/* Codes_SRS_AZUREFUNCTIONS_HL_04_009: [ Azure_Functions_HL_Create shall call STRING_construct to create relativePath based on input host address. ] */
							config.relativePath = STRING_construct(relativePath);
							if (config.relativePath == NULL)
							{
								/* Codes_SRS_AZUREFUNCTIONS_HL_04_010: [ If creating the strings fails, then Azure_Functions_HL_Create shall fail and return NULL. ] */
								LogError("error buliding relative path String.");
								result = NULL;
							}
							else
							{
								/* Codes_SRS_AZUREFUNCTIONS_HL_04_011: [ Azure_Functions_HL_Create shall invoke Azure Functions module's create, passing in the message broker handle and the Azure_Functions_CONFIG. ] */
								MODULE_APIS apis;
								memset(&apis, 0, sizeof(MODULE_APIS));
								MODULE_STATIC_GETAPIS(AZUREFUNCTIONS_MODULE)(&apis);

								/* Codes_SRS_AZUREFUNCTIONS_HL_04_012: [ When the lower layer Azure Functions module create succeeds, Azure_Functions_HL_Create shall succeed and return a non-NULL value. ] */
								result = apis.Module_Create(broker, &config);
							}
						}

						if (result == NULL)
						{
							/* Codes_SRS_AZUREFUNCTIONS_HL_04_013: [ If the lower layer Azure Functions module create fails, Azure_Functions_HL_Create shall fail and return NULL. ] */
							/*return result "as is" - that is - NULL*/
							LogError("unable to create Logger");
						}
						else
						{
							/*return result "as is" - that is - not NULL*/
						}
						/* Codes_SRS_AZUREFUNCTIONS_HL_04_014: [ Azure_Functions_HL_Create shall release all data it allocated. ] */
						STRING_delete(config.hostAddress);
						STRING_delete(config.relativePath);
						STRING_delete(config.securityKey);
					}
                }
            }
			/* Codes_SRS_AZUREFUNCTIONS_HL_04_014: [ Azure_Functions_HL_Create shall release all data it allocated. ] */
            json_value_free(json);
        }
    }
    
    return result;
}

/*
* @brief	Destroy a Azure Functions HL module.
*/
static void Azure_Functions_HL_Destroy(MODULE_HANDLE moduleHandle)
{
	if (moduleHandle != NULL)
	{
		MODULE_APIS apis;
		memset(&apis, 0, sizeof(MODULE_APIS));
		MODULE_STATIC_GETAPIS(AZUREFUNCTIONS_MODULE)(&apis);
		/* Codes_SRS_AZUREFUNCTIONS_HL_04_015: [ Azure_Functions_HL_Destroy shall free all used resources. ] */
		apis.Module_Destroy(moduleHandle);
	}
	else
	{
		/* Codes_SRS_AZUREFUNCTIONS_HL_04_017: [ Azure_Functions_HL_Destroy shall do nothing if moduleHandle is NULL. ] */
		LogError("'module' is NULL");
	}
}


/*
 * @brief	Receive a message from the message broker.
 */
static void Azure_Functions_HL_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
	if (moduleHandle != NULL && messageHandle != NULL)
	{
		MODULE_APIS apis;
		memset(&apis, 0, sizeof(MODULE_APIS));
		MODULE_STATIC_GETAPIS(AZUREFUNCTIONS_MODULE)(&apis);
		/* Codes_SRS_AZUREFUNCTIONS_HL_04_016: [ Azure_Functions_HL_Receive shall pass the received parameters to the underlying identity map module receive function. ] */
		apis.Module_Receive(moduleHandle, messageHandle);
	}
	else
	{
		/* Codes_SRS_AZUREFUNCTIONS_HL_04_018: [ Azure_Functions_HL_Receive shall do nothing if any parameter is NULL. ] */
		LogError("'module' and/or 'message_handle' is NULL");
	}
	
}


/*
 *	Required for all modules:  the public API and the designated implementation functions.
 */
static const MODULE_APIS Azure_functions_HL_APIS_all =
{
	Azure_Functions_HL_Create,
	Azure_Functions_HL_Destroy,
	Azure_Functions_HL_Receive
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT void MODULE_STATIC_GETAPIS(AZUREFUNCTIONS_MODULE_HL)(MODULE_APIS* apis)
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
		/* Codes_SRS_AZUREFUNCTIONS_HL_04_001: [ Module_GetAPIS shall fill the provided MODULE_APIS function with the required function pointers. ] */
		(*apis) = Azure_functions_HL_APIS_all;
	}
}
