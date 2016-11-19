// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "azure_c_shared_utility/gballoc.h"
#include <string.h>

#include "azure_c_shared_utility/xlogging.h"
#include "parson.h"

#include "module.h"
#include "module_access.h"
#include "module_loader.h"
#include "module_loaders/dotnet_loader.h"
#include "dotnet.h"
#include "dynamic_library.h"
#include "azure_c_shared_utility/crt_abstractions.h"

typedef struct DOTNET_MODULE_HANDLE_DATA_TAG
{
    DYNAMIC_LIBRARY_HANDLE  binding_module;
    const MODULE_API* api;
}DOTNET_MODULE_HANDLE_DATA;


static DYNAMIC_LIBRARY_HANDLE DotnetModuleLoader_LoadBindingModule(const MODULE_LOADER* loader)
{
    // find the binding path from the loader configuration; if there isn't one, use
    // a default path
    const char* binding_path = DOTNET_BINDING_MODULE_NAME;
    if (loader->configuration != NULL && loader->configuration->binding_path != NULL)
    {
        binding_path = STRING_c_str(loader->configuration->binding_path);
    }

    return DynamicLibrary_LoadLibrary(binding_path);
}


static MODULE_LIBRARY_HANDLE DotnetModuleLoader_Load(const MODULE_LOADER* loader, const void* entrypoint)
{
    DOTNET_MODULE_HANDLE_DATA* result;

    // Codes_SRS_DOTNET_MODULE_LOADER_04_001: [ DotnetModuleLoader_Load shall return NULL if loader is NULL. ]
    // Codes_SRS_DOTNET_MODULE_LOADER_04_030: [ DotnetModuleLoader_Load shall return NULL if entrypoint is NULL. ]
    if (loader == NULL || entrypoint == NULL)
    {
        result = NULL;
        LogError(
            "invalid input - loader = %p, entrypoint = %p",
            loader, entrypoint
        );
    }
    else
    {
        if (loader->type != DOTNET)
        {
            // Codes_SRS_DOTNET_MODULE_LOADER_04_002: [ DotnetModuleLoader_Load shall return NULL if loader->type is not DOTNET. ]
            result = NULL;
            LogError("loader->type is not DOTNET");
        }
        else
        {
            DOTNET_LOADER_ENTRYPOINT* dotnet_loader_entrypoint = (DOTNET_LOADER_ENTRYPOINT*)entrypoint;
            if (dotnet_loader_entrypoint->dotnetModuleEntryClass == NULL)
            {
                // Codes_SRS_DOTNET_MODULE_LOADER_04_031: [ DotnetModuleLoader_Load shall return NULL if entrypoint->dotnetModuleEntryClass is NULL. ]
                result = NULL;
                LogError("dotnetModuleEntryClass is NULL");
            }
            else if (dotnet_loader_entrypoint->dotnetModulePath == NULL)
            {
                // Codes_SRS_DOTNET_MODULE_LOADER_04_032: [ DotnetModuleLoader_Load shall return NULL if entrypoint->dotnetModulePath is NULL. ]
                result = NULL;
                LogError("dotnetModulePath is NULL");
            }
            else
            {
                result = (DOTNET_MODULE_HANDLE_DATA*)malloc(sizeof(DOTNET_MODULE_HANDLE_DATA));
                if (result == NULL)
                {
                    // Codes_SRS_DOTNET_MODULE_LOADER_04_003: [ DotnetModuleLoader_Load shall return NULL if an underlying platform call fails. ]
                    LogError("malloc(sizeof(DYNAMIC_MODULE_HANDLE_DATA)) failed");
                }
                else
                {
                    /* load the DLL */
                    //Codes_SRS_DOTNET_MODULE_LOADER_04_033: [ DotnetModuleLoader_Load shall use the the binding module path given in loader->configuration->binding_path if loader->configuration is not NULL. Otherwise it shall use the default binding path name. ]
                    result->binding_module = DotnetModuleLoader_LoadBindingModule(loader);
                    if (result->binding_module == NULL)
                    {
                        // Codes_SRS_DOTNET_MODULE_LOADER_04_003: [ DotnetModuleLoader_Load shall return NULL if an underlying platform call fails. ]
                        free(result);
                        result = NULL;
                        LogError("DotnetModuleLoader_LoadBindingModule returned NULL");
                    }
                    else
                    {
                        //Codes_SRS_DOTNET_MODULE_LOADER_04_034: [ DotnetModuleLoader_Load shall verify that the library has an exported function called Module_GetApi ]
                        pfModule_GetApi pfnGetAPI = (pfModule_GetApi)DynamicLibrary_FindSymbol(result->binding_module, MODULE_GETAPI_NAME);
                        if (pfnGetAPI == NULL)
                        {
                            // Codes_SRS_DOTNET_MODULE_LOADER_04_003: [ DotnetModuleLoader_Load shall return NULL if an underlying platform call fails. ]
                            DynamicLibrary_UnloadLibrary(result->binding_module);
                            free(result);
                            result = NULL;
                            LogError("DynamicLibrary_FindSymbol() returned NULL");
                        }
                        else
                        {
                            result->api = pfnGetAPI(Module_ApiGatewayVersion);

                            /* if any of the required functions is NULL then we have a misbehaving module */
                            //Codes_SRS_DOTNET_MODULE_LOADER_04_035: [ DotnetModuleLoader_Load shall verify if api version is lower or equal than MODULE_API_VERSION_1 and if MODULE_CREATE, MODULE_DESTROY and MODULE_RECEIVE are implemented, otherwise return NULL ]
                            if (result->api == NULL ||
                                result->api->version > Module_ApiGatewayVersion ||
                                MODULE_CREATE(result->api) == NULL ||
                                MODULE_DESTROY(result->api) == NULL ||
                                MODULE_RECEIVE(result->api) == NULL)
                            {
                                DynamicLibrary_UnloadLibrary(result->binding_module);
                                free(result);
                                result = NULL;
                                LogError("pfnGetapi() returned NULL");
                            }
                        }
                    }
                }
            }
        }
    }

    //Codes_SRS_DOTNET_MODULE_LOADER_04_005: [ DotnetModuleLoader_Load shall return a non-NULL pointer of type MODULE_LIBRARY_HANDLE when successful. ]
    return result;
}

static const MODULE_API* DotnetModuleLoader_GetModuleApi(const MODULE_LOADER* loader, MODULE_LIBRARY_HANDLE moduleLibraryHandle)
{
    (void)loader;

    const MODULE_API* result;

    if (moduleLibraryHandle == NULL)
    {
        //Codes_SRS_DOTNET_MODULE_LOADER_04_006: [ DotnetModuleLoader_GetModuleApi shall return NULL if moduleLibraryHandle is NULL. ]
        result = NULL;
        LogError("moduleLibraryHandle is NULL");
    }
    else
    {
        //Codes_SRS_DOTNET_MODULE_LOADER_04_007: [ DotnetModuleLoader_GetModuleApi shall return a non-NULL MODULE_API pointer when successful. ]
        DOTNET_MODULE_HANDLE_DATA* loader_data = (DOTNET_MODULE_HANDLE_DATA*)moduleLibraryHandle;
        result = loader_data->api;
    }

    return result;
}



static void DotnetModuleLoader_Unload(const MODULE_LOADER* loader, MODULE_LIBRARY_HANDLE moduleLibraryHandle)
{
    (void)loader;

    if (moduleLibraryHandle != NULL)
    {
        DOTNET_MODULE_HANDLE_DATA* loader_data = moduleLibraryHandle;
        //Codes_SRS_DOTNET_MODULE_LOADER_04_009: [ DotnetModuleLoader_Unload shall unload the binding module from memory by calling DynamicLibrary_UnloadLibrary. ]
        DynamicLibrary_UnloadLibrary(loader_data->binding_module);
        //Codes_SRS_DOTNET_MODULE_LOADER_04_010: [ DotnetModuleLoader_Unload shall free resources allocated when loading the binding module. ]
        free(loader_data);
    }
    else
    {
        //Codes_SRS_DOTNET_MODULE_LOADER_04_008: [ DotnetModuleLoader_Unload shall do nothing if moduleLibraryHandle is NULL. ]
        LogError("moduleLibraryHandle is NULL");
    }
}

static void* DotnetModuleLoader_ParseEntrypointFromJson(const MODULE_LOADER* loader, const JSON_Value* json)
{
    (void)loader;


    // The input is a JSON object that looks like this:
    // "entrypoint": {
    //     "assembly.name": "SensorModule",
    //     "entry.type" : "SensorModule.DotNetSensorModule"
    // }
    DOTNET_LOADER_ENTRYPOINT* config;
    if (json == NULL)
    {
        //Codes_SRS_DOTNET_MODULE_LOADER_04_011: [ DotnetModuleLoader_ParseEntrypointFromJson shall return NULL if json is NULL. ]
        LogError("json is NULL");
        config = NULL;
    }
    else
    {
        // "json" must be an "object" type
        if (json_value_get_type(json) != JSONObject)
        {
            //Codes_SRS_DOTNET_MODULE_LOADER_04_012: [ DotnetModuleLoader_ParseEntrypointFromJson shall return NULL if the root json entity is not an object. ]
            LogError("'json' is not an object value");
            config = NULL;
        }
        else
        {
            JSON_Object* entrypoint = json_value_get_object(json);
            if (entrypoint == NULL)
            {
                //Codes_SRS_DOTNET_MODULE_LOADER_04_013: [ DotnetModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
                LogError("json_value_get_object failed");
                config = NULL;
            }
            else
            {
                //Codes_SRS_DOTNET_MODULE_LOADER_04_014: [ DotnetModuleLoader_ParseEntrypointFromJson shall retrieve the assembly_name by reading the value of the attribute assembly.name. ]
                const char* dotnetModulePath = json_object_get_string(entrypoint, "assembly.name");
                if (dotnetModulePath == NULL)
                {
                    //Codes_SRS_DOTNET_MODULE_LOADER_04_013: [ DotnetModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
                    LogError("json_object_get_string for 'assembly.name' returned NULL");
                    config = NULL;
                }
                else
                {
                    config = (DOTNET_LOADER_ENTRYPOINT*)malloc(sizeof(DOTNET_LOADER_ENTRYPOINT));
                    if (config != NULL)
                    {
                        config->dotnetModulePath = STRING_construct(dotnetModulePath);
                        if (config->dotnetModulePath == NULL)
                        {
                            //Codes_SRS_DOTNET_MODULE_LOADER_04_013: [ DotnetModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
                            LogError("STRING_construct failed");
                            free(config);
                            config = NULL;
                        }
                        else
                        {
                            //Codes_SRS_DOTNET_MODULE_LOADER_04_036: [ DotnetModuleLoader_ParseEntrypointFromJson shall retrieve the entry_type by reading the value of the attribute entry.type. ]
                            const char* dotnetModuleEntryClass = json_object_get_string(entrypoint, "entry.type");
                            if (dotnetModuleEntryClass == NULL)
                            {
                                //Codes_SRS_DOTNET_MODULE_LOADER_04_013: [ DotnetModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
                                LogError("json_object_get_string for 'entry.type' returned NULL");
                                STRING_delete(config->dotnetModulePath);
                                free(config);
                                config = NULL;

                            }
                            else
                            {
                                config->dotnetModuleEntryClass = STRING_construct(dotnetModuleEntryClass);
                                if (config->dotnetModuleEntryClass == NULL)
                                {
                                    //Codes_SRS_DOTNET_MODULE_LOADER_04_013: [ DotnetModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
                                    LogError("STRING_construct failed");
                                    STRING_delete(config->dotnetModulePath);
                                    free(config);
                                    config = NULL;
                                }
                                else
                                {
                                    //Codes_SRS_DOTNET_MODULE_LOADER_04_015: [ DotnetModuleLoader_ParseEntrypointFromJson shall return a non-NULL pointer to the parsed representation of the entrypoint when successful. ]
                                    /**
                                    * Everything's good.
                                    */
                                }
                            }
                        }
                    }
                    else
                    {
                        //Codes_SRS_DOTNET_MODULE_LOADER_04_013: [ DotnetModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
                        LogError("malloc failed");
                    }
                }
            }
        }
    }

    return (void*)config;
}

static void DotnetModuleLoader_FreeEntrypoint(const MODULE_LOADER* loader, void* entrypoint)
{
    (void)loader;

    if (entrypoint != NULL)
    {
        //Codes_SRS_DOTNET_MODULE_LOADER_04_017: [ DotnetModuleLoader_FreeEntrypoint shall free resources allocated during DotnetModuleLoader_ParseEntrypointFromJson. ]
        DOTNET_LOADER_ENTRYPOINT* ep = (DOTNET_LOADER_ENTRYPOINT*)entrypoint;
        STRING_delete(ep->dotnetModuleEntryClass);
        STRING_delete(ep->dotnetModulePath);

        free(ep);
    }
    else
    {
        //Codes_SRS_DOTNET_MODULE_LOADER_04_016: [ DotnetModuleLoader_FreeEntrypoint shall do nothing if entrypoint is NULL. ]
        LogError("entrypoint is NULL");
    }
}

static MODULE_LOADER_BASE_CONFIGURATION* DotnetModuleLoader_ParseConfigurationFromJson(const MODULE_LOADER* loader, const JSON_Value* json)
{
    (void)loader;

    MODULE_LOADER_BASE_CONFIGURATION* result = (MODULE_LOADER_BASE_CONFIGURATION*)malloc(sizeof(MODULE_LOADER_BASE_CONFIGURATION));
    if (result != NULL)
    {
        //Codes_SRS_DOTNET_MODULE_LOADER_04_019: [ DotnetModuleLoader_ParseConfigurationFromJson shall call ModuleLoader_ParseBaseConfigurationFromJson to parse the loader configuration and return the result. ]
        if (ModuleLoader_ParseBaseConfigurationFromJson(result, json) != MODULE_LOADER_SUCCESS)
        {
            LogError("ModuleLoader_ParseBaseConfigurationFromJson failed");
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
        //Codes_SRS_DOTNET_MODULE_LOADER_04_018: [ DotnetModuleLoader_ParseConfigurationFromJson shall return NULL if an underlying platform call fails. ]
        LogError(" malloc failed");
    }

    return result;
}

static void DotnetModuleLoader_FreeConfiguration(const MODULE_LOADER* loader, MODULE_LOADER_BASE_CONFIGURATION* configuration)
{
    (void)loader;

    if (configuration != NULL)
    {
        //Codes_SRS_DOTNET_MODULE_LOADER_04_021: [ DotnetModuleLoader_FreeConfiguration shall call ModuleLoader_FreeBaseConfiguration to free resources allocated by ModuleLoader_ParseBaseConfigurationFromJson. ]
        ModuleLoader_FreeBaseConfiguration(configuration);
        //Codes_SRS_DOTNET_MODULE_LOADER_04_022: [ DotnetModuleLoader_FreeConfiguration shall free resources allocated by DotnetModuleLoader_ParseConfigurationFromJson. ]
        free(configuration);
    }
    else
    {
        //Codes_SRS_DOTNET_MODULE_LOADER_04_020: [ DotnetModuleLoader_FreeConfiguration shall do nothing if configuration is NULL. ]
        LogError("configuration is NULL");
    }
}

void* DotnetModuleLoader_BuildModuleConfiguration(
    const MODULE_LOADER* loader,
    const void* entrypoint,
    const void* module_configuration
)
{
    DOTNET_HOST_CONFIG* result;
    if (entrypoint == NULL)
    {
        //Codes_SRS_DOTNET_MODULE_LOADER_04_023: [ DotnetModuleLoader_BuildModuleConfiguration shall return NULL if entrypoint is NULL. ]
        LogError("entrypoint is NULL.");
        result = NULL;
    }
    else
    {
        DOTNET_LOADER_ENTRYPOINT* dotnet_entrypoint = (DOTNET_LOADER_ENTRYPOINT*)entrypoint;
        if (dotnet_entrypoint->dotnetModulePath == NULL)
        {
            //Codes_SRS_DOTNET_MODULE_LOADER_04_024: [ DotnetModuleLoader_BuildModuleConfiguration shall return NULL if entrypoint->dotnetModulePath is NULL. ]
            LogError("entrypoint dotnetModulePath is NULL.");
            result = NULL;
        }
        else if (dotnet_entrypoint->dotnetModuleEntryClass == NULL)
        {
            //Codes_SRS_DOTNET_MODULE_LOADER_04_037: [ DotnetModuleLoader_BuildModuleConfiguration shall return NULL if entrypoint->dotnetModuleEntryClass is NULL. ]
            LogError("entrypoint dotnetModuleEntryClass is NULL.");
            result = NULL;
        }
        else
        {
            //Codes_SRS_DOTNET_MODULE_LOADER_04_026: [ DotnetModuleLoader_BuildModuleConfiguration shall build a DOTNET_HOST_CONFIG object by copying information from entrypoint and module_configuration and return a non-NULL pointer. ]
            result = (DOTNET_HOST_CONFIG*)malloc(sizeof(DOTNET_HOST_CONFIG));
            if (result == NULL)
            {
                //Codes_SRS_DOTNET_MODULE_LOADER_04_025: [ DotnetModuleLoader_BuildModuleConfiguration shall return NULL if an underlying platform call fails. ]
                LogError("malloc failed.");
            }
            else
            {
                if (mallocAndStrcpy_s(&((char*)result->entry_type), (char*)STRING_c_str(dotnet_entrypoint->dotnetModuleEntryClass)) != 0)
                {
                    //Codes_SRS_DOTNET_MODULE_LOADER_04_025: [ DotnetModuleLoader_BuildModuleConfiguration shall return NULL if an underlying platform call fails. ]
                    LogError("Failed to malloc and Copy dotnetModuleEntryClass String.");
                    free(result);
                    result = NULL;
                }
                else
                {
                    if (mallocAndStrcpy_s(&((char*)result->assembly_name), (char*)STRING_c_str(dotnet_entrypoint->dotnetModulePath)) != 0)
                    {
                        //Codes_SRS_DOTNET_MODULE_LOADER_04_025: [ DotnetModuleLoader_BuildModuleConfiguration shall return NULL if an underlying platform call fails. ]
                        LogError("Failed to malloc and Copy assembly_name failed.");
                        free((char*)result->entry_type);
                        free(result);
                        result = NULL;
                    }
                    else
                    {
                        if (module_configuration != NULL && mallocAndStrcpy_s(&((char*)result->module_args), (char*)module_configuration) != 0)
                        {
                            //Codes_SRS_DOTNET_MODULE_LOADER_04_025: [ DotnetModuleLoader_BuildModuleConfiguration shall return NULL if an underlying platform call fails. ]
                            LogError("Malloc and Copy for module_configuration failed.");
                            free((char*)result->assembly_name);
                            free((char*)result->entry_type);
                            free(result);
                            result = NULL;
                        }
                        else if(module_configuration == NULL)
                        {
                            result->module_args = NULL;
                        }
                        else
                        {
                            //Codes_SRS_DOTNET_MODULE_LOADER_04_026: [ DotnetModuleLoader_BuildModuleConfiguration shall build a DOTNET_HOST_CONFIG object by copying information from entrypoint and module_configuration and return a non-NULL pointer. ]
                            /**
                            * Everything's good.
                            */
                        }
                    }
                }
            }
        }
    }

    return result;
}

void DotnetModuleLoader_FreeModuleConfiguration(const MODULE_LOADER* loader, const void* module_configuration)
{
    (void)loader;

    if (module_configuration != NULL)
    {
        DOTNET_HOST_CONFIG* configuration = (DOTNET_HOST_CONFIG*)module_configuration;
        //Codes_SRS_DOTNET_MODULE_LOADER_04_028: [ DotnetModuleLoader_FreeModuleConfiguration shall free the DOTNET_HOST_CONFIG object. ]
        free((char*)configuration->module_args);
        free((char*)configuration->entry_type);
        free((char*)configuration->assembly_name);
        free((void*)module_configuration);
    }
    else
    {
        //Codes_SRS_DOTNET_MODULE_LOADER_04_027: [ DotnetModuleLoader_FreeModuleConfiguration shall do nothing if module_configuration is NULL. ]
        LogError("module_configuration is NULL");
    }
}

/* Codes_SRS*/
MODULE_LOADER_API Dotnet_Module_Loader_API =
{
    .Load = DotnetModuleLoader_Load,
    .Unload = DotnetModuleLoader_Unload,
    .GetApi = DotnetModuleLoader_GetModuleApi,

    .ParseEntrypointFromJson = DotnetModuleLoader_ParseEntrypointFromJson,
    .FreeEntrypoint = DotnetModuleLoader_FreeEntrypoint,

    .ParseConfigurationFromJson = DotnetModuleLoader_ParseConfigurationFromJson,
    .FreeConfiguration = DotnetModuleLoader_FreeConfiguration,

    .BuildModuleConfiguration = DotnetModuleLoader_BuildModuleConfiguration,
    .FreeModuleConfiguration = DotnetModuleLoader_FreeModuleConfiguration
};

MODULE_LOADER Dotnet_Module_Loader =
{
    DOTNET, //Codes_SRS_DOTNET_MODULE_LOADER_04_038: [ MODULE_LOADER::type shall be DOTNET. ]
    DOTNET_LOADER_NAME, //Codes_SRS_DOTNET_MODULE_LOADER_04_039: [ MODULE_LOADER::name shall be the string 'dotnet'. ]
    NULL,
    &Dotnet_Module_Loader_API
};

const MODULE_LOADER* DotnetLoader_Get(void)
{
    //Codes_SRS_DOTNET_MODULE_LOADER_04_029: [ DotnetLoader_Get shall return a non-NULL pointer to a MODULE_LOADER struct. ]
    return &Dotnet_Module_Loader;
}
