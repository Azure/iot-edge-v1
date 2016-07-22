// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <stdarg.h>

#include <glib.h>
#include <gio/gio.h>

#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/gballoc.h"
#include "gio_async_seq.h"

#define GIO_ASYNC_SEQ_ERROR_DOMAIN ((GQuark)0x42)

#define REPORT_ERROR(async_seq, message)  \
    do                                    \
    {                                     \
        LogError(message ".");         \
        report_error(async_seq, message); \
    } while(0)

typedef struct GIO_ASYNCSEQ_HANDLE_DATA_TAG
{
    gpointer                        async_seq_context;      // callback context for the entire sequence
    GPtrArray*                      callbacks_list;         // list of callbacks to be called in sequence
    GIO_ASYNCSEQ_ERROR_CALLBACK     error_callback;         // error callback
    GIO_ASYNCSEQ_COMPLETE_CALLBACK  complete_callback;      // called when the entire sequence completes successfully
    guint32                         current_callback_index; // specifies current callback index as sequence runs
    GIO_ASYNCSEQ_STATE              state;                  // current state of the sequence
}GIO_ASYNCSEQ_HANDLE_DATA;

typedef struct GIO_ASYNCSEQ_CALLBACK_DATA_TAG
{
    GIO_ASYNCSEQ_CALLBACK           callback;               // callback to be invoked
    GIO_ASYNCSEQ_FINISH_CALLBACK    finish_callback;        // callback to be invoked for finishing an
                                                            // async call and fetching the result/error
    gpointer                        callback_context;       // context to be supplied for this callback
}GIO_ASYNCSEQ_CALLBACK_DATA;

static void report_error(
    GIO_ASYNCSEQ_HANDLE_DATA* async_seq,
    const gchar* message
);

static void resolve_callback(
    GObject* source_object,
    GAsyncResult* result,
    gpointer user_data
);

GIO_ASYNCSEQ_HANDLE GIO_Async_Seq_Create(
    gpointer async_seq_context,
    GIO_ASYNCSEQ_ERROR_CALLBACK error_callback,
    GIO_ASYNCSEQ_COMPLETE_CALLBACK complete_callback
)
{
    GIO_ASYNCSEQ_HANDLE_DATA* result = (GIO_ASYNCSEQ_HANDLE_DATA*)malloc(
        sizeof(GIO_ASYNCSEQ_HANDLE_DATA)
    );
    if (result != NULL)
    {
        result->callbacks_list = g_ptr_array_new_with_free_func(free);
        if (result->callbacks_list != NULL)
        {
            /*Codes_SRS_GIO_ASYNCSEQ_13_001: [ GIO_Async_Seq_Create shall return a non-NULL handle on successful execution. ]*/
            /*Codes_SRS_GIO_ASYNCSEQ_13_005: [GIO_Async_Seq_Create shall save the async_seq_context pointer so that it can be retrieved later by calling GIO_Async_Seq_GetContext. ] */
            result->async_seq_context = async_seq_context;
            result->error_callback = error_callback;
            result->complete_callback = complete_callback;
            result->current_callback_index = 0;
            result->state = GIO_ASYNCSEQ_STATE_PENDING;
        }
        else
        {
            /*Codes_SRS_GIO_ASYNCSEQ_13_002: [ GIO_Async_Seq_Create shall return NULL when any of the underlying platform calls fail. ]*/
            LogError("g_ptr_array_new() returned NULL");
            free(result);
            result = NULL;
        }
    }
    else
    {
        /*Codes_SRS_GIO_ASYNCSEQ_13_002: [ GIO_Async_Seq_Create shall return NULL when any of the underlying platform calls fail. ]*/
        LogError("malloc() returned NULL");
        /* Fall through. 'result' is NULL. */
    }

    return (GIO_ASYNCSEQ_HANDLE)result;
}

void GIO_Async_Seq_Destroy(GIO_ASYNCSEQ_HANDLE async_seq_handle)
{
    /*
     * NOTE: We *DO NOT* attempt to cancel pending I/O operations if any are
     * in progress here.
     */
    if (async_seq_handle != NULL)
    {
        GIO_ASYNCSEQ_HANDLE_DATA* async_seq = (GIO_ASYNCSEQ_HANDLE_DATA*)async_seq_handle;
        if (async_seq->state == GIO_ASYNCSEQ_STATE_RUNNING)
        {
            LogError("Async sequence is still running. Destroying anyway.");
        }

        /*Codes_SRS_GIO_ASYNCSEQ_13_004: [ GIO_Async_Seq_Destroy shall free all resources associated with the handle. ]*/
        g_ptr_array_unref(async_seq->callbacks_list); // automatically calls 'free'
                                                      // for every entry in the array
        free(async_seq);
    }
    else
    {
        /*Codes_SRS_GIO_ASYNCSEQ_13_003: [ GIO_Async_Seq_Destroy shall do nothing if async_seq_handle is NULL. ]*/
        LogError("async_seq_handle is NULL.");
    }
}

gpointer GIO_Async_Seq_GetContext(GIO_ASYNCSEQ_HANDLE async_seq_handle)
{
    gpointer result;
    if (async_seq_handle != NULL)
    {
        GIO_ASYNCSEQ_HANDLE_DATA* async_seq = (GIO_ASYNCSEQ_HANDLE_DATA*)async_seq_handle;

        /*Codes_SRS_GIO_ASYNCSEQ_13_007: [ GIO_Async_Seq_GetContext shall return the value of the async_seq_context parameter that was passed in when calling GIO_Async_Seq_Create. ]*/
        result = async_seq->async_seq_context;
    }
    else
    {
        /*Codes_SRS_GIO_ASYNCSEQ_13_006: [ GIO_Async_Seq_GetContext shall return NULL if async_seq_handle is NULL. ]*/
        LogError("async_seq_handle is NULL.");
        result = NULL;
    }

    return result;
}

/**
 * The varargs part of this API is a sequence of parameter pairs, where each
 * pair is a GIO_ASYNCSEQ_CALLBACK pointer followed by a GIO_ASYNCSEQ_FINISH_CALLBACK
 * pointer. For e.g.,
 *
 *      GIO_Async_Seq_Add(async_seq_handle, NULL,
 *          on_get_bus, on_get_bus_finish,
 *          on_get_object_manager, on_get_object_manager_finish,
 *          on_get_device, on_get_device_finish,
 *          NULL
 *      );
 */
static GIO_ASYNCSEQ_RESULT seq_add_internal(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    gpointer callback_context,
    va_list args_list
)
{
    GIO_ASYNCSEQ_RESULT result;
    GIO_ASYNCSEQ_HANDLE_DATA* async_seq = (GIO_ASYNCSEQ_HANDLE_DATA*)async_seq_handle;

    // keep track of the end of the callbacks list before anything is added to it
    guint length_before = async_seq->callbacks_list->len;

    // the args list is a NULL terminated sequence of
    // (GIO_ASYNCSEQ_CALLBACK, GIO_ASYNCSEQ_FINISH_CALLBACK) pointer tuples
    GIO_ASYNCSEQ_CALLBACK current_callback = va_arg(args_list, GIO_ASYNCSEQ_CALLBACK);

    /*Codes_SRS_GIO_ASYNCSEQ_13_011: [ GIO_Async_Seq_Add shall add callbacks from the variable arguments list till a callback whose value is NULL is encountered. ]*/
    /*Codes_SRS_GIO_ASYNCSEQ_13_038: [ GIO_Async_Seq_Addv shall add callbacks from the variable arguments list till a callback whose value is NULL is encountered. ]*/
    while (current_callback != NULL)
    {
        GIO_ASYNCSEQ_FINISH_CALLBACK finish_callback = va_arg(
            args_list,
            GIO_ASYNCSEQ_FINISH_CALLBACK
        );
        GIO_ASYNCSEQ_CALLBACK_DATA* callback_data = NULL;

        if (
                /*Codes_SRS_GIO_ASYNCSEQ_13_012: [ When a GIO_ASYNCSEQ_CALLBACK is encountered in the varargs, the next argument MUST be non-NULL. ]*/
                /*Codes_SRS_GIO_ASYNCSEQ_13_039: [ When a GIO_ASYNCSEQ_CALLBACK is encountered in the varargs, the next argument MUST be non-NULL. ]*/
                // the finish callback MUST NOT be NULL
                finish_callback != NULL &&
                (
                    callback_data = (GIO_ASYNCSEQ_CALLBACK_DATA*)malloc(
                            sizeof(GIO_ASYNCSEQ_CALLBACK_DATA)
                    )
                ) != NULL
           )
        {
            callback_data->callback = current_callback;
            callback_data->finish_callback = finish_callback;

            /*Codes_SRS_GIO_ASYNCSEQ_13_014: [ The callback_context pointer value shall be saved so that they can be passed to the callbacks later when they are invoked. ]*/
            /*Codes_SRS_GIO_ASYNCSEQ_13_041: [ The callback_context pointer value shall be saved so that they can be passed to the callbacks later when they are invoked. ]*/
            callback_data->callback_context = callback_context;

            /*Codes_SRS_GIO_ASYNCSEQ_13_010: [ GIO_Async_Seq_Add shall append the new async operations to the end of the existing list of async operations if any. ]*/
            /*Codes_SRS_GIO_ASYNCSEQ_13_037: [ GIO_Async_Seq_Addv shall append the new async operations to the end of the existing list of async operations if any. ]*/
            g_ptr_array_add(async_seq->callbacks_list, callback_data);
        }
        else
        {
            LogError("malloc() returned NULL or no finish callback pair found.");

            // since we are bailing remove all the callbacks that were added
            // to the list so far; we want this function to be transactional with
            // respect to the callbacks added to the callbacks list
            if (length_before < async_seq->callbacks_list->len)
            {
                /*Codes_SRS_GIO_ASYNCSEQ_13_015: [ The list of callbacks that had been added to the sequence before calling this API will remain unchanged after the function returns when an error occurs. ]*/
                /*Codes_SRS_GIO_ASYNCSEQ_13_042: [ The list of callbacks that had been added to the sequence before calling this API will remain unchanged after the function returns when an error occurs. ]*/
                g_ptr_array_remove_range(
                    async_seq->callbacks_list,
                    length_before,
                    async_seq->callbacks_list->len - length_before
                );
            }

            // if we are here because of a missing finish callback then the
            // malloc would have happened; so we free it
            if (callback_data != NULL)
            {
                free(callback_data);
            }

            /*Codes_SRS_GIO_ASYNCSEQ_13_013: [ GIO_Async_Seq_Add shall return GIO_ASYNCSEQ_ERROR when any of the underlying platform calls fail. ]*/
            /*Codes_SRS_GIO_ASYNCSEQ_13_040: [ GIO_Async_Seq_Addv shall return GIO_ASYNCSEQ_ERROR when any of the underlying platform calls fail. ]*/
            result = GIO_ASYNCSEQ_ERROR;
            break;
        }

        current_callback = va_arg(args_list, GIO_ASYNCSEQ_CALLBACK);
    }

    if (current_callback == NULL)
    {
        /*Codes_SRS_GIO_ASYNCSEQ_13_016: [ GIO_Async_Seq_Add shall return GIO_ASYNCSEQ_OK if the API executes successfully. ]*/
        /*Codes_SRS_GIO_ASYNCSEQ_13_043: [ GIO_Async_Seq_Addv shall return GIO_ASYNCSEQ_OK if the API executes successfully. ]*/
        result = GIO_ASYNCSEQ_OK;
    }

    return result;
}

extern GIO_ASYNCSEQ_RESULT GIO_Async_Seq_Addv(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    gpointer callback_context,
    va_list args_list
)
{
    GIO_ASYNCSEQ_RESULT result;
    if (async_seq_handle != NULL)
    {
        GIO_ASYNCSEQ_HANDLE_DATA* async_seq = (GIO_ASYNCSEQ_HANDLE_DATA*)async_seq_handle;

        if (async_seq->state == GIO_ASYNCSEQ_STATE_PENDING)
        {
            result = seq_add_internal(async_seq_handle, callback_context, args_list);
        }
        else
        {
            /*Codes_SRS_GIO_ASYNCSEQ_13_036: [ GIO_Async_Seq_Addv shall return GIO_ASYNCSEQ_ERROR if the async sequence's state is not equal to GIO_ASYNCSEQ_STATE_PENDING. ]*/
            LogError("Async sequence must be in the 'pending' state when adding callbacks.");
            result = GIO_ASYNCSEQ_ERROR;
        }
    }
    else
    {
        /*Codes_SRS_GIO_ASYNCSEQ_13_035: [ GIO_Async_Seq_Addv shall return GIO_ASYNCSEQ_ERROR if async_seq_handle is NULL. ]*/
        LogError("async_seq_handle is NULL.");
        result = GIO_ASYNCSEQ_ERROR;
    }

    return result;
}

extern GIO_ASYNCSEQ_RESULT GIO_Async_Seq_Add(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    gpointer callback_context,
    ...
)
{
    GIO_ASYNCSEQ_RESULT result;
    if (async_seq_handle != NULL)
    {
        GIO_ASYNCSEQ_HANDLE_DATA* async_seq = (GIO_ASYNCSEQ_HANDLE_DATA*)async_seq_handle;

        if (async_seq->state == GIO_ASYNCSEQ_STATE_PENDING)
        {
            va_list args_list;
            va_start(args_list, callback_context);
            result = seq_add_internal(async_seq_handle, callback_context, args_list);
            va_end(args_list);
        }
        else
        {
            /*Codes_SRS_GIO_ASYNCSEQ_13_009: [ GIO_Async_Seq_Add shall return GIO_ASYNCSEQ_ERROR if the async sequence's state is not equal to GIO_ASYNCSEQ_STATE_PENDING. ]*/
            LogError("Async sequence must be in the 'pending' state when adding callbacks.");
            result = GIO_ASYNCSEQ_ERROR;
        }
    }
    else
    {
        /*Codes_SRS_GIO_ASYNCSEQ_13_008: [ GIO_Async_Seq_Add shall return GIO_ASYNCSEQ_ERROR if async_seq_handle is NULL. ]*/
        LogError("async_seq_handle is NULL.");
        result = GIO_ASYNCSEQ_ERROR;
    }

    return result;
}

GIO_ASYNCSEQ_RESULT GIO_Async_Seq_Run_Async(GIO_ASYNCSEQ_HANDLE async_seq_handle)
{
    GIO_ASYNCSEQ_RESULT result;
    
    if (async_seq_handle != NULL)
    {
        GIO_ASYNCSEQ_HANDLE_DATA* async_seq = (GIO_ASYNCSEQ_HANDLE_DATA*)async_seq_handle;

        // sequence must not already be running
        if (async_seq->state != GIO_ASYNCSEQ_STATE_RUNNING)
        {
            // we start running from the first callback
            async_seq->current_callback_index = 0;
            async_seq->state = GIO_ASYNCSEQ_STATE_RUNNING;

            if (async_seq->callbacks_list->len > 0)
            {
                GIO_ASYNCSEQ_CALLBACK_DATA* first_callback_data =
                    (GIO_ASYNCSEQ_CALLBACK_DATA*)g_ptr_array_index(
                        async_seq->callbacks_list, 0
                    );

                /*Codes_SRS_GIO_ASYNCSEQ_13_020: [ GIO_Async_Seq_Run shall invoke the 'start' callback of the first async operation in the sequence. ]*/
                /*Codes_SRS_GIO_ASYNCSEQ_13_021: [ GIO_Async_Seq_Run shall pass the callback context that was supplied to GIO_Async_Seq_Add when the first operation was added to the sequence when invoking the 'start' callback. ]*/
                /*Codes_SRS_GIO_ASYNCSEQ_13_023: [ GIO_Async_Seq_Run shall supply a non-NULL pointer to a function as the value of the async_callback parameter when calling the 'start' callback. ]*/
                /*Codes_SRS_GIO_ASYNCSEQ_13_031: [GIO_Async_Seq_Run_Async shall supply NULL as the value of the previous_result parameter of the 'start' callback. ]*/
                // kick-off sequence
                first_callback_data->callback(
                    async_seq_handle,
                    NULL, // no "previous" result for first callback
                    first_callback_data->callback_context,
                    resolve_callback
                );
            }
            else
            {
                /*Codes_SRS_GIO_ASYNCSEQ_13_019: [ GIO_Async_Seq_Run shall complete the sequence and invoke the sequence's complete callback if there are no asynchronous operations to process. ]*/
                // this is an empty sequence, so we are done
                LogInfo("Async sequence is empty.");
                async_seq->state = GIO_ASYNCSEQ_STATE_COMPLETE;
                if (async_seq->complete_callback != NULL)
                {
                    async_seq->complete_callback(async_seq_handle, NULL);
                }
            }
            
            /*Codes_SRS_GIO_ASYNCSEQ_13_022: [ GIO_Async_Seq_Run shall return GIO_ASYNCSEQ_OK when there are no errors. ]*/
            result = GIO_ASYNCSEQ_OK;
        }
        else
        {
            /*Codes_SRS_GIO_ASYNCSEQ_13_018: [ GIO_Async_Seq_Run shall return GIO_ASYNCSEQ_ERROR if the sequence's state is already GIO_ASYNCSEQ_STATE_RUNNING. ]*/
            result = GIO_ASYNCSEQ_ERROR;
            LogError("Async sequence is already running.");
        }
    }
    else
    {
        /*Codes_SRS_GIO_ASYNCSEQ_13_017: [ GIO_Async_Seq_Run shall return GIO_ASYNCSEQ_ERROR if async_seq_handle is NULL. ]*/
        result = GIO_ASYNCSEQ_ERROR;
        LogError("async_seq_handle is NULL.");
    }
    
    return result;
}

static void resolve_callback(
    GObject* source_object,
    GAsyncResult* result,
    gpointer user_data
)
{
    GIO_ASYNCSEQ_HANDLE_DATA* async_seq = (GIO_ASYNCSEQ_HANDLE_DATA*)user_data;
    // 'user_data' MUST be a GIO_ASYNCSEQ_HANDLE_DATA pointer
    if (user_data != NULL)
    {
        // sequence must be in 'running' state
        if (async_seq->state == GIO_ASYNCSEQ_STATE_RUNNING)
        {
            // get the current callback data & finish the async call
            GIO_ASYNCSEQ_CALLBACK_DATA* callback_data = (GIO_ASYNCSEQ_CALLBACK_DATA*)g_ptr_array_index(
                async_seq->callbacks_list,
                async_seq->current_callback_index
            );

            /*Codes_SRS_GIO_ASYNCSEQ_13_026: [ resolve_callback shall invoke the 'finish' callback of the async operation that was just concluded. ]*/
            GError* error = NULL;
            gpointer result_data = callback_data->finish_callback(
                (GIO_ASYNCSEQ_HANDLE)async_seq, result, &error
            );

            // handle error
            if (error != NULL)
            {
                /*Codes_SRS_GIO_ASYNCSEQ_13_027: [ resolve_callback shall invoke the sequence's error callback and suspend execution of the sequence if the 'finish' callback returns a non-NULL GError pointer. ]*/
                async_seq->state = GIO_ASYNCSEQ_STATE_ERROR;
                if (async_seq->error_callback != NULL)
                {
                    async_seq->error_callback((GIO_ASYNCSEQ_HANDLE)async_seq, error);
                }

                /*Codes_SRS_GIO_ASYNCSEQ_13_028: [ resolve_callback shall free the GError object by calling g_clear_error if the 'finish' callback returns a non-NULL GError pointer. ]*/
                g_clear_error(&error);
            }
            else
            {
                // invoke next callback in sequence, if any
                async_seq->current_callback_index = async_seq->current_callback_index + 1;
                if (async_seq->current_callback_index < async_seq->callbacks_list->len)
                {
                    GIO_ASYNCSEQ_CALLBACK_DATA* next_callback_data =
                        (GIO_ASYNCSEQ_CALLBACK_DATA*)g_ptr_array_index(
                            async_seq->callbacks_list,
                            async_seq->current_callback_index
                        );

                    /*Codes_SRS_GIO_ASYNCSEQ_13_029: [ resolve_callback shall invoke the 'start' callback of the next async operation in the sequence. ]*/
                    /*Codes_SRS_GIO_ASYNCSEQ_13_030: [ resolve_callback shall supply the result of calling the 'finish' callback of the current async operation as the value of the previous_result parameter of the 'start' callback of the next async operation. ]*/
                    /*Codes_SRS_GIO_ASYNCSEQ_13_032: [ resolve_callback shall pass the callback context that was supplied to GIO_Async_Seq_Add when the next async operation was added to the sequence when invoking the 'start' callback. ]*/
                    /*Codes_SRS_GIO_ASYNCSEQ_13_033: [ resolve_callback shall supply a non-NULL pointer to a function as the value of the async_callback parameter when calling the 'start' callback. ]*/
                    next_callback_data->callback(
                        (GIO_ASYNCSEQ_HANDLE)async_seq,
                        result_data,
                        next_callback_data->callback_context,
                        resolve_callback
                    );
                }
                else
                {
                    /*Codes_SRS_GIO_ASYNCSEQ_13_034: [ resolve_callback shall complete the sequence and invoke the sequence's complete callback if there are no more asynchronous operations to process. ]*/
                    // sequence is complete
                    async_seq->state = GIO_ASYNCSEQ_STATE_COMPLETE;
                    if (async_seq->complete_callback != NULL)
                    {
                        async_seq->complete_callback((GIO_ASYNCSEQ_HANDLE)async_seq, result_data);
                    }
                }
            }
        }
        else
        {
            /*Codes_SRS_GIO_ASYNCSEQ_13_025: [ resolve_callback shall invoke the sequence's error callback and suspend execution of the sequence if the state of the sequence is not equal to GIO_ASYNCSEQ_STATE_RUNNING. ]*/
            async_seq->state = GIO_ASYNCSEQ_STATE_ERROR;
            REPORT_ERROR(async_seq, "Async sequence is not in 'running' state");
        }
    }
    else
    {
        /*Codes_SRS_GIO_ASYNCSEQ_13_024: [ resolve_callback shall do nothing else if user_data is NULL. ]*/
        LogError("user_data is NULL.");
    }
}

static void report_error(
    GIO_ASYNCSEQ_HANDLE_DATA* async_seq,
    const gchar* message
)
{
    static GQuark error_domain = GIO_ASYNC_SEQ_ERROR_DOMAIN;
    if(async_seq->error_callback != NULL)
    {
        GError* error = g_error_new_literal(
            error_domain, GIO_ASYNCSEQ_ERROR, message
        );
        async_seq->error_callback((GIO_ASYNCSEQ_HANDLE)async_seq, error);
        g_clear_error(&error);
    }
}