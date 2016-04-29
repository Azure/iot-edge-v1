// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef E2EMODULE_H
#define E2EMODULE_H

/*including module.h dictates that MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void); is implemented by the module*/
#include "module.h"

typedef struct E2EMODULE_CONFIG
{
	const char* macAddress;
	const char* sendData;
}E2EMODULE_CONFIG;

#endif /*E2EMODULE_H*/
