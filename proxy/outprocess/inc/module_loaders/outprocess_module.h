// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file       outprocess_module.h
 *  @brief      The proxy module for an out of process module.
 *
 *  @details    The gateway SDK uses this module as a proxy for a remote module.
 */
#ifndef OUTPROCESS_MODULE_H
#define OUTPROCESS_MODULE_H

#include "module.h"
#include "azure_c_shared_utility/macro_utils.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define OUTPROCESS_MODULE_LIFECYCLE_VALUES \
	OUTPROCESS_LIFECYCLE_ASYNC, \
	OUTPROCESS_LIFECYCLE_SYNC

DEFINE_ENUM(OUTPROCESS_MODULE_LIFECYCLE, OUTPROCESS_MODULE_LIFECYCLE_VALUES);

/** @brief Structure to configure an out of process proxy module */
typedef struct OUTPROCESS_MODULE_CONFIG_DATA
{
	OUTPROCESS_MODULE_LIFECYCLE lifecycle_model;
    /** @brief The URI for the module host control channel.*/
    STRING_HANDLE control_uri;
    /** @brief The URI for the gateway message channel.*/
    STRING_HANDLE message_uri;
    /** @brief Arguments for the module .*/
    STRING_HANDLE outprocess_module_args;
} OUTPROCESS_MODULE_CONFIG;

/** @brief the API fr this module */
extern const MODULE_API_1 Outprocess_Module_API_all;

#ifdef __cplusplus
}
#endif

#endif /*OUTPROCESS_MODULE_H*/