// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef NODE_LOADER_H
#define NODE_LOADER_H

#include "azure_c_shared_utility/strings.h"

#include "module.h"
#include "module_loader.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define NODE_LOADER_NAME            "NODE"

#if WIN32
#define NODE_BINDING_MODULE_NAME    "nodejs_binding.dll"
#elif __linux__
#define NODE_BINDING_MODULE_NAME    "libnodejs_binding.so"
#error Cannot build a default binding module name for your platform.
#endif

typedef struct NODE_LOADER_ENTRYPOINT_TAG
{
    STRING_HANDLE mainPath;
} NODE_LOADER_ENTRYPOINT;

extern const MODULE_LOADER* NodeLoader_Get(void);

#ifdef __cplusplus
}
#endif

#endif // NODE_LOADER_H
