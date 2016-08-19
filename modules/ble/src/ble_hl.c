// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <string.h>

#include "azure_c_shared_utility/gballoc.h"

/*because it is linked statically, this include will bring in some uniquely (by convention) named functions*/
#include "ble.h"
#include "ble_hl.h"
#include "ble_utils.h"
#include "ble_instr_utils.h"

#include "azure_c_shared_utility/xlogging.h"
#include "parson.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/base64.h"
#include "messageproperties.h"
#include "message.h"
#include "azure_c_shared_utility/constmap.h"
#include "azure_c_shared_utility/map.h"

static MODULE_HANDLE BLE_HL_Create(BROKER_HANDLE broker, const void* configuration)
{
    MODULE_HANDLE result;

    /*Codes_SRS_BLE_HL_13_001: [ BLE_HL_Create shall return NULL if the broker or configuration parameters are NULL. ]*/
    if(
        (broker == NULL) ||
        (configuration == NULL)
      )
    {
        LogError("NULL parameter detected broker=%p configuration=%p", broker, configuration);
        result = NULL;
    }
    else
    {
        JSON_Value* json = json_parse_string((const char*)configuration);
        if (json == NULL)
        {
            /*Codes_SRS_BLE_HL_13_002: [ BLE_HL_Create shall return NULL if any of the underlying platform calls fail. ]*/
            LogError("unable to json_parse_string");
            result = NULL;
        }
        else
        {
            JSON_Object* root = json_value_get_object(json);
            if (root == NULL)
            {
                /*Codes_SRS_BLE_HL_13_003: [ BLE_HL_Create shall return NULL if the JSON does not start with an object. ]*/
                LogError("unable to json_value_get_object");
                result = NULL;
            }
            else
            {
                // get controller index
                int controller_index = (int)json_object_get_number(root, "controller_index");
                if (controller_index < 0)
                {
                    /*Codes_SRS_BLE_HL_13_005: [ BLE_HL_Create shall return NULL if the controller_index value in the JSON is less than zero. ]*/
                    LogError("Invalid BLE controller index specified");
                    result = NULL;
                }
                else
                {
                    const char* mac_address = json_object_get_string(root, "device_mac_address");
                    if (mac_address == NULL)
                    {
                        /*Codes_SRS_BLE_HL_13_004: [ BLE_HL_Create shall return NULL if there is no device_mac_address property in the JSON. ]*/
                        LogError("json_object_get_string failed for property 'device_mac_address'");
                        result = NULL;
                    }
                    else
                    {
                        JSON_Array* instructions = json_object_get_array(root, "instructions");
                        if (instructions == NULL)
                        {
                            /*Codes_SRS_BLE_HL_13_006: [ BLE_HL_Create shall return NULL if the instructions array does not exist in the JSON. ]*/
                            LogError("json_object_get_array failed for property 'instructions'");
                            result = NULL;
                        }
                        else
                        {
                            VECTOR_HANDLE ble_instructions = parse_instructions(instructions);
                            if (ble_instructions == NULL)
                            {
                                LogError("parse_instructions returned NULL");
                                result = NULL;
                            }
                            else
                            {
                                BLE_CONFIG ble_config;
                                if (parse_mac_address(
                                        mac_address,
                                        &(ble_config.device_config.device_addr)
                                    ) == false)
                                {
                                    /*Codes_SRS_BLE_HL_13_013: [ BLE_HL_Create shall return NULL if the device_mac_address property's value is not a well-formed MAC address. ]*/
                                    LogError("parse_mac_address returned false");
                                    free_instructions(ble_instructions);
                                    VECTOR_destroy(ble_instructions);
                                    result = NULL;
                                }
                                else
                                {
                                    ble_config.device_config.ble_controller_index = controller_index;
                                    ble_config.instructions = ble_instructions;

                                    /*Codes_SRS_BLE_HL_13_014: [ BLE_HL_Create shall call the underlying module's 'create' function. ]*/
                                    result = MODULE_STATIC_GETAPIS(BLE_MODULE)()->Module_Create(
                                        broker, (const void*)&ble_config
                                    );
                                    if (result == NULL)
                                    {
                                        /*Codes_SRS_BLE_HL_13_022: [ BLE_HL_Create shall return NULL if calling the underlying module's create function fails. ]*/
                                        LogError("Unable to create BLE low level module");
                                        free_instructions(ble_instructions);
                                        VECTOR_destroy(ble_instructions);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            json_value_free(json);
        }
    }
    
    /*Codes_SRS_BLE_HL_13_023: [ BLE_HL_Create shall return a non-NULL handle if calling the underlying module's create function succeeds. ]*/
    return result;
}

static void BLE_HL_Destroy(MODULE_HANDLE module)
{
    if(module != NULL)
    {
        /*Codes_SRS_BLE_HL_13_015: [ BLE_HL_Destroy shall destroy all used resources. ]*/
        MODULE_STATIC_GETAPIS(BLE_MODULE)()->Module_Destroy(module);
    }
    else
    {
        /*Codes_SRS_BLE_HL_13_017: [ BLE_HL_Destroy shall do nothing if module is NULL. ]*/
        LogError("'module' is NULL");
    }
}

static void BLE_HL_Receive(MODULE_HANDLE module, MESSAGE_HANDLE message_handle)
{
    /*Codes_SRS_BLE_HL_13_016: [ BLE_HL_Receive shall do nothing if module is NULL. ]*/
    /*Codes_SRS_BLE_HL_17_001: [ BLE_HL_Receive shall do nothing if message_handle is NULL. ]*/
    if(module != NULL && message_handle != NULL)
    {
        /*Codes_SRS_BLE_HL_13_024: [ BLE_HL_Receive shall pass the received parameters to the underlying BLE module's receive function. ]*/
        MODULE_STATIC_GETAPIS(BLE_MODULE)()->Module_Receive(module, message_handle);
    }
    else
    {
        /*Codes_SRS_BLE_HL_13_016: [ BLE_HL_Receive shall do nothing if module is NULL. ]*/
        LogError("'module' and/or 'message_handle' is NULL");
    }
}

/*
 *	Required for all modules: the public API and the designated implementation functions.
 */
static const MODULE_APIS BLE_HL_APIS_all =
{
    BLE_HL_Create,
    BLE_HL_Destroy,
    BLE_HL_Receive
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(BLE_MODULE_HL)(void)
#else
MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void)
#endif
{
    /*Codes_SRS_BLE_HL_13_019: [ Module_GetAPIS shall return a non-NULL pointer to a structure of type MODULE_APIS that has all fields initialized to non-NULL values. ]*/
    return &BLE_HL_APIS_all;
}