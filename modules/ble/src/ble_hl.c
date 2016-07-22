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

typedef struct BLE_HL_HANDLE_DATA_TAG
{
    MESSAGE_BUS_HANDLE      bus;
    STRING_HANDLE           mac_address;
    MODULE_HANDLE           module_handle;
}BLE_HL_HANDLE_DATA;

static bool parse_instruction(const char* type, JSON_Object* instr, BLE_INSTRUCTION* ble_instr, size_t index);
static VECTOR_HANDLE parse_instructions(JSON_Array* instructions);
static bool parse_read_periodic(JSON_Object* instr, BLE_INSTRUCTION* ble_instr);
static bool parse_write(JSON_Object* instr, BLEIO_SEQ_INSTRUCTION_TYPE type, BLE_INSTRUCTION* ble_instr, size_t index);
static void free_instruction(BLE_INSTRUCTION* instr);
static void free_instructions(VECTOR_HANDLE instructions);

static MODULE_HANDLE BLE_HL_Create(MESSAGE_BUS_HANDLE bus, const void* configuration)
{
    BLE_HL_HANDLE_DATA* result;

    /*Codes_SRS_BLE_HL_13_001: [ BLE_HL_Create shall return NULL if the bus or configuration parameters are NULL. ]*/
    if(
        (bus == NULL) ||
        (configuration == NULL)
      )
    {
        LogError("NULL parameter detected bus=%p configuration=%p", bus, configuration);
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
                                    result = (BLE_HL_HANDLE_DATA*)malloc(sizeof(BLE_HL_HANDLE_DATA));
                                    if (result == NULL)
                                    {
                                        /*Codes_SRS_BLE_HL_13_002: [ BLE_HL_Create shall return NULL if any of the underlying platform calls fail. ]*/
                                        LogError("malloc failed");
                                        free_instructions(ble_instructions);
                                        VECTOR_destroy(ble_instructions);
                                    }
                                    else
                                    {
                                        result->mac_address = STRING_construct(mac_address);
                                        if (result->mac_address == NULL)
                                        {
                                            LogError("STRING_create failed");
                                            free_instructions(ble_instructions);
                                            VECTOR_destroy(ble_instructions);
                                            free(result);
                                            result = NULL;
                                        }
                                        else
                                        {
                                            ble_config.device_config.ble_controller_index = controller_index;
                                            ble_config.instructions = ble_instructions;

                                            /*Codes_SRS_BLE_HL_13_014: [ BLE_HL_Create shall call the underlying module's 'create' function. ]*/
                                            result->module_handle = MODULE_STATIC_GETAPIS(BLE_MODULE)()->Module_Create(
                                                bus, (const void*)&ble_config
                                                );
                                            if (result->module_handle == NULL)
                                            {
                                                /*Codes_SRS_BLE_HL_13_022: [ BLE_HL_Create shall return NULL if calling the underlying module's create function fails. ]*/
                                                LogError("Unable to create BLE low level module");
                                                free_instructions(ble_instructions);
                                                VECTOR_destroy(ble_instructions);
                                                STRING_delete(result->mac_address);
                                                free(result);
                                                result = NULL;
                                            }
                                            else
                                            {
                                                /*Codes_SRS_BLE_HL_13_023: [ BLE_HL_Create shall return a non-NULL handle if calling the underlying module's create function succeeds. ]*/
                                                result->bus = bus;
                                            }
                                        }
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
    
    return (MODULE_HANDLE)result;
}

static void free_instruction(BLE_INSTRUCTION* instr)
{
    // free string handle for the characteristic uuid
    if(instr->characteristic_uuid != NULL)
    {
        STRING_delete(instr->characteristic_uuid);
    }

    // free write buffer
    if(
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

static void free_instructions(VECTOR_HANDLE instructions)
{
    size_t len = VECTOR_size(instructions);
    for(size_t i = 0; i < len; ++i)
    {
        BLE_INSTRUCTION* instr = (BLE_INSTRUCTION*)VECTOR_element(instructions, i);
        free_instruction(instr);
    }
}

static bool parse_read_periodic(
    JSON_Object* instr,
    BLE_INSTRUCTION* ble_instr
)
{
    ble_instr->instruction_type = READ_PERIODIC;
    ble_instr->data.interval_in_ms = (uint32_t)json_object_get_number(instr, "interval_in_ms");
    return ble_instr->data.interval_in_ms > 0;
}

static bool parse_write(
    JSON_Object* instr,
    BLEIO_SEQ_INSTRUCTION_TYPE type,
    BLE_INSTRUCTION* ble_instr,
    size_t index
)
{
    bool result;
    ble_instr->instruction_type = type;
    const char* base64_encoded_data = json_object_get_string(instr, "data");
    if(base64_encoded_data == NULL)
    {
        /*Codes_SRS_BLE_HL_13_011: [ BLE_HL_Create shall return NULL if an instruction of type write_at_init or write_at_exit does not have a data property. ]*/
        LogError("json_value_get_string returned NULL for the property 'data' while processing instruction %zu", index);
        result = false;
    }
    else
    {
        ble_instr->data.buffer = Base64_Decoder(base64_encoded_data);
        if(ble_instr->data.buffer == NULL)
        {
            /*Codes_SRS_BLE_HL_13_012: [ BLE_HL_Create shall return NULL if an instruction of type write_at_init or write_at_exit has a data property whose value does not decode successfully from base 64. ]*/
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

static bool parse_instruction(
    const char* type,
    JSON_Object* instr,
    BLE_INSTRUCTION* ble_instr,
    size_t index
)
{
    bool result;
    if(strcmp(type, "read_once") == 0)
    {
        ble_instr->instruction_type = READ_ONCE;
        result = true;
    }
    else if(strcmp(type, "read_periodic") == 0)
    {
        if(parse_read_periodic(instr, ble_instr) == false)
        {
            /*Codes_SRS_BLE_HL_13_010: [ BLE_HL_Create shall return NULL if the interval_in_ms value for a read_periodic instruction isn't greater than zero. ]*/
            LogError("parse_read_periodic returned false while processing instruction %zu", index);
            result = false;
        }
        else
        {
            result = true;
        }
    }
    else if(strcmp(type, "write_at_init") == 0)
    {
        if(parse_write(instr, WRITE_AT_INIT, ble_instr, index) == false)
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
    else if(strcmp(type, "write_at_exit") == 0)
    {
        if(parse_write(instr, WRITE_AT_EXIT, ble_instr, index) == false)
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
        /*Codes_SRS_BLE_HL_13_021: [ BLE_HL_Create shall return NULL if a given instruction's type property is unrecognized. ]*/
        LogError("Unknown instruction type '%s' encountered for instruction number %zu", type, index);
        result = false;
    }
    
    return result;
}

static VECTOR_HANDLE parse_instructions(JSON_Array* instructions)
{
    VECTOR_HANDLE result;
    size_t count = json_array_get_count(instructions);
    if (count == 0)
    {
        /*Codes_SRS_BLE_HL_13_020: [ BLE_HL_Create shall return NULL if the instructions array length is equal to zero. ]*/
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
                    /*Codes_SRS_BLE_HL_13_007: [ BLE_HL_Create shall return NULL if each instruction is not an object. ]*/
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
                        /*Codes_SRS_BLE_HL_13_008: [ BLE_HL_Create shall return NULL if a given instruction does not have a type property. ]*/
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
                            /*Codes_SRS_BLE_HL_13_009: [ BLE_HL_Create shall return NULL if a given instruction does not have a characteristic_uuid property. ]*/
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
                                /*Codes_SRS_BLE_HL_13_002: [ BLE_HL_Create shall return NULL if any of the underlying platform calls fail. ]*/
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
                                        /*Codes_SRS_BLE_HL_13_002: [ BLE_HL_Create shall return NULL if any of the underlying platform calls fail. ]*/
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

static void BLE_HL_Destroy(MODULE_HANDLE module)
{
    if(module != NULL)
    {
        /*Codes_SRS_BLE_HL_13_015: [ BLE_HL_Destroy shall destroy all used resources. ]*/
        BLE_HL_HANDLE_DATA* handle_data = (BLE_HL_HANDLE_DATA*)module;
        MODULE_STATIC_GETAPIS(BLE_MODULE)()->Module_Destroy(handle_data->module_handle);
        STRING_delete(handle_data->mac_address);
        free(handle_data);
    }
    else
    {
        /*Codes_SRS_BLE_HL_13_017: [ BLE_HL_Destroy shall do nothing if module is NULL. ]*/
        LogError("'module' is NULL");
    }
}

static bool recognize_bus_message(BLE_HL_HANDLE_DATA* handle_data, CONSTMAP_HANDLE properties)
{
    bool result;
    const char * message_mac = ConstMap_GetValue(properties, GW_MAC_ADDRESS_PROPERTY);
    if (message_mac != NULL && (strcmp(message_mac, STRING_c_str(handle_data->mac_address)) == 0))
    {
        const char * message_source = ConstMap_GetValue(properties, GW_SOURCE_PROPERTY);
        if ((message_source != NULL) && (strcmp(message_source, GW_IDMAP_MODULE) == 0))
        {
            result = true; /* recognized */
        }
        else
        {
            /*Codes_SRS_BLE_HL_17_004: [ If messageHandle properties does not contain "source" property, then this function shall return. ]*/
            /*Codes_SRS_BLE_HL_17_005: [ If the source of the message properties is not "mapping", then this function shall return. ]*/
            result = false; /* unrecognized */
        }
    }
    else
    {
        /*Codes_SRS_BLE_HL_17_003: [ If macAddress of the message property does not match this module's MAC address, then this function shall return. ]*/
        /*Codes_SRS_BLE_HL_17_002: [ If messageHandle properties does not contain "macAddress" property, then this function shall return. ]*/
        result = false; /* unrecognized */
    }
    return result;
}

static int forward_new_message(BLE_HL_HANDLE_DATA* handle_data, CONSTMAP_HANDLE properties, BLE_INSTRUCTION* ble_instr)
{
    int result;
    /*Codes_SRS_BLE_HL_17_018: [ BLE_HL_Receive shall call ConstMap_CloneWriteable on the message properties. ]*/
    MAP_HANDLE new_message_props = ConstMap_CloneWriteable(properties);
    if (new_message_props != NULL)
    {
        /*Codes_SRS_BLE_HL_17_020: [ BLE_HL_Receive shall call Map_AddOrUpdate with key of "source" and value of "BLE". ]*/
        if (Map_AddOrUpdate(new_message_props, GW_SOURCE_PROPERTY, GW_SOURCE_BLE_COMMAND) == MAP_OK)
        {
            MESSAGE_CONFIG cfg;
            cfg.size = sizeof(BLE_INSTRUCTION);
            cfg.source = (const unsigned char *)ble_instr;
            cfg.sourceProperties = new_message_props;
            /*Codes_SRS_BLE_HL_17_023: [ BLE_HL_Receive shall create a new message by calling Message_Create with new map and BLE_INSTRUCTION as the buffer. ]*/
            MESSAGE_HANDLE new_message_handle = Message_Create(&cfg);
            if (new_message_handle != NULL)
            {
                /*Codes_SRS_BLE_HL_13_018: [ BLE_HL_Receive shall forward new message to the underlying module. ]*/
                MODULE_STATIC_GETAPIS(BLE_MODULE)()->Module_Receive(handle_data->module_handle, new_message_handle);
                Message_Destroy(new_message_handle);
                result = 0;
            }
            else
            {
                /*Codes_SRS_BLE_HL_17_024: [ If creating new message fails, BLE_HL_Receive shall deallocate all resources and return. ]*/
                LogError("Forward message creation failed");
                result = __LINE__;
            }
        }
        else
        {
            LogError("Unable to set properties");
            result = __LINE__;
        }
        Map_Destroy(new_message_props);
    }
    else
    {
        /*Codes_SRS_BLE_HL_17_019: [ If ConstMap_CloneWriteable fails, BLE_HL_Receive shall return. ]*/
        LogError("Unable to get writeable properties");
        result = __LINE__;
    }
    return result;
}

static void BLE_HL_Receive(MODULE_HANDLE module, MESSAGE_HANDLE message_handle)
{
    /*Codes_SRS_BLE_HL_13_016: [ BLE_HL_Receive shall do nothing if module is NULL. ]*/
    /*Codes_SRS_BLE_HL_17_001: [ BLE_HL_Receive shall do nothing if message_handle is NULL. ]*/
    if(module != NULL && message_handle != NULL)
    {
        /*Codes_SRS_BLE_HL_13_018: [ BLE_HL_Receive shall forward the call to the underlying module. ]*/
        BLE_HL_HANDLE_DATA* handle_data = (BLE_HL_HANDLE_DATA*)module;

        CONSTMAP_HANDLE properties = Message_GetProperties(message_handle);
        if (properties != NULL)
        {
            if (recognize_bus_message(handle_data, properties) == true)
            {
                const CONSTBUFFER * message_content = Message_GetContent(message_handle);
                if (message_content != NULL)
                {
                    /*Codes_SRS_BLE_HL_17_006: [ BLE_HL_Receive shall parse the message contents as a JSON object. ]*/
                    /*Codes_SRS_BLE_HL_17_007: [ If the message contents do not parse, then BLE_HL_Receive shall return. ]*/
                    JSON_Value* json = json_parse_string((const char*)(message_content->buffer));
                    if (json != NULL)
                    {
                        JSON_Object* instr = json_value_get_object(json);
                        if (instr != NULL)
                        {
                            const char* type = json_object_get_string(instr, "type");
                            if (type != NULL)
                            {
                                /*Codes_SRS_BLE_HL_17_008: [ BLE_HL_Receive shall return if the JSON object does not contain the following fields: "type", "characteristic_uuid", and "data". ]*/
                                const char* characteristic_uuid = json_object_get_string(instr, "characteristic_uuid");
                                if (characteristic_uuid != NULL)
                                {
                                    BLE_INSTRUCTION ble_instr = { 0 };
                                    /*Codes_SRS_BLE_HL_17_012: [ BLE_HL_Receive shall create a STRING_HANDLE from the characteristic_uuid data field. ]*/
                                    /*Codes_SRS_BLE_HL_17_016: [ BLE_HL_Receive shall set characteristic_uuid to the created STRING. ]*/
                                    ble_instr.characteristic_uuid = STRING_construct(characteristic_uuid);
                                    if (ble_instr.characteristic_uuid != NULL)
                                    {
                                        /*Codes_SRS_BLE_HL_17_014: [ BLE_HL_Receive shall parse the json object to fill in a new BLE_INSTRUCTION. ]*/
                                        if (parse_instruction(type, instr, &ble_instr, 0) == true)
                                        {
                                            if (forward_new_message(handle_data, properties, &ble_instr) != 0)
                                            {
                                                free_instruction(&ble_instr);
                                            }
                                        }
                                        else
                                        {
                                            /*Codes_SRS_BLE_HL_17_026: [ If the json object does not parse, BLE_HL_Receive shall return. ]*/
                                            /*Codes_SRS_BLE_HL_17_008: [ BLE_HL_Receive shall return if the JSON object does not contain the following fields: "type", "characteristic_uuid", and "data". ]*/
                                            LogError("Not a valid BLE instruction");
                                            free_instruction(&ble_instr);
                                        }
                                    }
                                    else
                                    {
                                        /*Codes_SRS_BLE_HL_17_013: [ If the string creation fails, BLE_HL_Receive shall return. ]*/
                                        LogError("Characteristic uuid string creation failed.");
                                    }
                                }
                                else
                                {
                                    LogError("Characteristic uuid not found");
                                }
                            }
                            else
                            {
                                LogError("BLE Instruction type not found");
                            }
                        }
                        else
                        {
                            LogError("JSON Object expected, not received.");
                        }
                        json_value_free(json);
                    }
                    else
                    {
                        LogError("JSON parsing failed");
                    }
                }
                else
                {
                    LogError("No Message Content");
                }
            }
            /*Codes_SRS_BLE_HL_17_025: [ BLE_HL_Receive shall free all resources created. ]*/
            ConstMap_Destroy(properties);
        }
    }
    else
    {
        /*Codes_SRS_BLE_HL_13_016: [ BLE_HL_Receive shall do nothing if module is NULL. ]*/
        LogError("'module' is NULL");
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