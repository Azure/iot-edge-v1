// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <glib.h>

#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/refcount.h"

#include "ble_gatt_io.h"
#include "bleio_seq.h"
#include "bleio_seq_linux_common.h"

typedef struct TIMER_CONTEXT_TAG {
    BLEIO_SEQ_HANDLE_DATA*  handle_data;
    BLEIO_SEQ_INSTRUCTION*  instruction;
    ON_INTERNAL_IO_COMPLETE on_internal_read_complete;
}TIMER_CONTEXT;

static gboolean on_timer(gpointer user_data)
{
    TIMER_CONTEXT* context = (TIMER_CONTEXT*)user_data;

    /*Codes_SRS_BLEIO_SEQ_13_009: [ If there are active instructions of type READ_PERIODIC in progress then the timers associated with those instructions shall be cancelled. ]*/
    // we cancel the timer when the sequencer is not in 'running' state
    gboolean result = (context->handle_data->state == BLEIO_SEQ_STATE_RUNNING) ?
        G_SOURCE_CONTINUE : G_SOURCE_REMOVE;

    if (result == G_SOURCE_CONTINUE)
    {
        BLEIO_SEQ_RESULT read_result = schedule_read(
            context->handle_data,
            context->instruction,
            NULL
        );
        if (read_result == BLEIO_SEQ_ERROR)
        {
            LogError("An error occurred while scheduling instruction of type %d for characteristic %s",
                context->instruction->instruction_type,
                STRING_c_str(context->instruction->characteristic_uuid)
            );

            context->handle_data->on_read_complete(
                (BLEIO_SEQ_HANDLE)context->handle_data,
                context->instruction->context,
                STRING_c_str(context->instruction->characteristic_uuid),
                context->instruction->instruction_type,
                read_result,
                NULL
            );
        }
    }
    else
    {
        // invoke the internal complete callback if we have one
        if (context->on_internal_read_complete != NULL)
        {
            context->on_internal_read_complete(context->handle_data, context->instruction);
        }

        // dec ref the handle data
        dec_ref_handle(context->handle_data);
        free(context);
    }

    return result;
}

BLEIO_SEQ_RESULT schedule_periodic(
    BLEIO_SEQ_HANDLE_DATA* handle_data,
    BLEIO_SEQ_INSTRUCTION* instruction,
    ON_INTERNAL_IO_COMPLETE on_internal_read_complete
)
{
    BLEIO_SEQ_RESULT result;
    TIMER_CONTEXT* context = (TIMER_CONTEXT*)malloc(sizeof(TIMER_CONTEXT));

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
        // outstanding timer running that needs access to it;
        //
        // the reason why we increment the reference here as opposed to when
        // we know that 'g_timeout_add' was successful is because
        // the first timer callback could occur even before we hit the if
        // check after this call and 'on-timer' might have run by then
        // in which case it would have done a DEC_REF and the ref counts will be
        // out of whack
        INC_REF(BLEIO_SEQ_HANDLE_DATA, handle_data);

        /*Codes_SRS_BLEIO_SEQ_13_017: [ BLEIO_Seq_Run shall create timers at the specified intervals for scheduling execution of all READ_PERIODIC instructions. ]*/
        /*Codes_SRS_BLEIO_SEQ_13_039: [ BLEIO_Seq_AddInstruction shall create a timer at the specified interval if the instruction is a READ_PERIODIC instruction. ]*/
        guint timer_id = g_timeout_add(
            instruction->data.interval_in_ms,
            on_timer,
            context
        );
        if (timer_id == 0)
        {
            LogError("g_timeout_add failed");
            DEC_REF(BLEIO_SEQ_HANDLE_DATA, handle_data);
            free(context);
            result = BLEIO_SEQ_ERROR;
        }
        else
        {
            result = BLEIO_SEQ_OK;
        }
    }

    return result;
}
