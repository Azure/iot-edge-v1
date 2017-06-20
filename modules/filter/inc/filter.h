// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef _FILTER_H
#define _FILTER_H

#include "module.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct FILTER_CONFIG_TAG
{
    int avaliables;
    char* macAddress;
    char* resolverlibrary;
    void* nextEntry;
} FILTER_CONFIG;

#ifdef __cplusplus
}
#endif

#endif /*_FILTER_H*/
