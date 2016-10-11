// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/macro_utils.h"
#include "gateway.h"
#include "parson.h"

#define MODULES_KEY "modules"
#define MODULE_NAME_KEY "module name"
#define MODULE_PATH_KEY "module path"
#define ARG_KEY "args"

#define LINKS_KEY "links"
#define SOURCE_KEY "source"
#define SINK_KEY "sink"

#define PARSE_JSON_RESULT_VALUES \
    PARSE_JSON_SUCCESS, \
    PARSE_JSON_FAILURE, \
    PARSE_JSON_MISSING_OR_MISCONFIGURED_CONFIG, \
    PARSE_JSON_VECTOR_FAILURE, \
    PARSE_JSON_MISCONFIGURED_OR_OTHER

DEFINE_ENUM(PARSE_JSON_RESULT, PARSE_JSON_RESULT_VALUES);

static PARSE_JSON_RESULT parse_json_internal(GATEWAY_PROPERTIES* out_properties, JSON_Value *root);
static void destroy_properties_internal(GATEWAY_PROPERTIES* properties);

GATEWAY_HANDLE Gateway_Create_From_JSON(const char* file_path)
{
    GATEWAY_HANDLE gw;

    if (file_path != NULL)
    {
        JSON_Value *root_value;
        
        /*Codes_SRS_GATEWAY_14_002: [The function shall use parson to read the file and parse the JSON string to a parson JSON_Value structure.]*/
        root_value = json_parse_file(file_path);
        if (root_value != NULL)
        {
            /*Codes_SRS_GATEWAY_14_004: [The function shall traverse the JSON_Value object to initialize a GATEWAY_PROPERTIES instance.]*/
            GATEWAY_PROPERTIES *properties = (GATEWAY_PROPERTIES*)malloc(sizeof(GATEWAY_PROPERTIES));

            if (properties != NULL)
            {
				properties->gateway_modules = NULL;
				properties->gateway_links = NULL;
                if (parse_json_internal(properties, root_value) == PARSE_JSON_SUCCESS)
                {
                    /*Codes_SRS_GATEWAY_14_007: [The function shall use the GATEWAY_PROPERTIES instance to create and return a GATEWAY_HANDLE using the lower level API.]*/
                    gw = Gateway_LL_Create(properties);

                    if (gw == NULL)
                    {
                        LogError("Failed to create gateway using lower level library.");
                    }
					else
					{
						/*Codes_SRS_GATEWAY_17_001: [ Upon successful creation, this function shall start the gateway. ]*/
						GATEWAY_START_RESULT start_result;
						start_result = Gateway_LL_Start(gw);
						if (start_result != GATEWAY_START_SUCCESS)
						{
							/*Codes_SRS_GATEWAY_17_002: [ This function shall return NULL if starting the gateway fails. ]*/
							LogError("failed to start gateway");
							Gateway_LL_Destroy(gw);
							gw = NULL;
						}
					}
                }
                /*Codes_SRS_GATEWAY_14_006: [The function shall return NULL if the JSON_Value contains incomplete information.]*/
                else
                {
                    gw = NULL;
                    LogError("Failed to create properties structure from JSON configuration.");
                }
				destroy_properties_internal(properties);
                free(properties);
            }
            /*Codes_SRS_GATEWAY_14_008: [This function shall return NULL upon any memory allocation failure.]*/
            else
            {
                gw = NULL;
                LogError("Failed to allocate GATEWAY_PROPERTIES.");
            }

            json_value_free(root_value);
        }
        else
        {
            /*Codes_SRS_GATEWAY_14_003: [The function shall return NULL if the file contents could not be read and / or parsed to a JSON_Value.]*/
            gw = NULL;
            LogError("Input file [%s] could not be read.", file_path);
        }
    }
    /*Codes_SRS_GATEWAY_14_001: [If file_path is NULL the function shall return NULL.]*/
    else
    {
        gw = NULL;
        LogError("Input file path is NULL.");
    }

    return gw;
}

static void destroy_properties_internal(GATEWAY_PROPERTIES* properties)
{
	if (properties->gateway_modules != NULL)
	{
		size_t vector_size = VECTOR_size(properties->gateway_modules);
		for (size_t element_index = 0; element_index < vector_size; ++element_index)
		{
			GATEWAY_MODULES_ENTRY* element = (GATEWAY_MODULES_ENTRY*)VECTOR_element(properties->gateway_modules, element_index);
			json_free_serialized_string((char*)(element->module_configuration));
		}

		VECTOR_destroy(properties->gateway_modules);
		properties->gateway_modules = NULL;
	}
    
	if (properties->gateway_links != NULL)
	{
		VECTOR_destroy(properties->gateway_links);
		properties->gateway_links = NULL;
	}
}

static PARSE_JSON_RESULT parse_json_internal(GATEWAY_PROPERTIES* out_properties, JSON_Value *root)
{
    PARSE_JSON_RESULT result;

    JSON_Object *json_document = json_value_get_object(root);
    if (json_document != NULL)
    {
        JSON_Array *modules_array = json_object_get_array(json_document, MODULES_KEY);
		JSON_Array *links_array = json_object_get_array(json_document, LINKS_KEY);

        if (modules_array != NULL && links_array != NULL)
        {
            out_properties->gateway_modules = VECTOR_create(sizeof(GATEWAY_MODULES_ENTRY));
            if (out_properties->gateway_modules != NULL)
            {
                JSON_Object *module;
                size_t module_count = json_array_get_count(modules_array);
                for (size_t module_index = 0; module_index < module_count; ++module_index)
                {
                    module = json_array_get_object(modules_array, module_index);
                    const char* module_name = json_object_get_string(module, MODULE_NAME_KEY);
                    const char* module_path = json_object_get_string(module, MODULE_PATH_KEY);

                    if (module_name != NULL && module_path != NULL)
                    {
                        /*Codes_SRS_GATEWAY_14_005: [The function shall set the value of const void* module_properties in the GATEWAY_PROPERTIES instance to a char* representing the serialized args value for the particular module.]*/
                        JSON_Value *args = json_object_get_value(module, ARG_KEY);
                        char* args_str = json_serialize_to_string(args);

                        GATEWAY_MODULES_ENTRY entry = {
                            module_name,
                            module_path,
                            args_str
                        };

                        /*Codes_SRS_GATEWAY_14_006: [The function shall return NULL if the JSON_Value contains incomplete information.]*/
                        if (VECTOR_push_back(out_properties->gateway_modules, &entry, 1) == 0)
                        {
                            result = PARSE_JSON_SUCCESS;
                        }
                        else
                        {
                            json_free_serialized_string(args_str);
                            result = PARSE_JSON_VECTOR_FAILURE;
                            LogError("Failed to push data into properties vector.");
                            break;
                        }
                    }
                    /*Codes_SRS_GATEWAY_14_006: [The function shall return NULL if the JSON_Value contains incomplete information.]*/
                    else
                    {
                        result = PARSE_JSON_MISSING_OR_MISCONFIGURED_CONFIG;
                        LogError("\"module name\" or \"module path\" in input JSON configuration is missing or misconfigured.");
                        break;
                    }
                }

				if (result == PARSE_JSON_SUCCESS)
				{
					/* Codes_SRS_GATEWAY_04_001: [ The function shall create a Vector to Store all links to this gateway. ] */
					out_properties->gateway_links = VECTOR_create(sizeof(GATEWAY_LINK_ENTRY));
					if (out_properties->gateway_links != NULL)
					{
						JSON_Object *route;
						size_t links_count = json_array_get_count(links_array);
						for (size_t links_index = 0; links_index < links_count; ++links_index)
						{
							route = json_array_get_object(links_array, links_index);
							const char* module_source = json_object_get_string(route, SOURCE_KEY);
							const char* module_sink = json_object_get_string(route, SINK_KEY);

							if (module_source != NULL && module_sink != NULL)
							{
								GATEWAY_LINK_ENTRY entry = {
									module_source,
									module_sink
								};

								/* Codes_SRS_GATEWAY_04_002: [ The function shall add all modules source and sink to GATEWAY_PROPERTIES inside gateway_links. ] */
								if (VECTOR_push_back(out_properties->gateway_links, &entry, 1) == 0)
								{
									result = PARSE_JSON_SUCCESS;
								}
								else
								{
									result = PARSE_JSON_VECTOR_FAILURE;
									LogError("Failed to push data into links vector.");
									break;
								}
							}
							/*Codes_SRS_GATEWAY_14_006: [The function shall return NULL if the JSON_Value contains incomplete information.]*/
							else
							{
								result = PARSE_JSON_MISSING_OR_MISCONFIGURED_CONFIG;
								LogError("\"source\" or \"sink\" in input JSON configuration is missing or misconfigured.");
								break;
							}
						}
					}
					/* Codes_SRS_GATEWAY_14_008: [ This function shall return NULL upon any memory allocation failure. ] */
					else
					{
						result = PARSE_JSON_VECTOR_FAILURE;
						LogError("Failed to create links vector. ");
					}
				}
            }
			/* Codes_SRS_GATEWAY_14_008: [ This function shall return NULL upon any memory allocation failure. ] */
            else
            {
                result = PARSE_JSON_VECTOR_FAILURE;
                LogError("Failed to create properties vector. ");
            }
        }
        /*Codes_SRS_GATEWAY_14_006: [The function shall return NULL if the JSON_Value contains incomplete information.]*/
        else
        {
            result = PARSE_JSON_MISCONFIGURED_OR_OTHER;
            LogError("JSON Configuration file is configured incorrectly or some other error occurred while parsing.");
        }
    }
    /*Codes_SRS_GATEWAY_14_006: [The function shall return NULL if the JSON_Value contains incomplete information.]*/
    else
    {
        result = PARSE_JSON_MISCONFIGURED_OR_OTHER;
        LogError("JSON Configuration file is configured incorrectly or some other error occurred while parsing.");
    }
    return result;
}

