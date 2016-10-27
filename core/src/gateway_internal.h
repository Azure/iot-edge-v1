// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef GATEWAY_INTERNAL_H
#define GATEWAY_INTERNAL_H

#include "module_loader.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct MODULE_DATA_TAG {
    /** @brief  The name of the module added. This name is unique on a gateway.
     */
    char* module_name;

    /** @brief  The MODULE_LIBRARY_HANDLE associated with 'module' */
    MODULE_LIBRARY_HANDLE module_library_handle;

    /** @brief  The module loader associated with this module. */
    const MODULE_LOADER* module_loader;

    /** @brief  The MODULE_HANDLE of the same module that lives on the message
     *          broker.
     */
    MODULE_HANDLE module;
} MODULE_DATA;

typedef struct GATEWAY_HANDLE_DATA_TAG {

    /** @brief  Vector of MODULE_DATA modules that the Gateway must track */
    VECTOR_HANDLE modules;

    /** @brief  The message broker contained within this Gateway */
    BROKER_HANDLE broker;

    /** @brief  Handle for callback event system coupled with this Gateway */
    EVENTSYSTEM_HANDLE event_system;

    /** @brief  Vector of LINK_DATA links that the Gateway must track */
    VECTOR_HANDLE links;
} GATEWAY_HANDLE_DATA;

typedef struct LINK_DATA_TAG {
    bool from_any_source;
    MODULE_DATA *module_source;
    MODULE_DATA *module_sink;
} LINK_DATA;

GATEWAY_HANDLE gateway_create_internal(const GATEWAY_PROPERTIES* properties, bool use_json);
void gateway_destroy_internal(GATEWAY_HANDLE gw);
MODULE_HANDLE gateway_addmodule_internal(GATEWAY_HANDLE_DATA* gateway_handle, const GATEWAY_MODULES_ENTRY* entry, bool use_json);
void gateway_removemodule_internal(GATEWAY_HANDLE_DATA* gateway_handle, MODULE_DATA** module);
bool gateway_addlink_internal(GATEWAY_HANDLE_DATA* gateway_handle, const GATEWAY_LINK_ENTRY* link_entry);
void gateway_removelink_internal(GATEWAY_HANDLE_DATA* gateway_handle, LINK_DATA* link_data);
int add_module_to_any_source(GATEWAY_HANDLE_DATA* gateway_handle, MODULE_DATA* module);
void remove_module_from_any_source(GATEWAY_HANDLE_DATA* gateway_handle, MODULE_DATA* module);
int add_any_source_link(GATEWAY_HANDLE_DATA* gateway_handle, const GATEWAY_LINK_ENTRY* link_entry);
void remove_any_source_link(GATEWAY_HANDLE_DATA* gateway_handle, LINK_DATA* link_entry);
bool module_name_find(const void* element, const void* module_name);
bool link_data_find(const void* element, const void* link_data);

#ifdef __cplusplus
}
#endif

#endif // GATEWAY_INTERNAL_H
