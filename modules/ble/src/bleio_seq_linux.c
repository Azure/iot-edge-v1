// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <string.h>

#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/refcount.h"

#include "ble_gatt_io.h"
#include "bleio_seq.h"
#include "bleio_seq_linux_common.h"

static bool validate_instructions(VECTOR_HANDLE instructions);

BLEIO_SEQ_HANDLE BLEIO_Seq_Create(
    BLEIO_GATT_HANDLE bleio_gatt_handle,
    VECTOR_HANDLE instructions,
    ON_BLEIO_SEQ_READ_COMPLETE on_read_complete,
    ON_BLEIO_SEQ_WRITE_COMPLETE on_write_complete
)
{
    BLEIO_SEQ_HANDLE_DATA* result;
    
    /*Codes_SRS_BLEIO_SEQ_13_001: [ BLEIO_Seq_Create shall return NULL if bleio_gatt_handle is NULL. ]*/
    /*Codes_SRS_BLEIO_SEQ_13_002: [ BLEIO_Seq_Create shall return NULL if instructions is NULL. ]*/
    /*Codes_SRS_BLEIO_SEQ_13_003: [ BLEIO_Seq_Create shall return NULL if the vector instructions is empty. ]*/
    if(
        bleio_gatt_handle == NULL ||
        instructions == NULL ||
        VECTOR_size(instructions) == 0
      )
    {
        LogError("Invalid input");
        result = NULL;
    }
    else
    {
        if (validate_instructions(instructions) == false)
        {
            result = NULL;
        }
        else
        {
            result = REFCOUNT_TYPE_CREATE(BLEIO_SEQ_HANDLE_DATA);
            if (result != NULL)
            {
                /*Codes_SRS_BLEIO_SEQ_13_005: [ BLEIO_Seq_Create shall return a non-NULL handle on successful execution. ]*/
                result->bleio_gatt_handle = bleio_gatt_handle;
                result->state = BLEIO_SEQ_STATE_IDLE;
                result->instructions = instructions;
                result->on_read_complete = on_read_complete;
                result->on_write_complete = on_write_complete;
                result->on_destroy_complete = NULL;
                result->destroy_context = NULL;
            }
            else
            {
                /*Codes_SRS_BLEIO_SEQ_13_004: [ BLEIO_Seq_Create shall return NULL if any of the underlying platform calls fail. ]*/
                /* Do nothing. Let control fall through. */
                LogError("malloc failed");
            }
        }
    }
    
    return (BLEIO_SEQ_HANDLE)result;
}

static bool is_write_instruction(BLEIO_SEQ_INSTRUCTION* instruction)
{
    return
    (
        instruction->instruction_type == WRITE_AT_INIT ||
        instruction->instruction_type == WRITE_AT_EXIT ||
        instruction->instruction_type == WRITE_ONCE
    );
}


static bool validate_instruction(BLEIO_SEQ_INSTRUCTION* instruction)
{
    bool result = true;

    // if the characteristic UUID is NULL/empty then it's an error
    if (instruction->characteristic_uuid == NULL || STRING_length(instruction->characteristic_uuid) == 0)
    {
        /*Codes_SRS_BLEIO_SEQ_13_025: [ BLEIO_Seq_Create shall return NULL if the characteristic_uuid field for any instruction is NULL or empty. ]*/
        /*Codes_SRS_BLEIO_SEQ_13_049: [ BLEIO_Seq_AddInstruction shall return NULL if the characteristic_uuid field for any instruction is NULL or empty. ]*/
        LogError("characteristic_uuid is NULL or empty for instruction.");
        result = false;
    }

    // if this is a read periodic instruction then interval_in_ms should be greater than zero
    else if (instruction->instruction_type == READ_PERIODIC && instruction->data.interval_in_ms == 0)
    {
        /*Codes_SRS_BLEIO_SEQ_13_023: [ BLEIO_Seq_Create shall return NULL if a READ_PERIODIC instruction's interval_in_ms field is zero. ]*/
        /*Codes_SRS_BLEIO_SEQ_13_047: [ BLEIO_Seq_AddInstruction shall return NULL if a READ_PERIODIC instruction's interval_in_ms field is zero. ]*/
        LogError("Instruction is a READ_PERIODIC instruction but interval is zero.");
        result = false;
    }

    // if this is a write instruction then buffer should not be NULL
    else if (
                 is_write_instruction(instruction)
                 &&
                 instruction->data.buffer == NULL
            )
    {
        /*Codes_SRS_BLEIO_SEQ_13_024: [ BLEIO_Seq_Create shall return NULL if a WRITE_AT_INIT or a WRITE_AT_EXIT or a WRITE_ONCE instruction has a NULL value in the buffer field. ]*/
        /*Codes_SRS_BLEIO_SEQ_13_048: [ BLEIO_Seq_AddInstruction shall return NULL if a WRITE_AT_INIT or a WRITE_AT_EXIT or a WRITE_ONCE instruction has a NULL value in the buffer field. ]*/
        LogError("Instruction is a write instruction but buffer is NULL.");
        result = false;
    }
    else
    {
        result = true;
    }

    return result;
}

static bool validate_instructions(VECTOR_HANDLE instructions)
{
    bool result = true;
    size_t size = VECTOR_size(instructions);
    for (size_t i = 0; i < size; i++)
    {
        BLEIO_SEQ_INSTRUCTION* instruction = (BLEIO_SEQ_INSTRUCTION*)VECTOR_element(instructions, i);
        if (instruction == NULL)
        {
            LogError("VECTOR_element return NULL for index %zu.", i);
            result = false;
            break;
        }

        if (validate_instruction(instruction) == false)
        {
            LogError("validate_instruction failed for index %zu.", i);
            result = false;
            break;
        }
    }

    return result;
}

void dec_ref_handle(BLEIO_SEQ_HANDLE_DATA* handle_data)
{
    size_t ref_count = DEC_REF(BLEIO_SEQ_HANDLE_DATA, handle_data);
    if (ref_count == DEC_RETURN_ZERO)
    {
        /*Codes_SRS_BLEIO_SEQ_13_031: [ If on_destroy_complete is not NULL then BLEIO_Seq_Destroy shall invoke on_destroy_complete once all WRITE_AT_EXIT instructions have been executed. ]*/
        // call on_destroy_complete
        if (handle_data->on_destroy_complete != NULL)
        {
            /*Codes_SRS_BLEIO_SEQ_13_032: [ If on_destroy_complete is not NULL then BLEIO_Seq_Destroy shall pass context as-is to on_destroy_complete. ]*/
            handle_data->on_destroy_complete((BLEIO_SEQ_HANDLE)handle_data, handle_data->destroy_context);
        }

        /*Codes_SRS_BLEIO_SEQ_13_006: [ BLEIO_Seq_Destroy shall free all resources associated with the handle once all the pending I/O operations are complete. ]*/
        // free the strings and buffers
        for (size_t i = 0, len = VECTOR_size(handle_data->instructions); i < len; ++i)
        {
            BLEIO_SEQ_INSTRUCTION *instruction = (BLEIO_SEQ_INSTRUCTION *)VECTOR_element(handle_data->instructions, i);
            if (instruction->characteristic_uuid != NULL)
            {
                STRING_delete(instruction->characteristic_uuid);
            }

            if (is_write_instruction(instruction))
            {
                if (instruction->data.buffer != NULL)
                {
                    BUFFER_delete(instruction->data.buffer);
                }
            }
        }

        VECTOR_destroy(handle_data->instructions);
        BLEIO_gatt_destroy(handle_data->bleio_gatt_handle);
        free(handle_data);
    }
}

void BLEIO_Seq_Destroy(
    BLEIO_SEQ_HANDLE bleio_seq_handle,
    ON_BLEIO_SEQ_DESTROY_COMPLETE on_destroy_complete,
    void* context
)
{
    BLEIO_SEQ_HANDLE_DATA* handle_data = (BLEIO_SEQ_HANDLE_DATA*)bleio_seq_handle;

    /*Codes_SRS_BLEIO_SEQ_13_007: [ If bleio_seq_handle is NULL then BLEIO_Seq_Destroy shall do nothing. ]*/
    if (handle_data != NULL)
    {
        // save the destroy complete callback
        handle_data->on_destroy_complete = on_destroy_complete;
        handle_data->destroy_context = context;

        // mark the sequencer as shutting down
        handle_data->state = BLEIO_SEQ_STATE_SHUTTING_DOWN;

        // schedule WRITE_AT_EXIT operations
        size_t size = VECTOR_size(handle_data->instructions);
        for (size_t i = 0; i < size; i++)
        {
            // this MUST NOT be NULL
            BLEIO_SEQ_INSTRUCTION* instruction = (BLEIO_SEQ_INSTRUCTION*)VECTOR_element(handle_data->instructions, i);
            if (instruction->instruction_type == WRITE_AT_EXIT)
            {
                /*Codes_SRS_BLEIO_SEQ_13_011: [ BLEIO_Seq_Destroy shall schedule the execution of all WRITE_AT_EXIT instructions.*/
                if (schedule_write(handle_data, instruction, NULL) != BLEIO_SEQ_OK)
                {
                    LogError("Scheduling WRITE_AT_EXIT instruction at index %zu failed.", i);
                }
            }
        }

        // decrement the reference
        dec_ref_handle(handle_data);
    }
    else
    {
        LogError("bleio_seq_handle is NULL");
    }
}

static BLEIO_SEQ_RESULT schedule_instruction(
    BLEIO_SEQ_HANDLE_DATA* handle_data,
    BLEIO_SEQ_INSTRUCTION* instruction,
    ON_INTERNAL_IO_COMPLETE on_internal_read_complete
)
{
    BLEIO_SEQ_RESULT result;

    switch (instruction->instruction_type)
    {
    case READ_ONCE:
        /*Codes_SRS_BLEIO_SEQ_13_015: [ BLEIO_Seq_Run shall schedule execution of all READ_ONCE instructions. ]*/
        result = schedule_read(handle_data, instruction, on_internal_read_complete);
        break;

    case READ_PERIODIC:
        result = schedule_periodic(handle_data, instruction, on_internal_read_complete);
        break;

    case WRITE_ONCE:
    case WRITE_AT_INIT:
        /*Codes_SRS_BLEIO_SEQ_13_016: [ BLEIO_Seq_Run shall schedule execution of all WRITE_AT_INIT instructions. ]*/
        /*Codes_SRS_BLEIO_SEQ_13_033: [ BLEIO_Seq_Run shall schedule execution of all WRITE_ONCE instructions. ]*/
        result = schedule_write(handle_data, instruction, on_internal_read_complete);
        break;

    case WRITE_AT_EXIT:
        if (VECTOR_push_back(handle_data->instructions, instruction, 1) != 0)
        {
            LogError("VECTOR_push_back failed");
            result = BLEIO_SEQ_ERROR;
        }
        else
        {
            result = BLEIO_SEQ_OK;
        }

        break;

    default:
        LogError("Unknown instruction type");
        result = BLEIO_SEQ_ERROR;
        break;
    }

    return result;
}

BLEIO_SEQ_RESULT BLEIO_Seq_Run(BLEIO_SEQ_HANDLE bleio_seq_handle)
{
    BLEIO_SEQ_RESULT result;

    /*Codes_SRS_BLEIO_SEQ_13_010: [ BLEIO_Seq_Run shall return BLEIO_SEQ_ERROR if bleio_seq_handle is NULL. ]*/
    if (bleio_seq_handle == NULL)
    {
        LogError("bleio_seq_handle  is NULL");
        result = BLEIO_SEQ_ERROR;
    }
    else
    {
        BLEIO_SEQ_HANDLE_DATA* handle_data = (BLEIO_SEQ_HANDLE_DATA*)bleio_seq_handle;

        /*Codes_SRS_BLEIO_SEQ_13_013: [ BLEIO_Seq_Run shall return BLEIO_SEQ_ERROR if BLEIO_Seq_Run was previously called on this handle. ]*/
        if (handle_data->state != BLEIO_SEQ_STATE_IDLE)
        {
            LogError("Sequence should be in state BLEIO_SEQ_STATE_IDLE but was found to be in: %d", handle_data->state);
            result = BLEIO_SEQ_ERROR;
        }
        else
        {
            // update the state of the sequence; we do this ahead of starting the scheduling
            // process because it is possible that an async I/O operation completes execution
            // even before the 'schedule_*' function returns in which case the callbacks for
            // I/O operation might think that the sequence is in an 'idle' state when it is
            // actually 'running'
            handle_data->state = BLEIO_SEQ_STATE_RUNNING;

            for (size_t i = 0, size = VECTOR_size(handle_data->instructions); i < size; i++)
            {
                // this MUST NOT be NULL
                BLEIO_SEQ_INSTRUCTION* instruction = (BLEIO_SEQ_INSTRUCTION*)VECTOR_element(handle_data->instructions, i);

                // schedule this instruction only if it is not a WRITE_AT_EXIT instruction
                if (instruction->instruction_type != WRITE_AT_EXIT)
                {
                    result = schedule_instruction(handle_data, instruction, NULL);

                    // bail if a schedule operation didn't succeed
                    if (result != BLEIO_SEQ_OK)
                    {
                        LogError("An error occurred while scheduling an instruction of type %d for characteristic %s",
                            instruction->instruction_type, STRING_c_str(instruction->characteristic_uuid));
                        break;
                    }
                }
            }
        }

        // update the state of the sequence again depending on whether the scheduling process
        // worked or not
        handle_data->state = (result == BLEIO_SEQ_OK) ? BLEIO_SEQ_STATE_RUNNING : BLEIO_SEQ_STATE_ERROR;
    }

    return result;
}

static void on_instruction_complete(
    BLEIO_SEQ_HANDLE_DATA* bleio_seq_handle,
    BLEIO_SEQ_INSTRUCTION* instruction
)
{
    // free the characteristic UUID
    STRING_delete(instruction->characteristic_uuid);

    /**
     * NOTE: We don't free the BUFFER_HANDLE for a write instruction
     * because the caller of this callback should have done that in
     * bleio_seq_linux_schedule_write.c.
     */

    // free the instruction
    free(instruction);
}

BLEIO_SEQ_RESULT BLEIO_Seq_AddInstruction(
    BLEIO_SEQ_HANDLE bleio_seq_handle,
    BLEIO_SEQ_INSTRUCTION* instruction
)
{
    BLEIO_SEQ_RESULT result;

    /*Codes_SRS_BLEIO_SEQ_13_036: [ BLEIO_Seq_AddInstruction shall return BLEIO_SEQ_ERROR if bleio_seq_handle is NULL. ]*/
    /*Codes_SRS_BLEIO_SEQ_13_046: [ BLEIO_Seq_AddInstruction shall return BLEIO_SEQ_ERROR if instruction is NULL. ]*/
    if (
            bleio_seq_handle == NULL ||
            instruction == NULL ||
            instruction->characteristic_uuid == NULL ||
            validate_instruction(instruction) == false
       )
    {
        LogError("Invalid instruction provided");
        result = BLEIO_SEQ_ERROR;
    }
    else
    {
        BLEIO_SEQ_HANDLE_DATA* handle_data = (BLEIO_SEQ_HANDLE_DATA*)bleio_seq_handle;

        /*Codes_SRS_BLEIO_SEQ_13_045: [ BLEIO_Seq_AddInstruction shall return BLEIO_SEQ_ERROR if BLEIO_Seq_Run was NOT called first. ]*/
        if (handle_data->state != BLEIO_SEQ_STATE_RUNNING)
        {
            LogError("Sequence should be in state BLEIO_SEQ_STATE_RUNNING but was found to be in: %d", handle_data->state);
            result = BLEIO_SEQ_ERROR;
        }
        else
        {
            // copy the instruction into a new struct
            BLEIO_SEQ_INSTRUCTION* instr = (BLEIO_SEQ_INSTRUCTION*)malloc(sizeof(BLEIO_SEQ_INSTRUCTION));
            if (instr == NULL)
            {
                /*Codes_SRS_BLEIO_SEQ_13_037: [ BLEIO_Seq_AddInstruction shall return BLEIO_SEQ_ERROR if an underlying platform call fails. ]*/
                LogError("malloc failed");
                result = BLEIO_SEQ_ERROR;
            }
            else
            {
                instr->instruction_type = instruction->instruction_type;
                instr->characteristic_uuid = instruction->characteristic_uuid;
                instr->context = instruction->context;
                instr->data = instruction->data;

                // we save the result of this condition in a boolean because after the
                // 'schedule_instruction' call below, there is no guarantee that "instr"
                // is valid anymore because the instruction might get executed and then
                // freed (in 'on_instruction_complete') by the time we hit the next line
                // (in case of Linux, if we're using GLib however, our use of GLib loops
                // *does* in fact guarantee that nothing will happen in parallel on this
                // thread but we might potentially be working with other threading// models)
                bool is_write_at_exit_instr = (instr->instruction_type == WRITE_AT_EXIT);

                /*Codes_SRS_BLEIO_SEQ_13_038: [ BLEIO_Seq_AddInstruction shall schedule execution of the instruction. ]*/
                LogInfo("Scheduling a new instruction.");
                result = schedule_instruction(handle_data, instr, on_instruction_complete);
                if (result != BLEIO_SEQ_OK)
                {
                    free(instr);
                    LogError("An error occurred while scheduling an instruction of type %d for characteristic %s",
                        instruction->instruction_type, STRING_c_str(instruction->characteristic_uuid));
                }
                else
                {
                    // if this is a WRITE_AT_EXIT instruction then it has been added to the instructions vector
                    // so that it is run when the sequence is destroyed; so we should free the memory allocated
                    // for "instr" since the vector maintains its own copy of the instruction
                    if (is_write_at_exit_instr == true)
                    {
                        free(instr);
                    }
                }
            }
        }
    }

    return result;
}
