// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/condition.h"
#include "azure_c_shared_utility/singlylinkedlist.h"

#include "gateway.h"
#include "experimental/event_system.h"

#include <assert.h>
#include <stdlib.h>

struct EVENTSYSTEM_DATA {
    VECTOR_HANDLE event_callbacks[GATEWAY_EVENTS_COUNT];
    /* Should some callback or thread creation fail all next event reports will be no-op */
    int is_errored;
    /* @brief Decides whether the thread should have delayed shutdown or not */
    int delay_when_queue_empty;

    THREAD_HANDLE callback_thread;
    // The last thread that quit already and needs cleaning up of the handle
    THREAD_HANDLE destroyed_thread;
    LOCK_HANDLE internal_change_lock;
    LOCK_HANDLE thread_queue_lock;
    COND_HANDLE thread_queue_condition;
    SINGLYLINKEDLIST_HANDLE thread_queue;
};

typedef struct CALLBACK_CLOSURE_TAG {
    GATEWAY_CALLBACK call;
    void* user_param;
} CALLBACK_CLOSURE;

typedef struct THREAD_QUEUE_ROW_TAG {
    GATEWAY_HANDLE gateway;
    GATEWAY_EVENT event_type;
    VECTOR_HANDLE callbacks;
    GATEWAY_EVENT_CTX context;
} THREAD_QUEUE_ROW;

/** @brief How long should the thread stay alive before shutting down when the processing queue is empty */
static const int THREAD_EMPTY_QUEUE_TIMEOUT_MS = 200;

static void destroy_event_system(EVENTSYSTEM_HANDLE handle);
static void callbacks_call(EVENTSYSTEM_HANDLE event_system, GATEWAY_HANDLE gw, GATEWAY_EVENT event_type, VECTOR_HANDLE callbacks, GATEWAY_EVENT_CTX context);
static int add_to_thread_queue(EVENTSYSTEM_HANDLE event_system, THREAD_QUEUE_ROW* row);
static THREAD_QUEUE_ROW* get_from_thread_queue(EVENTSYSTEM_HANDLE event_system, int timeout_ms);
static void destroy_thread_row(THREAD_QUEUE_ROW* row);
static int callback_thread_main_func(void* event_system_param);
static GATEWAY_EVENT_CTX handle_module_list_update(EVENTSYSTEM_HANDLE event_system, GATEWAY_HANDLE gateway, VECTOR_HANDLE callbacks);

/** @brief This function assumes that the context is a #VECTOR_HANDLE and destroys it */
static void callback_destroy_modulelist(GATEWAY_HANDLE gateway, GATEWAY_EVENT event_type, GATEWAY_EVENT_CTX context, void* user_param);

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
        /* NULL everything for easier free() in case of VECTOR malloc failure */
        memset(result, 0, sizeof(struct EVENTSYSTEM_DATA));

        result->internal_change_lock = Lock_Init();
        result->thread_queue_lock = Lock_Init();
        result->thread_queue_condition = Condition_Init();
        /* Codes_SRS_EVENTSYSTEM_26_002: [ This function shall return NULL upon any internal error during event system creation. ] */
        if (result->internal_change_lock == NULL || result->thread_queue_lock == NULL || result->thread_queue_condition == NULL)
        {
            LogError("failed to initialize event system locks or condition");
            destroy_event_system(result);
            result = NULL;
        }
        else
        {
            for (int i = 0; i < GATEWAY_EVENTS_COUNT; i++)
            {
                VECTOR_HANDLE event_callbacks = VECTOR_create(sizeof(CALLBACK_CLOSURE));
                /* Codes_SRS_EVENTSYSTEM_26_002: [ This function shall return NULL upon any internal error during event system creation. ] */
                if (event_callbacks == NULL)
                {
                    LogError("failed to initialize callback vectors");
                    destroy_event_system(result);
                    result = NULL;
                    break;
                }
                else
                {
                    result->event_callbacks[i] = event_callbacks;
                }
            }

            /* callback creation might have failed */
            if (result != NULL)
            {
                result->delay_when_queue_empty = 1;

                result->thread_queue = singlylinkedlist_create();
                /* Codes_SRS_EVENTSYSTEM_26_002: [ This function shall return NULL upon any internal error during event system creation. ] */
                if (result->thread_queue == NULL)
                {
                    LogError("failed to create thread queue during event system init");
                    destroy_event_system(result);
                    result = NULL;
                }
            }
        }
    }
    return result;
}

void EventSystem_AddEventCallback(EVENTSYSTEM_HANDLE event_system, GATEWAY_EVENT event_type, GATEWAY_CALLBACK callback, void* user_param)
{
    /* Codes_SRS_EVENTSYSTEM_26_012: [ This function shall log a failure and do nothing else when either `event_system` or `callback` parameters are NULL. ] */
    if (event_system == NULL || callback == NULL)
    {
        LogError("null gateway events handle or callback parameters during event registration");
    }
    /* Codes_SRS_EVENTSYSTEM_26_011: [ This function shall register given GATEWAY_CALLBACK and call it when given GATEWAY_EVENT event happens inside of the gateway. ] */
    else
    {
        CALLBACK_CLOSURE closure = {
            callback,
            user_param
        };
        if (VECTOR_push_back(event_system->event_callbacks[event_type], &closure, 1) != 0)
        {
            /* Codes_SRS_EVENTSYSTEM_26_013: [ Should the worker thread ever fail to be created or any internall callbacks fail, failure will be logged and no further callbacks will be called during gateway's lifecycle. ] */
            LogError("failed to register callback");
            event_system->is_errored = 1;
        }
    }
}

void EventSystem_ReportEvent(EVENTSYSTEM_HANDLE event_system, GATEWAY_HANDLE gw, GATEWAY_EVENT event_type)
{
    /* Codes_SRS_EVENTSYSTEM_26_014: [ This function shall do nothing when `event_system` parameter is NULL. ] */
    if (event_system == NULL)
    {
        LogError("null gateway handle or gateway event handle when reporting event");
    }
    /* Codes_SRS_EVENTSYSTEM_26_013: [ Should the worker thread ever fail to be created or any internall callbacks fail, failure will be logged and no further callbacks will be called during gateway's lifecycle. ] */
    else if (!event_system->is_errored)
    {
        /* Lock-avoiding mechanism, we get a probably-past state with previous if, then check synchronized state to be sure */
        int real_is_errored = 0;
        Lock(event_system->internal_change_lock);
        real_is_errored = event_system->is_errored;
        Unlock(event_system->internal_change_lock);
        
        if (!real_is_errored)
        {
            /* We need to copy the callback queue because the callback might register another function */
            /* Codes_SRS_EVENTSYSTEM_26_007: [ This function shan't call any callbacks registered for any other GATEWAY_EVENT other than the one given as parameter. ] */
            VECTOR_HANDLE callbacks = event_system->event_callbacks[event_type];
            size_t vector_size = VECTOR_size(callbacks);

            if (vector_size > 0)
            {
                VECTOR_HANDLE call_queue = VECTOR_create(sizeof(CALLBACK_CLOSURE));
                if (call_queue == NULL)
                {
                    /*Codes_SRS_EVENTSYSTEM_26_013: [ Should the worker thread ever fail to be created or any internall callbacks fail, failure will be logged and no further callbacks will be called during gateway's lifecycle. ] */
                    LogError("Failed to create call queue during event report");
                    event_system->is_errored = 1;
                }
                else
                {
                    if (VECTOR_push_back(call_queue, VECTOR_front(callbacks), vector_size) != 0)
                    {
                        /*Codes_SRS_EVENTSYSTEM_26_013: [ Should the worker thread ever fail to be created or any internall callbacks fail, failure will be logged and no further callbacks will be called during gateway's lifecycle. ] */
                        LogError("Failed to copy callback queue during event report");
                        event_system->is_errored = 1;
                        VECTOR_destroy(call_queue);
                    }
                    else
                    {
                        GATEWAY_EVENT_CTX context = NULL;
                        /* handlers might change event_system->is_errored */
                        switch (event_type)
                        {
                        case GATEWAY_MODULE_LIST_CHANGED:
                            context = handle_module_list_update(event_system, gw, call_queue);
                            break;
                        default:
                            break;
                        }

                        if (event_system->is_errored)
                            VECTOR_destroy(call_queue);
                        else
                            callbacks_call(event_system, gw, event_type, call_queue, context);
                    }
                }
            }
        }
    }
}

void EventSystem_Destroy(EVENTSYSTEM_HANDLE handle)
{
    destroy_event_system(handle);
}

/*********************
 * Private functions *
 *********************/

static void destroy_event_system(EVENTSYSTEM_HANDLE handle)
{
    /* Codes_SRS_EVENTSYSTEM_26_004: [ This function shall do nothing when `event_system` parameter is NULL. ] */
    if (handle != NULL)
    {
        if (handle->thread_queue_lock != NULL && handle->thread_queue_condition != NULL)
        {
            /* force the thread to not wait for new stuff on the queue and return as soon as the queue is empty */
            Lock(handle->thread_queue_lock);

            /* in case the thread is still running callbacks, notify it that it shouldn't wait for new data later on */
            handle->delay_when_queue_empty = 0;
            /* in case the thread is already waiting and we want it to stop waiting */
            Condition_Post(handle->thread_queue_condition);

            Unlock(handle->thread_queue_lock);
        }

        THREAD_HANDLE callback_thread = NULL, destroyed_thread = NULL;
        if (handle->internal_change_lock != NULL)
        {
            // The thread might be running, we want to wait for it to finish, but it might be moving the handles
            // So we need to have a copy of those
            Lock(handle->internal_change_lock);

            callback_thread = handle->callback_thread;
            destroyed_thread = handle->destroyed_thread;

            Unlock(handle->internal_change_lock);
        }

        int thread_res;
        /* Codes_SRS_EVENTSYSTEM_26_005: [ This function shall wait for all callbacks to finish before returning. ] */
        if (callback_thread != NULL)
            ThreadAPI_Join(callback_thread, &thread_res);
        if (destroyed_thread != NULL)
            ThreadAPI_Join(destroyed_thread, &thread_res);
        /* Codes_SRS_EVENTSYSTEM_26_003: [ This function shall destroy and free resources of the given event system. ] */
        Condition_Deinit(handle->thread_queue_condition);
        Lock_Deinit(handle->thread_queue_lock);
        Lock_Deinit(handle->internal_change_lock);

        /* Something might have been left on the list if thread errored out */
        LIST_ITEM_HANDLE node = NULL;
        while ((node = singlylinkedlist_get_head_item(handle->thread_queue)) != NULL)
        {
            destroy_thread_row((THREAD_QUEUE_ROW*)singlylinkedlist_item_get_value(node));
            singlylinkedlist_remove(handle->thread_queue, node);
        }
        singlylinkedlist_destroy(handle->thread_queue);

        for (int i = 0; i < GATEWAY_EVENTS_COUNT; i++)
            VECTOR_destroy(handle->event_callbacks[i]);
        free(handle);
    }
}

static void callbacks_call(EVENTSYSTEM_HANDLE event_system, GATEWAY_HANDLE gw, GATEWAY_EVENT event_type, VECTOR_HANDLE callbacks, GATEWAY_EVENT_CTX context)
{
    THREAD_QUEUE_ROW* row = (THREAD_QUEUE_ROW*)malloc(sizeof(THREAD_QUEUE_ROW));
    if (row == NULL)
    {
        LogError("malloc failed when trying to create event system queue row");
        VECTOR_destroy(callbacks);
    }
    else
    {
        row->gateway = gw;
        row->event_type = event_type;
        row->callbacks = callbacks;
        row->context = context;
        /* Failed to add to queue, we have allocated row which won't be freed during EventSystem destroy */
        if (add_to_thread_queue(event_system, row))
        {
            destroy_thread_row(row);
            row = NULL;
        }
    }

    Lock(event_system->internal_change_lock);

    // There's a thread to cleanup that was destroyed but has a floating handle
    if (event_system->destroyed_thread != NULL)
    {
        int res;
        ThreadAPI_Join(event_system->destroyed_thread, &res);
        event_system->destroyed_thread = NULL;
    }

    /* Codes_SRS_EVENTSYSTEM_26_013: [ Should the worker thread ever fail to be created or any internall callbacks fail, failure will be logged and no further callbacks will be called during gateway's lifecycle. ] */
    if (row == NULL)
    {
        event_system->is_errored = 1;
    }
    else if (event_system->callback_thread == NULL)
    {
        /* Codes_SRS_EVENTSYSTEM_26_008: [ This function shall call all registered callbacks on a seperate thread. ] */
        THREADAPI_RESULT result = ThreadAPI_Create(&event_system->callback_thread, callback_thread_main_func, (void*)event_system);
        /* Codes_SRS_EVENTSYSTEM_26_013: [ Should the worker thread ever fail to be created or any internall callbacks fail, failure will be logged and no further callbacks will be called during gateway's lifecycle. ] */
        /* Stuff on the queue will be deleted when destroying EventSystem */
        if (result != THREADAPI_OK)
            event_system->is_errored = 1;
    }

    Unlock(event_system->internal_change_lock);
}

static int add_to_thread_queue(EVENTSYSTEM_HANDLE event_system, THREAD_QUEUE_ROW* row)
{
    Lock(event_system->thread_queue_lock);

    int errored = !singlylinkedlist_add(event_system->thread_queue, row);
    Condition_Post(event_system->thread_queue_condition);

    Unlock(event_system->thread_queue_lock);

    return errored;
}

static THREAD_QUEUE_ROW* get_from_thread_queue(EVENTSYSTEM_HANDLE event_system, int timeout_ms)
{
    THREAD_QUEUE_ROW* row = NULL;
    
    Lock(event_system->thread_queue_lock);
    
    LIST_ITEM_HANDLE node = singlylinkedlist_get_head_item(event_system->thread_queue);
    if (node == NULL && event_system->delay_when_queue_empty)
    {
        Condition_Wait(event_system->thread_queue_condition, event_system->thread_queue_lock, timeout_ms);
        node = singlylinkedlist_get_head_item(event_system->thread_queue);
    }

    /* Condition might have timed out or we don't have delayed destroy so the node can be NULL */
    if (node != NULL)
    {
        row = (THREAD_QUEUE_ROW*)singlylinkedlist_item_get_value(node);
        singlylinkedlist_remove(event_system->thread_queue, node);
    }
    
    Unlock(event_system->thread_queue_lock);
    
    return row;
}

static void destroy_thread_row(THREAD_QUEUE_ROW* row)
{
    if (row != NULL)
        VECTOR_destroy(row->callbacks);
    free(row);
}

static int callback_thread_main_func(void* event_system_param)
{
    EVENTSYSTEM_HANDLE event_system = (EVENTSYSTEM_HANDLE)event_system_param;
    THREAD_QUEUE_ROW* row;
    while ((row = get_from_thread_queue(event_system, THREAD_EMPTY_QUEUE_TIMEOUT_MS)) != NULL)
    {
        size_t vector_size = VECTOR_size(row->callbacks);
        /* Codes_SRS_EVENTSYSTEM_26_006: [ This function shall call all registered callbacks for the given GATEWAY_EVENT. ] */
        /* Codes_SRS_EVENTSYSTEM_26_009: [ This function shall call all registered callbacks in First - In - First - Out order in terms registration. ] */
        for (size_t i = 0; i < vector_size; i++)
        {
            CALLBACK_CLOSURE *closure = (CALLBACK_CLOSURE*)VECTOR_element(row->callbacks, i);
            /* Codes_SRS_EVENTSYSTEM_26_010: [ The given `GATEWAY_CALLBACK` function shall be called with proper `GATEWAY_HANDLE`, `GATEWAY_EVENT` and provided user parameter as function parameters coresponding to the gateway and the event that occured. ] */
            closure->call(row->gateway, row->event_type, row->context, closure->user_param);
        }
        destroy_thread_row(row);
    }

    Lock(event_system->internal_change_lock);

    event_system->destroyed_thread = event_system->callback_thread;
    event_system->callback_thread = NULL;

    Unlock(event_system->internal_change_lock);

    return THREADAPI_OK;
}

static GATEWAY_EVENT_CTX handle_module_list_update(EVENTSYSTEM_HANDLE event_system, GATEWAY_HANDLE gateway, VECTOR_HANDLE callbacks)
{
    /* Codes_SRS_EVENTSYSTEM_26_016: [ This event shall provide `VECTOR_HANDLE` as returned from #Gateway_GetModuleList as the event context in callbacks ] */
    VECTOR_HANDLE modules = Gateway_GetModuleList(gateway);
    if (modules == NULL)
    {
        event_system->is_errored = 1;
    }
    else
    {
        CALLBACK_CLOSURE closure = {
            callback_destroy_modulelist,
            NULL
        };
        /* Codes_SRS_EVENTSYSTEM_26_015: [ This event shall clean up the `VECTOR_HANDLE` of #Gateway_GetModuleList after finishing all the callbacks ] */
        if (VECTOR_push_back(callbacks, &closure, 1) != 0)
        {
            LogError("Failed to push back during handling module list updated event");
            VECTOR_destroy(modules);
            event_system->is_errored = 1;
            modules = NULL;
        }
    }
    return modules;
}


static void callback_destroy_modulelist(GATEWAY_HANDLE gateway, GATEWAY_EVENT event_type, GATEWAY_EVENT_CTX context, void* user_param)
{
    Gateway_DestroyModuleList((VECTOR_HANDLE)context);
}