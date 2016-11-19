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
#include "ble_c2d.h"
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

typedef struct BLE_C2D_HANDLE_DATA_TAG
{
    BROKER_HANDLE broker;
}BLE_C2D_HANDLE_DATA;

static MODULE_HANDLE BLE_C2D_Create(BROKER_HANDLE broker, const void* configuration)
{
    BLE_C2D_HANDLE_DATA* result;

    /*Codes_SRS_BLE_CTOD_13_001: [ BLE_C2D_Create shall return NULL if the broker parameter is NULL. ]*/
    if(broker == NULL)
    {
        LogError("NULL parameter detected broker=%p", broker);
        result = NULL;
    }
    else
    {
        result = (BLE_C2D_HANDLE_DATA*)malloc(sizeof(BLE_C2D_HANDLE_DATA));
        if (result == NULL)
        {
            /*Codes_SRS_BLE_CTOD_13_002: [ BLE_C2D_Create shall return NULL if any of the underlying platform calls fail. ]*/
            LogError("malloc failed");
        }
        else
        {
            result->broker = broker;
        }
    }
    
    /*Codes_SRS_BLE_CTOD_13_023: [ BLE_C2D_Create shall return a non-NULL handle when the function succeeds. ]*/
    return (MODULE_HANDLE)result;
}

static void* BLE_C2D_ParseConfigurationFromJson(const char* configuration)
{
    /*Codes_SRS_BLE_CTOD_17_027: [ BLE_C2D_ParseConfigurationFromJson shall return NULL. ]*/
    return NULL;
}

static void BLE_C2D_FreeConfiguration(void* configuration)
{
	/*Codes_SRS_BLE_CTOD_17_028: [ BLE_C2D_FreeConfiguration shall do nothing. ]*/
	return;
}

static void BLE_C2D_Destroy(MODULE_HANDLE module)
{
    if(module != NULL)
    {
        /*Codes_SRS_BLE_CTOD_13_015: [ BLE_C2D_Destroy shall destroy all used resources. ]*/
        free(module);
    }
    else
    {
        /*Codes_SRS_BLE_CTOD_13_017: [ BLE_C2D_Destroy shall do nothing if module is NULL. ]*/
        LogError("'module' is NULL");
    }
}

static bool validate_message(BLE_C2D_HANDLE_DATA* handle_data, CONSTMAP_HANDLE properties)
{
    bool result;
    const char * message_mac = ConstMap_GetValue(properties, GW_MAC_ADDRESS_PROPERTY);
    if (message_mac != NULL)
    {
        const char * message_source = ConstMap_GetValue(properties, GW_SOURCE_PROPERTY);
        if ((message_source != NULL) && (strcmp(message_source, GW_IDMAP_MODULE) == 0))
        {
            result = true; /* recognized */
        }
        else
        {
            /*Codes_SRS_BLE_CTOD_17_004: [ If message_handle properties does not contain "source" property, then this function shall do nothing. ]*/
            /*Codes_SRS_BLE_CTOD_17_005: [ If the source of the message properties is not "mapping", then this function shall do nothing. ]*/
            result = false; /* unrecognized */
        }
    }
    else
    {
        /*Codes_SRS_BLE_CTOD_17_002: [ If message_handle properties does not contain "macAddress" property, then this function shall do nothing. ]*/
        result = false;
    }

    return result;
}

static int publish_instruction(BLE_C2D_HANDLE_DATA* handle_data, CONSTMAP_HANDLE properties, BLE_INSTRUCTION* ble_instr)
{
    int result;

    MAP_HANDLE new_message_props = ConstMap_CloneWriteable(properties);
    if (new_message_props != NULL)
    {
        /*Codes_SRS_BLE_CTOD_17_020: [ BLE_C2D_Receive shall call add a property with key of "source" and value of "bleCommand". ]*/
        if (Map_AddOrUpdate(new_message_props, GW_SOURCE_PROPERTY, GW_SOURCE_BLE_COMMAND) == MAP_OK)
        {
            MESSAGE_CONFIG cfg;
            cfg.size = sizeof(BLE_INSTRUCTION);
            cfg.source = (const unsigned char *)ble_instr;
            cfg.sourceProperties = new_message_props;

            /*Codes_SRS_BLE_CTOD_17_023: [ BLE_C2D_Receive shall create a new message by calling Message_Create with new map and BLE_INSTRUCTION as the buffer. ]*/
            MESSAGE_HANDLE new_message_handle = Message_Create(&cfg);
            if (new_message_handle != NULL)
            {
                /*Codes_SRS_BLE_CTOD_13_018: [ BLE_C2D_Receive shall publish the new message to the broker. ]*/
                if (Broker_Publish(handle_data->broker, (MODULE_HANDLE)handle_data, new_message_handle) != BROKER_OK)
                {
                    LogError("Broker_Publish failed");
                    result = __LINE__;
                }
                else
                {
                    result = 0;
                }

                Message_Destroy(new_message_handle);
            }
            else
            {
                /*Codes_SRS_BLE_CTOD_17_024: [ If creating new message fails, BLE_C2D_Receive shall de-allocate all resources and return. ]*/
                LogError("Message creation failed");
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
        LogError("Unable to get writeable properties");
        result = __LINE__;
    }

    return result;
}

static void BLE_C2D_Receive(MODULE_HANDLE module, MESSAGE_HANDLE message_handle)
{
    if(module != NULL && message_handle != NULL)
    {
        BLE_C2D_HANDLE_DATA* handle_data = (BLE_C2D_HANDLE_DATA*)module;
        CONSTMAP_HANDLE properties = Message_GetProperties(message_handle);
        if (properties != NULL)
        {
            if (validate_message(handle_data, properties) == true)
            {
                const CONSTBUFFER * message_content = Message_GetContent(message_handle);
                if (message_content != NULL)
                {
                    /*Codes_SRS_BLE_CTOD_17_006: [ BLE_C2D_Receive shall parse the message contents as a JSON object. ]*/
                    JSON_Value* json = json_parse_string((const char*)(message_content->buffer));
                    if (json != NULL)
                    {
                        JSON_Object* instr = json_value_get_object(json);
                        if (instr != NULL)
                        {
                            const char* type = json_object_get_string(instr, "type");
                            if (type != NULL)
                            {
                                const char* characteristic_uuid = json_object_get_string(instr, "characteristic_uuid");
                                if (characteristic_uuid != NULL)
                                {
                                    BLE_INSTRUCTION ble_instr = { 0 };

                                    ble_instr.characteristic_uuid = STRING_construct(characteristic_uuid);
                                    if (ble_instr.characteristic_uuid != NULL)
                                    {
                                        /*Codes_SRS_BLE_CTOD_17_014: [ BLE_C2D_Receive shall parse the json object to fill in a new BLE_INSTRUCTION. ]*/
                                        if (parse_instruction(type, instr, &ble_instr, 0) == true)
                                        {
                                            if (publish_instruction(handle_data, properties, &ble_instr) != 0)
                                            {
                                                free_instruction(&ble_instr);
                                            }

                                            /**
                                             * NOTE:
                                             *  We don't free the instruction if the publish is successful because the
                                             *  BLE module will do that. Note that we are passing the string handle for
                                             *  the characteristic UUID and the data buffer (in case of write instructions)
                                             *  as pointers. This means that this won't really work with out-process modules.
                                             */
                                        }
                                        else
                                        {
                                            /*Codes_SRS_BLE_CTOD_17_026: [ If the json object does not parse, BLE_C2D_Receive shall return. ]*/
                                            LogError("Not a valid BLE instruction");
                                            free_instruction(&ble_instr);
                                        }
                                    }
                                    else
                                    {
                                        /*Codes_SRS_BLE_CTOD_13_024: [ BLE_C2D_Receive shall do nothing if an underlying API call fails. ]*/
                                        LogError("Characteristic uuid string creation failed.");
                                    }
                                }
                                else
                                {
                                    /*Codes_SRS_BLE_CTOD_17_008: [ BLE_C2D_Receive shall return if the JSON object does not contain the following fields: "type" and "characteristic_uuid". ]*/
                                    LogError("Characteristic uuid not found");
                                }
                            }
                            else
                            {
                                /*Codes_SRS_BLE_CTOD_17_008: [ BLE_C2D_Receive shall return if the JSON object does not contain the following fields: "type" and "characteristic_uuid". ]*/
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
                        /*Codes_SRS_BLE_CTOD_17_007: [ If the message contents do not parse, then BLE_C2D_Receive shall do nothing. ]*/
                        LogError("JSON parsing failed");
                    }
                }
                else
                {
                    LogError("No Message Content");
                }
            }

            ConstMap_Destroy(properties);
        }
    }
    else
    {
        /*Codes_SRS_BLE_CTOD_13_016: [ BLE_C2D_Receive shall do nothing if module is NULL. ]*/
        /*Codes_SRS_BLE_CTOD_17_001: [ BLE_C2D_Receive shall do nothing if message_handle is NULL. ]*/
        LogError("'module' and/or 'message_handle' is NULL");
    }
}

/*
 *    Required for all modules: the public API and the designated implementation functions.
 */
static const MODULE_API_1 BLE_C2D_APIS_all =
{
    {MODULE_API_VERSION_1},
    BLE_C2D_ParseConfigurationFromJson,
	BLE_C2D_FreeConfiguration,
    BLE_C2D_Create,
    BLE_C2D_Destroy,
    BLE_C2D_Receive,
    NULL
};


#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(BLE_MODULE_C2D)(MODULE_API_VERSION gateway_api_version)
#else
MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version)
#endif
{
    /*Codes_SRS_BLE_CTOD_26_001: [ Module_GetApi shall return a pointer to the MODULE_API structure. ]*/
    (void)gateway_api_version;
    return (const MODULE_API*)&BLE_C2D_APIS_all;
}
