// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file       outprocess_loader.h
 *  @brief      Library for loading proxy module for an out of process module.
 *
 *  @details    The gateway SDK uses this module loader library to load a proxy 
 *              module.
 */

#ifndef OUTPROCESS_LOADER_H
#define OUTPROCESS_LOADER_H

#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/umock_c_prod.h"

#include "module.h"
#include "module_loader.h"
#include "gateway_export.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define OUTPROCESS_LOADER_NAME "outprocess"

#define OUTPROCESS_LOADER_ACTIVATION_TYPE_VALUES \
    OUTPROCESS_LOADER_ACTIVATION_NONE

/**
 * @brief Enumeration listing all supported module loaders
 */
DEFINE_ENUM(OUTPROCESS_LOADER_ACTIVATION_TYPE, OUTPROCESS_LOADER_ACTIVATION_TYPE_VALUES);

/** @brief Structure to load an out of process proxy module */
typedef struct OUTPROCESS_LOADER_ENTRYPOINT_TAG
{
    /** 
     * @brief Tells the module and loader how much control the gateway 
     * has over the module host process.
     */
    OUTPROCESS_LOADER_ACTIVATION_TYPE activation_type;
    /** @brief The URI for the module host control channel.*/
    STRING_HANDLE control_id;
    /** @brief The URI for the gateway message channel.*/
    STRING_HANDLE message_id;
} OUTPROCESS_LOADER_ENTRYPOINT;

/** @brief      The API for the out of process proxy module loader. */
MOCKABLE_FUNCTION(, GATEWAY_EXPORT const MODULE_LOADER*, OutprocessLoader_Get);

#ifdef __cplusplus
}
#endif

#endif // OUTPROCESS_LOADER_H
