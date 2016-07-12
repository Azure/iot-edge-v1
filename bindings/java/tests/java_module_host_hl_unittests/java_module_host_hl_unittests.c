// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umock_c_negative_tests.h"
#include "umocktypes_charptr.h"

#define ENABLE_MOCKS

#include "java_module_host_hl.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/strings.h"
#include "parson.h"

//=============================================================================
//Globals
//=============================================================================

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

static bool malloc_will_fail = false;

//=============================================================================
//MOCKS
//=============================================================================

//JavaModuleHost mocks
MOCKABLE_FUNCTION(, MODULE_HANDLE, JavaModuleHost_Create, MESSAGE_BUS_HANDLE, bus, const void*, configuration);
MODULE_HANDLE my_JavaModuleHost_Create(MESSAGE_BUS_HANDLE bus, const void* configuration)
{
	MODULE_HANDLE handle = NULL;
	if (bus != NULL && configuration != NULL)
	{
		handle = (MODULE_HANDLE)malloc(1);
	}
	return handle;
}

MOCKABLE_FUNCTION(, void, JavaModuleHost_Destroy, MODULE_HANDLE, module);
void my_JavaModuleHost_Destroy(MODULE_HANDLE module)
{
	free(module);
}

MOCKABLE_FUNCTION(, void, JavaModuleHost_Receive, MODULE_HANDLE, module, MESSAGE_HANDLE, message);

static const MODULE_APIS JavaModuleHost_APIS =
{
	JavaModuleHost_Create,
	JavaModuleHost_Destroy,
	JavaModuleHost_Receive
};

const MODULE_APIS* MODULE_STATIC_GETAPIS(JAVA_MODULE_HOST)(void)
{
	return &JavaModuleHost_APIS;
}

//parson mocks
MOCKABLE_FUNCTION(, JSON_Value*, json_parse_string, const char*, string);
JSON_Value* my_json_parse_string(const char* string)
{
	JSON_Value* value = NULL;
	if (string != NULL)
	{
		value = (JSON_Value*)malloc(1);
	}
	return value;
}

MOCK_FUNCTION_WITH_CODE(, JSON_Object*, json_value_get_object, const JSON_Value*, value)
JSON_Object* obj = NULL;
if (value != NULL)
{
	obj = (JSON_Object*)0x42;
}
MOCK_FUNCTION_END(obj)

MOCK_FUNCTION_WITH_CODE(, void, json_free_serialized_string, char*, string)
free(string);
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void, json_value_free, JSON_Value*, value)
free(value);
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, const char*, json_object_get_string, const JSON_Object*, object, const char*, name)
const char* str = NULL;
if (object != NULL && name != NULL)
{
	str = "hello_world";
}
MOCK_FUNCTION_END(str)

MOCK_FUNCTION_WITH_CODE(, char*, json_serialize_to_string, const JSON_Value*, value)
char* serialized_string = NULL;
if (value != NULL)
{
	serialized_string = (char*)malloc(strlen("hello_world") +1);
	sprintf(serialized_string, "%s", "hello_world");
}
MOCK_FUNCTION_END(serialized_string)

MOCK_FUNCTION_WITH_CODE(, JSON_Object*, json_object_get_object, const JSON_Object*, object, const char*, name)
JSON_Object* obj = NULL;
if (object != NULL && name != NULL)
{
	obj = (JSON_Object*)0x42;
}
MOCK_FUNCTION_END(obj)

MOCK_FUNCTION_WITH_CODE(, double, json_object_get_number, const JSON_Object*, object, const char*, name)
double num = 0;
if (object != NULL && name != NULL)
{
	num = 1;
}
MOCK_FUNCTION_END(num)

MOCK_FUNCTION_WITH_CODE(, int, json_object_get_boolean, const JSON_Object*, object, const char*, name)
int num = -1;
if (object != NULL && name != NULL)
{
	num = 1;
}
MOCK_FUNCTION_END(num)

MOCK_FUNCTION_WITH_CODE(, JSON_Array*, json_object_get_array, const JSON_Object*, object, const char*, name)
JSON_Array* arr = NULL;
if (object != NULL && name != NULL)
{
	arr = (JSON_Array*)0x42;
}
MOCK_FUNCTION_END(arr)

MOCK_FUNCTION_WITH_CODE(, JSON_Value*, json_object_get_value, const JSON_Object*, object, const char*, name)
JSON_Value* value = NULL;
if (object != NULL && name != NULL)
{
	value = (JSON_Value*)0x42;
}
MOCK_FUNCTION_END(value)

MOCK_FUNCTION_WITH_CODE(, size_t, json_array_get_count, const JSON_Array*, arr)
size_t num = -1;
if (arr != NULL)
{
	num = 1;
}
MOCK_FUNCTION_END(num)

MOCK_FUNCTION_WITH_CODE(, const char*, json_array_get_string, const JSON_Array*, arr, size_t, index)
const char* str = NULL;
if (arr != NULL && index >= 0)
{
	str = "hello_world";
}
MOCK_FUNCTION_END(str)

//=============================================================================
//HOOKS
//=============================================================================
#ifdef __cplusplus
extern "C"
{
#endif
	VECTOR_HANDLE real_VECTOR_create(size_t elementSize);
	void real_VECTOR_destroy(VECTOR_HANDLE handle);

	/* insertion */
	int real_VECTOR_push_back(VECTOR_HANDLE handle, const void* elements, size_t numElements);

	/* removal */
	void real_VECTOR_erase(VECTOR_HANDLE handle, void* elements, size_t numElements);
	void real_VECTOR_clear(VECTOR_HANDLE handle);

	/* access */
	void* real_VECTOR_element(const VECTOR_HANDLE handle, size_t index);
	void* real_VECTOR_front(const VECTOR_HANDLE handle);
	void* real_VECTOR_back(const VECTOR_HANDLE handle);
	void* real_VECTOR_find_if(const VECTOR_HANDLE handle, PREDICATE_FUNCTION pred, const void* value);

	/* capacity */
	size_t real_VECTOR_size(const VECTOR_HANDLE handle);

	//String mocks
	STRING_HANDLE real_STRING_construct(const char* psz);
	void real_STRING_delete(STRING_HANDLE handle);
	const char* real_STRING_c_str(STRING_HANDLE handle);
	STRING_HANDLE real_STRING_clone(STRING_HANDLE handle);
	int real_STRING_concat(STRING_HANDLE handle, const char* s2);
#ifdef __cplusplus
}
#endif
void* my_gballoc_malloc(size_t size)
{
	void* result = NULL;
	if (malloc_will_fail == false)
	{
		result = malloc(size);
	}

	return result;
}

void my_gballoc_free(void* ptr)
{
	free(ptr);
}

void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
	ASSERT_FAIL("umock_c reported error");
}

#include "azure_c_shared_utility/gballoc.h"

#undef ENABLE_MOCKS

/*these are simple cached variables*/
static pfModule_Create  JavaModuleHost_HL_Create = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Destroy JavaModuleHost_HL_Destroy = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Receive JavaModuleHost_HL_Receive = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/

BEGIN_TEST_SUITE(JavaModuleHost_HL_UnitTests)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
	TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
	g_testByTest = TEST_MUTEX_CREATE();
	ASSERT_IS_NOT_NULL(g_testByTest);

	JavaModuleHost_HL_Create = Module_GetAPIS()->Module_Create;
	JavaModuleHost_HL_Destroy = Module_GetAPIS()->Module_Destroy;
	JavaModuleHost_HL_Receive = Module_GetAPIS()->Module_Receive;

	umock_c_init(on_umock_c_error);
	umocktypes_charptr_register_types();

	REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
	REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

	REGISTER_GLOBAL_MOCK_HOOK(VECTOR_create, real_VECTOR_create);
	REGISTER_GLOBAL_MOCK_HOOK(VECTOR_push_back, real_VECTOR_push_back);
	REGISTER_GLOBAL_MOCK_HOOK(VECTOR_size, real_VECTOR_size);
	REGISTER_GLOBAL_MOCK_HOOK(VECTOR_destroy, real_VECTOR_destroy);
	REGISTER_GLOBAL_MOCK_HOOK(VECTOR_element, real_VECTOR_element);

	REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, real_STRING_construct);
	REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, real_STRING_delete);
	REGISTER_GLOBAL_MOCK_HOOK(STRING_c_str, real_STRING_c_str);
	REGISTER_GLOBAL_MOCK_HOOK(STRING_clone, real_STRING_clone);
	REGISTER_GLOBAL_MOCK_HOOK(STRING_concat, real_STRING_concat);

	REGISTER_GLOBAL_MOCK_HOOK(json_parse_string, my_json_parse_string);

	REGISTER_GLOBAL_MOCK_HOOK(JavaModuleHost_Create, my_JavaModuleHost_Create);
	REGISTER_GLOBAL_MOCK_HOOK(JavaModuleHost_Destroy, my_JavaModuleHost_Destroy);

	REGISTER_UMOCK_ALIAS_TYPE(MODULE_HANDLE, void*);

	REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_BUS_HANDLE, void*);

	REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_HANDLE, void*);

	REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);

	REGISTER_UMOCK_ALIAS_TYPE(VECTOR_HANDLE, void*);
	REGISTER_UMOCK_ALIAS_TYPE(const VECTOR_HANDLE, void*);

	REGISTER_UMOCK_ALIAS_TYPE(const char*, char*);
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
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
	TEST_MUTEX_RELEASE(g_testByTest);
}

/*Tests_SRS_JAVA_MODULE_HOST_HL_14_002: [This function shall return NULL if bus is NULL or configuration is NULL.]*/
TEST_FUNCTION(JavaModuleHost_HL_Create_return_NULL_for_NULL_bus_param)
{
	//Arrange

	//Act
	MODULE_HANDLE module = JavaModuleHost_HL_Create(NULL, (const void*)0x42);

	//Assert
	ASSERT_IS_NULL(module);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//Cleanup
}

/*Tests_SRS_JAVA_MODULE_HOST_HL_14_002: [This function shall return NULL if bus is NULL or configuration is NULL.]*/
TEST_FUNCTION(JavaModuleHost_HL_Create_return_NULL_for_NULL_configuration_param)
{
	//Arrange

	//Act
	MODULE_HANDLE module = JavaModuleHost_HL_Create((MESSAGE_BUS_HANDLE)0x42, NULL);

	//Assert
	ASSERT_IS_NULL(module);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//Cleanup
}

/*Tests_SRS_JAVA_MODULE_HOST_HL_14_003: [This function shall return NULL if configuration is not a valid JSON object.]*/
/*Tests_SRS_JAVA_MODULE_HOST_HL_14_004: [This function shall return NULL if configuration.args does not contain a field named class_name.]*/
/*Tests_SRS_JAVA_MODULE_HOST_HL_14_005: [This function shall parse the configuration.args JSON object and initialize a new JAVA_MODULE_HOST_CONFIG setting default values to all missing fields.]*/
/*Tests_SRS_JAVA_MODULE_HOST_HL_14_006: [This function shall pass bus and the newly created JAVA_MODULE_HOST_CONFIG structure to JavaModuleHost_Create.]*/
/*Tests_SRS_JAVA_MODULE_HOST_HL_14_007: [This function shall fail or succeed after this function call and return the value from this function call.]*/
/*Tests_SRS_JAVA_MODULE_HOST_HL_14_010: [This function shall return NULL if any underlying API call fails.]*/
TEST_FUNCTION(JavaModuleHost_HL_Create_API_failure_tests)
{
	//Arrange
	MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)0x42;
	const void* configuration = "{\"json\": \"string\"}";

	int result = 0;
	result = umock_c_negative_tests_init();
	ASSERT_ARE_EQUAL(int, 0, result);

	STRICT_EXPECTED_CALL(json_parse_string((const char*)configuration))
		.SetFailReturn(NULL);
	STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetFailReturn(NULL);
	STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, JAVA_MODULE_CLASS_NAME_KEY))
		.IgnoreArgument(1)
		.SetFailReturn(NULL);

	//Will not fail
	STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, JAVA_MODULE_CLASS_NAME_KEY))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, JAVA_MODULE_ARGS_KEY))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_serialize_to_string(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, JAVA_MODULE_CLASS_PATH_KEY))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, JAVA_MODULE_LIBRARY_PATH_KEY))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, JAVA_MODULE_JVM_OPTIONS_KEY))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_object_get_number(IGNORED_PTR_ARG, JAVA_MODULE_JVM_OPTIONS_VERSION_KEY))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_object_get_boolean(IGNORED_PTR_ARG, JAVA_MODULE_JVM_OPTIONS_DEBUG_KEY))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_object_get_number(IGNORED_PTR_ARG, JAVA_MODULE_JVM_OPTIONS_DEBUG_PORT_KEY))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_object_get_boolean(IGNORED_PTR_ARG, JAVA_MODULE_JVM_OPTIONS_VERBOSE_KEY))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_object_get_array(IGNORED_PTR_ARG, JAVA_MODULE_JVM_OPTIONS_ADDITIONAL_OPTIONS_KEY))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_array_get_count(IGNORED_PTR_ARG))
		.IgnoreArgument(1);


	STRICT_EXPECTED_CALL(VECTOR_create(sizeof(STRING_HANDLE)))
		.SetFailReturn(NULL);
	STRICT_EXPECTED_CALL(json_array_get_string(IGNORED_PTR_ARG, 0))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetFailReturn(NULL);
	STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1)
		.IgnoreArgument(2)
		.SetFailReturn(-1);

	STRICT_EXPECTED_CALL(JavaModuleHost_Create(bus, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.SetFailReturn(NULL);

	umock_c_negative_tests_snapshot();

	//Act
	for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
	{
		if (i <= 2 || i >= 15)
		{
			// arrange
			umock_c_negative_tests_reset();
			umock_c_negative_tests_fail_call(i);

			// act
			MODULE_HANDLE module = JavaModuleHost_HL_Create(bus, configuration);

			// assert
			ASSERT_ARE_EQUAL(void_ptr, NULL, module);
		}
	}
	umock_c_negative_tests_deinit();

	//Assert

	//Cleanup
}

/*Tests_SRS_JAVA_MODULE_HOST_HL_14_005: [This function shall parse the configuration.args JSON object and initialize a new JAVA_MODULE_HOST_CONFIG setting default values to all missing fields.]*/
/*Tests_SRS_JAVA_MODULE_HOST_HL_14_006: [This function shall pass bus and the newly created JAVA_MODULE_HOST_CONFIG structure to JavaModuleHost_Create.]*/
/*Tests_SRS_JAVA_MODULE_HOST_HL_14_007: [This function shall fail or succeed after this function call and return the value from this function call.]*/
TEST_FUNCTION(JavaModuleHost_HL_Create_success)
{
	//Arrange
	MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)0x42;
	const void* configuration = "{\"json\": \"string\"}";

	STRICT_EXPECTED_CALL(json_parse_string((const char*)configuration));
	STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, JAVA_MODULE_CLASS_NAME_KEY))
		.IgnoreArgument(1);

	//Will not fail
	STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, JAVA_MODULE_CLASS_NAME_KEY))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, JAVA_MODULE_ARGS_KEY))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_serialize_to_string(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, JAVA_MODULE_CLASS_PATH_KEY))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, JAVA_MODULE_LIBRARY_PATH_KEY))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, JAVA_MODULE_JVM_OPTIONS_KEY))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_object_get_number(IGNORED_PTR_ARG, JAVA_MODULE_JVM_OPTIONS_VERSION_KEY))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_object_get_boolean(IGNORED_PTR_ARG, JAVA_MODULE_JVM_OPTIONS_DEBUG_KEY))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_object_get_number(IGNORED_PTR_ARG, JAVA_MODULE_JVM_OPTIONS_DEBUG_PORT_KEY))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_object_get_boolean(IGNORED_PTR_ARG, JAVA_MODULE_JVM_OPTIONS_VERBOSE_KEY))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_object_get_array(IGNORED_PTR_ARG, JAVA_MODULE_JVM_OPTIONS_ADDITIONAL_OPTIONS_KEY))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(json_array_get_count(IGNORED_PTR_ARG))
		.IgnoreArgument(1);


	STRICT_EXPECTED_CALL(VECTOR_create(sizeof(STRING_HANDLE)));
	STRICT_EXPECTED_CALL(json_array_get_string(IGNORED_PTR_ARG, 0))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1)
		.IgnoreArgument(2);

	STRICT_EXPECTED_CALL(JavaModuleHost_Create(bus, IGNORED_PTR_ARG))
		.IgnoreArgument(2);

	STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 0))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	// act
	MODULE_HANDLE module = JavaModuleHost_HL_Create(bus, configuration);

	// Assert
	ASSERT_IS_NOT_NULL(module);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//Cleanup
	JavaModuleHost_HL_Destroy(module);
}

/*Tests_SRS_JAVA_MODULE_HOST_HL_14_009: [ This function shall call the underlying Java Module Host's _Receive function using this module MODULE_HANDLE and message MESSAGE_HANDLE. ]*/
TEST_FUNCTION(JavaModuleHost_HL_Receive_LL_function_call_test)
{
	//Arrange
	MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)0x42;
	const void* configuration = "{\"json\": \"string\"}";
	MODULE_HANDLE module = JavaModuleHost_HL_Create(bus, configuration);
	MESSAGE_HANDLE message = (MESSAGE_HANDLE)0x43;
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(JavaModuleHost_Receive(module, message));

	//Act

	JavaModuleHost_HL_Receive(module, message);

	//Assert

	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//Cleanup
	JavaModuleHost_HL_Destroy(module);
}

/*Tests_SRS_JAVA_MODULE_HOST_HL_14_008: [ This function shall call the underlying Java Module Host's _Destroy function using this module MODULE_HANDLE. ]*/
TEST_FUNCTION(JavaModuleHostHL_Destroy_LL_function_call_test)
{
	//Arrange
	MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)0x42;
	const void* configuration = "{\"json\": \"string\"}";
	MODULE_HANDLE module = JavaModuleHost_HL_Create(bus, configuration);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(JavaModuleHost_Destroy(module));

	//Act

	JavaModuleHost_HL_Destroy(module);

	//Assert

	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//Cleanup
}

/*Tests_SRS_JAVA_MODULE_HOST_HL_14_001: [ This function shall return a non-NULL pointer to a structure of type MODULE_APIS that has all fields non-NULL. ]*/
TEST_FUNCTION(Module_GetAPIS_returns_non_NULL)
{
	//Arrange

	//Act

	const MODULE_APIS* apis = Module_GetAPIS();

	//Assert
	ASSERT_IS_NOT_NULL(apis);
	ASSERT_IS_NOT_NULL(apis->Module_Create);
	ASSERT_IS_NOT_NULL(apis->Module_Destroy);
	ASSERT_IS_NOT_NULL(apis->Module_Receive);
}

END_TEST_SUITE(JavaModuleHost_HL_UnitTests)
