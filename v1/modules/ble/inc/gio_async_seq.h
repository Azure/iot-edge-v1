// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef GIO_ASYNC_SEQ_H
#define GIO_ASYNC_SEQ_H

#include <glib.h>
#include <gio/gio.h>

#include "azure_c_shared_utility/macro_utils.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct GIO_ASYNCSEQ_HANDLE_DATA_TAG* GIO_ASYNCSEQ_HANDLE;

#define GIO_ASYNCSEQ_RESULT_VALUES \
    GIO_ASYNCSEQ_ERROR, \
    GIO_ASYNCSEQ_OK

DEFINE_ENUM(GIO_ASYNCSEQ_RESULT, GIO_ASYNCSEQ_RESULT_VALUES);

#define GIO_ASYNCSEQ_STATE_VALUES \
    GIO_ASYNCSEQ_STATE_PENDING, \
    GIO_ASYNCSEQ_STATE_RUNNING, \
    GIO_ASYNCSEQ_STATE_COMPLETE, \
    GIO_ASYNCSEQ_STATE_ERROR

DEFINE_ENUM(GIO_ASYNCSEQ_STATE, GIO_ASYNCSEQ_STATE_VALUES);

typedef void (*GIO_ASYNCSEQ_CALLBACK)(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    gpointer previous_result,
    gpointer callback_context,
    GAsyncReadyCallback async_callback
);

typedef void (*GIO_ASYNCSEQ_ERROR_CALLBACK)(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    const GError* error
);

typedef gpointer (*GIO_ASYNCSEQ_FINISH_CALLBACK)(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    GAsyncResult* result,
    GError** error
);

typedef void(*GIO_ASYNCSEQ_COMPLETE_CALLBACK)(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    gpointer previous_result
);

extern GIO_ASYNCSEQ_HANDLE GIO_Async_Seq_Create(
    gpointer async_seq_context,
    GIO_ASYNCSEQ_ERROR_CALLBACK error_callback,
    GIO_ASYNCSEQ_COMPLETE_CALLBACK complete_callback
);

extern void GIO_Async_Seq_Destroy(GIO_ASYNCSEQ_HANDLE async_seq_handle);

extern gpointer GIO_Async_Seq_GetContext(GIO_ASYNCSEQ_HANDLE async_seq_handle);

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
extern GIO_ASYNCSEQ_RESULT GIO_Async_Seq_Add(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    gpointer callback_context,
    ...
);

extern GIO_ASYNCSEQ_RESULT GIO_Async_Seq_Addv(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    gpointer callback_context,
    va_list args_list
);

extern GIO_ASYNCSEQ_RESULT GIO_Async_Seq_Run_Async(GIO_ASYNCSEQ_HANDLE async_seq_handle);

#ifdef __cplusplus
}
#endif

#endif // GIO_ASYNC_SEQ_H
