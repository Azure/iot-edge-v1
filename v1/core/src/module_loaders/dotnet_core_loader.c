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
#include "module_loaders/dotnet_core_loader.h"
#include "dotnetcore.h"
#include "dynamic_library.h"
#include "azure_c_shared_utility/crt_abstractions.h"

#define CONFIGURATION         ((DOTNET_CORE_LOADER_CONFIGURATION*)(loader->configuration))

typedef struct DOTNET_CORE_MODULE_HANDLE_DATA_TAG
{
    DYNAMIC_LIBRARY_HANDLE  binding_module;
    const MODULE_API* api;
}DOTNET_CORE_MODULE_HANDLE_DATA;

static DOTNET_CORE_LOADER_CONFIGURATION* create_default_config();

static int set_default_paths(DOTNET_CORE_CLR_OPTIONS* options);

static DYNAMIC_LIBRARY_HANDLE DotnetCoreModuleLoader_LoadBindingModule(const MODULE_LOADER* loader)
{
    // find the binding path from the loader configuration; if there isn't one, use
    // a default path
    const char* binding_path = DOTNET_CORE_BINDING_MODULE_NAME;
    if (loader != NULL && CONFIGURATION != NULL && CONFIGURATION->base.binding_path != NULL)
    {
        binding_path = STRING_c_str(CONFIGURATION->base.binding_path);
    }

    return DynamicLibrary_LoadLibrary(binding_path);
}


static MODULE_LIBRARY_HANDLE DotnetCoreModuleLoader_Load(const MODULE_LOADER* loader, const void* entrypoint)
{
    DOTNET_CORE_MODULE_HANDLE_DATA* result;
   
    // Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_001: [ DotnetModuleLoader_Load shall return NULL if loader is NULL. ]
    // Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_030: [ DotnetModuleLoader_Load shall return NULL if entrypoint is NULL. ]
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
        if (loader->type != DOTNETCORE)
        {
            // Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_002: [ DotnetModuleLoader_Load shall return NULL if loader->type is not DOTNETCORE. ]
            result = NULL;
            LogError("loader->type is not DOTNETCORE");
        }
        else
        {
            DOTNET_CORE_LOADER_ENTRYPOINT* dotnet_core_loader_entrypoint = (DOTNET_CORE_LOADER_ENTRYPOINT*)entrypoint;
            if (dotnet_core_loader_entrypoint->dotnetCoreModuleEntryClass == NULL)
            {
                // Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_031: [ DotnetModuleLoader_Load shall return NULL if entrypoint->dotnetModuleEntryClass is NULL. ]
                result = NULL;
                LogError("dotnetCoreModuleEntryClass is NULL");
            }
            else if (dotnet_core_loader_entrypoint->dotnetCoreModulePath == NULL)
            {
                // Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_032: [ DotnetModuleLoader_Load shall return NULL if entrypoint->dotnetModulePath is NULL. ]
                result = NULL;
                LogError("dotnetCoreModulePath is NULL");
            }
            else
            {
                result = (DOTNET_CORE_MODULE_HANDLE_DATA*)malloc(sizeof(DOTNET_CORE_MODULE_HANDLE_DATA));
                if (result == NULL)
                {
                    // Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_003: [ DotnetModuleLoader_Load shall return NULL if an underlying platform call fails. ]
                    LogError("malloc(sizeof(DYNAMIC_MODULE_HANDLE_DATA)) failed");
                }
                else
                {
                    /* load the DLL */
                    //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_033: [ DotnetModuleLoader_Load shall use the the binding module path given in loader->configuration->binding_path if loader->configuration is not NULL. Otherwise it shall use the default binding path name. ]
                    result->binding_module = DotnetCoreModuleLoader_LoadBindingModule(loader);
                    if (result->binding_module == NULL)
                    {
                        // Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_003: [ DotnetModuleLoader_Load shall return NULL if an underlying platform call fails. ]
                        free(result);
                        result = NULL;
                        LogError("DotnetCoreModuleLoader_LoadBindingModule returned NULL");
                    }
                    else
                    {
                        //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_034: [ DotnetModuleLoader_Load shall verify that the library has an exported function called Module_GetApi ]
                        pfModule_GetApi pfnGetAPI = (pfModule_GetApi)DynamicLibrary_FindSymbol(result->binding_module, MODULE_GETAPI_NAME);
                        if (pfnGetAPI == NULL)
                        {
                            // Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_003: [ DotnetModuleLoader_Load shall return NULL if an underlying platform call fails. ]
                            DynamicLibrary_UnloadLibrary(result->binding_module);
                            free(result);
                            result = NULL;
                            LogError("DynamicLibrary_FindSymbol() returned NULL");
                        }
                        else
                        {
                            result->api = pfnGetAPI(Module_ApiGatewayVersion);

                            /* if any of the required functions is NULL then we have a misbehaving module */
                            //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_035: [ DotnetModuleLoader_Load shall verify if api version is lower or equal than MODULE_API_VERSION_1 and if MODULE_CREATE, MODULE_DESTROY and MODULE_RECEIVE are implemented, otherwise return NULL ]
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

    //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_005: [ DotnetModuleLoader_Load shall return a non-NULL pointer of type MODULE_LIBRARY_HANDLE when successful. ]
    return result;
}

static const MODULE_API* DotnetCoreModuleLoader_GetModuleApi(const MODULE_LOADER* loader, MODULE_LIBRARY_HANDLE moduleLibraryHandle)
{
    (void)loader;

    const MODULE_API* result;

    if (moduleLibraryHandle == NULL)
    {
        //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_006: [ DotnetModuleLoader_GetModuleApi shall return NULL if moduleLibraryHandle is NULL. ]
        result = NULL;
        LogError("moduleLibraryHandle is NULL");
    }
    else
    {
        //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_007: [ DotnetModuleLoader_GetModuleApi shall return a non-NULL MODULE_API pointer when successful. ]
        DOTNET_CORE_MODULE_HANDLE_DATA* loader_data = (DOTNET_CORE_MODULE_HANDLE_DATA*)moduleLibraryHandle;
        result = loader_data->api;
    }

    return result;
}



static void DotnetCoreModuleLoader_Unload(const MODULE_LOADER* loader, MODULE_LIBRARY_HANDLE moduleLibraryHandle)
{
    (void)loader;

    if (moduleLibraryHandle != NULL)
    {
        DOTNET_CORE_MODULE_HANDLE_DATA* loader_data = moduleLibraryHandle;
        //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_009: [ DotnetModuleLoader_Unload shall unload the binding module from memory by calling DynamicLibrary_UnloadLibrary. ]
        DynamicLibrary_UnloadLibrary(loader_data->binding_module);
        //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_010: [ DotnetModuleLoader_Unload shall free resources allocated when loading the binding module. ]
        free(loader_data);
    }
    else
    {
        //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_008: [ DotnetModuleLoader_Unload shall do nothing if moduleLibraryHandle is NULL. ]
        LogError("moduleLibraryHandle is NULL");
    }
}

static void* DotnetCoreModuleLoader_ParseEntrypointFromJson(const MODULE_LOADER* loader, const JSON_Value* json)
{
    (void)loader;


    // The input is a JSON object that looks like this:
    // "entrypoint": {
    //     "assembly.name": "SensorModule",
    //     "entry.type" : "SensorModule.DotNetSensorModule"
    // }
    DOTNET_CORE_LOADER_ENTRYPOINT* config;
    if (json == NULL)
    {
        //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_011: [ DotnetModuleLoader_ParseEntrypointFromJson shall return NULL if json is NULL. ]
        LogError("json is NULL");
        config = NULL;
    }
    else
    {
        // "json" must be an "object" type
        if (json_value_get_type(json) != JSONObject)
        {
            //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_012: [ DotnetModuleLoader_ParseEntrypointFromJson shall return NULL if the root json entity is not an object. ]
            LogError("'json' is not an object value");
            config = NULL;
        }
        else
        {
            JSON_Object* entrypoint = json_value_get_object(json);
            if (entrypoint == NULL)
            {
                //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_013: [ DotnetModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
                LogError("json_value_get_object failed");
                config = NULL;
            }
            else
            {
                //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_014: [ DotnetModuleLoader_ParseEntrypointFromJson shall retrieve the assemblyName by reading the value of the attribute assembly.name. ]
                const char* dotnetCoreModulePath = json_object_get_string(entrypoint, "assembly.name");
                if (dotnetCoreModulePath == NULL)
                {
                    //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_013: [ DotnetModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
                    LogError("json_object_get_string for 'assembly.name' returned NULL");
                    config = NULL;
                }
                else
                {
                    config = (DOTNET_CORE_LOADER_ENTRYPOINT*)malloc(sizeof(DOTNET_CORE_LOADER_ENTRYPOINT));
                    if (config != NULL)
                    {
                        config->dotnetCoreModulePath = STRING_construct(dotnetCoreModulePath);
                        if (config->dotnetCoreModulePath == NULL)
                        {
                            //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_013: [ DotnetModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
                            LogError("STRING_construct failed");
                            free(config);
                            config = NULL;
                        }
                        else
                        {
                            //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_036: [ DotnetModuleLoader_ParseEntrypointFromJson shall retrieve the entryType by reading the value of the attribute entry.type. ]
                            const char* dotnetCoreModuleEntryClass = json_object_get_string(entrypoint, "entry.type");
                            if (dotnetCoreModuleEntryClass == NULL)
                            {
                                //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_013: [ DotnetModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
                                LogError("json_object_get_string for 'entry.type' returned NULL");
                                STRING_delete(config->dotnetCoreModulePath);
                                free(config);
                                config = NULL;

                            }
                            else
                            {
                                config->dotnetCoreModuleEntryClass = STRING_construct(dotnetCoreModuleEntryClass);
                                if (config->dotnetCoreModuleEntryClass == NULL)
                                {
                                    //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_013: [ DotnetModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
                                    LogError("STRING_construct failed");
                                    STRING_delete(config->dotnetCoreModulePath);
                                    free(config);
                                    config = NULL;
                                }
                                else
                                {
                                    //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_015: [ DotnetModuleLoader_ParseEntrypointFromJson shall return a non-NULL pointer to the parsed representation of the entrypoint when successful. ]
                                    /**
                                    * Everything's good.
                                    */
                                }
                            }
                        }
                    }
                    else
                    {
                        //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_013: [ DotnetModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
                        LogError("malloc failed");
                    }
                }
            }
        }
    }

    return (void*)config;
}

static void DotnetCoreModuleLoader_FreeEntrypoint(const MODULE_LOADER* loader, void* entrypoint)
{
    (void)loader;

    if (entrypoint != NULL)
    {
        //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_017: [ DotnetModuleLoader_FreeEntrypoint shall free resources allocated during DotnetModuleLoader_ParseEntrypointFromJson. ]
        DOTNET_CORE_LOADER_ENTRYPOINT* ep = (DOTNET_CORE_LOADER_ENTRYPOINT*)entrypoint;
        STRING_delete(ep->dotnetCoreModuleEntryClass);
        STRING_delete(ep->dotnetCoreModulePath);

        free(ep);
    }
    else
    {
        //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_016: [ DotnetModuleLoader_FreeEntrypoint shall do nothing if entrypoint is NULL. ]
        LogError("entrypoint is NULL");
    }
}

static MODULE_LOADER_BASE_CONFIGURATION* DotnetCoreModuleLoader_ParseConfigurationFromJson(const MODULE_LOADER* loader, const JSON_Value* json)
{
    (void)loader;

    DOTNET_CORE_LOADER_CONFIGURATION* config;

    if (json == NULL)
    {
        /* Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_018: [ DotnetCoreModuleLoader_ParseConfigurationFromJson shall return NULL if an underlying platform call fails. ] */
        LogError("json is NULL.");
        config = NULL;
    }
    else
    {
        JSON_Object* object = json_value_get_object(json);
        if (object == NULL)
        {
            /* Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_018: [ DotnetCoreModuleLoader_ParseConfigurationFromJson shall return NULL if an underlying platform call fails. ] */
            config = NULL;
            LogError("could not retrieve json object.");
        }
        else
        {
            config = (DOTNET_CORE_LOADER_CONFIGURATION*)malloc(sizeof(DOTNET_CORE_LOADER_CONFIGURATION));
            if (config == NULL)
            {
                LogError("malloc failed.");
            }
            else
            {
                config->clrOptions = (DOTNET_CORE_CLR_OPTIONS*)malloc(sizeof(DOTNET_CORE_CLR_OPTIONS));
                if (config->clrOptions == NULL)
                {
                    /* Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_018: [ DotnetCoreModuleLoader_ParseConfigurationFromJson shall return NULL if an underlying platform call fails. ] */
                    free(config);
                    config = NULL;
                    LogError("malloc failed.");
                }
                else
                {
                    /* Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_040: [ DotnetCoreModuleLoader_ParseConfigurationFromJson shall set default paths on clrOptions. ] */
                    int status = set_default_paths(config->clrOptions);

                    if (status != 0)
                    {
                        free(config->clrOptions);
                        config->clrOptions = NULL;
                        free(config);
                        config = NULL;
                        LogError("Failed to set default paths.");
                    }
                    else
                    {
                        const char* temp = json_object_get_string(object, DOTNET_CORE_CLR_PATH_KEY);
                        if (temp != NULL)
                        {
                            free((char*)(config->clrOptions->coreClrPath));
                            /* Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_041: [ DotnetCoreModuleLoader_ParseConfigurationFromJson shall check if JSON contains DOTNET_CORE_CLR_PATH_KEY, and if it has it shall change the value of clrOptions->coreClrPath. ] */
                            status = mallocAndStrcpy_s((char**)(&config->clrOptions->coreClrPath), temp);
                        }

                        if (status != 0)
                        {
                            free(config->clrOptions);
                            config->clrOptions = NULL;
                            free(config);
                            config = NULL;
                            LogError("Failed to allocate binding.coreClrPath");
                        }
                        else
                        {
                            const char* temp = json_object_get_string(object, DOTNET_CORE_TRUSTED_PLATFORM_ASSEMBLIES_LOCATION_KEY);
                            if (temp != NULL)
                            {
                                free((char*)(config->clrOptions->trustedPlatformAssembliesLocation));
                                /* Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_042: [ DotnetCoreModuleLoader_ParseConfigurationFromJson shall check if JSON contains DOTNET_CORE_TRUSTED_PLATFORM_ASSEMBLIES_LOCATION_KEY, and if it has it shall change the value of clrOptions->trustedPlatformAssembliesLocation . ] */
                                status = mallocAndStrcpy_s((char**)(&config->clrOptions->trustedPlatformAssembliesLocation), temp);
                            }

                            if (status != 0)
                            {
                                free((char*)config->clrOptions->coreClrPath);
                                free(config->clrOptions);
                                config->clrOptions = NULL;
                                free(config);
                                config = NULL;
                                LogError("Failed to allocate binding.coreClrPath");
                            }
                            else
                            {
                                //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_019: [ DotnetModuleLoader_ParseConfigurationFromJson shall call ModuleLoader_ParseBaseConfigurationFromJson to parse the loader configuration and return the result. ]
                                if (ModuleLoader_ParseBaseConfigurationFromJson(&((DOTNET_CORE_LOADER_CONFIGURATION*)(config))->base, json) != MODULE_LOADER_SUCCESS)
                                {
                                    LogError("ModuleLoader_ParseBaseConfigurationFromJson failed");
                                    free((char*)config->clrOptions->trustedPlatformAssembliesLocation);
                                    free((char*)config->clrOptions->coreClrPath);
                                    free(config->clrOptions);
                                    config->clrOptions = NULL;
                                    free(config);
                                    config = NULL;
                                    LogError("Failed to allocate binding.coreClrPath");
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
            }
        }
    }

    return (MODULE_LOADER_BASE_CONFIGURATION*)config;
}

static void DotnetCoreModuleLoader_FreeConfiguration(const MODULE_LOADER* loader, MODULE_LOADER_BASE_CONFIGURATION* configuration)
{
    (void)loader;

    if (configuration != NULL)
    {
        //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_021: [ DotnetModuleLoader_FreeConfiguration shall call ModuleLoader_FreeBaseConfiguration to free resources allocated by ModuleLoader_ParseBaseConfigurationFromJson. ]
        ModuleLoader_FreeBaseConfiguration(configuration);
        //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_022: [ DotnetModuleLoader_FreeConfiguration shall free resources allocated by DotnetModuleLoader_ParseConfigurationFromJson. ]
        DOTNET_CORE_LOADER_CONFIGURATION* dotNetCoreLoaderConfig = (DOTNET_CORE_LOADER_CONFIGURATION*)configuration;

        free((char*)dotNetCoreLoaderConfig->clrOptions->coreClrPath);
        free((char*)dotNetCoreLoaderConfig->clrOptions->trustedPlatformAssembliesLocation);
        free(dotNetCoreLoaderConfig->clrOptions);
        free(configuration);
    }
    else
    {
        //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_020: [ DotnetModuleLoader_FreeConfiguration shall do nothing if configuration is NULL. ]
        LogError("configuration is NULL");
    }
}

void* DotnetCoreModuleLoader_BuildModuleConfiguration(
    const MODULE_LOADER* loader,
    const void* entrypoint,
    const void* module_configuration
)
{
    DOTNET_CORE_HOST_CONFIG* result;
    if (entrypoint == NULL)
    {
        //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_023: [ DotnetModuleLoader_BuildModuleConfiguration shall return NULL if entrypoint is NULL. ]
        LogError("entrypoint is NULL.");
        result = NULL;
    }
    else
    {
        DOTNET_CORE_LOADER_ENTRYPOINT* dotnet_core_entrypoint = (DOTNET_CORE_LOADER_ENTRYPOINT*)entrypoint;
        if (dotnet_core_entrypoint->dotnetCoreModulePath == NULL)
        {
            //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_024: [ DotnetModuleLoader_BuildModuleConfiguration shall return NULL if entrypoint->dotnetModulePath is NULL. ]
            LogError("entrypoint dotnetModulePath is NULL.");
            result = NULL;
        }
        else if (dotnet_core_entrypoint->dotnetCoreModuleEntryClass == NULL)
        {
            //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_037: [ DotnetModuleLoader_BuildModuleConfiguration shall return NULL if entrypoint->dotnetModuleEntryClass is NULL. ]
            LogError("entrypoint dotnetModuleEntryClass is NULL.");
            result = NULL;
        }
        else
        {
            if (loader == NULL || CONFIGURATION->clrOptions == NULL)
            {
                result = NULL;
                LogError("loader or loader clrOptions is NULL");
            }
            else
            {
                //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_026: [ DotnetModuleLoader_BuildModuleConfiguration shall build a DOTNET_HOST_CONFIG object by copying information from entrypoint and module_configuration and return a non-NULL pointer. ]
                result = (DOTNET_CORE_HOST_CONFIG*)malloc(sizeof(DOTNET_CORE_HOST_CONFIG));
                if (result == NULL)
                {
                    //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_025: [ DotnetModuleLoader_BuildModuleConfiguration shall return NULL if an underlying platform call fails. ]
                    LogError("malloc failed.");
                }
                else
                {
                    if (mallocAndStrcpy_s((char**)&(result->entryType), (char*)STRING_c_str(dotnet_core_entrypoint->dotnetCoreModuleEntryClass)) != 0)
                    {
                        //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_025: [ DotnetModuleLoader_BuildModuleConfiguration shall return NULL if an underlying platform call fails. ]
                        LogError("Failed to malloc and Copy dotnetModuleEntryClass String.");
                        free(result);
                        result = NULL;
                    }
                    else
                    {
                        if (mallocAndStrcpy_s((char**)&result->assemblyName, (char*)STRING_c_str(dotnet_core_entrypoint->dotnetCoreModulePath)) != 0)
                        {
                            //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_025: [ DotnetModuleLoader_BuildModuleConfiguration shall return NULL if an underlying platform call fails. ]
                            LogError("Failed to malloc and Copy assemblyName failed.");
                            free((char*)result->entryType);
                            free(result);
                            result = NULL;
                        }
                        else
                        {
                            if (module_configuration != NULL && mallocAndStrcpy_s((char**)&(result->moduleArgs), (char*)module_configuration) != 0)
                            {
                                //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_025: [ DotnetModuleLoader_BuildModuleConfiguration shall return NULL if an underlying platform call fails. ]
                                LogError("Malloc and Copy for module_configuration failed.");
                                free((char*)result->assemblyName);
                                free((char*)result->entryType);
                                free(result);
                                result = NULL;
                            }
                            else if (module_configuration == NULL)
                            {
                                result->moduleArgs = NULL;
                            }
                            else
                            {
                                //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_026: [ DotnetModuleLoader_BuildModuleConfiguration shall build a DOTNET_HOST_CONFIG object by copying information from entrypoint and module_configuration and return a non-NULL pointer. ]
                                /**
                                * Everything's good.
                                */
                                result->clrOptions = CONFIGURATION->clrOptions;
                            }
                        }
                    }
                }
            }
        }
    }

    return result;
}

void DotnetCoreModuleLoader_FreeModuleConfiguration(const MODULE_LOADER* loader, const void* module_configuration)
{
    (void)loader;

    if (module_configuration != NULL)
    {
        DOTNET_CORE_HOST_CONFIG* configuration = (DOTNET_CORE_HOST_CONFIG*)module_configuration;
        //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_028: [ DotnetModuleLoader_FreeModuleConfiguration shall free the DOTNET_HOST_CONFIG object. ]
        free((char*)configuration->moduleArgs);
        free((char*)configuration->entryType);
        free((char*)configuration->assemblyName);
        free((void*)module_configuration);
    }
    else
    {
        //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_027: [ DotnetModuleLoader_FreeModuleConfiguration shall do nothing if module_configuration is NULL. ]
        LogError("module_configuration is NULL");
    }
}

/* Codes_SRS*/
MODULE_LOADER_API Dotnet_Core_Module_Loader_API =
{
    .Load = DotnetCoreModuleLoader_Load,
    .Unload = DotnetCoreModuleLoader_Unload,
    .GetApi = DotnetCoreModuleLoader_GetModuleApi,

    .ParseEntrypointFromJson = DotnetCoreModuleLoader_ParseEntrypointFromJson,
    .FreeEntrypoint = DotnetCoreModuleLoader_FreeEntrypoint,

    .ParseConfigurationFromJson = DotnetCoreModuleLoader_ParseConfigurationFromJson,
    .FreeConfiguration = DotnetCoreModuleLoader_FreeConfiguration,

    .BuildModuleConfiguration = DotnetCoreModuleLoader_BuildModuleConfiguration,
    .FreeModuleConfiguration = DotnetCoreModuleLoader_FreeModuleConfiguration
};


MODULE_LOADER Dotnet_Core_Module_Loader =
{
    DOTNETCORE, //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_038: [ MODULE_LOADER::type shall be DOTNETCORE. ]
    DOTNET_CORE_LOADER_NAME, //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_039: [ MODULE_LOADER::name shall be the string 'dotnetcore'. ]
    NULL,
    &Dotnet_Core_Module_Loader_API
};

const MODULE_LOADER* DotnetCoreLoader_Get(void)
{
    MODULE_LOADER* loader;

    MODULE_LOADER_BASE_CONFIGURATION* default_config = (MODULE_LOADER_BASE_CONFIGURATION*)create_default_config();

    if (default_config == NULL)
    {
        loader = NULL;
        LogError("Error creating default config.");
    }
    else
    {
        loader = &Dotnet_Core_Module_Loader;
        loader->configuration = default_config;
    }


    //Codes_SRS_DOTNET_CORE_MODULE_LOADER_04_029: [ DotnetLoader_Get shall return a non-NULL pointer to a MODULE_LOADER struct. ]
    return loader;
}

static int set_default_paths(DOTNET_CORE_CLR_OPTIONS* options)
{
    int result = 0;

    if (options == NULL)
    {
        result = __LINE__;
    }
    else
    {
        //set coreClrPath 
        result = mallocAndStrcpy_s((char**)(&options->coreClrPath), DOTNET_CORE_CLR_PATH_DEFAULT);
        if (result != 0)
        {
            result = __LINE__;
        }
        else
        {
            //set trustedPlatformAssembliesLocation
            result = mallocAndStrcpy_s((char**)(&options->trustedPlatformAssembliesLocation), DOTNET_CORE_TRUSTED_PLATFORM_ASSEMBLIES_LOCATION_DEFAULT);
            if (result != 0)
            {
                free((char*)(options->coreClrPath));
                result = __LINE__;
            }
        }
    }

    return result;
}

static DOTNET_CORE_LOADER_CONFIGURATION* create_default_config()
{
    DOTNET_CORE_LOADER_CONFIGURATION* result;

    DOTNET_CORE_CLR_OPTIONS* default_clr_options = (DOTNET_CORE_CLR_OPTIONS*)malloc(sizeof(DOTNET_CORE_CLR_OPTIONS));
    if (default_clr_options == NULL)
    {
        result = NULL;
        LogError("malloc failed.");
    }
    else
    {
        if (set_default_paths(default_clr_options) != 0)
        {
            free(default_clr_options);
            result = NULL;
            LogError("Failed to set default paths.");
        }
        else
        {
            result = (DOTNET_CORE_LOADER_CONFIGURATION*)malloc(sizeof(DOTNET_CORE_LOADER_CONFIGURATION));
            if (result == NULL)
            {
                free((char*)(default_clr_options->coreClrPath));
                free((char*)(default_clr_options->trustedPlatformAssembliesLocation));
                free(default_clr_options);
                result = NULL;
                LogError("malloc failed.");
            }
            else
            {
                result->base.binding_path = NULL;
                result->clrOptions = default_clr_options;
            }
        }
    }
    return result;
}
