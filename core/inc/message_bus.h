// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file		message_bus.h
*	@brief		Library for configuring and using the gateway's message bus.
*
*	@details	This is the API to create a reference counted and thread safe 
*				gateway message bus. The message bus broadcasts the messages to 
*				the subscribers. Messages on the message bus have a bag of 
*				properties (name, value) and an opaque array of bytes that is 
*				the message content.
*/

#ifndef MESSAGE_BUS_H
#define MESSAGE_BUS_H

/** @brief Struct representing a particular message bus. */
typedef struct MESSAGE_BUS_HANDLE_DATA_TAG* MESSAGE_BUS_HANDLE;

#include "azure_c_shared_utility/macro_utils.h"
#include "message.h"
#include "module.h"

#ifdef __cplusplus
#include <cstddef>
extern "C"
{
#else
#include <stddef.h>
#endif

#define MESSAGE_BUS_RESULT_VALUES \
    MESSAGE_BUS_OK, \
    MESSAGE_BUS_ERROR, \
    MESSAGE_BUS_INVALIDARG

/** @brief	Enumeration describing the result of ::MessageBus_Publish, 
*			::MessageBus_AddModule, and ::MessageBus_RemoveModule.
*/
DEFINE_ENUM(MESSAGE_BUS_RESULT, MESSAGE_BUS_RESULT_VALUES);

/** @brief	Creates a new message bus.
*
*	@return	A valid #MESSAGE_BUS_HANDLE upon success, or @c NULL upon failure.
*/
extern MESSAGE_BUS_HANDLE MessageBus_Create(void);

/** @brief		Creates a clone of the message bus.
*
*	@details	This function will simply increment the internal reference
*				count of the provided #MESSAGE_BUS_HANDLE.
*
*	@param		bus		The #MESSAGE_BUS_HANDLE to be cloned.
*/
extern void MessageBus_IncRef(MESSAGE_BUS_HANDLE bus);

/** @brief		Decrements the reference count of a message bus.
*
*	@details	This function will simply decrement the internal reference
*				count of the provided #MESSAGE_BUS_HANDLE, destroying the message
*				bus when the reference count reaches 0.
*
*	@param		bus		The #MESSAGE_BUS_HANDLE whose ref count will be decremented.
*/
extern void MessageBus_DecRef(MESSAGE_BUS_HANDLE bus);

/** @brief		Publishes a message onto the message bus.
*
*	@details	For details about threading with regard to the message bus and
*				modules connected to the message bus, see
*				<a href="https://github.com/Azure/azure-iot-gateway-sdk/blob/develop/core/devdoc/message_bus_hld.md">Bus High Level Design Documentation</a>.
*
*	@param		bus		The #MESSAGE_BUS_HANDLE onto which the message will be
*						published.
*	@param		source	The #MODULE_HANDLE from which the message will be
*						published. The bus will not publish the message to this
*                       module. (optional, may be NULL)
*	@param		message	The #MESSAGE_HANDLE representing the message to be
*						published.
*
*	@return		A #MESSAGE_BUS_RESULT describing the result of the function.
*/
extern MESSAGE_BUS_RESULT MessageBus_Publish(MESSAGE_BUS_HANDLE bus, MODULE_HANDLE source, MESSAGE_HANDLE message);

/** @brief		Adds a module onto the message bus.
*
*	@details	For details about threading with regard to the message bus and
*				modules connected to the message bus, see 
*				<a href="https://github.com/Azure/azure-iot-gateway-sdk/blob/develop/core/devdoc/message_bus_hld.md">Bus High Level Design Documentation</a>.
*
*	@param		bus				The #MESSAGE_BUS_HANDLE onto which the module will be 
*								added.
*	@param		module			The #MODULE for the module that will be added 
*								to this message bus.
*
*	@return		A #MESSAGE_BUS_RESULT describing the result of the function.
*/
extern MESSAGE_BUS_RESULT MessageBus_AddModule(MESSAGE_BUS_HANDLE bus, const MODULE* module);

/** @brief	Removes a module from the message bus.
*
*	@param	bus		The #MESSAGE_BUS_HANDLE from which the module will be removed.
*	@param	module	The #MODULE of the module to be removed.
*
*	@return	A #MESSAGE_BUS_RESULT describing the result of the function.
*/
extern MESSAGE_BUS_RESULT MessageBus_RemoveModule(MESSAGE_BUS_HANDLE bus, const MODULE* module);

/** @brief Disposes of resources allocated by a message bus.
*
*	@param	bus		The #MESSAGE_BUS_HANDLE to be destroyed.
*/
extern void MessageBus_Destroy(MESSAGE_BUS_HANDLE bus);

// This variable is used only for unit testing purposes.
extern size_t BUS_offsetof_quit_worker;

#ifdef __cplusplus
}
#endif


#endif /*MESSAGE_BUS_H*/
