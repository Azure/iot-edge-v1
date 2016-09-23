// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef FUNCTIONSHTTPTRIGGER_H
#define FUNCTIONSHTTPTRIGGER_H

#include "module.h"
#include "azure_c_shared_utility\strings.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct FUNCTIONS_HTTP_TRIGGER_CONFIG_TAG
{
    STRING_HANDLE hostAddress;
	STRING_HANDLE relativePath;
} FUNCTIONS_HTTP_TRIGGER_CONFIG;

MODULE_EXPORT void MODULE_STATIC_GETAPIS(FUNCTIONSHTTPTRIGGER_MODULE)(MODULE_APIS* apis);

#ifdef __cplusplus
}
#endif

#endif /*FUNCTIONSHTTPTRIGGER_H*/
