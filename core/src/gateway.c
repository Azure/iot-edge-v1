// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"

#include <stddef.h>
#include <assert.h>

#include "gateway.h"
#include "broker.h"
#include "module_loader.h"
#include "experimental/event_system.h"
#include "module_access.h"
#include "gateway_internal.h"

static bool module_info_name_find(const void* element, const void* module_name);
static void gateway_destroymodulelist_internal(GATEWAY_MODULE_INFO* infos, size_t count);
static bool module_data_find(const void* element, const void* value);

VECTOR_HANDLE Gateway_GetModuleList(GATEWAY_HANDLE gw)
{
    VECTOR_HANDLE result;

    /*Codes_SRS_GATEWAY_26_008: [ If the `gw` parameter is NULL, the function shall return NULL handle and not allocate any data. ] */
    if (gw == NULL)
    {
        LogError("NULL gateway handle given to GetModuleLIst");
        result = NULL;
    }
    else
    {
        result = VECTOR_create(sizeof(GATEWAY_MODULE_INFO));
        size_t module_count = VECTOR_size(gw->modules);

        /*Codes_SRS_GATEWAY_26_009: [ This function shall return a NULL handle should any internal callbacks fail. ]*/
        if (result == NULL)
        {
            LogError("Failed to init vector during GetModuleList");
        }
        else if (module_count > 0)
        {
            // Workaround for no VECTOR_resize
            void *resize = calloc(module_count, sizeof(GATEWAY_MODULE_INFO));
            if (resize == NULL)
            {
                /*Codes_SRS_GATEWAY_26_009: [ This function shall return a NULL handle should any internal callbacks fail ]*/
                LogError("Calloc fail");
                VECTOR_destroy(result);
                result = NULL;
            }
            else
            {
                if (VECTOR_push_back(result, resize, module_count) != 0)
                {
                    /*Codes_SRS_GATEWAY_26_009: [ This function shall return a NULL handle should any internal callbacks fail ]*/
                    LogError("Failed to resize vector");
                    VECTOR_destroy(result);
                    result = NULL;
                }
                else
                {
                    /*
                    * Two pass way of copying graph edges and nodes.
                    * First we create the nodes and map old structs to new struct ptr.
                    * Then in the second pass we create the edges using the map
                    */
                    int has_failed = 0;
                    /*Codes_SRS_GATEWAY_26_007: [ This function shall return a snapshot copy of information about current gateway modules. ]*/
                    for (size_t i = 0; i < module_count; i++)
                    {
                        MODULE_DATA *module_data = *(MODULE_DATA**)VECTOR_element(gw->modules, i);
                        GATEWAY_MODULE_INFO *info = (GATEWAY_MODULE_INFO*)VECTOR_element(result, i);
                        info->module_name = module_data->module_name;
                        info->module_sources = VECTOR_create(sizeof(GATEWAY_MODULE_INFO*));
                        if (info->module_sources == NULL)
                        {
                            /*Codes_SRS_GATEWAY_26_009: [ This function shall return a NULL handle should any internal callbacks fail. ]*/
                            has_failed = 1;
                            LogError("Failed to create links vector");
                            gateway_destroymodulelist_internal(VECTOR_front(result), module_count);
                            VECTOR_destroy(result);
                            result = NULL;
                            break;
                        }
                    }

                    if (!has_failed)
                    {
                        size_t links_count = VECTOR_size(gw->links);

                        /*Codes_SRS_GATEWAY_26_013: [ For each module returned this function shall provide a snapshot copy vector of link sources for that module. ]*/
                        for (size_t i = 0; i < links_count; i++)
                        {
                            LINK_DATA *link_data = (LINK_DATA*)VECTOR_element(gw->links, i);

                            GATEWAY_MODULE_INFO *sink = VECTOR_find_if(result, module_info_name_find, link_data->module_sink->module_name);
                            assert(sink != NULL);

                            if (!link_data->from_any_source)
                            {
                                GATEWAY_MODULE_INFO *src = VECTOR_find_if(result, module_info_name_find, link_data->module_source->module_name);
                                assert(src != NULL);

                                if (VECTOR_push_back(sink->module_sources, &src, 1) != 0)
                                {
                                    LogError("Failed to push_back link to module info");
                                    gateway_destroymodulelist_internal(VECTOR_front(result), module_count);
                                    VECTOR_destroy(result);
                                    result = NULL;
                                    break;
                                }
                            }
                            else
                            {
                                /*Codes_SRS_GATEWAY_26_014: [ For each module returned that has '*' as a link source this function shall provide NULL vector pointer as it's sources vector. ]*/
                                VECTOR_destroy(sink->module_sources);
                                sink->module_sources = NULL;
                            }
                        }
                    }
                }
                free(resize);
            }
        }
    }
    return result;
}

void Gateway_DestroyModuleList(VECTOR_HANDLE module_list)
{
    gateway_destroymodulelist_internal(VECTOR_front(module_list), VECTOR_size(module_list));
    /*Codes_SRS_GATEWAY_26_012: [ This function shall destroy the list of `GATEWAY_MODULE_INFO` ]*/
    VECTOR_destroy(module_list);
}

void Gateway_AddEventCallback(GATEWAY_HANDLE gw, GATEWAY_EVENT event_type, GATEWAY_CALLBACK callback, void* user_param)
{
    /* Codes_SRS_GATEWAY_26_006: [ This function shall log a failure and do nothing else when `gw` parameter is NULL. ] */
    if (gw == NULL)
    {
        LogError("invalid gateway when registering callback");
    }
    else
    {
        EventSystem_AddEventCallback(gw->event_system, event_type, callback, user_param);
    }
}

GATEWAY_HANDLE Gateway_Create(const GATEWAY_PROPERTIES* properties)
{
    GATEWAY_HANDLE result;
    /*Codes_SRS_GATEWAY_17_016: [ This function shall initialize the default module loaders. ] */
    if (ModuleLoader_Initialize() != MODULE_LOADER_SUCCESS)
    {
        LogError("Gateway_Create() - ModuleLoader_Initialize failed");
        result = NULL;
    }
    else
    {
        result = gateway_create_internal(properties, false);
        if (result == NULL)
        {
            /* Codes_SRS_GATEWAY_17_017: [ This function shall destroy the default module loaders upon any failure. ]*/
            ModuleLoader_Destroy();
        }
    }

    return result;
}

GATEWAY_START_RESULT Gateway_Start(GATEWAY_HANDLE gw)
{
    GATEWAY_START_RESULT result;
    if (gw != NULL)
    {
        GATEWAY_HANDLE_DATA* gateway_handle = (GATEWAY_HANDLE_DATA*)gw;

        /*Codes_SRS_GATEWAY_17_010: [ This function shall call Module_Start for every module which defines the start function. ]*/
        size_t module_count = VECTOR_size(gateway_handle->modules);
        size_t m;
        for (m = 0; m < module_count; m++)
        {
            MODULE_DATA** module_data = VECTOR_element(gateway_handle->modules, m);
            pfModule_Start pfStart = MODULE_START((*module_data)->module_loader->api->GetApi((*module_data)->module_loader, (*module_data)->module_library_handle));
            if (pfStart != NULL)
            {
                /*Codes_SRS_GATEWAY_17_010: [ This function shall call Module_Start for every module which defines the start function. ]*/
                (pfStart)((*module_data)->module);
            }
        }
        /*Codes_SRS_GATEWAY_17_012: [ This function shall report a GATEWAY_STARTED event. ]*/
        EventSystem_ReportEvent(gw->event_system, gw, GATEWAY_STARTED);
        /*Codes_SRS_GATEWAY_17_013: [ This function shall return GATEWAY_START_SUCCESS upon completion. ]*/
        result = GATEWAY_START_SUCCESS;
    }
    else
    {
        /*Codes_SRS_GATEWAY_17_009: [ This function shall return GATEWAY_START_INVALID_ARGS if a NULL gateway is received. ]*/
        result = GATEWAY_START_INVALID_ARGS;
    }
    return result;
}

void Gateway_Destroy(GATEWAY_HANDLE gw)
{
    gateway_destroy_internal(gw);
    /*Codes_SRS_GATEWAY_17_019: [ The function shall destroy the module loader list. ]*/
    ModuleLoader_Destroy();
}

MODULE_HANDLE Gateway_AddModule(GATEWAY_HANDLE gw, const GATEWAY_MODULES_ENTRY* entry)
{
    MODULE_HANDLE module;
    /*Codes_SRS_GATEWAY_14_011: [ If gw, entry, or GATEWAY_MODULES_ENTRY's loader_configuration or loader_api is NULL the function shall return NULL. ]*/
    if (gw != NULL && entry != NULL)
    {
        module = gateway_addmodule_internal(gw, entry, false);

        if (module == NULL)
        {
            LogError("Gateway_AddModule(): Unable to add module '%s'.", entry->module_name);
        }
        else
        {
            /*Codes_SRS_GATEWAY_26_011: [ The function shall report `GATEWAY_MODULE_LIST_CHANGED` event after successfully adding the module. ]*/
            EventSystem_ReportEvent(gw->event_system, gw, GATEWAY_MODULE_LIST_CHANGED);
        }
    }
    else
    {
        module = NULL;
        LogError("Gateway_AddModule(): Unable to add module to NULL GATEWAY_HANDLE or from NULL GATEWAY_MODULES_ENTRY*. gw = %p, entry = %p.", gw, entry);
    }

    return module;
}

extern void Gateway_StartModule(GATEWAY_HANDLE gw, MODULE_HANDLE module)
{
    if (gw != NULL)
    {
        GATEWAY_HANDLE_DATA* gateway_handle = (GATEWAY_HANDLE_DATA*)gw;
        MODULE_DATA** module_data = (MODULE_DATA**)VECTOR_find_if(gateway_handle->modules, module_data_find, module);
        if (module_data != NULL)
        {
            pfModule_Start pfStart = MODULE_START((*module_data)->module_loader->api->GetApi((*module_data)->module_loader, (*module_data)->module_library_handle));
            if (pfStart != NULL)
            {
                /*Codes_SRS_GATEWAY_17_008: [ When module is found, if the Module_Start function is defined for this module, the Module_Start function shall be called. ]*/
                (pfStart)((*module_data)->module);
            }
        }
        else
        {
            /*Codes_SRS_GATEWAY_17_007: [ If module is not found in the gateway, this function shall do nothing. ]*/
            LogError("Gateway_StartModule(): Failed to start module because the MODULE_DATA not found.");
        }
    }
    else
    {
        /*Codes_SRS_GATEWAY_17_006: [ If gw is NULL, this function shall do nothing. ]*/
        LogError("Gateway_StartModule(): Failed to start module because the GATEWAY_HANDLE is NULL.");
    }
}


void Gateway_RemoveModule(GATEWAY_HANDLE gw, MODULE_HANDLE module)
{
    /*Codes_SRS_GATEWAY_14_020: [ If gw or module is NULL the function shall return. ]*/
    if (gw != NULL)
    {
        GATEWAY_HANDLE_DATA* gateway_handle = (GATEWAY_HANDLE_DATA*)gw;

        /*Codes_SRS_GATEWAY_14_023: [The function shall locate the MODULE_DATA object in GATEWAY_HANDLE_DATA's modules containing module and return if it cannot be found. ]*/
        MODULE_DATA** module_data = (MODULE_DATA**)VECTOR_find_if(gateway_handle->modules, module_data_find, module);

        if (module_data != NULL)
        {
            gateway_removemodule_internal(gateway_handle, module_data);
            /*Codes_SRS_GATEWAY_26_012: [ The function shall report `GATEWAY_MODULE_LIST_CHANGED` event after successfully removing the module. ]*/
            EventSystem_ReportEvent(gw->event_system, gw, GATEWAY_MODULE_LIST_CHANGED);
        }
        else
        {
            LogError("Gateway_RemoveModule(): Failed to remove module because the MODULE_DATA pointer is NULL.");
        }
    }
    else
    {
        LogError("Gateway_RemoveModule(): Failed to remove module because the GATEWA_HANDLE is NULL.");
    }
}

int Gateway_RemoveModuleByName(GATEWAY_HANDLE gw, const char *module_name)
{
    int result;
    if (gw != NULL && module_name != NULL)
    {
        MODULE_DATA **module_data = (MODULE_DATA**)VECTOR_find_if(gw->modules, module_name_find, module_name);
        if (module_data != NULL)
        {
            /* Codes_SRS_GATEWAY_26_016: [** The function shall return 0 if the module was found. ] */
            result = 0;
            gateway_removemodule_internal(gw, module_data);
            EventSystem_ReportEvent(gw->event_system, gw, GATEWAY_MODULE_LIST_CHANGED);
        }
        else
        {
            /* Codes_SRS_GATEWAY_26_017: [** If module with `module_name` name is not found this function shall return non - zero and do nothing. ] */
            LogError("Couldn't find module with the specified name");
            result = __LINE__;
        }
    }
    else
    {
        /* Codes_SRS_GATEWAY_26_015: [ If `gw` or `module_name` is `NULL` the function shall do nothing and return with non - zero result. ] */
        LogError("NULL gateway or module_name given to Gateway_RemoveModuleByName()");
        result = __LINE__;
    }
    return result;
}

GATEWAY_ADD_LINK_RESULT Gateway_AddLink(GATEWAY_HANDLE gw, const GATEWAY_LINK_ENTRY* entryLink)
{
    GATEWAY_ADD_LINK_RESULT result;

    if (gw == NULL || entryLink == NULL || entryLink->module_source == NULL || entryLink->module_sink == NULL)
    {
        /*Codes_SRS_GATEWAY_04_008: [ If gw , entryLink, entryLink->module_source or entryLink->module_source is NULL the function shall return GATEWAY_ADD_LINK_INVALID_ARG. ]*/
        result = GATEWAY_ADD_LINK_INVALID_ARG;
        LogError("Failed to add link because either the GATEWAY_HANDLE is NULL, entryLink, module_source string is NULL or empty or module_sink is NULL or empty. gw = %p, module_source = '%s', module_sink = '%s'.", gw, entryLink, (entryLink != NULL)? entryLink->module_source:NULL, (entryLink != NULL) ? entryLink->module_sink : NULL);
    }
    else
    {
        if (!gateway_addlink_internal(gw, entryLink))
        {
            /*Codes_SRS_GATEWAY_04_010: [ If the entryLink already exists it the function shall return GATEWAY_ADD_LINK_ERROR ] */
            /*Codes_SRS_GATEWAY_04_011: [ If the module referenced by the entryLink->module_source or entryLink->module_sink doesn't exists this function shall return GATEWAY_ADD_LINK_ERROR ] */
            result = GATEWAY_ADD_LINK_ERROR;
        }
        else
        {
            /*Codes_SRS_GATEWAY_04_013: [ If adding the link succeed this function shall return GATEWAY_ADD_LINK_SUCCESS ]*/
            result = GATEWAY_ADD_LINK_SUCCESS;
            /*Codes_SRS_GATEWAY_26_019: [ The function shall report `GATEWAY_MODULE_LIST_CHANGED` event after successfully adding the link. ]*/
            EventSystem_ReportEvent(gw->event_system, gw, GATEWAY_MODULE_LIST_CHANGED);
        }
    }

    return result;
}

void Gateway_RemoveLink(GATEWAY_HANDLE gw, const GATEWAY_LINK_ENTRY* entryLink)
{
    /*Codes_SRS_GATEWAY_04_005: [ If gw or entryLink is NULL the function shall return. ]*/
    if (gw == NULL || entryLink == NULL)
    {
        LogError("Gateway_RemoveLink(): Failed to remove link because the GATEWAY_HANDLE is NULL or entryLink is NULL.");
    }
    else
    {
        GATEWAY_HANDLE_DATA* gateway_handle = (GATEWAY_HANDLE_DATA*)gw;

        /*Codes_SRS_GATEWAY_04_006: [ The function shall locate the LINK_DATA object in GATEWAY_HANDLE_DATA's links containing link and return if it cannot be found. ]*/
        LINK_DATA* link_data = (LINK_DATA*)VECTOR_find_if(gateway_handle->links, link_data_find, entryLink);

        if (link_data != NULL)
        {
            gateway_removelink_internal(gateway_handle, link_data);
            /*Codes_SRS_GATEWAY_26_018: [ The function shall report `GATEWAY_MODULE_LIST_CHANGED` event. ]*/
            EventSystem_ReportEvent(gw->event_system, gw, GATEWAY_MODULE_LIST_CHANGED);
        }
        else
        {
            LogError("Gateway_RemoveLink(): Could not find link given it's source/sink.");
        }
    }
}

/*Private*/

static void gateway_destroymodulelist_internal(GATEWAY_MODULE_INFO* infos, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        GATEWAY_MODULE_INFO *info = (infos + i);
        /*Codes_SRS_GATEWAY_26_011: [ This function shall destroy the `module_sources` list of each `GATEWAY_MODULE_INFO` ]*/
        VECTOR_destroy(info->module_sources);
    }
}

static bool module_data_find(const void* element, const void* value)
{
    return (*(MODULE_DATA**)element)->module == value;
}

static bool module_info_name_find(const void* element, const void* module_name)
{
    const char* name = (const char*)module_name;
    return (strcmp(((GATEWAY_MODULE_INFO*)element)->module_name, name) == 0);
}
