// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "azure_c_shared_utility/gballoc.h"
#include <string.h>

#include "azure_c_shared_utility/xlogging.h"

#include "module.h"
#include "module_access.h"
#include "module_loader.h"
#include "dynamic_loader.h"
#include "dynamic_library.h"

typedef struct MODULE_LIBRARY_HANDLE_DATA_TAG
{
    void* library;
    const MODULE_API* api;
}MODULE_LIBRARY_HANDLE_DATA;



static MODULE_LIBRARY_HANDLE DynamicModuleLoader_Load(const void* loader_config)
{
    MODULE_LIBRARY_HANDLE_DATA* result;

    // configuration cannot be null/empty
    if (loader_config == NULL)
    {
        /*Codes_SRS_MODULE_LOADER_17_001: [DynamicModuleLoader_Load shall validate the moduleLibraryFileName, if it is NULL, it will return NULL.]*/
        result = NULL;
        LogError("DynamicModuleLoader_Load() - loader_config is NULL");
    }
    else
    {
        DYNAMIC_LOADER_CONFIG* dynamic_loader_config = (DYNAMIC_LOADER_CONFIG*)loader_config;
        if (dynamic_loader_config->moduleLibraryFileName == NULL)
        {
            result = NULL;
            LogError("DynamicModuleLoader_Load() - moduleLibraryFileName is NULL");
        }
        else
        {
            const char * moduleLibraryFileName = dynamic_loader_config->moduleLibraryFileName;
            /* Codes_SRS_MODULE_LOADER_17_005: [DynamicModuleLoader_Load shall allocate memory for the structure MODULE_LIBRARY_HANDLE.] */
            result = (MODULE_LIBRARY_HANDLE_DATA*)malloc(sizeof(MODULE_LIBRARY_HANDLE_DATA));
            if (result == NULL)
            {
                /*Codes_SRS_MODULE_LOADER_17_014: [If memory allocation is not successful, the load shall fail, and it shall return NULL.]*/
                LogError("DynamicModuleLoader_Load() - malloc(sizeof(MODULE_LIBRARY_HANDLE_DATA)) failed");
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
                    LogError("DynamicModuleLoader_Load() - DynamicLibrary_LoadLibrary() returned NULL");
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
                        LogError("DynamicModuleLoader_Load() - DynamicLibrary_FindSymbol() returned NULL");
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
                            LogError("DynamicModuleLoader_Load() - pfnGetapi() returned NULL");
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
        MODULE_LIBRARY_HANDLE_DATA* loader_data = moduleLibraryHandle;
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
        MODULE_LIBRARY_HANDLE_DATA* loader_data = moduleLibraryHandle;
        DynamicLibrary_UnloadLibrary(loader_data->library);
        free(loader_data);
    }
    else
    {
      LogError("DynamicModuleLoader_Unload() - moduleLibraryHandle is NULL");
    }
}

/* Codes_SRS_MODULE_LOADER_17_015: [ DynamicLoader_GetApi shall set all the fields of the MODULE_LOADER_API structure. ]*/
const MODULE_LOADER_API Dynamic_Module_Loader_API =
{
    .Load = DynamicModuleLoader_Load,
    .Unload = DynamicModuleLoader_Unload,
    .GetApi = DynamicModuleLoader_GetModuleApi
};

const MODULE_LOADER_API * DynamicLoader_GetApi(void)
{
    /* Codes_SRS_MODULE_LOADER_17_020: [ DynamicLoader_GetApi shall return a non-NULL pointer to a MODULER_LOADER structure. ] */
    return &Dynamic_Module_Loader_API;
}
