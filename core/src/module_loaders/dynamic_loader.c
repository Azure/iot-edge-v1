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
#include "module_loaders/dynamic_loader.h"
#include "dynamic_library.h"

typedef struct DYNAMIC_MODULE_HANDLE_DATA_TAG
{
    void* library;
    const MODULE_API* api;
}DYNAMIC_MODULE_HANDLE_DATA;

static MODULE_LIBRARY_HANDLE DynamicModuleLoader_Load(const MODULE_LOADER* loader, const void* entrypoint)
{
    DYNAMIC_MODULE_HANDLE_DATA* result;

    // loader & entrypoint cannot be null
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
        if (loader->type != NATIVE)
        {
            result = NULL;
            LogError("loader->type is not NATIVE");
        }
        else
        {
            DYNAMIC_LOADER_ENTRYPOINT* dynamic_loader_entrypoint = (DYNAMIC_LOADER_ENTRYPOINT*)entrypoint;
            if (dynamic_loader_entrypoint->moduleLibraryFileName == NULL)
            {
                result = NULL;
                LogError("moduleLibraryFileName is NULL");
            }
            else
            {
                const char * moduleLibraryFileName = STRING_c_str(dynamic_loader_entrypoint->moduleLibraryFileName);
                /* Codes_SRS_MODULE_LOADER_17_005: [DynamicModuleLoader_Load shall allocate memory for the structure MODULE_LIBRARY_HANDLE.] */
                result = (DYNAMIC_MODULE_HANDLE_DATA*)malloc(sizeof(DYNAMIC_MODULE_HANDLE_DATA));
                if (result == NULL)
                {
                    /*Codes_SRS_MODULE_LOADER_17_014: [If memory allocation is not successful, the load shall fail, and it shall return NULL.]*/
                    LogError("malloc(sizeof(DYNAMIC_MODULE_HANDLE_DATA)) failed");
                }
                else
                {
                    /* load the DLL */
                    /* Codes_SRS_MODULE_LOADER_17_002: [DynamicModuleLoader_Load shall load the library as a file, the filename given by the moduleLibraryFileName.]*/
                    result->library = DynamicLibrary_LoadLibrary(moduleLibraryFileName);
                    if (result->library == NULL)
                    {
                        /* Codes_SRS_MODULE_LOADER_17_012: [If the attempt is not successful, the load shall fail, and it shall return NULL.]*/
                        free(result);
                        result = NULL;
                        LogError("DynamicLibrary_LoadLibrary() returned NULL");
                    }
                    else
                    {
                        /* Codes_SRS_MODULE_LOADER_17_003: [DynamicModuleLoader_Load shall locate the function defined by MODULE_GETAPI=_NAME in the open library.] */
                        pfModule_GetApi pfnGetAPI = (pfModule_GetApi)DynamicLibrary_FindSymbol(result->library, MODULE_GETAPI_NAME);
                        if (pfnGetAPI == NULL)
                        {
                            /* Codes_SRS_MODULE_LOADER_17_013: [If locating the function is not successful, the load shall fail, and it shall return NULL.]*/
                            DynamicLibrary_UnloadLibrary(result->library);
                            free(result);
                            result = NULL;
                            LogError("DynamicLibrary_FindSymbol() returned NULL");
                        }
                        else
                        {
                            /* Codes_SRS_MODULE_LOADER_17_004: [DynamicModuleLoader_Load shall call the function defined by MODULE_GETAPI_NAME in the open library.]*/
                            result->api = pfnGetAPI(Module_ApiGatewayVersion);

                            /* if any of the required functions is NULL then we have a misbehaving module */
                            if (result->api == NULL ||
                                result->api->version > Module_ApiGatewayVersion ||
                                MODULE_CREATE(result->api) == NULL ||
                                MODULE_DESTROY(result->api) == NULL ||
                                MODULE_RECEIVE(result->api) == NULL)
                            {
                                /*Codes_SRS_MODULE_LOADER_26_001: [ If the get API call doesn't set required functions, the load shall fail and it shall return `NULL`. ]*/
                                DynamicLibrary_UnloadLibrary(result->library);
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

    /*Codes_SRS_MODULE_LOADER_17_006: [DynamicModuleLoader_Load shall return a non-NULL handle to a MODULE_LIBRARY_DATA_TAG upon success.]*/
    return result;
}

static const MODULE_API* DynamicModuleLoader_GetModuleApi(MODULE_LIBRARY_HANDLE moduleLibraryHandle)
{
    const MODULE_API* result;

    if (moduleLibraryHandle == NULL)
    {
        /*Codes_SRS_MODULE_LOADER_17_007: [DynamicModuleLoader_GetModuleApi shall return NULL if the moduleLibraryHandle is NULL.]*/
        result = NULL;
        LogError("DynamicModuleLoader_GetModuleApi() - moduleLibraryHandle is NULL");
    }
    else
    {
        /*Codes_SRS_MODULE_LOADER_17_008: [DynamicModuleLoader_GetModuleApi shall return a valid pointer to MODULE_API on success.]*/
        DYNAMIC_MODULE_HANDLE_DATA* loader_data = moduleLibraryHandle;
        result = loader_data->api;
    }

    return result;
}


/*Codes_SRS_MODULE_LOADER_17_009: [DynamicModuleLoader_Unload shall do nothing if the moduleLibraryHandle is NULL.]*/
/*Codes_SRS_MODULE_LOADER_17_010: [DynamicModuleLoader_Unload shall attempt to unload the library.]*/
/*Codes_SRS_MODULE_LOADER_17_011: [ModuleLoader_UnLoad shall deallocate memory for the structure MODULE_LIBRARY_HANDLE.]*/

static void DynamicModuleLoader_Unload(MODULE_LIBRARY_HANDLE moduleLibraryHandle)
{
    if (moduleLibraryHandle != NULL)
    {
        DYNAMIC_MODULE_HANDLE_DATA* loader_data = moduleLibraryHandle;
        DynamicLibrary_UnloadLibrary(loader_data->library);
        free(loader_data);
    }
    else
    {
        LogError("DynamicModuleLoader_Unload() - moduleLibraryHandle is NULL");
    }
}

static void* DynamicModuleLoader_ParseEntrypointFromJson(const JSON_Value* json)
{
    // The input is a JSON object that looks like this:
    //  "entrypoint": {
    //      "module.path": "path/to/module"
    //  }
    DYNAMIC_LOADER_ENTRYPOINT* config;
    if (json == NULL)
    {
        LogError("DynamicModuleLoader_ParseEntrypointFromJson() - json is NULL");
        config = NULL;
    }
    else
    {
        // "json" must be an "object" type
        if (json_value_get_type(json) != JSONObject)
        {
            LogError("DynamicModuleLoader_ParseEntrypointFromJson() - 'json' is not an object value");
            config = NULL;
        }
        else
        {
            JSON_Object* entrypoint = json_value_get_object(json);
            if (entrypoint == NULL)
            {
                LogError("DynamicModuleLoader_ParseEntrypointFromJson() - json_value_get_object failed");
                config = NULL;
            }
            else
            {
                const char* modulePath = json_object_get_string(entrypoint, "module.path");
                if (modulePath == NULL)
                {
                    LogError("DynamicModuleLoader_ParseEntrypointFromJson() - json_object_get_string for 'module.path' returned NULL");
                    config = NULL;
                }
                else
                {
                    config = (DYNAMIC_LOADER_ENTRYPOINT*)malloc(sizeof(DYNAMIC_LOADER_ENTRYPOINT));
                    if (config != NULL)
                    {
                        config->moduleLibraryFileName = STRING_construct(modulePath);
                        if (config->moduleLibraryFileName == NULL)
                        {
                            LogError("DynamicModuleLoader_ParseEntrypointFromJson() - STRING_construct failed");
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
                        LogError("DynamicModuleLoader_ParseEntrypointFromJson() - malloc failed");
                    }
                }
            }
        }
    }

    return (void*)config;
}

static void DynamicModuleLoader_FreeEntrypoint(void* entrypoint)
{
    if (entrypoint != NULL)
    {
        DYNAMIC_LOADER_ENTRYPOINT* ep = (DYNAMIC_LOADER_ENTRYPOINT*)entrypoint;
        STRING_delete(ep->moduleLibraryFileName);
        free(ep);
    }
    else
    {
        LogError("DynamicModuleLoader_FreeEntrypoint - entrypoint is NULL");
    }
}

static MODULE_LOADER_BASE_CONFIGURATION* DynamicModuleLoader_ParseConfigurationFromJson(const JSON_Value* json)
{
    /**
     * The dynamic loader does not have any configuration so we always return NULL.
     */
    return NULL;
}

static void DynamicModuleLoader_FreeConfiguration(MODULE_LOADER_BASE_CONFIGURATION* configuration)
{
    /**
     * Nothing to free.
     */
}

static void* DynamicModuleLoader_BuildModuleConfiguration(
    const MODULE_LOADER* loader,
    const void* entrypoint,
    const void* module_configuration
)
{
   /**
    * The native dynamic loader does not need to do any special configuration translation.
    * We simply return the module_configuration as is.
    */
    return (void *)module_configuration;
}

static void DynamicModuleLoader_FreeModuleConfiguration(const void* module_configuration)
{
    /**
     * Nothing to free.
     */
}

/* Codes_SRS_MODULE_LOADER_17_015: [ DynamicLoader_GetApi shall set all the fields of the MODULE_LOADER_API structure. ]*/
static MODULE_LOADER_API Dynamic_Module_Loader_API =
{
    .Load = DynamicModuleLoader_Load,
    .Unload = DynamicModuleLoader_Unload,
    .GetApi = DynamicModuleLoader_GetModuleApi,

    .ParseEntrypointFromJson = DynamicModuleLoader_ParseEntrypointFromJson,
    .FreeEntrypoint = DynamicModuleLoader_FreeEntrypoint,

    .ParseConfigurationFromJson = DynamicModuleLoader_ParseConfigurationFromJson,
    .FreeConfiguration = DynamicModuleLoader_FreeConfiguration,

    .BuildModuleConfiguration = DynamicModuleLoader_BuildModuleConfiguration,
    .FreeModuleConfiguration = DynamicModuleLoader_FreeModuleConfiguration
};

static MODULE_LOADER Dynamic_Module_Loader =
{
    NATIVE,
    DYNAMIC_LOADER_NAME,
    NULL,
    &Dynamic_Module_Loader_API
};

const MODULE_LOADER* DynamicLoader_Get(void)
{
    /* Codes_SRS_MODULE_LOADER_17_020: [ DynamicLoader_GetApi shall return a non-NULL pointer to a MODULER_LOADER structure. ] */
    return &Dynamic_Module_Loader;
}
