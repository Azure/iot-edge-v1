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

typedef struct READ_CONTEXT_TAG {
    BLEIO_SEQ_HANDLE_DATA* handle_data;
    BLEIO_SEQ_INSTRUCTION* instruction;
    ON_INTERNAL_IO_COMPLETE on_internal_read_complete;
}READ_CONTEXT;

static void on_read_complete(
    BLEIO_GATT_HANDLE bleio_gatt_handle,
    void* read_context,
    BLEIO_GATT_RESULT result,
    const unsigned char* data,
    size_t size
)
{
    // this MUST NOT be NULL
    READ_CONTEXT* context = (READ_CONTEXT*)read_context;
    if (context->handle_data->on_read_complete != NULL)
    {
        if (result != BLEIO_GATT_OK)
        {
            LogError("An error occurred while executing instruction of type %d for characteristic %s",
                context->instruction->instruction_type,
                STRING_c_str(context->instruction->characteristic_uuid)
            );
        }

        BUFFER_HANDLE buffer = (result == BLEIO_GATT_OK) ? BUFFER_create(data, size) : NULL;
        if (buffer == NULL)
        {
            LogError("BUFFER_create failed.");
        }

        /*Codes_SRS_BLEIO_SEQ_13_018: [ When a READ_ONCE or a READ_PERIODIC instruction completes execution this API shall invoke the on_read_complete callback passing in the data that was read along with the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure.]*/
        /*Codes_SRS_BLEIO_SEQ_13_040: [ When a READ_ONCE or a READ_PERIODIC instruction completes execution this API shall invoke the on_read_complete callback passing in the data that was read along with the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure. ]*/
        context->handle_data->on_read_complete(
            (BLEIO_SEQ_HANDLE)(context->handle_data),
            context->instruction->context,
            STRING_c_str(context->instruction->characteristic_uuid),
            context->instruction->instruction_type,
            (result == BLEIO_GATT_OK && buffer != NULL) ? BLEIO_SEQ_OK : BLEIO_SEQ_ERROR,
            buffer
        );
    }

    // invoke the internal complete callback if we have one
    if (context->on_internal_read_complete != NULL)
    {
        context->on_internal_read_complete(context->handle_data, context->instruction);
    }

    // dec ref the handle data
    dec_ref_handle(context->handle_data);
    free(context);
}

BLEIO_SEQ_RESULT schedule_read(
    BLEIO_SEQ_HANDLE_DATA* handle_data,
    BLEIO_SEQ_INSTRUCTION* instruction,
    ON_INTERNAL_IO_COMPLETE on_internal_read_complete
)
{
    BLEIO_SEQ_RESULT result;
    READ_CONTEXT* context = (READ_CONTEXT*)malloc(sizeof(READ_CONTEXT));

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

        // add ref to the handle data object since we now will have an
        // outstanding I/O operation; the reason why we increment the
        // reference here as opposed to when we know that BLEIO_gatt_read_char_by_uuid
        // was successful is because the operation could potentially complete
        // even before we hit the if check after this call and 'on_read_complete'
        // might have run by then in which case it would have done a DEC_REF and
        // the ref counts will be out of whack
        INC_REF(BLEIO_SEQ_HANDLE_DATA, handle_data);

        int read_result = BLEIO_gatt_read_char_by_uuid(
            handle_data->bleio_gatt_handle,
            STRING_c_str(instruction->characteristic_uuid),
            on_read_complete,
            context
        );

        if (read_result != 0)
        {
            /*Codes_SRS_BLEIO_SEQ_13_014: [ BLEIO_Seq_Run shall return BLEIO_SEQ_ERROR if an underlying platform call fails. ]*/
            result = BLEIO_SEQ_ERROR;
            free(context);
            DEC_REF(BLEIO_SEQ_HANDLE_DATA, handle_data);
            LogError("BLEIO_gatt_read_char_by_uuid failed with %d.", read_result);
        }
        else
        {
            result = BLEIO_SEQ_OK;
        }
    }

    return result;
}
