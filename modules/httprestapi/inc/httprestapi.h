// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef _HTTP_REST_API_H
#define _HTTP_REST_API_H

#include "module.h"

#ifdef __cplusplus
extern "C"
{
#endif

MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(_HTTP_REST_API_MODULE)(MODULE_API_VERSION gateway_api_version);

#ifdef __cplusplus
}
#endif

#define MSG_KEY_SOURCE "source"
#define MSG_VALUE_SOURCE "httprestapi"
#define MSG_KEY_MAC_ADDRESS "macAddress"


#endif /*_HTTP_REST_API_H*/