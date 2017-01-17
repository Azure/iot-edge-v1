// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/lock.h"
#include "parson.h"

#include "azure_c_shared_utility/xlogging.h"

#include "module.h"
#include "module_loader.h"
#include "module_loaders/dynamic_loader.h"
#include "module_loaders/outprocess_loader.h"

#ifdef NODE_BINDING_ENABLED
#include "module_loaders/node_loader.h"
#endif
#ifdef JAVA_BINDING_ENABLED
#include "module_loaders/java_loader.h"
#endif
#ifdef DOTNET_BINDING_ENABLED
#include "module_loaders/dotnet_loader.h"
#endif
#ifdef DOTNET_CORE_BINDING_ENABLED
#include "module_loaders/dotnet_core_loader.h"
#endif

static MODULE_LOADER_RESULT add_module_loader(const MODULE_LOADER* loader);

static struct
{
    // List of module loaders that we support.
    VECTOR_HANDLE module_loaders;

    // Lock used to protect this instance.
    LOCK_HANDLE   lock;

} g_module_loaders = { 0 };

MODULE_LOADER_RESULT ModuleLoader_Initialize(void)
{
    MODULE_LOADER_RESULT result;

    /*Codes_SRS_MODULE_LOADER_13_001: [ ModuleLoader_Initialize shall initialize g_module_loader.lock. ]*/
    g_module_loaders.lock = Lock_Init();
    if (g_module_loaders.lock == NULL)
    {
        LogError("Lock_Init failed");

        /*Codes_SRS_MODULE_LOADER_13_002: [ ModuleLoader_Initialize shall return MODULE_LOADER_ERROR if an underlying platform call fails. ]*/
        result = MODULE_LOADER_ERROR;
    }
    else
    {
        /*Codes_SRS_MODULE_LOADER_13_003: [ ModuleLoader_Initialize shall acquire the lock on g_module_loader.lock. ]*/
        if (Lock(g_module_loaders.lock) != LOCK_OK)
        {
            LogError("Lock failed");
            Lock_Deinit(g_module_loaders.lock);
            g_module_loaders.lock = NULL;

            /*Codes_SRS_MODULE_LOADER_13_002: [ ModuleLoader_Initialize shall return MODULE_LOADER_ERROR if an underlying platform call fails. ]*/
            result = MODULE_LOADER_ERROR;
        }
        else
        {
            /*Codes_SRS_MODULE_LOADER_13_004: [ ModuleLoader_Initialize shall initialize g_module.module_loaders by calling VECTOR_create. ]*/
            g_module_loaders.module_loaders = VECTOR_create(sizeof(MODULE_LOADER*));
            if (g_module_loaders.module_loaders == NULL)
            {
                LogError("VECTOR_create failed");
                Unlock(g_module_loaders.lock);
                Lock_Deinit(g_module_loaders.lock);
                g_module_loaders.lock = NULL;

                /*Codes_SRS_MODULE_LOADER_13_002: [ ModuleLoader_Initialize shall return MODULE_LOADER_ERROR if an underlying platform call fails. ]*/
                result = MODULE_LOADER_ERROR;
            }
            else
            {
                // add all supported module loaders
                const MODULE_LOADER* supported_loaders[] =
                {
                    DynamicLoader_Get()
#ifdef NODE_BINDING_ENABLED
                    , NodeLoader_Get()
#endif
#ifdef JAVA_BINDING_ENABLED
                    , JavaLoader_Get()
#endif
#ifdef DOTNET_BINDING_ENABLED
                    , DotnetLoader_Get()
#endif
#ifdef DOTNET_CORE_BINDING_ENABLED
                    , DotnetCoreLoader_Get()
#endif
                    , OutprocessLoader_Get()
                };

                size_t loaders_count = sizeof(supported_loaders) / sizeof(supported_loaders[0]);
                size_t i;
                for (i = 0; i < loaders_count; i++)
                {
                    /*Codes_SRS_MODULE_LOADER_13_005: [ ModuleLoader_Initialize shall add the default support module loaders to g_module.module_loaders. ]*/
                    if (add_module_loader(supported_loaders[i]) != MODULE_LOADER_SUCCESS)
                    {
                        LogError("Could not add loader - %s", supported_loaders[i]->name);
                        break;
                    }
                }

                /*Codes_SRS_MODULE_LOADER_13_007: [ ModuleLoader_Initialize shall unlock g_module.lock. ]*/
                Unlock(g_module_loaders.lock);

                // adding loaders failed if we bailed early from the loop above
                if (i < loaders_count)
                {
                    ModuleLoader_Destroy();

                    /*Codes_SRS_MODULE_LOADER_13_002: [ ModuleLoader_Initialize shall return MODULE_LOADER_ERROR if an underlying platform call fails. ]*/
                    result = MODULE_LOADER_ERROR;
                }
                else
                {
                    /*Codes_SRS_MODULE_LOADER_13_006: [ ModuleLoader_Initialize shall return MODULE_LOADER_SUCCESS once all the default loaders have been added successfully. ]*/
                    result = MODULE_LOADER_SUCCESS;
                }
            }
        }
    }

    return result;
}

static bool validate_module_loader_api(MODULE_LOADER_API* api)
{
    return
        api != NULL &&
        api->BuildModuleConfiguration != NULL &&
        api->FreeConfiguration != NULL &&
        api->FreeEntrypoint != NULL &&
        api->FreeModuleConfiguration != NULL &&
        api->GetApi != NULL &&
        api->Load != NULL &&
        api->ParseConfigurationFromJson != NULL &&
        api->ParseEntrypointFromJson != NULL &&
        api->Unload != NULL;
}

MODULE_LOADER_RESULT ModuleLoader_Add(const MODULE_LOADER* loader)
{
    MODULE_LOADER_RESULT result;

    if (
        loader == NULL || loader->name == NULL || loader->type == UNKNOWN ||
        validate_module_loader_api(loader->api) == false
       )
    {
        LogError(
            "invalid input supplied. loader = %p, loader->api = %p, "
            "loader->name = %s, loader->type = %d",
            loader,
            loader != NULL ? loader->api : 0x0,
            loader != NULL ? loader->name : "NULL",
            loader != NULL ? loader->type : UNKNOWN
        );

        /*
        Codes_SRS_MODULE_LOADER_13_008: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader is NULL. ]
        Codes_SRS_MODULE_LOADER_13_011: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader->api is NULL ]
        Codes_SRS_MODULE_LOADER_13_012: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader->name is NULL ]
        Codes_SRS_MODULE_LOADER_13_013: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader->type is UNKNOWN ]
        Codes_SRS_MODULE_LOADER_13_014: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader->api->BuildModuleConfiguration is NULL ]
        Codes_SRS_MODULE_LOADER_13_015: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader->api->FreeConfiguration is NULL ]
        Codes_SRS_MODULE_LOADER_13_016: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader->api->FreeEntrypoint is NULL ]
        Codes_SRS_MODULE_LOADER_13_017: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader->api->FreeModuleConfiguration is NULL ]
        Codes_SRS_MODULE_LOADER_13_018: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader->api->GetApi is NULL ]
        Codes_SRS_MODULE_LOADER_13_019: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader->api->Load is NULL ]
        Codes_SRS_MODULE_LOADER_13_020: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader->api->ParseConfigurationFromJson is NULL ]
        Codes_SRS_MODULE_LOADER_13_021: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader->api->ParseEntrypointFromJson is NULL ]
        Codes_SRS_MODULE_LOADER_13_022: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader->api->Unload is NULL ]
        */
        result = MODULE_LOADER_ERROR;
    }
    else
    {
        if (g_module_loaders.lock == NULL || g_module_loaders.module_loaders == NULL)
        {
            LogError("ModuleLoader_Initialize has not been called yet");

            /*
            Codes_SRS_MODULE_LOADER_13_009: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if g_module_loaders.lock is NULL. ]
            Codes_SRS_MODULE_LOADER_13_010: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if g_module_loader.module_loaders is NULL. ]
            */
            result = MODULE_LOADER_ERROR;
        }
        else
        {
            /*Codes_SRS_MODULE_LOADER_13_027: [ ModuleLoader_Add shall lock g_module_loaders.lock. ]*/
            if (Lock(g_module_loaders.lock) != LOCK_OK)
            {
                LogError("Lock failed");

                /*Codes_SRS_MODULE_LOADER_13_023: [ModuleLoader_Add shall return MODULE_LOADER_ERROR if an underlying platform call fails..]*/
                result = MODULE_LOADER_ERROR;
            }
            else
            {
                /*Codes_SRS_MODULE_LOADER_13_024: [ ModuleLoader_Add shall add the new module to the global module loaders list. ]*/
                if (add_module_loader(loader) != MODULE_LOADER_SUCCESS)
                {
                    LogError("add_module_loader failed");

                    /*Codes_SRS_MODULE_LOADER_13_023: [ModuleLoader_Add shall return MODULE_LOADER_ERROR if an underlying platform call fails..]*/
                    result = MODULE_LOADER_ERROR;
                }
                else
                {
                    /*Codes_SRS_MODULE_LOADER_13_026: [ ModuleLoader_Add shall return MODULE_LOADER_SUCCESS if the loader has been added successfully. ]*/
                    result = MODULE_LOADER_SUCCESS;
                }

                /*Codes_SRS_MODULE_LOADER_13_025: [ ModuleLoader_Add shall unlock g_module_loaders.lock. ]*/
                Unlock(g_module_loaders.lock);
            }
        }
    }

    return result;
}

MODULE_LOADER_RESULT ModuleLoader_UpdateConfiguration(
    MODULE_LOADER* loader,
    MODULE_LOADER_BASE_CONFIGURATION* configuration
)
{
    MODULE_LOADER_RESULT result;
    if (loader == NULL)
    {
        LogError("loader is NULL");

        /*Codes_SRS_MODULE_LOADER_13_028: [ ModuleLoader_UpdateConfiguration shall return MODULE_LOADER_ERROR if loader is NULL. ]*/
        result = MODULE_LOADER_ERROR;
    }
    else
    {
        if (g_module_loaders.lock == NULL || g_module_loaders.module_loaders == NULL)
        {
            LogError("ModuleLoader_Initialize has not been called yet");

            /*
            Codes_SRS_MODULE_LOADER_13_029: [ ModuleLoader_UpdateConfiguration shall return MODULE_LOADER_ERROR if g_module_loaders.lock is NULL. ]
            Codes_SRS_MODULE_LOADER_13_030: [ ModuleLoader_UpdateConfiguration shall return MODULE_LOADER_ERROR if g_module_loaders.module_loaders is NULL. ]
            */
            result = MODULE_LOADER_ERROR;
        }
        else
        {
            /*Codes_SRS_MODULE_LOADER_13_032: [ ModuleLoader_UpdateConfiguration shall lock g_module_loaders.lock ]*/
            if (Lock(g_module_loaders.lock) != LOCK_OK)
            {
                LogError("Lock failed");

                /*Codes_SRS_MODULE_LOADER_13_031: [ ModuleLoader_UpdateConfiguration shall return MODULE_LOADER_ERROR if an underlying platform call fails. ]*/
                result = MODULE_LOADER_ERROR;
            }
            else
            {
                /*Codes_SRS_MODULE_LOADER_13_074: [If the existing configuration on the loader is not NULL ModuleLoader_UpdateConfiguration shall call FreeConfiguration on the configuration pointer.]*/
                if (loader->configuration != NULL)
                {
                    loader->api->FreeConfiguration(loader, loader->configuration);
                }

                /*Codes_SRS_MODULE_LOADER_13_033: [ ModuleLoader_UpdateConfiguration shall assign configuration to the module loader. ]*/
                loader->configuration = configuration;

                /*Codes_SRS_MODULE_LOADER_13_034: [ ModuleLoader_UpdateConfiguration shall unlock g_module_loaders.lock. ]*/
                Unlock(g_module_loaders.lock);

                /*Codes_SRS_MODULE_LOADER_13_035: [ ModuleLoader_UpdateConfiguration shall return MODULE_LOADER_SUCCESS if the loader has been updated successfully. ]*/
                result = MODULE_LOADER_SUCCESS;
            }
        }
    }

    return result;
}

static bool find_module_loader_predicate(const void* element, const void* value)
{
    MODULE_LOADER* loader = *((MODULE_LOADER**)element);
    return strcmp(loader->name, (const char*)value) == 0;
}

MODULE_LOADER* ModuleLoader_FindByName(const char* name)
{
    MODULE_LOADER* result;
    if (name == NULL)
    {
        LogError("name is NULL");

        /*Codes_SRS_MODULE_LOADER_13_036: [ ModuleLoader_FindByName shall return NULL if name is NULL. ]*/
        result = NULL;
    }
    else
    {
        if (g_module_loaders.lock == NULL || g_module_loaders.module_loaders == NULL)
        {
            LogError("ModuleLoader_Initialize has not been called yet");

            /*Codes_SRS_MODULE_LOADER_13_037: [ ModuleLoader_FindByName shall return NULL if g_module_loaders.lock is NULL. ]*/
            /*Codes_SRS_MODULE_LOADER_13_038: [ ModuleLoader_FindByName shall return NULL if g_module_loaders.module_loaders is NULL.]*/
            result = NULL;
        }
        else
        {
            /*Codes_SRS_MODULE_LOADER_13_040: [ ModuleLoader_FindByName shall lock g_module_loaders.lock. ]*/
            if (Lock(g_module_loaders.lock) != LOCK_OK)
            {
                LogError("Lock failed");

                /*Codes_SRS_MODULE_LOADER_13_039: [ ModuleLoader_FindByName shall return NULL if an underlying platform call fails. ]*/
                result = NULL;
            }
            else
            {
                /*Codes_SRS_MODULE_LOADER_13_041: [ ModuleLoader_FindByName shall search for a module loader whose name is equal to name. The comparison is case sensitive. ]*/
                /*Codes_SRS_MODULE_LOADER_13_042: [ ModuleLoader_FindByName shall return NULL if a matching module loader is not found. ]*/
                /*Codes_SRS_MODULE_LOADER_13_043: [ ModuleLoader_FindByName shall return a pointer to the MODULE_LOADER if a matching entry is found. ]*/
                MODULE_LOADER** loader_ref = (MODULE_LOADER**)VECTOR_find_if(
                    g_module_loaders.module_loaders,
                    find_module_loader_predicate,
                    name
                );
                result = loader_ref == NULL ? NULL : *loader_ref;

                /*Codes_SRS_MODULE_LOADER_13_044: [ ModuleLoader_FindByName shall unlock g_module_loaders.lock. ]*/
                Unlock(g_module_loaders.lock);
            }
        }
    }

    return result;
}

void ModuleLoader_Destroy(void)
{
    if (g_module_loaders.lock != NULL)
    {
        if (Lock(g_module_loaders.lock) != LOCK_OK)
        {
            LogError("Lock failed. Proceeding with destruction anyway.");
        }
    }
    if(g_module_loaders.module_loaders != NULL)
    {
        // free all module loader resources
        size_t length = VECTOR_size(g_module_loaders.module_loaders);
        for (size_t i = 0; i < length; i++)
        {
            MODULE_LOADER* loader = *((MODULE_LOADER **)VECTOR_element(g_module_loaders.module_loaders, i));

            // NOTE: We free the configuration object even for default loaders because
            // the configuration may have been replaced by the gateway for default
            // loaders.

            /*Codes_SRS_MODULE_LOADER_13_046: [ ModuleLoader_Destroy shall invoke FreeConfiguration on every module loader's configuration field. ]*/
            if (loader->configuration != NULL)
            {
                loader->api->FreeConfiguration(loader, loader->configuration);
            }

            // if this is not a default loader then free resources allocated in
            // add_loader_from_json
            if (ModuleLoader_IsDefaultLoader(loader->name) == false)
            {
                /*Codes_SRS_MODULE_LOADER_13_047: [ ModuleLoader_Destroy shall free the loader's name and the loader itself if it is not a default loader. ]*/
                free((void *)loader->name);
                free(loader);
            }
        }

        /*Codes_SRS_MODULE_LOADER_13_048: [ ModuleLoader_Destroy shall destroy the loaders vector. ]*/
        VECTOR_destroy(g_module_loaders.module_loaders);
        g_module_loaders.module_loaders = NULL;
    }

    if (g_module_loaders.lock != NULL)
    {
        if (Unlock(g_module_loaders.lock) != LOCK_OK)
        {
            LogError("Unlock failed.");
        }

        /*Codes_SRS_MODULE_LOADER_13_045: [ ModuleLoader_Destroy shall free g_module_loaders.lock if it is not NULL. ]*/
        Lock_Deinit(g_module_loaders.lock);
        g_module_loaders.lock = NULL;
    }
}

static MODULE_LOADER_RESULT add_module_loader(const MODULE_LOADER* loader)
{
    MODULE_LOADER_RESULT result;

    if (VECTOR_push_back(g_module_loaders.module_loaders, &loader, 1) == 0)
    {
        result = MODULE_LOADER_SUCCESS;
    }
    else
    {
        LogError("VECTOR_push_back failed");
        result = MODULE_LOADER_ERROR;
    }

    return result;
}

MODULE_LOADER_RESULT ModuleLoader_ParseBaseConfigurationFromJson(
    MODULE_LOADER_BASE_CONFIGURATION* configuration,
    const JSON_Value* json
)
{
    // The JSON is expected to be an object that has a string
    // property called "binding.path"
    MODULE_LOADER_RESULT result;

    JSON_Value_Type value_type = json_value_get_type(json);
    if (
        configuration == NULL ||
        json == NULL ||
        value_type != JSONObject
       )
    {
        LogError(
            "Invalid input arguments. "
            "configuration = %p, json = %p, json value type = %d",
            configuration,
            json,
            (json != NULL) ? value_type : 0
        );

        /*
        Codes_SRS_MODULE_LOADER_13_049: [ ModuleLoader_ParseBaseConfigurationFromJson shall return MODULE_LOADER_ERROR if configuration is NULL. ]
        Codes_SRS_MODULE_LOADER_13_050 : [ModuleLoader_ParseBaseConfigurationFromJson shall return MODULE_LOADER_ERROR if json is NULL.]
        Codes_SRS_MODULE_LOADER_13_051 : [ModuleLoader_ParseBaseConfigurationFromJson shall return MODULE_LOADER_ERROR if json is not a JSON object.]
        */
        result = MODULE_LOADER_ERROR;
    }
    else
    {
        JSON_Object* config = json_value_get_object(json);
        if (config == NULL)
        {
            LogError("json_value_get_object failed");

            /*Codes_SRS_MODULE_LOADER_13_052: [ ModuleLoader_ParseBaseConfigurationFromJson shall return MODULE_LOADER_ERROR if an underlying platform call fails. ]*/
            result = MODULE_LOADER_ERROR;
        }
        else
        {
            // It is acceptable to have binding.path be NULL.

            /*Codes_SRS_MODULE_LOADER_13_053: [ ModuleLoader_ParseBaseConfigurationFromJson shall read the value of the string attribute binding.path from the JSON object and assign to configuration->binding_path. ]*/
            const char* binding_path = json_object_get_string(config, "binding.path");
            configuration->binding_path = STRING_construct(binding_path);

            /*Codes_SRS_MODULE_LOADER_13_054: [ ModuleLoader_ParseBaseConfigurationFromJson shall return MODULE_LOADER_SUCCESS if the parsing is successful. ]*/
            result = MODULE_LOADER_SUCCESS;
        }
    }

    return result;
}

void ModuleLoader_FreeBaseConfiguration(MODULE_LOADER_BASE_CONFIGURATION* configuration)
{
    if(configuration != NULL)
    {
        /*Codes_SRS_MODULE_LOADER_13_056: [ ModuleLoader_FreeBaseConfiguration shall free configuration->binding_path. ]*/
        STRING_delete(configuration->binding_path);
    }
    else
    {
        /*Codes_SRS_MODULE_LOADER_13_055: [ ModuleLoader_FreeBaseConfiguration shall do nothing if configuration is NULL. ]*/
        LogError("configuration is NULL");
    }
}

MODULE_LOADER* ModuleLoader_GetDefaultLoaderForType(MODULE_LOADER_TYPE type)
{
    MODULE_LOADER* result;

    switch (type)
    {
    case NATIVE:
        /*Codes_SRS_MODULE_LOADER_13_058: [ ModuleLoader_GetDefaultLoaderForType shall return a non-NULL MODULE_LOADER pointer when the loader type is a recongized type. ]*/
        result = ModuleLoader_FindByName(DYNAMIC_LOADER_NAME);
        break;

#ifdef NODE_BINDING_ENABLED
    case NODEJS:
        /*Codes_SRS_MODULE_LOADER_13_058: [ ModuleLoader_GetDefaultLoaderForType shall return a non-NULL MODULE_LOADER pointer when the loader type is a recongized type. ]*/
        result = ModuleLoader_FindByName(NODE_LOADER_NAME);
        break;
#endif

#ifdef JAVA_BINDING_ENABLED
    case JAVA:
        /*Codes_SRS_MODULE_LOADER_13_058: [ ModuleLoader_GetDefaultLoaderForType shall return a non-NULL MODULE_LOADER pointer when the loader type is a recongized type. ]*/
        result = ModuleLoader_FindByName(JAVA_LOADER_NAME);
        break;
#endif

#ifdef DOTNET_BINDING_ENABLED
    case DOTNET:
        /*Codes_SRS_MODULE_LOADER_13_058: [ ModuleLoader_GetDefaultLoaderForType shall return a non-NULL MODULE_LOADER pointer when the loader type is a recongized type. ]*/
        result = ModuleLoader_FindByName(DOTNET_LOADER_NAME);
        break;
#endif

    default:
        /*Codes_SRS_MODULE_LOADER_13_057: [ ModuleLoader_GetDefaultLoaderForType shall return NULL if type is not a recongized loader type. ]*/
        result = NULL;
        break;
    }

    return result;
}

MODULE_LOADER_TYPE ModuleLoader_ParseType(const char* type)
{
    MODULE_LOADER_TYPE loader_type;
    if (strcmp(type, "native") == 0)
        /*Codes_SRS_MODULE_LOADER_13_060: [ ModuleLoader_ParseType shall return a valid MODULE_LOADER_TYPE if type is a recognized module loader type string. ]*/
        loader_type = NATIVE;
    else if (strcmp(type, "outprocess") == 0)
        /*Codes_SRS_MODULE_LOADER_13_060: [ ModuleLoader_ParseType shall return a valid MODULE_LOADER_TYPE if type is a recognized module loader type string. ]*/
        loader_type = OUTPROCESS;
    else if (strcmp(type, "node") == 0)
        /*Codes_SRS_MODULE_LOADER_13_060: [ ModuleLoader_ParseType shall return a valid MODULE_LOADER_TYPE if type is a recognized module loader type string. ]*/
        loader_type = NODEJS;
    else if (strcmp(type, "java") == 0)
        /*Codes_SRS_MODULE_LOADER_13_060: [ ModuleLoader_ParseType shall return a valid MODULE_LOADER_TYPE if type is a recognized module loader type string. ]*/
        loader_type = JAVA;
    else if (strcmp(type, "dotnet") == 0)
        /*Codes_SRS_MODULE_LOADER_13_060: [ ModuleLoader_ParseType shall return a valid MODULE_LOADER_TYPE if type is a recognized module loader type string. ]*/
        loader_type = DOTNET;
    else if (strcmp(type, "dotnetcore") == 0)
        /*Codes_SRS_MODULE_LOADER_13_060: [ ModuleLoader_ParseType shall return a valid MODULE_LOADER_TYPE if type is a recognized module loader type string. ]*/
        loader_type = DOTNETCORE;
    else
        /*Codes_SRS_MODULE_LOADER_13_059: [ ModuleLoader_ParseType shall return UNKNOWN if type is not a recognized module loader type string. ]*/
        loader_type = UNKNOWN;

    return loader_type;
}

bool ModuleLoader_IsDefaultLoader(const char* name)
{
    /*Codes_SRS_MODULE_LOADER_13_061: [ ModuleLoader_IsDefaultLoader shall return true if name is the name of a default module loader and false otherwise. The default module loader names are 'native', 'node', 'java' , 'dotnet' and 'dotnetcore'. ]*/
    return strcmp(name, DYNAMIC_LOADER_NAME) == 0
           ||
           strcmp(name, OUTPROCESS_LOADER_NAME) == 0
           ||
           strcmp(name, "node") == 0
           ||
           strcmp(name, "java") == 0
           ||
           strcmp(name, "dotnet") == 0
           ||
           strcmp(name, "dotnetcore") == 0;
}

static MODULE_LOADER_RESULT add_loader_from_json(const JSON_Value* loader, size_t index)
{
    MODULE_LOADER_RESULT result;

    if (json_value_get_type(loader) != JSONObject)
    {
        LogError("loader %zu is not a JSON object", index);

        /*Codes_SRS_MODULE_LOADER_13_065: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if an entry in the loaders array is not a JSON object. ]*/
        result = MODULE_LOADER_ERROR;
    }
    else
    {
        JSON_Object* loader_object = json_value_get_object(loader);
        if (loader_object == NULL)
        {
            LogError("json_value_get_object failed for loader %zu", index);

            /*Codes_SRS_MODULE_LOADER_13_064: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if an underlying platform call fails. ]*/
            result = MODULE_LOADER_ERROR;
        }
        else
        {
            const char* type = json_object_get_string(loader_object, "type");
            const char* loader_name = json_object_get_string(loader_object, "name");
            if ((type == NULL || loader_name == NULL) ||
                (strlen(type) == 0 || strlen(loader_name) == 0))
            {
                LogError(
                    "invalid type and/or name for loader %zu. "
                    "type = %s, name = %s",
                    index, type, loader_name
                );

                /*Codes_SRS_MODULE_LOADER_13_066: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if a loader entry's name or type fields are NULL or are empty strings. ]*/
                result = MODULE_LOADER_ERROR;
            }
            else
            {
                MODULE_LOADER_TYPE loader_type = ModuleLoader_ParseType(type);
                if (loader_type == UNKNOWN)
                {
                    LogError("loader type %s for loader %zu is invalid", type, index);

                    /*Codes_SRS_MODULE_LOADER_13_067: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if a loader entry's type field has an unknown value. ]*/
                    result = MODULE_LOADER_ERROR;
                }
                else
                {
                    // get the default loader for the given type
                    MODULE_LOADER* default_loader = ModuleLoader_GetDefaultLoaderForType(loader_type);
                    if (default_loader == NULL)
                    {
                        LogError(
                            "no default module loader for type %s was "
                            "found for loader entry %zu",
                            type, index
                        );

                        /*Codes_SRS_MODULE_LOADER_13_068: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if no default loader could be located for a loader entry. ]*/
                        result = MODULE_LOADER_ERROR;
                    }
                    else
                    {
                        JSON_Value* configuration = json_object_get_value(loader_object, "configuration");
                        MODULE_LOADER_BASE_CONFIGURATION* loader_configuration;
                        if (configuration != NULL)
                        {
                            /*Codes_SRS_MODULE_LOADER_13_069: [ ModuleLoader_InitializeFromJson shall invoke ParseConfigurationFromJson to parse the loader entry's configuration JSON. ]*/
                            loader_configuration = default_loader->api->ParseConfigurationFromJson(default_loader, configuration);
                        }
                        else
                        {
                            loader_configuration = NULL;
                        }

                        // now we have all the information we need to construct/replace a MODULE_LOADER
                        bool is_default_loader = ModuleLoader_IsDefaultLoader(loader_name);
                        MODULE_LOADER* new_loader = is_default_loader ? default_loader :
                                                        (MODULE_LOADER*)malloc(sizeof(MODULE_LOADER));
                        if (new_loader == NULL)
                        {
                            LogError("malloc failed for MODULE_LOADER struct for loader %zu", index);

                            /*Codes_SRS_MODULE_LOADER_13_064: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if an underlying platform call fails. ]*/
                            result = MODULE_LOADER_ERROR;
                        }
                        else
                        {
                            if (!is_default_loader)
                            {
                                new_loader->name = malloc(strlen(loader_name) + 1);
                                if (new_loader->name == NULL)
                                {
                                    LogError("malloc failed for loader name string for loader %zu", index);
                                    if (loader_configuration != NULL)
                                    {
                                        default_loader->api->FreeConfiguration(default_loader, loader_configuration);
                                    }
                                    free(new_loader);

                                    /*Codes_SRS_MODULE_LOADER_13_064: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if an underlying platform call fails. ]*/
                                    result = MODULE_LOADER_ERROR;
                                }
                                else
                                {
                                    strcpy((char *)new_loader->name, loader_name);
                                    new_loader->type = loader_type;
                                    new_loader->configuration = loader_configuration;
                                    new_loader->api = default_loader->api;

                                    /*Codes_SRS_MODULE_LOADER_13_070: [ ModuleLoader_InitializeFromJson shall allocate a MODULE_LOADER and add the loader to the gateway by calling ModuleLoader_Add if the loader entry is not for a default loader. ]*/

                                    // add this module to the loaders list
                                    result = ModuleLoader_Add(new_loader);
                                    if (result != MODULE_LOADER_SUCCESS)
                                    {
                                        /*Codes_SRS_MODULE_LOADER_13_071: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if ModuleLoader_Add fails. ]*/
                                        LogError("ModuleLoader_Add failed");
                                        if (loader_configuration != NULL)
                                        {
                                            default_loader->api->FreeConfiguration(default_loader, loader_configuration);
                                        }
                                        free((void *)new_loader->name);
                                        free(new_loader);
                                    }
                                }
                            }
                            else
                            {
                                /*Codes_SRS_MODULE_LOADER_13_072: [ ModuleLoader_InitializeFromJson shall update the configuration on the default loader if the entry is for a default loader by calling ModuleLoader_UpdateConfiguration. ]*/

                                // since we are updating the configuration on an existing loader, we only
                                // replace the configuration field
                                result = ModuleLoader_UpdateConfiguration(new_loader, loader_configuration);
                            }
                        }
                    }
                }
            }
        }
    }

    return result;
}

MODULE_LOADER_RESULT ModuleLoader_InitializeFromJson(const JSON_Value* loaders)
{
    MODULE_LOADER_RESULT result;

    if (loaders == NULL)
    {
        LogError("loaders is NULL");

        /*Codes_SRS_MODULE_LOADER_13_062: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if loaders is NULL. ]*/
        result = MODULE_LOADER_ERROR;
    }
    else
    {
        if (json_value_get_type(loaders) != JSONArray)
        {
            LogError("loaders is not a JSON array");

            /*Codes_SRS_MODULE_LOADER_13_063: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if loaders is not a JSON array. ]*/
            result = MODULE_LOADER_ERROR;
        }
        else
        {
            JSON_Array* loaders_array = json_value_get_array(loaders);
            if (loaders_array == NULL)
            {
                LogError("json_value_get_array failed");

                /*Codes_SRS_MODULE_LOADER_13_064: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if an underlying platform call fails. ]*/
                result = MODULE_LOADER_ERROR;
            }
            else
            {
                size_t length = json_array_get_count(loaders_array);
                size_t i;
                for (i = 0; i < length; i++)
                {
                    JSON_Value* loader = json_array_get_value(loaders_array, i);
                    if (loader == NULL)
                    {
                        /*Codes_SRS_MODULE_LOADER_13_064: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if an underlying platform call fails. ]*/
                        LogError("json_array_get_value failed for loader %zu", i);
                        break;
                    }

                    if (add_loader_from_json(loader, i) != MODULE_LOADER_SUCCESS)
                    {
                        LogError("add_loader_from_json failed for loader %zu", i);
                        break;
                    }
                }

                /*Codes_SRS_MODULE_LOADER_13_073: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_SUCCESS if the the JSON has been processed successfully. ]*/

                // if the loop terminated early then something went wrong
                result = (i < length) ? MODULE_LOADER_ERROR : MODULE_LOADER_SUCCESS;
            }
        }
    }

    return result;
}
