// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUB_H
#define IOTHUB_H

#include "module.h"
#include <iothub_client_ll.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct IOTHUB_CONFIG_TAG
{
	const char* IoTHubName;
	const char* IoTHubSuffix;
    IOTHUB_CLIENT_TRANSPORT_PROVIDER transportProvider;
}IOTHUB_CONFIG; /*this needs to be passed to the Module_Create function*/

MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(IOTHUB_MODULE)(void);

#ifdef __cplusplus
}
#endif

#endif /*IOTHUB_H*/
