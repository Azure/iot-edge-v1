// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"

#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/buffer_.h"
#include "ble_gatt_io.h"
#include "bleio_seq.h"

BLEIO_SEQ_HANDLE BLEIO_Seq_Create(
    BLEIO_GATT_HANDLE bleio_gatt_handle,
    VECTOR_HANDLE instructions,
    ON_BLEIO_SEQ_READ_COMPLETE on_read_complete,
    ON_BLEIO_SEQ_WRITE_COMPLETE on_write_complete
)
{
    /*Codes_SRS_BLEIO_SEQ_13_028: [On Windows, this function shall return NULL.]*/
    LogError("BLEIO_Seq_Create not implemented on Windows yet.");
    return NULL;
}

void BLEIO_Seq_Destroy(
    BLEIO_SEQ_HANDLE bleio_seq_handle,
    ON_BLEIO_SEQ_DESTROY_COMPLETE on_destroy_complete,
    void* context
)
{
    /*Codes_SRS_BLEIO_SEQ_13_029: [ On Windows, this function shall do nothing. ]*/
    LogError("BLEIO_Seq_Destroy not implemented on Windows yet.");
}

BLEIO_SEQ_RESULT BLEIO_Seq_Run(BLEIO_SEQ_HANDLE bleio_seq_handle)
{
    /*Codes_SRS_BLEIO_SEQ_13_030: [ On Windows this function shall return BLEIO_SEQ_ERROR. ]*/
    LogError("BLEIO_Seq_Run not implemented on Windows yet.");
    return BLEIO_SEQ_ERROR;
}

BLEIO_SEQ_RESULT BLEIO_Seq_AddInstruction(
    BLEIO_SEQ_HANDLE bleio_seq_handle,
    BLEIO_SEQ_INSTRUCTION* instruction
)
{
    /*Codes_SRS_BLEIO_SEQ_13_044: [ On Windows this function shall return BLEIO_SEQ_ERROR. ]*/
    return BLEIO_SEQ_ERROR;
}