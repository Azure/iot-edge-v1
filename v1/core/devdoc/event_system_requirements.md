# Event System Requirements

## Overview
Throughout the lifecycle of a gateway there are many useful events produced. Gateway Events module allows a developer to register callbacks for specific events and respond to them on a separate worker thread without disturbing the normal operation of gateway.

## References

## EventSystem_Init
```
extern EVENTSYSTEM_HANDLE EventSystem_Init(void);
```

**SRS_EVENTSYSTEM_26_001: [** This function shall create `EVENTSYSTEM_HANDLE` representing the created event system. **]**

**SRS_EVENTSYSTEM_26_002: [** This function shall return `NULL` upon any internal error during event system creation. **]**

## EventSystem_Destroy
```
extern void EventSystem_Destroy(EVENTSYSTEM_HANDLE event_system);
```

**SRS_EVENTSYSTEM_26_003: [** This function shall destroy and free resources of the given event system. **]**

**SRS_EVENTSYSTEM_26_004: [** This function shall do nothing when `event_system` parameter is NULL. **]**

**SRS_EVENTSYSTEM_26_005: [** This function shall wait for all callbacks to finish before returning.  **]**

## EventSystem_ReportEvent
```
extern void EventSystem_ReportEvent(EVENT_SYSTEM_HANDLE event_system, GATEWAY_HANDLE gw, GATEWAY_EVENT event_type);
```

**SRS_EVENTSYSTEM_26_006: [** This function shall call all registered callbacks for the given `GATEWAY_EVENT`. **]**

**SRS_EVENTSYSTEM_26_007: [** This function shan't call any callbacks registered for any other `GATEWAY_EVENT` other than the one given as parameter. **]**

**SRS_EVENTSYSTEM_26_008: [** This function shall call all registered callbacks on a seperate thread. **]**

**SRS_EVENTSYSTEM_26_009: [** This function shall call all registered callbacks in First-In-First-Out order in terms registration. **]**

**SRS_EVENTSYSTEM_26_010: [** The given `GATEWAY_CALLBACK` function shall be called with proper `GATEWAY_HANDLE`, `GATEWAY_EVENT` and provided user parameter as function parameters coresponding to the gateway and the event that occured. **]**

**SRS_EVENTSYSTEM_26_014: [** This function shall do nothing when `event_system` parameter is NULL. **]**

## EventSystem_AddEventCallback
```
extern void EventSystem_AddEventCallback(EVENTSYSTEM_HANDLE event_system, GATEWAY_EVENT event_type, GATEWAY_CALLBACK callback, void* user_param);
```

**SRS_EVENTSYSTEM_26_011: [** This function shall register given `GATEWAY_CALLBACK` and call it when given `GATEWAY_EVENT` event happens inside of the gateway. **]**

**SRS_EVENTSYSTEM_26_012: [** This function shall log a failure and do nothing else when either `event_system` or `callback` parameters are NULL. **]**

## Event reporting

**SRS_EVENTSYSTEM_26_013: [** Should the worker thread ever fail to be created or any internall callbacks fail, failure will be logged and no further callbacks will be called during gateway's lifecycle. **]**

## Callback events requirements
```
GATEWAY_MODULE_LIST_UPDATED
```

**SRS_EVENTSYSTEM_26_016: [** This event shall provide `VECTOR_HANDLE` as returned from #Gateway_GetModuleList as the event context in callbacks **]**

**SRS_EVENTSYSTEM_26_015: [** This event shall clean up the `VECTOR_HANDLE` of #Gateway_GetModuleList after finishing all the callbacks **]**
