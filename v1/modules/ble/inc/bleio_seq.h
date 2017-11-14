// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef BLEIO_SEQ_H
#define BLEIO_SEQ_H

#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/buffer_.h"
#include "ble_gatt_io.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct BLEIO_SEQ_HANDLE_DATA_TAG* BLEIO_SEQ_HANDLE;

#define BLEIO_SEQ_RESULT_VALUES \
    BLEIO_SEQ_ERROR, \
    BLEIO_SEQ_OK
DEFINE_ENUM(BLEIO_SEQ_RESULT, BLEIO_SEQ_RESULT_VALUES);

#define BLEIO_SEQ_INSTRUCTION_TYPE_VALUES \
    READ_ONCE,     \
    READ_PERIODIC, \
    WRITE_ONCE,    \
    WRITE_AT_INIT, \
    WRITE_AT_EXIT
DEFINE_ENUM(BLEIO_SEQ_INSTRUCTION_TYPE, BLEIO_SEQ_INSTRUCTION_TYPE_VALUES);

typedef struct BLEIO_SEQ_INSTRUCTION_TAG
{
    /**
     * The type of instruction this is from the BLEIO_SEQ_INSTRUCTION_TYPE enum.
     */
    BLEIO_SEQ_INSTRUCTION_TYPE  instruction_type;

    /**
     * The GATT characteristic to read from/write to.
     */
    STRING_HANDLE               characteristic_uuid;

    /**
     * Context data that should be passed back to the callback that is invoked
     * when this instruction completes execution (or for every instance of
     * completion in case this is a recurring instruction).
     */
    void*                       context;

    union
    {
        /**
         * If 'instruction_type' is equal to READ_PERIODIC then this
         * value indicates the polling interval in milliseconds.
         */
        uint32_t                interval_in_ms;

        /**
         * If 'instruction_type' is equal to WRITE_AT_INIT or WRITE_AT_EXIT
         * or WRITE_ONCE then this is the buffer that is to be written.
         */
        BUFFER_HANDLE           buffer;
    }data;
}BLEIO_SEQ_INSTRUCTION;

/**
 * Callback invoked when the sequencer completes a read operation.
 */
typedef void(*ON_BLEIO_SEQ_READ_COMPLETE)(
    BLEIO_SEQ_HANDLE bleio_seq_handle,
    void* context,
    const char* characteristic_uuid,
    BLEIO_SEQ_INSTRUCTION_TYPE type,
    BLEIO_SEQ_RESULT result,
    BUFFER_HANDLE data
);

/**
 * Callback invoked when the sequencer completes a write operation.
 */
typedef void(*ON_BLEIO_SEQ_WRITE_COMPLETE)(
    BLEIO_SEQ_HANDLE bleio_seq_handle,
    void* context,
    const char* characteristic_uuid,
    BLEIO_SEQ_INSTRUCTION_TYPE type,
    BLEIO_SEQ_RESULT result
);

/**
* Callback invoked when the sequence has been destroyed.
*/
typedef void(*ON_BLEIO_SEQ_DESTROY_COMPLETE)(BLEIO_SEQ_HANDLE bleio_seq_handle, void* context);

extern BLEIO_SEQ_HANDLE BLEIO_Seq_Create(
    BLEIO_GATT_HANDLE bleio_gatt_handle,
    VECTOR_HANDLE instructions,
    ON_BLEIO_SEQ_READ_COMPLETE on_read_complete,
    ON_BLEIO_SEQ_WRITE_COMPLETE on_write_complete
);

extern void BLEIO_Seq_Destroy(
    BLEIO_SEQ_HANDLE bleio_seq_handle,
    ON_BLEIO_SEQ_DESTROY_COMPLETE on_destroy_complete,
    void* context
);

extern BLEIO_SEQ_RESULT BLEIO_Seq_Run(BLEIO_SEQ_HANDLE bleio_seq_handle);

extern BLEIO_SEQ_RESULT BLEIO_Seq_AddInstruction(
    BLEIO_SEQ_HANDLE bleio_seq_handle,
    BLEIO_SEQ_INSTRUCTION* instruction
);

#ifdef __cplusplus
}
#endif

#endif // BLEIO_SEQ_H
