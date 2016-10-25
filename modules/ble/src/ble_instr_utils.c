// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/base64.h"
#include "azure_c_shared_utility/xlogging.h"
#include "parson.h"

#include "ble.h"
#include "bleio_seq.h"

void free_instruction(BLE_INSTRUCTION* instr)
{
    // free string handle for the characteristic uuid
    if (instr->characteristic_uuid != NULL)
    {
        STRING_delete(instr->characteristic_uuid);
    }

    // free write buffer
    if (
        (
            instr->instruction_type == WRITE_AT_INIT ||
            instr->instruction_type == WRITE_ONCE ||
            instr->instruction_type == WRITE_AT_EXIT
            )
        &&
        (
            instr->data.buffer != NULL
            )
        )
    {
        BUFFER_delete(instr->data.buffer);
    }
}

void free_instructions(VECTOR_HANDLE instructions)
{
    size_t len = VECTOR_size(instructions);
    for (size_t i = 0; i < len; ++i)
    {
        BLE_INSTRUCTION* instr = (BLE_INSTRUCTION*)VECTOR_element(instructions, i);
        free_instruction(instr);
    }
}

bool parse_read_periodic(
    JSON_Object* instr,
    BLE_INSTRUCTION* ble_instr
)
{
    ble_instr->instruction_type = READ_PERIODIC;
    ble_instr->data.interval_in_ms = (uint32_t)json_object_get_number(instr, "interval_in_ms");
    return ble_instr->data.interval_in_ms > 0;
}

bool parse_write(
    JSON_Object* instr,
    BLEIO_SEQ_INSTRUCTION_TYPE type,
    BLE_INSTRUCTION* ble_instr,
    size_t index
)
{
    bool result;
    ble_instr->instruction_type = type;
    const char* base64_encoded_data = json_object_get_string(instr, "data");
    if (base64_encoded_data == NULL)
    {
        /*Codes_SRS_BLE_05_011: [ BLE_CreateFromJson shall return NULL if an instruction of type write_at_init or write_at_exit does not have a data property. ]*/ 
        LogError("json_value_get_string returned NULL for the property 'data' while processing instruction %zu", index);
        result = false;
    }
    else
    {
        ble_instr->data.buffer = Base64_Decoder(base64_encoded_data);
        if (ble_instr->data.buffer == NULL)
        {
            /*Codes_SRS_BLE_05_012: [ BLE_CreateFromJson shall return NULL if an instruction of type write_at_init or write_at_exit has a data property whose value does not decode successfully from base 64. ]*/ 
            LogError("Base64_Decoder returned NULL for the property 'data' while processing instruction %zu", index);
            result = false;
        }
        else
        {
            result = true;
        }
    }

    return result;
}

bool parse_instruction(
    const char* type,
    JSON_Object* instr,
    BLE_INSTRUCTION* ble_instr,
    size_t index
)
{
    bool result;
    if (strcmp(type, "read_once") == 0)
    {
        ble_instr->instruction_type = READ_ONCE;
        result = true;
    }
    else if (strcmp(type, "read_periodic") == 0)
    {
        if (parse_read_periodic(instr, ble_instr) == false)
        {
            /*Codes_SRS_BLE_05_010: [ BLE_CreateFromJson shall return NULL if the interval_in_ms value for a read_periodic instruction isn't greater than zero. ]*/ 
            LogError("parse_read_periodic returned false while processing instruction %zu", index);
            result = false;
        }
        else
        {
            result = true;
        }
    }
    else if (strcmp(type, "write_at_init") == 0)
    {
        if (parse_write(instr, WRITE_AT_INIT, ble_instr, index) == false)
        {
            LogError("parse_write returned false while processing instruction %zu", index);
            result = false;
        }
        else
        {
            result = true;
        }
    }
    else if (strcmp(type, "write_once") == 0)
    {
        if (parse_write(instr, WRITE_ONCE, ble_instr, index) == false)
        {
            LogError("parse_write returned false while processing instruction %zu", index);
            result = false;
        }
        else
        {
            result = true;
        }
    }
    else if (strcmp(type, "write_at_exit") == 0)
    {
        if (parse_write(instr, WRITE_AT_EXIT, ble_instr, index) == false)
        {
            LogError("parse_write returned false while processing instruction %zu", index);
            result = false;
        }
        else
        {
            result = true;
        }
    }
    else
    {
        /*Codes_SRS_BLE_05_021: [ BLE_CreateFromJson shall return NULL if a given instruction's type property is unrecognized. ]*/
        LogError("Unknown instruction type '%s' encountered for instruction number %zu", type, index);
        result = false;
    }

    return result;
}

VECTOR_HANDLE parse_instructions(JSON_Array* instructions)
{
    VECTOR_HANDLE result;
    size_t count = json_array_get_count(instructions);
    if (count == 0)
    {
        /*Codes_SRS_BLE_05_020: [ BLE_CreateFromJson shall return NULL if the instructions array length is equal to zero. ]*/
        LogError("json_array_get_count returned zero");
        result = NULL;
    }
    else
    {
        result = VECTOR_create(sizeof(BLE_INSTRUCTION));
        if (result != NULL)
        {
            for (size_t i = 0; i < count; ++i)
            {
                JSON_Object* instr = json_array_get_object(instructions, i);
                if (instr == NULL)
                {
                    /*Codes_SRS_BLE_05_007: [ BLE_CreateFromJson shall return NULL if each instruction is not an object. ]*/
                    LogError("json_array_get_object returned NULL for instruction number %zu", i);
                    free_instructions(result);
                    VECTOR_destroy(result);
                    result = NULL;
                    break;
                }
                else
                {
                    const char* type = json_object_get_string(instr, "type");
                    if (type == NULL)
                    {
                        /*Codes_SRS_BLE_05_008: [ BLE_CreateFromJson shall return NULL if a given instruction does not have a type property. ]*/
                        LogError("json_object_get_string returned NULL for the property 'type' for instruction number %zu", i);
                        free_instructions(result);
                        VECTOR_destroy(result);
                        result = NULL;
                        break;
                    }
                    else
                    {
                        const char* characteristic_uuid = json_object_get_string(instr, "characteristic_uuid");
                        if (characteristic_uuid == NULL)
                        {
                            /*Codes_SRS_BLE_05_009: [ BLE_CreateFromJson shall return NULL if a given instruction does not have a characteristic_uuid property. ]*/
                            LogError("json_object_get_string returned NULL for the property 'characteristic_uuid' for instruction number %zu", i);
                            free_instructions(result);
                            VECTOR_destroy(result);
                            result = NULL;
                            break;
                        }
                        else
                        {
                            BLE_INSTRUCTION ble_instr = { 0 };
                            ble_instr.characteristic_uuid = STRING_construct(characteristic_uuid);
                            if (ble_instr.characteristic_uuid == NULL)
                            {
                                /*Codes_SRS_BLE_05_002: [ BLE_CreateFromJson shall return NULL if any of the underlying platform calls fail. ]*/
                                LogError("STRING_construct returned NULL while processing instruction %zu", i);
                                free_instructions(result);
                                VECTOR_destroy(result);
                                result = NULL;
                                break;
                            }
                            else
                            {
                                if (parse_instruction(type, instr, &ble_instr, i) == false)
                                {
                                    LogError("parse_instruction returned false while processing instruction %zu", i);
                                    STRING_delete(ble_instr.characteristic_uuid);
                                    free_instructions(result);
                                    VECTOR_destroy(result);
                                    result = NULL;
                                    break;
                                }
                                else
                                {
                                    // if we get here then we have a valid instruction
                                    if (VECTOR_push_back(result, &ble_instr, 1) != 0)
                                    {
                                        /*Codes_SRS_BLE_05_002: [ BLE_CreateFromJson shall return NULL if any of the underlying platform calls fail. ]*/
                                        LogError("VECTOR_push_back returned a non-zero value while processing instruction %zu", i);
                                        free_instruction(&ble_instr);
                                        free_instructions(result);
                                        VECTOR_destroy(result);
                                        result = NULL;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            LogError("VECTOR_create returned NULL");
        }
    }

    return result;
}
