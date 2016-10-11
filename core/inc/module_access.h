// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file       module_access.h
*    @brief     Accessing module API functions
*
*    @details   Convenience macros for accessing module API functions.
*/

#ifndef MODULE_ACCESS_H
#define MODULE_ACCESS_H

#include "module.h"


#ifdef __cplusplus
extern "C"
{
#endif

/** @brief macro to get the Module_Create from a MODULES_API pointer */
#define MODULE_CREATE_FROM_JSON(module_api_ptr) ( ((const MODULE_API_1*)(module_api_ptr))->Module_CreateFromJson)
/** @brief macro to get the Module_Create from a MODULES_API pointer */
#define MODULE_CREATE(module_api_ptr) (((const MODULE_API_1*)(module_api_ptr))->Module_Create)
/** @brief macro to get the Module_Destroy from a MODULES_API pointer */
#define MODULE_DESTROY(module_api_ptr) (((const MODULE_API_1*)(module_api_ptr))->Module_Destroy)
/** @brief macro to get the Module_Start from a MODULES_API pointer */
#define MODULE_START(module_api_ptr) (((const MODULE_API_1*)(module_api_ptr))->Module_Start)
/** @brief macro to get the Module_Receive from a MODULES_API pointer */
#define MODULE_RECEIVE(module_api_ptr) (((const MODULE_API_1*)(module_api_ptr))->Module_Receive)

#ifdef __cplusplus
}
#endif

#endif // MODULE_ACCESS_H
