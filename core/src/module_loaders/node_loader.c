// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "azure_c_shared_utility/gballoc.h"
#include <string.h>

#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/strings.h"
#include "parson.h"

#include "module.h"
#include "module_access.h"
#include "dynamic_library.h"
#include "module_loader.h"
#include "module_loaders/node_loader.h"
#include "nodejs.h"

typedef struct NODE_MODULE_HANDLE_DATA_TAG
{
    DYNAMIC_LIBRARY_HANDLE    binding_module;
    const MODULE_API*         api;
}NODE_MODULE_HANDLE_DATA;

static DYNAMIC_LIBRARY_HANDLE NodeModuleLoader_LoadBindingModule(const MODULE_LOADER* loader)
{
    // find the binding path from the loader configuration; if there isn't one, use
    // a default path
    const char* binding_path = NODE_BINDING_MODULE_NAME;
    if (loader->configuration != NULL && loader->configuration->binding_path != NULL)
    {
        binding_path = STRING_c_str(loader->configuration->binding_path);
    }

    return DynamicLibrary_LoadLibrary(binding_path);
}

static MODULE_LIBRARY_HANDLE NodeModuleLoader_Load(const MODULE_LOADER* loader, const void* entrypoint)
{
    NODE_MODULE_HANDLE_DATA* result;

    // loader cannot be null
    if (loader == NULL)
    {
        result = NULL;
        LogError(
            "NodeModuleLoader_Load() - invalid inputs - loader = %p",
            loader
        );
    }
    else
    {
        if (loader->type != NODEJS)
        {
            result = NULL;
            LogError("NodeModuleLoader_Load() - loader->type is not NODEJS");
        }
        else
        {
            result = (NODE_MODULE_HANDLE_DATA*)malloc(sizeof(NODE_MODULE_HANDLE_DATA));
            if (result == NULL)
            {
                LogError("NodeModuleLoader_Load() - malloc returned NULL");
            }
            else
            {
                result->binding_module = NodeModuleLoader_LoadBindingModule(loader);
                if (result->binding_module == NULL)
                {
                    LogError("NodeModuleLoader_Load() - NodeModuleLoader_LoadBindingModule returned NULL");
                    free(result);
                    result = NULL;
                }
                else
                {
                    pfModule_GetApi pfnGetAPI = (pfModule_GetApi)DynamicLibrary_FindSymbol(result->binding_module, MODULE_GETAPI_NAME);
                    if (pfnGetAPI == NULL)
                    {
                        DynamicLibrary_UnloadLibrary(result->binding_module);
                        free(result);
                        result = NULL;
                        LogError("NodeModuleLoader_Load() - DynamicLibrary_FindSymbol() returned NULL");
                    }
                    else
                    {
                        result->api = pfnGetAPI(Module_ApiGatewayVersion);

                        /* if any of the required functions is NULL then we have a misbehaving module */
                        if (result->api == NULL ||
                            result->api->version > Module_ApiGatewayVersion ||
                            MODULE_CREATE(result->api) == NULL ||
                            MODULE_DESTROY(result->api) == NULL ||
                            MODULE_RECEIVE(result->api) == NULL)
                        {
                            DynamicLibrary_UnloadLibrary(result->binding_module);
                            free(result);
                            result = NULL;
                            LogError("NodeModuleLoader_Load() - pfnGetapi() returned NULL");
                        }
                    }
                }
            }
        }
    }

    return result;
}

static const MODULE_API* NodeModuleLoader_GetModuleApi(MODULE_LIBRARY_HANDLE moduleLibraryHandle)
{
    const MODULE_API* result;

    if (moduleLibraryHandle == NULL)
    {
        result = NULL;
        LogError("NodeModuleLoader_GetModuleApi() - moduleLibraryHandle is NULL");
    }
    else
    {
        NODE_MODULE_HANDLE_DATA* loader_data = moduleLibraryHandle;
        result = loader_data->api;
    }

    return result;
}


static void NodeModuleLoader_Unload(MODULE_LIBRARY_HANDLE moduleLibraryHandle)
{
    if (moduleLibraryHandle != NULL)
    {
        NODE_MODULE_HANDLE_DATA* loader_data = moduleLibraryHandle;
        DynamicLibrary_UnloadLibrary(loader_data->binding_module);
        free(loader_data);
    }
    else
    {
        LogError("NodeModuleLoader_Unload() - moduleLibraryHandle is NULL");
    }
}

static void* NodeModuleLoader_ParseEntrypointFromJson(const JSON_Value* json)
{
    // The input is a JSON object that looks like this:
    //  "entrypoint": {
    //      "main.path": "path/to/module"
    //  }
    NODE_LOADER_ENTRYPOINT* config;
    if (json == NULL)
    {
        LogError("NodeModuleLoader_ParseEntrypointFromJson() - json is NULL");
        config = NULL;
    }
    else
    {
        // "json" must be an "object" type
        if (json_value_get_type(json) != JSONObject)
        {
            LogError("NodeModuleLoader_ParseEntrypointFromJson() - 'json' is not an object value");
            config = NULL;
        }
        else
        {
            JSON_Object* entrypoint = json_value_get_object(json);
            if (entrypoint == NULL)
            {
                LogError("NodeModuleLoader_ParseEntrypointFromJson() - json_value_get_object failed");
                config = NULL;
            }
            else
            {
                const char* mainPath = json_object_get_string(entrypoint, "main.path");
                if (mainPath == NULL)
                {
                    LogError("NodeModuleLoader_ParseEntrypointFromJson() - json_object_get_string for 'main.path' returned NULL");
                    config = NULL;
                }
                else
                {
                    config = (NODE_LOADER_ENTRYPOINT*)malloc(sizeof(NODE_LOADER_ENTRYPOINT));
                    if (config != NULL)
                    {
                        config->mainPath = STRING_construct(mainPath);
                        if (config->mainPath == NULL)
                        {
                            LogError("NodeModuleLoader_ParseEntrypointFromJson() - STRING_construct failed");
                            free(config);
                            config = NULL;
                        }
                        else
                        {
                            /**
                             * Everything's good.
                             */
                        }
                    }
                    else
                    {
                        LogError("NodeModuleLoader_ParseEntrypointFromJson() - malloc failed");
                    }
                }
            }
        }
    }

    return (void*)config;
}

static void NodeModuleLoader_FreeEntrypoint(void* entrypoint)
{
    if (entrypoint != NULL)
    {
        NODE_LOADER_ENTRYPOINT* ep = (NODE_LOADER_ENTRYPOINT*)entrypoint;
        STRING_delete(ep->mainPath);
        free(ep);
    }
    else
    {
        LogError("NodeModuleLoader_FreeEntrypoint - entrypoint is NULL");
    }
}

static MODULE_LOADER_BASE_CONFIGURATION* NodeModuleLoader_ParseConfigurationFromJson(const JSON_Value* json)
{
    MODULE_LOADER_BASE_CONFIGURATION* result = (MODULE_LOADER_BASE_CONFIGURATION*)malloc(sizeof(MODULE_LOADER_BASE_CONFIGURATION));
    if (result != NULL)
    {
        if (ModuleLoader_ParseBaseConfigurationFromJson(result, json) != MODULE_LOADER_SUCCESS)
        {
            LogError("NodeModuleLoader_ParseConfigurationFromJson() - ModuleLoader_ParseBaseConfigurationFromJson failed");
            free(result);
            result = NULL;
        }
        else
        {
            /**
             * Everything's good.
             */
        }
    }
    else
    {
        LogError("NodeModuleLoader_ParseConfigurationFromJson() - malloc failed");
    }

    return result;
}

static void NodeModuleLoader_FreeConfiguration(MODULE_LOADER_BASE_CONFIGURATION* configuration)
{
    if (configuration != NULL)
    {
        ModuleLoader_FreeBaseConfiguration(configuration);
        free(configuration);
    }
    else
    {
        LogError("NodeModuleLoader_FreeConfiguration() - configuration is NULL");
    }
}

void* NodeModuleLoader_BuildModuleConfiguration(
    const MODULE_LOADER* loader,
    const void* entrypoint,
    const void* module_configuration
)
{
    NODEJS_MODULE_CONFIG* result;
    if (entrypoint == NULL)
    {
        LogError("NodeModuleLoader_BuildModuleConfiguration() - entrypoint is NULL.");
        result = NULL;
    }
    else
    {
        NODE_LOADER_ENTRYPOINT* node_entrypoint = (NODE_LOADER_ENTRYPOINT*)entrypoint;
        if (node_entrypoint->mainPath == NULL)
        {
            LogError("NodeModuleLoader_BuildModuleConfiguration() - entrypoint mainPath is NULL.");
            result = NULL;
        }
        else
        {
            result = (NODEJS_MODULE_CONFIG*)malloc(sizeof(NODEJS_MODULE_CONFIG));
            if (result == NULL)
            {
                LogError("NodeModuleLoader_BuildModuleConfiguration() - malloc failed.");
            }
            else
            {
                result->main_path = STRING_clone(node_entrypoint->mainPath);
                if (result->main_path == NULL)
                {
                    LogError("NodeModuleLoader_BuildModuleConfiguration() - STRING_clone for mainPath failed.");
                    free(result);
                    result = NULL;
                }
                else
                {
                    // module_configuration is a STRING_HANDLE - see NODEJS_ParseConfigurationFromJson in nodejs.cpp
                    // PERF NOTE: We really need ref-counted strings to avoid redundant string clones.
                    result->configuration_json = (module_configuration == NULL) ? NULL :
                        STRING_clone((STRING_HANDLE)module_configuration);

                    if (module_configuration != NULL && result->configuration_json == NULL)
                    {
                        LogError("NodeModuleLoader_BuildModuleConfiguration() - STRING_clone for module configuration failed.");
                        STRING_delete(result->main_path);
                        free(result);
                        result = NULL;
                    }
                    else
                    {
                        /**
                         * Everything's good.
                         */
                    }
                }
            }
        }
    }

    return result;
}

void NodeModuleLoader_FreeModuleConfiguration(const void* module_configuration)
{
    if (module_configuration == NULL)
    {
        LogError("module_configuration is NULL");
    }
    else
    {
        NODEJS_MODULE_CONFIG* config = (NODEJS_MODULE_CONFIG*)module_configuration;
        STRING_delete(config->main_path);
        STRING_delete(config->configuration_json);
        free(config);
    }
}

MODULE_LOADER_API Node_Module_Loader_API =
{
    .Load = NodeModuleLoader_Load,
    .Unload = NodeModuleLoader_Unload,
    .GetApi = NodeModuleLoader_GetModuleApi,

    .ParseEntrypointFromJson = NodeModuleLoader_ParseEntrypointFromJson,
    .FreeEntrypoint = NodeModuleLoader_FreeEntrypoint,

    .ParseConfigurationFromJson = NodeModuleLoader_ParseConfigurationFromJson,
    .FreeConfiguration = NodeModuleLoader_FreeConfiguration,

    .BuildModuleConfiguration = NodeModuleLoader_BuildModuleConfiguration,
    .FreeModuleConfiguration = NodeModuleLoader_FreeModuleConfiguration
};

MODULE_LOADER Node_Module_Loader =
{
    NODEJS,                 // the loader type
    NODE_LOADER_NAME,       // the name of this loader
    NULL,                   // default loader configuration is NULL unless user overrides
    &Node_Module_Loader_API // the module loader function pointer table
};

const MODULE_LOADER* NodeLoader_Get(void)
{
    return &Node_Module_Loader;
}
