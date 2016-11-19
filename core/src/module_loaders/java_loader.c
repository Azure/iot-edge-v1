// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <stdarg.h>
#include "azure_c_shared_utility/gballoc.h"
#include <string.h>

#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/strings.h"
#include "parson.h"

#include "module.h"
#include "module_access.h"
#include "dynamic_library.h"
#include "module_loader.h"
#include "module_loaders/java_loader.h"
#include "java_module_host.h"

#ifdef UNDER_TEST
#undef ENV_VARS
#define ENV_VARS
#undef PREFIXES
#define PREFIXES
#endif

#define CONFIGURATION         ((JAVA_LOADER_CONFIGURATION*)(loader->configuration))

typedef struct JAVA_MODULE_HANDLE_DATA_TAG
{
    DYNAMIC_LIBRARY_HANDLE      binding_module;
    const MODULE_API*           api;
} JAVA_MODULE_HANDLE_DATA;

static JVM_OPTIONS* parse_jvm_config(JSON_Object*);
static void free_jvm_config(JVM_OPTIONS*);
static DYNAMIC_LIBRARY_HANDLE system_load_env(int, ...);
static DYNAMIC_LIBRARY_HANDLE system_load_prefixes(int, ...);
static int set_default_paths(JVM_OPTIONS*);
static JAVA_LOADER_CONFIGURATION* create_default_config();

static DYNAMIC_LIBRARY_HANDLE JavaModuleLoader_LoadBindingModule(const MODULE_LOADER* loader)
{
    DYNAMIC_LIBRARY_HANDLE result;
    /*Codes_SRS_JAVA_MODULE_LOADER_14_005: [JavaModuleLoader_Load shall use the binding module path given in loader->configuration->binding_path if loader->configuration is not NULL.]*/
    if (loader == NULL)
    {
        result = NULL;
        LogError("Invalid inputs - loader = %p", loader);
    }
    else
    {
        const char* binding_path = JAVA_BINDING_MODULE_NAME;
        if (CONFIGURATION != NULL && CONFIGURATION->base.binding_path != NULL)
        {
            binding_path = STRING_c_str(CONFIGURATION->base.binding_path);
        }
        result = DynamicLibrary_LoadLibrary(binding_path);

        if (result == NULL)
        {
            //If the binding could not be loaded, check the system locations
            result = system_load_env(COUNT_ARG(ENV_VARS)-1 ENV_VARS);

            if (result == NULL)
            {
                result = system_load_prefixes(COUNT_ARG(PREFIXES)-1 PREFIXES);
                LogError("The Java binding module could not be located.");
            }
        }

    }

    return result;
}

static MODULE_LIBRARY_HANDLE JavaModuleLoader_Load(const MODULE_LOADER* loader, const void* entrypoint)
{
    JAVA_MODULE_HANDLE_DATA* result;

    /*Codes_SRS_JAVA_MODULE_LOADER_14_001: [JavaModuleLoader_Load shall return NULL if loader is NULL.]*/
    if (loader == NULL)
    {
        result = NULL;
        LogError("Invalid inputs - loader = %p", loader);
    }
    else
    {
        /*Codes_SRS_JAVA_MODULE_LOADER_14_002: [JavaModuleLoader_Load shall return NULL if loader->type is not JAVA.]*/
        if (loader->type != JAVA)
        {
            result = NULL;
            LogError("loader->type is not JAVA");
        }
        else
        {
            result = (JAVA_MODULE_HANDLE_DATA*)malloc(sizeof(JAVA_MODULE_HANDLE_DATA));

            /*Codes_SRS_JAVA_MODULE_LOADER_14_003: [JavaModuleLoader_Load shall return NULL if an underlying platform call fails.]*/
            if (result == NULL)
            {
                LogError("malloc returned NULL");
            }
            else
            {
                /*Codes_SRS_JAVA_MODULE_LOADER_14_004: [JavaModuleLoader_Load shall load the binding module library into memory by calling DynamicLibrary_LoadLibrary.]*/
                result->binding_module = JavaModuleLoader_LoadBindingModule(loader);
                if (result->binding_module == NULL)
                {
                    free(result);
                    result = NULL;
                    LogError("JavaModuleLoader_LoadBindingModule returned NULL");
                }
                else
                {
                    /*Codes_SRS_JAVA_MODULE_LOADER_14_006: [JavaModuleLoader_Load shall call DynamicLibrary_FindSymbol on the binding module handle with the symbol name Module_GetApi to acquire the module's API table. ]*/
                    pfModule_GetApi pfnGetAPI = (pfModule_GetApi)DynamicLibrary_FindSymbol(result->binding_module, MODULE_GETAPI_NAME);
                    if (pfnGetAPI == NULL)
                    {
                        DynamicLibrary_UnloadLibrary(result->binding_module);
                        free(result);
                        result = NULL;
                        LogError("DynamicLibrary_FindSymbol() returned NULL");
                    }
                    else
                    {
                        result->api = pfnGetAPI(Module_ApiGatewayVersion);

                        /*Codes_SRS_JAVA_MODULE_LOADER_14_008: [JavaModuleLoader_Load shall return NULL if MODULE_API returned by the binding module is NULL.]*/
                        /*Codes_SRS_JAVA_MODULE_LOADER_14_009: [JavaModuleLoader_Load shall return NULL if MODULE_API::version is great than Module_ApiGatewayVersion.]*/
                        /*Codes_SRS_JAVA_MODULE_LOADER_14_010: [JavaModuleLoader_Load shall return NULL if the Module_Create function in MODULE_API is NULL.]*/
                        /*Codes_SRS_JAVA_MODULE_LOADER_14_011: [JavaModuleLoader_Load shall return NULL if the Module_Receive function in MODULE_API is NULL.]*/
                        /*Codes_SRS_JAVA_MODULE_LOADER_14_012: [JavaModuleLoader_Load shall return NULL if the Module_Destroy function in MODULE_API is NULL.]*/
                        if (result->api == NULL ||
                            result->api->version > Module_ApiGatewayVersion ||
                            MODULE_CREATE(result->api) == NULL ||
                            MODULE_DESTROY(result->api) == NULL ||
                            MODULE_RECEIVE(result->api) == NULL)
                        {
                            DynamicLibrary_UnloadLibrary(result->binding_module);
                            free(result);
                            result = NULL;
                            LogError("pfnGetAPI() failed.");
                        }
                    }
                }
            }
        }
    }

    /*Codes_SRS_JAVA_MODULE_LOADER_14_007: [JavaModuleLoader_Load shall return a non - NULL pointer of type MODULE_LIBRARY_HANDLE when successful.]*/
    return result;
}

static const MODULE_API* JavaModuleLoader_GetModuleApi(const MODULE_LOADER* loader, MODULE_LIBRARY_HANDLE moduleLibraryHandle)
{
    (void)loader;

    /*Codes_SRS_JAVA_MODULE_LOADER_14_013: [JavaModuleLoader_GetModuleApi shall return NULL if moduleLibraryHandle is NULL.]*/
    const MODULE_API* result;
    if (moduleLibraryHandle == NULL)
    {
        result = NULL;
        LogError("moduleLibraryHandle is NULL");
    }
    else
    {
        /*Codes_SRS_JAVA_MODULE_LOADER_14_014: [JavaModuleLoader_GetModuleApi shall return a non - NULL MODULE_API pointer when successful.]*/
        JAVA_MODULE_HANDLE_DATA* loader_data = moduleLibraryHandle;
        result = loader_data->api;
    }

    return result;
}

static void JavaModuleLoader_Unload(const MODULE_LOADER* loader, MODULE_LIBRARY_HANDLE moduleLibraryHandle)
{
    (void)loader;

    /*Codes_SRS_JAVA_MODULE_LOADER_14_015: [JavaModuleLoader_Unload shall do nothing if moduleLibraryHandle is NULL.]*/
    if (moduleLibraryHandle == NULL)
    {
        LogError("moduleLibraryHandle is NULL.");
    }
    else
    {
        /*Codes_SRS_JAVA_MODULE_LOADER_14_016: [JavaModuleLoader_Unload shall unload the binding module from memory by calling DynamicLibrary_UnloadLibrary.]*/
        JAVA_MODULE_HANDLE_DATA* loader_data = moduleLibraryHandle;
        DynamicLibrary_UnloadLibrary(loader_data->binding_module);

        /*Codes_SRS_JAVA_MODULE_LOADER_14_017: [JavaModuleLoader_Unload shall free resources allocated when loading the binding module.]*/
        free(loader_data);
    }
}

static void* JavaModuleLoader_ParseEntrypointFromJson(const MODULE_LOADER* loader, const JSON_Value* json)
{
    JAVA_LOADER_ENTRYPOINT* entrypoint;

    /*Codes_SRS_JAVA_MODULE_LOADER_14_043: [ JavaModuleLoader_ParseEntrypointFromJson shall return NULL if loader is NULL. ]*/
    if (loader == NULL)
    {
        entrypoint = NULL;
        LogError("'loader' is NULL.");
    }
    else
    {
        /*Codes_SRS_JAVA_MODULE_LOADER_14_018: [JavaModuleLoader_ParseEntrypointFromJson shall return NULL if json is NULL.]*/
        if (json == NULL)
        {
            entrypoint = NULL;
            LogError("'json' is NULL");
        }
        else
        {
            /*Codes_SRS_JAVA_MODULE_LOADER_14_059: [JavaModuleLoader_ParseEntrypointFromJson shall return NULL if loader->configuration.]*/
            /*Codes_SRS_JAVA_MODULE_LOADER_14_060: [JavaModuleLoader_ParseEntrypointFromJson shall return NULL if loader->configuration->options is NULL.]*/
            if (CONFIGURATION == NULL || CONFIGURATION->options == NULL)
            {
                entrypoint = NULL;
                LogError("'loader->configuration' is NULL or 'loader->configuration->options' is NULL.");
            }
            else
            {
                /*Codes_SRS_JAVA_MODULE_LOADER_14_019: [JavaModuleLoader_ParseEntrypointFromJson shall return NULL if the root json entity is not an object.]*/
                if (json_value_get_type(json) != JSONObject)
                {
                    entrypoint = NULL;
                    LogError("'json' is not an object.");
                }
                else
                {
                    JSON_Object* entrypoint_json = json_value_get_object(json);
                    if (entrypoint_json == NULL)
                    {
                        entrypoint = NULL;
                        LogError("json_value_get_object failed.");
                    }
                    else
                    {
                        /*Codes_SRS_JAVA_MODULE_LOADER_14_021: [JavaModuleLoader_ParseEntrypointFromJson shall retreive the fully qualified class name by reading the value of the attribute class.name.]*/
                        const char* className = json_object_get_string(entrypoint_json, ENTRYPOINT_CLASSNAME);
                        if (className == NULL)
                        {
                            /*Codes_SRS_JAVA_MODULE_LOADER_14_058: [ JavaModuleLoader_ParseEntrypointFromJson shall return NULL if either class.name or class.path is non-existent. ]*/
                            entrypoint = NULL;
                            LogError("json_object_get_string for '%s' failed.", ENTRYPOINT_CLASSNAME);
                        }
                        else
                        {
                            /*Codes_SRS_JAVA_MODULE_LOADER_14_022: [JavaModuleLoader_ParseEntrypointFromJson shall retreive the full class path containing the class definition for the module by reading the value of the attribute class.path.]*/
                            const char* classPath = json_object_get_string(entrypoint_json, ENTRYPOINT_CLASSPATH);
                            if (classPath == NULL)
                            {
                                /*Codes_SRS_JAVA_MODULE_LOADER_14_058: [ JavaModuleLoader_ParseEntrypointFromJson shall return NULL if either class.name or class.path is non-existent. ]*/
                                entrypoint = NULL;
                                LogError("json_object_get_string for '%s' failed.", ENTRYPOINT_CLASSPATH);
                            }
                            else
                            {
                                /*Codes_SRS_JAVA_MODULE_LOADER_14_023: [JavaModuleLoader_ParseEntrypointFromJson shall return a non - NULL pointer to the parsed representation of the entrypoint when successful.]*/
                                entrypoint = (JAVA_LOADER_ENTRYPOINT*)malloc(sizeof(JAVA_LOADER_ENTRYPOINT));
                                if (entrypoint == NULL)
                                {
                                    /*Codes_SRS_JAVA_MODULE_LOADER_14_020: [JavaModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails.]*/
                                    LogError("malloc failed.");
                                }
                                else
                                {
                                    entrypoint->className = STRING_construct(className);
                                    if (entrypoint->className == NULL)
                                    {
                                        /*Codes_SRS_JAVA_MODULE_LOADER_14_020: [JavaModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails.]*/
                                        free(entrypoint);
                                        entrypoint = NULL;
                                        LogError("STRING_construct for %s failed.", className);
                                    }
                                    else
                                    {
                                        entrypoint->classPath = STRING_construct(classPath);
                                        if (entrypoint->classPath == NULL)
                                        {
                                            /*Codes_SRS_JAVA_MODULE_LOADER_14_020: [JavaModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails.]*/
                                            STRING_delete(entrypoint->className);
                                            free(entrypoint);
                                            entrypoint = NULL;
                                            LogError("STRING_construct for %s failed.", classPath);
                                        }
                                        else
                                        {
                                            /*Codes_SRS_JAVA_MODULE_LOADER_14_044: [ JavaModuleLoader_ParseEntrypointFromJson shall append the classpath to the loader's configuration JVM_OPTIONS class_path member.]*/
                                            STRING_HANDLE cp = STRING_construct(CONFIGURATION->options->class_path);
                                            if (cp == NULL)
                                            {
                                                STRING_delete(entrypoint->className);
                                                STRING_delete(entrypoint->classPath);
                                                free(entrypoint);
                                                entrypoint = NULL;
                                                LogError("Unable to construct a string from the JVM_OPTIONS classpath");
                                            }
                                            else
                                            {
                                                if (STRING_concat(cp, SEPARATOR) != 0)
                                                {
                                                    STRING_delete(entrypoint->className);
                                                    STRING_delete(entrypoint->classPath);
                                                    free(entrypoint);
                                                    entrypoint = NULL;
                                                    LogError("STRING_concat failed");
                                                }
                                                else
                                                {
                                                    if (STRING_concat_with_STRING(cp, entrypoint->classPath) != 0)
                                                    {
                                                        STRING_delete(entrypoint->className);
                                                        STRING_delete(entrypoint->classPath);
                                                        free(entrypoint);
                                                        entrypoint = NULL;
                                                        LogError("STRING_concat_with_STRING failed");
                                                    }
                                                    else
                                                    {
                                                        const char* str = STRING_c_str(cp);
                                                        if (str == NULL)
                                                        {
                                                            STRING_delete(entrypoint->className);
                                                            STRING_delete(entrypoint->classPath);
                                                            free(entrypoint);
                                                            entrypoint = NULL;
                                                            LogError("STRING_c_str failed");
                                                        }
                                                        else
                                                        {
                                                            //We must first free the class_path here so that the new class path with the appended module class_path can be assigned
                                                            free((char*)(CONFIGURATION->options->class_path));
                                                            if (mallocAndStrcpy_s((char**)(&(CONFIGURATION->options->class_path)), str) != 0)
                                                            {
                                                                CONFIGURATION->options->class_path = NULL;
                                                                STRING_delete(entrypoint->className);
                                                                STRING_delete(entrypoint->classPath);
                                                                free(entrypoint);
                                                                entrypoint = NULL;
                                                                LogError("Failed to allocate class.path");
                                                            }
                                                        }
                                                    }
                                                }
                                                STRING_delete(cp);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return entrypoint;
}

static void JavaModuleLoader_FreeEntrypoint(const MODULE_LOADER* loader, void* entrypoint)
{
    (void)loader;

    /*Codes_SRS_JAVA_MODULE_LOADER_14_024: [JavaModuleLoader_FreeEntrypoint shall do nothing if entrypoint is NULL.]*/
    if (entrypoint == NULL)
    {
        LogError("entrypoint is NULL.");
    }
    else
    {
        /*Codes_SRS_JAVA_MODULE_LOADER_14_025: [JavaModuleLoader_FreeEntrypoint shall free resources allocated during JavaModuleLoader_ParseEntrypointFromJson.]*/
        JAVA_LOADER_ENTRYPOINT* ep = (JAVA_LOADER_ENTRYPOINT*)entrypoint;
        STRING_delete(ep->className);
        STRING_delete(ep->classPath);
        free(ep);
    }
}

static MODULE_LOADER_BASE_CONFIGURATION* JavaModuleLoader_ParseConfigurationFromJson(const MODULE_LOADER* loader, const JSON_Value* json)
{
    (void)loader;

    JAVA_LOADER_CONFIGURATION* config;

    /*Codes_SRS_JAVA_MODULE_LOADER_14_026: [JavaModuleLoader_ParseConfigurationFromJson shall return NULL if json is NULL.]*/
    if (json == NULL)
    {
        LogError("json is NULL.");
        config = NULL;
    }
    else
    {
        /*Codes_SRS_JAVA_MODULE_LOADER_14_027: [JavaModuleLoader_ParseConfigurationFromJson shall return NULL if json is not a valid JSON object.]*/
        JSON_Object* object = json_value_get_object(json);
        if (object == NULL)
        {
            config = NULL;
            LogError("could not retrieve json object.");
        }
        else
        {
            JSON_Object* options_object = json_object_get_object(object, JVM_OPTIONS_KEY);
            if (options_object == NULL)
            {
                config = NULL;
                LogError("could not retrieve json object %s", JVM_OPTIONS_KEY);
            }
            else
            {
                config = (JAVA_LOADER_CONFIGURATION*)malloc(sizeof(JAVA_LOADER_CONFIGURATION));
                if (config == NULL)
                {
                    LogError("malloc failed.");
                }
                else
                {
                    /*Codes_SRS_JAVA_MODULE_LOADER_14_029: [JavaModuleLoader_ParseConfigurationFromJson shall set any missing field to NULL, false, or 0.]*/
                    /*Codes_SRS_JAVA_MODULE_LOADER_14_030: [JavaModuleLoader_ParseConfigurationFromJson shall parse the jvm.options JSON object and initialize a new JAVA_LOADER_CONFIGURATION.]*/
                    /*Codes_SRS_JAVA_MODULE_LOADER_14_031: [JavaModuleLoader_ParseConfigurationFromJson shall parse the jvm.options.library.path.]*/
                    /*Codes_SRS_JAVA_MODULE_LOADER_14_032: [JavaModuleLoader_ParseConfigurationFromJson shall parse the jvm.options.version.]*/
                    /*Codes_SRS_JAVA_MODULE_LOADER_14_033: [JavaModuleLoader_ParseConfigurationFromJson shall parse the jvm.options.debug.]*/
                    /*Codes_SRS_JAVA_MODULE_LOADER_14_034: [JavaModuleLoader_ParseConfigurationFromJson shall parse the jvm.options.debug.port.]*/
                    /*Codes_SRS_JAVA_MODULE_LOADER_14_035: [JavaModuleLoader_ParseConfigurationFromJson shall parse the jvm.options.additional.options object and create a new STRING_HANDLE for each.]*/
                    JVM_OPTIONS* jvm_options = parse_jvm_config(options_object);
                    if (jvm_options == NULL)
                    {
                        /*Codes_SRS_JAVA_MODULE_LOADER_14_036: [JavaModuleLoader_ParseConfigurationFromJson shall return NULL if any present field cannot be parsed.]*/
                        /*Codes_SRS_JAVA_MODULE_LOADER_14_028: [JavaModuleLoader_ParseConfigurationFromJson shall return NULL if any underlying platform call fails.]*/
                        free(config);
                        config = NULL;
                        LogError("could not create the JVM_OPTIONS structure.");
                    }
                    else
                    {
                        /*Codes_SRS_JAVA_MODULE_LOADER_14_038: [JavaModuleLoader_ParseConfigurationFromJson shall set the options member of the JAVA_LOADER_CONFIGURATION to the parsed JVM_OPTIONS structure.]*/
                        ((JAVA_LOADER_CONFIGURATION*)(config))->options = jvm_options;

                        /*Codes_SRS_JAVA_MODULE_LOADER_14_039: [JavaModuleLoader_ParseConfigurationFromJson shall set the base member of the JAVA_LOADER_CONFIGURATION by calling to the base module loader.]*/
                        if (ModuleLoader_ParseBaseConfigurationFromJson(&((JAVA_LOADER_CONFIGURATION*)(config))->base, json) != MODULE_LOADER_SUCCESS)
                        {
                            /*Codes_SRS_JAVA_MODULE_LOADER_14_028: [JavaModuleLoader_ParseConfigurationFromJson shall return NULL if any underlying platform call fails.]*/
                            free_jvm_config(((JAVA_LOADER_CONFIGURATION*)(config))->options);
                            free(config);
                            config = NULL;
                            LogError("ModuleLoader_ParseBaseConfigurationFromJson failed.");
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

    /*Codes_SRS_JAVA_MODULE_LOADER_14_028: [JavaModuleLoader_ParseConfigurationFromJson shall return NULL if any underlying platform call fails.]*/
    /*Codes_SRS_JAVA_MODULE_LOADER_14_037: [JavaModuleLoader_ParseConfigurationFromJson shall return a non - NULL JAVA_LOADER_CONFIGURATION containing all user - specified values.]*/

    return (MODULE_LOADER_BASE_CONFIGURATION*)config;
}

static void JavaModuleLoader_FreeConfiguration(const MODULE_LOADER* loader, MODULE_LOADER_BASE_CONFIGURATION* configuration)
{
    (void)loader;

    if (configuration != NULL)
    {
        /*Codes_SRS_JAVA_MODULE_LOADER_14_041: [JavaModuleLoader_FreeConfiguration shall call ModuleLoader_FreeBaseConfiguration to free resources allocated by ModuleLoader_ParseBaseConfigurationJson.]*/
        ModuleLoader_FreeBaseConfiguration(configuration);

        /*Codes_SRS_JAVA_MODULE_LOADER_14_042: [JavaModuleLoader_FreeConfiguration shall free resources allocated by JavaModuleLoader_ParseConfigurationFromJson.]*/
        free_jvm_config(((JAVA_LOADER_CONFIGURATION*)configuration)->options);
        free(configuration);
    }
    else
    {
        /*Codes_SRS_JAVA_MODULE_LOADER_14_040: [JavaModuleLoader_FreeConfiguration shall do nothing if configuration is NULL.]*/
        LogError("configuration is NULL");
    }
}

static void* JavaModuleLoader_BuildModuleConfiguration(const MODULE_LOADER* loader, const void* entrypoint, const void* module_configuration)
{
    JAVA_MODULE_HOST_CONFIG* result;

    /*Codes_SRS_JAVA_MODULE_LOADER_14_045: [JavaModuleLoader_BuildModuleConfiguration shall return NULL if entrypoint is NULL.]*/
    /*Codes_SRS_JAVA_MODULE_LOADER_14_046: [JavaModuleLoader_BuildModuleConfiguration shall return NULL if entrypoint->className is NULL.]*/
    if (entrypoint == NULL || ((JAVA_LOADER_ENTRYPOINT*)entrypoint)->className == NULL)
    {
        result = NULL;
        LogError("entrypoint or entrypoint->className is NULL.");
    }
    else
    {
        /*Codes_SRS_JAVA_MODULE_LOADER_14_047: [JavaModuleLoader_BuildModuleConfiguration shall return NULL if loader is NULL.]*/
        /*Codes_SRS_JAVA_MODULE_LOADER_14_048: [JavaModuleLoader_BuildModuleConfiguration shall return NULL if loader->options is NULL.]*/
        if (loader == NULL || CONFIGURATION->options == NULL)
        {
            result = NULL;
            LogError("loader or loader options is NULL");
        }
        else
        {
            /*Codes_SRS_JAVA_MODULE_LOADER_14_050: [JavaModuleLoader_BuildModuleConfiguration shall build a JAVA_MODULE_HOST_CONFIG object by copying information from entrypoint, module_configuration, and loader->options and return a non - NULL pointer.]*/
            result = (JAVA_MODULE_HOST_CONFIG*)malloc(sizeof(JAVA_MODULE_HOST_CONFIG));
            if (result == NULL)
            {
                /*Codes_SRS_JAVA_MODULE_LOADER_14_049: [JavaModuleLoader_BuildModuleConfiguration shall return NULL if an underlying platform call fails.]*/
                LogError("malloc failed.");
            }
            else
            {
                if (mallocAndStrcpy_s((char**)(&(result->class_name)), STRING_c_str(((JAVA_LOADER_ENTRYPOINT*)entrypoint)->className)) != 0)
                {
                    free(result);
                    result = NULL;
                    LogError("failed to allocate class_name.");
                }
                else
                {
                    if (module_configuration != NULL && mallocAndStrcpy_s((char**)(&(result->configuration_json)), module_configuration) != 0)
                    {
                        free((char*)(result->class_name));
                        free(result);
                        result = NULL;
                        LogError("failed to allocate configuration_json.");
                    }
                    else
                    {
                        result->options = CONFIGURATION->options;
                    }
                }
            }
        }
    }

    return result;
}

static void JavaModuleLoader_FreeModuleConfiguration(const MODULE_LOADER* loader, const void* module_configuration)
{
    (void)loader;

    if (module_configuration == NULL)
    {
        /*Codes_SRS_JAVA_MODULE_LOADER_14_051: [JavaModuleLoader_FreeModuleConfiguration shall do nothing if module_configuration is NULL.]*/
        LogError("module_configuration is NULL");
    }
    else
    {
        JAVA_MODULE_HOST_CONFIG* config = (JAVA_MODULE_HOST_CONFIG*)module_configuration;
        /*Codes_SRS_JAVA_MODULE_LOADER_14_052: [JavaModuleLoader_FreeModuleConfiguration shall free the JAVA_MODULE_HOST_CONFIG object.]*/
        free((char*)(config->class_name));
        free((char*)(config->configuration_json));
        free(config);
    }
}

MODULE_LOADER_API Java_Module_Loader_API =
{
    .Load = JavaModuleLoader_Load,
    .Unload = JavaModuleLoader_Unload,
    .GetApi = JavaModuleLoader_GetModuleApi,

    .ParseEntrypointFromJson = JavaModuleLoader_ParseEntrypointFromJson,
    .FreeEntrypoint = JavaModuleLoader_FreeEntrypoint,

    .ParseConfigurationFromJson = JavaModuleLoader_ParseConfigurationFromJson,
    .FreeConfiguration = JavaModuleLoader_FreeConfiguration,

    .BuildModuleConfiguration = JavaModuleLoader_BuildModuleConfiguration,
    .FreeModuleConfiguration = JavaModuleLoader_FreeModuleConfiguration
};

MODULE_LOADER Java_Module_Loader =
{
    /*Codes_SRS_JAVA_MODULE_LOADER_14_054: [MODULE_LOADER::type shall by JAVA.]*/
    JAVA,                   //the loader type

    /*Codes_SRS_JAVA_MODULE_LOADER_14_055: [MODULE_LOADER::name shall be the string java.]*/
    JAVA_LOADER_NAME,       // the name of this loader

    NULL,                   // default loader configuration is NULL unless user overrides
    
    &Java_Module_Loader_API // the module loader function pointer table
};

const MODULE_LOADER* JavaLoader_Get(void)
{
    MODULE_LOADER* loader;

    /*Codes_SRS_JAVA_MODULE_LOADER_14_056: [JavaLoader_Get shall set the loader->configuration to a default JAVA_LOADER_CONFIGURATION by setting it to default values.]*/
    MODULE_LOADER_BASE_CONFIGURATION* default_config = (MODULE_LOADER_BASE_CONFIGURATION*)create_default_config();

    if (default_config == NULL)
    {
        /*Codes_SRS_JAVA_MODULE_LOADER_14_057: [JavaLoader_Get shall return NULL if any underlying call fails while attempting to set the default JAVA_LOADER_CONFIGURATION.]*/
        loader = NULL;
        LogError("Error creating default config.");
    }
    else
    {
        loader = &Java_Module_Loader;
        loader->configuration = default_config;
    }

    /*Codes_SRS_JAVA_MODULE_LOADER_14_053: [JavaLoader_Get shall return a non - NULL pointer to a MODULE_LOADER struct.]*/
    return loader;
}

static JAVA_LOADER_CONFIGURATION* create_default_config()
{
    JAVA_LOADER_CONFIGURATION* result;

    JVM_OPTIONS* default_options = (JVM_OPTIONS*)malloc(sizeof(JVM_OPTIONS));
    
    if (default_options == NULL)
    {
        result = NULL;
        LogError("malloc failed.");
    }
    else
    {
        if (set_default_paths(default_options) != 0)
        {
            free(default_options);
            result = NULL;
            LogError("Failed to set default paths.");
        }
        else
        {
            default_options->version = 0;
            default_options->debug = false;
            default_options->debug_port = 0;
            default_options->verbose = false;
            default_options->additional_options = NULL;

            
            result = (JAVA_LOADER_CONFIGURATION*)malloc(sizeof(JAVA_LOADER_CONFIGURATION));
            if (result == NULL)
            {
                free((char*)(default_options->class_path));
                free((char*)(default_options->library_path));
                free(default_options);
                result = NULL;
                LogError("malloc failed.");
            }
            else
            {
                result->base.binding_path = NULL;
                result->options = default_options;
            }
        }
    }
    return result;
}

const char* set_path(const char* install_location)
{
    const char* path;

    path = GET_PREFIX;

    if (path == NULL)
    {
        LogError("Failed to get the prefix.");
    }
    else
    {
        STRING_HANDLE str = STRING_construct(path);
        if (str == NULL)
        {
            path = NULL;
            LogError("STRING_construct failed.");
        }
        else
        {
            int sprintf_result = STRING_sprintf(str, "%s%s%s%s%s", SLASH, INSTALL_NAME, "-", VERSION, install_location);

            if (sprintf_result != 0)
            {
                path = NULL;
                LogError("STRING_sprintf failed");
            }
            else
            {
                if (mallocAndStrcpy_s((char**)(&path), STRING_c_str(str)) != 0)
                {
                    path = NULL;
                    LogError("Failed to copy string.");
                }
            }

            STRING_delete(str);
        }
    }

    return path;
}

static int set_default_paths(JVM_OPTIONS* options)
{
    int result = 0;

    if (options == NULL)
    {
        result = __LINE__;
    }
    else
    {
        //Set classpath
        const char* class_path = set_path(BINDINGS_INSTALL_LOCATION);
        if (class_path == NULL)
        {
            result = __LINE__;
        }
        else
        {
            result = mallocAndStrcpy_s((char**)(&options->class_path), class_path);
            if (result != 0)
            {
                result = __LINE__;
            }
            else
            {
                //Set librarypath
                const char* library_path = set_path(MODULES_INSTALL_LOCATION);
                if (library_path == NULL)
                {
                    free((char*)(options->class_path));
                    result = __LINE__;
                }
                else
                {
                    result = mallocAndStrcpy_s((char**)(&options->library_path), library_path);
                    if (result != 0)
                    {
                        free((char*)(options->class_path));
                        result = __LINE__;
                    }
                    free((char*)library_path);
                }
            }
            free((char*)class_path);
        }
    }

    return result;
}

static void free_jvm_config(JVM_OPTIONS* options)
{
    if (options != NULL)
    {
        for (size_t index = 0; index < VECTOR_size(options->additional_options); index++)
        {
            STRING_HANDLE* element = (STRING_HANDLE*)VECTOR_element(options->additional_options, index);
            if (element != NULL)
            {
                STRING_delete(*element);
            }
        }
        VECTOR_destroy(options->additional_options);
        free((char*)(options->class_path));
        free((char*)(options->library_path));
        free(options);
    }
}

static JVM_OPTIONS* parse_jvm_config(JSON_Object* object)
{
    JVM_OPTIONS* options;

    options = (JVM_OPTIONS*)malloc(sizeof(JVM_OPTIONS));
    if (options == NULL)
    {
        LogError("malloc failed.");
    }
    else
    {
        int status = set_default_paths(options);

        if (status != 0)
        {
            free(options);
            options = NULL;
            LogError("Failed to set default paths.");
        }
        else
        {
            const char* temp = json_object_get_string(object, JAVA_MODULE_CLASS_PATH_KEY);

            if (temp != NULL)
            {
                free((char*)(options->class_path));
                status = mallocAndStrcpy_s((char**)(&options->class_path), temp);
            }

            if (status != 0)
            {
                free((char*)(options->library_path));
                free(options);
                options = NULL;
                LogError("Failed to allocate class.path");
            }
            else
            {
                temp = json_object_get_string(object, JAVA_MODULE_LIBRARY_PATH_KEY);

                if (temp != NULL)
                {
                    free((char*)(options->library_path));
                    status = mallocAndStrcpy_s((char**)(&options->library_path), temp);
                }

                if (status != 0)
                {
                    free((char*)(options->class_path));
                    free(options);
                    options = NULL;
                    LogError("Failed to allocate library.path");
                }
                else
                {
                    options->version = (int)json_object_get_number(object, JAVA_MODULE_JVM_OPTIONS_VERSION_KEY);
                    options->debug = json_object_get_boolean(object, JAVA_MODULE_JVM_OPTIONS_DEBUG_KEY) == 1 ? true : false;
                    options->debug_port = (int)json_object_get_number(object, JAVA_MODULE_JVM_OPTIONS_DEBUG_PORT_KEY);
                    options->verbose = json_object_get_boolean(object, JAVA_MODULE_JVM_OPTIONS_VERBOSE_KEY) == 1 ? true : false;

                    JSON_Array* arr = json_object_get_array(object, JAVA_MODULE_JVM_OPTIONS_ADDITIONAL_OPTIONS_KEY);
                    size_t additional_options_count = json_array_get_count(arr);

                    options->additional_options = additional_options_count == 0 ? NULL : VECTOR_create(sizeof(STRING_HANDLE));
                    if (options->additional_options == NULL && additional_options_count != 0)
                    {
                        free((char*)(options->class_path));
                        free((char*)(options->library_path));
                        free(options);
                        options = NULL;
                        LogError("VECTOR_create failed.");
                    }
                    else
                    {
                        for (size_t index = 0; index < additional_options_count && status == 0; ++index)
                        {
                            STRING_HANDLE str = STRING_construct(json_array_get_string(arr, index));
                            if (str == NULL)
                            {
                                LogError("Failed to construct string.");
                                status = __LINE__;
                            }
                            else
                            {
                                if (VECTOR_push_back(options->additional_options, &str, 1) != 0)
                                {
                                    LogError("Failed to push additional option %i onto vector.", (int)index);
                                    STRING_delete(str);
                                    status = __LINE__;
                                }
                            }
                        }

                        if (status != 0)
                        {
                            free_jvm_config(options);
                            options = NULL;
                            LogError("Failed to properly create vector of additional options.");
                        }
                    }
                }
            }
        }
    }
    return options;
}

static DYNAMIC_LIBRARY_HANDLE system_load_prefixes(int args_c, ...)
{
    DYNAMIC_LIBRARY_HANDLE result = NULL;

    if (args_c == 0)
    {
        result = NULL;
    }
    else
    {
        va_list prefixes;
        va_start(prefixes, args_c);

        for (int i = 0; i < args_c && result == NULL; ++i)
        {
            const char* prefix = va_arg(prefixes, const char*);
            if (prefix != NULL)
            {
                STRING_HANDLE path = STRING_construct(prefix);
                if (path == NULL)
                {
                    result = NULL;
                    LogError("STRING_construct failed.");
                    break;
                }
                else
                {
                    int sprintf_result = STRING_sprintf(path, "%s%s%s%s%s%s%s", SLASH, INSTALL_NAME, "-", VERSION, MODULES_INSTALL_LOCATION, SLASH, JAVA_BINDING_MODULE_NAME);
                    if (sprintf_result != 0)
                    {
                        result = NULL;
                        LogError("STRING_sprintf failed.");
                        break;
                    }
                    else
                    {
                        LogInfo("Path is %s", STRING_c_str(path));
                        result = DynamicLibrary_LoadLibrary(STRING_c_str(path));
                    }
                    STRING_delete(path);
                }

            }
        }

        va_end(prefixes);
    }
    return result;
}

static DYNAMIC_LIBRARY_HANDLE system_load_env(int args_c, ...)
{
    DYNAMIC_LIBRARY_HANDLE result = NULL;

    if (args_c == 0)
    {
        result = NULL;
    }
    else
    {
        va_list env;
        va_start(env, args_c);

        for (int i = 0; i < args_c && result == NULL; ++i)
        {
            const char* env_var = va_arg(env, const char*);
            const char* value = getenv(env_var);
            if (value != NULL)
            {
                result = system_load_prefixes(1, value);
            }
        }
        va_end(env);
    }

    return result;
}