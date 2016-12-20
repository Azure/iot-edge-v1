// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file       event_system.h
*   @brief      Header file with internal API for managing the callback system
*/

#ifndef EVENT_SYSTEM_H
#define EVENT_SYSTEM_H

#include "gateway.h"
#include "gateway_export.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

typedef struct EVENTSYSTEM_DATA* EVENTSYSTEM_HANDLE;

/** @brief      Struct representing current information about a single module */
typedef struct GATEWAY_MODULE_INFO_TAG
{
    /** @brief  The name of the module */
    const char* module_name;

    /** @brief  A vector of pointers to @c GATEWAY_MODULE_INFO that this module
     *          will receive data from (link sources, this one being the sink). 
     * 
     *  If the handle == NULL this module receives data from all other modules. 
     */
    VECTOR_HANDLE module_sources;
} GATEWAY_MODULE_INFO;

/** @brief      Enum representing different gateway events that have support
 *              for callbacks.
 */
typedef enum GATEWAY_EVENT_TAG
{
    /** @brief  Called when the gateway is created. */
    GATEWAY_CREATED = 0,
    /** @brief  Called when the gateway is started. */
    GATEWAY_STARTED,
    /** @brief  Called every time a list of modules or links changed and during
     *          gateway creation.
     *
     *  The VECTOR_HANDLE from #Gateway_GetModuleList will be provided as the
     *  context to the callback, and be later cleaned-up automatically.
     */
    GATEWAY_MODULE_LIST_CHANGED,

    /** @brief  Called when the gateway is destroyed. */
    GATEWAY_DESTROYED,

    /* @brief   Not an actual event, used to keep track of count of different
     *          events
     */
    GATEWAY_EVENTS_COUNT
} GATEWAY_EVENT;

/** @brief      Context that is provided to the callback.
 * 
 *  The context is shared between all the callbacks for a current event and
 *  cleaned up afterwards. See #GATEWAY_EVENT for specific contexts for each
 *  event.
 */
typedef void* GATEWAY_EVENT_CTX;

/** @brief      Function pointer that can be registered and will be called for
 *              gateway events 
 *
 *  @c context may be NULL if the #GATEWAY_EVENT does not provide a context.
 */
typedef void(*GATEWAY_CALLBACK)(GATEWAY_HANDLE gateway, GATEWAY_EVENT event_type, GATEWAY_EVENT_CTX context, void* user_param);

EVENTSYSTEM_HANDLE EventSystem_Init(void);
void EventSystem_AddEventCallback(EVENTSYSTEM_HANDLE event_system, GATEWAY_EVENT event_type, GATEWAY_CALLBACK callback, void* user_param);
void EventSystem_ReportEvent(EVENTSYSTEM_HANDLE event_system, GATEWAY_HANDLE gw, GATEWAY_EVENT event_type);
void EventSystem_Destroy(EVENTSYSTEM_HANDLE event_system);

/** @brief      Registers a function to be called on a callback thread when_all
 *              #GATEWAY_EVENT happens
 *        
 *  @param      gw          Pointer to a #GATEWAY_HANDLE to which the callback
 *                          will be added
 *  @param      event_type  Enum stating on which event should the callback be
 *                          called
 *  @param      callback    Pointer to a function that will be called when the
 *                          event happens
 *  @param      user_param  User defined parameter that will be later provided
 *                          to the called callback
 */
void Gateway_AddEventCallback(GATEWAY_HANDLE gw, GATEWAY_EVENT event_type, GATEWAY_CALLBACK callback, void* user_param);

/** @brief      Returns a snapshot copy of information about running modules.
 *
 *              Since this function allocates new memory for the snapshot, the
 *              vector handle should be later destroyed with
 *              @c Gateway_DestroyModuleList.
 *
 *  @param      gw      Pointer to a #GATEWAY_HANDLE from which the module list
 *                      should be snapshoted
 *
 *  @return     A #VECTOR_HANDLE of pointers to #GATEWAY_MODULE_INFO on success.
 *              NULL on failure.
 */
VECTOR_HANDLE Gateway_GetModuleList(GATEWAY_HANDLE gw);

/** @brief      Destroys the list returned by @c Gateway_GetModuleList
 *
 *  @param      module_list A vector handle as returned from
 *              @c Gateway_GetModuleList
 */
void Gateway_DestroyModuleList(VECTOR_HANDLE module_list);

#ifdef __cplusplus
}
#endif // __cplusplus


#endif // !EVENT_SYSTEM_H