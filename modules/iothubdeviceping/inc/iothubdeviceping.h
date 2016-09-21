// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUBDEVICEPING_H
#define IOTHUBDEVICEPING_H

#include "module.h"
#ifdef linux
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct IOTHUBDEVICEPING_CONFIG_TAG
{
	const char* DeviceConnectionString;
}IOTHUBDEVICEPING_CONFIG; /*this needs to be passed to the Module_Create function*/

MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(IOTHUBDEVICEPING_MODULE)(void);

#ifdef __cplusplus
}
#endif

#endif /*IOTHUBDEVICEPING_H*/
