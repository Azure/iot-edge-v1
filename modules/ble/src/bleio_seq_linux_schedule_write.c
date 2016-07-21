// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/refcount.h"

#include "ble_gatt_io.h"
#include "bleio_seq.h"
#include "bleio_seq_linux_common.h"

typedef struct WRITE_CONTEXT_TAG {
    BLEIO_SEQ_HANDLE_DATA* handle_data;
    BLEIO_SEQ_INSTRUCTION* instruction;
    ON_INTERNAL_IO_COMPLETE on_internal_read_complete;
}WRITE_CONTEXT;

static void on_write_complete(BLEIO_GATT_HANDLE bleio_gatt_handle, void* write_context, BLEIO_GATT_RESULT result)
{
    // this MUST NOT be NULL
    WRITE_CONTEXT* context = (WRITE_CONTEXT*)write_context;
    if (context->handle_data->on_write_complete != NULL)
    {
        if (result != BLEIO_GATT_OK)
        {
            LogError("An error occurred while executing instruction of type %d for characteristic %s",
                context->instruction->instruction_type,
                STRING_c_str(context->instruction->characteristic_uuid)
            );
        }

        /*Codes_SRS_BLEIO_SEQ_13_021: [ When the WRITE_AT_EXIT instruction completes execution this API shall invoke the on_write_complete callback passing in the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure. ]*/
        /*Codes_SRS_BLEIO_SEQ_13_020: [ When the WRITE_AT_INIT instruction completes execution this API shall invoke the on_write_complete callback passing in the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure. ]*/
        /*Codes_SRS_BLEIO_SEQ_13_034: [ When the WRITE_ONCE instruction completes execution this API shall invoke the on_write_complete callback passing in the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure. ]*/
        /*Codes_SRS_BLEIO_SEQ_13_042: [ When a WRITE_ONCE or a WRITE_AT_INIT instruction completes execution this API shall invoke the on_write_complete callback passing in the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure. ]*/
        context->handle_data->on_write_complete(
            (BLEIO_SEQ_HANDLE)context->handle_data,
            context->instruction->context,
            STRING_c_str(context->instruction->characteristic_uuid),
            context->instruction->instruction_type,
            result == BLEIO_GATT_OK ? BLEIO_SEQ_OK : BLEIO_SEQ_ERROR
        );
    }

    /*Codes_SRS_BLEIO_SEQ_13_026: [ When the WRITE_AT_INIT instruction completes execution this API shall free the buffer that was passed in via the instruction. ]*/
    /*Codes_SRS_BLEIO_SEQ_13_041: [ When a WRITE_AT_INIT or a WRITE_ONCE instruction completes execution this API shall free the buffer that was passed in via the instruction. ]*/
    /*Codes_SRS_BLEIO_SEQ_13_027: [ When the WRITE_AT_EXIT instruction completes execution this API shall free the buffer that was passed in via the instruction. ]*/
    /*Codes_SRS_BLEIO_SEQ_13_035: [ When the WRITE_ONCE instruction completes execution this API shall free the buffer that was passed in via the instruction. ]*/
    // free the buffer that was written
    BUFFER_delete(context->instruction->data.buffer);
    context->instruction->data.buffer = NULL;

    // invoke the internal complete callback if we have one
    if (context->on_internal_read_complete != NULL)
    {
        context->on_internal_read_complete(context->handle_data, context->instruction);
    }

    // dec ref the handle data
    // NOTE: The call below MUST occur *after* the BUFFER_delete call above
    // because `dec_ref_handle` might end up destroying the sequence itself
    // at which point context->instruction no longer exists (because it is
    // simply a pointer to the instructions vector in the sequence).
    dec_ref_handle(context->handle_data);

    free(context);
}

BLEIO_SEQ_RESULT schedule_write(
    BLEIO_SEQ_HANDLE_DATA* handle_data,
    BLEIO_SEQ_INSTRUCTION* instruction,
    ON_INTERNAL_IO_COMPLETE on_internal_read_complete
)
{
    BLEIO_SEQ_RESULT result;
    WRITE_CONTEXT* context = (WRITE_CONTEXT*)malloc(sizeof(WRITE_CONTEXT));

    /*Codes_SRS_BLEIO_SEQ_13_014: [ BLEIO_Seq_Run shall return BLEIO_SEQ_ERROR if an underlying platform call fails. ]*/
    if (context == NULL)
    {
        LogError("malloc failed");
        result = BLEIO_SEQ_ERROR;
    }
    else
    {
        context->handle_data = handle_data;
        context->instruction = instruction;
        context->on_internal_read_complete = on_internal_read_complete;

        const unsigned char* buffer = BUFFER_u_char(instruction->data.buffer);
        size_t buffer_size = BUFFER_length(instruction->data.buffer);

        // add ref to the handle data object since we now will have an
        // outstanding I/O operation; the reason why we increment the
        // reference here as opposed to when we know that BLEIO_gatt_read_char_by_uuid
        // was successful is because the operation could potentially complete
        // even before we hit the if check after this call and 'on_read_complete'
        // might have run by then in which case it would have done a DEC_REF and
        // the ref counts will be out of whack
        INC_REF(BLEIO_SEQ_HANDLE_DATA, handle_data);

        int write_result = BLEIO_gatt_write_char_by_uuid(
            handle_data->bleio_gatt_handle,
            STRING_c_str(instruction->characteristic_uuid),
            buffer,
            buffer_size,
            on_write_complete,
            context
        );
        if (write_result != 0)
        {
            /*Codes_SRS_BLEIO_SEQ_13_014: [ BLEIO_Seq_Run shall return BLEIO_SEQ_ERROR if an underlying platform call fails. ]*/
            result = BLEIO_SEQ_ERROR;
            free(context);
            DEC_REF(BLEIO_SEQ_HANDLE_DATA, handle_data);
            LogError("BLEIO_gatt_write_char_by_uuid failed with %d.", write_result);
        }
        else
        {
            result = BLEIO_SEQ_OK;
        }
    }

    return result;
}
