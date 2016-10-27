// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file       module_loader.h
 *  @brief      Definition of the functions and structures that must be
 *              implemented by a gateway module loader.
 *
 *  @details    A gateway module loader library must make available to the
 *              gateway application (e.g. via a function) an instance of the
 *              #MODULE_LOADER_API structure. This structure must contain valid
 *              pointers to functions that can be invoked by the gateway to load
 *              and unload a gateway module, and get its interface.
 */

#ifndef MODULE_LOADER_H
#define MODULE_LOADER_H

#include "azure_c_shared_utility/macro_utils.h"

#include "module.h"
#include "parson.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** @brief handle for a module library */
typedef void* MODULE_LIBRARY_HANDLE;

struct MODULE_LOADER_TAG;

/**
 * Configuration that is common to all module loaders goes here. The expectation
 * is that a given module loader will define its own configuration struct but
 * always have an instance of this struct as the first field.
 */
typedef struct MODULE_LOADER_BASE_CONFIGURATION_TAG
{
    STRING_HANDLE binding_path;
} MODULE_LOADER_BASE_CONFIGURATION;

typedef MODULE_LIBRARY_HANDLE(*pfModuleLoader_Load)(const struct MODULE_LOADER_TAG* loader, const void* entrypoint);
typedef void(*pfModuleLoader_Unload)(MODULE_LIBRARY_HANDLE handle);
typedef const MODULE_API*(*pfModuleLoader_GetApi)(MODULE_LIBRARY_HANDLE handle);

typedef void*(*pfModuleLoader_ParseEntrypointFromJson)(const JSON_Value* json);
typedef void(*pfModuleLoader_FreeEntrypoint)(void* entrypoint);

typedef MODULE_LOADER_BASE_CONFIGURATION*(*pfModuleLoader_ParseConfigurationFromJson)(const JSON_Value* json);
typedef void(*pfModuleLoader_FreeConfiguration)(MODULE_LOADER_BASE_CONFIGURATION* configuration);

typedef void*(*pfModuleLoader_BuildModuleConfiguration)(const struct MODULE_LOADER_TAG* loader, const void* entrypoint, const void* module_configuration);
typedef void(*pfModuleLoader_FreeModuleConfiguration)(const void* module_configuration);

/** @brief  Function table for loading modules into a gateway */
typedef struct MODULE_LOADER_API_TAG
{
    /** @brief  Load function, loads module for gateway, returns a valid handle
     *          on success */
    pfModuleLoader_Load Load;

    /** @brief Unload function, unloads the library from the gateway */
    pfModuleLoader_Unload Unload;

    /** @brief GetApi function, gets the MODULE_API for the loaded module */
    pfModuleLoader_GetApi GetApi;

    pfModuleLoader_ParseEntrypointFromJson ParseEntrypointFromJson;
    pfModuleLoader_FreeEntrypoint FreeEntrypoint;

    pfModuleLoader_ParseConfigurationFromJson ParseConfigurationFromJson;
    pfModuleLoader_FreeConfiguration FreeConfiguration;

    pfModuleLoader_BuildModuleConfiguration BuildModuleConfiguration;
    pfModuleLoader_FreeModuleConfiguration FreeModuleConfiguration;
} MODULE_LOADER_API;

/**
 * Module loader type enumeration.
 */
typedef enum MODULE_LOADER_TYPE_TAG
{
    UNKNOWN,
    NATIVE,
    JAVA,
    DOTNET,
    NODEJS
} MODULE_LOADER_TYPE;

/**
 * The Module Loader.
 */
typedef struct MODULE_LOADER_TAG
{
    MODULE_LOADER_TYPE                  type;
    const char*                         name;
    MODULE_LOADER_BASE_CONFIGURATION*   configuration;
    MODULE_LOADER_API*                  api;
} MODULE_LOADER;

#define MODULE_LOADER_RESULT_VALUES \
    MODULE_LOADER_SUCCESS, \
    MODULE_LOADER_ERROR

DEFINE_ENUM(MODULE_LOADER_RESULT, MODULE_LOADER_RESULT_VALUES);

/**
 * Utility function for parsing a JSON object that has a property called
 * "binding.path" into a MODULE_LOADER_BASE_CONFIGURATION instance.
 */
MODULE_LOADER_RESULT ModuleLoader_ParseBaseConfigurationFromJson(
    MODULE_LOADER_BASE_CONFIGURATION* configuration,
    const JSON_Value* json
);

/**
 * Utility function for freeing a MODULE_LOADER_BASE_CONFIGURATION* instance
 * from a previous call to ModuleLoader_ParseBaseConfigurationFromJson.
 */
void ModuleLoader_FreeBaseConfiguration(MODULE_LOADER_BASE_CONFIGURATION* configuration);

/**
 * This function creates the default set of module loaders that the gateway supports.
 */
MODULE_LOADER_RESULT ModuleLoader_Initialize(void);

/**
 * This function frees resources allocated for tracking module loaders.
 */
void ModuleLoader_Destroy(void);

/**
 * Adds a new module loader to the gateway's collection of module loaders.
 */
MODULE_LOADER_RESULT ModuleLoader_Add(const MODULE_LOADER* loader);

/**
 * Safely replaces the module loader configuration for the given loader.
 */
MODULE_LOADER_RESULT ModuleLoader_UpdateConfiguration(
    MODULE_LOADER* loader,
    MODULE_LOADER_BASE_CONFIGURATION* configuration
);

/**
 * Searches the module loader collection given the loader's name.
 */
MODULE_LOADER* ModuleLoader_FindByName(const char* name);

/**
 * Given a module loader type - returns the default loader.
 */
MODULE_LOADER* ModuleLoader_GetDefaultLoaderForType(MODULE_LOADER_TYPE type);

/**
 * Given a string representation of a module loader type, returns the corresponding
 * enum value.
 */
MODULE_LOADER_TYPE ModuleLoader_ParseType(const char* type);

/**
 * Given a module name, determines if it is a default module loader or a custom one.
 */
bool ModuleLoader_IsDefaultLoader(const char* name);

/**
 * Updates the global loaders array from a JSON that looks like this:
 *   "loaders": [
 *       {
 *           "type": "node",
 *           "name": "node",
 *           "configuration": {
 *               "binding.path": "./bin/libnode_binding.so"
 *           }
 *       },
 *       {
 *           "type": "java",
 *           "name": "java",
 *           "configuration": {
 *               "jvm.options": {
 *                   "memory": 1073741824
 *               },
 *               "gateway.class.path": "./bin/gateway.jar",
 *               "binding.path": "./bin/libjava_binding.so"
 *           }
 *       },
 *       {
 *           "type": "dotnet",
 *           "name": "dotnet",
 *           "configuration": {
 *               "gateway.assembly.path": "./bin/gateway.dll",
 *               "binding.path": "./bin/libdotnet_binding.so"
 *           }
 *       }
 *   ]
 */
MODULE_LOADER_RESULT ModuleLoader_InitializeFromJson(const JSON_Value* loaders);

#ifdef __cplusplus
}
#endif

#endif // MODULE_LOADER_H
