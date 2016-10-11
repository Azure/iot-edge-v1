// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MODULE_LOADER_H
#define MODULE_LOADER_H

#include "module.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** @brief handle for a module library */
typedef struct MODULE_LIBRARY_HANDLE_DATA_TAG* MODULE_LIBRARY_HANDLE;

typedef MODULE_LIBRARY_HANDLE(*pfModuleLoader_Load)(const void * config);
typedef void(*pfModuleLoader_Unload)(MODULE_LIBRARY_HANDLE handle);
typedef const MODULE_API*(*pfModuleLoader_GetApi)(MODULE_LIBRARY_HANDLE handle);


/** @brief function table for loading modules into a gateway */
typedef struct MODULE_LOADER_API_TAG
{
    /** @brief Load function, loads module for gateway, returns a valid handle on success */    
	pfModuleLoader_Load Load;
    /** @brief Unload function, unloads the library from the gateway */    
	pfModuleLoader_Unload Unload;
    /** @brief GetApi function, gets the MODULE_API for the loaded module */  
	pfModuleLoader_GetApi GetApi;
} MODULE_LOADER_API;

#ifdef __cplusplus
}
#endif

#endif // MODULE_LOADER_H
