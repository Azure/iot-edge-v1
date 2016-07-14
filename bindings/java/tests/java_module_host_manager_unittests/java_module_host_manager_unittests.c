// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umock_c_negative_tests.h"
#include "umock_c_prod.h"

#define ENABLE_MOCKS

#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/lock.h"

//=============================================================================
//Globals
//=============================================================================

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

static bool malloc_will_fail = false;

//=============================================================================
//MOCKS
//=============================================================================

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

	//mallocAndStrcpy_s
	int real_mallocAndStrcpy_s(char** destination, const char* source);
#ifdef __cplusplus
}
#endif

LOCK_HANDLE my_Lock_Init(void)
{
	return (LOCK_HANDLE)malloc(1);
}

LOCK_RESULT my_Lock(LOCK_HANDLE handle)
{
	LOCK_RESULT result = LOCK_ERROR;
	if (handle != NULL)
	{
		result = LOCK_OK;
	}
	return result;
}

LOCK_RESULT my_Unlock(LOCK_HANDLE handle)
{
	LOCK_RESULT result = LOCK_ERROR;
	if (handle != NULL)
	{
		result = LOCK_OK;
	}
	return result;
}

LOCK_RESULT my_Lock_Deinit(LOCK_HANDLE handle)
{
	LOCK_RESULT result = LOCK_ERROR;
	if (handle != NULL)
	{
		free(handle);
		result = LOCK_OK;
	}
	return result;
}

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

#include "java_module_host_manager.h"

static JAVA_MODULE_HOST_CONFIG* global_config;

TEST_DEFINE_ENUM_TYPE(JAVA_MODULE_HOST_MANAGER_RESULT, JAVA_MODULE_HOST_MANAGER_RESULT_VALUES);

IMPLEMENT_UMOCK_C_ENUM_TYPE(LOCK_RESULT, LOCK_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(JAVA_MODULE_HOST_MANAGER_RESULT, JAVA_MODULE_HOST_MANAGER_RESULT_VALUES);

BEGIN_TEST_SUITE(JavaModuleHost_Manager_UnitTests)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
	TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
	g_testByTest = TEST_MUTEX_CREATE();
	ASSERT_IS_NOT_NULL(g_testByTest);

	umock_c_init(on_umock_c_error);
	
	REGISTER_GLOBAL_MOCK_FAIL_RETURN(Lock, LOCK_ERROR);
	REGISTER_GLOBAL_MOCK_FAIL_RETURN(Unlock, LOCK_ERROR);
	REGISTER_GLOBAL_MOCK_FAIL_RETURN(Lock_Deinit, LOCK_ERROR);

	REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
	REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

	//Lock Hooks
	REGISTER_GLOBAL_MOCK_HOOK(Lock_Init, my_Lock_Init);
	REGISTER_GLOBAL_MOCK_HOOK(Lock, my_Lock);
	REGISTER_GLOBAL_MOCK_HOOK(Unlock, my_Unlock);
	REGISTER_GLOBAL_MOCK_HOOK(Lock_Deinit, my_Lock_Deinit);

	//Vector Hooks
	REGISTER_GLOBAL_MOCK_HOOK(VECTOR_create, real_VECTOR_create);
	REGISTER_GLOBAL_MOCK_HOOK(VECTOR_push_back, real_VECTOR_push_back);
	REGISTER_GLOBAL_MOCK_HOOK(VECTOR_size, real_VECTOR_size);
	REGISTER_GLOBAL_MOCK_HOOK(VECTOR_destroy, real_VECTOR_destroy);
	REGISTER_GLOBAL_MOCK_HOOK(VECTOR_element, real_VECTOR_element);

	//String Hooks
	REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, real_STRING_construct);
	REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, real_STRING_delete);
	REGISTER_GLOBAL_MOCK_HOOK(STRING_c_str, real_STRING_c_str);
	REGISTER_GLOBAL_MOCK_HOOK(STRING_clone, real_STRING_clone);
	REGISTER_GLOBAL_MOCK_HOOK(STRING_concat, real_STRING_concat);

	//mallocAndStrcpy_s Hook
	REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, real_mallocAndStrcpy_s);

	REGISTER_UMOCK_ALIAS_TYPE(LOCK_HANDLE, void*);

	REGISTER_UMOCK_ALIAS_TYPE(VECTOR_HANDLE, void*);
	REGISTER_UMOCK_ALIAS_TYPE(const VECTOR_HANDLE, void*);
	
	REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);

	REGISTER_TYPE(LOCK_RESULT, LOCK_RESULT);
	REGISTER_TYPE(JAVA_MODULE_HOST_MANAGER_RESULT, JAVA_MODULE_HOST_MANAGER_RESULT);

	global_config = (JAVA_MODULE_HOST_CONFIG*)malloc(sizeof(JAVA_MODULE_HOST_CONFIG));;
	global_config->class_name = "foo";
	global_config->configuration_json = "{\"hello\":\"world\"}";
	global_config->options = (JVM_OPTIONS*)malloc(sizeof(JVM_OPTIONS));
	global_config->options->class_path = "cp";
	global_config->options->library_path = "lp";
	global_config->options->debug = true;
	global_config->options->debug_port = 1234;
	global_config->options->verbose = false;
	global_config->options->version = 1;
	global_config->options->additional_options = VECTOR_create(1);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
	VECTOR_destroy(global_config->options->additional_options);
	free(global_config->options);
	free(global_config);

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

/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_002: [ The function shall allocate memory for the JAVA_MODULE_HOST_MANAGER_HANDLE if the global instance is NULL. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_004: [ The function shall initialize a new LOCK_HANDLE and set the JAVA_MODULE_HOST_MANAGER_HANDLE structures LOCK_HANDLE member. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_006: [ The function shall set the module_count member variable to 0. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_007: [ The function shall shall set the global JAVA_MODULE_HOST_MANAGER_HANDLE instance. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_008: [ The function shall return the global JAVA_MODULE_HOST_MANAGER_HANDLE instance. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_028: [The function shall do a deep copy of the config parameter and keep a pointer to the copied JAVA_MODULE_HOST_CONFIG.]*/
TEST_FUNCTION(JavaModuleHostManager_Create_success)
{
	//Arrange

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());

	//Expected calls for config copy
	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JAVA_MODULE_HOST_CONFIG)));

	STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, global_config->class_name))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, global_config->configuration_json))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JVM_OPTIONS)));

	STRICT_EXPECTED_CALL(VECTOR_create(sizeof(STRING_HANDLE)));
	STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, global_config->options->class_path))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, global_config->options->library_path))
		.IgnoreArgument(1);

	//Act
	JAVA_MODULE_HOST_MANAGER_HANDLE manager = JavaModuleHostManager_Create(global_config);

	//Assert
	ASSERT_IS_NOT_NULL(manager);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//Cleanup
	JavaModuleHostManager_Destroy(manager);
}

/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_001: [ The function shall return the global JAVA_MODULE_HOST_MANAGER_HANDLE instance if it is not NULL. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_008: [ The function shall return the global JAVA_MODULE_HOST_MANAGER_HANDLE instance. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_028: [The function shall do a deep copy of the config parameter and keep a pointer to the copied JAVA_MODULE_HOST_CONFIG.]*/
/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_030: [The function shall check against the singleton JAVA_MODULE_HOST_MANAGER_HANDLE instances JAVA_MODULE_HOST_CONFIG structure for equality in each field.]*/
TEST_FUNCTION(JavaModuleHostManager_Create_multiple_success)
{
	//Arrange

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());

	//Expected calls for config copy
	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JAVA_MODULE_HOST_CONFIG)));

	STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, global_config->class_name))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, global_config->configuration_json))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JVM_OPTIONS)));

	STRICT_EXPECTED_CALL(VECTOR_create(sizeof(STRING_HANDLE)));
	STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, global_config->options->class_path))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, global_config->options->library_path))
		.IgnoreArgument(1);

	//For comparison
	STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
		.IgnoreArgument(1);


	//Act
	JAVA_MODULE_HOST_MANAGER_HANDLE manager = JavaModuleHostManager_Create(global_config);
	JAVA_MODULE_HOST_MANAGER_HANDLE manager2 = JavaModuleHostManager_Create(global_config);

	//Assert
	ASSERT_IS_NOT_NULL(manager);
	ASSERT_ARE_EQUAL(void_ptr, manager, manager2);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//Cleanup
	JavaModuleHostManager_Destroy(manager);
}

/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_003: [ The function shall return NULL if memory allocation fails. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_005: [ The function shall return NULL if lock initialization fails. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_029: [If the deep copy fails, the function shall return NULL.]*/
TEST_FUNCTION(JavaModuleHostManager_Create_failure)
{
	//Arrange

	int result = 0;
	result = umock_c_negative_tests_init();
	ASSERT_ARE_EQUAL(int, 0, result);

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());

	//Expected calls for config copy
	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JAVA_MODULE_HOST_CONFIG)));

	STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, global_config->class_name))
		.IgnoreArgument(1)
		.SetFailReturn(-1);
	STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, global_config->configuration_json))
		.IgnoreArgument(1)
		.SetFailReturn(-1);

	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JVM_OPTIONS)));


	STRICT_EXPECTED_CALL(VECTOR_create(sizeof(STRING_HANDLE)));
	STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetFailReturn(-1);

	STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, global_config->options->class_path))
		.IgnoreArgument(1)
		.SetFailReturn(-1);
	STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, global_config->options->library_path))
		.IgnoreArgument(1)
		.SetFailReturn(-1);

	umock_c_negative_tests_snapshot();

	//Act
	for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
	{
		// arrange
		umock_c_negative_tests_reset();
		umock_c_negative_tests_fail_call(i);

		// act
		JAVA_MODULE_HOST_MANAGER_HANDLE manager = JavaModuleHostManager_Create(global_config);

		// assert
		ASSERT_ARE_EQUAL(void_ptr, NULL, manager);
	}
	umock_c_negative_tests_deinit();
}

/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_031: [The function shall return NULL if the JAVA_MODULE_HOST_CONFIG structures do not match.]*/
TEST_FUNCTION(JavaModuleHostManager_Create_multiple_config_not_match_failure_classpath)
{
	//Arrange

	VECTOR_HANDLE additional_options = VECTOR_create(1);

	JVM_OPTIONS options =
	{
		"cp1",
		"lp",
		1,
		true,
		1234,
		false,
		additional_options
	};

	JAVA_MODULE_HOST_CONFIG config2 =
	{
		"foo",
		"{\"hello\": \"world\"}",
		&options
	};

	//Act
	JAVA_MODULE_HOST_MANAGER_HANDLE manager = JavaModuleHostManager_Create(global_config);
	JAVA_MODULE_HOST_MANAGER_HANDLE manager2 = JavaModuleHostManager_Create(&config2);

	//Assert
	ASSERT_IS_NULL(manager2);

	//Cleanup
	JavaModuleHostManager_Destroy(manager);

	VECTOR_destroy((&config2)->options->additional_options);
}

/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_031: [The function shall return NULL if the JAVA_MODULE_HOST_CONFIG structures do not match.]*/
TEST_FUNCTION(JavaModuleHostManager_Create_multiple_config_not_match_failure_librarypath)
{
	//Arrange

	VECTOR_HANDLE additional_options = VECTOR_create(1);

	JVM_OPTIONS options =
	{
		"cp",
		"lp1",
		1,
		true,
		1234,
		false,
		additional_options
	};

	JAVA_MODULE_HOST_CONFIG config2 =
	{
		"foo",
		"{\"hello\": \"world\"}",
		&options
	};

	//Act
	JAVA_MODULE_HOST_MANAGER_HANDLE manager = JavaModuleHostManager_Create(global_config);
	JAVA_MODULE_HOST_MANAGER_HANDLE manager2 = JavaModuleHostManager_Create(&config2);

	//Assert
	ASSERT_IS_NULL(manager2);

	//Cleanup
	JavaModuleHostManager_Destroy(manager);

	VECTOR_destroy((&config2)->options->additional_options);
}

/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_031: [The function shall return NULL if the JAVA_MODULE_HOST_CONFIG structures do not match.]*/
TEST_FUNCTION(JavaModuleHostManager_Create_multiple_config_not_match_failure_version)
{
	//Arrange

	VECTOR_HANDLE additional_options = VECTOR_create(1);

	JVM_OPTIONS options =
	{
		"cp",
		"lp",
		2,
		true,
		1234,
		false,
		additional_options
	};

	JAVA_MODULE_HOST_CONFIG config2 =
	{
		"foo",
		"{\"hello\": \"world\"}",
		&options
	};

	//Act
	JAVA_MODULE_HOST_MANAGER_HANDLE manager = JavaModuleHostManager_Create(global_config);
	JAVA_MODULE_HOST_MANAGER_HANDLE manager2 = JavaModuleHostManager_Create(&config2);

	//Assert
	ASSERT_IS_NULL(manager2);

	//Cleanup
	JavaModuleHostManager_Destroy(manager);

	VECTOR_destroy((&config2)->options->additional_options);
}

/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_031: [The function shall return NULL if the JAVA_MODULE_HOST_CONFIG structures do not match.]*/
TEST_FUNCTION(JavaModuleHostManager_Create_multiple_config_not_match_failure_debug)
{
	//Arrange

	VECTOR_HANDLE additional_options = VECTOR_create(1);

	JVM_OPTIONS options =
	{
		"cp",
		"lp",
		1,
		false,
		1234,
		false,
		additional_options
	};

	JAVA_MODULE_HOST_CONFIG config2 =
	{
		"foo",
		"{\"hello\": \"world\"}",
		&options
	};

	//Act
	JAVA_MODULE_HOST_MANAGER_HANDLE manager = JavaModuleHostManager_Create(global_config);
	JAVA_MODULE_HOST_MANAGER_HANDLE manager2 = JavaModuleHostManager_Create(&config2);

	//Assert
	ASSERT_IS_NULL(manager2);

	//Cleanup
	JavaModuleHostManager_Destroy(manager);

	VECTOR_destroy((&config2)->options->additional_options);
}

/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_031: [The function shall return NULL if the JAVA_MODULE_HOST_CONFIG structures do not match.]*/
TEST_FUNCTION(JavaModuleHostManager_Create_multiple_config_not_match_failure_debug_port)
{
	//Arrange

	VECTOR_HANDLE additional_options = VECTOR_create(1);

	JVM_OPTIONS options =
	{
		"cp",
		"lp",
		1,
		true,
		9876,
		false,
		additional_options
	};

	JAVA_MODULE_HOST_CONFIG config2 =
	{
		"foo",
		"{\"hello\": \"world\"}",
		&options
	};

	//Act
	JAVA_MODULE_HOST_MANAGER_HANDLE manager = JavaModuleHostManager_Create(global_config);
	JAVA_MODULE_HOST_MANAGER_HANDLE manager2 = JavaModuleHostManager_Create(&config2);

	//Assert
	ASSERT_IS_NULL(manager2);

	//Cleanup
	JavaModuleHostManager_Destroy(manager);

	VECTOR_destroy((&config2)->options->additional_options);
}

/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_031: [The function shall return NULL if the JAVA_MODULE_HOST_CONFIG structures do not match.]*/
TEST_FUNCTION(JavaModuleHostManager_Create_multiple_config_not_match_failure_verbose)
{
	//Arrange

	VECTOR_HANDLE additional_options = VECTOR_create(1);

	JVM_OPTIONS options =
	{
		"cp",
		"lp",
		1,
		true,
		1234,
		true,
		additional_options
	};

	JAVA_MODULE_HOST_CONFIG config2 =
	{
		"foo",
		"{\"hello\": \"world\"}",
		&options
	};

	//Act
	JAVA_MODULE_HOST_MANAGER_HANDLE manager = JavaModuleHostManager_Create(global_config);
	JAVA_MODULE_HOST_MANAGER_HANDLE manager2 = JavaModuleHostManager_Create(&config2);

	//Assert
	ASSERT_IS_NULL(manager2);

	//Cleanup
	JavaModuleHostManager_Destroy(manager);

	VECTOR_destroy((&config2)->options->additional_options);
}

/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_031: [The function shall return NULL if the JAVA_MODULE_HOST_CONFIG structures do not match.]*/
TEST_FUNCTION(JavaModuleHostManager_Create_multiple_config_not_match_failure_additional_options)
{
	//Arrange

	VECTOR_HANDLE additional_options = VECTOR_create(sizeof(STRING_HANDLE));
	STRING_HANDLE str = STRING_construct("hello");
	VECTOR_push_back(additional_options, &str, 1);

	JVM_OPTIONS options =
	{
		"cp",
		"lp",
		1,
		true,
		1234,
		false,
		additional_options
	};

	JAVA_MODULE_HOST_CONFIG config2 =
	{
		"foo",
		"{\"hello\": \"world\"}",
		&options
	};

	//Act
	JAVA_MODULE_HOST_MANAGER_HANDLE manager = JavaModuleHostManager_Create(global_config);
	JAVA_MODULE_HOST_MANAGER_HANDLE manager2 = JavaModuleHostManager_Create(&config2);

	//Assert
	ASSERT_IS_NULL(manager2);

	//Cleanup
	JavaModuleHostManager_Destroy(manager);

	STRING_delete(str);
	VECTOR_destroy((&config2)->options->additional_options);
}

/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_014: [ The function shall return MANAGER_ERROR if handle is NULL. ]*/
TEST_FUNCTION(JavaModuleHostManager_Add_NULL)
{
	//Arrange

	//Act
	JAVA_MODULE_HOST_MANAGER_RESULT result = JavaModuleHostManager_Add(NULL);

	//Assert
	ASSERT_ARE_EQUAL(JAVA_MODULE_HOST_MANAGER_RESULT, MANAGER_ERROR, result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_014: [ The function shall return MANAGER_ERROR if handle is NULL. ]*/
TEST_FUNCTION(JavaModuleHostManager_Remove_NULL)
{
	//Arrange

	//Act
	JAVA_MODULE_HOST_MANAGER_RESULT result = JavaModuleHostManager_Remove(NULL);

	//Assert
	ASSERT_ARE_EQUAL(JAVA_MODULE_HOST_MANAGER_RESULT, MANAGER_ERROR, result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_015: [ The function shall acquire the handle parameters lock. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_017: [ The function shall increment or decrement handle->module_count. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_018: [ The function shall release the handle parameters lock. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_020: [ The function shall return MANAGER_OK if all succeeds. ]*/
TEST_FUNCTION(JavaModuleHostManager_Add_success)
{
	//Arrange
	JAVA_MODULE_HOST_MANAGER_HANDLE manager = JavaModuleHostManager_Create(global_config);
	size_t initial_count = JavaModuleHostManager_Size(manager);

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	//Act
	JAVA_MODULE_HOST_MANAGER_RESULT result = JavaModuleHostManager_Add(manager);

	//Assert
	ASSERT_ARE_EQUAL(JAVA_MODULE_HOST_MANAGER_RESULT, MANAGER_OK, result);
	ASSERT_ARE_EQUAL(size_t, initial_count + 1, 1);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//Cleanup
	JavaModuleHostManager_Remove(manager);
	JavaModuleHostManager_Destroy(manager);
}

/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_016: [ If lock acquisition fails, this function shall return MANAGER_ERROR. ]*/
TEST_FUNCTION(JavaModuleHostManager_Add_failure)
{
	//Arrange

	JAVA_MODULE_HOST_MANAGER_HANDLE manager = JavaModuleHostManager_Create(global_config);

	umock_c_reset_all_calls();

	int result = 0;
	result = umock_c_negative_tests_init();
	ASSERT_ARE_EQUAL(int, 0, result);

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	umock_c_negative_tests_snapshot();

	//Act
	for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
	{
		// arrange
		umock_c_negative_tests_reset();
		umock_c_negative_tests_fail_call(i);

		// act
		JAVA_MODULE_HOST_MANAGER_RESULT res = JavaModuleHostManager_Add(manager);

		// assert
		if (i == 0)
		{
			ASSERT_ARE_EQUAL(JAVA_MODULE_HOST_MANAGER_RESULT, MANAGER_ERROR, res);
		}
		else
		{
			ASSERT_ARE_EQUAL(JAVA_MODULE_HOST_MANAGER_RESULT, MANAGER_OK, res);
		}
	}
	umock_c_negative_tests_deinit();

	//Cleanup
	JavaModuleHostManager_Remove(manager);
	JavaModuleHostManager_Destroy(manager);
}

/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_015: [ The function shall acquire the handle parameters lock. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_017: [ The function shall increment or decrement handle->module_count. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_018: [ The function shall release the handle parameters lock. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_020: [ The function shall return MANAGER_OK if all succeeds. ]*/
TEST_FUNCTION(JavaModuleHostManager_Remove_success)
{
	//Arrange
	JAVA_MODULE_HOST_MANAGER_HANDLE manager = JavaModuleHostManager_Create(global_config);
	JAVA_MODULE_HOST_MANAGER_RESULT res = JavaModuleHostManager_Add(manager);
	ASSERT_ARE_EQUAL(JAVA_MODULE_HOST_MANAGER_RESULT, MANAGER_OK, res);
	size_t initial_count = JavaModuleHostManager_Size(manager);

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	//Act
	JAVA_MODULE_HOST_MANAGER_RESULT result = JavaModuleHostManager_Remove(manager);

	//Assert
	ASSERT_ARE_EQUAL(JAVA_MODULE_HOST_MANAGER_RESULT, MANAGER_OK, result);
	ASSERT_ARE_EQUAL(size_t, 0, initial_count - 1);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//Cleanup
	JavaModuleHostManager_Destroy(manager);
}

/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_016: [ If lock acquisition fails, this function shall return MANAGER_ERROR. ]*/
TEST_FUNCTION(JavaModuleHostManager_Remove_failure)
{
	//Arrange

	JAVA_MODULE_HOST_MANAGER_HANDLE manager = JavaModuleHostManager_Create(global_config);
	JAVA_MODULE_HOST_MANAGER_RESULT res = JavaModuleHostManager_Add(manager);
	ASSERT_ARE_EQUAL(JAVA_MODULE_HOST_MANAGER_RESULT, MANAGER_OK, res);

	umock_c_reset_all_calls();

	int result = 0;
	result = umock_c_negative_tests_init();
	ASSERT_ARE_EQUAL(int, 0, result);

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	umock_c_negative_tests_snapshot();

	//Act
	for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
	{
		// arrange
		umock_c_negative_tests_reset();
		umock_c_negative_tests_fail_call(i);

		// act
		JAVA_MODULE_HOST_MANAGER_RESULT res = JavaModuleHostManager_Remove(manager);

		// assert
		if (i == 0)
		{
			ASSERT_ARE_EQUAL(JAVA_MODULE_HOST_MANAGER_RESULT, MANAGER_ERROR, res);
		}
		else
		{
			ASSERT_ARE_EQUAL(JAVA_MODULE_HOST_MANAGER_RESULT, MANAGER_OK, res);
		}
	}
	umock_c_negative_tests_deinit();

	//Cleanup
	JavaModuleHostManager_Destroy(manager);
}

/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_027: [ The function shall return MANAGER_ERROR if an attempt is made to decrement past 0. ]*/
TEST_FUNCTION(JavaModuleHostManager_Remove_below_zero_failure)
{
	//Arrange
	JAVA_MODULE_HOST_MANAGER_HANDLE manager = JavaModuleHostManager_Create(global_config);
	size_t initial_count = JavaModuleHostManager_Size(manager);
	ASSERT_ARE_EQUAL(size_t, 0, initial_count);

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	//Act
	JAVA_MODULE_HOST_MANAGER_RESULT result = JavaModuleHostManager_Remove(manager);

	//Assert
	ASSERT_ARE_EQUAL(JAVA_MODULE_HOST_MANAGER_RESULT, MANAGER_ERROR, result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	size_t count = JavaModuleHostManager_Size(manager);
	ASSERT_ARE_EQUAL(size_t, 0, count);

	//Cleanup
	JavaModuleHostManager_Destroy(manager);
}

/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_011: [ The function shall deinitialize handle->lock. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_013: [ The function shall free the handle parameter. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_032: [ The function shall destroy the JAVA_MODULE_HOST_CONFIG structure. ]*/
TEST_FUNCTION(JavaModuleHostManager_Destroy_test_success)
{
	//Arrange
	JAVA_MODULE_HOST_MANAGER_HANDLE manager = JavaModuleHostManager_Create(global_config);

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	//Config Free
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	
	//Act
	JavaModuleHostManager_Destroy(manager);
	
	//Assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_012: [ The function shall do nothing if lock deinitialization fails. ]*/
TEST_FUNCTION(JavaModuleHostManager_Destroy_deinit_failure)
{
	//Arrange
	JAVA_MODULE_HOST_MANAGER_HANDLE manager = JavaModuleHostManager_Create(global_config);

	umock_c_reset_all_calls();

	int result = 0;
	result = umock_c_negative_tests_init();
	ASSERT_ARE_EQUAL(int, 0, result);

	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	
	umock_c_negative_tests_snapshot();

	umock_c_negative_tests_reset();
	umock_c_negative_tests_fail_call(0);

	//Act
	JavaModuleHostManager_Destroy(manager);
	
	//Assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	umock_c_negative_tests_deinit();
}

/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_010: [ The function shall do nothing if handle->module_count is not 0. ]*/
TEST_FUNCTION(JavaModuleHostManager_Destroy_non_zero_does_nothing)
{
	//Arrange
	JAVA_MODULE_HOST_MANAGER_HANDLE manager = JavaModuleHostManager_Create(global_config);
	JAVA_MODULE_HOST_MANAGER_RESULT result = JavaModuleHostManager_Add(manager);
	ASSERT_ARE_EQUAL(JAVA_MODULE_HOST_MANAGER_RESULT, MANAGER_OK, result);

	umock_c_reset_all_calls();

	//Act
	JavaModuleHostManager_Destroy(manager);

	//Assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//Cleanup
	result = JavaModuleHostManager_Remove(manager);
	ASSERT_ARE_EQUAL(JAVA_MODULE_HOST_MANAGER_RESULT, MANAGER_OK, result);
	JavaModuleHostManager_Destroy(manager);
}

/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_009: [ The function shall do nothing if handle is NULL. ]*/
TEST_FUNCTION(JavaModuleHostManager_Destroy_test_NULL_param)
{
	//Arrange
	JAVA_MODULE_HOST_MANAGER_HANDLE manager = JavaModuleHostManager_Create(global_config);

	umock_c_reset_all_calls();

	//Act
	JavaModuleHostManager_Destroy(NULL);

	//Assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//Cleanup
	JavaModuleHostManager_Destroy(manager);
}

/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_021: [ If handle is NULL the function shall return -1. ]*/
TEST_FUNCTION(JavaModuleHostManager_Size_NULL_handle)
{
	//Arrange
	
	//Act
	size_t count = JavaModuleHostManager_Size(NULL);

	//Assert
	ASSERT_ARE_EQUAL(size_t, -1, count);
}

/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_022: [ The function shall return handle->module_count. ]*/
TEST_FUNCTION(JavaModuleHostManager_Size_returns_count)
{
	//Arrange
	JAVA_MODULE_HOST_MANAGER_HANDLE manager = JavaModuleHostManager_Create(global_config);
	for (size_t i = 0; i < 10; ++i) {
		JAVA_MODULE_HOST_MANAGER_RESULT result = JavaModuleHostManager_Add(manager);
		ASSERT_ARE_EQUAL(JAVA_MODULE_HOST_MANAGER_RESULT, MANAGER_OK, result);
	}

	umock_c_reset_all_calls();

	//Act
	size_t count = JavaModuleHostManager_Size(manager);

	//Assert
	ASSERT_ARE_EQUAL(size_t, 10, count);

	for (size_t i = 0; i < 10; ++i) {
		JAVA_MODULE_HOST_MANAGER_RESULT result = JavaModuleHostManager_Remove(manager);
		ASSERT_ARE_EQUAL(JAVA_MODULE_HOST_MANAGER_RESULT, MANAGER_OK, result);
	}

	JavaModuleHostManager_Destroy(manager);
}

/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_023: [The function shall acquire the handle parameters lock.]*/
/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_025: [The function shall release the handle parameters lock.]*/
TEST_FUNCTION(JavaModuleHostManager_Size_success)
{
	//Arrange
	JAVA_MODULE_HOST_MANAGER_HANDLE manager = JavaModuleHostManager_Create(global_config);

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	//Act
	size_t count = JavaModuleHostManager_Size(manager);

	//Assert
	ASSERT_ARE_EQUAL(size_t, 0, count);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//Cleanup
	JavaModuleHostManager_Destroy(manager);
}

/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_024: [If lock acquisition fails, this function shall return -1.]*/
/*Tests_SRS_JAVA_MODULE_HOST_MANAGER_14_026: [If the function fails to release the lock, this function shall report the error.]*/
TEST_FUNCTION(JavaModuleHostManager_Size_failure)
{
	//Arrange
	JAVA_MODULE_HOST_MANAGER_HANDLE manager = JavaModuleHostManager_Create(global_config);

	umock_c_reset_all_calls();

	int result = 0;
	result = umock_c_negative_tests_init();
	ASSERT_ARE_EQUAL(int, 0, result);

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	umock_c_negative_tests_snapshot();

	//Act
	//Assert
	umock_c_negative_tests_reset();
	umock_c_negative_tests_fail_call(0);
	size_t count = JavaModuleHostManager_Size(manager);
	ASSERT_ARE_EQUAL(size_t, -1, count);

	umock_c_negative_tests_reset();
	umock_c_negative_tests_fail_call(1);
	count = JavaModuleHostManager_Size(manager);
	ASSERT_ARE_EQUAL(size_t, 0, count);


	//Cleanup
	JavaModuleHostManager_Destroy(manager);
	umock_c_negative_tests_deinit();
}

END_TEST_SUITE(JavaModuleHost_Manager_UnitTests)
