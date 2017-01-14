// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUBDEVICEPING_H
#define IOTHUBDEVICEPING_H

#include "module.h"
#include <iothub_client_ll.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct IOTHUBDEVICEPING_CONFIG_TAG
{
    const char *DeviceConnectionString;
    const char *EH_HOST;
    const char *EH_KEY_NAME;
    const char *EH_KEY;
    const char *EH_COMP_NAME;
    const char *EH_PARTITION_NUM;
    const char *DataProtocol;
} IOTHUBDEVICEPING_CONFIG; /*this needs to be passed to the Module_Create function*/

typedef struct BINARY_DATA_TAG
{
    const unsigned char *bytes;
    size_t length;
} BINARY_DATA;

MODULE_EXPORT void MODULE_STATIC_GETAPIS(IOTHUBDEVICEPING_MODULE)(MODULE_APIS *apis);

int message_get_body_amqp_data(MESSAGE_HANDLE message, size_t index, BINARY_DATA *binary_data);

#ifdef __cplusplus
}
#endif

#endif /*IOTHUBDEVICEPING_H*/
