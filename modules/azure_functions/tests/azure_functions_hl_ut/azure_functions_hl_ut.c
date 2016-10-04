// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "umock_c.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif


#define ENABLE_MOCKS

#include "azure_c_shared_utility/strings.h"
#include "parson.h"
#include "azure_functions.h"
#include "module.h"

/*forward declarations*/

MODULE_HANDLE azure_functions_Create(BROKER_HANDLE broker, const void* configuration);

/*this destroys (frees resources) of the module parameter*/
void azure_functions_Destroy(MODULE_HANDLE moduleHandle);

/*this is the module's callback function - gets called when a message is to be received by the module*/
void azure_functions_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle);

static MODULE_APIS azure_functions_APIS_all =
{
	azure_functions_Create,
	azure_functions_Destroy,
	azure_functions_Receive
};


MOCKABLE_FUNCTION(, JSON_Value*, json_parse_string, const char *, string);
MOCKABLE_FUNCTION(, const char*, json_object_get_string, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, void, json_value_free, JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Object*, json_value_get_object, const JSON_Value *, value);


MOCKABLE_FUNCTION(, MODULE_HANDLE, azure_functions_Create, BROKER_HANDLE, broker, const void*, configuration);

MOCKABLE_FUNCTION(, void, azure_functions_Destroy, MODULE_HANDLE, moduleHandle);

MOCKABLE_FUNCTION(, void, azure_functions_Receive, MODULE_HANDLE, moduleHandle, MESSAGE_HANDLE, messageHandle);


MOCK_FUNCTION_WITH_CODE(, void, MODULE_STATIC_GETAPIS(AZUREFUNCTIONS_MODULE), MODULE_APIS*, apis)
     (*apis) = azure_functions_APIS_all;
MOCK_FUNCTION_END();



#undef ENABLE_MOCKS

#include "azure_functions_hl.h"



static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
	char temp_str[256];
	(void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
	ASSERT_FAIL(temp_str);
}


BEGIN_TEST_SUITE(azure_functions_hl_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
	TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
	g_testByTest = TEST_MUTEX_CREATE();
	ASSERT_IS_NOT_NULL(g_testByTest);

	umock_c_init(on_umock_c_error);

	REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);

	REGISTER_UMOCK_ALIAS_TYPE(BROKER_HANDLE, void*);
	
	REGISTER_UMOCK_ALIAS_TYPE(MODULE_HANDLE, void*);

	REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_HANDLE, void*);

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

/* Tests_SRS_AZUREFUNCTIONS_HL_04_001: [ Module_GetAPIS shall fill the provided MODULE_APIS function with the required function pointers. ] */
TEST_FUNCTION(AZUREFUNCTIONS_HL_Module_GetAPIS_returns_non_NULL)
{
	// arrange

	// act
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	//assert
	ASSERT_IS_TRUE(apis.Module_Destroy != NULL);
	ASSERT_IS_TRUE(apis.Module_Create != NULL);
	ASSERT_IS_TRUE(apis.Module_Receive != NULL);
}

/* Tests_SRS_AZUREFUNCTIONS_HL_04_002: [ If broker is NULL then Azure_Functions_HL_Create shall fail and return NULL. ] */
TEST_FUNCTION(AZUREFUNCTIONS_HL_Create_returns_NULL_when_broker_is_NULL)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	// act
	MODULE_HANDLE result = apis.Module_Create(NULL, (const void*)0x42);

	//assert
	ASSERT_IS_NULL(result);
}

/* Tests_SRS_AZUREFUNCTIONS_HL_04_003: [ If configuration is NULL then Azure_Functions_HL_Create shall fail and return NULL. ] */
TEST_FUNCTION(AZUREFUNCTIONS_HL_Create_returns_NULL_when_configuration_is_NULL)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	// act
	MODULE_HANDLE result = apis.Module_Create((BROKER_HANDLE)0x42, NULL);
	
	//assert
	ASSERT_IS_NULL(result);
}


/* Tests_SRS_AZUREFUNCTIONS_HL_04_005: [ Azure_Functions_HL_Create shall parse the configuration as a JSON array of strings. ] */
/* Tests_SRS_AZUREFUNCTIONS_HL_04_004: [ If configuration is not a JSON string, then Azure_Functions_HL_Create shall fail and return NULL. ] */
TEST_FUNCTION(AZUREFUNCTIONS_HL_Create_returns_NULL_when_configuration_is_not_validJson)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	STRICT_EXPECTED_CALL(json_parse_string((const char*) 0x42))
		.SetReturn((JSON_Value*)NULL);

	// act
	MODULE_HANDLE result = apis.Module_Create((BROKER_HANDLE)0x42, (const void*) 0x42);

	//assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_HL_04_005: [ Azure_Functions_HL_Create shall parse the configuration as a JSON array of strings. ] */
/* Tests_SRS_AZUREFUNCTIONS_HL_04_004: [ If configuration is not a JSON string, then Azure_Functions_HL_Create shall fail and return NULL. ] */
TEST_FUNCTION(AZUREFUNCTIONS_HL_Create_returns_NULL_when_failedToRetrieveJsonObject)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	STRICT_EXPECTED_CALL(json_parse_string((const char*)0x42))
		.SetReturn((JSON_Value*)0x42);
	STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x42))
		.SetFailReturn((JSON_Object*) NULL);
	STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x42));

	// act
	MODULE_HANDLE result = apis.Module_Create((BROKER_HANDLE)0x42, (const void*)0x42);

	//assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_HL_04_006: [ If the array object does not contain a value named "hostAddress" then Azure_Functions_HL_Create shall fail and return NULL. ] */
TEST_FUNCTION(AZUREFUNCTIONS_HL_Create_returns_NULL_when_hostAddress_not_present)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	STRICT_EXPECTED_CALL(json_parse_string((const char*)0x42))
		.SetReturn((JSON_Value*)0x42);
	STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x42))
		.SetReturn((JSON_Object*)0x42);

	STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, "hostname"))
		.IgnoreArgument(2)
		.SetFailReturn((const char*)NULL);

	STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x42));

	// act
	MODULE_HANDLE result = apis.Module_Create((BROKER_HANDLE)0x42, (const void*)0x42);

	//assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_HL_04_007: [ If the array object does not contain a value named "relativePath" then Azure_Functions_HL_Create shall fail and return NULL. ] */
TEST_FUNCTION(AZUREFUNCTIONS_HL_Create_returns_NULL_when_helativePath_not_present)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

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
	MODULE_HANDLE result = apis.Module_Create((BROKER_HANDLE)0x42, (const void*)0x42);

	//assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_HL_04_008: [ Azure_Functions_HL_Create shall call STRING_construct to create hostAddress based on input host address. ] */
/* Tests_SRS_AZUREFUNCTIONS_HL_04_010: [ If creating the strings fails, then Azure_Functions_HL_Create shall fail and return NULL. ] */
TEST_FUNCTION(AZUREFUNCTIONS_HL_Create_returns_NULL_when_STRING_construct__hostname_fail)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

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
	MODULE_HANDLE result = apis.Module_Create((BROKER_HANDLE)0x42, (const void*)0x42);

	//assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_HL_04_009: [ Azure_Functions_HL_Create shall call STRING_construct to create relativePath based on input host address. ] */
/* Tests_SRS_AZUREFUNCTIONS_HL_04_010: [ If creating the strings fails, then Azure_Functions_HL_Create shall fail and return NULL. ] */
TEST_FUNCTION(AZUREFUNCTIONS_HL_Create_returns_NULL_when_STRING_construct__relativepath_fail)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

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

	STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

	STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x42));

	// act
	MODULE_HANDLE result = apis.Module_Create((BROKER_HANDLE)0x42, (const void*)0x42);

	//assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_HL_04_011: [ Azure_Functions_HL_Create shall invoke Azure Functions module's create, passing in the message broker handle and the Azure_Functions_CONFIG. ] */
/* Tests_SRS_AZUREFUNCTIONS_HL_04_013: [ If the lower layer Azure Functions module create fails, Azure_Functions_HL_Create shall fail and return NULL. ] */
TEST_FUNCTION(AZUREFUNCTIONS_HL_Create_returns_NULL_when_ModuleCreate_fail)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

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

	STRICT_EXPECTED_CALL(MODULE_STATIC_GETAPIS(AZUREFUNCTIONS_MODULE)(IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	STRICT_EXPECTED_CALL(azure_functions_Create((BROKER_HANDLE)0x42, (const void*)0x42))
		.IgnoreArgument(2)
		.SetFailReturn(NULL);

	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x42));

	// act
	MODULE_HANDLE result = apis.Module_Create((BROKER_HANDLE)0x42, (const void*)0x42);

	//assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_HL_04_019: [ If the array object contains a value named "key" then Azure_Functions_HL_Create shall create a securityKey based on input key ] */
/* Tests_SRS_AZUREFUNCTIONS_HL_04_014: [ Azure_Functions_HL_Create shall release all data it allocated. ] */
/* Tests_SRS_AZUREFUNCTIONS_HL_04_012: [ When the lower layer Azure Functions module create succeeds, Azure_Functions_HL_Create shall succeed and return a non-NULL value. ] */
/* Tests_SRS_AZUREFUNCTIONS_HL_04_011: [ Azure_Functions_HL_Create shall invoke Azure Functions module's create, passing in the message broker handle and the Azure_Functions_CONFIG. ] */
/* Tests_SRS_AZUREFUNCTIONS_HL_04_009: [ Azure_Functions_HL_Create shall call STRING_construct to create relativePath based on input host address. ] */
/* Tests_SRS_AZUREFUNCTIONS_HL_04_008: [ Azure_Functions_HL_Create shall call STRING_construct to create hostAddress based on input host address. ] */
/* Tests_SRS_AZUREFUNCTIONS_HL_04_007: [ If the array object does not contain a value named "relativePath" then Azure_Functions_HL_Create shall fail and return NULL. ] */
/* Tests_SRS_AZUREFUNCTIONS_HL_04_006: [ If the array object does not contain a value named "hostAddress" then Azure_Functions_HL_Create shall fail and return NULL. ] */
/* Tests_SRS_AZUREFUNCTIONS_HL_04_005: [ Azure_Functions_HL_Create shall parse the configuration as a JSON array of strings. ] */
TEST_FUNCTION(AZUREFUNCTIONS_HL_Create_happy_path)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

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

	STRICT_EXPECTED_CALL(MODULE_STATIC_GETAPIS(AZUREFUNCTIONS_MODULE)(IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	STRICT_EXPECTED_CALL(azure_functions_Create((BROKER_HANDLE)0x42, (const void*)0x42))
		.IgnoreArgument(2)
		.SetReturn((MODULE_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

	STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

	STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

	STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x42));


	// act
	MODULE_HANDLE result = apis.Module_Create((BROKER_HANDLE)0x42, (const void*)0x42);
	//assert
	ASSERT_IS_NOT_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_HL_04_017: [ Azure_Functions_HL_Destroy shall do nothing if moduleHandle is NULL. ] */
TEST_FUNCTION(AZUREFUNCTIONS_HL_Destroy_does_nothing_if_module_handle_null)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	apis.Module_Destroy(NULL);

	//assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

///* Tests_SRS_AZUREFUNCTIONS_HL_04_015: [ Azure_Functions_HL_Destroy shall free all used resources. ] */
TEST_FUNCTION(AZUREFUNCTIONS_HL_Destroy_happy_path)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

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
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(MODULE_STATIC_GETAPIS(AZUREFUNCTIONS_MODULE)(IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	STRICT_EXPECTED_CALL(azure_functions_Create((BROKER_HANDLE)0x42, (const void*)0x42))
		.IgnoreArgument(2)
		.SetReturn((MODULE_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_delete(NULL));

	STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

	STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

	STRICT_EXPECTED_CALL(json_value_free((JSON_Value*)0x42));

	MODULE_HANDLE result = apis.Module_Create((BROKER_HANDLE)0x42, (const void*)0x42);
	ASSERT_IS_NOT_NULL(result);

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(MODULE_STATIC_GETAPIS(AZUREFUNCTIONS_MODULE)(IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	STRICT_EXPECTED_CALL(azure_functions_Destroy((BROKER_HANDLE)0x42));

	//act
	apis.Module_Destroy(result);

	//assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_HL_04_018: [ Azure_Functions_HL_Receive shall do nothing if any parameter is NULL. ] */
TEST_FUNCTION(AZUREFUNCTIONS_HL_Receive_doesNothing_if_moduleHandleIsNull)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

  //act
	apis.Module_Receive(NULL, (MESSAGE_HANDLE)0x42);

	//assert
  ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_HL_04_018: [ Azure_Functions_HL_Receive shall do nothing if any parameter is NULL. ] */
TEST_FUNCTION(AZUREFUNCTIONS_HL_Receive_doesNothing_if_messageHandleIsNull)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	//act
	apis.Module_Receive((MODULE_HANDLE)0x42, NULL);

	//assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AZUREFUNCTIONS_HL_04_016: [ Azure_Functions_HL_Receive shall pass the received parameters to the underlying identity map module receive function. ] */
TEST_FUNCTION(AZUREFUNCTIONS_HL_Receive_happyPath)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	STRICT_EXPECTED_CALL(MODULE_STATIC_GETAPIS(AZUREFUNCTIONS_MODULE)(IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	STRICT_EXPECTED_CALL(azure_functions_Receive((MODULE_HANDLE)0x42, (MESSAGE_HANDLE)0x42));

	//act
	apis.Module_Receive((MODULE_HANDLE)0x42, (MESSAGE_HANDLE)0x42);

	//assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(azure_functions_hl_ut)
