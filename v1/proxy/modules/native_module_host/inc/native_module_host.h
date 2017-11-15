// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#ifndef OOP_MODULE_HOST_H
#define OOP_MODULE_HOST_H

#include "module.h"

#define OOP_MODULE_LOADERS_ARRAY_KEY "outprocess.loaders"
#define OOP_MODULE_LOADER_KEY "outprocess.loader"
#define OOP_MODULE_ARGS_KEY "module.args"

#ifdef __cplusplus
extern "C"
{
#endif

MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(OOP_MODULE_HOST)(MODULE_API_VERSION gateway_api_version);

#ifdef __cplusplus
}
#endif

#endif //OOP_MODULE_HOST_H
