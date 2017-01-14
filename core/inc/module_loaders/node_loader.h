// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef NODE_LOADER_H
#define NODE_LOADER_H

#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/umock_c_prod.h"

#include "module.h"
#include "module_loader.h"
#include "gateway_export.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define NODE_LOADER_NAME            "node"

#if WIN32
#define NODE_BINDING_MODULE_NAME    "nodejs_binding.dll"
#else
#define NODE_BINDING_MODULE_NAME    "libnodejs_binding.so"
#endif

typedef struct NODE_LOADER_ENTRYPOINT_TAG
{
    STRING_HANDLE mainPath;
} NODE_LOADER_ENTRYPOINT;

MOCKABLE_FUNCTION(, GATEWAY_EXPORT const MODULE_LOADER*, NodeLoader_Get);

#ifdef __cplusplus
}
#endif

#endif // NODE_LOADER_H
