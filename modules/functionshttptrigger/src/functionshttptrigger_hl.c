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

#include "functionshttptrigger.h"
#include "functionshttptrigger_hl.h"

/*
 * @brief	Create an identity map HL module.
 */
static MODULE_HANDLE Functions_Http_Trigger_HL_Create(BROKER_HANDLE broker, const void* configuration)
{
    MODULE_HANDLE result;
    if(
        (broker == NULL) ||
        (configuration == NULL)
    )
    { 
        LogError("NULL parameter detected broker=%p configuration=%p", broker, configuration);
        result = NULL;
    }
    else
    {
        JSON_Value* json = json_parse_string((const char*)configuration);
        if (json == NULL)
        {
            LogError("unable to json_parse_string");
            result = NULL;
        }
        else
        {
            JSON_Object* obj = json_value_get_object(json);
            if (obj == NULL)
            {
                LogError("unable to json_value_get_object");
                result = NULL;
            }
            else
            {
                const char* hostAddress = json_object_get_string(obj, "hostname");
                if (hostAddress == NULL)
                {
                    LogError("json_object_get_string failed. hostname");
                    result = NULL;
                }
				else
				{
					const char* relativePath = json_object_get_string(obj, "relativePath");
					if (relativePath == NULL)
					{
						LogError("json_object_get_string failed. relativePath.");
						result = NULL;
					}
					else
					{
						FUNCTIONS_HTTP_TRIGGER_CONFIG config;
						config.hostAddress = STRING_construct(hostAddress);

						if (config.hostAddress == NULL)
						{
							LogError("error buliding hostAddress String.");
							result = NULL;
						}
						else
						{
							config.relativePath = STRING_construct(relativePath);
							if (config.relativePath == NULL)
							{
								LogError("error buliding relative path String.");
								result = NULL;
							}

						}						

						result = MODULE_STATIC_GETAPIS(FUNCTIONSHTTPTRIGGER_MODULE)()->Module_Create(broker, &config);

						if (result == NULL)
						{
							/*return result "as is" - that is - NULL*/
							LogError("unable to create Logger");
						}
						else
						{
							/*return result "as is" - that is - not NULL*/
						}
						STRING_delete(config.hostAddress);
						STRING_delete(config.relativePath);
					}
                }
            }
            json_value_free(json);
        }
    }
    
    return result;
}

/*
* @brief	Destroy an identity map HL module.
*/
static void Functions_Http_Trigger_HL_Destroy(MODULE_HANDLE moduleHandle)
{
	MODULE_STATIC_GETAPIS(FUNCTIONSHTTPTRIGGER_MODULE)()->Module_Destroy(moduleHandle);
}


/*
 * @brief	Receive a message from the message broker.
 */
static void Functions_Http_Trigger_HL_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
	MODULE_STATIC_GETAPIS(FUNCTIONSHTTPTRIGGER_MODULE)()->Module_Receive(moduleHandle, messageHandle);
}


/*
 *	Required for all modules:  the public API and the designated implementation functions.
 */
static const MODULE_APIS FunctionsHttpTrigger_HL_APIS_all =
{
	Functions_Http_Trigger_HL_Create,
	Functions_Http_Trigger_HL_Destroy,
	Functions_Http_Trigger_HL_Receive
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(FUNCTIONSHTTPTRIGGER_MODULE_HL)(void)
#else
MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void)
#endif
{
	return &FunctionsHttpTrigger_HL_APIS_all;
}
