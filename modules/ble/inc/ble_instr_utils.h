// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef BLE_INSTR_UTILS_H
#define BLE_INSTR_UTILS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdlib.h>

#include "azure_c_shared_utility/vector.h"
#include "parson.h"

#include "ble.h"
#include "bleio_seq.h"

    bool parse_instruction(const char* type, JSON_Object* instr, BLE_INSTRUCTION* ble_instr, size_t index);
    VECTOR_HANDLE parse_instructions(JSON_Array* instructions);
    bool parse_read_periodic(JSON_Object* instr, BLE_INSTRUCTION* ble_instr);
    bool parse_write(JSON_Object* instr, BLEIO_SEQ_INSTRUCTION_TYPE type, BLE_INSTRUCTION* ble_instr, size_t index);
    void free_instruction(BLE_INSTRUCTION* instr);
    void free_instructions(VECTOR_HANDLE instructions);

#ifdef __cplusplus
}
#endif

#endif /*BLE_INSTR_UTILS_H*/

