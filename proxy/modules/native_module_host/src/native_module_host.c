// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdlib.h>

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/macro_utils.h"

#include "gateway.h"
#include "broker.h"
#include "module_loader.h"
#include "message.h"
#include "experimental/event_system.h"
#include "module.h"
#include "module_access.h"
#include "module_loaders/dynamic_loader.h"
#include "native_module_host.h"
#include "parson.h"


#define LOADER_NAME_KEY "name"
#define LOADER_ENTRYPOINT_KEY "entrypoint"


typedef struct MODULE_HOST_TAG
{
    MODULE_LIBRARY_HANDLE module_library_handle;
    const MODULE_LOADER* module_loader;
    MODULE_HANDLE module;
    BROKER_HANDLE module_host_broker;
} MODULE_HOST;

static void* NativeModuleHost_ParseConfigurationFromJson(const char* configuration)
{
    char* config_str;
    if (configuration == NULL)
    {
        /*Codes_SRS_NATIVEMODULEHOST_17_003: [ NativeModuleHost_ParseConfigurationFromJson shall return NULL if configuration is NULL. ]*/
        LogError("NULL configuration given, this isn't correct for Native Module Host.");
        config_str = NULL;
    }
    else
    {
        /*Codes_SRS_NATIVEMODULEHOST_17_004: [ On success, NativeModuleHost_ParseConfigurationFromJson shall return a valid copy of the configuration string given. ]*/
        if (mallocAndStrcpy_s(&config_str, configuration) != 0)
        {
            /*Codes_SRS_NATIVEMODULEHOST_17_005: [ NativeModuleHost_ParseConfigurationFromJson shall return NULL if any system call fails. ]*/
            LogError("Unable to copy the configuration, returning NULL");
            config_str = NULL;
        }
    }
    return config_str;
}

static void NativeModuleHost_FreeConfiguration(void* configuration)
{
    /*Codes_SRS_NATIVEMODULEHOST_17_006: [ NativeModuleHost_FreeConfiguration shall do nothing if passed a NULL configuration. ]*/
    if (configuration != NULL)
    {
        /*Codes_SRS_NATIVEMODULEHOST_17_007: [ NativeModuleHost_FreeConfiguration shall free configuration. ]*/
        free((char*)configuration);
    }
}

static int parse_loader(JSON_Object* loader_json, GATEWAY_MODULE_LOADER_INFO* loader_info)
{
    int result;

    // get loader name; we assume it is "native" if the JSON doesn't have a name
    /*Codes_SRS_NATIVEMODULEHOST_17_013: [ NativeModuleHost_Create shall get "name" string from the "outprocess.loader" object. ]*/
    const char* loader_name = json_object_get_string(loader_json, LOADER_NAME_KEY);
    if (loader_name == NULL)
    {
        /*Codes_SRS_NATIVEMODULEHOST_17_014: [ If the loader name string is not present, then NativeModuleHost_Create shall assume the loader is the native dynamic library loader name. ]*/
        loader_name = DYNAMIC_LOADER_NAME;
    }

    // locate the loader
    /*Codes_SRS_NATIVEMODULEHOST_17_015: [ NativeModuleHost_Create shall find the module loader by name. ]*/
    const MODULE_LOADER* loader = ModuleLoader_FindByName(loader_name);
    if (loader == NULL)
    {
        /*Codes_SRS_NATIVEMODULEHOST_17_026: [ If any step above fails, then NativeModuleHost_Create shall free all resources allocated and return NULL. ]*/
        LogError("Loader JSON has a non-existent loader 'name' specified - %s.", loader_name);
        result = __LINE__;
    }
    else
    {
        loader_info->loader = loader;

        // get entrypoint
        /*Codes_SRS_NATIVEMODULEHOST_17_016: [ NativeModuleHost_Create shall get "entrypoint" string from the "outprocess.loader" object. ]*/
        JSON_Value* entrypoint_json = json_object_get_value(loader_json, LOADER_ENTRYPOINT_KEY);
        if (entrypoint_json == NULL)
        {
            /*Codes_SRS_NATIVEMODULEHOST_17_026: [ If any step above fails, then NativeModuleHost_Create shall free all resources allocated and return NULL. ]*/
            LogError("An error occurred when getting the entrypoint for loader - %s.", loader_name);
            result = __LINE__;
        }
        else
        {
            /*Codes_SRS_NATIVEMODULEHOST_17_017: [ NativeModuleHost_Create shall parse the entrypoint string. ]*/
            loader_info->entrypoint = loader->api->ParseEntrypointFromJson(loader, entrypoint_json);
            if (loader_info->entrypoint == NULL)
            {
                /*Codes_SRS_NATIVEMODULEHOST_17_026: [ If any step above fails, then NativeModuleHost_Create shall free all resources allocated and return NULL. ]*/
                LogError("An error occurred when parsing the entrypoint for loader - %s.", loader_name);
                result = __LINE__;
            }
            else
            {
                result = 0;
            }
        }
    }
    return result;
}

static int module_create(MODULE_HOST * module_host, BROKER_HANDLE broker, GATEWAY_MODULE_LOADER_INFO * gw_loader_info, const char * remote_module_args)
{
    int result;
    if (remote_module_args == NULL)
    {
        /*Codes_SRS_NATIVEMODULEHOST_17_026: [ If any step above fails, then NativeModuleHost_Create shall free all resources allocated and return NULL. ]*/
        LogError("An error occurred getting module args for module");
        result = __LINE__;
    }
    else
    {
        /*Codes_SRS_NATIVEMODULEHOST_17_019: [ NativeModuleHost_Create shall load the module library with the pasred entrypoint. ]*/
        module_host->module_library_handle = gw_loader_info->loader->api->Load(
            gw_loader_info->loader,
            gw_loader_info->entrypoint);
        if (module_host->module_library_handle == NULL)
        {
            /*Codes_SRS_NATIVEMODULEHOST_17_026: [ If any step above fails, then NativeModuleHost_Create shall free all resources allocated and return NULL. ]*/
            result = __LINE__;
        }
        else
        {
            MODULE_LOADER_API* loader_api = gw_loader_info->loader->api;
            /*Codes_SRS_NATIVEMODULEHOST_17_020: [ NativeModuleHost_Create shall get the MODULE_API pointer from the loader. ]*/
            const MODULE_API* module_apis = loader_api->GetApi(gw_loader_info->loader, module_host->module_library_handle);
            if (module_apis == NULL)
            {
                /*Codes_SRS_NATIVEMODULEHOST_17_026: [ If any step above fails, then NativeModuleHost_Create shall free all resources allocated and return NULL. ]*/
                loader_api->Unload(gw_loader_info->loader, module_host->module_library_handle);
                result = __LINE__;
            }
            else
            {
                /*Codes_SRS_NATIVEMODULEHOST_17_021: [ NativeModuleHost_Create shall call the module's _ParseConfigurationFromJson on the "module.args" object. ]*/
                const void* module_configuration = MODULE_PARSE_CONFIGURATION_FROM_JSON(module_apis)(remote_module_args);
                /*Codes_SRS_NATIVEMODULEHOST_17_022: [ NativeModuleHost_Create shall build the module confguration from the parsed entrypoint and parsed module arguments. ]*/
                const void* transformed_module_configuration = loader_api->BuildModuleConfiguration(
                    gw_loader_info->loader,
                    gw_loader_info->entrypoint,
                    module_configuration
                );
                /*Codes_SRS_NATIVEMODULEHOST_17_023: [ NativeModuleHost_Create shall call the module's _Create on the built module configuration. ]*/
                module_host->module = MODULE_CREATE(module_apis)(broker, transformed_module_configuration);
                if (module_host->module == NULL)
                {
                    /*Codes_SRS_NATIVEMODULEHOST_17_026: [ If any step above fails, then NativeModuleHost_Create shall free all resources allocated and return NULL. ]*/
                    loader_api->Unload(gw_loader_info->loader, module_host->module_library_handle);
                    result = __LINE__;
                }
                else
                {
                    /*Codes_SRS_NATIVEMODULEHOST_17_025: [ NativeModuleHost_Create shall return a non-null pointer to a MODULE_HANDLE on success. ]*/
                    module_host->module_loader = gw_loader_info->loader;
                    module_host->module_host_broker = broker;
                        
                    result = 0;
                }
                /*Codes_SRS_NATIVEMODULEHOST_17_024: [ NativeModuleHost_Create shall free all resources used during module loading. ]*/
                MODULE_FREE_CONFIGURATION(module_apis)((void*)module_configuration);
                loader_api->FreeModuleConfiguration(gw_loader_info->loader, transformed_module_configuration);
            }
        }
    }
    return result;
}

static MODULE_HANDLE NativeModuleHost_Create(BROKER_HANDLE broker, const void* configuration)
{
    MODULE_HOST * result;
    if (broker == NULL || configuration == NULL)
    {
        /*Codes_SRS_NATIVEMODULEHOST_17_008: [ NativeModuleHost_Create shall return NULL if broker is NULL. ]*/
        /*Codes_SRS_NATIVEMODULEHOST_17_009: [ NativeModuleHost_Create shall return NULL if configuration does not contain valid JSON. ]*/
        LogError("broker [%p] or configuration [%p] is NULL, both are required", broker, configuration);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_NATIVEMODULEHOST_17_010: [ NativeModuleHost_Create shall intialize the Module_Loader. ]*/
        if (ModuleLoader_Initialize() != MODULE_LOADER_SUCCESS)
        {
            /*Codes_SRS_NATIVEMODULEHOST_17_026: [ If any step above fails, then NativeModuleHost_Create shall free all resources allocated and return NULL. ]*/
            LogError("ModuleLoader_Initialize failed");
            result = NULL;
        }
        else
        {
            /*Codes_SRS_NATIVEMODULEHOST_17_011: [ NativeModuleHost_Create shall parse the configuration JSON. ]*/
            char * outprocess_module_args = (char *)configuration;
            JSON_Value *root_value = json_parse_string(outprocess_module_args);
            JSON_Object * module_host_args;
            if ((root_value == NULL) ||
                ((module_host_args = json_value_get_object(root_value)) == NULL))
            {
                if (root_value != NULL)
                {
                    json_value_free(root_value);
                }
                LogError("NativeModuleHost_Create could not parse arguments as JSON");
                result = NULL;
            }
            else
            {
                /*Codes_SRS_NATIVEMODULEHOST_17_035: [ If the "outprocess.loaders" array exists in the configuration JSON, NativeModuleHost_Create shall initialize the Module_Loader from this array. ]*/
                JSON_Value * loaders_array = json_object_get_value(module_host_args, OOP_MODULE_LOADERS_ARRAY_KEY);
                if ((loaders_array != NULL) &&
                    (ModuleLoader_InitializeFromJson(loaders_array) != MODULE_LOADER_SUCCESS))
                {
                    LogError("NativeModuleHost_Create could not extract loaders array from module arguments.");
                    result = NULL;
                }
                else
                {
                    /*Codes_SRS_NATIVEMODULEHOST_17_012: [ NativeModuleHost_Create shall get the "outprocess.loader" object from the configuration JSON. ]*/
                    JSON_Object * loader_args = json_object_get_object(module_host_args, OOP_MODULE_LOADER_KEY);
                    if (loader_args == NULL)
                    {
                        LogError("NativeModuleHost_Create could not get loader arguments.");
                        result = NULL;
                    }
                    else
                    {
                        GATEWAY_MODULE_LOADER_INFO loader_info;
                        if (parse_loader(loader_args, &loader_info) != 0)
                        {
                            /*Codes_SRS_NATIVEMODULEHOST_17_026: [ If any step above fails, then NativeModuleHost_Create shall free all resources allocated and return NULL. ]*/
                            LogError("NativeModuleHost_Create could not extract loader information from loader arguments.");
                            result = NULL;
                        }
                        else
                        {
                            // Have loader and entrypoint now, get module.
                            result = (MODULE_HOST*)malloc(sizeof(MODULE_HOST));
							if (result == NULL)
							{
								LogError("NativeModuleHost_Create could not allocate module.");
								result = NULL;
							}
							else
							{
								/*Codes_SRS_NATIVEMODULEHOST_17_018: [ NativeModuleHost_Create shall get the "module.args" object from the configuration JSON. ]*/
								JSON_Value * module_args = json_object_get_value(module_host_args, OOP_MODULE_ARGS_KEY);
								char * module_args_string = json_serialize_to_string(module_args);
								if (module_create(result, broker, &loader_info, module_args_string) != 0)
								{
									/*Codes_SRS_NATIVEMODULEHOST_17_026: [ If any step above fails, then NativeModuleHost_Create shall free all resources allocated and return NULL. ]*/
									LogError("NativeModuleHost_Create could not load module.");
									free(result);
									result = NULL;
								}
								/*Codes_SRS_NATIVEMODULEHOST_17_024: [ NativeModuleHost_Create shall free all resources used during module loading. ]*/
								json_free_serialized_string(module_args_string);
							}
                            loader_info.loader->api->FreeEntrypoint(loader_info.loader, loader_info.entrypoint);
                        }
                    }
                }
                /*Codes_SRS_NATIVEMODULEHOST_17_024: [ NativeModuleHost_Create shall free all resources used during module loading. ]*/
                json_value_free(root_value);
            }
            if (result == NULL)
            {
                // failed to create a module, give up entirely.
                /*Codes_SRS_NATIVEMODULEHOST_17_026: [ If any step above fails, then NativeModuleHost_Create shall free all resources allocated and return NULL. ]*/
                ModuleLoader_Destroy();
            }
        }
    }
    return result;
}

static void NativeModuleHost_Destroy(MODULE_HANDLE moduleHandle)
{
    if (moduleHandle != NULL)
    {
        MODULE_HOST* module_host = (MODULE_HOST*)moduleHandle;
        if (module_host->module != NULL)
        {
            const MODULE_LOADER* module_loader = module_host->module_loader;
            MODULE_LIBRARY_HANDLE module_library = module_host->module_library_handle;

            if (module_loader != NULL)
            {
                MODULE_LOADER_API * loader_api = module_loader->api;
                if ((module_library != NULL) && (loader_api != NULL))
                {
                    const MODULE_API* module_apis = loader_api->GetApi(module_loader, module_library);
                    if (MODULE_DESTROY(module_apis) != NULL)
                    {
                        /*Codes_SRS_NATIVEMODULEHOST_17_028: [ NativeModuleHost_Destroy shall free all remaining allocated resources if moduleHandle is not NULL. ]*/
                        MODULE_DESTROY(module_apis)(module_host->module);
                    }

                    loader_api->Unload(module_loader, module_library);
                }
            }
            module_host->module = NULL;
            module_host->module_library_handle = NULL;
            module_host->module_host_broker = NULL;
            free(module_host);
        }
    }
    /*Codes_SRS_NATIVEMODULEHOST_17_027: [ NativeModuleHost_Destroy shall always destroy the module loader. ]*/
    ModuleLoader_Destroy();
}

static void NativeModuleHost_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    if (moduleHandle != NULL)
    {
        MODULE_HOST* module_host = (MODULE_HOST*)moduleHandle;
        /*Codes_SRS_NATIVEMODULEHOST_17_030: [ NativeModuleHost_Receive shall get the loaded module's MODULE_API pointer. ]*/
        const MODULE_API* module_apis = module_host->module_loader->api->GetApi(module_host->module_loader, module_host->module_library_handle);
        if (module_apis != NULL)
        {
            pfModule_Receive pfReceive = MODULE_RECEIVE(module_apis);
            if (pfReceive != NULL)
            {
                /*Codes_SRS_NATIVEMODULEHOST_17_031: [ NativeModuleHost_Receive shall call the loaded module's _Receive function, passing the messageHandle along. ]*/
                (pfReceive)(module_host->module, messageHandle);
            }
            else
            {
                LogError("Module API did not have a Receive function");
            }
        }
        else
        {
            LogError("Module API not found");
        }
    }
    else
    {
        /*Codes_SRS_NATIVEMODULEHOST_17_029: [ NativeModuleHost_Receive shall do nothing if moduleHandle is NULL. ]*/
        LogError("Given module handle is NULL");
    }
}

static void NativeModuleHost_Start(MODULE_HANDLE moduleHandle)
{
    if (moduleHandle != NULL)
    {
        MODULE_HOST* module_host = (MODULE_HOST*)moduleHandle;
        /*Codes_SRS_NATIVEMODULEHOST_17_033: [ NativeModuleHost_Start shall get the loaded module's MODULE_API pointer. ]*/
        const MODULE_API* module_apis = module_host->module_loader->api->GetApi(module_host->module_loader, module_host->module_library_handle);
        if (module_apis != NULL)
        {
            pfModule_Start pfStart = MODULE_START(module_apis);
            /* it's not an error if the module doen't have a start function */
            if (pfStart != NULL)
            {
                /*Codes_SRS_NATIVEMODULEHOST_17_034: [ NativeModuleHost_Start shall call the loaded module's _Start function, if defined. ]*/
                (pfStart)(module_host->module);
            }
        }
        else
        {
            LogError("Module API not found");
        }
    }
    else
    {
        /*Codes_SRS_NATIVEMODULEHOST_17_032: [ NativeModuleHost_Start shall do nothing if moduleHandle is NULL. ]*/
        LogError("Given module handle is NULL");
    }
}

static const MODULE_API_1 NATIVEMODULEHOST_APIS_all =
{
    {MODULE_API_VERSION_1},

    NativeModuleHost_ParseConfigurationFromJson,
    NativeModuleHost_FreeConfiguration,
    NativeModuleHost_Create,
    NativeModuleHost_Destroy,
    NativeModuleHost_Receive,
    NativeModuleHost_Start
};


#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(OOP_MODULE_HOST)(MODULE_API_VERSION gateway_api_version)
#else
MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version)
#endif
{
    /*Codes_SRS_NATIVEMODULEHOST_17_001: [ Module_GetApi shall return a valid pointer to a MODULE_API structure. ]*/
    /*Codes_SRS_NATIVEMODULEHOST_17_002: [ The MODULE_API structure returned shall have valid pointers for all pointers in the strcuture. ]*/
    (void)gateway_api_version;
    return (const MODULE_API *)&NATIVEMODULEHOST_APIS_all;
}
