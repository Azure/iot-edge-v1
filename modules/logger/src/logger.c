// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "azure_c_shared_utility/gballoc.h"

#include "azure_c_shared_utility/gb_stdio.h"
#include "azure_c_shared_utility/gb_time.h"

#include "logger.h"

#include <errno.h>

#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/base64.h"
#include "azure_c_shared_utility/map.h"
#include "azure_c_shared_utility/constmap.h"

typedef struct LOGGER_HANDLE_DATA_TAG
{
    FILE* fout;
}LOGGER_HANDLE_DATA;

/*this function adds a JSON object to the output*/
/*function assumes the file already has a json array in it. and that the last character is ] (when the first json is appended) or , when subsequent jsons are appended*/
static int addJSONString(FILE* fout, const char* jsonString)
{
    int result;
    if (fseek(fout, -1, SEEK_END) != 0) /*rewind 1 character*/ /*What is this in C standard... "A binary stream need not meaningfully support fseek calls with a whence value of SEEK_END."???*/
    {
        LogError("unable to fseek");
        result = __LINE__;
    }
    else
    {
        if (fprintf(fout, "%s", jsonString) < 0)
        {
            LogError("fprintf failed");
            result = __LINE__;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}



static int LogStartStop_Print(char* destination, size_t destinationSize, bool appendStart, bool isAbsoluteStart)
{
    int result;
    time_t temp = time(NULL);
    if (temp == (time_t)-1)
    {
        LogError("unable to time(NULL)");
        result = __LINE__;
    }
    else
    {
        struct tm* t = localtime(&temp);
        if (t == NULL)
        {
            LogError("localtime failed");
            result = __LINE__;
        }
        else
        {
            const char* format = appendStart ?
                (isAbsoluteStart?"{\"time\":\"%c\",\"content\":\"Log started\"}]" : ",{\"time\":\"%c\",\"content\":\"Log started\"}]"):
                ",{\"time\":\"%c\",\"content\":\"Log stopped\"}]";
            if (strftime(destination, destinationSize, format, t) == 0) 
            {
                LogError("unable to strftime");
                result = __LINE__;
            }
            else
            {
                result = 0;
            }
        }
    }
    return result;
}

static int append_logStartStop(FILE* fout, bool appendStart, bool isAbsoluteStart)
{
    int result;
    /*Codes_SRS_LOGGER_02_017: [Logger_Create shall add the following JSON value to the existing array of JSON values in the file:]*/
    char temp[80] = { 0 };
    if (LogStartStop_Print(temp, sizeof(temp) / sizeof(temp[0]), appendStart, isAbsoluteStart) != 0 )
    {
        LogError("unable to create start/stop time json string");
        result = __LINE__;
    }
    else
    {
        if (addJSONString(fout, temp) != 0)
        {
            LogError("internal error in addJSONString");
            result = __LINE__;
        }
        else
        {
            result = 0; /*all is fine*/
        }
    }
    return result;
}

static MODULE_HANDLE Logger_Create(MESSAGE_BUS_HANDLE busHandle, const void* configuration)
{
    LOGGER_HANDLE_DATA* result;
    /*Codes_SRS_LOGGER_02_001: [If busHandle is NULL then Logger_Create shall fail and return NULL.]*/
    /*Codes_SRS_LOGGER_02_002: [If configuration is NULL then Logger_Create shall fail and return NULL.]*/
    if (
        (busHandle == NULL) ||
        (configuration == NULL) 
        )
    {
        LogError("invalid arg busHandle=%p configuration=%p", busHandle, configuration);
        result = NULL;
    }
    else
    {
        const LOGGER_CONFIG* config = configuration;
        /*Codes_SRS_LOGGER_02_003: [If configuration->selector has a value different than LOGGING_TO_FILE then Logger_Create shall fail and return NULL.]*/
        if (config->selector != LOGGING_TO_FILE)
        {
            LogError("invalid arg config->selector=%d", config->selector);
            result = NULL;
        }
        else
        {
            /*Codes_SRS_LOGGER_02_004: [If configuration->selectee.loggerConfigFile.name is NULL then Logger_Create shall fail and return NULL.]*/
            if (config->selectee.loggerConfigFile.name == NULL)
            {
                LogError("invalid arg config->selectee.loggerConfigFile.name=NULL");
                result = NULL;
            }
            else
            {
                /*Codes_SRS_LOGGER_02_005: [Logger_Create shall allocate memory for the below structure.]*/
                result = malloc(sizeof(LOGGER_HANDLE_DATA));
                /*Codes_SRS_LOGGER_02_007: [If Logger_Create encounters any errors while creating the LOGGER_HANDLE_DATA then it shall fail and return NULL.]*/
                if (result == NULL)
                {
                    LogError("malloc failed");
                    /*return as is*/
                }
                else
                {
                    /*Codes_SRS_LOGGER_02_006: [Logger_Create shall open the file configuration the filename selectee.loggerConfigFile.name in update (reading and writing) mode and assign the result of fopen to fout field. ]*/
                    result->fout = fopen(config->selectee.loggerConfigFile.name, "r+b"); /*open binary file for update (reading and writing)*/
                    if (result->fout == NULL) 
                    {
                        /*if the file does not exist [or other error, indistinguishable here] try to create it*/
                        /*Codes_SRS_LOGGER_02_020: [If the file selectee.loggerConfigFile.name does not exist, it shall be created.]*/
                        result->fout = fopen(config->selectee.loggerConfigFile.name, "w+b");/*truncate to zero length or create binary file for update*/
                    }

                    /*Codes_SRS_LOGGER_02_007: [If Logger_Create encounters any errors while creating the LOGGER_HANDLE_DATA then it shall fail and return NULL.]*/
                    /*Codes_SRS_LOGGER_02_021: [If creating selectee.loggerConfigFile.name fails then Logger_Create shall fail and return NULL.]*/
                    if (result->fout == NULL)
                    {
                        LogError("unable to open file %s", config->selectee.loggerConfigFile.name);
                        free(result);
                        result = NULL;
                    }
                    else
                    {
                        /*Codes_SRS_LOGGER_02_018: [If the file does not contain a JSON array, then it shall create it.]*/
                        if (fseek(result->fout, 0, SEEK_END) != 0)
                        {
                            /*Codes_SRS_LOGGER_02_007: [If Logger_Create encounters any errors while creating the LOGGER_HANDLE_DATA then it shall fail and return NULL.]*/
                            LogError("unable to fseek to end of file");
                            if (fclose(result->fout) != 0)
                            {
                                LogError("unable to close file %s", config->selectee.loggerConfigFile.name);
                            }
                            free(result);
                            result = NULL;
                        }
                        else
                        {
                            /*the verifications here are weak, content of file is not verified*/
                            errno = 0;
                            long int fileSize;
                            if ((fileSize = ftell(result->fout)) == -1L)
                            {
                                /*Codes_SRS_LOGGER_02_007: [If Logger_Create encounters any errors while creating the LOGGER_HANDLE_DATA then it shall fail and return NULL.]*/
                                LogError("unable to ftell (errno=%d)", errno);
                                if (fclose(result->fout) != 0)
                                {
                                    LogError("unable to close file %s", config->selectee.loggerConfigFile.name);
                                }
                                free(result);
                                result = NULL;
                            }
                            else
                            {
                                /*Codes_SRS_LOGGER_02_018: [If the file does not contain a JSON array, then it shall create it.]*/
                                if (fileSize == 0)
                                {
                                    /*Codes_SRS_LOGGER_02_018: [If the file does not contain a JSON array, then it shall create it.]*/
                                    if (fprintf(result->fout, "[]") < 0) /*add an empty array of JSONs to the output file*/
                                    {
                                        /*Codes_SRS_LOGGER_02_007: [If Logger_Create encounters any errors while creating the LOGGER_HANDLE_DATA then it shall fail and return NULL.]*/
                                        LogError("unable to write to output file");
                                        if (fclose(result->fout) != 0)
                                        {
                                            LogError("unable to close file %s", config->selectee.loggerConfigFile.name);
                                        }
                                        free(result);
                                        result = NULL;
                                    }
                                    else
                                    {
                                        /*Codes_SRS_LOGGER_02_017: [Logger_Create shall add the following JSON value to the existing array of JSON values in the file:]*/
                                        if (append_logStartStop(result->fout, true, true) != 0)
                                        {
                                            LogError("append_logStartStop failed");
                                            if (fclose(result->fout) != 0)
                                            {
                                                LogError("unable to close file %s", config->selectee.loggerConfigFile.name);
                                            }
                                            free(result);
                                            result = NULL;
                                        }
                                        /*Codes_SRS_LOGGER_02_008: [Otherwise Logger_Create shall return a non-NULL pointer.]*/
                                        /*that is, return as is*/
                                    }
                                }
                                else
                                {
                                    /*Codes_SRS_LOGGER_02_017: [Logger_Create shall add the following JSON value to the existing array of JSON values in the file:]*/
                                    if (append_logStartStop(result->fout, true, false) != 0)
                                    {
                                        LogError("append_logStartStop failed");
                                    }
                                    /*Codes_SRS_LOGGER_02_008: [Otherwise Logger_Create shall return a non-NULL pointer.]*/
                                    /*that is, return as is*/
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return result;
}

static void Logger_Destroy(MODULE_HANDLE module)
{
    /*Codes_SRS_LOGGER_02_014: [If moduleHandle is NULL then Logger_Destroy shall return.]*/
    if (module != NULL)
    {
        /*Codes_SRS_LOGGER_02_019: [Logger_Destroy shall add to the log file the following end of log JSON object:]*/
        LOGGER_HANDLE_DATA* moduleHandleData = (LOGGER_HANDLE_DATA *)module;
        if (append_logStartStop(moduleHandleData->fout, false, false) != 0)
        {
            LogError("unable to append log ending time");
        }

        /*Codes_SRS_LOGGER_02_015: [Otherwise Logger_Destroy shall unuse all used resources.]*/
        if (fclose(moduleHandleData->fout) != 0)
        {
            LogError("unable to fclose");
        }

        free(moduleHandleData);

    }
}

static void Logger_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    /*Codes_SRS_LOGGER_02_009: [If moduleHandle is NULL then Logger_Receive shall fail and return.]*/
    /*Codes_SRS_LOGGER_02_010: [If messageHandle is NULL then Logger_Receive shall fail and return.]*/
    if (
        (moduleHandle == NULL) ||
        (messageHandle == NULL)
        )
    {
        LogError("invalid arg moduleHandle = %p", moduleHandle);
    }
    else
    {
        /*Codes_SRS_LOGGER_02_011: [Logger_Receive shall write in the fout FILE the following information in JSON format:]*/
        /*
        {
            "time":"timeAsPrinted by strftime(\"%c\")",
            "properties":
            {
                "property1":"value1",
                "property2":"value2"
            },
            "content":"base64 encode of the message content"
        },
        */

        /*the function will gather first all the values then will dump them into a STRING_HANDLE that is JSON*/

        /*getting the time*/
        time_t temp = time(NULL);
        if (temp == (time_t)-1)
        {
            LogError("time function failed");
        }
        else
        {
            struct tm* t = localtime(&temp);
            if (t == NULL)
            {
                LogError("localtime failed");
            }
            else
            {
                char timetemp[80] = { 0 };
                if (strftime(timetemp, sizeof(timetemp) / sizeof(timetemp[0]), "%c", t) == 0)
                {
                    LogError("unable to strftime");
                    /*Codes_SRS_LOGGER_02_012: [If producing the JSON format or writing it to the file fails, then Logger_Receive shall fail and return.]*/
                    /*just return*/
                }
                else
                {
                    /*getting the properties*/
                    /*getting the constmap*/
                    CONSTMAP_HANDLE originalProperties = Message_GetProperties(messageHandle); /*by contract this is never NULL*/
                    MAP_HANDLE propertiesAsMap = ConstMap_CloneWriteable(originalProperties); /*sigh, if only there'd be a constmap_tojson*/
                    if (propertiesAsMap == NULL)
                    {
                        LogError("ConstMap_CloneWriteable failed");
                    }
                    else
                    {
                        STRING_HANDLE jsonProperties = Map_ToJSON(propertiesAsMap);
                        if (jsonProperties == NULL)
                        {
                            LogError("unable to Map_ToJSON");
                        }
                        else
                        {
                            /*getting the base64 encode of the message*/
                            const CONSTBUFFER * content = Message_GetContent(messageHandle); /*by contract, this is never NULL*/
                            STRING_HANDLE contentAsJSON = Base64_Encode_Bytes(content->buffer, content->size);
                            if (contentAsJSON == NULL)
                            {
                                LogError("unable to Base64_Encode_Bytes");
                            }
                            else
                            {
                                STRING_HANDLE jsonToBeAppended = STRING_construct(",{\"time\":\"");
                                if (jsonToBeAppended == NULL)
                                {
                                    LogError("unable to STRING_construct");
                                }
                                else
                                {

                                    if (!(
                                        (STRING_concat(jsonToBeAppended, timetemp) == 0) &&
                                        (STRING_concat(jsonToBeAppended, "\",\"properties\":") == 0) &&
                                        (STRING_concat_with_STRING(jsonToBeAppended, jsonProperties) == 0) &&
                                        (STRING_concat(jsonToBeAppended, ",\"content\":\"") == 0) &&
                                        (STRING_concat_with_STRING(jsonToBeAppended, contentAsJSON) == 0) &&
                                        (STRING_concat(jsonToBeAppended, "\"}]") == 0)
                                        ))
                                    {
                                        LogError("STRING concatenation error");
                                    }
                                    else
                                    {
                                        LOGGER_HANDLE_DATA *handleData = (LOGGER_HANDLE_DATA *)moduleHandle;
                                        if (addJSONString(handleData->fout, STRING_c_str(jsonToBeAppended)) != 0)
                                        {
                                            LogError("failed top add a json string to the output file");
                                        }
                                        else
                                        {
                                            /*all seems fine*/
                                        }
                                    }
                                    STRING_delete(jsonToBeAppended);
                                }
                                STRING_delete(contentAsJSON);
                            }
                            STRING_delete(jsonProperties);
                        }
                        Map_Destroy(propertiesAsMap);
                    }
                    ConstMap_Destroy(originalProperties);
                }
            }
        }
    }
    /*Codes_SRS_LOGGER_02_013: [Logger_Receive shall return.]*/
}

/*
 *	Required for all modules:  the public API and the designated implementation functions.
 */
static const MODULE_APIS Logger_APIS_all =
{
    Logger_Create,
    Logger_Destroy,
    Logger_Receive
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(LOGGER_MODULE)(void)
#else
MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void)
#endif
{
    /*Codes_SRS_LOGGER_02_016: [Module_GetAPIS shall return a non-NULL pointer to a structure of type MODULE_APIS that has all fields non-NULL.]*/
    return &Logger_APIS_all;
}
