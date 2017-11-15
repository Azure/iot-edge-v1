// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef BLE_H
#define BLE_H

#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/strings.h"

#include "module.h"

#include "ble_gatt_io.h"
#include "bleio_seq.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct BLE_INSTRUCTION_TAG
{
    /**
    * The type of instruction this is from the BLEIO_SEQ_INSTRUCTION_TYPE enum.
    */
    BLEIO_SEQ_INSTRUCTION_TYPE  instruction_type;

    /**
    * The GATT characteristic to read from/write to.
    */
    STRING_HANDLE               characteristic_uuid;

    union
    {
        /**
        * If 'instruction_type' is equal to READ_PERIODIC then this
        * value indicates the polling interval in milliseconds.
        */
        uint32_t                interval_in_ms;

        /**
        * If 'instruction_type' is equal to WRITE_AT_INIT or WRITE_AT_EXIT
        * then this is the buffer that is to be written.
        */
        BUFFER_HANDLE           buffer;
    }data;
}BLE_INSTRUCTION;

typedef struct BLE_CONFIG_TAG
{
    BLE_DEVICE_CONFIG   device_config;  // BLE device information
    VECTOR_HANDLE       instructions;   // array of BLE_INSTRUCTION objects to be executed
}BLE_CONFIG;

MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(BLE_MODULE)(MODULE_API_VERSION gateway_api_version);

#ifdef __cplusplus
}
#endif

#endif /*BLE_H*/
