// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef JAVA_LOADER_H
#define JAVA_LOADER_H

#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/umock_c_prod.h"

#include "module.h"
#include "module_loader.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define JAVA_LOADER_NAME            "java"

#if WIN32
#define JAVA_BINDING_MODULE_NAME    "java_module_host.dll"
#else
#define JAVA_BINDING_MODULE_NAME    "libjava_module_host.so"
#endif

MOCKABLE_FUNCTION(, const MODULE_LOADER*, JavaLoader_Get);

#ifdef __cplusplus
}
#endif

#endif // JAVA_LOADER_H
