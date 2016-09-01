// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file       event_system.h
*   @brief      Header file with internal API for managing the callback system
*/

#ifndef EVENT_SYSTEM_H
#define EVENT_SYSTEM_H

#include "gateway_ll.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

typedef struct EVENTSYSTEM_DATA* EVENTSYSTEM_HANDLE;

#ifndef UWP_BINDING

extern EVENTSYSTEM_HANDLE EventSystem_Init(void);
extern void EventSystem_AddEventCallback(EVENTSYSTEM_HANDLE event_system, GATEWAY_EVENT event_type, GATEWAY_CALLBACK callback, void* user_param);
extern void EventSystem_ReportEvent(EVENTSYSTEM_HANDLE event_system, GATEWAY_HANDLE gw, GATEWAY_EVENT event_type);
extern void EventSystem_Destroy(EVENTSYSTEM_HANDLE event_system);

#endif

#ifdef __cplusplus
}
#endif // __cplusplus


#endif // !EVENT_SYSTEM_H