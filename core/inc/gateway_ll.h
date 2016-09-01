// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file		gateway_ll.h
*	@brief		Library that allows a user to create and configure a gateway.
*
*	@details	Gateway LL is the lower level library that allows a developer to 
*				create, configure, and manage a gateway. The library provides a 
*				mechanism for creating and destroying a gateway, as well as 
*				adding and removing modules. Developers looking for the high
*				level library should see gateway.h.
*/

#ifndef GATEWAY_LL_H
#define GATEWAY_LL_H

#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/vector.h"
#include "module.h"

#ifdef __cplusplus
extern "C"
{
#endif



#define GATEWAY_ADD_LINK_RESULT_VALUES \
    GATEWAY_ADD_LINK_SUCCESS, \
    GATEWAY_ADD_LINK_ERROR, \
    GATEWAY_ADD_LINK_INVALID_ARG

/** @brief	Enumeration describing the result of ::Gateway_LL_AddLink.
*/
DEFINE_ENUM(GATEWAY_ADD_LINK_RESULT, GATEWAY_ADD_LINK_RESULT_VALUES);

/** @brief Struct representing a single link for a gateway. */
typedef struct GATEWAY_LINK_ENTRY_TAG
{
	/** @brief The name of the module which is going to send messages. */
	const char* module_source;

	/** @brief The name of the module which is going to receive messages. */
	const char* module_sink;
} GATEWAY_LINK_ENTRY;

/** @brief Struct representing a particular gateway. */
typedef struct GATEWAY_HANDLE_DATA_TAG* GATEWAY_HANDLE;

/** @brief Struct representing a single entry of the #GATEWAY_PROPERTIES. */
typedef struct GATEWAY_MODULES_ENTRY_TAG
{
	/** @brief The (possibly @c NULL) name of the module */
	const char* module_name;
	
	/** @brief The path to the .dll or .so of the module */
	const char* module_path;
	
	/** @brief The user-defined configuration object for the module */
	const void* module_configuration;
} GATEWAY_MODULES_ENTRY;

/** @brief	Struct representing the properties that should be used when 
			creating a module; each entry of the @c VECTOR_HANDLE being a 
*			#GATEWAY_MODULES_ENTRY. 
*/
typedef struct GATEWAY_PROPERTIES_DATA_TAG
{
	/** @brief Vector of #GATEWAY_MODULES_ENTRY objects. */
	VECTOR_HANDLE gateway_modules;

	/** @brief Vector of #GATEWAY_LINK_ENTRY objects. */
	VECTOR_HANDLE gateway_links;
} GATEWAY_PROPERTIES;

/** @brief Struct representing current information about a single module */
typedef struct GATEWAY_MODULE_INFO_TAG
{
	/** @brief The name of the module */
	const char* module_name;

	/** @brief A vector of pointers to @c GATEWAY_MODULE_INFO that this module will receive data from
	 * (link sources, this one being the sink). 
	 * 
	 * If the handle == NULL this module receives data from all other modules. 
	 */
	VECTOR_HANDLE module_sources;
} GATEWAY_MODULE_INFO;

/** @brief  Enum representing different gateway events that have support for callbacks. */
typedef enum GATEWAY_EVENT_TAG
{
	/** @brief Called when the gateway is created. */
	GATEWAY_CREATED = 0,

	/** @brief  Called every time a list of modules or links changed and during gateway creation.
	 *
	 * The VECTOR_HANDLE from #Gateway_LL_GetModuleList will be provided as the context to the callback,
	 * and be later cleaned-up automatically.
	 */
	GATEWAY_MODULE_LIST_CHANGED,

	/** @brief Called when the gateway is destroyed. */
	GATEWAY_DESTROYED,

	/* @brief  Not an actual event, used to keep track of count of different events */
	GATEWAY_EVENTS_COUNT
} GATEWAY_EVENT;

/** @brief Context that is provided to the callback.
 * 
 * The context is shared between all the callbacks for a current event and cleaned up afterwards.
 * See #GATEWAY_EVENT for specific contexts for each event.
 */
typedef void* GATEWAY_EVENT_CTX;

/** @brief	Function pointer that can be registered and will be called for gateway events 
 *
 * @c context may be NULL if the #GATEWAY_EVENT does not provide a context.
 */
typedef void(*GATEWAY_CALLBACK)(GATEWAY_HANDLE gateway, GATEWAY_EVENT event_type, GATEWAY_EVENT_CTX context, void* user_param);

/** @brief		Creates a new gateway using the provided #GATEWAY_PROPERTIES.
*
*	@param		properties		#GATEWAY_PROPERTIES structure containing 
*								specific module properties and information.
*
*	@return		A non-NULL #GATEWAY_HANDLE that can be used to manage the 
*				gateway or @c NULL on failure.
*/
extern GATEWAY_HANDLE Gateway_LL_Create(const GATEWAY_PROPERTIES* properties);

/** @brief		Destroys the gateway and disposes of all associated data.
*
*	@param		gw		#GATEWAY_HANDLE to be destroyed.
*/
extern void Gateway_LL_Destroy(GATEWAY_HANDLE gw);

/** @brief		Creates a new module based on the GATEWAY_MODULES_ENTRY*.
*
*	@param		gw		Pointer to a #GATEWAY_HANDLE to add the Module onto.
*	@param		entry	Pointer to a #GATEWAY_MODULES_ENTRY structure
*						describing the module.
*
*	@return		A non-NULL #MODULE_HANDLE to the newly created and added
*				Module, or @c NULL on failure.
*/
extern MODULE_HANDLE Gateway_LL_AddModule(GATEWAY_HANDLE gw, const GATEWAY_MODULES_ENTRY* entry);



/** @brief		Removes the provided module from the gateway and all links that involves this module.
*
*	@param		gw		Pointer to a #GATEWAY_HANDLE from which to remove the
*						Module.
*	@param		module	Pointer to a #MODULE_HANDLE that needs to be removed.
*/
extern void Gateway_LL_RemoveModule(GATEWAY_HANDLE gw, MODULE_HANDLE module);

/** @brief      Removes module by its unique name
 *
 *  @param      gw          Pointer to a #GATEWAY_HANDLE from which to remove the
 *                          module.
 *  @param      module_name A C string representing the name of module to be
 *                          removed
 *
 *  @return                 0 on success and a non-zero value when an error occurs.
 */
int Gateway_LL_RemoveModuleByName(GATEWAY_HANDLE gw, const char *module_name);

/** @brief		Registers a function to be called on a callback thread when #GATEWAY_EVENT happens
*		
*   @param		gw			Pointer to a #GATEWAY_HANDLE to which register callback to
*   @param		event_type 	Enum stating on which event should the callback be called
*   @param		callback	Pointer to a function that will be called when the event happens
*   @param		user_param	User defined parameter that will be later provided to the called callback
*/
extern void Gateway_LL_AddEventCallback(GATEWAY_HANDLE gw, GATEWAY_EVENT event_type, GATEWAY_CALLBACK callback, void* user_param);

/** @brief		Returns a snapshot copy of information about running modules.
*
*				Since this function allocates new memory for the snapshot, the vector handle should be 
*				later destroyed with @c Gateway_DestroyModuleList.
*
*   @param 		gw			Pointer to a #GATEWAY_HANDLE from which the module list should be snapshoted
*
*   @return					A #VECTOR_HANDLE of pointers to #GATEWAY_MODULE_INFO. NULL if function errored.
*/
extern VECTOR_HANDLE Gateway_LL_GetModuleList(GATEWAY_HANDLE gw);

/** @brief 		Destroys the list returned by @c Gateway_LL_GetModuleList
*
*   @param 		module_list	A vector handle as returned from @c Gateway_LL_GetModuleList
*/
extern void Gateway_LL_DestroyModuleList(VECTOR_HANDLE module_list);

/** @brief		Adds a link to a gateway message broker.
*
*	@param		gw		    Pointer to a #GATEWAY_HANDLE from which link is going to be added.
*
*	@param		entryLink	Pointer to a #GATEWAY_LINK_ENTRY to be added.
*
*	@return		A GATEWAY_ADD_LINK_RESULT with the operation result.
*/
extern GATEWAY_ADD_LINK_RESULT Gateway_LL_AddLink(GATEWAY_HANDLE gw, const GATEWAY_LINK_ENTRY* entryLink);

/** @brief		Remove a link from a gateway message broker.
*
*	@param		gw		    Pointer to a #GATEWAY_HANDLE from which link is going to be removed.
*
*	@param		entryLink	Pointer to a #GATEWAY_LINK_ENTRY to be removed.
*/
extern void Gateway_LL_RemoveLink(GATEWAY_HANDLE gw, const GATEWAY_LINK_ENTRY* entryLink);

#ifdef UWP_BINDING

/** @brief		Creates a new gateway using the provided #MODULEs and #BROKER_HANDLE.
*
*	@param		modules   		#VECTOR_HANDLE structure containing
*								specific modules.
*
*	@param		broker          #BROKER_HANDLE structure containing
*								specific message broker instance.
*
*	@return		A non-NULL #GATEWAY_HANDLE that can be used to manage the
*				gateway or @c NULL on failure.
*/
extern GATEWAY_HANDLE Gateway_LL_UwpCreate(const VECTOR_HANDLE modules, BROKER_HANDLE broker);

/** @brief		Destroys the gateway and disposes of all associated data.
*
*	@param		gw		#GATEWAY_HANDLE to be destroyed.
*/
extern void Gateway_LL_UwpDestroy(GATEWAY_HANDLE gw);

#endif // UWP_BINDING

#ifdef __cplusplus
}
#endif

#endif // GATEWAY_LL_H
