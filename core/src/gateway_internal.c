// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <azure_c_shared_utility/gballoc.h>
#include <azure_c_shared_utility/xlogging.h>

#include <stdbool.h>
#include <azure_c_shared_utility/vector.h>

#include "experimental/event_system.h"
#include "broker.h"
#include "module_access.h"

#include "gateway_internal.h"

#define GATEWAY_ALL "*"

static MODULE_DATA *no_module = NULL;

bool module_name_find(const void* element, const void* module_name)
{
    const char* module_name_casted = (const char*)module_name;
    return (strcmp((*(MODULE_DATA**)element)->module_name, module_name_casted) == 0);
}

static bool link_name_both_find(const void* link_void, const void* name_void)
{
    const char* name = (const char*)name_void;
    const LINK_DATA *link = (LINK_DATA*)link_void;
    return
        strcmp(link->module_sink->module_name, name) == 0 ||
        (!link->from_any_source && strcmp(link->module_source->module_name, name) == 0);
}

static bool check_if_link_exists(GATEWAY_HANDLE_DATA* gateway_handle, const GATEWAY_LINK_ENTRY* link_entry)
{
    bool exists = false;

    LINK_DATA* link_data = (LINK_DATA*)VECTOR_find_if(gateway_handle->links, link_data_find, link_entry);

    return link_data == NULL ? false : true;
}

static int add_one_link_to_broker(GATEWAY_HANDLE_DATA* gateway_handle, MODULE_HANDLE source, MODULE_HANDLE sink)
{
    int result;
    BROKER_LINK_DATA broker_link_entry =
    {
        source,
        sink
    };
    if (Broker_AddLink(gateway_handle->broker, &broker_link_entry) != BROKER_OK)
    {
        LogError("Could not add link to broker [%p] -> [%p]", source, sink);
        result = __LINE__;
    }
    else
    {
        result = 0;
    }
    return result;
}

static int remove_one_link_from_broker(GATEWAY_HANDLE_DATA* gateway_handle, MODULE_HANDLE source, MODULE_HANDLE sink)
{
    int result;
    BROKER_LINK_DATA broker_link_entry =
    {
        source,
        sink
    };
    if (Broker_RemoveLink(gateway_handle->broker, &broker_link_entry) != BROKER_OK)
    {
        LogError("Could not remove link from broker [%p] -> [%p]", source, sink);
        result = __LINE__;
    }
    else
    {
        result = 0;
    }
    return result;
}

static int add_regular_link(GATEWAY_HANDLE_DATA* gateway_handle, const GATEWAY_LINK_ENTRY* link_entry)
{
    int result;
    MODULE_DATA** module_source_handle = (MODULE_DATA**)VECTOR_find_if(gateway_handle->modules, module_name_find, link_entry->module_source);

    //Check of Source Module exists.
    /*Codes_SRS_GATEWAY_04_011: [If the module referenced by the entryLink->module_source or entryLink->module_sink doesn't exists this function shall return GATEWAY_ADD_LINK_ERROR ] */
    if (module_source_handle == NULL)
    {
        LogError("Failed to add the link. Source module doesn't exists on this gateway. Module Name: %s.", link_entry->module_source);
        result = __LINE__;
    }
    else
    {
        MODULE_DATA** module_sink_handle = (MODULE_DATA**)VECTOR_find_if(gateway_handle->modules, module_name_find, link_entry->module_sink);
        /*Codes_SRS_GATEWAY_04_011: [If the module referenced by the entryLink->module_source or entryLink->module_sink doesn't exists this function shall return GATEWAY_ADD_LINK_ERROR ] */
        if (module_sink_handle == NULL)
        {
            LogError("Failed to add the link. Sink module doesn't exists on this gateway. Module Name: %s.", link_entry->module_sink);
            result = __LINE__;
        }
        else
        {
            if (add_one_link_to_broker(gateway_handle, (*module_source_handle)->module, (*module_sink_handle)->module) != 0)
            {
                LogError("Unable to add link to Broker.");
                result = __LINE__;
            }
            else
            {
                LINK_DATA link_data =
                {
                    false,
                    *module_source_handle,
                    *module_sink_handle
                };

                /*Codes_SRS_GATEWAY_04_012: [ This function shall add the entryLink to the gw->links ] */
                if (VECTOR_push_back(gateway_handle->links, &link_data, 1) != 0)
                {
                    LogError("Unable to add LINK_DATA* to the gateway links vector.");
                    remove_one_link_from_broker(gateway_handle, (*module_source_handle)->module, (*module_sink_handle)->module);
                    result = __LINE__;
                }
                else
                {
                    result = 0;
                }
            }
        }
    }
    return result;
}

GATEWAY_HANDLE gateway_create_internal(const GATEWAY_PROPERTIES* properties, bool use_json)
{
    GATEWAY_HANDLE_DATA* gateway;
    /*Codes_SRS_GATEWAY_14_001: [This function shall create a GATEWAY_HANDLE representing the newly created gateway.]*/
    gateway = (GATEWAY_HANDLE_DATA*)malloc(sizeof(GATEWAY_HANDLE_DATA));

    if (gateway != NULL)
    {
        /* For freeing up NULL ptrs in case of create failure */
        memset(gateway, 0, sizeof(GATEWAY_HANDLE_DATA));

        /*Codes_SRS_GATEWAY_14_003: [This function shall create a new BROKER_HANDLE for the gateway representing this gateway's message broker. ]*/
        gateway->broker = Broker_Create();
        if (gateway->broker == NULL)
        {
            /*Codes_SRS_GATEWAY_14_004: [This function shall return NULL if a BROKER_HANDLE cannot be created.]*/
            gateway_destroy_internal(gateway);
            gateway = NULL;
            LogError("Gateway_Create(): Broker_Create() failed.");
        }
        else
        {
            /*Codes_SRS_GATEWAY_14_033: [ The function shall create a vector to store each MODULE_DATA. ]*/
            gateway->modules = VECTOR_create(sizeof(MODULE_DATA*));
            if (gateway->modules == NULL)
            {
                /*Codes_SRS_GATEWAY_14_034: [ This function shall return NULL if a VECTOR_HANDLE cannot be created. ]*/
                /*Codes_SRS_GATEWAY_14_035: [ This function shall destroy the previously created BROKER_HANDLE and free the GATEWAY_HANDLE if the VECTOR_HANDLE cannot be created. ]*/
                gateway_destroy_internal(gateway);
                gateway = NULL;
                LogError("Gateway_Create(): VECTOR_create failed.");
            }
            else
            {
                /* Codes_SRS_GATEWAY_04_001: [ The function shall create a vector to store each LINK_DATA ] */
                gateway->links = VECTOR_create(sizeof(LINK_DATA));
                if (gateway->links == NULL)
                {
                    gateway_destroy_internal(gateway);
                    gateway = NULL;
                    LogError("Gateway_Create(): VECTOR_create for links failed.");
                }
                else
                {
                    if (properties != NULL && properties->gateway_modules != NULL)
                    {
                        /*Codes_SRS_GATEWAY_14_009: [The function shall use each of GATEWAY_PROPERTIES's gateway_modules to create and add a module to the gateway's message broker. ]*/
                        size_t entries_count = VECTOR_size(properties->gateway_modules);
                        if (entries_count > 0)
                        {
                            //Add the first module, if successful add others
                            GATEWAY_MODULES_ENTRY* entry = (GATEWAY_MODULES_ENTRY*)VECTOR_element(properties->gateway_modules, 0);
                            MODULE_HANDLE module = gateway_addmodule_internal(gateway, entry, use_json);

                            //Continue adding modules until all are added or one fails
                            for (size_t properties_index = 1; properties_index < entries_count && module != NULL; ++properties_index)
                            {
                                entry = (GATEWAY_MODULES_ENTRY*)VECTOR_element(properties->gateway_modules, properties_index);
                                module = gateway_addmodule_internal(gateway, entry, use_json);
                            }

                            /*Codes_SRS_GATEWAY_14_036: [ If any MODULE_HANDLE is unable to be created from a GATEWAY_MODULES_ENTRY the GATEWAY_HANDLE will be destroyed. ]*/
                            if (module == NULL)
                            {
                                gateway_destroy_internal(gateway);
                                gateway = NULL;
                            }
                        }

                        if (gateway != NULL)
                        {
                            if (properties->gateway_links != NULL)
                            {
                                /* Codes_SRS_GATEWAY_04_002: [ The function shall use each GATEWAY_LINK_ENTRY of GATEWAY_PROPERTIES's gateway_links to add a LINK to GATEWAY_HANDLE's broker. ] */
                                size_t entries_count = VECTOR_size(properties->gateway_links);

                                if (entries_count > 0)
                                {
                                    //Add the first link, if successfull add others
                                    GATEWAY_LINK_ENTRY* entry = (GATEWAY_LINK_ENTRY*)VECTOR_element(properties->gateway_links, 0);
                                    bool linkAdded = gateway_addlink_internal(gateway, entry);

                                    //Continue adding links until all are added or one fails
                                    for (size_t links_index = 1; links_index < entries_count && linkAdded; ++links_index)
                                    {
                                        entry = (GATEWAY_LINK_ENTRY*)VECTOR_element(properties->gateway_links, links_index);
                                        linkAdded = gateway_addlink_internal(gateway, entry);
                                    }

                                    /*Codes_SRS_GATEWAY_04_003: [If any GATEWAY_LINK_ENTRY is unable to be added to the broker the GATEWAY_HANDLE will be destroyed.]*/
                                    if (!linkAdded)
                                    {
                                        LogError("Gateway_Create(): Unable to add link from '%s' to '%s'.The gateway will be destroyed.", entry->module_source, entry->module_sink);
                                        gateway_destroy_internal(gateway);
                                        gateway = NULL;
                                    }
                                }
                            }
                        }
                    }

                    if (gateway != NULL)
                    {
                        /* TODO: Seperate the gateway init from gateway start-up so that plugins have the chance
                        * register themselves */
                        /*Codes_SRS_GATEWAY_26_001: [ This function shall initialize attached Gateway Events callback system and report GATEWAY_STARTED event. ] */
                        gateway->event_system = EventSystem_Init();
                        /*Codes_SRS_GATEWAY_26_002: [ If Gateway Events module fails to be initialized the gateway module shall be destroyed with no events reported. ] */
                        if (gateway->event_system == NULL)
                        {
                            LogError("Gateway_Create(): Unable to initialize callback system");
                            gateway_destroy_internal(gateway);
                            gateway = NULL;
                        }
                        else
                        {
                            /*Codes_SRS_GATEWAY_26_001: [ This function shall initialize attached Gateway Events callback system and report GATEWAY_STARTED event. ] */
                            EventSystem_ReportEvent(gateway->event_system, gateway, GATEWAY_CREATED);
                            /*Codes_SRS_GATEWAY_26_010: [ This function shall report `GATEWAY_MODULE_LIST_CHANGED` event. ] */
                            EventSystem_ReportEvent(gateway->event_system, gateway, GATEWAY_MODULE_LIST_CHANGED);
                        }
                    }
                }
            }
        }
    }
    /*Codes_SRS_GATEWAY_14_002: [This function shall return NULL upon any  failure.]*/
    else
    {
        LogError("Gateway_Create(): malloc failed.");
    }
    return gateway;
}

void gateway_destroy_internal(GATEWAY_HANDLE gw)
{
    /*Codes_SRS_GATEWAY_14_005: [If gw is NULL the function shall do nothing.]*/
    if (gw != NULL)
    {
        GATEWAY_HANDLE_DATA* gateway_handle = (GATEWAY_HANDLE_DATA*)gw;

        if (gateway_handle->event_system != NULL)
        {
            /* event_system might be NULL here if destroying during failed creation, event system API should cleanly handle that */
            /* Codes_SRS_GATEWAY_26_003: [ If the Gateway Events module is initialized, this function shall report GATEWAY_DESTROYED event. ] */
            EventSystem_ReportEvent(gateway_handle->event_system, gateway_handle, GATEWAY_DESTROYED);
            /* Codes_SRS_GATEWAY_26_004: [ This function shall destroy the attached Gateway Events callback system. ] */
            EventSystem_Destroy(gateway_handle->event_system);
            gateway_handle->event_system = NULL;
        }

        if (gateway_handle->links != NULL)
        {
            /*Codes_SRS_GATEWAY_04_014: [ The function shall remove each link in GATEWAY_HANDLE_DATA's links vector and destroy GATEWAY_HANDLE_DATA's link. ]*/
            while (VECTOR_size(gateway_handle->links) > 0)
            {
                LINK_DATA* link_data = (LINK_DATA*)VECTOR_front(gateway_handle->links);
                gateway_removelink_internal(gateway_handle, link_data);
            }
            VECTOR_destroy(gateway_handle->links);
            gateway_handle->links = NULL;
        }

        if (gateway_handle->modules != NULL)
        {
            /*Codes_SRS_GATEWAY_14_028: [The function shall remove each module in GATEWAY_HANDLE_DATA's modules vector and destroy GATEWAY_HANDLE_DATA's modules.]*/
            while (VECTOR_size(gateway_handle->modules) > 0)
            {
                MODULE_DATA** module_data = (MODULE_DATA**)VECTOR_front(gateway_handle->modules);
                //By design, there will be no NULL module_data_pptr pointers in the vector
                /*Codes_SRS_GATEWAY_14_037: [If GATEWAY_HANDLE_DATA's message broker cannot remove a module, the function shall log the error and continue removing the modules from the GATEWAY_HANDLE. ]*/
                gateway_removemodule_internal(gateway_handle, module_data);
            }

            VECTOR_destroy(gateway_handle->modules);
        }

        if (gateway_handle->broker != NULL)
        {
            /*Codes_SRS_GATEWAY_14_006: [The function shall destroy the GATEWAY_HANDLE_DATA's `broker` `BROKER_HANDLE`. ]*/
            Broker_Destroy(gateway_handle->broker);
        }

        free(gateway_handle);
    }
    else
    {
        LogError("Gateway_Destroy(): The GATEWAY_HANDLE is null.");
    }
}

bool checkIfModuleExists(GATEWAY_HANDLE_DATA* gateway_handle, const char* module_name)
{
    bool exists = false;

    MODULE_DATA** module_data = (MODULE_DATA**)VECTOR_find_if(gateway_handle->modules, module_name_find, module_name);

    return module_data == NULL ? false : true;
}

MODULE_HANDLE gateway_addmodule_internal(GATEWAY_HANDLE_DATA* gateway_handle, const GATEWAY_MODULES_ENTRY* module_entry, bool use_json)
{
    MODULE_HANDLE module_result;

    /*Codes_SRS_GATEWAY_14_011: [ If gw, entry, or GATEWAY_MODULES_ENTRY's loader_configuration or loader_api is NULL the function shall return NULL. ]*/
    if (
        gateway_handle == NULL ||
        module_entry == NULL ||
        module_entry->module_name == NULL ||
        module_entry->module_loader_info.loader == NULL ||
        module_entry->module_loader_info.entrypoint == NULL ||
		module_entry->module_loader_info.loader->api == NULL
       )
    {
        module_result = NULL;
        LogError(
            "Failed to add module because a required input parameter is NULL. gw = %p, module_name = '%s', loader = %p, entrypoint = %p.",
            gateway_handle,
            module_entry != NULL ? module_entry->module_name : "NULL",
            module_entry != NULL ? module_entry->module_loader_info.loader : NULL,
            module_entry != NULL ? module_entry->module_loader_info.entrypoint : NULL
        );
    }
    else if (strcmp(module_entry->module_name, GATEWAY_ALL) == 0)
    {
        /*Codes_SRS_GATEWAY_17_001: [ This function shall not accept "*" as a module name. ]*/
        module_result = NULL;
        LogError("Failed to add module because the module_name is invalid [%s]", module_entry->module_name);
    }
    else
    {
        //First check if a module with a given name already exists.
        /*Codes_SRS_GATEWAY_04_004: [ If a module with the same module_name already exists, this function shall fail and the GATEWAY_HANDLE will be destroyed. ]*/
        bool moduleExist = checkIfModuleExists(gateway_handle, module_entry->module_name);
        if (!moduleExist)
        {
            MODULE_DATA * new_module_data = (MODULE_DATA*)malloc(sizeof(MODULE_DATA));
            if (new_module_data == NULL)
            {
                /*Codes_SRS_GATEWAY_14_031: [If unsuccessful, the function shall return NULL.]*/
                module_result = NULL;
                LogError("Failed to add module because it could not allocate memory.");
            }
            else
            {
                /*Codes_SRS_GATEWAY_14_012: [The function shall load the module located at GATEWAY_MODULES_ENTRY's module_path into a MODULE_LIBRARY_HANDLE. ]*/
                /*Codes_SRS_GATEWAY_17_015: [ The function shall use the module's specified loader and the module's entrypoint to get each module's MODULE_LIBRARY_HANDLE. ]*/
                MODULE_LIBRARY_HANDLE module_library_handle = module_entry->module_loader_info.loader->api->Load(
                    module_entry->module_loader_info.loader,
                    module_entry->module_loader_info.entrypoint
                );

                /*Codes_SRS_GATEWAY_14_031: [If unsuccessful, the function shall return NULL.]*/
                if (module_library_handle == NULL)
                {
                    free(new_module_data);
                    module_result = NULL;
                    LogError("Failed to add module because the module could not be loaded.");
                }
                else
                {
                    //Should always be a safe call.
                    /*Codes_SRS_GATEWAY_14_013: [The function shall get the const MODULE_API* from the MODULE_LIBRARY_HANDLE.]*/
                    const MODULE_API* module_apis = module_entry->module_loader_info.loader->api->GetApi(module_entry->module_loader_info.loader, module_library_handle);

                    // parse module args if needed
                    const void* module_configuration = module_entry->module_configuration;
                    const void* transformed_module_configuration;
                    if (use_json)
                    {
                        module_configuration = MODULE_PARSE_CONFIGURATION_FROM_JSON(module_apis)(
                            (const char *)(module_entry->module_configuration)
                        );
					}

                    // request the loader to transform the module configuration to what the module expects
                    /*Codes_SRS_GATEWAY_17_018: [ The function shall construct module configuration from module's entrypoint and module's module_configuration. ]*/
                    /*Codes_SRS_GATEWAY_17_021: [ The function shall construct module configuration from module's entrypoint and module's module_configuration. ]*/
                    /*Codes_SRS_GATEWAY_JSON_17_011: [ The function shall the loader's BuildModuleConfiguration to construct module input from module's "args" and "loader.entrypoint". ]*/
                    transformed_module_configuration = module_entry->module_loader_info.loader->api->BuildModuleConfiguration(
                        module_entry->module_loader_info.loader,
                        module_entry->module_loader_info.entrypoint,
                        module_configuration
                    );

                    /*Codes_SRS_GATEWAY_14_015: [The function shall use the MODULE_API to create a MODULE_HANDLE using the GATEWAY_MODULES_ENTRY's module_configuration. ]*/
                    MODULE_HANDLE module_handle = MODULE_CREATE(module_apis)(gateway_handle->broker, transformed_module_configuration);

                    // free the configurations
                    /*Codes_SRS_GATEWAY_17_020: [ The function shall clean up any constructed resources. ]*/
                    /*Codes_SRS_GATEWAY_17_022: [ The function shall clean up any constructed resources. ]*/
                    if (use_json)
                    {
                        MODULE_FREE_CONFIGURATION(module_apis)((void*)module_configuration);
                    }
                    module_entry->module_loader_info.loader->api->FreeModuleConfiguration(module_entry->module_loader_info.loader, transformed_module_configuration);

                    /*Codes_SRS_GATEWAY_14_016: [If the module creation is unsuccessful, the function shall return NULL.]*/
                    if (module_handle == NULL)
                    {
                        free(new_module_data);
                        module_result = NULL;
                        module_entry->module_loader_info.loader->api->Unload(module_entry->module_loader_info.loader, module_library_handle);
                        LogError("Module_Create failed.");
                    }
                    else
                    {
                        /*Codes_SRS_GATEWAY_99_011: [The function shall assign `module_apis` to `MODULE::module_apis`. ]*/
                        MODULE module;
                        module.module_apis = module_apis;
                        module.module_handle = module_handle;

                        /*Codes_SRS_GATEWAY_14_017: [The function shall attach the module to the GATEWAY_HANDLE_DATA's broker using a call to Broker_AddModule. ]*/
                        /*Codes_SRS_GATEWAY_14_018: [If the function cannot attach the module to the message broker, the function shall return NULL.]*/
                        if (Broker_AddModule(gateway_handle->broker, &module) != BROKER_OK)
                        {
                            free(new_module_data);
                            module_result = NULL;
                            LogError("Failed to add module to the gateway's broker.");
                        }
                        else
                        {
                            char* name_copied = NULL;
                            /*Codes_SRS_GATEWAY_26_020: [ The function shall make a copy of the name of the module for internal use. ]*/
                            mallocAndStrcpy_s(&name_copied, module_entry->module_name);
                            if (name_copied == NULL)
                            {
                                free(new_module_data);
                                module_result = NULL;
                                if (Broker_RemoveModule(gateway_handle->broker, &module) != BROKER_OK)
                                {
                                    LogError("Failed to remove module [%p] from the gateway message broker. This module will remain attached.", &module);
                                }
                                LogError("Unable to malloc for module name");
                            }
                            else
                            {
                                strcpy(name_copied, module_entry->module_name);
                                /*Codes_SRS_GATEWAY_14_039: [ The function shall increment the BROKER_HANDLE reference count if the MODULE_HANDLE was successfully added to the GATEWAY_HANDLE_DATA's broker. ]*/
                                Broker_IncRef(gateway_handle->broker);
                                /*Codes_SRS_GATEWAY_14_029: [ The function shall create a new MODULE_DATA containing the MODULE_HANDLE, MODULE_LOADER_API and MODULE_LIBRARY_HANDLE if the module was successfully linked to the message broker. ]*/
                                MODULE_DATA module_data =
                                {
                                    name_copied,
                                    module_library_handle,
                                    module_entry->module_loader_info.loader,
                                    module_handle
                                };
                                *new_module_data = module_data;
                                /*Codes_SRS_GATEWAY_14_032: [The function shall add the new MODULE_DATA to GATEWAY_HANDLE_DATA's modules if the module was successfully attached to the message broker. ]*/
                                if (VECTOR_push_back(gateway_handle->modules, &new_module_data, 1) != 0)
                                {
                                    /*Codes_SRS_GATEWAY_14_019: [The function shall return the newly created MODULE_HANDLE only if each API call returns successfully.]*/
                                    Broker_DecRef(gateway_handle->broker);
                                    free(new_module_data);
                                    free(name_copied);
                                    module_result = NULL;
                                    if (Broker_RemoveModule(gateway_handle->broker, &module) != BROKER_OK)
                                    {
                                        LogError("Failed to remove module [%p] from the gateway message broker. This module will remain attached.", &module);
                                    }
                                    LogError("Unable to add MODULE_DATA* to the gateway module vector.");
                                }
                                else
                                {
                                    if (add_module_to_any_source(gateway_handle, *(MODULE_DATA**)VECTOR_back(gateway_handle->modules)) != 0)
                                    {
                                        /*Codes_SRS_GATEWAY_14_019: [The function shall return the newly created MODULE_HANDLE only if each API call returns successfully.]*/
                                        Broker_DecRef(gateway_handle->broker);
                                        module_result = NULL;
                                        if (Broker_RemoveModule(gateway_handle->broker, &module) != BROKER_OK)
                                        {
                                            LogError("Failed to remove module [%p] from the gateway message broker. This module will remain attached.", &module);
                                        }
                                        VECTOR_erase(gateway_handle->modules, VECTOR_back(gateway_handle->modules), 1);
                                        free(new_module_data);
                                        free(name_copied);
                                        LogError("Unable to add MODULE_DATA* to existing broker links.");
                                    }
                                    else
                                    {
                                        /*Codes_SRS_GATEWAY_14_019: [The function shall return the newly created MODULE_HANDLE only if each API call returns successfully.]*/
                                        module_result = module_handle;
                                    }
                                }
                            }
                        }

                        /*Codes_SRS_GATEWAY_14_030: [If any internal API call is unsuccessful after a module is created, the library will be unloaded and the module destroyed.]*/
                        if (module_result == NULL)
                        {
                            MODULE_DESTROY(module_apis)(module_handle);
                            module_entry->module_loader_info.loader->api->Unload(module_entry->module_loader_info.loader, module_library_handle);
                        }
                    }
                }
            }
        }
        else
        {
            module_result = NULL;
            LogError("Error to add module. Duplicated module name: %s", module_entry->module_name);
        }
    }

    return module_result;
}

void gateway_removemodule_internal(GATEWAY_HANDLE_DATA* gateway_handle, MODULE_DATA** module_data_pptr)
{
    MODULE module;
    module.module_apis = NULL;
    module.module_handle = (*module_data_pptr)->module;

    remove_module_from_any_source(gateway_handle, *module_data_pptr);
    LINK_DATA *link;
    /* Codes_SRS_GATEWAY_26_018: [ This function shall remove any links that contain the removed module either as a source or sink. ] */
    while ((link = VECTOR_find_if(gateway_handle->links, link_name_both_find, (*module_data_pptr)->module_name)) != NULL)
        gateway_removelink_internal(gateway_handle, link);

    free((*module_data_pptr)->module_name);

    /*Codes_SRS_GATEWAY_14_021: [ The function shall detach module from the GATEWAY_HANDLE_DATA's broker BROKER_HANDLE. ]*/
    /*Codes_SRS_GATEWAY_14_022: [ If GATEWAY_HANDLE_DATA's broker cannot detach module, the function shall log the error and continue unloading the module from the GATEWAY_HANDLE. ]*/
    if (Broker_RemoveModule(gateway_handle->broker, &module) != BROKER_OK)
    {
        LogError("Failed to remove module [%p] from the message broker. This module will remain linked to the broker but will be removed from the gateway.", (*module_data_pptr)->module);
    }
    /*Codes_SRS_GATEWAY_14_038: [ The function shall decrement the BROKER_HANDLE reference count. ]*/
    Broker_DecRef(gateway_handle->broker);

    /*Codes_SRS_GATEWAY_14_024: [ The function shall use the MODULE_DATA's module_library_handle to retrieve the MODULE_API and destroy module. ]*/
    MODULE_DESTROY((*module_data_pptr)->module_loader->api->GetApi((*module_data_pptr)->module_loader, (*module_data_pptr)->module_library_handle))((*module_data_pptr)->module);

    /*Codes_SRS_GATEWAY_14_025: [The function shall unload MODULE_DATA's module_library_handle. ]*/
    (*module_data_pptr)->module_loader->api->Unload((*module_data_pptr)->module_loader, (*module_data_pptr)->module_library_handle);

    /*Codes_SRS_GATEWAY_14_026:[The function shall remove that MODULE_DATA from GATEWAY_HANDLE_DATA's modules. ]*/
    MODULE_DATA * module_data_ptr = *module_data_pptr;
    VECTOR_erase(gateway_handle->modules, module_data_pptr, 1);
    free(module_data_ptr);
}

bool gateway_addlink_internal(GATEWAY_HANDLE_DATA* gateway_handle, const GATEWAY_LINK_ENTRY* link_entry)
{
    bool result;

    //First check if a link with a given source/sink pair already exists.
    /*Codes_SRS_GATEWAY_04_009: [ This function shall check if a given link already exists. ]*/
    bool linkExist = check_if_link_exists(gateway_handle, link_entry);

    if (!linkExist)
    {
        if (strcmp(GATEWAY_ALL, link_entry->module_source) == 0)
        {
            /*Codes_SRS_GATEWAY_17_002: [ The gateway shall accept a link with a source of "*" and a sink of a valid module. ]*/
            if (add_any_source_link(gateway_handle, link_entry) != 0)
            {
                LogError("Failed to add a any_source link sink = %s", link_entry->module_sink);
                result = false;
            }
            else
            {
                result = true;
            }
        }
        else
        {
            if (add_regular_link(gateway_handle, link_entry) != 0)
            {
                LogError("Failed to add a any_source link sink = %s", link_entry->module_sink);
                result = false;
            }
            else
            {
                result = true;
            }
        }
    }
    else
    {
        result = false;
        LogError("Error to add link. Duplicated link found. Source_name: %s, Sink_name: %s", link_entry->module_source, link_entry->module_sink);
    }

    return result;
}

void gateway_removelink_internal(GATEWAY_HANDLE_DATA* gateway_handle, LINK_DATA* link_data)
{
    /*Codes_SRS_GATEWAY_04_007: [The functional shall remove that LINK_DATA from GATEWAY_HANDLE_DATA's links. ]*/

    if (link_data->from_any_source)
    {
        remove_any_source_link(gateway_handle, link_data);
    }
    else
    {

        BROKER_LINK_DATA broker_data =
        {
            link_data->module_source->module,
            link_data->module_sink->module
        };

        Broker_RemoveLink(gateway_handle->broker, &broker_data);
    }

    VECTOR_erase(gateway_handle->links, link_data, 1);
}

int add_module_to_any_source(GATEWAY_HANDLE_DATA* gateway_handle, MODULE_DATA* module)
{
    int result = 0;
    size_t link;
    size_t num_links = VECTOR_size(gateway_handle->links);
    for (link = 0; link < num_links; link++)
    {
        LINK_DATA * link_data = VECTOR_element(gateway_handle->links, link);
        if (link_data->from_any_source)
        {
            MODULE_DATA** module_sink = (MODULE_DATA**)VECTOR_find_if(gateway_handle->modules, module_name_find, link_data->module_sink->module_name);
            if (module_sink == NULL)
            {
                LogError("Link failure between [%s] and [%s]", link_data->module_sink->module_name, module->module_name);
                result = __LINE__;
                break;
            }
            else
            {
                if (add_one_link_to_broker(gateway_handle, module->module, (*module_sink)->module) != 0)
                {
                    result = __LINE__;
                    break;
                }
            }
        }
    }
    if (result != 0)
    {
        remove_module_from_any_source(gateway_handle, module);
    }

    return result;
}

void remove_module_from_any_source(GATEWAY_HANDLE_DATA* gateway_handle, MODULE_DATA* module)
{
    size_t link;
    size_t num_links = VECTOR_size(gateway_handle->links);
    for (link = 0; link < num_links; link++)
    {
        LINK_DATA * link_data = VECTOR_element(gateway_handle->links, link);
        if (link_data->from_any_source)
        {
            MODULE_DATA** module_sink = (MODULE_DATA**)VECTOR_find_if(gateway_handle->modules, module_name_find, link_data->module_sink->module_name);
            if (module_sink == NULL)
            {
                LogError("Could not find sink for link [%s]", link_data->module_sink);
            }
            else
            {
                if (remove_one_link_from_broker(gateway_handle, module->module, (*module_sink)->module) != 0)
                {
                    LogError("Unable to remove link to Broker.");
                }
            }
        }
    }
}

int add_any_source_link(GATEWAY_HANDLE_DATA* gateway_handle, const GATEWAY_LINK_ENTRY* link_entry)
{
    int result;
    MODULE_DATA** module_sink_data = (MODULE_DATA**)VECTOR_find_if(gateway_handle->modules, module_name_find, link_entry->module_sink);

    /*Codes_SRS_GATEWAY_04_011: [If the module referenced by the entryLink->module_source or entryLink->module_sink doesn't exists this function shall return GATEWAY_ADD_LINK_ERROR ] */
    if (module_sink_data == NULL)
    {
        LogError("Failed to add the link. Sink module doesn't exists on this gateway. Module Name: %s.", link_entry->module_sink);
        result = __LINE__;
    }
    else
    {
        /*Codes_SRS_GATEWAY_17_004: [ The gateway shall accept a link containing "*" as entryLink->module_source, and a valid module name as a entryLink->module_sink. ]*/
        LINK_DATA link_data =
        {
            true,
            no_module,
            *module_sink_data
        };

        /*Codes_SRS_GATEWAY_04_012: [ This function shall add the entryLink to the gw->links ] */
        if (VECTOR_push_back(gateway_handle->links, &link_data, 1) != 0)
        {
            LogError("Unable to add LINK_DATA* to the gateway links vector.");
            result = __LINE__;
        }
        else
        {
            /*Codes_SRS_GATEWAY_17_003: [ The gateway shall treat a source of "*" as link to the sink module from every other module in gateway. ]*/
            size_t m;
            size_t num_modules = VECTOR_size(gateway_handle->modules);
            result = 0;
            for (m = 0; m < num_modules; m++)
            {
                MODULE_DATA **source_module_data = (MODULE_DATA **)VECTOR_element(gateway_handle->modules, m);
                /*Codes_SRS_GATEWAY_17_005: [ For this link, the sink shall receive all messages publish by other modules. ]*/
                if ((*source_module_data)->module != (*module_sink_data)->module &&
                    add_one_link_to_broker(gateway_handle, (*source_module_data)->module, (*module_sink_data)->module) != 0)
                {
                    result = __LINE__;
                    break;
                }
            }
            if (result != 0)
            {
                remove_any_source_link(gateway_handle, &link_data);
                VECTOR_erase(gateway_handle->links, VECTOR_back(gateway_handle->links), 1);
            }
        }
    }
    return result;
}

void remove_any_source_link(GATEWAY_HANDLE_DATA* gateway_handle, LINK_DATA* link_entry)
{
    MODULE_DATA** module_sink_data = (MODULE_DATA**)VECTOR_find_if(gateway_handle->modules, module_name_find, link_entry->module_sink->module_name);

    /*Codes_SRS_GATEWAY_04_011: [If the module referenced by the entryLink->module_source or entryLink->module_sink doesn't exists this function shall return GATEWAY_ADD_LINK_ERROR ] */
    if (module_sink_data != NULL)
    {
        size_t m;
        size_t num_modules = VECTOR_size(gateway_handle->modules);
        for (m = 0; m < num_modules; m++)
        {
            MODULE_DATA **source_module_data = (MODULE_DATA **)VECTOR_element(gateway_handle->modules, m);
            if ((*source_module_data)->module != (*module_sink_data)->module &&
                remove_one_link_from_broker(gateway_handle, (*source_module_data)->module, (*module_sink_data)->module) != 0)
            {
                LogError("Unable to remove link to Broker.");
            }
        }
    }
    else
    {
        LogError("Sink module doesn't exists on this gateway. Module Name: %s.", link_entry->module_sink);
    }

}

/* Searches both sources and sinks. */
bool link_data_find(const void* element, const void* linkEntry)
{
    bool result;
    GATEWAY_LINK_ENTRY* link_entry_casted = (GATEWAY_LINK_ENTRY*)linkEntry;
    LINK_DATA * element_casted = (LINK_DATA*)element;

    if (strcmp(GATEWAY_ALL, link_entry_casted->module_source) == 0)
    {
        if (element_casted->from_any_source)
        {
            result = (strcmp(element_casted->module_sink->module_name, link_entry_casted->module_sink) == 0);
        }
        else
        {
            result = false;
        }
    }
    else
    {
        if (element_casted->from_any_source)
        {
            result = false;
        }
        else
        {
            result = ((strcmp(element_casted->module_sink->module_name, link_entry_casted->module_sink) == 0) &&
                (strcmp(element_casted->module_source->module_name, link_entry_casted->module_source) == 0));
        }
    }

    return result;
}
