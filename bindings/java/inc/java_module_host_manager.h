// Copyright (c) Microsoft. All rights reserved.
// Licensed under MIT license. See LICENSE file in the project root for full license information.

/**	@file		java_module_host_manager.h
 *	@brief		A library to safely manage the number of Java module hosts on
 *				the gateway.
 *	@details	This library uses the @c LOCK API to safely manage the number of
 *				Java module hosts on the gateway. This library will provide a 
 *				singleton instance of a @c JAVA_MODULE_HOST_MANAGER_HANDLE.
 */

#ifndef JAVA_MODULE_HOST_MANAGER_H
#define JAVA_MODULE_HOST_MANAGER_H

#include "azure_c_shared_utility/umock_c_prod.h"
#include "java_module_host.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define JAVA_MODULE_HOST_MANAGER_RESULT_VALUES \
	MANAGER_OK, \
	MANAGER_ERROR \


typedef struct JAVA_MODULE_HOST_MANAGER_DATA_TAG* JAVA_MODULE_HOST_MANAGER_HANDLE;

/** @brief	Enumeration specifying the result of the JavaModuleHostManager 
 *			operation
 */
DEFINE_ENUM(JAVA_MODULE_HOST_MANAGER_RESULT, JAVA_MODULE_HOST_MANAGER_RESULT_VALUES);

/**
 * @brief	This API creates and returns a valid singleton JavaModuleHostManager
 *			handle.
 *
 * @return	A valid @c JAVA_MODULE_HOST_MANAGER_HANDLE when successful or @c 
 *			NULL otherwise.
 */
MOCKABLE_FUNCTION(, JAVA_MODULE_HOST_MANAGER_HANDLE, JavaModuleHostManager_Create, JAVA_MODULE_HOST_CONFIG*, config);

/**
 * @brief	This API destroys the @c JAVA_MODULE_HOST_MANAGER_HANDLE if the 
 *			number of Java module hosts is 0. If the handle is @c NULL this
 *			function will do nothing.
 *
 * @param	handle	A valid handle to the JavaModuleHostManager.
 */
MOCKABLE_FUNCTION(, void, JavaModuleHostManager_Destroy, JAVA_MODULE_HOST_MANAGER_HANDLE, handle);

/**
 * @brief	This API safely increments the count of Java module hosts.
 *
 * @param	handle	A valid handle to the JavaModuleHostManager
 *
 * @return	Returns @c MANAGER_OK upon success, and @c MANAGER_ERROR on error.
 */
MOCKABLE_FUNCTION(, JAVA_MODULE_HOST_MANAGER_RESULT, JavaModuleHostManager_Add, JAVA_MODULE_HOST_MANAGER_HANDLE, handle);

/**
* @brief	This API safely decrements the count of Java module hosts.
*
* @param	handle	A valid handle to the JavaModuleHostManager
*
* @return	Returns @c MANAGER_OK upon success, and @c MANAGER_ERROR on error.
*/
MOCKABLE_FUNCTION(, JAVA_MODULE_HOST_MANAGER_RESULT, JavaModuleHostManager_Remove, JAVA_MODULE_HOST_MANAGER_HANDLE, handle);

/**
 * @brief	This API retreives the number of Java module hosts.
 *
 * @param	handle	A valid handle to the JavaModuleHostManager
 *
 * @return	Returns the number of Java module hosts known by this manager.
 */
MOCKABLE_FUNCTION(, size_t, JavaModuleHostManager_Size, JAVA_MODULE_HOST_MANAGER_HANDLE, handle);

#ifdef __cplusplus
}
#endif

#endif /*JAVA_MODULE_HOST_MANAGER_H*/
