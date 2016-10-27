// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file       dynamic_loader.h
 *  @brief      Library for loading gateway modules from dynamically-linked
 *              libraries (Windows) or shared objects (Linux).
 *
 *  @details    The gateway SDK uses this module loader library by default to
 *              load modules from DLLs or SOs.
 */

#ifndef DYNAMIC_LOADER_H
#define DYNAMIC_LOADER_H

#include "azure_c_shared_utility/strings.h"

#include "module.h"
#include "module_loader.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define DYNAMIC_LOADER_NAME "native"

#if WIN32
#define NATIVE_BINDING_MODULE_NAME    "native_binding.dll"
#elif __linux__
#define NATIVE_BINDING_MODULE_NAME    "libnative_binding.so"
#else
#error Cannot build a default binding module name for your platform.
#endif

/** @brief Structure to load a dynamically linked module */
typedef struct DYNAMIC_LOADER_ENTRYPOINT_TAG
{
    /** @brief file name, path to shared library */
    STRING_HANDLE moduleLibraryFileName;
} DYNAMIC_LOADER_ENTRYPOINT;

/** @brief      The API for the dynamically linked module loader. */
extern const MODULE_LOADER* DynamicLoader_Get(void);

#ifdef __cplusplus
}
#endif

#endif // DYNAMIC_LOADER_H
