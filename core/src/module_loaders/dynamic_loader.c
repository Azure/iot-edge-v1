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

    if (loader == NULL || entrypoint == NULL)
    {
        //Codes_SRS_DYNAMIC_MODULE_LOADER_13_001: [ DynamicModuleLoader_Load shall return NULL if loader is NULL. ]
        //Codes_SRS_DYNAMIC_MODULE_LOADER_13_041: [ DynamicModuleLoader_Load shall return NULL if entrypoint is NULL. ]
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
            //Codes_SRS_DYNAMIC_MODULE_LOADER_13_002: [ DynamicModuleLoader_Load shall return NULL if loader->type is not NATIVE. ]
            result = NULL;
            LogError("loader->type is not NATIVE");
        }
        else
        {
            DYNAMIC_LOADER_ENTRYPOINT* dynamic_loader_entrypoint = (DYNAMIC_LOADER_ENTRYPOINT*)entrypoint;
            if (dynamic_loader_entrypoint->moduleLibraryFileName == NULL)
            {
                //Codes_SRS_DYNAMIC_MODULE_LOADER_13_039: [ DynamicModuleLoader_Load shall return NULL if entrypoint->moduleLibraryFileName is NULL. ]
                result = NULL;
                LogError("moduleLibraryFileName is NULL");
            }
            else
            {
                const char * moduleLibraryFileName = STRING_c_str(dynamic_loader_entrypoint->moduleLibraryFileName);
                result = (DYNAMIC_MODULE_HANDLE_DATA*)malloc(sizeof(DYNAMIC_MODULE_HANDLE_DATA));
                if (result == NULL)
                {
                    //Codes_SRS_DYNAMIC_MODULE_LOADER_13_003: [ DynamicModuleLoader_Load shall return NULL if an underlying platform call fails. ]
                    LogError("malloc(sizeof(DYNAMIC_MODULE_HANDLE_DATA)) failed");
                }
                else
                {
                    /* load the DLL */
                    //Codes_SRS_DYNAMIC_MODULE_LOADER_13_004: [ DynamicModuleLoader_Load shall load the module into memory by calling DynamicLibrary_LoadLibrary. ]
                    result->library = DynamicLibrary_LoadLibrary(moduleLibraryFileName);
                    if (result->library == NULL)
                    {
                        //Codes_SRS_DYNAMIC_MODULE_LOADER_13_003: [ DynamicModuleLoader_Load shall return NULL if an underlying platform call fails. ]
                        free(result);
                        result = NULL;
                        LogError("DynamicLibrary_LoadLibrary() returned NULL");
                    }
                    else
                    {
                        //Codes_SRS_DYNAMIC_MODULE_LOADER_13_033: [ DynamicModuleLoader_Load shall call DynamicLibrary_FindSymbol on the module handle with the symbol name Module_GetApi to acquire the function that returns the module's API table. ]
                        pfModule_GetApi pfnGetAPI = (pfModule_GetApi)DynamicLibrary_FindSymbol(result->library, MODULE_GETAPI_NAME);
                        if (pfnGetAPI == NULL)
                        {
                            //Codes_SRS_DYNAMIC_MODULE_LOADER_13_003: [ DynamicModuleLoader_Load shall return NULL if an underlying platform call fails. ]
                            DynamicLibrary_UnloadLibrary(result->library);
                            free(result);
                            result = NULL;
                            LogError("DynamicLibrary_FindSymbol() returned NULL");
                        }
                        else
                        {
                            //Codes_SRS_DYNAMIC_MODULE_LOADER_13_040: [ DynamicModuleLoader_Load shall call the module's Module_GetAPI callback to acquire the module API table. ]
                            result->api = pfnGetAPI(Module_ApiGatewayVersion);

                            /* if any of the required functions is NULL then we have a misbehaving module */
                            if (result->api == NULL ||
                                result->api->version > Module_ApiGatewayVersion ||
                                MODULE_CREATE(result->api) == NULL ||
                                MODULE_DESTROY(result->api) == NULL ||
                                MODULE_RECEIVE(result->api) == NULL)
                            {
                                //Codes_SRS_DYNAMIC_MODULE_LOADER_13_034: [ DynamicModuleLoader_Load shall return NULL if the MODULE_API pointer returned by the module is NULL. ]
                                //Codes_SRS_DYNAMIC_MODULE_LOADER_13_035: [ DynamicModuleLoader_Load shall return NULL if MODULE_API::version is greater than Module_ApiGatewayVersion. ]
                                //Codes_SRS_DYNAMIC_MODULE_LOADER_13_036: [ DynamicModuleLoader_Load shall return NULL if the Module_Create function in MODULE_API is NULL. ]
                                //Codes_SRS_DYNAMIC_MODULE_LOADER_13_037: [ DynamicModuleLoader_Load shall return NULL if the Module_Receive function in MODULE_API is NULL. ]
                                //Codes_SRS_DYNAMIC_MODULE_LOADER_13_038: [ DynamicModuleLoader_Load shall return NULL if the Module_Destroy function in MODULE_API is NULL. ]
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

    //Codes_SRS_DYNAMIC_MODULE_LOADER_13_005: [ DynamicModuleLoader_Load shall return a non-NULL pointer of type MODULE_LIBRARY_HANDLE when successful. ]
    return result;
}

static const MODULE_API* DynamicModuleLoader_GetModuleApi(const MODULE_LOADER* loader, MODULE_LIBRARY_HANDLE moduleLibraryHandle)
{
    (void)loader;

    const MODULE_API* result;

    if (moduleLibraryHandle == NULL)
    {
        /*Codes_SRS_MODULE_LOADER_17_007: [DynamicModuleLoader_GetModuleApi shall return NULL if the moduleLibraryHandle is NULL.]*/
        result = NULL;
        LogError("moduleLibraryHandle is NULL");
    }
    else
    {
        /*Codes_SRS_MODULE_LOADER_17_008: [DynamicModuleLoader_GetModuleApi shall return a valid pointer to MODULE_API on success.]*/
        DYNAMIC_MODULE_HANDLE_DATA* loader_data = moduleLibraryHandle;
        result = loader_data->api;
    }

    return result;
}

static void DynamicModuleLoader_Unload(const MODULE_LOADER* loader, MODULE_LIBRARY_HANDLE moduleLibraryHandle)
{
    (void)loader;

    if (moduleLibraryHandle != NULL)
    {
        DYNAMIC_MODULE_HANDLE_DATA* loader_data = moduleLibraryHandle;

        /*Codes_SRS_MODULE_LOADER_17_010: [DynamicModuleLoader_Unload shall attempt to unload the library.]*/
        DynamicLibrary_UnloadLibrary(loader_data->library);

        /*Codes_SRS_MODULE_LOADER_17_011: [DynamicModuleLoader_Unload shall deallocate memory for the structure MODULE_LIBRARY_HANDLE.]*/
        free(loader_data);
    }
    else
    {
        /*Codes_SRS_MODULE_LOADER_17_009: [DynamicModuleLoader_Unload shall do nothing if the moduleLibraryHandle is NULL.]*/
        LogError("moduleLibraryHandle is NULL");
    }
}

static void* DynamicModuleLoader_ParseEntrypointFromJson(const MODULE_LOADER* loader, const JSON_Value* json)
{
    (void)loader;
    // The input is a JSON object that looks like this:
    //  "entrypoint": {
    //      "module.path": "path/to/module"
    //  }
    DYNAMIC_LOADER_ENTRYPOINT* config;
    if (json == NULL)
    {
        LogError("json is NULL");

        //Codes_SRS_DYNAMIC_MODULE_LOADER_13_042 : [DynamicModuleLoader_ParseEntrypointFromJson shall return NULL if json is NULL.]
        config = NULL;
    }
    else
    {
        // "json" must be an "object" type
        if (json_value_get_type(json) != JSONObject)
        {
            LogError("'json' is not an object value");

            //Codes_SRS_DYNAMIC_MODULE_LOADER_13_043 : [DynamicModuleLoader_ParseEntrypointFromJson shall return NULL if the root json entity is not an object.]
            config = NULL;
        }
        else
        {
            JSON_Object* entrypoint = json_value_get_object(json);
            if (entrypoint == NULL)
            {
                LogError("json_value_get_object failed");

                //Codes_SRS_DYNAMIC_MODULE_LOADER_13_044 : [DynamicModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails.]
                config = NULL;
            }
            else
            {
                //Codes_SRS_DYNAMIC_MODULE_LOADER_13_045 : [DynamicModuleLoader_ParseEntrypointFromJson shall retrieve the path to the module library file by reading the value of the attribute module.path.]
                const char* modulePath = json_object_get_string(entrypoint, "module.path");
                if (modulePath == NULL)
                {
                    LogError("json_object_get_string for 'module.path' returned NULL");

                    //Codes_SRS_DYNAMIC_MODULE_LOADER_13_047: [ DynamicModuleLoader_ParseEntrypointFromJson shall return NULL if module.path does not exist. ]
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
                            LogError("STRING_construct failed");
                            free(config);

                            //Codes_SRS_DYNAMIC_MODULE_LOADER_13_044 : [DynamicModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails.]
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
                        //Codes_SRS_DYNAMIC_MODULE_LOADER_13_044 : [DynamicModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails.]
                        LogError("malloc failed");
                    }
                }
            }
        }
    }

    //Codes_SRS_DYNAMIC_MODULE_LOADER_13_046: [DynamicModuleLoader_ParseEntrypointFromJson shall return a non - NULL pointer to the parsed representation of the entrypoint when successful.]
    return (void*)config;
}

static void DynamicModuleLoader_FreeEntrypoint(const MODULE_LOADER* loader, void* entrypoint)
{
    (void)loader;

    if (entrypoint != NULL)
    {
        //Codes_SRS_DYNAMIC_MODULE_LOADER_13_048: [ DynamicModuleLoader_FreeEntrypoint shall free resources allocated during DynamicModuleLoader_ParseEntrypointFromJson. ]
        DYNAMIC_LOADER_ENTRYPOINT* ep = (DYNAMIC_LOADER_ENTRYPOINT*)entrypoint;
        STRING_delete(ep->moduleLibraryFileName);
        free(ep);
    }
    else
    {
        //Codes_SRS_DYNAMIC_MODULE_LOADER_13_049: [ DynamicModuleLoader_FreeEntrypoint shall do nothing if entrypoint is NULL. ]
        LogError("entrypoint is NULL");
    }
}

static MODULE_LOADER_BASE_CONFIGURATION* DynamicModuleLoader_ParseConfigurationFromJson(const MODULE_LOADER* loader, const JSON_Value* json)
{
    (void)loader;

    /**
     * The dynamic loader does not have any configuration so we always return NULL.
     */
    //Codes_SRS_DYNAMIC_MODULE_LOADER_13_050: [ `DynamicModuleLoader_ParseConfigurationFromJson` shall return `NULL`. ]
    return NULL;
}

static void DynamicModuleLoader_FreeConfiguration(const MODULE_LOADER* loader, MODULE_LOADER_BASE_CONFIGURATION* configuration)
{
    (void)loader;

    /**
     * Nothing to free.
     */
    //Codes_SRS_DYNAMIC_MODULE_LOADER_13_051: [ `DynamicModuleLoader_FreeConfiguration` shall do nothing. ]
}

static void* DynamicModuleLoader_BuildModuleConfiguration(
    const MODULE_LOADER* loader,
    const void* entrypoint,
    const void* module_configuration
)
{
    (void)loader;

   /**
    * The native dynamic loader does not need to do any special configuration translation.
    * We simply return the module_configuration as is.
    */
    //Codes_SRS_DYNAMIC_MODULE_LOADER_13_052: [DynamicModuleLoader_BuildModuleConfiguration shall return module_configuration. ]
    return (void *)module_configuration;
}

static void DynamicModuleLoader_FreeModuleConfiguration(const MODULE_LOADER* loader, const void* module_configuration)
{
    (void)loader;

    /**
     * Nothing to free.
     */
    //Codes_SRS_DYNAMIC_MODULE_LOADER_13_053: [ `DynamicModuleLoader_FreeModuleConfiguration` shall do nothing. ]
}

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
    //Codes_SRS_DYNAMIC_MODULE_LOADER_13_054: [DynamicModuleLoader_Get shall return a non - NULL pointer to a MODULE_LOADER struct.]
    //Codes_SRS_DYNAMIC_MODULE_LOADER_13_055 : [MODULE_LOADER::type shall be NATIVE.]
    //Codes_SRS_DYNAMIC_MODULE_LOADER_13_056 : [MODULE_LOADER::name shall be the string native.]
    return &Dynamic_Module_Loader;
}
