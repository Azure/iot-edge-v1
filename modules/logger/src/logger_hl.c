// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "azure_c_shared_utility/gballoc.h"

/*because it is linked statically, this include will bring in some uniquely (by convention) named functions*/
#include "logger.h"

#include "logger_hl.h"
#include "azure_c_shared_utility/xlogging.h"
#include "parson.h"

static MODULE_HANDLE Logger_HL_Create(MESSAGE_BUS_HANDLE busHandle, const void* configuration)
{
    
    MODULE_HANDLE result;
    /*Codes_SRS_LOGGER_HL_02_001: [ If busHandle is NULL then Logger_HL_Create shall fail and return NULL. ]*/
    /*Codes_SRS_LOGGER_HL_02_003: [ If configuration is NULL then Logger_HL_Create shall fail and return NULL. ]*/
    if(
        (busHandle == NULL) ||
        (configuration == NULL)
    )
    { 
        LogError("NULL parameter detected busHandle=%p configuration=%p", busHandle, configuration);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_LOGGER_HL_02_011: [ If configuration is not a JSON object, then Logger_HL_Create shall fail and return NULL. ]*/
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
                /*Codes_SRS_LOGGER_HL_02_012: [ If the JSON object does not contain a value named "filename" then Logger_HL_Create shall fail and return NULL. ]*/
                const char* fileNameValue = json_object_get_string(obj, "filename");
                if (fileNameValue == NULL)
                {
                    LogError("json_object_get_string failed");
                    result = NULL;
                }
                else
                {
                    /*fileNameValue is believed at this moment to be a string that might point to a filename on the system*/
                    
                    LOGGER_CONFIG config;
                    config.selector = LOGGING_TO_FILE;
                    config.selectee.loggerConfigFile.name = fileNameValue;
                    
                    /*Codes_SRS_LOGGER_HL_02_005: [ Logger_HL_Create shall pass busHandle and the filename to Logger_Create. ]*/
                    result = MODULE_STATIC_GETAPIS(LOGGER_MODULE)()->Module_Create(busHandle, &config);

                    if (result == NULL)
                    {
                        /*Codes_SRS_LOGGER_HL_02_007: [ If Logger_Create fails then Logger_HL_Create shall fail and return NULL. ]*/
                        /*return result "as is" - that is - NULL*/
                        LogError("unable to create Logger");
                    }
                    else
                    {
                        /*Codes_SRS_LOGGER_HL_02_006: [ If Logger_Create succeeds then Logger_HL_Create shall succeed and return a non-NULL value. ]*/
                        /*return result "as is" - that is - not NULL*/
                    }
                }
            }
            json_value_free(json);
        }
    }
    
    return result;
}

static void Logger_HL_Destroy(MODULE_HANDLE module)
{
    /*Codes_SRS_LOGGER_HL_02_009: [ Logger_HL_Destroy shall destroy all used resources. ]*/ /*in this case "all" is "none"*/
    MODULE_STATIC_GETAPIS(LOGGER_MODULE)()->Module_Destroy(module);
}

static void Logger_HL_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    /*Codes_SRS_LOGGER_HL_02_008: [ Logger_HL_Receive shall pass the received parameters to the underlying Logger's _Receive function. ]*/
    MODULE_STATIC_GETAPIS(LOGGER_MODULE)()->Module_Receive(moduleHandle, messageHandle);
}

/*
 *	Required for all modules:  the public API and the designated implementation functions.
 */
static const MODULE_APIS Logger_HL_APIS_all =
{
	Logger_HL_Create,
	Logger_HL_Destroy,
	Logger_HL_Receive
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(LOGGER_MODULE_HL)(void)
#else
MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void)
#endif
{
    /*Codes_SRS_LOGGER_HL_02_010: [ Module_GetAPIS shall return a non-NULL pointer to a structure of type MODULE_APIS that has all fields non-NULL. ]*/
	return &Logger_HL_APIS_all;
}
