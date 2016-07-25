// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "azure_c_shared_utility/gballoc.h"
#include <string.h>

#include "azure_c_shared_utility/xlogging.h"

#include "module_loader.h"
#include "dynamic_library.h"

typedef struct MODULE_LIBRARY_HANDLE_DATA_TAG
{
    void* library;
    const MODULE_APIS* apis;
}MODULE_LIBRARY_HANDLE_DATA;



MODULE_LIBRARY_HANDLE ModuleLoader_Load(const char* moduleLibraryFileName)
{
  MODULE_LIBRARY_HANDLE_DATA* result;

  // moduleLibraryFileName cannot be null/empty
  if (moduleLibraryFileName == NULL)
  {
      /*Codes_SRS_MODULE_LOADER_17_001: [ModuleLoader_Load shall validate the moduleLibraryFileName, if it is NULL, it will return NULL.]*/
      result = NULL;
      LogError("ModuleLoader_Load() - moduleLibraryFileName is NULL");
  }
  else
  {
      /* Codes_SRS_MODULE_LOADER_17_005: [ModuleLoader_Load shall allocate memory for the structure MODULE_LIBRARY_HANDLE.] */
      result = (MODULE_LIBRARY_HANDLE_DATA*)malloc(sizeof(MODULE_LIBRARY_HANDLE_DATA));
      if (result == NULL)
      {
          /*Codes_SRS_MODULE_LOADER_17_014: [If memory allocation is not successful, the load shall fail, and it shall return NULL.]*/
          LogError("ModuleLoader_Load() - malloc(sizeof(MODULE_LIBRARY_HANDLE_DATA)) failed");
      }
      else
      {
          /* load the DLL */
          /* Codes_SRS_MODULE_LOADER_17_002: [ModuleLoader_Load shall load the library as a file, the filename given by the moduleLibraryFileName.]*/
          result->library = DynamicLibrary_LoadLibrary(moduleLibraryFileName);
          if (result->library == NULL)
          {
              /* Codes_SRS_MODULE_LOADER_17_012: [If the attempt is not successful, the load shall fail, and it shall return NULL.]*/
              free(result);
              result = NULL;
              LogError("ModuleLoader_Load() - DynamicLibrary_LoadLibrary() returned NULL");
          }
          else
          {
              /* Codes_SRS_MODULE_LOADER_17_003: [ModuleLoader_Load shall locate the function defined by MODULE_GETAPIS_NAME in the open library.] */
              pfModule_GetAPIS pfnGetAPIS = (pfModule_GetAPIS)DynamicLibrary_FindSymbol(result->library, MODULE_GETAPIS_NAME);
              if (pfnGetAPIS == NULL)
              {
                  /* Codes_SRS_MODULE_LOADER_17_013: [If locating the function is not successful, the load shall fail, and it shall return NULL.]*/
                  DynamicLibrary_UnloadLibrary(result->library); 
                  free(result);
                  result = NULL;
                  LogError("ModuleLoader_Load() - DynamicLibrary_FindSymbol() returned NULL");
              }
              else
              {
                  /* Codes_SRS_MODULE_LOADER_17_004: [ModuleLoader_Load shall call the function defined by MODULE_GETAPIS_NAME in the open library.]*/
                  result->apis = pfnGetAPIS();

                  /* if "apis" is NULL then we have a misbehaving module */
                  if (result->apis == NULL)
                  {
                      /* Codes_SRS_MODULE_LOADER_17_015: [If the get API call returns NULL, the load shall fail, and it shall return NULL.]*/
                      DynamicLibrary_UnloadLibrary(result->library);
                      free(result);
                      result = NULL;
                      LogError("ModuleLoader_Load() - pfnGetAPIS() returned NULL");
                  }
              }
          }
      }
  }

  /*Codes_SRS_MODULE_LOADER_17_006: [ModuleLoader_Load shall return a non-NULL handle to a MODULE_LIBRARY_DATA_TAG upon success.]*/
  return result;
}

const MODULE_APIS* ModuleLoader_GetModuleAPIs(MODULE_LIBRARY_HANDLE moduleLibraryHandle)
{
    const MODULE_APIS* result;

    if (moduleLibraryHandle == NULL)
    {
        /*Codes_SRS_MODULE_LOADER_17_007: [ModuleLoader_GetModuleAPIs shall return NULL if the moduleLibraryHandle is NULL.]*/
        result = NULL;
        LogError("ModuleLoader_GetModuleAPIs() - moduleLibraryHandle is NULL");
    }
    else
    {
        /*Codes_SRS_MODULE_LOADER_17_008: [ModuleLoader_GetModuleAPIs shall return a valid pointer to MODULE_APIS on success.]*/
        MODULE_LIBRARY_HANDLE_DATA* loader_data = moduleLibraryHandle;
        result = loader_data->apis;
    }

    return result;
}


/*Codes_SRS_MODULE_LOADER_17_009: [ModuleLoader_Unload shall do nothing if the moduleLibraryHandle is NULL.]*/
/*Codes_SRS_MODULE_LOADER_17_010: [ModuleLoader_Unload shall attempt to unload the library.]*/
/*Codes_SRS_MODULE_LOADER_17_011: [ModuleLoader_UnLoad shall deallocate memory for the structure MODULE_LIBRARY_HANDLE.]*/

void ModuleLoader_Unload(MODULE_LIBRARY_HANDLE moduleLibraryHandle)
{
    if (moduleLibraryHandle != NULL)
    {
        MODULE_LIBRARY_HANDLE_DATA* loader_data = moduleLibraryHandle;
            DynamicLibrary_UnloadLibrary(loader_data->library);
        free(loader_data);
    }
    else
    {
      LogError("ModuleLoader_Unload() - moduleLibraryHandle is NULL");
    }
}
