// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

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

#define GATEWAY_EXPORT_H
#define GATEWAY_EXPORT

#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umock_c_negative_tests.h"
#include "umocktypes_charptr.h"
#include "umocktypes_bool.h"
#include "umocktypes_stdint.h"

#include "real_strings.h"
#define ENABLE_MOCKS


#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/uniqueid.h"

#include "parson.h"
#include "module_loader.h"

#include  "control_message.h"

#undef ENABLE_MOCKS

#include "module_loaders/outprocess_loader.h"
#include "module_loaders/outprocess_module.h"

static pfModuleLoader_Load OutprocessModuleLoader_Load = NULL;
static pfModuleLoader_Unload OutprocessModuleLoader_Unload = NULL;
static pfModuleLoader_GetApi OutprocessModuleLoader_GetModuleApi = NULL;
static pfModuleLoader_ParseEntrypointFromJson OutprocessModuleLoader_ParseEntrypointFromJson = NULL;
static pfModuleLoader_FreeEntrypoint OutprocessModuleLoader_FreeEntrypoint = NULL;
static pfModuleLoader_ParseConfigurationFromJson OutprocessModuleLoader_ParseConfigurationFromJson = NULL;
static pfModuleLoader_FreeConfiguration OutprocessModuleLoader_FreeConfiguration = NULL;
static pfModuleLoader_BuildModuleConfiguration OutprocessModuleLoader_BuildModuleConfiguration = NULL;
static pfModuleLoader_FreeModuleConfiguration OutprocessModuleLoader_FreeModuleConfiguration = NULL;

MOCKABLE_FUNCTION(, void, json_free_serialized_string, char*, string);
MOCKABLE_FUNCTION(, char*, json_serialize_to_string, const JSON_Value*, value);
MOCKABLE_FUNCTION(, JSON_Value*, json_object_get_value, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, const char*, json_object_get_string, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, JSON_Object*, json_value_get_object, const JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Value_Type, json_value_get_type, const JSON_Value*, value);

//=============================================================================
//Globals
//=============================================================================

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;
const MODULE_API_1 Outprocess_Module_API_all =
{
	{ MODULE_API_VERSION_1 },
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error");
}

MOCK_FUNCTION_WITH_CODE(, MODULE_API*, Fake_GetAPI, MODULE_API_VERSION, gateway_api_version)
MODULE_API* val = (MODULE_API*)0x42;
MOCK_FUNCTION_END(val)

STRING_HANDLE myURL_EncodeString(const char* textEncode)
{
	STRING_HANDLE val = real_STRING_construct(textEncode);
	return val;
}

//parson mocks

MOCK_FUNCTION_WITH_CODE(, JSON_Object*, json_value_get_object, const JSON_Value*, value)
    JSON_Object* obj = NULL;
    if (value != NULL)
    {
        obj = (JSON_Object*)0x42;
    }
MOCK_FUNCTION_END(obj)

MOCK_FUNCTION_WITH_CODE(, const char*, json_object_get_string, const JSON_Object*, object, const char*, name)
    const char* str = NULL;
    if (object != NULL && name != NULL)
    {
        str = "hello_world";
    }
MOCK_FUNCTION_END(str)

MOCK_FUNCTION_WITH_CODE(, JSON_Value*, json_object_get_value, const JSON_Object*, object, const char*, name)
    JSON_Value* value = NULL;
    if (object != NULL && name != NULL)
    {
        value = (JSON_Value*)0x42;
    }
MOCK_FUNCTION_END(value)

MOCK_FUNCTION_WITH_CODE(, JSON_Value_Type, json_value_get_type, const JSON_Value*, value)
    JSON_Value_Type val = JSONError;
    if (value != NULL)
    {
        val = JSONString;
    }
MOCK_FUNCTION_END(val)

MOCK_FUNCTION_WITH_CODE(, void, json_free_serialized_string, char*, string)
	my_gballoc_free(string);
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, char*, json_serialize_to_string, const JSON_Value*, value)
char * newString = NULL;
if (value != NULL)
{
	newString = (char*)my_gballoc_malloc(1);
	if (newString != NULL)
		*newString = '\0';
}
MOCK_FUNCTION_END(newString)


#undef ENABLE_MOCKS

TEST_DEFINE_ENUM_TYPE(OUTPROCESS_LOADER_ACTIVATION_TYPE, OUTPROCESS_LOADER_ACTIVATION_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(OUTPROCESS_LOADER_ACTIVATION_TYPE, OUTPROCESS_LOADER_ACTIVATION_TYPE_VALUES);

TEST_DEFINE_ENUM_TYPE(MODULE_LOADER_TYPE, MODULE_LOADER_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(MODULE_LOADER_TYPE, MODULE_LOADER_TYPE_VALUES);

BEGIN_TEST_SUITE(OutprocessLoader_UnitTests)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);
    umocktypes_charptr_register_types();
    umocktypes_stdint_register_types();

    REGISTER_UMOCK_ALIAS_TYPE(MODULE_LOADER_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(MODULE_LOADER_TYPE, int);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MODULE_LIBRARY_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(JSON_Value_Type, int);
	REGISTER_UMOCK_ALIAS_TYPE(MODULE_API_VERSION, int);
	REGISTER_UMOCK_ALIAS_TYPE(UNIQUEID_RESULT, int);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_object, NULL);

    // malloc/free hooks
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    // Strings hooks
    REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, real_STRING_construct);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_clone, real_STRING_clone);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, real_STRING_delete);
	REGISTER_GLOBAL_MOCK_HOOK(STRING_c_str, real_STRING_c_str);
	REGISTER_GLOBAL_MOCK_HOOK(URL_EncodeString, myURL_EncodeString);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_clone, NULL);

    const MODULE_LOADER* loader = OutprocessLoader_Get();
    OutprocessModuleLoader_Load = loader->api->Load;
    OutprocessModuleLoader_Unload = loader->api->Unload;
    OutprocessModuleLoader_GetModuleApi = loader->api->GetApi;
    OutprocessModuleLoader_ParseEntrypointFromJson = loader->api->ParseEntrypointFromJson;
    OutprocessModuleLoader_FreeEntrypoint = loader->api->FreeEntrypoint;
    OutprocessModuleLoader_ParseConfigurationFromJson = loader->api->ParseConfigurationFromJson;
    OutprocessModuleLoader_FreeConfiguration = loader->api->FreeConfiguration;
    OutprocessModuleLoader_BuildModuleConfiguration = loader->api->BuildModuleConfiguration;
    OutprocessModuleLoader_FreeModuleConfiguration = loader->api->FreeModuleConfiguration;
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
    TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest) != 0)
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    umock_c_reset_all_calls();
    malloc_will_fail = false;
    malloc_fail_count = 0;
    malloc_count = 0;
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

/*Tests_SRS_OUTPROCESS_LOADER_17_001: [ If loader or entrypoint are NULL, then this function shall return NULL. ]*/
TEST_FUNCTION(OutprocessModuleLoader_Load_returns_NULL_when_loader_is_NULL)
{
    // act
    MODULE_LIBRARY_HANDLE result = OutprocessModuleLoader_Load(NULL, (void*)0x42);

    // assert
    ASSERT_IS_NULL(result);
}

/*Tests_SRS_OUTPROCESS_LOADER_17_001: [ If loader or entrypoint are NULL, then this function shall return NULL. ]*/
TEST_FUNCTION(OutprocessModuleLoader_Load_returns_NULL_when_entrypoint_is_NULL)
{
    // act
    MODULE_LIBRARY_HANDLE result = OutprocessModuleLoader_Load((const MODULE_LOADER*)0x42, NULL);

    // assert
    ASSERT_IS_NULL(result);
}

/*Tests_SRS_OUTPROCESS_LOADER_17_042: [ If the loader type is not OUTPROCESS, then this function shall return NULL. ]*/
TEST_FUNCTION(OutprocessModuleLoader_Load_returns_NULL_when_loader_type_is_not_OUTPROCESS)
{
    // arrange
    MODULE_LOADER loader =
    {
        NODEJS,
        NULL, NULL, NULL
    };

    // act
    MODULE_LIBRARY_HANDLE result = OutprocessModuleLoader_Load(&loader, (void*)0x42);

    // assert
    ASSERT_IS_NULL(result);
}

/*Tests_SRS_OUTPROCESS_LOADER_17_002: [ If the entrypoint's control_id is NULL, then this function shall return NULL. ]*/
TEST_FUNCTION(OutprocessModuleLoader_Load_returns_NULL_when_control_id_is_NULL)
{
	// arrange
	OUTPROCESS_LOADER_ENTRYPOINT entrypoint = { OUTPROCESS_LOADER_ACTIVATION_NONE, NULL, (STRING_HANDLE)0x42 };
	MODULE_LOADER loader =
	{
		OUTPROCESS,
		NULL, NULL, NULL
	};

	// act
	MODULE_LIBRARY_HANDLE result = OutprocessModuleLoader_Load(&loader, &entrypoint);

	// assert
	ASSERT_IS_NULL(result);
}

/*Tests_SRS_OUTPROCESS_LOADER_17_003: [ If the entrypoint's activation_type is not NONE, the this function shall return NULL. ]*/
TEST_FUNCTION(OutprocessModuleLoader_Load_returns_NULL_when_activation_type_is_not_NONE)
{
	// arrange
	OUTPROCESS_LOADER_ENTRYPOINT entrypoint = { (OUTPROCESS_LOADER_ACTIVATION_TYPE)0x42, (STRING_HANDLE)0x42, (STRING_HANDLE)0x42};
	MODULE_LOADER loader =
	{
		OUTPROCESS,
		NULL, NULL, NULL
	};

	// act
	MODULE_LIBRARY_HANDLE result = OutprocessModuleLoader_Load(&loader, &entrypoint);

	// assert
	ASSERT_IS_NULL(result);
}

/*Tests_SRS_OUTPROCESS_LOADER_17_008: [ If any call in this function fails, this function shall return NULL. ]*/
TEST_FUNCTION(OutprocessModuleLoader_Load_returns_NULL_when_things_fail)
{
    // arrange
	OUTPROCESS_LOADER_ENTRYPOINT entrypoint = { OUTPROCESS_LOADER_ACTIVATION_NONE, (STRING_HANDLE)0x42, (STRING_HANDLE)0x42 };
	MODULE_LOADER loader =
	{
		OUTPROCESS,
		NULL, NULL, NULL
	};
    umock_c_reset_all_calls();

    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        // arrange
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        MODULE_LIBRARY_HANDLE result = OutprocessModuleLoader_Load(&loader, &entrypoint);

        // assert
        ASSERT_IS_NULL(result);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

/*Tests_SRS_OUTPROCESS_LOADER_17_004: [ The loader shall allocate memory for the loader handle. ]*/
/*Tests_SRS_OUTPROCESS_LOADER_17_006: [ The loader shall store the Outprocess_Module_API_all in the loader handle. ]*/
/*Tests_SRS_OUTPROCESS_LOADER_17_007: [ Upon success, this function shall return a valid pointer to the loader handle. ]*/
TEST_FUNCTION(OutprocessModuleLoader_Load_succeeds)
{
    // arrange
	OUTPROCESS_LOADER_ENTRYPOINT entrypoint = { OUTPROCESS_LOADER_ACTIVATION_NONE, (STRING_HANDLE)0x42, (STRING_HANDLE)0x42 };
	MODULE_LOADER loader =
	{
		OUTPROCESS,
		NULL, NULL, NULL
	};

    umock_c_reset_all_calls();


    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    // act
    MODULE_LIBRARY_HANDLE result = OutprocessModuleLoader_Load(&loader, &entrypoint);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    OutprocessModuleLoader_Unload(&loader, result);
}

TEST_FUNCTION(OutprocessModuleLoader_GetModuleApi_returns_NULL_when_moduleLibraryHandle_is_NULL)
{
    // act
    const MODULE_API* result = OutprocessModuleLoader_GetModuleApi(NULL, NULL);

    // assert
    ASSERT_IS_NULL(result);
}

/*Tests_SRS_OUTPROCESS_LOADER_17_009: [ This function shall return a valid pointer to the Outprocess_Module_API_all MODULE_API. ]*/
TEST_FUNCTION(OutprocessModuleLoader_GetModuleApi_succeeds)
{
    // arrange
	OUTPROCESS_LOADER_ENTRYPOINT entrypoint = { OUTPROCESS_LOADER_ACTIVATION_NONE, (STRING_HANDLE)0x42, (STRING_HANDLE)0x42};
	MODULE_LOADER loader =
	{
		OUTPROCESS,
		NULL, NULL, NULL
	};

    MODULE_LIBRARY_HANDLE module = OutprocessModuleLoader_Load(&loader, &entrypoint);
    ASSERT_IS_NOT_NULL(module);

    umock_c_reset_all_calls();

    // act
    const MODULE_API* result = OutprocessModuleLoader_GetModuleApi(&loader, module);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    OutprocessModuleLoader_Unload(&loader, module);
}

/*Tests_SRS_OUTPROCESS_LOADER_17_010: [ This function shall do nothing if moduleLibraryHandle is NULL. ]*/
TEST_FUNCTION(OutprocessModuleLoader_Unload_does_nothing_when_moduleLibraryHandle_is_NULL)
{
    // act
    OutprocessModuleLoader_Unload(NULL, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_OUTPROCESS_LOADER_17_011: [ This function shall release all resources created by this loader. ]*/
TEST_FUNCTION(OutprocessModuleLoader_Unload_frees_things)
{
    // arrange
	OUTPROCESS_LOADER_ENTRYPOINT entrypoint = { OUTPROCESS_LOADER_ACTIVATION_NONE, (STRING_HANDLE)0x42, (STRING_HANDLE)0x42};
	MODULE_LOADER loader =
	{
		OUTPROCESS,
		NULL, NULL, NULL
	};

    MODULE_LIBRARY_HANDLE module = OutprocessModuleLoader_Load(&loader, &entrypoint);
    ASSERT_IS_NOT_NULL(module);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    OutprocessModuleLoader_Unload(&loader, module);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/*Tests_SRS_OUTPROCESS_LOADER_17_012: [ This function shall return NULL if json is NULL. ]*/
TEST_FUNCTION(OutprocessModuleLoader_ParseEntrypointFromJson_returns_NULL_when_json_is_NULL)
{
    // act
    void* result = OutprocessModuleLoader_ParseEntrypointFromJson(NULL, NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_OUTPROCESS_LOADER_17_012: [ This function shall return NULL if json is NULL. ]*/
TEST_FUNCTION(OutprocessModuleLoader_ParseEntrypointFromJson_returns_NULL_when_json_is_not_an_object)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONArray);

    // act
    void* result = OutprocessModuleLoader_ParseEntrypointFromJson(NULL, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_OUTPROCESS_LOADER_17_012: [ This function shall return NULL if json is NULL. ]*/
TEST_FUNCTION(OutprocessModuleLoader_ParseEntrypointFromJson_returns_NULL_when_json_value_get_object_returns_NULL)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn(NULL);

    // act
    void* result = OutprocessModuleLoader_ParseEntrypointFromJson(NULL, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


/*Tests_SRS_OUTPROCESS_LOADER_17_013: [ This function shall return NULL if "activation.type" is not present in json. ]*/
TEST_FUNCTION(OutprocessModuleLoader_ParseEntrypointFromJson_returns_NULL_when_json_object_acitvation_type_NULL)
{
    // arrange

	char * activation_type = NULL;
	char * control_id = "a url";

	STRICT_EXPECTED_CALL(json_value_get_type((JSON_Value*)0x42))
		.SetReturn(JSONObject);
	STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x42))
		.SetReturn((JSON_Object*)0x43);
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "activation.type"))
		.SetReturn(activation_type);
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "control.id"))
		.SetReturn(control_id);
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "message.id"))
		.SetReturn(NULL);

    // act
    void* result = OutprocessModuleLoader_ParseEntrypointFromJson(NULL, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_OUTPROCESS_LOADER_17_041: [ This function shall return NULL if "control.id" is not present in json. ]*/
TEST_FUNCTION(OutprocessModuleLoader_ParseEntrypointFromJson_returns_NULL_when_json_object_controlurl_NULL)
{
	// arrange
	char * activation_type = "none";
	char * control_id = "a url";

	STRICT_EXPECTED_CALL(json_value_get_type((JSON_Value*)0x42))
		.SetReturn(JSONObject);
	STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x42))
		.SetReturn((JSON_Object*)0x43);
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "activation.type"))
		.SetReturn(activation_type);
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "control.id"))
		.SetReturn(NULL);
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "message.id"))
		.SetReturn(NULL);

	// act
	void* result = OutprocessModuleLoader_ParseEntrypointFromJson(NULL, (JSON_Value*)0x42);

	// assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_OUTPROCESS_LOADER_17_014: [ This function shall return NULL if "activation.type is not "NONE". ]*/
TEST_FUNCTION(OutprocessModuleLoader_ParseEntrypointFromJson_returns_NULL_when_json_object_type_not_none)
{
	// arrange
	char * activation_type = "forked";
	char * control_id = "a url";

	STRICT_EXPECTED_CALL(json_value_get_type((JSON_Value*)0x42))
		.SetReturn(JSONObject);
	STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x42))
		.SetReturn((JSON_Object*)0x43);
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "activation.type"))
		.SetReturn(activation_type);
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "control.id"))
		.SetReturn(control_id);
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "message.id"))
		.SetReturn(NULL);

	// act
	void* result = OutprocessModuleLoader_ParseEntrypointFromJson(NULL, (JSON_Value*)0x42);

	// assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_OUTPROCESS_LOADER_17_021: [ This function shall return NULL if any calls fails. ]*/
TEST_FUNCTION(OutprocessModuleLoader_ParseEntrypointFromJson_returns_NULL_when_malloc_fails)
{
	// arrange
	char * activation_type = "none";
	char * control_id = "a url";

	STRICT_EXPECTED_CALL(json_value_get_type((JSON_Value*)0x42))
		.SetReturn(JSONObject);
	STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x42))
		.SetReturn((JSON_Object*)0x43);
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "activation.type"))
		.SetReturn(activation_type);
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "control.id"))
		.SetReturn(control_id);
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "message.id"))
		.SetReturn(NULL);
	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(OUTPROCESS_LOADER_ENTRYPOINT)))
		.SetReturn(NULL);


	// act
	void* result = OutprocessModuleLoader_ParseEntrypointFromJson(NULL, (JSON_Value*)0x42);

	// assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_OUTPROCESS_LOADER_17_021: [ This function shall return NULL if any calls fails. ]*/
TEST_FUNCTION(OutprocessModuleLoader_ParseEntrypointFromJson_returns_NULL_when_urlencoding_fails)
{
    // arrange
	char * activation_type = "none";
	char * control_id = "a url";

	STRICT_EXPECTED_CALL(json_value_get_type((JSON_Value*)0x42))
		.SetReturn(JSONObject);
	STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x42))
		.SetReturn((JSON_Object*)0x43);
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "activation.type"))
		.SetReturn(activation_type);
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "control.id"))
		.SetReturn(control_id);
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "message.id"))
		.SetReturn(NULL);
	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(OUTPROCESS_LOADER_ENTRYPOINT)));
	STRICT_EXPECTED_CALL(URL_EncodeString(control_id))
		.SetReturn(NULL);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);


	// act
	void* result = OutprocessModuleLoader_ParseEntrypointFromJson(NULL, (JSON_Value*)0x42);

	// assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_OUTPROCESS_LOADER_17_016: [ This function shall allocate a OUTPROCESS_LOADER_ENTRYPOINT structure. ]*/
/*Tests_SRS_OUTPROCESS_LOADER_17_017: [ This function shall assign the entrypoint activation_type to NONE. ]*/
/*Tests_SRS_OUTPROCESS_LOADER_17_018: [ This function shall assign the entrypoint control_id to the string value of "control.id" in json. ]*/
/*Tests_SRS_OUTPROCESS_LOADER_17_019: [ This function shall assign the entrypoint message_id to the string value of "message.id" in json, NULL if not present. ]*/
/*Tests_SRS_OUTPROCESS_LOADER_17_022: [ This function shall return a valid pointer to an OUTPROCESS_LOADER_ENTRYPOINT on success. ]*/
TEST_FUNCTION(OutprocessModuleLoader_ParseEntrypointFromJson_succeeds)
{
	// arrange
	char * activation_type = "none";
	char * control_id = "a url";

	STRICT_EXPECTED_CALL(json_value_get_type((JSON_Value*)0x42))
		.SetReturn(JSONObject);
	STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x42))
		.SetReturn((JSON_Object*)0x43);
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "activation.type"))
		.SetReturn(activation_type);
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "control.id"))
		.SetReturn(control_id);
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "message.id"))
		.SetReturn(NULL);
	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(OUTPROCESS_LOADER_ENTRYPOINT)));
	STRICT_EXPECTED_CALL(URL_EncodeString(control_id));
	STRICT_EXPECTED_CALL(URL_EncodeString(NULL));

	// act
	void* result = OutprocessModuleLoader_ParseEntrypointFromJson(NULL, (JSON_Value*)0x42);

	// assert
	ASSERT_IS_NOT_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	OutprocessModuleLoader_FreeEntrypoint(NULL, result);
}

/*Tests_SRS_OUTPROCESS_LOADER_17_023: [ This function shall release all resources allocated by OutprocessModuleLoader_ParseEntrypointFromJson. ]*/
TEST_FUNCTION(OutprocessModuleLoader_FreeEntrypoint_does_nothing_when_entrypoint_is_NULL)
{
    // act
    OutprocessModuleLoader_FreeEntrypoint(NULL, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_OUTPROCESS_LOADER_17_023: [ This function shall release all resources allocated by OutprocessModuleLoader_ParseEntrypointFromJson. ]*/
TEST_FUNCTION(OutprocessModuleLoader_FreeEntrypoint_frees_resources)
{
    // arrange
	char * activation_type = "none";
	char * control_id = "a url";
	char * message_id = "another url";

	STRICT_EXPECTED_CALL(json_value_get_type((JSON_Value*)0x42))
		.SetReturn(JSONObject);
	STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x42))
		.SetReturn((JSON_Object*)0x43);
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "activation.type"))
		.SetReturn(activation_type);
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "control.id"))
		.SetReturn(control_id);
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "message.id"))
		.SetReturn(message_id);
	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(OUTPROCESS_LOADER_ENTRYPOINT)));
	STRICT_EXPECTED_CALL(URL_EncodeString(control_id));
	STRICT_EXPECTED_CALL(URL_EncodeString(message_id));

	void* entrypoint = OutprocessModuleLoader_ParseEntrypointFromJson(NULL, (JSON_Value*)0x42);
    ASSERT_IS_NOT_NULL(entrypoint);

    umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    OutprocessModuleLoader_FreeEntrypoint(NULL, entrypoint);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_OUTPROCESS_LOADER_17_024: [ The out of process loader does not have any configuration. So this method shall return NULL. ]*/
TEST_FUNCTION(OutprocessModuleLoader_ParseConfigurationFromJson_returns_NULL)
{
    // arrange

    // act
    MODULE_LOADER_BASE_CONFIGURATION* result = OutprocessModuleLoader_ParseConfigurationFromJson(NULL, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_OUTPROCESS_LOADER_17_025: [ This function shall move along, nothing to free here. ]*/
TEST_FUNCTION(OutprocessModuleLoader_FreeConfiguration_does_nothing)
{
    // act
    OutprocessModuleLoader_FreeConfiguration(NULL, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_OUTPROCESS_LOADER_17_026: [ This function shall return NULL if loader, entrypoint, control_id, or module_configuration is NULL. ]*/
TEST_FUNCTION(OutprocessModuleLoader_BuildModuleConfiguration_returns_null_on_null_entrypoint)
{
    // act
    void* result = OutprocessModuleLoader_BuildModuleConfiguration(NULL, NULL, (void*)0x42);

    // assert
	ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_OUTPROCESS_LOADER_17_026: [ This function shall return NULL if loader, entrypoint, control_id, or module_configuration is NULL. ]*/
TEST_FUNCTION(OutprocessModuleLoader_BuildModuleConfiguration_returns_null_on_null_config)
{
	// act
	void* result = OutprocessModuleLoader_BuildModuleConfiguration(NULL, (void*)0x42, NULL);

	// assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_OUTPROCESS_LOADER_17_033: [ This function shall allocate and copy each string in OUTPROCESS_LOADER_ENTRYPOINT and assign them to the corresponding fields in OUTPROCESS_MODULE_CONFIG. ]*/
/*Tests_SRS_OUTPROCESS_LOADER_17_034: [ This function shall allocate and copy the module_configuration string and assign it the OUTPROCESS_MODULE_CONFIG::outprocess_module_args field. ]*/
/*Tests_SRS_OUTPROCESS_LOADER_17_035: [ Upon success, this function shall return a valid pointer to an OUTPROCESS_MODULE_CONFIG structure. ]*/
/*Tests_SRS_OUTPROCESS_LOADER_17_027: [ This function shall allocate a OUTPROCESS_MODULE_CONFIG structure. ]*/
TEST_FUNCTION(OutprocessModuleLoader_BuildModuleConfiguration_success_with_msg_url)
{
	//arrange
	OUTPROCESS_LOADER_ENTRYPOINT ep =
	{
		OUTPROCESS_LOADER_ACTIVATION_NONE,
		STRING_construct("control_id"),
		STRING_construct("message_id")
	};
	STRING_HANDLE mc = STRING_construct("message config");

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(OUTPROCESS_MODULE_CONFIG)));
	STRICT_EXPECTED_CALL(STRING_c_str(ep.message_id));
	STRICT_EXPECTED_CALL(STRING_c_str(ep.control_id));
	STRICT_EXPECTED_CALL(STRING_clone(mc));

	//act
	void * result = OutprocessModuleLoader_BuildModuleConfiguration(NULL, &ep, mc);
	OUTPROCESS_MODULE_CONFIG *omc = (OUTPROCESS_MODULE_CONFIG*)result;

	//assert
	ASSERT_IS_NOT_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	ASSERT_ARE_EQUAL(char_ptr, STRING_c_str(omc->control_uri), "ipc://control_id.ipc");
	ASSERT_ARE_EQUAL(char_ptr, STRING_c_str(omc->message_uri), "ipc://message_id.ipc");
	ASSERT_ARE_EQUAL(char_ptr, STRING_c_str(omc->outprocess_module_args), STRING_c_str(mc));

	//cleanup
	OutprocessModuleLoader_FreeModuleConfiguration(NULL, result);
	STRING_delete(ep.control_id);
	STRING_delete(ep.message_id);
	STRING_delete(mc);
}

/*Tests_SRS_OUTPROCESS_LOADER_17_029: [ If the entrypoint's message_id is NULL, then the loader shall construct an IPC url. ]*/
/*Tests_SRS_OUTPROCESS_LOADER_17_030: [ The loader shall create a unique id, if needed for URL constrution. ]*/
/*Tests_SRS_OUTPROCESS_LOADER_17_032: [ The message url shall be composed of "ipc://" + unique id. ]*/
TEST_FUNCTION(OutprocessModuleLoader_BuildModuleConfiguration_success_with_no_msg_url)
{
	//arrange
	OUTPROCESS_LOADER_ENTRYPOINT ep =
	{
		OUTPROCESS_LOADER_ACTIVATION_NONE,
		STRING_construct("control_id"),
		NULL,
	};
	STRING_HANDLE mc = STRING_construct("message config");

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(OUTPROCESS_MODULE_CONFIG)));
	STRICT_EXPECTED_CALL(UniqueId_Generate(IGNORED_PTR_ARG, 37))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_c_str(ep.control_id));
	STRICT_EXPECTED_CALL(STRING_clone(mc));

	//act
	void * result = OutprocessModuleLoader_BuildModuleConfiguration(NULL, &ep, mc);

	//assert
	ASSERT_IS_NOT_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//cleanup
	OutprocessModuleLoader_FreeModuleConfiguration(NULL, result);
	STRING_delete(ep.control_id);
	STRING_delete(mc);
}

/*Tests_SRS_OUTPROCESS_LOADER_17_036: [ If any call fails, this function shall return NULL. ]*/
TEST_FUNCTION(OutprocessModuleLoader_BuildModuleConfiguration_returns_null_on_modcfg_clone_fail)
{
	//arrange
	OUTPROCESS_LOADER_ENTRYPOINT ep =
	{
		OUTPROCESS_LOADER_ACTIVATION_NONE,
		STRING_construct("control_id"),
		STRING_construct("message_id"),
	};
	STRING_HANDLE mc = STRING_construct("message config");

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(OUTPROCESS_MODULE_CONFIG)));
	STRICT_EXPECTED_CALL(STRING_c_str(ep.message_id));
	STRICT_EXPECTED_CALL(STRING_c_str(ep.control_id));
	STRICT_EXPECTED_CALL(STRING_clone(mc))
		.SetReturn(NULL);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	//act
	void * result = OutprocessModuleLoader_BuildModuleConfiguration(NULL, &ep, mc);

	//assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//cleanup
	OutprocessModuleLoader_FreeModuleConfiguration(NULL, result);
	STRING_delete(ep.control_id);
	STRING_delete(ep.message_id);
	STRING_delete(mc);
}


/*Tests_SRS_OUTPROCESS_LOADER_17_036: [ If any call fails, this function shall return NULL. ]*/
TEST_FUNCTION(OutprocessModuleLoader_BuildModuleConfiguration_returns_null_on_msg_create_fail)
{
	//arrange
	OUTPROCESS_LOADER_ENTRYPOINT ep =
	{
		OUTPROCESS_LOADER_ACTIVATION_NONE,
		STRING_construct("control_id"),
		NULL,
	};
	STRING_HANDLE mc = STRING_construct("message config");

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(OUTPROCESS_MODULE_CONFIG)));
	STRICT_EXPECTED_CALL(UniqueId_Generate(IGNORED_PTR_ARG, 37))
		.IgnoreArgument(1)
		.SetReturn(UNIQUEID_INVALID_ARG);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	//act
	void * result = OutprocessModuleLoader_BuildModuleConfiguration(NULL, &ep, mc);

	//assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//cleanup
	OutprocessModuleLoader_FreeModuleConfiguration(NULL, result);
	STRING_delete(ep.control_id);
	STRING_delete(mc);
}

/*Tests_SRS_OUTPROCESS_LOADER_17_036: [ If any call fails, this function shall return NULL. ]*/
TEST_FUNCTION(OutprocessModuleLoader_BuildModuleConfiguration_returns_null_on_malloc_fail)
{
	//arrange
	OUTPROCESS_LOADER_ENTRYPOINT ep =
	{
		OUTPROCESS_LOADER_ACTIVATION_NONE,
		STRING_construct("control_id"),
		NULL,
	};
	STRING_HANDLE mc = STRING_construct("message config");

	umock_c_reset_all_calls();
	malloc_will_fail = true;
	malloc_fail_count = 1;
	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(OUTPROCESS_MODULE_CONFIG)));

	//act
	void * result = OutprocessModuleLoader_BuildModuleConfiguration(NULL, &ep, mc);

	//assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//cleanup
	OutprocessModuleLoader_FreeModuleConfiguration(NULL, result);
	STRING_delete(ep.control_id);
	STRING_delete(mc);
}

/*Tests_SRS_OUTPROCESS_LOADER_17_037: [ This function shall release all memory allocated by OutprocessModuleLoader_BuildModuleConfiguration. ]*/
TEST_FUNCTION(OutprocessModuleLoader_FreeModuleConfiguration_does_nothing_with_null)
{
    // act
    OutprocessModuleLoader_FreeModuleConfiguration(NULL, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_OUTPROCESS_LOADER_17_037: [ This function shall release all memory allocated by OutprocessModuleLoader_BuildModuleConfiguration. ]*/
TEST_FUNCTION(OutprocessModuleLoader_FreeModuleConfiguration_frees_stuff)
{
	//arrange
	OUTPROCESS_LOADER_ENTRYPOINT ep =
	{
		OUTPROCESS_LOADER_ACTIVATION_NONE,
		STRING_construct("control_id"),
		NULL,
	};
	STRING_HANDLE mc = STRING_construct("message config");

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(OUTPROCESS_MODULE_CONFIG)));
	STRICT_EXPECTED_CALL(UniqueId_Generate(IGNORED_PTR_ARG, 37))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_clone(ep.control_id));
	STRICT_EXPECTED_CALL(STRING_clone(mc));

	void * result = OutprocessModuleLoader_BuildModuleConfiguration(NULL, &ep, mc);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(result));

	// act
	OutprocessModuleLoader_FreeModuleConfiguration(NULL, result);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//cleanup
	STRING_delete(ep.control_id);
	STRING_delete(mc);
}

/*Tests_SRS_OUTPROCESS_LOADER_17_038: [ OutprocessModuleLoader_Get shall return a non-NULL pointer to a MODULE_LOADER struct. ]*/
/*Tests_SRS_OUTPROCESS_LOADER_17_039: [ MODULE_LOADER::type shall be OUTPROCESS. ]*/
/*Tests_SRS_OUTPROCESS_LOADER_17_040: [ MODULE_LOADER::name shall be the string 'outprocess'. */
TEST_FUNCTION(OutprocessLoader_Get_succeeds)
{
    // act
    const MODULE_LOADER* loader = OutprocessLoader_Get();

    // assert
    ASSERT_IS_NOT_NULL(loader);
    ASSERT_ARE_EQUAL(MODULE_LOADER_TYPE, loader->type, OUTPROCESS);
    ASSERT_IS_TRUE(strcmp(loader->name, "outprocess") == 0);
}

END_TEST_SUITE(OutprocessLoader_UnitTests);
