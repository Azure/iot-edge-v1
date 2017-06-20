// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include <glib.h>

#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/constmap.h"
#include "azure_c_shared_utility/constbuffer.h"

#include "module.h"
#include "message.h"
#include "broker.h"
#include "module_loader.h"

#include "filter.h"

#include <parson.h>

typedef struct FILTER_RESOLVER_HANDLE_DATA_TAG
{
    BROKER_HANDLE broker;
    int flag;
    int availables;
    char* macAddress;
    void* nextEntry;
} FILTER_RESOLVER_HANDLE_DATA;

typedef void (*MESSAGE_RESOLVER)(FILTER_RESOLVER_HANDLE_DATA* handle, const char* name, const char* buffer);


typedef struct DIPATCH_ENTRY_TAG
{
    const char* name;
    const char* characteristic_uuid;
    MESSAGE_RESOLVER message_resolver;
}DIPATCH_ENTRY;

static void resolve_default(FILTER_RESOLVER_HANDLE_DATA* handle, const char* name, const CONSTBUFFER* buffer)
{
    (void)handle;
    
	STRING_HANDLE result = STRING_construct("\"");
	STRING_concat(result, name);
	STRING_concat(result, "\":\"");
	STRING_HANDLE rawData = getRawData(buffer->buffer, buffer->size);
	sprintf(resolveTempBuf, "%s", STRING_c_str(rawData));
	STRING_concat(result, resolveTempBuf);
	STRING_concat(result, "\"");
	return result;
}



MODULE_HANDLE FILTER_Create(BROKER_HANDLE broker, const void* configuration)
{
    FILTER_RESOLVER_HANDLE_DATA* result = NULL;
    if(broker == NULL)
    {
        LogError("NULL parameter detected broker=%p", broker);
        result = NULL;
    }
    else
    {
        TICC2650_CONFIG* config = (TICC2650_CONFIG*)configuration;
        FILTER_RESOLVER_HANDLE_DATA* lastResult = NULL;
        while (config!=NULL){
            FILTER_RESOLVER_HANDLE_DATA* cresult = (FILTER_RESOLVER_HANDLE_DATA*)malloc(sizeof(FILTER_RESOLVER_HANDLE_DATA));
            if (cresult == NULL)
            {
                /*Codes_SRS_BLE_CTOD_13_002: [ FILTER_RESOLVER_Create shall return NULL if any of the underlying platform calls fail. ]*/
                LogError("malloc failed");
            }
            else
            {
                cresult->broker = broker;
                cresult->flag = 0x00000000;
//                if (configuration!=NULL){
  //                  TICC2650_CONFIG* config = (TICC2650_CONFIG*)configuration;
                cresult->availables = config->availables;
            //    }
                cresult->macAddress = (char*)malloc(strlen(config->macAddress)+1);
                strcpy(cresult->macAddress, config->macAddress);
                if(result==NULL){
                    result = cresult;
                }
                else {
                    lastResult->nextEntry = cresult;
                }
                cresult->nextEntry = NULL;
                lastResult = cresult;
            }
            config = config->nextEntry;
        }
    }
    
    /*Codes_SRS_BLE_CTOD_13_023: [ FILTER_RESOLVER_Create shall return a non-NULL handle when the function succeeds. ]*/

    return (MODULE_HANDLE)result;
}

/*
     "args": {
        "sensor-types":[
          {"sensor-type":"temperature"},
          {"sensor-type":"movement", "config":"4G"} <- config is optional
        ]
      }
 return TICC2650_Config
*/
void* FILTER_ParseConfigurationFromJson(const char* configuration)
{
    TICC2650_CONFIG* configTop = NULL;
    TICC2650_CONFIG* config = NULL;
    if (configuration!=NULL){
        JSON_Value* json = json_parse_string((const char*)configuration);
        if (json == NULL)
        {
            // error
            LogError("Failed to parse json configuration");
        }
        else {
            JSON_Array* targets = json_value_get_array(json);
            if (targets==NULL){
                // error
                LogError("Failed to parse json configuration");
            }
            else {
                size_t tcount = json_array_get_count(targets);
                for(size_t t=0;t<tcount;t++)
                {
                    JSON_Object* target = json_array_get_object(targets, t);
                    // this should be sensor-tag, sensor-SENSOR_TYPES_JSON
                    TICC2650_CONFIG* configTemp = (TICC2650_CONFIG*)malloc(sizeof(TICC2650_CONFIG));
                    if (configTop==NULL)
                    {
                        configTop = configTemp;
                    }
                    else
                    {
                        config->nextEntry = configTemp;
                    }
                    config = configTemp;
                    config->availables = 0;
                    const char* macAddress = json_object_get_string(target,SENSOR_TAG_JSON);
                    config->macAddress = (char*)malloc(strlen(macAddress)+1);
                    strcpy(config->macAddress, macAddress);
                    config->accRange = 0;
                    JSON_Array* sensorTypes = json_object_get_array(target,SENSOR_TYPES_JSON);
                    if (sensorTypes==NULL){
                        // error
                        LogError("Can't find 'sensor-types' specification.");
                    }
                    else{
                        size_t count = json_array_get_count(sensorTypes);
                        for (size_t i=0;i<count;i++){
                            JSON_Object* sensorType = json_array_get_object(sensorTypes,i);
                            if (sensorType==NULL){
                                // error
                                LogError("Can't find 'sensor-type' specification.");
                            }
                            const char* typeName = json_object_get_string(sensorType,SENSOR_TYPE_JSON);
                            if (typeName == NULL){
                                // error
                                LogError("Can't find 'sensor-type' value.");
                            }
                            else {
                                if (g_ascii_strcasecmp(typeName, SENSOR_TYPE_TEMPERATURE)==0){
                                    config->availables |= SENSOR_MASK_TEMPERATURE;
                                } else if (g_ascii_strcasecmp(typeName, SENSOR_TYPE_HUMIDITY)==0){
                                    config->availables |= SENSOR_MASK_HUMIDITY;
                                } else if (g_ascii_strcasecmp(typeName, SENSOR_TYPE_PRESSURE)==0){
                                    config->availables |= SENSOR_MASK_PRESSURE;                                
                                } else if (g_ascii_strcasecmp(typeName, SENSOR_TYPE_MOVEMENT)==0){
                                    config->availables |= SENSOR_MASK_MOVEMENT;
                                    const char* movementConfig = json_object_get_string(sensorType,SENSOR_CONFIG_JSON);
                                    if (movementConfig!=NULL){
                                        // set accRange
                                    }
                                }else if (g_ascii_strcasecmp(typeName, SENSOR_TPYE_BRIGHTNESS)==0){
                                    config->availables |= SENSOR_MASK_BRIGHTNESS;
                                }
                                else {
                                    // error
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return configTop;
}

void FILTER_FreeConfiguration(void * configuration)
{
    if (configuration!=NULL){
        free(configuration);
    }
    return;
}

#define MSG_TMP_BUF_SIZE 1024

void FILTER_Receive(MODULE_HANDLE module, MESSAGE_HANDLE message)
{
    FILTER_RESOLVER_HANDLE_DATA* handle;
    if (module == NULL){
        LogError("module is NULL");
    }
    else{
        handle = (FILTER_RESOLVER_HANDLE_DATA*)module;
        if (message != NULL)
        {
            CONSTMAP_HANDLE props = Message_GetProperties(message);
            if (props != NULL)
            {
                const char* source = ConstMap_GetValue(props, GW_SOURCE_PROPERTY);
                if (source != NULL && strcmp(source, GW_SOURCE_BLE_TELEMETRY) == 0)
                {
                    handle = (FILTER_RESOLVER_HANDLE_DATA*)module;
               //     const char* ble_controller_id = ConstMap_GetValue(props, GW_BLE_CONTROLLER_INDEX_PROPERTY);
                    const char* mac_address_str = ConstMap_GetValue(props, GW_MAC_ADDRESS_PROPERTY);
                    const char* timestamp = ConstMap_GetValue(props, GW_TIMESTAMP_PROPERTY);
                    const char* characteristic_uuid = ConstMap_GetValue(props, GW_CHARACTERISTIC_UUID_PROPERTY);
                    const CONSTBUFFER* buffer = Message_GetContent(message);
                    if (buffer != NULL && characteristic_uuid != NULL)
                    {
                        FILTER_RESOLVER_HANDLE_DATA* targetHandle=NULL;
                        // dispatch the message based on the characteristic uuid
                        size_t i;
                        for (i = 0; i < g_dispatch_entries_length; i++)
                        {
                            if (g_ascii_strcasecmp(
                                    characteristic_uuid,
                                    g_dispatch_entries[i].characteristic_uuid
                                ) == 0)
                            {
                                FILTER_RESOLVER_HANDLE_DATA* currentHandle=handle;
                                while (currentHandle!=NULL){
                                    if(g_ascii_strcasecmp(mac_address_str, currentHandle->macAddress)==0)
                                    {
                                        g_dispatch_entries[i].message_resolver(
                                            currentHandle,
                                            g_dispatch_entries[i].name,
                                            buffer
                                        );
                                        targetHandle = currentHandle;
                                         break;
                                    }
                                    currentHandle = currentHandle->nextEntry;
                                }
                            }
                            if (targetHandle!=NULL){
                                break;
                            }
                    if (i == g_dispatch_entries_length)
                    {
                        // dispatch to default printer
                        resolve_default(handle, characteristic_uuid, buffer);
                    }
                    if (targetHandle!=NULL){
                        handle=targetHandle;
                        if((handle->flag & handle->availables)==handle->availables)
                        {
                            JSON_Value* json_init = json_value_init_object();
                            JSON_Object* json = json_value_get_object(json_init);
                            JSON_Status json_status = json_object_set_string(json, "time", timestamp);
                            if ((handle->flag&SENSOR_MASK_TEMPERATURE)!=0) {
                                json_status = json_object_set_number(json, "ambience", handle->lastAmbience);
                                json_status= json_object_set_number(json,"object", handle->lastObject);
                            }
                            if ((handle->flag&SENSOR_MASK_HUMIDITY)!=0){
                                json_status = json_object_set_number(json, "humidity", handle->lastHumidity);
                                json_status = json_object_set_number(json, "humidityTemperature", handle->lastHumidityTemperature);
                            }
                            if ((handle->flag&SENSOR_MASK_PRESSURE)!=0){
                                json_status = json_object_set_number(json, "pressure", handle->lastPressure);
                                json_status = json_object_set_number(json, "pressureTemperature", handle->lastPressureTemperature);
                            }
                            if ((handle->flag & SENSOR_MASK_MOVEMENT)!=0){
                                json_status = json_object_set_number(json, "accelx", handle->lastAccelX);
                                json_status = json_object_set_number(json, "accely", handle->lastAccelY);
                                json_status = json_object_set_number(json, "accelz", handle->lastAccelZ);
                                json_status = json_object_set_number(json, "gyrox", handle->lastGyroX);
                                json_status = json_object_set_number(json, "gyroy", handle->lastGyroY);
                                json_status = json_object_set_number(json, "gyroz", handle->lastGyroZ);
                                json_status = json_object_set_number(json, "magx", handle->lastMagX);
                                json_status = json_object_set_number(json, "magy", handle->lastMagY);
                                json_status = json_object_set_number(json, "magz", handle->lastMagZ);
                            }
                            if ((handle->flag & SENSOR_MASK_BRIGHTNESS)!=0){
                                json_status = json_object_set_number(json, "brightness", handle->lastBrightness);
                            }

                            char* jsonContent = json_serialize_to_string_pretty(json_init);
                            // I don't know how to remove /'s prefix.
                             char* tmpBuf = (char*)malloc(strlen(jsonContent));
                            int tmpi=0;
                            int tmpj=0;
                            while (jsonContent[tmpi]!='\0'){
                                if (jsonContent[tmpi]==0x5c&&jsonContent[tmpi+1]==0x2f){
                                    tmpBuf[tmpj] = jsonContent[tmpi+1];
                                    tmpi++;
                                }
                                else {
                                    tmpBuf[tmpj] = jsonContent[tmpi];
                                }
                                tmpi++; tmpj++;
                            }
                            tmpBuf[tmpj] = '\0';
                            CONSTBUFFER_HANDLE msgContent = CONSTBUFFER_Create((const unsigned char*)tmpBuf, strlen(tmpBuf));
                            MAP_HANDLE msgProps = ConstMap_CloneWriteable(props);
                            MESSAGE_BUFFER_CONFIG msgBufConfig =  {
                                msgContent,
                                msgProps
                            };
                            MESSAGE_HANDLE newMessage = Message_CreateFromBuffer(&msgBufConfig);
                            if (newMessage == NULL)
                            {
                                LogError("Message_CreateFromBuffer() failed");
                            }
                            else
                            {
                                if (Broker_Publish(handle->broker, (MODULE_HANDLE)handle, newMessage) != BROKER_OK)
                                {
                                   LogError("Broker_Publish() failed");
                                }
                                handle->flag = 0;
                                Message_Destroy(newMessage);
                           }
                           json_free_serialized_string(jsonContent);
                           json_value_free(json_init);
                            
                           CONSTBUFFER_Destroy(msgContent);
                           free(tmpBuf);

              //      ConstMap_Destory(props);
                    }
                    }
                }
                }
                else
                {
                    LogError("Message is invalid. Nothing to print.");
                }
            }
        }
        else
        {
            LogError("Message_GetProperties for the message returned NULL");
        }
    }
    else
    {
        LogError("message is NULL");
    }
}
}

void FILTER_Destroy(MODULE_HANDLE module)
{
    (void)module;
    // Nothing to do here
}

static const MODULE_API_1 Module_GetApi_Impl =
{
    {MODULE_API_VERSION_1},

    FILTER_ParseConfigurationFromJson,
    FILTER_FreeConfiguration,
    FILTER_Create,
    FILTER_Destroy,
    FILTER_Receive,
    NULL
};

MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version)
{
    (void)gateway_api_version;
    return (const MODULE_API*)&Module_GetApi_Impl;
}

