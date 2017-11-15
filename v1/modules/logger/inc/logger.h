// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef LOGGER_H
#define LOGGER_H

#include "module.h"

typedef enum LOGGER_TYPE_TAG
{
    LOGGING_TO_FILE
} LOGGER_TYPE;

typedef struct LOGGER_CONFIG_TAG
{
    LOGGER_TYPE selector;
    union
    {
        struct LOGGER_CONFIG_FILE_TAG
        {
            const char * name;
        } loggerConfigFile;
    } selectee;
} LOGGER_CONFIG; /*this needs to be passed to the Module_Create function*/

#ifdef __cplusplus
extern "C"
{
#endif

MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(LOGGER_MODULE)(MODULE_API_VERSION gateway_api_version);

#ifdef __cplusplus
}
#endif

#endif /*LOGGER_H*/
