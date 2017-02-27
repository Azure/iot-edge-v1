// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif

#include "testrunnerswitcher.h"
#include "umock_c.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void my_gballoc_free(void* s)
{
    free(s);
}

#define GATEWAY_EXPORT_H
#define GATEWAY_EXPORT

#define ENABLE_MOCKS
#include "module.h"
#include "module_access.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/httpapiex.h"
#include "message.h"
#include "azure_c_shared_utility/base64.h"
#include "azure_c_shared_utility/gballoc.h"
#include "parson.h"

MOCKABLE_FUNCTION(, JSON_Value*, json_parse_string, const char *, string);
MOCKABLE_FUNCTION(, const char*, json_object_get_string, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, void, json_value_free, JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Object*, json_value_get_object, const JSON_Value *, value);

#undef ENABLE_MOCKS


#include "azure_functions.h"


static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}


BEGIN_TEST_SUITE(azure_functions_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);

    REGISTER_UMOCK_ALIAS_TYPE(MODULE_HANDLE, void*);

    REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_HANDLE, void*);

    REGISTER_UMOCK_ALIAS_TYPE(HTTPAPIEX_HANDLE, void*);

    REGISTER_UMOCK_ALIAS_TYPE(HTTP_HEADERS_HANDLE, void*);

    REGISTER_UMOCK_ALIAS_TYPE(HTTP_HEADERS_RESULT, int);

    REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);

    REGISTER_UMOCK_ALIAS_TYPE(HTTPAPI_REQUEST_TYPE, int);

    REGISTER_UMOCK_ALIAS_TYPE(HTTPAPIEX_RESULT, int);
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
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}


/* Tests_SRS_AZUREFUNCTIONS_04_020: [ Module_GetApi shall return the MODULE_API structure. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Module_GetApi_returns_non_NULL)
{
    // arrange

    // act
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

    //assert
	ASSERT_IS_TRUE(MODULE_PARSE_CONFIGURATION_FROM_JSON(apis) != NULL);
	ASSERT_IS_TRUE(MODULE_FREE_CONFIGURATION(apis) != NULL);
    ASSERT_IS_TRUE(MODULE_CREATE(apis) != NULL);
    ASSERT_IS_TRUE(MODULE_DESTROY(apis) != NULL);
    ASSERT_IS_TRUE(MODULE_RECEIVE(apis) != NULL);
    ASSERT_IS_TRUE(MODULE_START(apis) == NULL);
}


/* Tests_SRS_AZUREFUNCTIONS_05_003: [ If configuration is NULL then Azure_Functions_CreateFromJson shall fail and return NULL. ] */
TEST_FUNCTION(AZUREFUNCTIONS_CreateFromJson_returns_NULL_when_configuration_is_NULL)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);


    // act
    void* result = MODULE_PARSE_CONFIGURATION_FROM_JSON(apis)(NULL);

    //assert
    ASSERT_IS_NULL(result);
}


/* Tests_SRS_AZUREFUNCTIONS_05_005: [ Azure_Functions_CreateFromJson shall parse the configuration as a JSON array of strings. ] */
/* Tests_SRS_AZUREFUNCTIONS_05_004: [ If configuration is not a JSON string, then Azure_Functions_CreateFromJson shall fail and return NULL. ] */
TEST_FUNCTION(AZUREFUNCTIONS_CreateFromJson_returns_NULL_when_configuration_is_not_validJson)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);


    STRICT_EXPECTED_CALL(json_parse_string((const char*)0x42))
        .SetReturn((JSON_Value*)NULL);

    // act
    void* result = MODULE_PARSE_CONFIGURATION_FROM_JSON(apis)((const char*)0x42);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_05_005: [ Azure_Functions_CreateFromJson shall parse the configuration as a JSON array of strings. ] */
/* Tests_SRS_AZUREFUNCTIONS_05_004: [ If configuration is not a JSON string, then Azure_Functions_CreateFromJson shall fail and return NULL. ] */
TEST_FUNCTION(AZUREFUNCTIONS_CreateFromJson_returns_NULL_when_failedToRetrieveJsonObject)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);


    STRICT_EXPECTED_CALL(json_parse_string((const char*)0x42))
        .SetReturn((JSON_Value*)0x42);
    STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x42))
        .SetFailReturn((JSON_Object*)NULL);
    STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x42));

    // act
    void* result = MODULE_PARSE_CONFIGURATION_FROM_JSON(apis)((const char*)0x42);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_05_006: [ If the array object does not contain a value named "hostAddress" then Azure_Functions_CreateFromJson shall fail and return NULL. ] */
TEST_FUNCTION(AZUREFUNCTIONS_CreateFromJson_returns_NULL_when_hostAddress_not_present)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);


    STRICT_EXPECTED_CALL(json_parse_string((const char*)0x42))
        .SetReturn((JSON_Value*)0x42);
    STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x42);

    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, "hostname"))
        .IgnoreArgument(2)
        .SetFailReturn((const char*)NULL);

    STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x42));

    // act
    void* result = MODULE_PARSE_CONFIGURATION_FROM_JSON(apis)((const char*)0x42);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_05_007: [ If the array object does not contain a value named "relativePath" then Azure_Functions_CreateFromJson shall fail and return NULL. ] */
TEST_FUNCTION(AZUREFUNCTIONS_CreateFromJson_returns_NULL_when_helativePath_not_present)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);


    STRICT_EXPECTED_CALL(json_parse_string((const char*)0x42))
        .SetReturn((JSON_Value*)0x42);
    STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x42);

    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, "hostname"))
        .IgnoreArgument(2)
        .SetReturn("HostName42");

    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, "relativePath"))
        .IgnoreArgument(2)
        .SetFailReturn((const char*)NULL);

    STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x42));

    // act
    void* result = MODULE_PARSE_CONFIGURATION_FROM_JSON(apis)((const char*)0x42);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_05_008: [ Azure_Functions_CreateFromJson shall call STRING_construct to create hostAddress based on input host address. ] */
/* Tests_SRS_AZUREFUNCTIONS_05_010: [ If creating the strings fails, then Azure_Functions_CreateFromJson shall fail and return NULL. ] */
TEST_FUNCTION(AZUREFUNCTIONS_CreateFromJson_returns_NULL_when_STRING_construct__hostname_fail)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);


    STRICT_EXPECTED_CALL(json_parse_string((const char*)0x42))
        .SetReturn((JSON_Value*)0x42);
    STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x42);

    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, "hostname"))
        .IgnoreArgument(2)
        .SetReturn("HostName42");

    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, "relativePath"))
        .IgnoreArgument(2)
        .SetReturn("relativePath42");

    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, "key"))
        .IgnoreArgument(2)
        .SetReturn("KeyCode42");

    STRICT_EXPECTED_CALL(STRING_construct((const char *)IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_construct((const char *)IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn(NULL);

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x42));

    // act
    void* result = MODULE_PARSE_CONFIGURATION_FROM_JSON(apis)((const char*)0x42);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_05_009: [ Azure_Functions_CreateFromJson shall call STRING_construct to create relativePath based on input host address. ] */
/* Tests_SRS_AZUREFUNCTIONS_05_010: [ If creating the strings fails, then Azure_Functions_CreateFromJson shall fail and return NULL. ] */
TEST_FUNCTION(AZUREFUNCTIONS_CreateFromJson_returns_NULL_when_STRING_construct__relativepath_fail)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);


    STRICT_EXPECTED_CALL(json_parse_string((const char*)0x42))
        .SetReturn((JSON_Value*)0x42);
    STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x42);

    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, "hostname"))
        .IgnoreArgument(2)
        .SetReturn("HostName42");

    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, "relativePath"))
        .IgnoreArgument(2)
        .SetReturn("relativePath42");

    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, "key"))
         .IgnoreArgument(2)
         .SetReturn(NULL);
 
     STRICT_EXPECTED_CALL(STRING_construct((const char *)IGNORED_PTR_ARG))
         .IgnoreAllArguments()
         .SetReturn(NULL);

    STRICT_EXPECTED_CALL(STRING_construct((const char *)IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_construct((const char *)IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn(NULL);

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
 
    STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x42));

    // act
    void* result = MODULE_PARSE_CONFIGURATION_FROM_JSON(apis)((const char*)0x42);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_AZUREFUNCTIONS_17_001: [ AzureFunctions_ParseConfigurationFromJson shall allocate an AZURE_FUNCTIONS_CONFIG structure. ]*/
/*Tests_SRS_AZUREFUNCTIONS_17_003: [ AzureFunctions_ParseConfigurationFromJson shall return NULL on failure. ]*/
TEST_FUNCTION(AZUREFUNCTIONS_CreateFromJson_returns_NULL_when_alloc_fails)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);


    STRICT_EXPECTED_CALL(json_parse_string((const char*)0x42))
        .SetReturn((JSON_Value*)0x42);
    STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x42);

    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, "hostname"))
        .IgnoreArgument(2)
        .SetReturn("HostName42");

    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, "relativePath"))
        .IgnoreArgument(2)
        .SetReturn("relativePath42");

    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, "key"))
        .IgnoreArgument(2)
        .SetReturn(NULL);
 
    STRICT_EXPECTED_CALL(STRING_construct((const char *)IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(STRING_construct((const char *)IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_construct((const char *)IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(AZURE_FUNCTIONS_CONFIG)))
        .SetReturn(NULL);

    // cleanup after forced failure

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x42));

    // act
    void* result = MODULE_PARSE_CONFIGURATION_FROM_JSON(apis)((const char*)0x42);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_05_019: [ If the array object contains a value named "key" then Azure_Functions_CreateFromJson shall create a securityKey based on input key ] */
/* Tests_SRS_AZUREFUNCTIONS_05_014: [ Azure_Functions_CreateFromJson shall release all data it allocated. ] */
/*Tests_SRS_AZUREFUNCTIONS_17_001: [ AzureFunctions_ParseConfigurationFromJson shall allocate an AZURE_FUNCTIONS_CONFIG structure. ]*/
/*Tests_SRS_AZUREFUNCTIONS_17_002: [ AzureFunctions_ParseConfigurationFromJson shall fill the structure with the constructed strings and return it upon success. ]*/
/* Tests_SRS_AZUREFUNCTIONS_05_009: [ Azure_Functions_CreateFromJson shall call STRING_construct to create relativePath based on input host address. ] */
/* Tests_SRS_AZUREFUNCTIONS_05_008: [ Azure_Functions_CreateFromJson shall call STRING_construct to create hostAddress based on input host address. ] */
/* Tests_SRS_AZUREFUNCTIONS_05_007: [ If the array object does not contain a value named "relativePath" then Azure_Functions_CreateFromJson shall fail and return NULL. ] */
/* Tests_SRS_AZUREFUNCTIONS_05_006: [ If the array object does not contain a value named "hostAddress" then Azure_Functions_CreateFromJson shall fail and return NULL. ] */
/* Tests_SRS_AZUREFUNCTIONS_05_005: [ Azure_Functions_CreateFromJson shall parse the configuration as a JSON array of strings. ] */
TEST_FUNCTION(AZUREFUNCTIONS_CreateFromJson_happy_path)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);


    STRICT_EXPECTED_CALL(json_parse_string((const char*)0x42))
        .SetReturn((JSON_Value*)0x42);

    STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x42);

    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, "hostname"))
        .IgnoreArgument(2)
        .SetReturn("HostName42");

    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, "relativePath"))
        .IgnoreArgument(2)
        .SetReturn("relativePath42");

    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, "key"))
         .IgnoreArgument(2)
         .SetReturn("codeKey42");
 
     STRICT_EXPECTED_CALL(STRING_construct((const char *)IGNORED_PTR_ARG))
         .IgnoreAllArguments()
         .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_construct((const char *)IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_construct((const char *)IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(AZURE_FUNCTIONS_CONFIG)));

    STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x42));

    // act
    void* result = MODULE_PARSE_CONFIGURATION_FROM_JSON(apis)((const char*)0x42);
    //assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    //cleanup
	MODULE_FREE_CONFIGURATION(apis)(result);

}

/*Tests_SRS_AZUREFUNCTIONS_17_004: [ AzureFunctions_FreeConfiguration shall do nothing if configuration is NULL. ]*/
TEST_FUNCTION(AZURE_FUNCTIONS_FreeConfiguration_null_does_nothing)
{
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

	umock_c_reset_all_calls();

	MODULE_FREE_CONFIGURATION(apis)(NULL);

	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_AZUREFUNCTIONS_17_005: [ AzureFunctions_FreeConfiguration shall release all allocated resources if configuration is not NULL. ]*/
TEST_FUNCTION(AZURE_FUNCTIONS_FreeConfiguration_frees_config)
{
	const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

	STRICT_EXPECTED_CALL(json_parse_string((const char*)0x42))
		.SetReturn((JSON_Value*)0x42);

	STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x42))
		.SetReturn((JSON_Object*)0x42);

	STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, "hostname"))
		.IgnoreArgument(2)
		.SetReturn("HostName42");

	STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, "relativePath"))
		.IgnoreArgument(2)
		.SetReturn("relativePath42");

	STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, "key"))
		.IgnoreArgument(2)
		.SetReturn("codeKey42");

	STRICT_EXPECTED_CALL(STRING_construct((const char *)IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_construct((const char *)IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_construct((const char *)IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(AZURE_FUNCTIONS_CONFIG)));

	STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x42));

	// act
	void* result = MODULE_PARSE_CONFIGURATION_FROM_JSON(apis)((const char*)0x42);

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	MODULE_FREE_CONFIGURATION(apis)(result);

	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

}

/* Tests_SRS_AZUREFUNCTIONS_04_001: [ Upon success, this function shall return a valid pointer to a MODULE_HANDLE. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Create_happy_Path_with_key)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);


    AZURE_FUNCTIONS_CONFIG config;
    config.relativePath = (STRING_HANDLE)0x42;
    config.hostAddress = (STRING_HANDLE)0x42;
    config.securityKey = (STRING_HANDLE)0x42;

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(AZURE_FUNCTIONS_CONFIG)));

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

  //act
    MODULE_HANDLE result = MODULE_CREATE(apis)((BROKER_HANDLE)0x42,  (const void*)&config);

    //assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    STRING_delete(config.relativePath);
    STRING_delete(config.hostAddress);
    MODULE_DESTROY(apis)(result);
}


/* Tests_SRS_AZUREFUNCTIONS_04_022: [ if securityKey STRING is NULL AzureFunctions_Create shall do nothing, since this STRING is optional. ] */
/* Tests_SRS_AZUREFUNCTIONS_04_001: [ Upon success, this function shall return a valid pointer to a MODULE_HANDLE. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Create_happy_Path_without_key)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);


    AZURE_FUNCTIONS_CONFIG config;
    config.relativePath = (STRING_HANDLE)0x42;
    config.hostAddress = (STRING_HANDLE)0x42;
    config.securityKey = NULL;

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(AZURE_FUNCTIONS_CONFIG)));

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    //act
    MODULE_HANDLE result = MODULE_CREATE(apis)((BROKER_HANDLE)0x42, (const void*)&config);

    //assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    STRING_delete(config.relativePath);
    STRING_delete(config.hostAddress);
    MODULE_DESTROY(apis)(result);
}



/* Tests_SRS_AZUREFUNCTIONS_04_002: [ If the broker is NULL, this function shall fail and return NULL. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Create_returns_NULL_when_broker_is_NULL)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);


    //act
    MODULE_HANDLE result = MODULE_CREATE(apis)(NULL, (BROKER_HANDLE)0x42);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_04_003: [ If the configuration is NULL, this function shall fail and return NULL. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Create_returns_NULL_when_configuration_is_NULL)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);


    //act
    MODULE_HANDLE result = MODULE_CREATE(apis)((BROKER_HANDLE)0x42, NULL);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_04_004: [ If any hostAddress or relativePath are NULL, this function shall fail and return NULL. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Create_returns_NULL_when_hostAddress_is_NULL)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);


    AZURE_FUNCTIONS_CONFIG config;
    config.relativePath = (STRING_HANDLE)0x42;
    config.hostAddress = NULL;

    umock_c_reset_all_calls();

    //act
    MODULE_HANDLE result = MODULE_CREATE(apis)((BROKER_HANDLE)0x42, (const void*)&config);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_04_004: [ If any hostAddress or relativePath are NULL, this function shall fail and return NULL. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Create_returns_NULL_when_relativePath_is_NULL)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);


    AZURE_FUNCTIONS_CONFIG config;
    config.relativePath = NULL;
    config.hostAddress = (STRING_HANDLE)0x42;

    umock_c_reset_all_calls();

    //act
    MODULE_HANDLE result = MODULE_CREATE(apis)((BROKER_HANDLE)0x42, (const void*)&config);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_04_005: [ If azure_functions_Create fails to allocate a new AZURE_FUNCTIONS_DATA structure, then this function shall fail, and return NULL. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Create_returns_NULL_failed_To_Allocate_Handle)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);


    AZURE_FUNCTIONS_CONFIG config;
    config.relativePath = (STRING_HANDLE)0x42;
    config.hostAddress = (STRING_HANDLE)0x42;
    
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .SetReturn(NULL);

    //act
    MODULE_HANDLE result = MODULE_CREATE(apis)((BROKER_HANDLE)0x42, (const void*)&config);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_04_005: [ If azure_functions_Create fails to allocate a new AZURE_FUNCTIONS_DATA structure, then this function shall fail, and return NULL. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Create_returns_NULL_failed_To_Allocate_configuration)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);


    AZURE_FUNCTIONS_CONFIG config;
    config.relativePath = (STRING_HANDLE)0x42;
    config.hostAddress = (STRING_HANDLE)0x42;

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(AZURE_FUNCTIONS_CONFIG)))
        .SetReturn(NULL);

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreAllArguments();


    //act
    MODULE_HANDLE result = MODULE_CREATE(apis)((BROKER_HANDLE)0x42, (const void*)&config);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_04_006: [ If azure_functions_Create fails to clone STRING for hostAddress, then this function shall fail and return NULL. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Create_returns_NULL_failed_to_clone_hostAddress)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);


    AZURE_FUNCTIONS_CONFIG config;
    config.relativePath = (STRING_HANDLE)0x42;
    config.hostAddress = (STRING_HANDLE)0x42;

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(AZURE_FUNCTIONS_CONFIG)));

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(NULL);

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreAllArguments();


    //act
    MODULE_HANDLE result = MODULE_CREATE(apis)((BROKER_HANDLE)0x42, (const void*)&config);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_04_007: [ If azure_functions_Create fails to clone STRING for relativePath, then this function shall fail and return NULL. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Create_returns_NULL_failed_to_clone_relativePath)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);


    AZURE_FUNCTIONS_CONFIG config;
    config.relativePath = (STRING_HANDLE)0x42;
    config.hostAddress = (STRING_HANDLE)0x42;

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(AZURE_FUNCTIONS_CONFIG)));

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(NULL);

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreAllArguments();


    //act
    MODULE_HANDLE result = MODULE_CREATE(apis)((BROKER_HANDLE)0x42, (const void*)&config);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_04_021: [ If AzureFunctions_Create fails to clone STRING for securityKey, then this function shall fail and return NULL. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Create_returns_NULL_failed_to_clone_securityKey)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

    AZURE_FUNCTIONS_CONFIG config;
    config.relativePath = (STRING_HANDLE)0x42;
    config.hostAddress = (STRING_HANDLE)0x42;
    config.securityKey = (STRING_HANDLE)0x42;

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(AZURE_FUNCTIONS_CONFIG)));

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(NULL);

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreAllArguments();


    //act
    MODULE_HANDLE result = MODULE_CREATE(apis)((BROKER_HANDLE)0x42, (const void*)&config);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_04_008: [ If moduleHandle is NULL, azure_functions_Destroy shall return. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Destroy_does_nothing_if_module_handle_null)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

    MODULE_DESTROY(apis)(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_04_009: [ azure_functions_Destroy shall release all resources allocated for the module. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Destroy_happy_path_with_securityKey)
{

    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

    AZURE_FUNCTIONS_CONFIG config;
    config.relativePath = (STRING_HANDLE)0x42;
    config.hostAddress = (STRING_HANDLE)0x42;
    config.securityKey = (STRING_HANDLE)0x42;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(AZURE_FUNCTIONS_CONFIG)));

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    MODULE_HANDLE result = MODULE_CREATE(apis)((BROKER_HANDLE)0x42, (const void*)&config);
    ASSERT_IS_NOT_NULL(result);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));
    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));
    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreAllArguments();


    //act
    MODULE_DESTROY(apis)(result);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_04_009: [ azure_functions_Destroy shall release all resources allocated for the module. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Destroy_happy_path_without_securityKey)
{

    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

    AZURE_FUNCTIONS_CONFIG config;
    config.relativePath = (STRING_HANDLE)0x42;
    config.hostAddress = (STRING_HANDLE)0x42;
    config.securityKey = NULL;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(AZURE_FUNCTIONS_CONFIG)));

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    MODULE_HANDLE result = MODULE_CREATE(apis)((BROKER_HANDLE)0x42, (const void*)&config);
    ASSERT_IS_NOT_NULL(result);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));
    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));
    STRICT_EXPECTED_CALL(STRING_delete(NULL));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreAllArguments();


    //act
    MODULE_DESTROY(apis)(result);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


/* Tests_SRS_AZUREFUNCTIONS_04_010: [If moduleHandle is NULL than azure_functions_Receive shall fail and return.] */
TEST_FUNCTION(AZURE_FUNCTIONS_Receive_doesNothing_if_moduleHandleIsNull)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

    //act
    MODULE_RECEIVE(apis)(NULL, (MESSAGE_HANDLE)0x42);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_04_011: [ If messageHandle is NULL than azure_functions_Receive shall fail and return. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Receive_doesNothing_if_messageHandleIsNull)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

    //act
    MODULE_RECEIVE(apis)((MODULE_HANDLE)0x42, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_04_012: [ azure_functions_Receive shall get the message content by calling Message_GetContent, if it fails it shall fail and return. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Receive_fails_when_Message_getContent_fail)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

    STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
        .SetReturn(NULL);

    //act
    MODULE_RECEIVE(apis)((MODULE_HANDLE)0x42, (MESSAGE_HANDLE)0x42);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_04_013: [ azure_functions_Receive shall base64 encode by calling Base64_Encode_Bytes, if it fails it shall fail and return. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Receive_fails_when_Base64_Encode_fail)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

    CONSTBUFFER buffer;
    buffer.buffer = (const unsigned char*)"12345";
    buffer.size = sizeof("12345");
    
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
        .SetReturn(&buffer);

    STRICT_EXPECTED_CALL(Base64_Encode_Bytes(buffer.buffer, buffer.size))
        .SetReturn(NULL);

    //act
    MODULE_RECEIVE(apis)((MODULE_HANDLE)0x42, (MESSAGE_HANDLE)0x42);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


/* Tests_SRS_AZUREFUNCTIONS_04_024: [ AzureFunctions_Receive shall create a JSON STRING with the content of the message received. If it fails it shall fail and return. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Receive_fails_when_STRING_construct_fail)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

    CONSTBUFFER buffer;
    buffer.buffer = (const unsigned char*)"12345";
    buffer.size = sizeof("12345");

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
        .SetReturn(&buffer);

    STRICT_EXPECTED_CALL(Base64_Encode_Bytes(buffer.buffer, buffer.size))
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn(NULL);

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));


    //act
    MODULE_RECEIVE(apis)((MODULE_HANDLE)0x42, (MESSAGE_HANDLE)0x42);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_04_024: [ AzureFunctions_Receive shall create a JSON STRING with the content of the message received. If it fails it shall fail and return. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Receive_fails_when_STRING_concat1_fail)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

    CONSTBUFFER buffer;
    buffer.buffer = (const unsigned char*)"12345";
    buffer.size = sizeof("12345");

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
        .SetReturn(&buffer);

    STRICT_EXPECTED_CALL(Base64_Encode_Bytes(buffer.buffer, buffer.size))
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(-1);

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));


    //act
    MODULE_RECEIVE(apis)((MODULE_HANDLE)0x42, (MESSAGE_HANDLE)0x42);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_04_024: [ AzureFunctions_Receive shall create a JSON STRING with the content of the message received. If it fails it shall fail and return. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Receive_fails_when_STRING_concat_with_STRING_fail)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

    CONSTBUFFER buffer;
    buffer.buffer = (const unsigned char*)"12345";
    buffer.size = sizeof("12345");

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
        .SetReturn(&buffer);

    STRICT_EXPECTED_CALL(Base64_Encode_Bytes(buffer.buffer, buffer.size))
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_concat_with_STRING((STRING_HANDLE)0x42, (STRING_HANDLE)0x42))
        .SetReturn(-1);

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));


    //act
    MODULE_RECEIVE(apis)((MODULE_HANDLE)0x42, (MESSAGE_HANDLE)0x42);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_04_024: [ AzureFunctions_Receive shall create a JSON STRING with the content of the message received. If it fails it shall fail and return. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Receive_fails_when_STRING_concat2_fail)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

    CONSTBUFFER buffer;
    buffer.buffer = (const unsigned char*)"12345";
    buffer.size = sizeof("12345");

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
        .SetReturn(&buffer);

    STRICT_EXPECTED_CALL(Base64_Encode_Bytes(buffer.buffer, buffer.size))
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_concat_with_STRING((STRING_HANDLE)0x42, (STRING_HANDLE)0x42))
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(-1);

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));


    //act
    MODULE_RECEIVE(apis)((MODULE_HANDLE)0x42, (MESSAGE_HANDLE)0x42);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_04_014: [ azure_functions_Receive shall call HTTPAPIEX_Create, passing hostAddress, it if fails it shall fail and return. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Receive_fail_when_HTTPAPIEX_create_Fail)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

    CONSTBUFFER buffer;
    buffer.buffer = (const unsigned char*)"12345";
    buffer.size = sizeof("12345");
    
    AZURE_FUNCTIONS_CONFIG config;
    config.relativePath = (STRING_HANDLE)0x42;
    config.hostAddress = (STRING_HANDLE)0x42;
    config.securityKey = (STRING_HANDLE)0x42;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(AZURE_FUNCTIONS_CONFIG)));

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    MODULE_HANDLE moduleInfo = MODULE_CREATE(apis)((BROKER_HANDLE)0x42, &config);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
        .SetReturn(&buffer);

    STRICT_EXPECTED_CALL(Base64_Encode_Bytes(buffer.buffer, buffer.size))
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_concat_with_STRING((STRING_HANDLE)0x42, (STRING_HANDLE)0x42))
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_c_str((STRING_HANDLE)0x42))
        .SetReturn("AnyContent42");

    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn(NULL);

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));


    //act
    MODULE_RECEIVE(apis)(moduleInfo, (MESSAGE_HANDLE)0x42);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    MODULE_DESTROY(apis)(moduleInfo);
}

/* Tests_SRS_AZUREFUNCTIONS_04_015: [ azure_functions_Receive shall call allocate memory to receive data from HTTPAPI by calling BUFFER_new, if it fail it shall fail and return. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Receive_fail_if_buffer_new_fails)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

    CONSTBUFFER buffer;
    buffer.buffer = (const unsigned char*)"12345";
    buffer.size = sizeof("12345");

    AZURE_FUNCTIONS_CONFIG config;
    config.relativePath = (STRING_HANDLE)0x42;
    config.hostAddress = (STRING_HANDLE)0x42;
    config.securityKey = (STRING_HANDLE)0x42;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(AZURE_FUNCTIONS_CONFIG)));

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    MODULE_HANDLE moduleInfo = MODULE_CREATE(apis)((BROKER_HANDLE)0x42, &config);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
        .SetReturn(&buffer);

    STRICT_EXPECTED_CALL(Base64_Encode_Bytes(buffer.buffer, buffer.size))
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_concat_with_STRING((STRING_HANDLE)0x42, (STRING_HANDLE)0x42))
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_c_str((STRING_HANDLE)0x42))
        .SetReturn("AnyContent42");

    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((HTTPAPIEX_HANDLE)0x42);

    STRICT_EXPECTED_CALL(BUFFER_new())
        .SetReturn(NULL);

    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy((HTTPAPIEX_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));


    //act
    MODULE_RECEIVE(apis)(moduleInfo, (MESSAGE_HANDLE)0x42);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup 
    MODULE_DESTROY(apis)(moduleInfo);
}

/* Tests_SRS_AZUREFUNCTIONS_04_016: [ azure_functions_Receive shall add name parameter to relative path, if it fail it shall fail and return. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Receive_fail_when_String_clone_fails)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

    CONSTBUFFER buffer;
    buffer.buffer = (const unsigned char*)"12345";
    buffer.size = sizeof("12345");

    AZURE_FUNCTIONS_CONFIG config;
    config.relativePath = (STRING_HANDLE)0x42;
    config.hostAddress = (STRING_HANDLE)0x42;
    config.securityKey = (STRING_HANDLE)0x42;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(AZURE_FUNCTIONS_CONFIG)));

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    MODULE_HANDLE moduleInfo = MODULE_CREATE(apis)((BROKER_HANDLE)0x42, &config);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
        .SetReturn(&buffer);

    STRICT_EXPECTED_CALL(Base64_Encode_Bytes(buffer.buffer, buffer.size))
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_concat_with_STRING((STRING_HANDLE)0x42, (STRING_HANDLE)0x42))
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_c_str((STRING_HANDLE)0x42))
        .SetReturn("AnyContent42");

    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((HTTPAPIEX_HANDLE)0x42);

    STRICT_EXPECTED_CALL(BUFFER_new())
        .SetReturn((BUFFER_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)0x42))
        .SetReturn(NULL);

    STRICT_EXPECTED_CALL(BUFFER_delete((BUFFER_HANDLE)0x42));


    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy((HTTPAPIEX_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));


    //act
    MODULE_RECEIVE(apis)(moduleInfo, (MESSAGE_HANDLE)0x42);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup 
    MODULE_DESTROY(apis)(moduleInfo);
}

/* Tests_SRS_AZUREFUNCTIONS_04_016: [ azure_functions_Receive shall add name parameter to relative path, if it fail it shall fail and return. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Receive_fail_when_String_concat1_fails)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

    CONSTBUFFER buffer;
    buffer.buffer = (const unsigned char*)"12345";
    buffer.size = sizeof("12345");

    AZURE_FUNCTIONS_CONFIG config;
    config.relativePath = (STRING_HANDLE)0x42;
    config.hostAddress = (STRING_HANDLE)0x42;
    config.securityKey = (STRING_HANDLE)0x42;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(AZURE_FUNCTIONS_CONFIG)));

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    MODULE_HANDLE moduleInfo = MODULE_CREATE(apis)((BROKER_HANDLE)0x42, &config);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
        .SetReturn(&buffer);

    STRICT_EXPECTED_CALL(Base64_Encode_Bytes(buffer.buffer, buffer.size))
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_concat_with_STRING((STRING_HANDLE)0x42, (STRING_HANDLE)0x42))
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_c_str((STRING_HANDLE)0x42))
        .SetReturn("AnyContent42");

    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((HTTPAPIEX_HANDLE)0x42);

    STRICT_EXPECTED_CALL(BUFFER_new())
        .SetReturn((BUFFER_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)0x42))
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(-1);

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    STRICT_EXPECTED_CALL(BUFFER_delete((BUFFER_HANDLE)0x42));

    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy((HTTPAPIEX_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));


    //act
    MODULE_RECEIVE(apis)(moduleInfo, (MESSAGE_HANDLE)0x42);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup 
    MODULE_DESTROY(apis)(moduleInfo);
}

/* Tests_SRS_AZUREFUNCTIONS_04_025: [ AzureFunctions_Receive shall add 2 HTTP Headers to POST Request. Content-Type:application/json and, if securityKey exists x-functions-key:securityKey. If it fails it shall fail and return. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Receive_fail_when_HTTPHeaders_Alloc_fails)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

    CONSTBUFFER buffer;
    buffer.buffer = (const unsigned char*)"12345";
    buffer.size = sizeof("12345");

    AZURE_FUNCTIONS_CONFIG config;
    config.relativePath = (STRING_HANDLE)0x42;
    config.hostAddress = (STRING_HANDLE)0x42;
    config.securityKey = (STRING_HANDLE)0x42;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(AZURE_FUNCTIONS_CONFIG)));

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    MODULE_HANDLE moduleInfo = MODULE_CREATE(apis)((BROKER_HANDLE)0x42, &config);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
        .SetReturn(&buffer);

    STRICT_EXPECTED_CALL(Base64_Encode_Bytes(buffer.buffer, buffer.size))
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_concat_with_STRING((STRING_HANDLE)0x42, (STRING_HANDLE)0x42))
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_c_str((STRING_HANDLE)0x42))
        .SetReturn("AnyContent42");

    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((HTTPAPIEX_HANDLE)0x42);

    STRICT_EXPECTED_CALL(BUFFER_new())
        .SetReturn((BUFFER_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)0x42))
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc())
        .SetReturn(NULL);

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    STRICT_EXPECTED_CALL(BUFFER_delete((BUFFER_HANDLE)0x42));

    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy((HTTPAPIEX_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));


    //act
    MODULE_RECEIVE(apis)(moduleInfo, (MESSAGE_HANDLE)0x42);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup 
    MODULE_DESTROY(apis)(moduleInfo);
}

/* Tests_SRS_AZUREFUNCTIONS_04_025: [ AzureFunctions_Receive shall add 2 HTTP Headers to POST Request. Content-Type:application/json and, if securityKey exists x-functions-key:securityKey. If it fails it shall fail and return. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Receive_fail_when_HTTPHeaders_AddHeaderNameValuePair_fails)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

    CONSTBUFFER buffer;
    buffer.buffer = (const unsigned char*)"12345";
    buffer.size = sizeof("12345");

    AZURE_FUNCTIONS_CONFIG config;
    config.relativePath = (STRING_HANDLE)0x42;
    config.hostAddress = (STRING_HANDLE)0x42;
    config.securityKey = (STRING_HANDLE)0x42;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(AZURE_FUNCTIONS_CONFIG)));

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    MODULE_HANDLE moduleInfo = MODULE_CREATE(apis)((BROKER_HANDLE)0x42, &config);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
        .SetReturn(&buffer);

    STRICT_EXPECTED_CALL(Base64_Encode_Bytes(buffer.buffer, buffer.size))
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_concat_with_STRING((STRING_HANDLE)0x42, (STRING_HANDLE)0x42))
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_c_str((STRING_HANDLE)0x42))
        .SetReturn("AnyContent42");

    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((HTTPAPIEX_HANDLE)0x42);

    STRICT_EXPECTED_CALL(BUFFER_new())
        .SetReturn((BUFFER_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)0x42))
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc())
        .SetReturn((HTTP_HEADERS_HANDLE)0x42);

    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair((HTTP_HEADERS_HANDLE)0x42, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .SetReturn(HTTP_HEADERS_ERROR);

    STRICT_EXPECTED_CALL(HTTPHeaders_Free((HTTP_HEADERS_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    STRICT_EXPECTED_CALL(BUFFER_delete((BUFFER_HANDLE)0x42));

    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy((HTTPAPIEX_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    //act
    MODULE_RECEIVE(apis)(moduleInfo, (MESSAGE_HANDLE)0x42);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup 
    MODULE_DESTROY(apis)(moduleInfo);
}

/* Tests_SRS_AZUREFUNCTIONS_04_025: [ AzureFunctions_Receive shall add 2 HTTP Headers to POST Request. Content-Type:application/json and, if securityKey exists x-functions-key:securityKey. If it fails it shall fail and return. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Receive_fail_when_HTTPHeaders_AddHeaderNameValuePair2_fails)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

    CONSTBUFFER buffer;
    buffer.buffer = (const unsigned char*)"12345";
    buffer.size = sizeof("12345");

    AZURE_FUNCTIONS_CONFIG config;
    config.relativePath = (STRING_HANDLE)0x42;
    config.hostAddress = (STRING_HANDLE)0x42;
    config.securityKey = (STRING_HANDLE)0x42;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(AZURE_FUNCTIONS_CONFIG)));

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    MODULE_HANDLE moduleInfo = MODULE_CREATE(apis)((BROKER_HANDLE)0x42, &config);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
        .SetReturn(&buffer);

    STRICT_EXPECTED_CALL(Base64_Encode_Bytes(buffer.buffer, buffer.size))
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_concat_with_STRING((STRING_HANDLE)0x42, (STRING_HANDLE)0x42))
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_c_str((STRING_HANDLE)0x42))
        .SetReturn("AnyContent42");

    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((HTTPAPIEX_HANDLE)0x42);

    STRICT_EXPECTED_CALL(BUFFER_new())
        .SetReturn((BUFFER_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)0x42))
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc())
        .SetReturn((HTTP_HEADERS_HANDLE)0x42);

    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair((HTTP_HEADERS_HANDLE)0x42, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .SetReturn(HTTP_HEADERS_OK);

    STRICT_EXPECTED_CALL(STRING_c_str((STRING_HANDLE)0x42))
        .SetReturn("AnyContent42");

    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair((HTTP_HEADERS_HANDLE)0x42, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .SetReturn(HTTP_HEADERS_ERROR);

    STRICT_EXPECTED_CALL(HTTPHeaders_Free((HTTP_HEADERS_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    STRICT_EXPECTED_CALL(BUFFER_delete((BUFFER_HANDLE)0x42));

    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy((HTTPAPIEX_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    //act
    MODULE_RECEIVE(apis)(moduleInfo, (MESSAGE_HANDLE)0x42);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup 
    MODULE_DESTROY(apis)(moduleInfo);
}

/* Tests_SRS_AZUREFUNCTIONS_04_017: [ azure_functions_Receive shall HTTPAPIEX_ExecuteRequest to send the HTTP POST to Azure Functions. If it fail it shall fail and return. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Receive_fail_when_BUFFER_create_PostContent_fails)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

    CONSTBUFFER buffer;
    buffer.buffer = (const unsigned char*)"12345";
    buffer.size = sizeof("12345");

    AZURE_FUNCTIONS_CONFIG config;
    config.relativePath = (STRING_HANDLE)0x42;
    config.hostAddress = (STRING_HANDLE)0x42;
    config.securityKey = (STRING_HANDLE)0x42;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(AZURE_FUNCTIONS_CONFIG)));

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    MODULE_HANDLE moduleInfo = MODULE_CREATE(apis)((BROKER_HANDLE)0x42, &config);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
        .SetReturn(&buffer);

    STRICT_EXPECTED_CALL(Base64_Encode_Bytes(buffer.buffer, buffer.size))
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_concat_with_STRING((STRING_HANDLE)0x42, (STRING_HANDLE)0x42))
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_c_str((STRING_HANDLE)0x42))
        .SetReturn("AnyContent42");

    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((HTTPAPIEX_HANDLE)0x42);

    STRICT_EXPECTED_CALL(BUFFER_new())
        .SetReturn((BUFFER_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)0x42))
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc())
        .SetReturn((HTTP_HEADERS_HANDLE)0x42);

    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair((HTTP_HEADERS_HANDLE)0x42, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .SetReturn(HTTP_HEADERS_OK);

    STRICT_EXPECTED_CALL(STRING_c_str((STRING_HANDLE)0x42))
        .SetReturn("AnyContent42");

    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair((HTTP_HEADERS_HANDLE)0x42, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .SetReturn(HTTP_HEADERS_OK);


    STRICT_EXPECTED_CALL(STRING_length((STRING_HANDLE)0x42))
        .SetReturn(42);

    STRICT_EXPECTED_CALL(STRING_c_str((STRING_HANDLE)0x42))
        .SetReturn("AnyContent42");

    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreAllArguments()
        .SetReturn(NULL);

    STRICT_EXPECTED_CALL(HTTPHeaders_Free((HTTP_HEADERS_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    STRICT_EXPECTED_CALL(BUFFER_delete((BUFFER_HANDLE)0x42));

    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy((HTTPAPIEX_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));


    //act
    MODULE_RECEIVE(apis)(moduleInfo, (MESSAGE_HANDLE)0x42);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup 
    MODULE_DESTROY(apis)(moduleInfo);
}

/* Tests_SRS_AZUREFUNCTIONS_04_017: [ azure_functions_Receive shall HTTPAPIEX_ExecuteRequest to send the HTTP POST to Azure Functions. If it fail it shall fail and return. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Receive_fail_when_httpapiex_executeRequest_fail)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

    CONSTBUFFER buffer;
    buffer.buffer = (const unsigned char*)"12345";
    buffer.size = sizeof("12345");

    AZURE_FUNCTIONS_CONFIG config;
    config.relativePath = (STRING_HANDLE)0x42;
    config.hostAddress = (STRING_HANDLE)0x42;
    config.securityKey = (STRING_HANDLE)0x42;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(AZURE_FUNCTIONS_CONFIG)));

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    MODULE_HANDLE moduleInfo = MODULE_CREATE(apis)((BROKER_HANDLE)0x42, &config);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
        .SetReturn(&buffer);

    STRICT_EXPECTED_CALL(Base64_Encode_Bytes(buffer.buffer, buffer.size))
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_concat_with_STRING((STRING_HANDLE)0x42, (STRING_HANDLE)0x42))
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_c_str((STRING_HANDLE)0x42))
        .SetReturn("AnyContent42");

    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((HTTPAPIEX_HANDLE)0x42);

    STRICT_EXPECTED_CALL(BUFFER_new())
        .SetReturn((BUFFER_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)0x42))
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc())
        .SetReturn((HTTP_HEADERS_HANDLE)0x42);

    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair((HTTP_HEADERS_HANDLE)0x42, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .SetReturn(HTTP_HEADERS_OK);

    STRICT_EXPECTED_CALL(STRING_c_str((STRING_HANDLE)0x42))
        .SetReturn("AnyContent42");

    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair((HTTP_HEADERS_HANDLE)0x42, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .SetReturn(HTTP_HEADERS_OK);


    STRICT_EXPECTED_CALL(STRING_length((STRING_HANDLE)0x42))
        .SetReturn(42);

    STRICT_EXPECTED_CALL(STRING_c_str((STRING_HANDLE)0x42))
        .SetReturn("AnyContent42");

    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreAllArguments()
        .SetReturn((BUFFER_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_c_str((STRING_HANDLE)0x42))
        .SetReturn("AnyContent42");

    STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest((HTTPAPIEX_HANDLE)0x42, HTTPAPI_REQUEST_POST, IGNORED_PTR_ARG, (HTTP_HEADERS_HANDLE)0x42, (BUFFER_HANDLE)0x42, IGNORED_PTR_ARG, NULL, (BUFFER_HANDLE)0x42))
        .IgnoreArgument(3)
        .IgnoreArgument(6)
        .SetReturn(HTTPAPIEX_ERROR);

    STRICT_EXPECTED_CALL(BUFFER_delete((BUFFER_HANDLE)0x42));

    STRICT_EXPECTED_CALL(HTTPHeaders_Free((HTTP_HEADERS_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    STRICT_EXPECTED_CALL(BUFFER_delete((BUFFER_HANDLE)0x42));

    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy((HTTPAPIEX_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));


    //act
    MODULE_RECEIVE(apis)(moduleInfo, (MESSAGE_HANDLE)0x42);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup 
    MODULE_DESTROY(apis)(moduleInfo);
}

/* Tests_SRS_AZUREFUNCTIONS_04_025: [ AzureFunctions_Receive shall add 2 HTTP Headers to POST Request. Content-Type:application/json and, if securityKey exists x-functions-key:securityKey. If it fails it shall fail and return. ] */
/* Tests_SRS_AZUREFUNCTIONS_04_024: [ AzureFunctions_Receive shall create a JSON STRING with the content of the message received. If it fails it shall fail and return. ] */
/* Tests_SRS_AZUREFUNCTIONS_04_019: [ azure_functions_Receive shall destroy any allocated memory before returning. ] */
/* Tests_SRS_AZUREFUNCTIONS_04_018: [ Upon success azure_functions_Receive shall log the response from HTTP POST and return. ] */
/* Tests_SRS_AZUREFUNCTIONS_04_017: [ azure_functions_Receive shall HTTPAPIEX_ExecuteRequest to send the HTTP POST to Azure Functions. If it fail it shall fail and return. ] */
/* Tests_SRS_AZUREFUNCTIONS_04_016: [ azure_functions_Receive shall add name and content parameter to relative path, if it fail it shall fail and return. ] */
/* Tests_SRS_AZUREFUNCTIONS_04_015: [ azure_functions_Receive shall call allocate memory to receive data from HTTPAPI by calling BUFFER_new, if it fail it shall fail and return. ] */
/* Tests_SRS_AZUREFUNCTIONS_04_014: [ azure_functions_Receive shall call HTTPAPIEX_Create, passing hostAddress, it if fails it shall fail and return. ] */
/* Tests_SRS_AZUREFUNCTIONS_04_013: [ azure_functions_Receive shall base64 encode by calling Base64_Encode_Bytes, if it fails it shall fail and return. ] */
/* Tests_SRS_AZUREFUNCTIONS_04_012: [ azure_functions_Receive shall get the message content by calling Message_GetContent, if it fails it shall fail and return. ] */
TEST_FUNCTION(AZURE_FUNCTIONS_Receive_happy_path)
{
    // arrange
    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

    CONSTBUFFER buffer;
    buffer.buffer = (const unsigned char*)"12345";
    buffer.size = sizeof("12345");

    AZURE_FUNCTIONS_CONFIG config;
    config.relativePath = (STRING_HANDLE)0x42;
    config.hostAddress = (STRING_HANDLE)0x42;
    config.securityKey = (STRING_HANDLE)0x42;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(AZURE_FUNCTIONS_CONFIG)));

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((STRING_HANDLE)0x42);

    MODULE_HANDLE moduleInfo = MODULE_CREATE(apis)((BROKER_HANDLE)0x42, &config);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
        .SetReturn(&buffer);

    STRICT_EXPECTED_CALL(Base64_Encode_Bytes(buffer.buffer, buffer.size))
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_concat_with_STRING((STRING_HANDLE)0x42, (STRING_HANDLE)0x42))
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_c_str((STRING_HANDLE)0x42))
        .SetReturn("AnyContent42");

    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((HTTPAPIEX_HANDLE)0x42);

    STRICT_EXPECTED_CALL(BUFFER_new())
        .SetReturn((BUFFER_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)0x42))
        .SetReturn((STRING_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(0);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc())
        .SetReturn((HTTP_HEADERS_HANDLE)0x42);

    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair((HTTP_HEADERS_HANDLE)0x42, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .SetReturn(HTTP_HEADERS_OK);

    STRICT_EXPECTED_CALL(STRING_c_str((STRING_HANDLE)0x42))
        .SetReturn("AnyContent42");

    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair((HTTP_HEADERS_HANDLE)0x42, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .SetReturn(HTTP_HEADERS_OK);


    STRICT_EXPECTED_CALL(STRING_length((STRING_HANDLE)0x42))
        .SetReturn(42);

    STRICT_EXPECTED_CALL(STRING_c_str((STRING_HANDLE)0x42))
        .SetReturn("AnyContent42");

    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreAllArguments()
        .SetReturn((BUFFER_HANDLE)0x42);

    STRICT_EXPECTED_CALL(STRING_c_str((STRING_HANDLE)0x42))
        .SetReturn("AnyContent42");

    STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest((HTTPAPIEX_HANDLE)0x42, HTTPAPI_REQUEST_POST, IGNORED_PTR_ARG, (HTTP_HEADERS_HANDLE)0x42, (BUFFER_HANDLE)0x42, IGNORED_PTR_ARG, NULL, (BUFFER_HANDLE)0x42))
        .IgnoreArgument(3)
        .IgnoreArgument(6)
        .SetReturn(HTTPAPIEX_OK);

    STRICT_EXPECTED_CALL(BUFFER_delete((BUFFER_HANDLE)0x42));

    STRICT_EXPECTED_CALL(HTTPHeaders_Free((HTTP_HEADERS_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));
    
    STRICT_EXPECTED_CALL(BUFFER_delete((BUFFER_HANDLE)0x42));

    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy((HTTPAPIEX_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

    //act
    MODULE_RECEIVE(apis)(moduleInfo, (MESSAGE_HANDLE)0x42);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup 
    MODULE_DESTROY(apis)(moduleInfo);
}

END_TEST_SUITE(azure_functions_ut)
