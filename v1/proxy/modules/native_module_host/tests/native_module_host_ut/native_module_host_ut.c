// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif

#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umock_c_negative_tests.h"
#include "umocktypes_charptr.h"
#include "umocktypes_stdint.h"
#include "azure_c_shared_utility/macro_utils.h"


static TEST_MUTEX_HANDLE g_dllByDll;

#ifdef __cplusplus
extern "C" {
#endif

static bool malloc_will_fail = false;
static size_t malloc_fail_count = 0;
static size_t malloc_count = 0;

void* my_gballoc_malloc(size_t size)
{
    ++malloc_count;

    void* result;
    if (malloc_will_fail == true && malloc_count == malloc_fail_count)
    {
        result = NULL;
    }
    else
    {
        result = malloc(size);
    }

    return result;
}

void my_gballoc_free(void* ptr)
{
    free(ptr);
}

int my_mallocAndStrcpy_s( char** destination, const char* source)
{
    int macs;
    (*destination) = (char*)my_gballoc_malloc(strlen(source) + 1);
    if ((*destination) != NULL)
    {
        strcpy(*destination, source);
        macs = 0;
    }
    else
    {
        macs = 1;
    }
    return macs;
}
#ifdef __cplusplus
}
#endif

#define ENABLE_MOCKS
#define GATEWAY_EXPORT_H
#define GATEWAY_EXPORT
#include "gateway.h"
#include "broker.h"
#include "module_loader.h"
#include "message.h"
#include "experimental/event_system.h"
#include "module_access.h"
#include "module.h"
#include "module_access.h"
#include "module_loaders/dynamic_loader.h"
#include "native_module_host.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "parson.h"

MOCKABLE_FUNCTION(, JSON_Value *, json_object_get_value, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(,JSON_Object *, json_object_get_object, const JSON_Object *,object, const char *, name);
MOCKABLE_FUNCTION(,char *, json_serialize_to_string, const JSON_Value *, value);
MOCKABLE_FUNCTION(, void, json_free_serialized_string, char *, string);
MOCKABLE_FUNCTION(, JSON_Value*, json_parse_string, const char *, string);
MOCKABLE_FUNCTION(, const char*, json_object_get_string, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, void, json_value_free, JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Object*, json_value_get_object, const JSON_Value *, value);

MOCKABLE_FUNCTION(, void*, mock_Module_ParseConfigurationFromJson, const char*, configuration);
MOCKABLE_FUNCTION(, void, mock_Module_FreeConfiguration, void*, configuration);
MOCKABLE_FUNCTION(, MODULE_HANDLE, mock_Module_Create, BROKER_HANDLE, broker, const void*, configuration);
MOCKABLE_FUNCTION(, void, mock_Module_Destroy, MODULE_HANDLE, moduleHandle);
MOCKABLE_FUNCTION(, void, mock_Module_Receive, MODULE_HANDLE, moduleHandle, MESSAGE_HANDLE, messageHandle);
MOCKABLE_FUNCTION(, void, mock_Module_Start, MODULE_HANDLE, moduleHandle);


#include "native_module_host.h"

static MODULE_API_1 dummyAPIs =
{
	{ MODULE_API_VERSION_1 },

	mock_Module_ParseConfigurationFromJson,
	mock_Module_FreeConfiguration,
	mock_Module_Create,
	mock_Module_Destroy,
	mock_Module_Receive,
	mock_Module_Start
};

MOCKABLE_FUNCTION(, MODULE_LIBRARY_HANDLE, mock_ModuleLoader_Load, const struct MODULE_LOADER_TAG*, loader, const void*, entrypoint)
MOCKABLE_FUNCTION(, void, mock_ModuleLoader_Unload,const struct MODULE_LOADER_TAG*, loader, MODULE_LIBRARY_HANDLE, handle);
MOCKABLE_FUNCTION(, const MODULE_API*, mock_ModuleLoader_GetApi,const struct MODULE_LOADER_TAG*, loader, MODULE_LIBRARY_HANDLE, handle);
MOCKABLE_FUNCTION(, void*, mock_ModuleLoader_ParseEntrypointFromJson,const struct MODULE_LOADER_TAG*, loader, const JSON_Value*, json);
MOCKABLE_FUNCTION(, void, mock_ModuleLoader_FreeEntrypoint,const struct MODULE_LOADER_TAG*, loader, void*, entrypoint);
MOCKABLE_FUNCTION(, MODULE_LOADER_BASE_CONFIGURATION*, mock_ModuleLoader_ParseConfigurationFromJson, const struct MODULE_LOADER_TAG*, loader, const JSON_Value*, json);
MOCKABLE_FUNCTION(, void, mock_ModuleLoader_FreeConfiguration, const struct MODULE_LOADER_TAG*, loader, MODULE_LOADER_BASE_CONFIGURATION*, configuration);
MOCKABLE_FUNCTION(, void*, mock_ModuleLoader_BuildModuleConfiguration, const struct MODULE_LOADER_TAG*, loader, const void*, entrypoint, const void*, module_configuration);
MOCKABLE_FUNCTION(, void, mock_ModuleLoader_FreeModuleConfiguration, const struct MODULE_LOADER_TAG*, loader, const void*, module_configuration);

static MODULE_LOADER_API module_loader_api =
{
	mock_ModuleLoader_Load,
	mock_ModuleLoader_Unload,
	mock_ModuleLoader_GetApi,

	mock_ModuleLoader_ParseEntrypointFromJson,
	mock_ModuleLoader_FreeEntrypoint,

	mock_ModuleLoader_ParseConfigurationFromJson,
	mock_ModuleLoader_FreeConfiguration,

	mock_ModuleLoader_BuildModuleConfiguration,
	mock_ModuleLoader_FreeModuleConfiguration
};

static MODULE_LOADER dummyModuleLoader =
{
	NATIVE,
	"dummy loader",
	NULL,
	&module_loader_api
};

#undef ENABLE_MOCKS

static void* my_Module_ParseConfigurationFromJson(const char* configuration)
{
    (void)configuration;
	return my_gballoc_malloc(13);
}
static void my_Module_FreeConfiguration(void* configuration)
{
	my_gballoc_free(configuration);
}
static MODULE_HANDLE my_Module_Create(BROKER_HANDLE broker, const void* configuration)
{
    (void)broker;
    (void)configuration;
	return (MODULE_HANDLE)my_gballoc_malloc(14);

}
static void my_Module_Destroy(MODULE_HANDLE moduleHandle)
{
	my_gballoc_free((void*)moduleHandle);
}

static MODULE_LIBRARY_HANDLE my_ModuleLoader_Load(const MODULE_LOADER* loader, const void* entrypoint)
{
    (void)loader;
    (void)entrypoint;
	MODULE_LIBRARY_HANDLE result = (MODULE_LIBRARY_HANDLE)my_gballoc_malloc(1);
	return result;
}

static void my_ModuleLoader_Unload(const MODULE_LOADER* loader, MODULE_LIBRARY_HANDLE moduleLibraryHandle)
{
    (void)loader;
	my_gballoc_free(moduleLibraryHandle);
}

static void* my_ModuleLoader_ParseEntrypointFromJson(const MODULE_LOADER* loader, const JSON_Value* json)
{
    (void)loader;
    (void)json;
	void* result = my_gballoc_malloc(2);
	return result;
}

static void my_ModuleLoader_FreeEntrypoint(const MODULE_LOADER* loader, void* entrypoint)
{
    (void)loader;
	my_gballoc_free(entrypoint);
}

static MODULE_LOADER_BASE_CONFIGURATION* my_ModuleLoader_ParseConfigurationFromJson(const MODULE_LOADER* loader, const JSON_Value* json)
{
    (void)loader;
    (void)json;
	MODULE_LOADER_BASE_CONFIGURATION* result = (MODULE_LOADER_BASE_CONFIGURATION*)my_gballoc_malloc(sizeof(MODULE_LOADER_BASE_CONFIGURATION));
	return result;
}

static void my_ModuleLoader_FreeConfiguration(const MODULE_LOADER* loader, MODULE_LOADER_BASE_CONFIGURATION* configuration)
{
   (void)loader;
	my_gballoc_free(configuration);
}

static void* my_ModuleLoader_BuildModuleConfiguration(
	const MODULE_LOADER* loader,
	const void* entrypoint,
	const void* module_configuration
)
{
    (void)loader;
    (void)entrypoint;
    (void)module_configuration;
	void* result = my_gballoc_malloc(3);
	return result;
}

static void my_ModuleLoader_FreeModuleConfiguration(const MODULE_LOADER* loader, const void* module_configuration)
{
    (void)loader;
    (void)module_configuration;
	my_gballoc_free((void*)module_configuration);
}


static TEST_MUTEX_HANDLE g_testByTest;

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    (void)error_code;
    ASSERT_FAIL("umock_c reported error");
}

BEGIN_TEST_SUITE(native_module_host_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);
	umocktypes_charptr_register_types();
	umocktypes_stdint_register_types();

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);
	REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);

	REGISTER_GLOBAL_MOCK_HOOK(mock_ModuleLoader_Load, my_ModuleLoader_Load);
	REGISTER_GLOBAL_MOCK_HOOK(mock_ModuleLoader_Unload, my_ModuleLoader_Unload);
	REGISTER_GLOBAL_MOCK_HOOK(mock_ModuleLoader_ParseEntrypointFromJson, 
		my_ModuleLoader_ParseEntrypointFromJson);
	REGISTER_GLOBAL_MOCK_HOOK(mock_ModuleLoader_FreeEntrypoint, 
		my_ModuleLoader_FreeEntrypoint);
	REGISTER_GLOBAL_MOCK_HOOK(mock_ModuleLoader_ParseConfigurationFromJson, 
		my_ModuleLoader_ParseConfigurationFromJson);
	REGISTER_GLOBAL_MOCK_HOOK(mock_ModuleLoader_FreeConfiguration, 
		my_ModuleLoader_FreeConfiguration);
	REGISTER_GLOBAL_MOCK_HOOK(mock_ModuleLoader_BuildModuleConfiguration, 
		my_ModuleLoader_BuildModuleConfiguration);
	REGISTER_GLOBAL_MOCK_HOOK(mock_ModuleLoader_FreeModuleConfiguration, 
		my_ModuleLoader_FreeModuleConfiguration);

	REGISTER_GLOBAL_MOCK_HOOK(mock_Module_ParseConfigurationFromJson, 
		my_Module_ParseConfigurationFromJson);
	REGISTER_GLOBAL_MOCK_HOOK(mock_Module_FreeConfiguration, my_Module_FreeConfiguration);
	REGISTER_GLOBAL_MOCK_HOOK(mock_Module_Create, my_Module_Create);
	REGISTER_GLOBAL_MOCK_HOOK(mock_Module_Destroy, my_Module_Destroy);

    REGISTER_UMOCK_ALIAS_TYPE(MODULE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_HANDLE, void*);
	REGISTER_UMOCK_ALIAS_TYPE(BROKER_HANDLE, void*);
	REGISTER_UMOCK_ALIAS_TYPE(BROKER_RESULT, int);
	REGISTER_UMOCK_ALIAS_TYPE(MODULE_LIBRARY_HANDLE, void*);
	REGISTER_UMOCK_ALIAS_TYPE(MODULE_LOADER_RESULT, int);


}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    TEST_MUTEX_DESTROY(g_testByTest);
    TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    umock_c_reset_all_calls();
    malloc_will_fail = false;
    malloc_fail_count = 0;
    malloc_count = 0;

}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

/*Tests_SRS_NATIVEMODULEHOST_17_001: [ Module_GetApi shall return a valid pointer to a MODULE_API structure. ]*/
/*Tests_SRS_NATIVEMODULEHOST_17_002: [ The MODULE_API structure returned shall have valid pointers for all pointers in the strcuture. ]*/
TEST_FUNCTION(NativeModuleHost_GetApi_returns_valid)
{
	///arrange
	///act
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
	///assert
	ASSERT_IS_TRUE(MODULE_PARSE_CONFIGURATION_FROM_JSON(apis) != NULL);
	ASSERT_IS_TRUE(MODULE_FREE_CONFIGURATION(apis) != NULL);
	ASSERT_IS_TRUE(MODULE_CREATE(apis) != NULL);
	ASSERT_IS_TRUE(MODULE_DESTROY(apis) != NULL);
	ASSERT_IS_TRUE(MODULE_RECEIVE(apis) != NULL);
	ASSERT_IS_TRUE(MODULE_START(apis) != NULL);
	///ablution
}

/*Tests_SRS_NATIVEMODULEHOST_17_004: [ On success, NativeModuleHost_ParseConfigurationFromJson shall return a valid copy of the configuration string given. ]*/
TEST_FUNCTION(NativeModuleHost_ParseConfigurationFromJson_success)
{
    ///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

    const char * config = "This is a string";
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, config))
        .IgnoreArgument(1);
    ///act
	void * result = MODULE_PARSE_CONFIGURATION_FROM_JSON(apis)(config);
    ///assert
	ASSERT_IS_NOT_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	ASSERT_ARE_EQUAL(char_ptr, config, (char*)result);
    ///ablution
	MODULE_FREE_CONFIGURATION(apis)(result);
}

/*Tests_SRS_NATIVEMODULEHOST_17_005: [ NativeModuleHost_ParseConfigurationFromJson shall return NULL if any system call fails. ]*/
TEST_FUNCTION(NativeModuleHost_ParseConfigurationFromJson_returns_null_when_alloc_fails)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

	const char * config = "This is a string";
	malloc_will_fail = true;
	malloc_fail_count = 1;
	STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, config))
		.IgnoreArgument(1);

	///act
	void * result = MODULE_PARSE_CONFIGURATION_FROM_JSON(apis)(config);
	///assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///ablution
}

/*Tests_SRS_NATIVEMODULEHOST_17_003: [ NativeModuleHost_ParseConfigurationFromJson shall return NULL if configuration is NULL. ]*/
TEST_FUNCTION(NativeModuleHost_ParseConfigurationFromJson_returns_null_when_gets_null)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

	const char * config = NULL;

	///act
	void * result = MODULE_PARSE_CONFIGURATION_FROM_JSON(apis)(config);
	///assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///ablution
}

/*Tests_SRS_NATIVEMODULEHOST_17_007: [ NativeModuleHost_FreeConfiguration shall free configuration. ]*/
TEST_FUNCTION(NativeModuleHost_FreeConfiguration_success)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

	const char * config = "This is a string";
	void * result = MODULE_PARSE_CONFIGURATION_FROM_JSON(apis)(config);
	umock_c_reset_all_calls();
	STRICT_EXPECTED_CALL(gballoc_free(result));

	///act
	MODULE_FREE_CONFIGURATION(apis)(result);

	///assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///ablution
}

/*Tests_SRS_NATIVEMODULEHOST_17_006: [ NativeModuleHost_FreeConfiguration shall do nothing if passed a NULL configuration. ]*/
TEST_FUNCTION(NativeModuleHost_FreeConfiguration_does_nothing_with_nothing)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

	///act
	MODULE_FREE_CONFIGURATION(apis)(NULL);

	///assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///ablution
}

/*Tests_SRS_NATIVEMODULEHOST_17_008: [ NativeModuleHost_Create shall return NULL if broker is NULL. ]*/
/*Tests_SRS_NATIVEMODULEHOST_17_009: [ NativeModuleHost_Create shall return NULL if configuration does not contain valid JSON. ]*/
TEST_FUNCTION(NativeModuleHost_Create_returns_null_on_null_input)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
	BROKER_HANDLE b = (BROKER_HANDLE)0x42;
	char * config = "Assume this is a valid config";

	///act
	MODULE_HANDLE m1 = MODULE_CREATE(apis)(NULL, config);
	MODULE_HANDLE m2 = MODULE_CREATE(apis)(b, NULL);
	///assert
	ASSERT_IS_NULL(m1);
	ASSERT_IS_NULL(m2);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///ablution
}

/*Tests_SRS_NATIVEMODULEHOST_17_010: [ NativeModuleHost_Create shall intialize the Module_Loader. ]*/
/*Tests_SRS_NATIVEMODULEHOST_17_011: [ NativeModuleHost_Create shall parse the configuration JSON. ]*/
/*Tests_SRS_NATIVEMODULEHOST_17_012: [ NativeModuleHost_Create shall get the "outprocess.loader" object from the configuration JSON. ]*/
/*Tests_SRS_NATIVEMODULEHOST_17_013: [ NativeModuleHost_Create shall get "name" string from the "outprocess.loader" object. ]*/
/*Tests_SRS_NATIVEMODULEHOST_17_014: [ If the loader name string is not present, then NativeModuleHost_Create shall assume the loader is the native dynamic library loader name. ]*/
/*Tests_SRS_NATIVEMODULEHOST_17_015: [ NativeModuleHost_Create shall find the module loader by name. ]*/
/*Tests_SRS_NATIVEMODULEHOST_17_016: [ NativeModuleHost_Create shall get "entrypoint" string from the "outprocess.loader" object. ]*/
/*Tests_SRS_NATIVEMODULEHOST_17_017: [ NativeModuleHost_Create shall parse the entrypoint string. ]*/
/*Tests_SRS_NATIVEMODULEHOST_17_018: [ NativeModuleHost_Create shall get the "module.args" object from the configuration JSON. ]*/
/*Tests_SRS_NATIVEMODULEHOST_17_019: [NativeModuleHost_Create shall load the module library with the pasred entrypoint.]*/
/*Tests_SRS_NATIVEMODULEHOST_17_020: [ NativeModuleHost_Create shall get the MODULE_API pointer from the loader. ]*/
/*Tests_SRS_NATIVEMODULEHOST_17_021: [ NativeModuleHost_Create shall call the module's _ParseConfigurationFromJson on the "module.args" object. ]*/
/*Tests_SRS_NATIVEMODULEHOST_17_022: [ NativeModuleHost_Create shall build the module confguration from the parsed entrypoint and parsed module arguments. ]*/
/*Tests_SRS_NATIVEMODULEHOST_17_023: [ NativeModuleHost_Create shall call the module's _Create on the built module configuration. ]*/
/*Tests_SRS_NATIVEMODULEHOST_17_024: [ NativeModuleHost_Create shall free all resources used during module loading. ]*/
/*Tests_SRS_NATIVEMODULEHOST_17_025: [ NativeModuleHost_Create shall return a non-null pointer to a MODULE_HANDLE on success. ]*/
/*Tests_SRS_NATIVEMODULEHOST_17_035: [ If the "outprocess.loaders" array exists in the configuration JSON, NativeModuleHost_Create shall initialize the Module_Loader from this array. ]*/
TEST_FUNCTION(NativeModuleHost_Create_success)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
	BROKER_HANDLE b = (BROKER_HANDLE)0x42;
	char * config = "Assume this is a valid config";
	STRICT_EXPECTED_CALL(ModuleLoader_Initialize());
	STRICT_EXPECTED_CALL(json_parse_string(config))
		.SetReturn((JSON_Value*)0x43);
	STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x43))
		.SetReturn((JSON_Object*)0x44);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Value*)0x55);
	STRICT_EXPECTED_CALL(ModuleLoader_InitializeFromJson((JSON_Value*)0x55))
		.SetReturn(MODULE_LOADER_SUCCESS);
	STRICT_EXPECTED_CALL(json_object_get_object((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Object*)0x45);
	//parse_loader
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x45, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn(NULL);
	STRICT_EXPECTED_CALL(ModuleLoader_FindByName(IGNORED_PTR_ARG))
		.IgnoreArgument(1).SetReturn(&dummyModuleLoader);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x45, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Value*)0x46);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_ParseEntrypointFromJson(&dummyModuleLoader, (JSON_Value*)0x46));
	//end parse_loader
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Value*)0x47);
	STRICT_EXPECTED_CALL(json_serialize_to_string((JSON_Value*)0x47))
		.SetReturn("a string");
	//module_create
	STRICT_EXPECTED_CALL(mock_ModuleLoader_Load(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_GetApi(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((const MODULE_API*)&dummyAPIs);
	STRICT_EXPECTED_CALL(mock_Module_ParseConfigurationFromJson(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_BuildModuleConfiguration(&dummyModuleLoader, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2).IgnoreArgument(3);
	STRICT_EXPECTED_CALL(mock_Module_Create(b, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mock_Module_FreeConfiguration(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_FreeModuleConfiguration(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_FreeEntrypoint(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x43));
	///act
	MODULE_HANDLE m = MODULE_CREATE(apis)(b, config);

	///assert
	ASSERT_IS_NOT_NULL(m);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///ablution
	umock_c_reset_all_calls();
	EXPECTED_CALL(mock_ModuleLoader_GetApi(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((const MODULE_API*)&dummyAPIs);
	MODULE_DESTROY(apis)(m);
}

/*Tests_SRS_NATIVEMODULEHOST_17_026: [ If any step above fails, then NativeModuleHost_Create shall free all resources allocated and return NULL. ]*/
TEST_FUNCTION(NativeModuleHost_Create_fails_module_create_fails)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
	BROKER_HANDLE b = (BROKER_HANDLE)0x42;
	char * config = "Assume this is a valid config";
	STRICT_EXPECTED_CALL(ModuleLoader_Initialize());
	STRICT_EXPECTED_CALL(json_parse_string(config))
		.SetReturn((JSON_Value*)0x43);
	STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x43))
		.SetReturn((JSON_Object*)0x44);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Value*)0x55);
	STRICT_EXPECTED_CALL(ModuleLoader_InitializeFromJson((JSON_Value*)0x55))
		.SetReturn(MODULE_LOADER_SUCCESS);
	STRICT_EXPECTED_CALL(json_object_get_object((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Object*)0x45);
	//parse_loader
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x45, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn(NULL);
	STRICT_EXPECTED_CALL(ModuleLoader_FindByName(IGNORED_PTR_ARG))
		.IgnoreArgument(1).SetReturn(&dummyModuleLoader);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x45, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Value*)0x46);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_ParseEntrypointFromJson(&dummyModuleLoader, (JSON_Value*)0x46));
	//end parse_loader
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Value*)0x47);
	STRICT_EXPECTED_CALL(json_serialize_to_string((JSON_Value*)0x47))
		.SetReturn("a string");
	//module_create
	STRICT_EXPECTED_CALL(mock_ModuleLoader_Load(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_GetApi(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((const MODULE_API*)&dummyAPIs);
	STRICT_EXPECTED_CALL(mock_Module_ParseConfigurationFromJson(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_BuildModuleConfiguration(&dummyModuleLoader, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2).IgnoreArgument(3);
	malloc_will_fail = true;
	malloc_fail_count = 6;
	STRICT_EXPECTED_CALL(mock_Module_Create(b, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_Unload(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mock_Module_FreeConfiguration(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_FreeModuleConfiguration(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_FreeEntrypoint(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x43));
	STRICT_EXPECTED_CALL(ModuleLoader_Destroy());

	///act
	MODULE_HANDLE m = MODULE_CREATE(apis)(b, config);

	///assert
	ASSERT_IS_NULL(m);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///ablution
}

/*Tests_SRS_NATIVEMODULEHOST_17_026: [ If any step above fails, then NativeModuleHost_Create shall free all resources allocated and return NULL. ]*/
TEST_FUNCTION(NativeModuleHost_Create_fails_GetModuleApi_fails)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
	BROKER_HANDLE b = (BROKER_HANDLE)0x42;
	char * config = "Assume this is a valid config";
	STRICT_EXPECTED_CALL(ModuleLoader_Initialize());
	STRICT_EXPECTED_CALL(json_parse_string(config))
		.SetReturn((JSON_Value*)0x43);
	STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x43))
		.SetReturn((JSON_Object*)0x44);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Value*)0x55);
	STRICT_EXPECTED_CALL(ModuleLoader_InitializeFromJson((JSON_Value*)0x55))
		.SetReturn(MODULE_LOADER_SUCCESS);
	STRICT_EXPECTED_CALL(json_object_get_object((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Object*)0x45);
	//parse_loader
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x45, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn(NULL);
	STRICT_EXPECTED_CALL(ModuleLoader_FindByName(IGNORED_PTR_ARG))
		.IgnoreArgument(1).SetReturn(&dummyModuleLoader);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x45, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Value*)0x46);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_ParseEntrypointFromJson(&dummyModuleLoader, (JSON_Value*)0x46));
	//end parse_loader
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Value*)0x47);
	STRICT_EXPECTED_CALL(json_serialize_to_string((JSON_Value*)0x47))
		.SetReturn("a string");
	//module_create
	STRICT_EXPECTED_CALL(mock_ModuleLoader_Load(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_GetApi(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn(NULL);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_Unload(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_FreeEntrypoint(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x43));
	STRICT_EXPECTED_CALL(ModuleLoader_Destroy());
	///act
	MODULE_HANDLE m = MODULE_CREATE(apis)(b, config);

	///assert
	ASSERT_IS_NULL(m);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///ablution
}

/*Tests_SRS_NATIVEMODULEHOST_17_026: [ If any step above fails, then NativeModuleHost_Create shall free all resources allocated and return NULL. ]*/
TEST_FUNCTION(NativeModuleHost_Create_fails_Load_fails)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
	BROKER_HANDLE b = (BROKER_HANDLE)0x42;
	char * config = "Assume this is a valid config";
	STRICT_EXPECTED_CALL(ModuleLoader_Initialize());
	STRICT_EXPECTED_CALL(json_parse_string(config))
		.SetReturn((JSON_Value*)0x43);
	STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x43))
		.SetReturn((JSON_Object*)0x44);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Value*)0x55);
	STRICT_EXPECTED_CALL(ModuleLoader_InitializeFromJson((JSON_Value*)0x55))
		.SetReturn(MODULE_LOADER_SUCCESS);
	STRICT_EXPECTED_CALL(json_object_get_object((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Object*)0x45);
	//parse_loader
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x45, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn(NULL);
	STRICT_EXPECTED_CALL(ModuleLoader_FindByName(IGNORED_PTR_ARG))
		.IgnoreArgument(1).SetReturn(&dummyModuleLoader);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x45, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Value*)0x46);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_ParseEntrypointFromJson(&dummyModuleLoader, (JSON_Value*)0x46));
	//end parse_loader
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Value*)0x47);
	STRICT_EXPECTED_CALL(json_serialize_to_string((JSON_Value*)0x47))
		.SetReturn("a string");
	//module_create
	malloc_will_fail = true;
	malloc_fail_count = 3;
	STRICT_EXPECTED_CALL(mock_ModuleLoader_Load(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_FreeEntrypoint(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x43));
	STRICT_EXPECTED_CALL(ModuleLoader_Destroy());
	///act
	MODULE_HANDLE m = MODULE_CREATE(apis)(b, config);

	///assert
	ASSERT_IS_NULL(m);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///ablution
}

/*Tests_SRS_NATIVEMODULEHOST_17_026: [ If any step above fails, then NativeModuleHost_Create shall free all resources allocated and return NULL. ]*/
TEST_FUNCTION(NativeModuleHost_Create_fails_json_mod_arg_fails)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
	BROKER_HANDLE b = (BROKER_HANDLE)0x42;
	char * config = "Assume this is a valid config";
	STRICT_EXPECTED_CALL(ModuleLoader_Initialize());
	STRICT_EXPECTED_CALL(json_parse_string(config))
		.SetReturn((JSON_Value*)0x43);
	STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x43))
		.SetReturn((JSON_Object*)0x44);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Value*)0x55);
	STRICT_EXPECTED_CALL(ModuleLoader_InitializeFromJson((JSON_Value*)0x55))
		.SetReturn(MODULE_LOADER_SUCCESS);
	STRICT_EXPECTED_CALL(json_object_get_object((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Object*)0x45);
	//parse_loader
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x45, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn(NULL);
	STRICT_EXPECTED_CALL(ModuleLoader_FindByName(IGNORED_PTR_ARG))
		.IgnoreArgument(1).SetReturn(&dummyModuleLoader);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x45, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Value*)0x46);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_ParseEntrypointFromJson(&dummyModuleLoader, (JSON_Value*)0x46));
	//end parse_loader
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Value*)0x47);
	STRICT_EXPECTED_CALL(json_serialize_to_string((JSON_Value*)0x47))
		.SetReturn(NULL);
	//module_create
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_FreeEntrypoint(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x43));
	STRICT_EXPECTED_CALL(ModuleLoader_Destroy());
	///act
	MODULE_HANDLE m = MODULE_CREATE(apis)(b, config);

	///assert
	ASSERT_IS_NULL(m);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///ablution
}

/*Tests_SRS_NATIVEMODULEHOST_17_026: [ If any step above fails, then NativeModuleHost_Create shall free all resources allocated and return NULL. ]*/
TEST_FUNCTION(NativeModuleHost_Create_fails_module_alloc_fails)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
	BROKER_HANDLE b = (BROKER_HANDLE)0x42;
	char * config = "Assume this is a valid config";
	STRICT_EXPECTED_CALL(ModuleLoader_Initialize());
	STRICT_EXPECTED_CALL(json_parse_string(config))
		.SetReturn((JSON_Value*)0x43);
	STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x43))
		.SetReturn((JSON_Object*)0x44);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Value*)0x55);
	STRICT_EXPECTED_CALL(ModuleLoader_InitializeFromJson((JSON_Value*)0x55))
		.SetReturn(MODULE_LOADER_SUCCESS);
	STRICT_EXPECTED_CALL(json_object_get_object((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Object*)0x45);
	//parse_loader
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x45, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn(NULL);
	STRICT_EXPECTED_CALL(ModuleLoader_FindByName(IGNORED_PTR_ARG))
		.IgnoreArgument(1).SetReturn(&dummyModuleLoader);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x45, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Value*)0x46);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_ParseEntrypointFromJson(&dummyModuleLoader, (JSON_Value*)0x46));
	//end parse_loader
	malloc_will_fail = true;
	malloc_fail_count = 2;
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	//module_create
	STRICT_EXPECTED_CALL(mock_ModuleLoader_FreeEntrypoint(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x43));
	STRICT_EXPECTED_CALL(ModuleLoader_Destroy());
	///act
	MODULE_HANDLE m = MODULE_CREATE(apis)(b, config);

	///assert
	ASSERT_IS_NULL(m);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///ablution
}
/*Tests_SRS_NATIVEMODULEHOST_17_026: [ If any step above fails, then NativeModuleHost_Create shall free all resources allocated and return NULL. ]*/
TEST_FUNCTION(NativeModuleHost_Create_fails_parseEntrypoint_fails)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
	BROKER_HANDLE b = (BROKER_HANDLE)0x42;
	char * config = "Assume this is a valid config";
	STRICT_EXPECTED_CALL(ModuleLoader_Initialize());
	STRICT_EXPECTED_CALL(json_parse_string(config))
		.SetReturn((JSON_Value*)0x43);
	STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x43))
		.SetReturn((JSON_Object*)0x44);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Value*)0x55);
	STRICT_EXPECTED_CALL(ModuleLoader_InitializeFromJson((JSON_Value*)0x55))
		.SetReturn(MODULE_LOADER_SUCCESS);
	STRICT_EXPECTED_CALL(json_object_get_object((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Object*)0x45);
	//parse_loader
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x45, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn(NULL);
	STRICT_EXPECTED_CALL(ModuleLoader_FindByName(IGNORED_PTR_ARG))
		.IgnoreArgument(1).SetReturn(&dummyModuleLoader);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x45, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Value*)0x46);
	malloc_will_fail = true;
	malloc_fail_count = 1;
	STRICT_EXPECTED_CALL(mock_ModuleLoader_ParseEntrypointFromJson(&dummyModuleLoader, (JSON_Value*)0x46));
	//end parse_loader

	STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x43));
	STRICT_EXPECTED_CALL(ModuleLoader_Destroy());
	///act
	MODULE_HANDLE m = MODULE_CREATE(apis)(b, config);

	///assert
	ASSERT_IS_NULL(m);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///ablution
}

/*Tests_SRS_NATIVEMODULEHOST_17_026: [ If any step above fails, then NativeModuleHost_Create shall free all resources allocated and return NULL. ]*/
TEST_FUNCTION(NativeModuleHost_Create_fails_json_get_entrypoint_fails)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
	BROKER_HANDLE b = (BROKER_HANDLE)0x42;
	char * config = "Assume this is a valid config";
	STRICT_EXPECTED_CALL(ModuleLoader_Initialize());
	STRICT_EXPECTED_CALL(json_parse_string(config))
		.SetReturn((JSON_Value*)0x43);
	STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x43))
		.SetReturn((JSON_Object*)0x44);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Value*)0x55);
	STRICT_EXPECTED_CALL(ModuleLoader_InitializeFromJson((JSON_Value*)0x55))
		.SetReturn(MODULE_LOADER_SUCCESS);
	STRICT_EXPECTED_CALL(json_object_get_object((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Object*)0x45);
	//parse_loader
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x45, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn(NULL);
	STRICT_EXPECTED_CALL(ModuleLoader_FindByName(IGNORED_PTR_ARG))
		.IgnoreArgument(1).SetReturn(&dummyModuleLoader);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x45, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn(NULL);
	//end parse_loader

	STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x43));
	STRICT_EXPECTED_CALL(ModuleLoader_Destroy());
	///act
	MODULE_HANDLE m = MODULE_CREATE(apis)(b, config);

	///assert
	ASSERT_IS_NULL(m);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///ablution
}

/*Tests_SRS_NATIVEMODULEHOST_17_026: [ If any step above fails, then NativeModuleHost_Create shall free all resources allocated and return NULL. ]*/
TEST_FUNCTION(NativeModuleHost_Create_fails_loader_not_found_fails)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
	BROKER_HANDLE b = (BROKER_HANDLE)0x42;
	char * config = "Assume this is a valid config";
	STRICT_EXPECTED_CALL(ModuleLoader_Initialize());
	STRICT_EXPECTED_CALL(json_parse_string(config))
		.SetReturn((JSON_Value*)0x43);
	STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x43))
		.SetReturn((JSON_Object*)0x44);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Value*)0x55);
	STRICT_EXPECTED_CALL(ModuleLoader_InitializeFromJson((JSON_Value*)0x55))
		.SetReturn(MODULE_LOADER_SUCCESS);
	STRICT_EXPECTED_CALL(json_object_get_object((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Object*)0x45);
	//parse_loader
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x45, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn(NULL);
	STRICT_EXPECTED_CALL(ModuleLoader_FindByName(IGNORED_PTR_ARG))
		.IgnoreArgument(1).SetReturn(NULL);
	//end parse_loader

	STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x43));
	STRICT_EXPECTED_CALL(ModuleLoader_Destroy());
	///act
	MODULE_HANDLE m = MODULE_CREATE(apis)(b, config);

	///assert
	ASSERT_IS_NULL(m);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///ablution
}

/*Tests_SRS_NATIVEMODULEHOST_17_026: [ If any step above fails, then NativeModuleHost_Create shall free all resources allocated and return NULL. ]*/
TEST_FUNCTION(NativeModuleHost_Create_fails_loader_key_not_found_fails)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
	BROKER_HANDLE b = (BROKER_HANDLE)0x42;
	char * config = "Assume this is a valid config";
	STRICT_EXPECTED_CALL(ModuleLoader_Initialize());
	STRICT_EXPECTED_CALL(json_parse_string(config))
		.SetReturn((JSON_Value*)0x43);
	STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x43))
		.SetReturn((JSON_Object*)0x44);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Value*)0x55);
	STRICT_EXPECTED_CALL(ModuleLoader_InitializeFromJson((JSON_Value*)0x55))
		.SetReturn(MODULE_LOADER_SUCCESS);
	STRICT_EXPECTED_CALL(json_object_get_object((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn(NULL);


	STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x43));
	STRICT_EXPECTED_CALL(ModuleLoader_Destroy());
	///act
	MODULE_HANDLE m = MODULE_CREATE(apis)(b, config);

	///assert
	ASSERT_IS_NULL(m);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///ablution
}
/*Tests_SRS_NATIVEMODULEHOST_17_026: [ If any step above fails, then NativeModuleHost_Create shall free all resources allocated and return NULL. ]*/
TEST_FUNCTION(NativeModuleHost_Create_fails_init_loaders_from_json_fails)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
	BROKER_HANDLE b = (BROKER_HANDLE)0x42;
	char * config = "Assume this is a valid config";
	STRICT_EXPECTED_CALL(ModuleLoader_Initialize());
	STRICT_EXPECTED_CALL(json_parse_string(config))
		.SetReturn((JSON_Value*)0x43);
	STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x43))
		.SetReturn((JSON_Object*)0x44);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Value*)0x55);
	STRICT_EXPECTED_CALL(ModuleLoader_InitializeFromJson((JSON_Value*)0x55))
		.SetReturn(MODULE_LOADER_ERROR);

	STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x43));
	STRICT_EXPECTED_CALL(ModuleLoader_Destroy());
	///act
	MODULE_HANDLE m = MODULE_CREATE(apis)(b, config);

	///assert
	ASSERT_IS_NULL(m);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///ablution
}

/*Tests_SRS_NATIVEMODULEHOST_17_026: [ If any step above fails, then NativeModuleHost_Create shall free all resources allocated and return NULL. ]*/
TEST_FUNCTION(NativeModuleHost_Create_fails_json_root_obj_not_found_fails)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
	BROKER_HANDLE b = (BROKER_HANDLE)0x42;
	char * config = "Assume this is a valid config";
	STRICT_EXPECTED_CALL(ModuleLoader_Initialize());
	STRICT_EXPECTED_CALL(json_parse_string(config))
		.SetReturn((JSON_Value*)0x43);
	STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x43))
		.SetReturn(NULL);

	STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x43));
	STRICT_EXPECTED_CALL(ModuleLoader_Destroy());
	///act
	MODULE_HANDLE m = MODULE_CREATE(apis)(b, config);

	///assert
	ASSERT_IS_NULL(m);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///ablution
}

/*Tests_SRS_NATIVEMODULEHOST_17_026: [ If any step above fails, then NativeModuleHost_Create shall free all resources allocated and return NULL. ]*/
TEST_FUNCTION(NativeModuleHost_Create_fails_json_parse_fails)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
	BROKER_HANDLE b = (BROKER_HANDLE)0x42;
	char * config = "Assume this is a valid config";
	STRICT_EXPECTED_CALL(ModuleLoader_Initialize());
	STRICT_EXPECTED_CALL(json_parse_string(config))
		.SetReturn(NULL);

	STRICT_EXPECTED_CALL(ModuleLoader_Destroy());
	///act
	MODULE_HANDLE m = MODULE_CREATE(apis)(b, config);

	///assert
	ASSERT_IS_NULL(m);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///ablution
}

/*Tests_SRS_NATIVEMODULEHOST_17_026: [ If any step above fails, then NativeModuleHost_Create shall free all resources allocated and return NULL. ]*/
TEST_FUNCTION(NativeModuleHost_Create_fails_moduleloader_init_fails)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
	BROKER_HANDLE b = (BROKER_HANDLE)0x42;
	char * config = "Assume this is a valid config";
	STRICT_EXPECTED_CALL(ModuleLoader_Initialize())
		.SetReturn(MODULE_LOADER_ERROR);

	///act
	MODULE_HANDLE m = MODULE_CREATE(apis)(b, config);

	///assert
	ASSERT_IS_NULL(m);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///ablution
}

/*Tests_SRS_NATIVEMODULEHOST_17_027: [ NativeModuleHost_Destroy shall always destroy the module loader. ]*/
TEST_FUNCTION(NativeModuleHost_Destroy_destroys_loader_with_nothing)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

	STRICT_EXPECTED_CALL(ModuleLoader_Destroy());

	///act
	MODULE_DESTROY(apis)(NULL);

	///assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///ablution
}

static MODULE_HANDLE create_a_module(const MODULE_API* apis)
{
	BROKER_HANDLE b = (BROKER_HANDLE)0x42;
	char * config = "Assume this is a valid config";
	STRICT_EXPECTED_CALL(ModuleLoader_Initialize());
	STRICT_EXPECTED_CALL(json_parse_string(config))
		.SetReturn((JSON_Value*)0x43);
	STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x43))
		.SetReturn((JSON_Object*)0x44);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn(NULL);
	STRICT_EXPECTED_CALL(json_object_get_object((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Object*)0x45);
	//parse_loader
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x45, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn(NULL);
	STRICT_EXPECTED_CALL(ModuleLoader_FindByName(IGNORED_PTR_ARG))
		.IgnoreArgument(1).SetReturn(&dummyModuleLoader);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x45, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Value*)0x46);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_ParseEntrypointFromJson(&dummyModuleLoader, (JSON_Value*)0x46));
	//end parse_loader
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_object_get_value((JSON_Object*)0x44, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((JSON_Value*)0x47);
	STRICT_EXPECTED_CALL(json_serialize_to_string((JSON_Value*)0x47))
		.SetReturn("a string");
	//module_create
	STRICT_EXPECTED_CALL(mock_ModuleLoader_Load(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_GetApi(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((const MODULE_API*)&dummyAPIs);
	STRICT_EXPECTED_CALL(mock_Module_ParseConfigurationFromJson(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_BuildModuleConfiguration(&dummyModuleLoader, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2).IgnoreArgument(3);
	STRICT_EXPECTED_CALL(mock_Module_Create(b, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mock_Module_FreeConfiguration(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_FreeModuleConfiguration(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_FreeEntrypoint(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x43));

	///act
	MODULE_HANDLE m = MODULE_CREATE(apis)(b, config);
	ASSERT_IS_NOT_NULL(m);
	umock_c_reset_all_calls();
	return m;
}

/*Tests_SRS_NATIVEMODULEHOST_17_028: [ NativeModuleHost_Destroy shall free all remaining allocated resources if moduleHandle is not NULL. ]*/
TEST_FUNCTION(NativeModuleHost_Destroy_success)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

	MODULE_HANDLE m = create_a_module(apis);

	STRICT_EXPECTED_CALL(mock_ModuleLoader_GetApi(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((const MODULE_API*)&dummyAPIs);
	STRICT_EXPECTED_CALL(mock_Module_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mock_ModuleLoader_Unload(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(ModuleLoader_Destroy());

	///act
	MODULE_DESTROY(apis)(m);

	///assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///ablution
}

/*Tests_SRS_NATIVEMODULEHOST_17_029: [ NativeModuleHost_Receive shall do nothing if moduleHandle is NULL. ]*/
TEST_FUNCTION(NativeModuleHost_Receive_does_nothing_with_nothing)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

	///act
	MODULE_RECEIVE(apis)(NULL, (MESSAGE_HANDLE)0x42);

	///assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///ablution
}

/*Tests_SRS_NATIVEMODULEHOST_17_030: [ NativeModuleHost_Receive shall get the loaded module's MODULE_API pointer. ]*/
/*Tests_SRS_NATIVEMODULEHOST_17_031: [ NativeModuleHost_Receive shall call the loaded module's _Receive function, passing the messageHandle along. ]*/
TEST_FUNCTION(NativeModuleHost_Receive_success)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
	MESSAGE_HANDLE TEST_MSG = (MESSAGE_HANDLE)0x42;
	MODULE_HANDLE m = create_a_module(apis);

	STRICT_EXPECTED_CALL(mock_ModuleLoader_GetApi(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((const MODULE_API*)&dummyAPIs);
	STRICT_EXPECTED_CALL(mock_Module_Receive(IGNORED_PTR_ARG, TEST_MSG))
		.IgnoreArgument(1);

	///act
	MODULE_RECEIVE(apis)(m, TEST_MSG);

	///ablution
	umock_c_reset_all_calls();
	EXPECTED_CALL(mock_ModuleLoader_GetApi(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((const MODULE_API*)&dummyAPIs);
	MODULE_DESTROY(apis)(m);
}

/*Tests_SRS_NATIVEMODULEHOST_17_030: [ NativeModuleHost_Receive shall get the loaded module's MODULE_API pointer. ]*/
TEST_FUNCTION(NativeModuleHost_Receive_no_receive_in_api)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
	MESSAGE_HANDLE TEST_MSG = (MESSAGE_HANDLE)0x42;
	MODULE_HANDLE m = create_a_module(apis);
	static MODULE_API_1 dummyAPIs_no_recv =
	{
		{ MODULE_API_VERSION_1 },

		mock_Module_ParseConfigurationFromJson,
		mock_Module_FreeConfiguration,
		mock_Module_Create,
		mock_Module_Destroy,
		NULL,
		mock_Module_Start
	};

	STRICT_EXPECTED_CALL(mock_ModuleLoader_GetApi(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((const MODULE_API*)&dummyAPIs_no_recv);

	///act
	MODULE_RECEIVE(apis)(m, TEST_MSG);

	///ablution
	umock_c_reset_all_calls();
	EXPECTED_CALL(mock_ModuleLoader_GetApi(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((const MODULE_API*)&dummyAPIs);
	MODULE_DESTROY(apis)(m);
}

/*Tests_SRS_NATIVEMODULEHOST_17_030: [ NativeModuleHost_Receive shall get the loaded module's MODULE_API pointer. ]*/
TEST_FUNCTION(NativeModuleHost_Receive_get_api_fails)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
	MESSAGE_HANDLE TEST_MSG = (MESSAGE_HANDLE)0x42;
	MODULE_HANDLE m = create_a_module(apis);

	STRICT_EXPECTED_CALL(mock_ModuleLoader_GetApi(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn(NULL);

	///act
	MODULE_RECEIVE(apis)(m, TEST_MSG);

	///ablution
	umock_c_reset_all_calls();
	EXPECTED_CALL(mock_ModuleLoader_GetApi(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((const MODULE_API*)&dummyAPIs);
	MODULE_DESTROY(apis)(m);
}

/*Tests_SRS_NATIVEMODULEHOST_17_032: [ NativeModuleHost_Start shall do nothing if moduleHandle is NULL. ]*/
TEST_FUNCTION(NativeModuleHost_Start_does_nothing_with_nothing)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

	///act
	MODULE_START(apis)(NULL);

	///assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///ablution
}

/*Tests_SRS_NATIVEMODULEHOST_17_033: [ NativeModuleHost_Start shall get the loaded module's MODULE_API pointer. ]*/
/*Tests_SRS_NATIVEMODULEHOST_17_034: [ NativeModuleHost_Start shall call the loaded module's _Start function, if defined. ]*/
TEST_FUNCTION(NativeModuleHost_Start_success)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
	MODULE_HANDLE m = create_a_module(apis);

	STRICT_EXPECTED_CALL(mock_ModuleLoader_GetApi(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((const MODULE_API*)&dummyAPIs);
	STRICT_EXPECTED_CALL(mock_Module_Start(IGNORED_PTR_ARG));

	///act
	MODULE_START(apis)(m);

	///ablution
	umock_c_reset_all_calls();
	EXPECTED_CALL(mock_ModuleLoader_GetApi(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((const MODULE_API*)&dummyAPIs);
	MODULE_DESTROY(apis)(m);
}

/*Tests_SRS_NATIVEMODULEHOST_17_033: [ NativeModuleHost_Start shall get the loaded module's MODULE_API pointer. ]*/
/*Tests_SRS_NATIVEMODULEHOST_17_034: [ NativeModuleHost_Start shall call the loaded module's _Start function, if defined. ]*/
TEST_FUNCTION(NativeModuleHost_Start_no_start_in_api)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
	MODULE_HANDLE m = create_a_module(apis);
	static MODULE_API_1 dummyAPIs_no_start =
	{
		{ MODULE_API_VERSION_1 },

		mock_Module_ParseConfigurationFromJson,
		mock_Module_FreeConfiguration,
		mock_Module_Create,
		mock_Module_Destroy,
		mock_Module_Receive,
		NULL
	};

	STRICT_EXPECTED_CALL(mock_ModuleLoader_GetApi(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((const MODULE_API*)&dummyAPIs_no_start);

	///act
	MODULE_START(apis)(m);

	///ablution
	umock_c_reset_all_calls();
	EXPECTED_CALL(mock_ModuleLoader_GetApi(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((const MODULE_API*)&dummyAPIs);
	MODULE_DESTROY(apis)(m);
}

/*Tests_SRS_NATIVEMODULEHOST_17_033: [ NativeModuleHost_Start shall get the loaded module's MODULE_API pointer. ]*/
TEST_FUNCTION(NativeModuleHost_Start_get_api_fails)
{
	///arrange
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
	MODULE_HANDLE m = create_a_module(apis);

	STRICT_EXPECTED_CALL(mock_ModuleLoader_GetApi(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn(NULL);

	///act
	MODULE_START(apis)(m);

	///ablution
	umock_c_reset_all_calls();
	EXPECTED_CALL(mock_ModuleLoader_GetApi(&dummyModuleLoader, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn((const MODULE_API*)&dummyAPIs);
	MODULE_DESTROY(apis)(m);
}


END_TEST_SUITE(native_module_host_ut)
