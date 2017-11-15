// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"

#include "gateway.h"
#include "experimental/event_system.h"

#include <stdlib.h>

struct EVENTSYSTEM_DATA {
    int event_stub;
};



EVENTSYSTEM_HANDLE EventSystem_Init(void)
{
    /* Codes_SRS_EVENTSYSTEM_26_001: [ This function shall create EVENTSYSTEM_HANDLE representing the created event system. ] */
    EVENTSYSTEM_HANDLE result = (EVENTSYSTEM_HANDLE)malloc(sizeof(struct EVENTSYSTEM_DATA));
    if (result == NULL)
    {
        LogError("malloc returned NULL during event system creation");
        /* Return as is */
    }
    else
    {
        memset(result, 0, sizeof(struct EVENTSYSTEM_DATA));
    }
    return result;
}

void EventSystem_AddEventCallback(EVENTSYSTEM_HANDLE event_system, GATEWAY_EVENT event_type, GATEWAY_CALLBACK callback, void* user_param)
{
    (void)event_system;
    (void)event_type;
    (void)callback;
    (void)user_param;
}

void EventSystem_ReportEvent(EVENTSYSTEM_HANDLE event_system, GATEWAY_HANDLE gw, GATEWAY_EVENT event_type)
{
    (void)event_system;
    (void)gw;
    (void)event_type;
}

void EventSystem_Destroy(EVENTSYSTEM_HANDLE handle)
{
    if (handle != NULL)
    {
        free (handle);
    }
}
