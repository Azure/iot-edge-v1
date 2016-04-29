// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUBHTTP_H
#define IOTHUBHTTP_H

#include "module.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct IOTHUBHTTP_CONFIG_TAG
{
	const char* IoTHubName;
	const char* IoTHubSuffix;
}IOTHUBHTTP_CONFIG; /*this needs to be passed to the Module_Create function*/

MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(IOTHUBHTTP_MODULE)(void);

#ifdef __cplusplus
}
#endif

#endif /*IOTHUBHTTP_H*/
