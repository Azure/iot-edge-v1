// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef BLEIO_SEQ_LINUX_COMMON_H
#define BLEIO_SEQ_LINUX_COMMON_H

#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/refcount.h"

#include "ble_gatt_io.h"
#include "bleio_seq.h"

#define BLEIO_SEQ_STATE_VALUES \
    BLEIO_SEQ_STATE_IDLE, \
    BLEIO_SEQ_STATE_RUNNING, \
    BLEIO_SEQ_STATE_SHUTTING_DOWN, \
    BLEIO_SEQ_STATE_ERROR

DEFINE_ENUM(BLEIO_SEQ_STATE, BLEIO_SEQ_STATE_VALUES);

typedef struct BLEIO_SEQ_HANDLE_DATA_TAG {
    BLEIO_GATT_HANDLE               bleio_gatt_handle;
    VECTOR_HANDLE                   instructions;
    BLEIO_SEQ_STATE                 state;
    ON_BLEIO_SEQ_READ_COMPLETE      on_read_complete;
    ON_BLEIO_SEQ_WRITE_COMPLETE     on_write_complete;
    ON_BLEIO_SEQ_DESTROY_COMPLETE   on_destroy_complete;
    void*                           destroy_context;
}BLEIO_SEQ_HANDLE_DATA;

/**
 * Internal optional callback invoked when a read/write operation completes.
 */
typedef void(*ON_INTERNAL_IO_COMPLETE)(BLEIO_SEQ_HANDLE_DATA* bleio_seq_handle, BLEIO_SEQ_INSTRUCTION* instruction);

// BLEIO_SEQ_HANDLE_DATA is a reference counted object. This is so because
// at any given point in time there could be numerous asynchronous I/O
// operations in progress. If the user calls BLEIO_Seq_Destroy while there
// are still outstanding I/O requests, we want to keep the handle data
// alive till all of them complete.
DEFINE_REFCOUNT_TYPE(BLEIO_SEQ_HANDLE_DATA);

BLEIO_SEQ_RESULT schedule_write(BLEIO_SEQ_HANDLE_DATA* handle_data, BLEIO_SEQ_INSTRUCTION* instruction, ON_INTERNAL_IO_COMPLETE on_internal_read_complete);
BLEIO_SEQ_RESULT schedule_read(BLEIO_SEQ_HANDLE_DATA* handle_data, BLEIO_SEQ_INSTRUCTION* instruction, ON_INTERNAL_IO_COMPLETE on_internal_read_complete);
BLEIO_SEQ_RESULT schedule_periodic(BLEIO_SEQ_HANDLE_DATA* handle_data, BLEIO_SEQ_INSTRUCTION* instruction, ON_INTERNAL_IO_COMPLETE on_internal_read_complete);
void dec_ref_handle(BLEIO_SEQ_HANDLE_DATA* handle_data);

#endif // BLEIO_SEQ_LINUX_COMMON_H
