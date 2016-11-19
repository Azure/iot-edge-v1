// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <stddef.h>
#include <stdbool.h>

static bool malloc_will_fail = false;

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

#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umock_c_negative_tests.h"
#include "umock_c_prod.h"
#include "umock_c_negative_tests.h"
#include "umocktypes_charptr.h"
#include "umocktypes_bool.h"
#include "umocktypes_stdint.h"

#include "real_strings.h"

#define ENABLE_MOCKS

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/strings.h"

#include "parson.h"
#include "dynamic_library.h"
#include "module_loader.h"

#undef ENABLE_MOCKS

#include "module_loaders/java_loader.h"

static pfModuleLoader_Load JavaModuleLoader_Load = NULL;
static pfModuleLoader_Unload JavaModuleLoader_Unload = NULL;
static pfModuleLoader_GetApi JavaModuleLoader_GetModuleApi = NULL;
static pfModuleLoader_ParseEntrypointFromJson JavaModuleLoader_ParseEntrypointFromJson = NULL;
static pfModuleLoader_FreeEntrypoint JavaModuleLoader_FreeEntrypoint = NULL;
static pfModuleLoader_ParseConfigurationFromJson JavaModuleLoader_ParseConfigurationFromJson = NULL;
static pfModuleLoader_FreeConfiguration JavaModuleLoader_FreeConfiguration = NULL;
static pfModuleLoader_BuildModuleConfiguration JavaModuleLoader_BuildModuleConfiguration = NULL;
static pfModuleLoader_FreeModuleConfiguration JavaModuleLoader_FreeModuleConfiguration = NULL;

//=============================================================================
//Globals
//=============================================================================

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;
static const MODULE_LOADER* g_loader;
static MODULE_LOADER_BASE_CONFIGURATION* g_config;

void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error");
}

//=============================================================================
//MOCKS
//=============================================================================

MOCKABLE_FUNCTION(, JSON_Value*, json_parse_string, const char*, string);
MOCKABLE_FUNCTION(, JSON_Object*, json_value_get_object, const JSON_Value*, value);
MOCKABLE_FUNCTION(, void, json_free_serialized_string, char*, string);
MOCKABLE_FUNCTION(, void, json_value_free, JSON_Value*, value);
MOCKABLE_FUNCTION(, const char*, json_object_get_string, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, char*, json_serialize_to_string, const JSON_Value*, value);
MOCKABLE_FUNCTION(, JSON_Object*, json_object_get_object, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, double, json_object_get_number, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, int, json_object_get_boolean, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, JSON_Array*, json_object_get_array, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, JSON_Value*, json_object_get_value, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, size_t, json_array_get_count, const JSON_Array*, arr);
MOCKABLE_FUNCTION(, const char*, json_array_get_string, const JSON_Array*, arr, size_t, index);
MOCKABLE_FUNCTION(, JSON_Array*, json_value_get_array, const JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Value*, json_array_get_value, const JSON_Array*, arr, size_t, index);
MOCKABLE_FUNCTION(, JSON_Value_Type, json_value_get_type, const JSON_Value*, value);

MOCK_FUNCTION_WITH_CODE(, MODULE_API*, Fake_GetAPI, MODULE_API_VERSION, gateway_api_version)
MODULE_API* val = (MODULE_API*)0x42;
MOCK_FUNCTION_END(val)

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

MOCK_FUNCTION_WITH_CODE(, size_t, json_array_get_count, const JSON_Array*, arr)
size_t num = 0;
if (arr != NULL)
{
    num = (size_t)arr;
}
MOCK_FUNCTION_END(num)

MOCK_FUNCTION_WITH_CODE(, JSON_Array*, json_value_get_array, const JSON_Value *, value)
JSON_Array* arr = NULL;
if (value != NULL)
{
    arr = (JSON_Array*)value;
}
MOCK_FUNCTION_END(arr)

MOCK_FUNCTION_WITH_CODE(, JSON_Value*, json_array_get_value, const JSON_Array*, arr, size_t, index)
JSON_Value* val = NULL;
if (arr != NULL && index >= 0)
{
    val = (JSON_Value*)0x42;
}
MOCK_FUNCTION_END(val)

MOCK_FUNCTION_WITH_CODE(, JSON_Value_Type, json_value_get_type, const JSON_Value*, value)
JSON_Value_Type val = JSONError;
if (value != NULL)
{
    val = JSONString;
}
MOCK_FUNCTION_END(val)

MOCK_FUNCTION_WITH_CODE(, JSON_Object*, json_object_get_object, const JSON_Object*, value, const char*, name)
JSON_Object* obj = NULL;
if (value != NULL && name != NULL)
{
    obj = (JSON_Object*)0x42;
}
MOCK_FUNCTION_END(obj)

MOCK_FUNCTION_WITH_CODE(, JSON_Array*, json_object_get_array, const JSON_Object*, value, const char*, name)
JSON_Array* arr = NULL;
if (value != NULL && name != NULL)
{
    arr = (JSON_Array*)(0x42);
}
MOCK_FUNCTION_END(arr)

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
    int real_STRING_concat_with_STRING(STRING_HANDLE s1, STRING_HANDLE s2);

    //mallocAndStrcpy_s
    int real_mallocAndStrcpy_s(char** destination, const char* source);
#ifdef __cplusplus
}
#endif

#undef ENABLE_MOCKS

TEST_DEFINE_ENUM_TYPE(MODULE_LOADER_RESULT, MODULE_LOADER_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(MODULE_LOADER_RESULT, MODULE_LOADER_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(MODULE_LOADER_TYPE, MODULE_LOADER_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(MODULE_LOADER_TYPE, MODULE_LOADER_TYPE_VALUES);

BEGIN_TEST_SUITE(JavaLoader_UnitTests)

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
    REGISTER_UMOCK_ALIAS_TYPE(VECTOR_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const VECTOR_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DYNAMIC_LIBRARY_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MODULE_LIBRARY_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(JSON_Value_Type, int);
    REGISTER_UMOCK_ALIAS_TYPE(MODULE_API_VERSION, int);
    REGISTER_GLOBAL_MOCK_RETURN(DynamicLibrary_LoadLibrary, (DYNAMIC_LIBRARY_HANDLE)0x42);
    REGISTER_GLOBAL_MOCK_RETURN(DynamicLibrary_FindSymbol, (void*)0x42);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_object, NULL);

    // malloc/free hooks
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    // Strings hooks
    REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, real_STRING_construct);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_clone, real_STRING_clone);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, real_STRING_delete);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_c_str, real_STRING_c_str);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_concat, real_STRING_concat);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_concat_with_STRING, real_STRING_concat_with_STRING)
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_clone, NULL);

    //Vector Hooks
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_create, real_VECTOR_create);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_push_back, real_VECTOR_push_back);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_size, real_VECTOR_size);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_destroy, real_VECTOR_destroy);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_element, real_VECTOR_element);

    //mallocAndStrcpy_s Hook
    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, real_mallocAndStrcpy_s);

    g_loader = JavaLoader_Get();
    g_config = g_loader->configuration;
    JavaModuleLoader_Load = g_loader->api->Load;
    JavaModuleLoader_Unload = g_loader->api->Unload;
    JavaModuleLoader_GetModuleApi = g_loader->api->GetApi;
    JavaModuleLoader_ParseEntrypointFromJson = g_loader->api->ParseEntrypointFromJson;
    JavaModuleLoader_FreeEntrypoint = g_loader->api->FreeEntrypoint;
    JavaModuleLoader_ParseConfigurationFromJson = g_loader->api->ParseConfigurationFromJson;
    JavaModuleLoader_FreeConfiguration = g_loader->api->FreeConfiguration;
    JavaModuleLoader_BuildModuleConfiguration = g_loader->api->BuildModuleConfiguration;
    JavaModuleLoader_FreeModuleConfiguration = g_loader->api->FreeModuleConfiguration;
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    g_loader->api->FreeConfiguration(g_loader, g_config);

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

//=============================================================================
//JavaModuleLoader_Load tests
//=============================================================================

/*Tests_SRS_JAVA_MODULE_LOADER_14_004: [JavaModuleLoader_Load shall load the binding module library into memory by calling DynamicLibrary_LoadLibrary.]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_005: [JavaModuleLoader_Load shall use the binding module path given in loader->configuration->binding_path if loader->configuration is not NULL.]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_006: [JavaModuleLoader_Load shall call DynamicLibrary_FindSymbol on the binding module handle with the symbol name Module_GetApi to acquire the module's API table. ]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_007: [JavaModuleLoader_Load shall return a non - NULL pointer of type MODULE_LIBRARY_HANDLE when successful.]*/
TEST_FUNCTION(JavaModuleLoader_Load_success_no_binding_path)
{
    //Arrange
    MODULE_LOADER loader =
    {
        JAVA,
        NULL,
        NULL,
        NULL
    };

    MODULE_API_1 api =
    {
        {
            MODULE_API_VERSION_1
        },
        NULL,
        NULL,
        (pfModule_Create)0x42,
        (pfModule_Destroy)0x42,
        (pfModule_Receive)0x42,
        NULL
    };

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DynamicLibrary_LoadLibrary(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DynamicLibrary_FindSymbol(IGNORED_PTR_ARG, MODULE_GETAPI_NAME))
        .IgnoreArgument(1)
        .SetReturn((void*)Fake_GetAPI);
    STRICT_EXPECTED_CALL(Fake_GetAPI((MODULE_API_VERSION)IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .SetReturn((MODULE_API*)&api);

    //Act
    MODULE_LIBRARY_HANDLE result = JavaModuleLoader_Load(&loader, NULL);

    //Assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    JavaModuleLoader_Unload(&loader, result);
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_004: [JavaModuleLoader_Load shall load the binding module library into memory by calling DynamicLibrary_LoadLibrary.]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_005: [JavaModuleLoader_Load shall use the binding module path given in loader->configuration->binding_path if loader->configuration is not NULL.]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_006: [JavaModuleLoader_Load shall call DynamicLibrary_FindSymbol on the binding module handle with the symbol name Module_GetApi to acquire the module's API table. ]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_007: [JavaModuleLoader_Load shall return a non - NULL pointer of type MODULE_LIBRARY_HANDLE when successful.]*/
TEST_FUNCTION(JavaModuleLoader_Load_success_with_binding_path)
{
    //Arrange
    STRING_HANDLE str = STRING_construct("path");
    umock_c_reset_all_calls();
    MODULE_LOADER_BASE_CONFIGURATION base =
    {
        str
    };

    MODULE_LOADER loader =
    {
        JAVA,
        NULL,
        &base,
        NULL
    };

    MODULE_API_1 api =
    {
        {
            MODULE_API_VERSION_1
        },
        NULL,
        NULL,
        (pfModule_Create)0x42,
        (pfModule_Destroy)0x42,
        (pfModule_Receive)0x42,
        NULL
    };

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(str));
    STRICT_EXPECTED_CALL(DynamicLibrary_LoadLibrary(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DynamicLibrary_FindSymbol(IGNORED_PTR_ARG, MODULE_GETAPI_NAME))
        .IgnoreArgument(1)
        .SetReturn((void*)Fake_GetAPI);
    STRICT_EXPECTED_CALL(Fake_GetAPI((MODULE_API_VERSION)IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .SetReturn((MODULE_API*)&api);

    //Act
    MODULE_LIBRARY_HANDLE result = JavaModuleLoader_Load(&loader, NULL);

    //Assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    JavaModuleLoader_Unload(&loader, result);
    STRING_delete(str);
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_001: [JavaModuleLoader_Load shall return NULL if loader is NULL.]*/
TEST_FUNCTION(JavaModuleLoader_Load_returns_NULL_when_loader_is_NULL)
{
    //Arrange
    

    //Act
    MODULE_LIBRARY_HANDLE result = JavaModuleLoader_Load(NULL, NULL);

    //Assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_002: [JavaModuleLoader_Load shall return NULL if loader->type is not JAVA.]*/
TEST_FUNCTION(JavaModuleLoader_Load_returns_NULL_when_type_not_JAVA)
{
    //Arrange
    MODULE_LOADER loader =
    {
        NATIVE,
        NULL,
        NULL,
        NULL
    };

    //Act
    MODULE_LIBRARY_HANDLE result = JavaModuleLoader_Load(&loader, NULL);

    //Assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_003: [JavaModuleLoader_Load shall return NULL if an underlying platform call fails.]*/
TEST_FUNCTION(JavaModuleLoader_Load_returns_NULL_when_things_fail)
{
    //Arrange
    MODULE_LOADER loader =
    {
        JAVA,
        NULL,
        NULL,
        NULL
    };

    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(DynamicLibrary_LoadLibrary(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(DynamicLibrary_FindSymbol(IGNORED_PTR_ARG, MODULE_GETAPI_NAME))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        //arrange
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        //act
        MODULE_LIBRARY_HANDLE result = JavaModuleLoader_Load(&loader, NULL);

        //Assert
        ASSERT_IS_NULL(result);
    }

    umock_c_negative_tests_deinit();
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_008: [JavaModuleLoader_Load shall return NULL if MODULE_API returned by the binding module is NULL.]*/
TEST_FUNCTION(JavaModuleLoader_Load_returns_NULL_when_GetAPI_returns_NULL)
{
    // arrange
    MODULE_LOADER loader =
    {
        JAVA,
        NULL, NULL, NULL
    };

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DynamicLibrary_LoadLibrary(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DynamicLibrary_FindSymbol(IGNORED_PTR_ARG, MODULE_GETAPI_NAME))
        .IgnoreArgument(1)
        .SetReturn((void*)Fake_GetAPI);
    STRICT_EXPECTED_CALL(Fake_GetAPI((MODULE_API_VERSION)IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(DynamicLibrary_UnloadLibrary(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    MODULE_LIBRARY_HANDLE result = JavaModuleLoader_Load(&loader, NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_009: [JavaModuleLoader_Load shall return NULL if MODULE_API::version is great than Module_ApiGatewayVersion.]*/
TEST_FUNCTION(JavaModuleLoader_Load_returns_NULL_when_GetAPI_returns_API_with_unsupported_version)
{
    // arrange
    MODULE_LOADER loader =
    {
        JAVA,
        NULL, NULL, NULL
    };
    MODULE_API api =
    {
        (MODULE_API_VERSION)(Module_ApiGatewayVersion + 1)
    };

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DynamicLibrary_LoadLibrary(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DynamicLibrary_FindSymbol(IGNORED_PTR_ARG, MODULE_GETAPI_NAME))
        .IgnoreArgument(1)
        .SetReturn((void*)Fake_GetAPI);
    STRICT_EXPECTED_CALL(Fake_GetAPI((MODULE_API_VERSION)IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .SetReturn(&api);
    STRICT_EXPECTED_CALL(DynamicLibrary_UnloadLibrary(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    MODULE_LIBRARY_HANDLE result = JavaModuleLoader_Load(&loader, NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}
/*Tests_SRS_JAVA_MODULE_LOADER_14_010: [JavaModuleLoader_Load shall return NULL if the Module_Create function in MODULE_API is NULL.]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_011: [JavaModuleLoader_Load shall return NULL if the Module_Receive function in MODULE_API is NULL.]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_012: [JavaModuleLoader_Load shall return NULL if the Module_Destroy function in MODULE_API is NULL.]*/
TEST_FUNCTION(JavaModuleLoader_Load_returns_NULL_when_GetAPI_returns_API_with_invalid_callbacks)
{
    // arrange
    MODULE_LOADER loader =
    {
        JAVA,
        NULL, NULL, NULL
    };
    MODULE_API_1 api_inputs[] =
    {
        {
            {
                MODULE_API_VERSION_1
            },
        NULL,
        NULL,
        NULL,
        (pfModule_Destroy)0x42,
        (pfModule_Receive)0x42,
        NULL
        },
        {
            {
                MODULE_API_VERSION_1
            },
        NULL,
        NULL,
        (pfModule_Create)0x42,
        NULL,
        (pfModule_Receive)0x42,
        NULL
        },
        {
            {
                MODULE_API_VERSION_1
            },
        NULL,
        NULL,
        (pfModule_Create)0x42,
        (pfModule_Destroy)0x42,
        NULL,
        NULL
        }
    };

    for (size_t i = 0; i < sizeof(api_inputs) / sizeof(api_inputs[0]); i++)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(DynamicLibrary_LoadLibrary(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(DynamicLibrary_FindSymbol(IGNORED_PTR_ARG, MODULE_GETAPI_NAME))
            .IgnoreArgument(1)
            .SetReturn((void*)Fake_GetAPI);
        STRICT_EXPECTED_CALL(Fake_GetAPI((MODULE_API_VERSION)IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .SetReturn((MODULE_API*)&(api_inputs[i]));
        STRICT_EXPECTED_CALL(DynamicLibrary_UnloadLibrary(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
    }

    for (size_t i = 0; i < sizeof(api_inputs) / sizeof(api_inputs[0]); i++)
    {
        // act
        MODULE_LIBRARY_HANDLE result = JavaModuleLoader_Load(&loader, NULL);

        // assert
        ASSERT_IS_NULL(result);
    }

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//=============================================================================
//JavaModuleLoader_GetModuleApi tests
//=============================================================================

/*Tests_SRS_JAVA_MODULE_LOADER_14_013: [ JavaModuleLoader_GetModuleApi shall return NULL if moduleLibraryHandle is NULL. ]*/
TEST_FUNCTION(JavaModuleLoader_GetModuleApi_returns_NULL_when_moduleLibraryHandle_is_NULL)
{
    // act
    const MODULE_API* result = JavaModuleLoader_GetModuleApi(IGNORED_PTR_ARG, NULL);

    // assert
    ASSERT_IS_NULL(result);
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_014: [ JavaModuleLoader_GetModuleApi shall return a non-NULL MODULE_API pointer when successful. ]*/
TEST_FUNCTION(JavaModuleLoader_GetModuleApi_succeeds)
{
    // arrange
    MODULE_LOADER loader =
    {
        JAVA,
        NULL, NULL, NULL
    };
    MODULE_API_1 api =
    {
        {
            MODULE_API_VERSION_1
        },
        NULL,
        NULL,
        (pfModule_Create)0x42,
        (pfModule_Destroy)0x42,
        (pfModule_Receive)0x42,
        NULL
    };

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DynamicLibrary_LoadLibrary(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DynamicLibrary_FindSymbol(IGNORED_PTR_ARG, MODULE_GETAPI_NAME))
        .IgnoreArgument(1)
        .SetReturn((void*)Fake_GetAPI);
    STRICT_EXPECTED_CALL(Fake_GetAPI((MODULE_API_VERSION)IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .SetReturn((MODULE_API*)&api);

    MODULE_LIBRARY_HANDLE module = JavaModuleLoader_Load(&loader, NULL);
    ASSERT_IS_NOT_NULL(module);

    umock_c_reset_all_calls();

    // act
    const MODULE_API* result = JavaModuleLoader_GetModuleApi(&loader, module);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    JavaModuleLoader_Unload(&loader, module);
}

//=============================================================================
//JavaModuleLoader_Unload tests
//=============================================================================

/*Tests_SRS_JAVA_MODULE_LOADER_14_015: [JavaModuleLoader_Unload shall do nothing if moduleLibraryHandle is NULL.]*/
TEST_FUNCTION(JavaModuleLoader_Unload_does_nothing_when_moduleLibraryHandle_is_NULL)
{
    // act
    JavaModuleLoader_Unload(IGNORED_PTR_ARG, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}
/*Tests_SRS_JAVA_MODULE_LOADER_14_016: [JavaModuleLoader_Unload shall unload the binding module from memory by calling DynamicLibrary_UnloadLibrary.]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_017: [JavaModuleLoader_Unload shall free resources allocated when loading the binding module.]*/
TEST_FUNCTION(JavaModuleLoader_Unload_frees_things)
{
    // arrange
    MODULE_LOADER loader =
    {
        JAVA,
        NULL, NULL, NULL
    };
    MODULE_API_1 api =
    {
        {
            MODULE_API_VERSION_1
        },
        NULL,
        NULL,
        (pfModule_Create)0x42,
        (pfModule_Destroy)0x42,
        (pfModule_Receive)0x42,
        NULL
    };

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DynamicLibrary_LoadLibrary(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DynamicLibrary_FindSymbol(IGNORED_PTR_ARG, MODULE_GETAPI_NAME))
        .IgnoreArgument(1)
        .SetReturn((void*)Fake_GetAPI);
    STRICT_EXPECTED_CALL(Fake_GetAPI((MODULE_API_VERSION)IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .SetReturn((MODULE_API*)&api);

    MODULE_LIBRARY_HANDLE module = JavaModuleLoader_Load(&loader, NULL);
    ASSERT_IS_NOT_NULL(module);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(DynamicLibrary_UnloadLibrary((DYNAMIC_LIBRARY_HANDLE)0x42));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    JavaModuleLoader_Unload(&loader, module);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_043: [JavaModuleLoader_ParseEntrypointFromJson shall return NULL if loader is NULL.]*/
TEST_FUNCTION(JavaModuleLoader_ParseEntrypointFromJson_returns_NULL_when_loader_is_NULL)
{
    //Act
    void* result = JavaModuleLoader_ParseEntrypointFromJson(NULL, (const JSON_Value*)0x42);

    //Asert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_018: [JavaModuleLoader_ParseEntrypointFromJson shall return NULL if json is NULL.]*/
TEST_FUNCTION(JavaModuleLoader_ParseEntrypointFromJson_returns_NULL_when_json_is_NULL)
{
    // act
    void* result = JavaModuleLoader_ParseEntrypointFromJson((MODULE_LOADER*)0x42, NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_019: [JavaModuleLoader_ParseEntrypointFromJson shall return NULL if the root json entity is not an object.]*/\
TEST_FUNCTION(JavaModuleLoader_ParseEntrypointFromJson_returns_NULL_when_json_is_not_an_object)
{
    STRING_HANDLE str = STRING_construct("base.path");

    MODULE_LOADER_BASE_CONFIGURATION base = 
    {
        str
    };

    JVM_OPTIONS options =
    {
        NULL,
        NULL,
        0,
        false,
        0,
        false,
        NULL
    };

    JAVA_LOADER_CONFIGURATION config =
    {
        base,
        &options
    };

    MODULE_LOADER loader = 
    {
        JAVA,
        NULL,
        (MODULE_LOADER_BASE_CONFIGURATION*)(&config),
        NULL
    };

    umock_c_reset_all_calls();

    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONArray);

    // act
    void* result = JavaModuleLoader_ParseEntrypointFromJson(&loader, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    
    //Cleanup
    STRING_delete(str);
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_058: [JavaModuleLoader_ParseEntrypointFromJson shall return NULL if either class.name or class.path is non - existent.]*/
TEST_FUNCTION(JavaModuleLoader_ParseEntrypointFromJson_returns_NULL_when_classname_doesnt_exist)
{
    STRING_HANDLE str = STRING_construct("base.path");

    MODULE_LOADER_BASE_CONFIGURATION base =
    {
        str
    };

    JVM_OPTIONS options =
    {
        NULL,
        NULL,
        0,
        false,
        0,
        false,
        NULL
    };

    JAVA_LOADER_CONFIGURATION config =
    {
        base,
        &options
    };

    MODULE_LOADER loader =
    {
        JAVA,
        NULL,
        (MODULE_LOADER_BASE_CONFIGURATION*)(&config),
        NULL
    };

    umock_c_reset_all_calls();
    //Arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x43);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, "class.name"))
        .SetReturn(NULL);

    //Act
    void* result = JavaModuleLoader_ParseEntrypointFromJson(&loader, (const JSON_Value*)0x42);
    
    //Assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    STRING_delete(str);
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_058: [JavaModuleLoader_ParseEntrypointFromJson shall return NULL if either class.name or class.path is non - existent.]*/
TEST_FUNCTION(JavaModuleLoader_ParseEntrypointFromJson_returns_NULL_when_classpath_doesnt_exist)
{
    STRING_HANDLE str = STRING_construct("base.path");

    MODULE_LOADER_BASE_CONFIGURATION base =
    {
        str
    };

    JVM_OPTIONS options =
    {
        NULL,
        NULL,
        0,
        false,
        0,
        false,
        NULL
    };

    JAVA_LOADER_CONFIGURATION config =
    {
        base,
        &options
    };

    MODULE_LOADER loader =
    {
        JAVA,
        NULL,
        (MODULE_LOADER_BASE_CONFIGURATION*)(&config),
        NULL
    };

    umock_c_reset_all_calls();
    //Arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x43);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, ENTRYPOINT_CLASSNAME))
        .SetReturn(ENTRYPOINT_CLASSNAME);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, ENTRYPOINT_CLASSPATH))
        .SetReturn(NULL);

    //Act
    void* result = JavaModuleLoader_ParseEntrypointFromJson(&loader, (const JSON_Value*)0x42);

    //Assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    STRING_delete(str);
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_059: [JavaModuleLoader_ParseEntrypointFromJson shall return NULL if loader->configuration.]*/
TEST_FUNCTION(JavaModuleLoader_ParseEntrypointFromJson_returns_NULL_if_loader_config_is_NULL)
{
    MODULE_LOADER loader =
    {
        JAVA,
        NULL,
        NULL,
        NULL
    };

    //Act
    void* result = JavaModuleLoader_ParseEntrypointFromJson(&loader, (const JSON_Value*)0x42);

    //Assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_060: [JavaModuleLoader_ParseEntrypointFromJson shall return NULL if loader->configuration->options is NULL.]*/
TEST_FUNCTION(JavaModuleLoader_ParseEntrypointFromJson_returns_NULL_if_loader_config_options_is_NULL)
{
    STRING_HANDLE str = STRING_construct("base.path");

    MODULE_LOADER_BASE_CONFIGURATION base =
    {
        str
    };

    JAVA_LOADER_CONFIGURATION config =
    {
        base,
        NULL
    };

    MODULE_LOADER loader =
    {
        JAVA,
        NULL,
        (MODULE_LOADER_BASE_CONFIGURATION*)(&config),
        NULL
    };

    umock_c_reset_all_calls();

    //Act
    void* result = JavaModuleLoader_ParseEntrypointFromJson(&loader, (const JSON_Value*)0x42);

    //Assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    STRING_delete(str);
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_020: [JavaModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails.]*/
TEST_FUNCTION(JavaModuleLoader_ParseEntrypointFromJson_failure_tests)
{
    STRING_HANDLE str = STRING_construct("base.path");
    const char* cp = (const char*)malloc(strlen(ENTRYPOINT_CLASSPATH) + 1);
    strcpy((char*)cp, ENTRYPOINT_CLASSPATH);
    const char* lp = (const char*)malloc(strlen(JAVA_MODULE_LIBRARY_PATH_KEY) + 1);
    strcpy((char*)lp, JAVA_MODULE_LIBRARY_PATH_KEY);

    MODULE_LOADER_BASE_CONFIGURATION base =
    {
        str
    };

    JVM_OPTIONS options =
    {
        cp,
        lp,
        0,
        false,
        0,
        false,
        NULL
    };

    JAVA_LOADER_CONFIGURATION config =
    {
        base,
        &options
    };

    MODULE_LOADER loader =
    {
        JAVA,
        NULL,
        (MODULE_LOADER_BASE_CONFIGURATION*)(&config),
        NULL
    };

    umock_c_reset_all_calls();

    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    //Arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONObject)
        .SetFailReturn(JSONArray);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x43)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, ENTRYPOINT_CLASSNAME))
        .SetReturn(ENTRYPOINT_CLASSNAME)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, ENTRYPOINT_CLASSPATH))
        .SetReturn(ENTRYPOINT_CLASSPATH)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (i != 11)
        {
            //arrange
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //Act
            void* result = JavaModuleLoader_ParseEntrypointFromJson(&loader, (const JSON_Value*)0x42);

            //Assert
            ASSERT_IS_NULL(result);
        }
    }

    umock_c_negative_tests_deinit();

    //Cleanup
    STRING_delete(str);
    //free((void*)cp);
    free((void*)lp);
    umock_c_reset_all_calls();
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_021: [JavaModuleLoader_ParseEntrypointFromJson shall retreive the fully qualified class name by reading the value of the attribute class.name.]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_022: [JavaModuleLoader_ParseEntrypointFromJson shall retreive the full class path containing the class definition for the module by reading the value of the attribute class.path.]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_023: [JavaModuleLoader_ParseEntrypointFromJson shall return a non - NULL pointer to the parsed representation of the entrypoint when successful.]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_044: [JavaModuleLoader_ParseEntrypointFromJson shall append the classpath to the loader's configuration JVM_OPTIONS class_path member. ]*/
TEST_FUNCTION(JavaModuleLoader_ParseEntrypointFromJson_gets_class_name)
{
    STRING_HANDLE str = STRING_construct("base.path");
    const char* cp = (const char*)malloc(strlen(ENTRYPOINT_CLASSPATH) + 1);
    strcpy((char*)cp, ENTRYPOINT_CLASSPATH);
    const char* lp = (const char*)malloc(strlen(JAVA_MODULE_LIBRARY_PATH_KEY) + 1);
    strcpy((char*)lp, JAVA_MODULE_LIBRARY_PATH_KEY);

    MODULE_LOADER_BASE_CONFIGURATION base =
    {
        str
    };

    JVM_OPTIONS options =
    {
        cp,
        lp,
        0,
        false,
        0,
        false,
        NULL
    };

    JAVA_LOADER_CONFIGURATION config =
    {
        base,
        &options
    };

    MODULE_LOADER loader =
    {
        JAVA,
        NULL,
        (MODULE_LOADER_BASE_CONFIGURATION*)(&config),
        NULL
    };

    umock_c_reset_all_calls();

    //Arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x43);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, ENTRYPOINT_CLASSNAME))
        .SetReturn(ENTRYPOINT_CLASSNAME);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, ENTRYPOINT_CLASSPATH))
        .SetReturn(ENTRYPOINT_CLASSPATH);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //Act
    void* result = JavaModuleLoader_ParseEntrypointFromJson(&loader, (const JSON_Value*)0x42);

    //Assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
#if WIN32
    ASSERT_ARE_EQUAL(char_ptr, "class.path;class.path", ((JAVA_LOADER_CONFIGURATION*)(loader.configuration))->options->class_path);
#else
    ASSERT_ARE_EQUAL(char_ptr, "class.path:class.path", ((JAVA_LOADER_CONFIGURATION*)(loader.configuration))->options->class_path);
#endif
    //Cleanup
    JavaModuleLoader_FreeEntrypoint(&loader, result);
    free((void*)options.class_path);
    free((void*)options.library_path);
    STRING_delete(str);
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_024: [JavaModuleLoader_FreeEntrypoint shall do nothing if entrypoint is NULL.]*/
TEST_FUNCTION(JavaModuleLoader_FreeEntrypoint_does_nothing_when_entrypoint_is_NULL)
{
    // act
    JavaModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_025 : [JavaModuleLoader_FreeEntrypoint shall free resources allocated during JavaModuleLoader_ParseEntrypointFromJson.]*/
TEST_FUNCTION(JavaModuleLoader_FreeEntrypoint_frees_resources)
{
    // arrange
    STRING_HANDLE str = STRING_construct("base.path");
    const char* cp = (const char*)malloc(strlen(ENTRYPOINT_CLASSPATH) + 1);
    strcpy((char*)cp, ENTRYPOINT_CLASSPATH);
    const char* lp = (const char*)malloc(strlen(JAVA_MODULE_LIBRARY_PATH_KEY) + 1);
    strcpy((char*)lp, JAVA_MODULE_LIBRARY_PATH_KEY);

    MODULE_LOADER_BASE_CONFIGURATION base =
    {
        str
    };

    JVM_OPTIONS options =
    {
        cp,
        lp,
        0,
        false,
        0,
        false,
        NULL
    };

    JAVA_LOADER_CONFIGURATION config =
    {
        base,
        &options
    };

    MODULE_LOADER loader =
    {
        JAVA,
        NULL,
        (MODULE_LOADER_BASE_CONFIGURATION*)(&config),
        NULL
    };

    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x43);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, ENTRYPOINT_CLASSNAME))
        .SetReturn(ENTRYPOINT_CLASSNAME);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, ENTRYPOINT_CLASSPATH))
        .SetReturn(ENTRYPOINT_CLASSPATH);

    void* entrypoint = JavaModuleLoader_ParseEntrypointFromJson(&loader, (const JSON_Value*)0x42);
    ASSERT_IS_NOT_NULL(entrypoint);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    JavaModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, entrypoint);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    
    //Cleanup
    STRING_delete(str);
    free((void*)options.class_path);
    free((void*)options.library_path);
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_026: [JavaModuleLoader_ParseConfigurationFromJson shall return NULL if json is NULL.]*/
TEST_FUNCTION(JavaModuleLoader_ParseConfigurationFromJson_returns_NULL_if_JSON_is_NULL)
{
    //Act
    MODULE_LOADER_BASE_CONFIGURATION* config = JavaModuleLoader_ParseConfigurationFromJson(IGNORED_PTR_ARG, NULL);

    //Assert
    ASSERT_IS_NULL(config);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_027: [JavaModuleLoader_ParseConfigurationFromJson shall return NULL if json is not a valid JSON object.]*/
TEST_FUNCTION(JavaModuleLoader_ParseConfigurationFromJson_return_NULL_if_json_object_is_invalid)
{
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn(NULL);

    //Act
    MODULE_LOADER_BASE_CONFIGURATION* config = JavaModuleLoader_ParseConfigurationFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    //Assert
    ASSERT_IS_NULL(config);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_028: [JavaModuleLoader_ParseConfigurationFromJson shall return NULL if any underlying platform call fails.]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_036: [JavaModuleLoader_ParseConfigurationFromJson shall return NULL if any present field cannot be parsed.]*/
TEST_FUNCTION(JavaModuleLoader_ParseConfigurationFromJson_failure_tests)
{
    //Arrange
    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x43)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(json_object_get_object((JSON_Object*)0x43, JVM_OPTIONS_KEY))
        .SetReturn((JSON_Object*)0x42)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JAVA_LOADER_CONFIGURATION)))
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JVM_OPTIONS)))
        .SetFailReturn(NULL);
    //START: set_default_paths
    //START: set_path
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    //END: set_path
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1);
    //START: set_path
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    //END: set_path
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    //END: set_default_paths
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, JAVA_MODULE_CLASS_PATH_KEY))
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, JAVA_MODULE_LIBRARY_PATH_KEY))
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1);
    STRICT_EXPECTED_CALL(json_object_get_number((const JSON_Object*)0x42, JAVA_MODULE_JVM_OPTIONS_VERSION_KEY));
    STRICT_EXPECTED_CALL(json_object_get_boolean((const JSON_Object*)0x42, JAVA_MODULE_JVM_OPTIONS_DEBUG_KEY));
    STRICT_EXPECTED_CALL(json_object_get_number((const JSON_Object*)0x42, JAVA_MODULE_JVM_OPTIONS_DEBUG_PORT_KEY));
    STRICT_EXPECTED_CALL(json_object_get_boolean((const JSON_Object*)0x42, JAVA_MODULE_JVM_OPTIONS_VERBOSE_KEY));
    STRICT_EXPECTED_CALL(json_object_get_array((const JSON_Object*)0x42, JAVA_MODULE_JVM_OPTIONS_ADDITIONAL_OPTIONS_KEY))
        .SetFailReturn(NULL)
        .SetReturn((JSON_Array*)0x02);
    STRICT_EXPECTED_CALL(json_array_get_count(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_create(sizeof(STRING_HANDLE)))
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(json_array_get_string(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1)
        .SetReturn("string")
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1);
    STRICT_EXPECTED_CALL(json_array_get_string(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .SetReturn("string")
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1);
    STRICT_EXPECTED_CALL(ModuleLoader_ParseBaseConfigurationFromJson(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(MODULE_LOADER_ERROR)
        .SetReturn(MODULE_LOADER_SUCCESS);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (i != 7 &&
            i != 12 &&
            i != 14 &&
            i != 15 &&
            i != 16 &&
            i != 17 &&
            i != 19 &&
            i != 20 &&
            i != 22 && 
            i != 23 && 
            i != 24 && 
            i != 25 && 
            i != 26 && 
            i != 27 )
        {
            //arrange
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //Act
            MODULE_LOADER_BASE_CONFIGURATION* result = JavaModuleLoader_ParseConfigurationFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

            //Assert
            ASSERT_IS_NULL(result);
        }
    }

    umock_c_negative_tests_deinit();
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_028: [JavaModuleLoader_ParseConfigurationFromJson shall return NULL if any underlying platform call fails.]*/
TEST_FUNCTION(JavaModuleLoader_ParseConfigurationFromJson_sprintf_failure)
{
    //Arrange
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x43);
    STRICT_EXPECTED_CALL(json_object_get_object((JSON_Object*)0x43, JVM_OPTIONS_KEY))
        .SetReturn((JSON_Object*)0x42);
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JAVA_LOADER_CONFIGURATION)));
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JVM_OPTIONS)));
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(NULL);

    //Act
    MODULE_LOADER_BASE_CONFIGURATION* result = JavaModuleLoader_ParseConfigurationFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    //Assert
    ASSERT_IS_NULL(result);
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_028: [JavaModuleLoader_ParseConfigurationFromJson shall return NULL if any underlying platform call fails.]*/
TEST_FUNCTION(JavaModuleLoader_ParseConfigurationFromJson_sprintf_failure_2)
{
    //Arrange
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x43);
    STRICT_EXPECTED_CALL(json_object_get_object((JSON_Object*)0x43, JVM_OPTIONS_KEY))
        .SetReturn((JSON_Object*)0x42);
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JAVA_LOADER_CONFIGURATION)));
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JVM_OPTIONS)));
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(NULL);

    //Act
    MODULE_LOADER_BASE_CONFIGURATION* result = JavaModuleLoader_ParseConfigurationFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    //Assert
    ASSERT_IS_NULL(result);
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_029: [JavaModuleLoader_ParseConfigurationFromJson shall set any missing field to NULL, false, or 0.]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_037: [JavaModuleLoader_ParseConfigurationFromJson shall return a non - NULL JAVA_LOADER_CONFIGURATION containing all user - specified values.]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_039: [JavaModuleLoader_ParseConfigurationFromJson shall set the base member of the JAVA_LOADER_CONFIGURATION by calling to the base module loader.]*/
TEST_FUNCTION(JavaModuleLoader_ParseConfigurationFromJson_sets_missing_fields)
{
    //Arrange
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x43);
    STRICT_EXPECTED_CALL(json_object_get_object((JSON_Object*)0x43, JVM_OPTIONS_KEY))
        .SetReturn((JSON_Object*)0x42);
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JAVA_LOADER_CONFIGURATION)));
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JVM_OPTIONS)));
    //START: set_default_paths
    //START: set_path
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    //END: set_path
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    //START: set_path
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    //END: set_path
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    //END: set_default_paths
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, JAVA_MODULE_CLASS_PATH_KEY))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, JAVA_MODULE_LIBRARY_PATH_KEY))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(json_object_get_number((const JSON_Object*)0x42, JAVA_MODULE_JVM_OPTIONS_VERSION_KEY))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(json_object_get_boolean((const JSON_Object*)0x42, JAVA_MODULE_JVM_OPTIONS_DEBUG_KEY))
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(json_object_get_number((const JSON_Object*)0x42, JAVA_MODULE_JVM_OPTIONS_DEBUG_PORT_KEY))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(json_object_get_boolean((const JSON_Object*)0x42, JAVA_MODULE_JVM_OPTIONS_VERBOSE_KEY))
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(json_object_get_array((const JSON_Object*)0x42, JAVA_MODULE_JVM_OPTIONS_ADDITIONAL_OPTIONS_KEY))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(json_array_get_count(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(ModuleLoader_ParseBaseConfigurationFromJson(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(MODULE_LOADER_SUCCESS);

    //Act
    MODULE_LOADER_BASE_CONFIGURATION *result = JavaModuleLoader_ParseConfigurationFromJson(IGNORED_PTR_ARG, (JSON_Value*)0x42);

    JVM_OPTIONS* options = ((JAVA_LOADER_CONFIGURATION*)result)->options;

    //Assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
#ifdef _WIN64
    ASSERT_ARE_EQUAL(char_ptr, "C:\\Program Files\\azure_iot_gateway_sdk-1.0.0\\lib\\modules", options->library_path);
    ASSERT_ARE_EQUAL(char_ptr, "C:\\Program Files\\azure_iot_gateway_sdk-1.0.0\\lib\\bindings\\java\\classes", options->class_path);
#else
#ifdef WIN32
    ASSERT_ARE_EQUAL(char_ptr, "C:\\Program Files (x86)\\azure_iot_gateway_sdk-1.0.0\\lib\\modules", options->library_path);
    ASSERT_ARE_EQUAL(char_ptr, "C:\\Program Files (x86)\\azure_iot_gateway_sdk-1.0.0\\lib\\bindings\\java\\classes", options->class_path);
#else
    ASSERT_ARE_EQUAL(char_ptr, "/usr/local/lib/azure_iot_gateway_sdk-1.0.0/modules", options->library_path);
    ASSERT_ARE_EQUAL(char_ptr, "/usr/local/lib/azure_iot_gateway_sdk-1.0.0/bindings/java/classes", options->class_path);
#endif
#endif
    ASSERT_ARE_EQUAL(int, 0, options->version);
    ASSERT_IS_TRUE(options->debug == false);
    ASSERT_ARE_EQUAL(int, 0, options->debug_port);
    ASSERT_IS_TRUE(options->verbose == false);

    //Cleanup
    JavaModuleLoader_FreeConfiguration(IGNORED_PTR_ARG, result);

}
/*Tests_SRS_JAVA_MODULE_LOADER_14_030: [JavaModuleLoader_ParseConfigurationFromJson shall parse the jvm.options JSON object and initialize a new JAVA_LOADER_CONFIGURATION.]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_031: [JavaModuleLoader_ParseConfigurationFromJson shall parse the jvm.options.library.path.]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_032: [JavaModuleLoader_ParseConfigurationFromJson shall parse the jvm.options.version.]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_033: [JavaModuleLoader_ParseConfigurationFromJson shall parse the jvm.options.debug.]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_034: [JavaModuleLoader_ParseConfigurationFromJson shall parse the jvm.options.debug.port.]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_035: [JavaModuleLoader_ParseConfigurationFromJson shall parse the jvm.options.additional.options object and create a new STRING_HANDLE for each.]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_037: [JavaModuleLoader_ParseConfigurationFromJson shall return a non - NULL JAVA_LOADER_CONFIGURATION containing all user - specified values.]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_038: [JavaModuleLoader_ParseConfigurationFromJson shall set the options member of the JAVA_LOADER_CONFIGURATION to the parsed JVM_OPTIONS structure.]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_039: [JavaModuleLoader_ParseConfigurationFromJson shall set the base member of the JAVA_LOADER_CONFIGURATION by calling to the base module loader.]*/
TEST_FUNCTION(JavaModuleLoader_ParseConfigurationFromJson_success)
{
    //Arrange
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x43);
    STRICT_EXPECTED_CALL(json_object_get_object((JSON_Object*)0x43, JVM_OPTIONS_KEY))
        .SetReturn((JSON_Object*)0x42);
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JAVA_LOADER_CONFIGURATION)));
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JVM_OPTIONS)));
    //START: set_default_paths
    //START: set_path
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    //END: set_path
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    //START: set_path
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    //END: set_path
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    //END: set_default_paths
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, JAVA_MODULE_CLASS_PATH_KEY))
        .SetReturn(JAVA_MODULE_CLASS_PATH_KEY);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, JAVA_MODULE_LIBRARY_PATH_KEY))
        .SetReturn(JAVA_MODULE_LIBRARY_PATH_KEY);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(json_object_get_number((const JSON_Object*)0x42, JAVA_MODULE_JVM_OPTIONS_VERSION_KEY))
        .SetReturn(8);
    STRICT_EXPECTED_CALL(json_object_get_boolean((const JSON_Object*)0x42, JAVA_MODULE_JVM_OPTIONS_DEBUG_KEY))
        .SetReturn(true);
    STRICT_EXPECTED_CALL(json_object_get_number((const JSON_Object*)0x42, JAVA_MODULE_JVM_OPTIONS_DEBUG_PORT_KEY))
        .SetReturn(99);
    STRICT_EXPECTED_CALL(json_object_get_boolean((const JSON_Object*)0x42, JAVA_MODULE_JVM_OPTIONS_VERBOSE_KEY))
        .SetReturn(true);
    STRICT_EXPECTED_CALL(json_object_get_array((const JSON_Object*)0x42, JAVA_MODULE_JVM_OPTIONS_ADDITIONAL_OPTIONS_KEY))
        .SetReturn((JSON_Array*)0x02);
    STRICT_EXPECTED_CALL(json_array_get_count(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_create(sizeof(STRING_HANDLE)));
    STRICT_EXPECTED_CALL(json_array_get_string(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1)
        .SetReturn("string");
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(json_array_get_string(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .SetReturn("string");
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(ModuleLoader_ParseBaseConfigurationFromJson(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(MODULE_LOADER_SUCCESS);

    //Act
    MODULE_LOADER_BASE_CONFIGURATION *result = JavaModuleLoader_ParseConfigurationFromJson(IGNORED_PTR_ARG, (JSON_Value*)0x42);

    JVM_OPTIONS* options = ((JAVA_LOADER_CONFIGURATION*)result)->options;

    //Assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, JAVA_MODULE_CLASS_PATH_KEY, options->class_path);
    ASSERT_ARE_EQUAL(char_ptr, JAVA_MODULE_LIBRARY_PATH_KEY, options->library_path);
    ASSERT_ARE_EQUAL(int, 8, options->version);
    ASSERT_IS_TRUE(options->debug == true);
    ASSERT_ARE_EQUAL(int, 99, options->debug_port);
    ASSERT_IS_TRUE(options->verbose == true);

    //Cleanup
    JavaModuleLoader_FreeConfiguration(IGNORED_PTR_ARG, result);
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_040: [JavaModuleLoader_FreeConfiguration shall do nothing if configuration is NULL.]*/
TEST_FUNCTION(JavaModuleLoader_FreeConfiguration_does_nothing_when_configuration_is_NULL)
{
    //Act
    JavaModuleLoader_FreeConfiguration(IGNORED_PTR_ARG, NULL);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_041: [JavaModuleLoader_FreeConfiguration shall call ModuleLoader_FreeBaseConfiguration to free resources allocated by ModuleLoader_ParseBaseConfigurationJson.]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_042: [JavaModuleLoader_FreeConfiguration shall free resources allocated by JavaModuleLoader_ParseConfigurationFromJson.]*/
TEST_FUNCTION(JavaModuleLoader_FreeConfiguration_frees_resources)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x43);
    STRICT_EXPECTED_CALL(json_object_get_object((JSON_Object*)0x43, JVM_OPTIONS_KEY))
        .SetReturn((JSON_Object*)0x42);
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JAVA_LOADER_CONFIGURATION)));
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JVM_OPTIONS)));
    //START: set_default_paths
    //START: set_path
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    //END: set_path
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    //START: set_path
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    //END: set_path
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    //END: set_default_paths
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, JAVA_MODULE_CLASS_PATH_KEY))
        .SetReturn(JAVA_MODULE_CLASS_PATH_KEY);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, JAVA_MODULE_LIBRARY_PATH_KEY))
        .SetReturn(JAVA_MODULE_LIBRARY_PATH_KEY);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(json_object_get_number((const JSON_Object*)0x42, JAVA_MODULE_JVM_OPTIONS_VERSION_KEY))
        .SetReturn(8);
    STRICT_EXPECTED_CALL(json_object_get_boolean((const JSON_Object*)0x42, JAVA_MODULE_JVM_OPTIONS_DEBUG_KEY))
        .SetReturn(true);
    STRICT_EXPECTED_CALL(json_object_get_number((const JSON_Object*)0x42, JAVA_MODULE_JVM_OPTIONS_DEBUG_PORT_KEY))
        .SetReturn(99);
    STRICT_EXPECTED_CALL(json_object_get_boolean((const JSON_Object*)0x42, JAVA_MODULE_JVM_OPTIONS_VERBOSE_KEY))
        .SetReturn(true);
    STRICT_EXPECTED_CALL(json_object_get_array((const JSON_Object*)0x42, JAVA_MODULE_JVM_OPTIONS_ADDITIONAL_OPTIONS_KEY))
        .SetReturn((JSON_Array*)0x02);
    STRICT_EXPECTED_CALL(json_array_get_count(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_create(sizeof(STRING_HANDLE)));
    STRICT_EXPECTED_CALL(json_array_get_string(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1)
        .SetReturn("string");
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(json_array_get_string(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .SetReturn("string");
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(ModuleLoader_ParseBaseConfigurationFromJson(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(MODULE_LOADER_SUCCESS);

    MODULE_LOADER_BASE_CONFIGURATION* config = JavaModuleLoader_ParseConfigurationFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(ModuleLoader_FreeBaseConfiguration((MODULE_LOADER_BASE_CONFIGURATION*)config));
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(config));

    // act
    JavaModuleLoader_FreeConfiguration(IGNORED_PTR_ARG, config);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_045: [JavaModuleLoader_BuildModuleConfiguration shall return NULL if entrypoint is NULL.]*/
TEST_FUNCTION(JavaModuleLoader_BuildModuleConfiguration_returns_NULL_when_entrypoint_is_NULL)
{
    //Act
    void* result = JavaModuleLoader_BuildModuleConfiguration((const MODULE_LOADER*)0x42, NULL, (const void*)0x42);

    //Assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_046: [JavaModuleLoader_BuildModuleConfiguration shall return NULL if entrypoint->className is NULL.]*/
TEST_FUNCTION(JavaModuleLoader_BuildModuleConfiguration_returns_NULL_when_entrypoint_classname_is_NULL)
{
    //Arrange
    JAVA_LOADER_ENTRYPOINT entrypoint =
    {
        NULL,
        (STRING_HANDLE)0x42
    };

    //Act
    void* result = JavaModuleLoader_BuildModuleConfiguration((const MODULE_LOADER*)0x42, &entrypoint, (const void*)0x42);

    //Assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_047: [JavaModuleLoader_BuildModuleConfiguration shall return NULL if loader is NULL.]*/
TEST_FUNCTION(JavaModuleLoader_BuildModuleConfiguration_returns_NULL_when_loader_is_NULL)
{
    //Arrange
    JAVA_LOADER_ENTRYPOINT entrypoint =
    {
        NULL,
        (STRING_HANDLE)0x42
    };

    //Act
    void* result = JavaModuleLoader_BuildModuleConfiguration(NULL, &entrypoint, (const void*)0x42);

    //Assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_048: [JavaModuleLoader_BuildModuleConfiguration shall return NULL if loader->options is NULL.]*/
TEST_FUNCTION(JavaModuleLoader_BuildModuleConfiguration_returns_NULL_when_loader_options_is_NULL)
{
    //Arrange
    JAVA_LOADER_ENTRYPOINT entrypoint =
    {
        NULL,
        (STRING_HANDLE)0x42
    };

    JAVA_LOADER_CONFIGURATION config =
    {
        NULL,
        NULL
    };

    MODULE_LOADER loader =
    {
        JAVA,
        NULL,
        (MODULE_LOADER_BASE_CONFIGURATION*)(&config),
        NULL
    };

    //Act
    void* result = JavaModuleLoader_BuildModuleConfiguration(&loader, &entrypoint, (const void*)0x42);

    //Assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_049: [JavaModuleLoader_BuildModuleConfiguration shall return NULL if an underlying platform call fails.]*/
TEST_FUNCTION(JavaModuleLoader_BuildModuleConfiguration_failure_test)
{
    //Arrange
    JAVA_LOADER_ENTRYPOINT entrypoint =
    {
        NULL,
        (STRING_HANDLE)0x42
    };

    JAVA_LOADER_CONFIGURATION config =
    {
        NULL,
        NULL
    };

    MODULE_LOADER loader =
    {
        JAVA,
        NULL,
        (MODULE_LOADER_BASE_CONFIGURATION*)(&config),
        NULL
    };

    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JAVA_MODULE_HOST_CONFIG)))
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        //arrange
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        //Act
        void* result = JavaModuleLoader_BuildModuleConfiguration(&loader, &entrypoint, "{config}");

        //Assert
        ASSERT_IS_NULL(result);
    }

    umock_c_negative_tests_deinit();
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_050: [JavaModuleLoader_BuildModuleConfiguration shall build a JAVA_MODULE_HOST_CONFIG object by copying information from entrypoint, module_configuration, and loader->options and return a non - NULL pointer.]*/
TEST_FUNCTION(JavaModuleLoader_BuildModuleConfiguration_copys_information_success)
{
    //Arrange
    STRING_HANDLE className = STRING_construct("class.name");
    STRING_HANDLE classPath = STRING_construct("class.path");
    JAVA_LOADER_ENTRYPOINT entrypoint =
    {
        className,
        classPath
    };

    JVM_OPTIONS options =
    {
        NULL,
        NULL,
        0,
        false,
        0,
        false,
        NULL
    };

    JAVA_LOADER_CONFIGURATION config =
    {
        NULL,
        &options
    };

    MODULE_LOADER loader =
    {
        JAVA,
        NULL,
        (MODULE_LOADER_BASE_CONFIGURATION*)(&config),
        NULL
    };

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JAVA_MODULE_HOST_CONFIG)));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    //Act
    void* result = JavaModuleLoader_BuildModuleConfiguration(&loader, &entrypoint, "{config}");

    //Assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(((JAVA_MODULE_HOST_CONFIG*)result)->options == config.options);

    //Cleanup
    JavaModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, result);
    STRING_delete(className);
    STRING_delete(classPath);
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_051: [JavaModuleLoader_FreeModuleConfiguration shall do nothing if module_configuration is NULL.]*/
TEST_FUNCTION(JavaModuleLoader_FreeModuleConfiguration_does_nothing_if_configuration_is_NULL)
{
    //Act
    JavaModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, NULL);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_052: [JavaModuleLoader_FreeModuleConfiguration shall free the JAVA_MODULE_HOST_CONFIG object.]*/
TEST_FUNCTION(JavaModuleLoader_FreeModuleConfiguration_frees_object)
{
    //Arrange
    STRING_HANDLE className = STRING_construct("class.name");
    STRING_HANDLE classPath = STRING_construct("class.path");
    JAVA_LOADER_ENTRYPOINT entrypoint =
    {
        className,
        classPath
    };

    JVM_OPTIONS options =
    {
        NULL,
        NULL,
        0,
        false,
        0,
        false,
        NULL
    };

    JAVA_LOADER_CONFIGURATION config =
    {
        NULL,
        &options
    };

    MODULE_LOADER loader =
    {
        JAVA,
        NULL,
        (MODULE_LOADER_BASE_CONFIGURATION*)(&config),
        NULL
    };

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JAVA_MODULE_HOST_CONFIG)));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    void* result = JavaModuleLoader_BuildModuleConfiguration(&loader, &entrypoint, "{config}");

    umock_c_reset_all_calls();

    JAVA_MODULE_HOST_CONFIG* casted_result = (JAVA_MODULE_HOST_CONFIG*)result;

    STRICT_EXPECTED_CALL(gballoc_free((char*)casted_result->class_name));
    STRICT_EXPECTED_CALL(gballoc_free((char*)casted_result->configuration_json));
    STRICT_EXPECTED_CALL(gballoc_free(casted_result));

    //Act
    JavaModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, result);
    
    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    STRING_delete(className);
    STRING_delete(classPath);
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_053: [JavaLoader_Get shall return a non - NULL pointer to a MODULE_LOADER struct.]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_054: [MODULE_LOADER::type shall by JAVA.]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_055: [MODULE_LOADER::name shall be the string java.]*/
/*Tests_SRS_JAVA_MODULE_LOADER_14_056: [JavaLoader_Get shall set the loader->configuration to a default JAVA_LOADER_CONFIGURATION by setting it to default values.]*/
TEST_FUNCTION(JavaLoader_Get_success)
{
    //Arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JVM_OPTIONS)));
    //START: set_default_paths
    //START: set_path
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    //END: set_path
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    //START: set_path
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    //END: set_path
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    //END: set_default_paths
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JAVA_LOADER_CONFIGURATION)));

    //Act
    MODULE_LOADER* loader = (MODULE_LOADER*)JavaLoader_Get();

    //Assert

    JVM_OPTIONS* options = ((JAVA_LOADER_CONFIGURATION*)(loader->configuration))->options;

    ASSERT_IS_NOT_NULL(loader);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(loader->type == JAVA);
    ASSERT_ARE_EQUAL(char_ptr, "java", loader->name);
#ifdef _WIN64
    ASSERT_ARE_EQUAL(char_ptr, "C:\\Program Files\\azure_iot_gateway_sdk-1.0.0\\lib\\modules", options->library_path);
    ASSERT_ARE_EQUAL(char_ptr, "C:\\Program Files\\azure_iot_gateway_sdk-1.0.0\\lib\\bindings\\java\\classes", options->class_path);
#else
#ifdef WIN32
    ASSERT_ARE_EQUAL(char_ptr, "C:\\Program Files (x86)\\azure_iot_gateway_sdk-1.0.0\\lib\\modules", options->library_path);
    ASSERT_ARE_EQUAL(char_ptr, "C:\\Program Files (x86)\\azure_iot_gateway_sdk-1.0.0\\lib\\bindings\\java\\classes", options->class_path);
#else
    ASSERT_ARE_EQUAL(char_ptr, "/usr/local/lib/azure_iot_gateway_sdk-1.0.0/modules", options->library_path);
    ASSERT_ARE_EQUAL(char_ptr, "/usr/local/lib/azure_iot_gateway_sdk-1.0.0/bindings/java/classes", options->class_path);
#endif
#endif
    ASSERT_ARE_EQUAL(int, 0, options->version);
    ASSERT_IS_TRUE(options->debug == false);
    ASSERT_ARE_EQUAL(int, 0, options->debug_port);
    ASSERT_IS_TRUE(options->verbose == false);

    //Cleanup
    loader->api->FreeConfiguration(IGNORED_PTR_ARG, loader->configuration);
}

/*Tests_SRS_JAVA_MODULE_LOADER_14_057: [JavaLoader_Get shall return NULL if any underlying call fails while attempting to set the default JAVA_LOADER_CONFIGURATION.]*/
TEST_FUNCTION(JavaLoader_Get_failure_tests)
{
    //Arrange
    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JVM_OPTIONS)))
        .SetFailReturn(NULL);
    //START: set_default_paths
    //START: set_path
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    //END: set_path
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1);
    //START: set_path
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    //END: set_path
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1);
    //END: set_default_paths
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JAVA_LOADER_CONFIGURATION)))
        .SetFailReturn(NULL);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (i != 4 && i != 9)
        {
            //arrange
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //Act
            MODULE_LOADER* loader = (MODULE_LOADER*)JavaLoader_Get();

            //Assert
            ASSERT_IS_NULL(loader);
        }
    }

    umock_c_negative_tests_deinit();
}

END_TEST_SUITE(JavaLoader_UnitTests)