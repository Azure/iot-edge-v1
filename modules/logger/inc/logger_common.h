// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef LOGGER_COMMON_H
#define LOGGER_COMMON_H

/*the below data structures are used by all versions of Logger (static/dynamic vanilla/hl)*/
typedef enum LOGGER_TYPE_TAG
{
    LOGGING_TO_FILE
}LOGGER_TYPE;

typedef struct LOGGER_CONFIG_TAG
{
    LOGGER_TYPE selector;
    union 
    {
        struct LOGGER_CONFIG_FILE_TAG
        {
            const char* name;
        } loggerConfigFile;
    }selectee;
}LOGGER_CONFIG; /*this needs to be passed to the Module_Create function*/

#endif /*LOGGER_COMMON_H*/
