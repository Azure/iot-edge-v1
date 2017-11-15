// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>

#include "gateway.h"

int main(int argc, char** argv)
{
    GATEWAY_HANDLE gateway;
    if (argc != 3)
    {
        printf("usage: dynamically_add_module_sample modulesConfigFile linksConfigFile\n");
        printf("where modulesConfigFile is the name of the file that contains gateway modules and linksConfigFile is the name of the file that contains gateway links.\n");
    }
    else
    {
        if ((gateway = Gateway_Create(NULL)) == NULL)
        {
            printf("failed to create the gateway\n");
        }
        else
        {
            printf("gateway successfully created.\n");

            JSON_Value *root_value_modules;
            root_value_modules = json_parse_file(argv[1]);
            if (root_value_modules == NULL)
            {
                printf("Failed to parse modules file.\n");
            }
            else
            {
                char* json_content_modules = json_serialize_to_string(root_value_modules);

                if (json_content_modules == NULL)
                {
                    printf("Failed to serialize modules JSON to string.\n");
                }
                else
                {
                    int result = Gateway_UpdateFromJson(gateway, json_content_modules);
                    if (result != 0)
                    {
                        printf("failed to update gateway adding modules from JSON.\n");
                    }
                    else
                    {
                        JSON_Value *root_value_links;
                        root_value_links = json_parse_file(argv[2]);
                        if (root_value_links == NULL)
                        {
                            printf("Failed to parse links file.\n");
                        }
                        else
                        {
                            char* json_content_links = json_serialize_to_string(root_value_links);

                            if (json_content_links == NULL)
                            {
                                printf("Failed to serialize links JSON to string.\n");
                            }
                            else
                            {
                                result = Gateway_UpdateFromJson(gateway, json_content_links);
                                if (result != 0)
                                {
                                    printf("failed to update gateway adding links from JSON.\n");
                                }
                                else
                                {
                                    if (Gateway_Start(gateway) != GATEWAY_START_SUCCESS)
                                    {
                                        printf("failed to start gateway.\n");
                                    }
                                    else
                                    {
                                        printf("gateway successfully started.\n");
                                        printf("gateway shall run until ENTER is pressed\n");
                                        (void)getchar();
                                    }
                                }
                            }
                        }
                    }
                }
            }
            Gateway_Destroy(gateway);
        }
    }
    return 0;
}

