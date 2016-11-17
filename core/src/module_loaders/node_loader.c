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
        //Codes_SRS_NODE_MODULE_LOADER_13_032: [ NodeModuleLoader_Load shall use the the binding module path given in loader->configuration->binding_path if loader->configuration is not NULL. ]
        binding_path = STRING_c_str(loader->configuration->binding_path);
    }

    //Codes_SRS_NODE_MODULE_LOADER_13_004 : [NodeModuleLoader_Load shall load the binding module library into memory by calling DynamicLibrary_LoadLibrary.]
    return DynamicLibrary_LoadLibrary(binding_path);
}

static MODULE_LIBRARY_HANDLE NodeModuleLoader_Load(const MODULE_LOADER* loader, const void* entrypoint)
{
    NODE_MODULE_HANDLE_DATA* result;

    // loader cannot be null
    if (loader == NULL)
    {
        //Codes_SRS_NODE_MODULE_LOADER_13_001 : [NodeModuleLoader_Load shall return NULL if loader is NULL.]
        result = NULL;
        LogError("invalid inputs - loader = %p", loader);
    }
    else
    {
        if (loader->type != NODEJS)
        {
            //Codes_SRS_NODE_MODULE_LOADER_13_002 : [NodeModuleLoader_Load shall return NULL if loader->type is not NODEJS.]
            result = NULL;
            LogError("loader->type is not NODEJS");
        }
        else
        {
            result = (NODE_MODULE_HANDLE_DATA*)malloc(sizeof(NODE_MODULE_HANDLE_DATA));
            if (result == NULL)
            {
                //Codes_SRS_NODE_MODULE_LOADER_13_003 : [NodeModuleLoader_Load shall return NULL if an underlying platform call fails.]
                LogError("malloc returned NULL");
            }
            else
            {
                result->binding_module = NodeModuleLoader_LoadBindingModule(loader);
                if (result->binding_module == NULL)
                {
                    LogError("NodeModuleLoader_LoadBindingModule returned NULL");
                    //Codes_SRS_NODE_MODULE_LOADER_13_003 : [NodeModuleLoader_Load shall return NULL if an underlying platform call fails.]
                    free(result);
                    result = NULL;
                }
                else
                {
                    //Codes_SRS_NODE_MODULE_LOADER_13_033: [ NodeModuleLoader_Load shall call DynamicLibrary_FindSymbol on the binding module handle with the symbol name Module_GetApi to acquire the function that returns the module's API table. ]
                    pfModule_GetApi pfnGetAPI = (pfModule_GetApi)DynamicLibrary_FindSymbol(result->binding_module, MODULE_GETAPI_NAME);
                    if (pfnGetAPI == NULL)
                    {
                        DynamicLibrary_UnloadLibrary(result->binding_module);
                        free(result);
                        //Codes_SRS_NODE_MODULE_LOADER_13_003 : [NodeModuleLoader_Load shall return NULL if an underlying platform call fails.]
                        result = NULL;
                        LogError("DynamicLibrary_FindSymbol() returned NULL");
                    }
                    else
                    {
                        //Codes_SRS_NODE_MODULE_LOADER_13_005 : [NodeModuleLoader_Load shall return a non - NULL pointer of type MODULE_LIBRARY_HANDLE when successful.]
                        result->api = pfnGetAPI(Module_ApiGatewayVersion);

                        //Codes_SRS_NODE_MODULE_LOADER_13_034: [ NodeModuleLoader_Load shall return NULL if the MODULE_API pointer returned by the binding module is NULL. ]
                        //Codes_SRS_NODE_MODULE_LOADER_13_035: [ NodeModuleLoader_Load shall return NULL if MODULE_API::version is greater than Module_ApiGatewayVersion. ]
                        //Codes_SRS_NODE_MODULE_LOADER_13_036: [ NodeModuleLoader_Load shall return NULL if the Module_Create function in MODULE_API is NULL. ]
                        //Codes_SRS_NODE_MODULE_LOADER_13_037: [ NodeModuleLoader_Load shall return NULL if the Module_Receive function in MODULE_API is NULL. ]
                        //Codes_SRS_NODE_MODULE_LOADER_13_038: [ NodeModuleLoader_Load shall return NULL if the Module_Destroy function in MODULE_API is NULL. ]

                        /* if any of the required functions is NULL then we have a misbehaving module */
                        if (result->api == NULL ||
                            result->api->version > Module_ApiGatewayVersion ||
                            MODULE_CREATE(result->api) == NULL ||
                            MODULE_DESTROY(result->api) == NULL ||
                            MODULE_RECEIVE(result->api) == NULL)
                        {
                            LogError(
                                "pfnGetapi() returned an invalid MODULE_API instance. "
                                "result->api = %p, "
                                "result->api->version = %d, "
                                "MODULE_CREATE(result->api) = %p, "
                                "MODULE_DESTROY(result->api) = %p, "
                                "MODULE_RECEIVE(result->api) = %p, ",
                                result->api,
                                result->api != NULL ? result->api->version : 0x0,
                                result->api != NULL ? MODULE_CREATE(result->api) : NULL,
                                result->api != NULL ? MODULE_DESTROY(result->api) : NULL,
                                result->api != NULL ? MODULE_RECEIVE(result->api) : NULL
                            );

                            DynamicLibrary_UnloadLibrary(result->binding_module);
                            free(result);
                            //Codes_SRS_NODE_MODULE_LOADER_13_003 : [NodeModuleLoader_Load shall return NULL if an underlying platform call fails.]
                            result = NULL;
                        }
                    }
                }
            }
        }
    }

    return result;
}

static const MODULE_API* NodeModuleLoader_GetModuleApi(const MODULE_LOADER* loader, MODULE_LIBRARY_HANDLE moduleLibraryHandle)
{
    (void)loader;

    const MODULE_API* result;

    if (moduleLibraryHandle == NULL)
    {
        //Codes_SRS_NODE_MODULE_LOADER_13_006 : [NodeModuleLoader_GetModuleApi shall return NULL if moduleLibraryHandle is NULL.]
        result = NULL;
        LogError("moduleLibraryHandle is NULL");
    }
    else
    {
        NODE_MODULE_HANDLE_DATA* loader_data = moduleLibraryHandle;
        //Codes_SRS_NODE_MODULE_LOADER_13_007 : [NodeModuleLoader_GetModuleApi shall return a non - NULL MODULE_API pointer when successful.]
        result = loader_data->api;
    }

    return result;
}

static void NodeModuleLoader_Unload(const MODULE_LOADER* loader, MODULE_LIBRARY_HANDLE moduleLibraryHandle)
{
    (void)loader;

    if (moduleLibraryHandle != NULL)
    {
        NODE_MODULE_HANDLE_DATA* loader_data = moduleLibraryHandle;
        //Codes_SRS_NODE_MODULE_LOADER_13_009: [ NodeModuleLoader_Unload shall unload the binding module from memory by calling DynamicLibrary_UnloadLibrary. ]
        DynamicLibrary_UnloadLibrary(loader_data->binding_module);

        //Codes_SRS_NODE_MODULE_LOADER_13_010: [ NodeModuleLoader_Unload shall free resources allocated when loading the binding module. ]
        free(loader_data);
    }
    else
    {
        //Codes_SRS_NODE_MODULE_LOADER_13_008: [ NodeModuleLoader_Unload shall do nothing if moduleLibraryHandle is NULL. ]
        LogError("moduleLibraryHandle is NULL");
    }
}

static void* NodeModuleLoader_ParseEntrypointFromJson(const MODULE_LOADER* loader, const JSON_Value* json)
{
    (void)loader;

    // The input is a JSON object that looks like this:
    //  "entrypoint": {
    //      "main.path": "path/to/module"
    //  }
    NODE_LOADER_ENTRYPOINT* config;
    if (json == NULL)
    {
        LogError("json is NULL");
        //Codes_SRS_NODE_MODULE_LOADER_13_011: [ NodeModuleLoader_ParseEntrypointFromJson shall return NULL if json is NULL. ]
        config = NULL;
    }
    else
    {
        // "json" must be an "object" type
        if (json_value_get_type(json) != JSONObject)
        {
            LogError("'json' is not an object value");
            //Codes_SRS_NODE_MODULE_LOADER_13_012: [ NodeModuleLoader_ParseEntrypointFromJson shall return NULL if the root json entity is not an object. ]
            config = NULL;
        }
        else
        {
            JSON_Object* entrypoint = json_value_get_object(json);
            if (entrypoint == NULL)
            {
                LogError("json_value_get_object failed");
                //Codes_SRS_NODE_MODULE_LOADER_13_013: [ NodeModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
                config = NULL;
            }
            else
            {
                //Codes_SRS_NODE_MODULE_LOADER_13_014: [ NodeModuleLoader_ParseEntrypointFromJson shall retrieve the path to the main JS file by reading the value of the attribute main.path. ]
                const char* mainPath = json_object_get_string(entrypoint, "main.path");
                if (mainPath == NULL)
                {
                    LogError("json_object_get_string for 'main.path' returned NULL");
                    //Codes_SRS_NODE_MODULE_LOADER_13_039: [ NodeModuleLoader_ParseEntrypointFromJson shall return NULL if main.path does not exist. ]
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
                            LogError("STRING_construct failed");
                            free(config);
                            //Codes_SRS_NODE_MODULE_LOADER_13_013: [ NodeModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
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
                        //Codes_SRS_NODE_MODULE_LOADER_13_013: [ NodeModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
                        LogError("malloc failed");
                    }
                }
            }
        }
    }

    //Codes_SRS_NODE_MODULE_LOADER_13_015: [ NodeModuleLoader_ParseEntrypointFromJson shall return a non-NULL pointer to the parsed representation of the entrypoint when successful. ]
    return (void*)config;
}

static void NodeModuleLoader_FreeEntrypoint(const MODULE_LOADER* loader, void* entrypoint)
{
    (void)loader;

    if (entrypoint != NULL)
    {
        NODE_LOADER_ENTRYPOINT* ep = (NODE_LOADER_ENTRYPOINT*)entrypoint;
        //Codes_SRS_NODE_MODULE_LOADER_13_017: [NodeModuleLoader_FreeEntrypoint shall free resources allocated during NodeModuleLoader_ParseEntrypointFromJson.]
        STRING_delete(ep->mainPath);
        free(ep);
    }
    else
    {
        //Codes_SRS_NODE_MODULE_LOADER_13_016: [ NodeModuleLoader_FreeEntrypoint shall do nothing if entrypoint is NULL. ]
        LogError("entrypoint is NULL");
    }
}

static MODULE_LOADER_BASE_CONFIGURATION* NodeModuleLoader_ParseConfigurationFromJson(const MODULE_LOADER* loader, const JSON_Value* json)
{
    (void)loader;

    MODULE_LOADER_BASE_CONFIGURATION* result = (MODULE_LOADER_BASE_CONFIGURATION*)malloc(sizeof(MODULE_LOADER_BASE_CONFIGURATION));
    if (result != NULL)
    {
        //Codes_SRS_NODE_MODULE_LOADER_13_019: [ NodeModuleLoader_ParseConfigurationFromJson shall call ModuleLoader_ParseBaseConfigurationFromJson to parse the loader configuration and return the result. ]
        if (ModuleLoader_ParseBaseConfigurationFromJson(result, json) != MODULE_LOADER_SUCCESS)
        {
            LogError("ModuleLoader_ParseBaseConfigurationFromJson failed");
            free(result);
            //Codes_SRS_NODE_MODULE_LOADER_13_018: [ NodeModuleLoader_ParseConfigurationFromJson shall return NULL if an underlying platform call fails. ]
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
        //Codes_SRS_NODE_MODULE_LOADER_13_018: [ NodeModuleLoader_ParseConfigurationFromJson shall return NULL if an underlying platform call fails. ]
        LogError("malloc failed");
    }

    return result;
}

static void NodeModuleLoader_FreeConfiguration(const MODULE_LOADER* loader, MODULE_LOADER_BASE_CONFIGURATION* configuration)
{
    (void)loader;

    if (configuration != NULL)
    {
        //Codes_SRS_NODE_MODULE_LOADER_13_021: [ NodeModuleLoader_FreeConfiguration shall call ModuleLoader_FreeBaseConfiguration to free resources allocated by ModuleLoader_ParseBaseConfigurationFromJson. ]
        ModuleLoader_FreeBaseConfiguration(configuration);

        //Codes_SRS_NODE_MODULE_LOADER_13_022: [ NodeModuleLoader_FreeConfiguration shall free resources allocated by NodeModuleLoader_ParseConfigurationFromJson. ]
        free(configuration);
    }
    else
    {
        //Codes_SRS_NODE_MODULE_LOADER_13_020: [ NodeModuleLoader_FreeConfiguration shall do nothing if configuration is NULL. ]
        LogError("configuration is NULL");
    }
}

static void* NodeModuleLoader_BuildModuleConfiguration(
    const MODULE_LOADER* loader,
    const void* entrypoint,
    const void* module_configuration
)
{
    (void)loader;

    NODEJS_MODULE_CONFIG* result;
    if (entrypoint == NULL)
    {
        LogError("entrypoint is NULL.");
        //Codes_SRS_NODE_MODULE_LOADER_13_023: [ NodeModuleLoader_BuildModuleConfiguration shall return NULL if entrypoint is NULL. ]
        result = NULL;
    }
    else
    {
        NODE_LOADER_ENTRYPOINT* node_entrypoint = (NODE_LOADER_ENTRYPOINT*)entrypoint;
        if (node_entrypoint->mainPath == NULL)
        {
            LogError("entrypoint mainPath is NULL.");
            //Codes_SRS_NODE_MODULE_LOADER_13_024: [ NodeModuleLoader_BuildModuleConfiguration shall return NULL if entrypoint->mainPath is NULL. ]
            result = NULL;
        }
        else
        {
            result = (NODEJS_MODULE_CONFIG*)malloc(sizeof(NODEJS_MODULE_CONFIG));
            if (result == NULL)
            {
                //Codes_SRS_NODE_MODULE_LOADER_13_025: [ NodeModuleLoader_BuildModuleConfiguration shall return NULL if an underlying platform call fails. ]
                LogError("malloc failed.");
            }
            else
            {
                result->main_path = STRING_clone(node_entrypoint->mainPath);
                if (result->main_path == NULL)
                {
                    LogError("STRING_clone for mainPath failed.");
                    free(result);
                    //Codes_SRS_NODE_MODULE_LOADER_13_025: [ NodeModuleLoader_BuildModuleConfiguration shall return NULL if an underlying platform call fails. ]
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
                        LogError("STRING_clone for module configuration failed.");
                        STRING_delete(result->main_path);
                        free(result);
                        //Codes_SRS_NODE_MODULE_LOADER_13_025: [ NodeModuleLoader_BuildModuleConfiguration shall return NULL if an underlying platform call fails. ]
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

    //Codes_SRS_NODE_MODULE_LOADER_13_026: [NodeModuleLoader_BuildModuleConfiguration shall build a NODEJS_MODULE_CONFIG object by copying information from entrypoint and module_configuration and return a non - NULL pointer.]
    return result;
}

static void NodeModuleLoader_FreeModuleConfiguration(const MODULE_LOADER* loader, const void* module_configuration)
{
    (void)loader;

    if (module_configuration == NULL)
    {
        //Codes_SRS_NODE_MODULE_LOADER_13_027: [ NodeModuleLoader_FreeModuleConfiguration shall do nothing if module_configuration is NULL. ]
        LogError("module_configuration is NULL");
    }
    else
    {
        NODEJS_MODULE_CONFIG* config = (NODEJS_MODULE_CONFIG*)module_configuration;
        //Codes_SRS_NODE_MODULE_LOADER_13_028: [ NodeModuleLoader_FreeModuleConfiguration shall free the NODEJS_MODULE_CONFIG object. ]
        STRING_delete(config->main_path);
        STRING_delete(config->configuration_json);
        free(config);
    }
}

static MODULE_LOADER_API Node_Module_Loader_API =
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

static MODULE_LOADER Node_Module_Loader =
{
    //Codes_SRS_NODE_MODULE_LOADER_13_030 : [MODULE_LOADER::type shall be NODEJS.]
    NODEJS,                 // the loader type

    //Codes_SRS_NODE_MODULE_LOADER_13_031 : [MODULE_LOADER::name shall be the string 'node'.]
    NODE_LOADER_NAME,       // the name of this loader

    NULL,                   // default loader configuration is NULL unless user overrides

    &Node_Module_Loader_API // the module loader function pointer table
};

const MODULE_LOADER* NodeLoader_Get(void)
{
    //Codes_SRS_NODE_MODULE_LOADER_13_029: [NodeLoader_Get shall return a non - NULL pointer to a MODULE_LOADER struct.]
    return &Node_Module_Loader;
}
