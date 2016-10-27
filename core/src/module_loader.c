// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <string.h>

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/lock.h"
#include "parson.h"

#include "azure_c_shared_utility/xlogging.h"

#include "module.h"
#include "module_loader.h"
#include "module_loaders/dynamic_loader.h"

#ifdef NODE_BINDING_ENABLED
#include "module_loaders/node_loader.h"
#endif
#ifdef JAVA_BINDING_ENABLED
#include "module_loaders/java_loader.h"
#endif
#ifdef DOTNET_BINDING_ENABLED
#include "module_loaders/dotnet_loader.h"
#endif

MODULE_LOADER_RESULT add_module_loader(const MODULE_LOADER* loader);

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

    g_module_loaders.lock = Lock_Init();
    if (g_module_loaders.lock == NULL)
    {
        LogError("ModuleLoader_Initialize() - Lock_Init failed");
        result = MODULE_LOADER_ERROR;
    }
    else
    {
        if (Lock(g_module_loaders.lock) != LOCK_OK)
        {
            LogError("ModuleLoader_Initialize() - Lock failed");
            Lock_Deinit(g_module_loaders.lock);
            g_module_loaders.lock = NULL;
            result = MODULE_LOADER_ERROR;
        }
        else
        {
            g_module_loaders.module_loaders = VECTOR_create(sizeof(MODULE_LOADER*));
            if (g_module_loaders.module_loaders == NULL)
            {
                LogError("ModuleLoader_Initialize() - VECTOR_create failed");
                Unlock(g_module_loaders.lock);
                Lock_Deinit(g_module_loaders.lock);
                g_module_loaders.lock = NULL;
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
                };

                size_t loaders_count = sizeof(supported_loaders) / sizeof(supported_loaders[0]);
                size_t i;
                for (i = 0; i < loaders_count; i++)
                {
                    if (add_module_loader(NodeLoader_Get()) != MODULE_LOADER_SUCCESS)
                    {
                        LogError("ModuleLoader_Initialize() - Could not add loader");
                        break;
                    }
                }

                Unlock(g_module_loaders.lock);

                // adding loaders failed if we bailed early from the loop above
                if (i < loaders_count)
                {
                    ModuleLoader_Destroy();
                    result = MODULE_LOADER_ERROR;
                }
            }
        }
    }

    return result;
}

MODULE_LOADER_RESULT ModuleLoader_Add(const MODULE_LOADER* loader)
{
    MODULE_LOADER_RESULT result;
    if (loader == NULL)
    {
        LogError("ModuleLoader_Add() - loader is NULL");
        result = MODULE_LOADER_ERROR;
    }
    else
    {
        if (g_module_loaders.lock == NULL || g_module_loaders.module_loaders == NULL)
        {
            LogError("ModuleLoader_Add() - ModuleLoader_Initialize has not been called yet");
            result = MODULE_LOADER_ERROR;
        }
        else
        {
            if (Lock(g_module_loaders.lock) != LOCK_OK)
            {
                LogError("ModuleLoader_Add() - Lock failed");
                result = MODULE_LOADER_ERROR;
            }
            else
            {
                if (add_module_loader(loader) != MODULE_LOADER_SUCCESS)
                {
                    LogError("ModuleLoader_Add() - add_module_loader failed");
                    result = MODULE_LOADER_ERROR;
                }
                else
                {
                    result = MODULE_LOADER_SUCCESS;
                }

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
        LogError("ModuleLoader_UpdateConfiguration() - loader is NULL");
        result = MODULE_LOADER_ERROR;
    }
    else
    {
        if (g_module_loaders.lock == NULL || g_module_loaders.module_loaders == NULL)
        {
            LogError("ModuleLoader_UpdateConfiguration() - ModuleLoader_Initialize has not been called yet");
            result = MODULE_LOADER_ERROR;
        }
        else
        {
            if (Lock(g_module_loaders.lock) != LOCK_OK)
            {
                LogError("ModuleLoader_UpdateConfiguration() - Lock failed");
                result = MODULE_LOADER_ERROR;
            }
            else
            {
                loader->configuration = configuration;
                Unlock(g_module_loaders.lock);
                result = MODULE_LOADER_SUCCESS;
            }
        }
    }

    return result;
}

static bool find_module_loader_predicate(const void* element, const void* value)
{
    MODULE_LOADER* loader = (MODULE_LOADER*)element;
    return strcmp(loader->name, (const char*)value) == 0;
}

MODULE_LOADER* ModuleLoader_FindByName(const char* name)
{
    MODULE_LOADER* result;
    if (name == NULL)
    {
        LogError("ModuleLoader_FindByName() - name is NULL");
        result = NULL;
    }
    else
    {
        if (g_module_loaders.lock == NULL || g_module_loaders.module_loaders == NULL)
        {
            LogError("ModuleLoader_FindByName() - ModuleLoader_Initialize has not been called yet");
            result = NULL;
        }
        else
        {
            if (Lock(g_module_loaders.lock) != LOCK_OK)
            {
                LogError("ModuleLoader_FindByName() - Lock failed");
                result = NULL;
            }
            else
            {
                result = VECTOR_find_if(
                    g_module_loaders.module_loaders,
                    find_module_loader_predicate,
                    name
                );

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
        Lock_Deinit(g_module_loaders.lock);
        g_module_loaders.lock = NULL;
    }
    if(g_module_loaders.module_loaders != NULL)
    {
        // free all module loader resources
        size_t length = VECTOR_size(g_module_loaders.module_loaders);
        for (size_t i = 0; i < length; i++)
        {
            MODULE_LOADER* loader = *((MODULE_LOADER **)VECTOR_element(g_module_loaders.module_loaders, i));

            // NOTE: We free the configuration object even for default loaders because
            // the configuration may have been replaced by the gateway even for default
            // loaders.
            loader->api->FreeConfiguration(loader->configuration);

            // if this is not a default loader then free resources allocated in
            // add_loader_from_json
            if (ModuleLoader_IsDefaultLoader(loader->name) == false)
            {
                free((void *)loader->name);
                free(loader);
            }
        }

        VECTOR_destroy(g_module_loaders.module_loaders);
        g_module_loaders.module_loaders = NULL;
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
        LogError("add_module_loader() - VECTOR_push_back failed");
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
    if (
        configuration == NULL ||
        json == NULL ||
        json_value_get_type(json) != JSONObject
       )
    {
        LogError(
            "ModuleLoader_ParseBaseConfigurationFromJson() - Invalid input arguments. "
            "configuration = %p, json = %p, json value type = %d",
            configuration,
            json,
            (json != NULL) ? json_value_get_type(json) : 0
        );
        result = MODULE_LOADER_ERROR;
    }
    else
    {
        JSON_Object* config = json_value_get_object(json);
        if (config == NULL)
        {
            LogError("ModuleLoader_ParseBaseConfigurationFromJson() - json_value_get_object failed");
            result = MODULE_LOADER_ERROR;
        }
        else
        {
            // It is acceptable to have binding.path be NULL.
            const char* binding_path = json_object_get_string(config, "binding.path");
            configuration->binding_path = STRING_construct(binding_path);

            result = MODULE_LOADER_SUCCESS;
        }
    }

    return result;
}

void ModuleLoader_FreeBaseConfiguration(MODULE_LOADER_BASE_CONFIGURATION* configuration)
{
    if(configuration != NULL)
    {
        STRING_delete(configuration->binding_path);
    }
    else
    {
        LogError("ModuleLoader_FreeBaseConfiguration() - configuration is NULL");
    }
}

MODULE_LOADER* ModuleLoader_GetDefaultLoaderForType(MODULE_LOADER_TYPE type)
{
    MODULE_LOADER* result;

    switch (type)
    {
    case NATIVE:
        result = ModuleLoader_FindByName(DYNAMIC_LOADER_NAME);
        break;

#ifdef NODE_BINDING_ENABLED
    case NODEJS:
        result = ModuleLoader_FindByName(NODE_LOADER_NAME);
        break;
#endif

#ifdef JAVA_BINDING_ENABLED
    case JAVA:
        result = ModuleLoader_FindByName(JAVA_LOADER_NAME);
        break;
#endif

#ifdef DOTNET_BINDING_ENABLED
    case DOTNET:
        result = ModuleLoader_FindByName(DOTNET_LOADER_NAME);
        break;
#endif

    default:
        result = NULL;
        break;
    }

    return result;
}

MODULE_LOADER_TYPE ModuleLoader_ParseType(const char* type)
{
    MODULE_LOADER_TYPE loader_type;
    if (strcmp(type, "native") == 0)
        loader_type = NATIVE;
    else if (strcmp(type, "node") == 0)
        loader_type = NODEJS;
    else if (strcmp(type, "java") == 0)
        loader_type = JAVA;
    else if (strcmp(type, "dotnet") == 0)
        loader_type = DOTNET;
    else
        loader_type = UNKNOWN;

    return loader_type;
}

bool ModuleLoader_IsDefaultLoader(const char* name)
{
    return strcmp(name, "native") == 0
           ||
           strcmp(name, "node") == 0
           ||
           strcmp(name, "java") == 0
           ||
           strcmp(name, "dotnet") == 0;
}

static MODULE_LOADER_RESULT add_loader_from_json(const JSON_Value* loader, size_t index)
{
    MODULE_LOADER_RESULT result;

    if (json_value_get_type(loader) != JSONObject)
    {
        LogError("add_loader_from_json() - loader %zu is not a JSON object", index);
        result = MODULE_LOADER_ERROR;
    }
    else
    {
        JSON_Object* loader_object = json_value_get_object(loader);
        if (loader_object == NULL)
        {
            LogError("add_loader_from_json() - json_value_get_object failed for loader %zu", index);
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
                    "add_loader_from_json() - invalid type and/or name for loader %zu. "
                    "type = %s, name = %s",
                    index, type, loader_name
                );
                result = MODULE_LOADER_ERROR;
            }
            else
            {
                MODULE_LOADER_TYPE loader_type = ModuleLoader_ParseType(type);
                if (loader_type == UNKNOWN)
                {
                    LogError("add_loader_from_json() - loader type %s for loader %zu is invalid", type, index);
                    result = MODULE_LOADER_ERROR;
                }
                else
                {
                    // get the default loader for the given type
                    MODULE_LOADER* default_loader = ModuleLoader_GetDefaultLoaderForType(loader_type);
                    if (default_loader == NULL)
                    {
                        LogError(
                            "add_loader_from_json() - no default module loader for type %s was "
                            "found for loader entry %zu",
                            type, index
                        );
                        result = MODULE_LOADER_ERROR;
                    }
                    else
                    {
                        JSON_Value* configuration = json_object_get_value(loader_object, "configuration");
                        MODULE_LOADER_BASE_CONFIGURATION* loader_configuration;
                        if (configuration != NULL)
                        {
                            loader_configuration = default_loader->api->ParseConfigurationFromJson(configuration);
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
                            LogError("add_loader_from_json() - malloc failed for MODULE_LOADER struct for loader %zu", index);
                            result = MODULE_LOADER_ERROR;
                        }
                        else
                        {
                            if (!is_default_loader)
                            {
                                new_loader->name = malloc(strlen(loader_name) + 1);
                                if (new_loader->name == NULL)
                                {
                                    LogError("add_loader_from_json() - malloc failed for loader name string for loader %zu", index);
                                    result = MODULE_LOADER_ERROR;
                                }
                                else
                                {
                                    strcpy((char *)new_loader->name, loader_name);
                                    new_loader->type = loader_type;
                                    new_loader->configuration = loader_configuration;
                                    new_loader->api = default_loader->api;

                                    // add this module to the loaders list
                                    result = ModuleLoader_Add(new_loader);
                                    if (result != MODULE_LOADER_SUCCESS)
                                    {
                                        LogError("add_loader_from_json() - ModuleLoader_Add failed");
                                        if (loader_configuration != NULL)
                                        {
                                            new_loader->api->FreeConfiguration(loader_configuration);
                                        }
                                        free((void *)new_loader->name);
                                        free(new_loader);
                                    }
                                }
                            }
                            else
                            {
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
        LogError("ModuleLoader_InitializeFromJson() - loaders is NULL");
        result = MODULE_LOADER_ERROR;
    }
    else
    {
        if (json_value_get_type(loaders) != JSONArray)
        {
            LogError("ModuleLoader_InitializeFromJson() - loaders is not a JSON array");
            result = MODULE_LOADER_ERROR;
        }
        else
        {
            JSON_Array* loaders_array = json_value_get_array(loaders);
            if (loaders_array == NULL)
            {
                LogError("ModuleLoader_InitializeFromJson() - json_value_get_array failed");
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
                        LogError("ModuleLoader_InitializeFromJson() - json_array_get_value failed for loader %zu", i);
                        break;
                    }

                    if (add_loader_from_json(loader, i) != MODULE_LOADER_SUCCESS)
                    {
                        LogError("ModuleLoader_InitializeFromJson() - add_loader_from_json failed for loader %zu", i);
                        break;
                    }
                }

                // if the loop terminated early then something went wrong
                result = (i < length) ? MODULE_LOADER_ERROR : MODULE_LOADER_SUCCESS;
            }
        }
    }

    return result;
}