// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

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


#include "parson.h"
#include "dynamic_library.h"
#include "module_loader.h"


//=============================================================================
//HOOKS
//=============================================================================

#ifdef __cplusplus
extern "C"
{
#endif
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


#undef ENABLE_MOCKS

#include "module_loaders/dotnet_loader.h"

static pfModuleLoader_Load DotnetModuleLoader_Load = NULL;
static pfModuleLoader_Unload DotnetModuleLoader_Unload = NULL;
static pfModuleLoader_GetApi DotnetModuleLoader_GetModuleApi = NULL;
static pfModuleLoader_ParseEntrypointFromJson DotnetModuleLoader_ParseEntrypointFromJson = NULL;
static pfModuleLoader_FreeEntrypoint DotnetModuleLoader_FreeEntrypoint = NULL;
static pfModuleLoader_ParseConfigurationFromJson DotnetModuleLoader_ParseConfigurationFromJson = NULL;
static pfModuleLoader_FreeConfiguration DotnetModuleLoader_FreeConfiguration = NULL;
static pfModuleLoader_BuildModuleConfiguration DotnetModuleLoader_BuildModuleConfiguration = NULL;
static pfModuleLoader_FreeModuleConfiguration DotnetModuleLoader_FreeModuleConfiguration = NULL;

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

//=============================================================================
//Globals
//=============================================================================

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error");
}

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
    size_t num = -1;
    if (arr != NULL)
    {
        num = 1;
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


#undef ENABLE_MOCKS

TEST_DEFINE_ENUM_TYPE(MODULE_LOADER_RESULT, MODULE_LOADER_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(MODULE_LOADER_RESULT, MODULE_LOADER_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(MODULE_LOADER_TYPE, MODULE_LOADER_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(MODULE_LOADER_TYPE, MODULE_LOADER_TYPE_VALUES);

BEGIN_TEST_SUITE(DotnetLoader_UnitTests)

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
    REGISTER_UMOCK_ALIAS_TYPE(DYNAMIC_LIBRARY_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MODULE_LIBRARY_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(JSON_Value_Type, int);
    REGISTER_UMOCK_ALIAS_TYPE(MODULE_API_VERSION, int);
    REGISTER_GLOBAL_MOCK_RETURN(DynamicLibrary_LoadLibrary, (DYNAMIC_LIBRARY_HANDLE)0x42);
    REGISTER_GLOBAL_MOCK_RETURN(DynamicLibrary_FindSymbol, (void*)0x42);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_object, NULL);

    // malloc/free hooks
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    // Strings hooks
    REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, real_STRING_construct);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_clone, real_STRING_clone);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, real_STRING_delete);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_c_str, real_STRING_c_str);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, -1);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_clone, NULL);

    //mallocAndStrcpy_s Hook
    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, real_mallocAndStrcpy_s);

    const MODULE_LOADER* loader = DotnetLoader_Get();
    DotnetModuleLoader_Load = loader->api->Load;
    DotnetModuleLoader_Unload = loader->api->Unload;
    DotnetModuleLoader_GetModuleApi = loader->api->GetApi;
    DotnetModuleLoader_ParseEntrypointFromJson = loader->api->ParseEntrypointFromJson;
    DotnetModuleLoader_FreeEntrypoint = loader->api->FreeEntrypoint;
    DotnetModuleLoader_ParseConfigurationFromJson = loader->api->ParseConfigurationFromJson;
    DotnetModuleLoader_FreeConfiguration = loader->api->FreeConfiguration;
    DotnetModuleLoader_BuildModuleConfiguration = loader->api->BuildModuleConfiguration;
    DotnetModuleLoader_FreeModuleConfiguration = loader->api->FreeModuleConfiguration;
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


// Tests_SRS_DOTNET_MODULE_LOADER_04_001: [ DotnetModuleLoader_Load shall return NULL if loader is NULL. ]
TEST_FUNCTION(DotnetModuleLoader_Load_returns_NULL_when_loader_is_NULL)
{
    // act
    MODULE_LIBRARY_HANDLE result = DotnetModuleLoader_Load(NULL, (const void*)0x42);

    // assert
    ASSERT_IS_NULL(result);
} 

// Tests_SRS_DOTNET_MODULE_LOADER_04_030: [ DotnetModuleLoader_Load shall return NULL if entrypoint is NULL. ]
TEST_FUNCTION(DotnetModuleLoader_Load_returns_NULL_when_entrypoint_is_NULL)
{
    // act
    MODULE_LIBRARY_HANDLE result = DotnetModuleLoader_Load((const MODULE_LOADER*)0x42, NULL);

    // assert
    ASSERT_IS_NULL(result);
}

// Tests_SRS_DOTNET_MODULE_LOADER_04_001: [ DotnetModuleLoader_Load shall return NULL if loader is NULL. ]
// Tests_SRS_DOTNET_MODULE_LOADER_04_030: [ DotnetModuleLoader_Load shall return NULL if entrypoint is NULL. ]
TEST_FUNCTION(DotnetModuleLoader_Load_returns_NULL_when_entrypoint_and_loader_is_NULL)
{
    // act
    MODULE_LIBRARY_HANDLE result = DotnetModuleLoader_Load(NULL, NULL);

    // assert
    ASSERT_IS_NULL(result);
}


// Tests_SRS_DOTNET_MODULE_LOADER_04_002: [ DotnetModuleLoader_Load shall return NULL if loader->type is not DOTNET. ]
TEST_FUNCTION(DotnetModuleLoader_Load_returns_NULL_when_loader_type_is_not_DOTNET)
{
    // arrange
    MODULE_LOADER loader =
    {
        NATIVE,
        NULL, NULL, NULL
    };

    // act
    MODULE_LIBRARY_HANDLE result = DotnetModuleLoader_Load(&loader, NULL);

    // assert
    ASSERT_IS_NULL(result);
}

// Tests_SRS_DOTNET_MODULE_LOADER_04_031: [ DotnetModuleLoader_Load shall return NULL if entrypoint->dotnetModuleEntryClass is NULL. ]
TEST_FUNCTION(DotnetModuleLoader_Load_returns_NULL_when_entrypoint_dotnetModuleEntryClass_is_null)
{
    // arrange
    MODULE_LOADER loader =
    {
        DOTNET,
        NULL, 
        NULL, 
        NULL
    };

    DOTNET_LOADER_ENTRYPOINT entrypoint =
    {
        (STRING_HANDLE)0x42, //ModulePath
        NULL //EntryClass
    };

    // act
    MODULE_LIBRARY_HANDLE result = DotnetModuleLoader_Load(&loader, (void*)&entrypoint);

    // assert
    ASSERT_IS_NULL(result);
}

//Tests_SRS_DOTNET_MODULE_LOADER_04_032: [ DotnetModuleLoader_Load shall return NULL if entrypoint->dotnetModulePath is NULL. ]
TEST_FUNCTION(DotnetModuleLoader_Load_returns_NULL_when_entrypoint_dotnetModulePath_is_null)
{
    // arrange
    MODULE_LOADER loader =
    {
        DOTNET,
        NULL,
        NULL,
        NULL
    };

    DOTNET_LOADER_ENTRYPOINT entrypoint =
    {
        NULL, //ModulePath
        (STRING_HANDLE)0x42 //EntryClass
    };

    // act
    MODULE_LIBRARY_HANDLE result = DotnetModuleLoader_Load(&loader, (void*)&entrypoint);

    // assert
    ASSERT_IS_NULL(result);
}


//Tests_SRS_DOTNET_MODULE_LOADER_04_003: [ DotnetModuleLoader_Load shall return NULL if an underlying platform call fails. ]
TEST_FUNCTION(DotnetModuleLoader_Load_returns_NULL_when_things_fail)
{
    // arrange
    MODULE_LOADER loader =
    {
        DOTNET,
        NULL, NULL, NULL
    };

    DOTNET_LOADER_ENTRYPOINT entrypoint =
    {
        (STRING_HANDLE)0x42, //ModulePath
        (STRING_HANDLE)0x42 //EntryClass
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
        // arrange
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        MODULE_LIBRARY_HANDLE result = DotnetModuleLoader_Load(&loader, (void*)&entrypoint);

        // assert
        ASSERT_IS_NULL(result);
    }

    umock_c_negative_tests_deinit();
}

//Tests_SRS_DOTNET_MODULE_LOADER_04_033: [ DotnetModuleLoader_Load shall use the the binding module path given in loader->configuration->binding_path if loader->configuration is not NULL. Otherwise it shall use the default binding path name. ]
//Tests_SRS_DOTNET_MODULE_LOADER_04_034: [ DotnetModuleLoader_Load shall verify that the library has an exported function called Module_GetApi ]
//Tests_SRS_DOTNET_MODULE_LOADER_04_035: [ DotnetModuleLoader_Load shall verify if api version is lower or equal than MODULE_API_VERSION_1 and if MODULE_CREATE, MODULE_DESTROY and MODULE_RECEIVE are implemented, otherwise return NULL ]
//Tests_SRS_DOTNET_MODULE_LOADER_04_005: [ DotnetModuleLoader_Load shall return a non-NULL pointer of type MODULE_LIBRARY_HANDLE when successful. ]
TEST_FUNCTION(DotnetModuleLoader_Load_succeed)
{
    // arrange
    MODULE_LOADER loader =
    {
        DOTNET,
        NULL,
        NULL,
        NULL
    };

    DOTNET_LOADER_ENTRYPOINT entrypoint =
    {
        (STRING_HANDLE)0x42, //ModulePath
        (STRING_HANDLE)0x42 //EntryClass
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

    // act
    MODULE_LIBRARY_HANDLE result = DotnetModuleLoader_Load(&loader, (void*)&entrypoint);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    DotnetModuleLoader_Unload(IGNORED_PTR_ARG, result);
}


//Tests_SRS_DOTNET_MODULE_LOADER_04_003: [ DotnetModuleLoader_Load shall return NULL if an underlying platform call fails. ]
//Tests_SRS_DOTNET_MODULE_LOADER_04_034: [ DotnetModuleLoader_Load shall verify that the library has an exported function called Module_GetApi ]
TEST_FUNCTION(DotnetModuleLoader_Load_returns_NULL_when_GetAPI_returns_NULL)
{
    // arrange
    MODULE_LOADER loader =
    {
        DOTNET,
        NULL, NULL, NULL
    };

    DOTNET_LOADER_ENTRYPOINT entrypoint =
    {
        (STRING_HANDLE)0x42, //ModulePath
        (STRING_HANDLE)0x42 //EntryClass
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
    MODULE_LIBRARY_HANDLE result = DotnetModuleLoader_Load(&loader, (void*)&entrypoint);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


//Tests_SRS_DOTNET_MODULE_LOADER_04_003: [ DotnetModuleLoader_Load shall return NULL if an underlying platform call fails. ]
//Tests_SRS_DOTNET_MODULE_LOADER_04_035: [ DotnetModuleLoader_Load shall verify if api version is lower or equal than MODULE_API_VERSION_1 and if MODULE_CREATE, MODULE_DESTROY and MODULE_RECEIVE are implemented, otherwise return NULL ]
TEST_FUNCTION(DotnetModuleLoader_Load_returns_NULL_when_GetAPI_returns_API_with_unsupported_version)
{
    // arrange
    MODULE_LOADER loader =
    {
        DOTNET,
        NULL, NULL, NULL
    };

    DOTNET_LOADER_ENTRYPOINT entrypoint =
    {
        (STRING_HANDLE)0x42, //ModulePath
        (STRING_HANDLE)0x42 //EntryClass
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
    MODULE_LIBRARY_HANDLE result = DotnetModuleLoader_Load(&loader, (void*)&entrypoint);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


//Tests_SRS_DOTNET_MODULE_LOADER_04_003: [ DotnetModuleLoader_Load shall return NULL if an underlying platform call fails. ]
//Tests_SRS_DOTNET_MODULE_LOADER_04_035: [ DotnetModuleLoader_Load shall verify if api version is lower or equal than MODULE_API_VERSION_1 and if MODULE_CREATE, MODULE_DESTROY and MODULE_RECEIVE are implemented, otherwise return NULL ]
TEST_FUNCTION(DotnetModuleLoader_Load_returns_NULL_when_GetAPI_returns_API_with_invalid_callbacks)
{
    // arrange
    MODULE_LOADER loader =
    {
        DOTNET,
        NULL, NULL, NULL
    };

    DOTNET_LOADER_ENTRYPOINT entrypoint =
    {
        (STRING_HANDLE)0x42, //ModulePath
        (STRING_HANDLE)0x42 //EntryClass
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
        MODULE_LIBRARY_HANDLE result = DotnetModuleLoader_Load(&loader, (void*)&entrypoint);

        // assert
        ASSERT_IS_NULL(result);
    }

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


//Tests_SRS_DOTNET_MODULE_LOADER_04_033: [ DotnetModuleLoader_Load shall use the the binding module path given in loader->configuration->binding_path if loader->configuration is not NULL. Otherwise it shall use the default binding path name. ]
TEST_FUNCTION(DotnetModuleLoader_Load_succeeds_with_custom_binding_path)
{
    // arrange
    MODULE_LOADER_BASE_CONFIGURATION config =
    {
        (STRING_HANDLE)0x42
    };
    DOTNET_LOADER_ENTRYPOINT entrypoint =
    {
        (STRING_HANDLE)0x42, //ModulePath
        (STRING_HANDLE)0x42 //EntryClass
    };
    MODULE_LOADER loader =
    {
        DOTNET,
        NULL,
        &config,
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
    STRICT_EXPECTED_CALL(STRING_c_str((STRING_HANDLE)0x42))
        .SetReturn(DOTNET_BINDING_MODULE_NAME);
    STRICT_EXPECTED_CALL(DynamicLibrary_LoadLibrary(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DynamicLibrary_FindSymbol(IGNORED_PTR_ARG, MODULE_GETAPI_NAME))
        .IgnoreArgument(1)
        .SetReturn((void*)Fake_GetAPI);
    STRICT_EXPECTED_CALL(Fake_GetAPI((MODULE_API_VERSION)IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .SetReturn((MODULE_API*)&api);

    // act
    MODULE_LIBRARY_HANDLE result = DotnetModuleLoader_Load(&loader, (void*)&entrypoint);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    DotnetModuleLoader_Unload(IGNORED_PTR_ARG, result);
}



//Tests_SRS_DOTNET_MODULE_LOADER_04_006: [ DotnetModuleLoader_GetModuleApi shall return NULL if moduleLibraryHandle is NULL. ]
TEST_FUNCTION(DotnetModuleLoader_GetModuleApi_returns_NULL_when_moduleLibraryHandle_is_NULL)
{
    // act
    const MODULE_API* result = DotnetModuleLoader_GetModuleApi(IGNORED_PTR_ARG, NULL);

    // assert
    ASSERT_IS_NULL(result);
}


//Tests_SRS_DOTNET_MODULE_LOADER_04_007: [ DotnetModuleLoader_GetModuleApi shall return a non-NULL MODULE_API pointer when successful. ]
TEST_FUNCTION(DotnetModuleLoader_GetModuleApi_succeeds)
{
    // arrange
    MODULE_LOADER loader =
    {
        DOTNET,
        NULL, NULL, NULL
    };
    DOTNET_LOADER_ENTRYPOINT entrypoint =
    {
        (STRING_HANDLE)0x42, //ModulePath
        (STRING_HANDLE)0x42 //EntryClass
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

    MODULE_LIBRARY_HANDLE module = DotnetModuleLoader_Load(&loader, (void*)&entrypoint);
    ASSERT_IS_NOT_NULL(module);

    umock_c_reset_all_calls();

    // act
    const MODULE_API* result = DotnetModuleLoader_GetModuleApi(IGNORED_PTR_ARG, module);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    DotnetModuleLoader_Unload(IGNORED_PTR_ARG, module);
}


//Tests_SRS_DOTNET_MODULE_LOADER_04_008: [ DotnetModuleLoader_Unload shall do nothing if moduleLibraryHandle is NULL. ]
TEST_FUNCTION(DotnetModuleLoader_Unload_does_nothing_when_moduleLibraryHandle_is_NULL)
{
    // act
    DotnetModuleLoader_Unload(IGNORED_PTR_ARG, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


//Tests_SRS_DOTNET_MODULE_LOADER_04_009: [ DotnetModuleLoader_Unload shall unload the binding module from memory by calling DynamicLibrary_UnloadLibrary. ]
//Tests_SRS_DOTNET_MODULE_LOADER_04_010: [ DotnetModuleLoader_Unload shall free resources allocated when loading the binding module. ]
TEST_FUNCTION(DotnetModuleLoader_Unload_frees_things)
{
    // arrange
    MODULE_LOADER loader =
    {
        DOTNET,
        NULL, NULL, NULL
    };

    DOTNET_LOADER_ENTRYPOINT entrypoint =
    {
        (STRING_HANDLE)0x42, //ModulePath
        (STRING_HANDLE)0x42 //EntryClass
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

    MODULE_LIBRARY_HANDLE module = DotnetModuleLoader_Load(&loader, (void*)&entrypoint);
    ASSERT_IS_NOT_NULL(module);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(DynamicLibrary_UnloadLibrary((DYNAMIC_LIBRARY_HANDLE)0x42));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    DotnetModuleLoader_Unload(IGNORED_PTR_ARG, module);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


//Tests_SRS_DOTNET_MODULE_LOADER_04_011: [ DotnetModuleLoader_ParseEntrypointFromJson shall return NULL if json is NULL. ]
TEST_FUNCTION(DotnetModuleLoader_ParseEntrypointFromJson_returns_NULL_when_json_is_NULL)
{
    // act
    void* result = DotnetModuleLoader_ParseEntrypointFromJson(IGNORED_PTR_ARG, NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//Tests_SRS_DOTNET_MODULE_LOADER_04_012: [ DotnetModuleLoader_ParseEntrypointFromJson shall return NULL if the root json entity is not an object. ]
TEST_FUNCTION(DotnetModuleLoader_ParseEntrypointFromJson_returns_NULL_when_json_is_not_an_object)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONArray);

    // act
    void* result = DotnetModuleLoader_ParseEntrypointFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


//Tests_SRS_DOTNET_MODULE_LOADER_04_013: [ DotnetModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
TEST_FUNCTION(DotnetModuleLoader_ParseEntrypointFromJson_returns_NULL_when_json_value_get_object_returns_NULL)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn(NULL);

    // act
    void* result = DotnetModuleLoader_ParseEntrypointFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


//Tests_SRS_DOTNET_MODULE_LOADER_04_014: [ DotnetModuleLoader_ParseEntrypointFromJson shall retrieve the assembly_name by reading the value of the attribute assembly.name. ]
//Tests_SRS_DOTNET_MODULE_LOADER_04_013: [ DotnetModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
TEST_FUNCTION(DotnetModuleLoader_ParseEntrypointFromJson_returns_NULL_when_json_object_get_string_returns_NULL)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x43);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, "assembly.name"))
        .SetReturn(NULL);

    // act
    void* result = DotnetModuleLoader_ParseEntrypointFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//Tests_SRS_DOTNET_MODULE_LOADER_04_013: [ DotnetModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
TEST_FUNCTION(DotnetModuleLoader_ParseEntrypointFromJson_returns_NULL_when_malloc_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x43);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, "assembly.name"))
        .SetReturn("foo");
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(DOTNET_LOADER_ENTRYPOINT)))
        .SetReturn(NULL);

    // act
    void* result = DotnetModuleLoader_ParseEntrypointFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//Tests_SRS_DOTNET_MODULE_LOADER_04_036: [ DotnetModuleLoader_ParseEntrypointFromJson shall retrieve the entry_type by reading the value of the attribute entry.type. ]
//Tests_SRS_DOTNET_MODULE_LOADER_04_013: [ DotnetModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
TEST_FUNCTION(DotnetModuleLoader_ParseEntrypointFromJson_returns_NULL_when_json_object_get_string_for_module_entry_class_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x43);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, "assembly.name"))
        .SetReturn("foo");
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(DOTNET_LOADER_ENTRYPOINT)));
    STRICT_EXPECTED_CALL(STRING_construct("foo"));
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, "entry.type"))
        .SetReturn(NULL);

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);


    // act
    void* result = DotnetModuleLoader_ParseEntrypointFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}



//Tests_SRS_DOTNET_MODULE_LOADER_04_013: [ DotnetModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
TEST_FUNCTION(DotnetModuleLoader_ParseEntrypointFromJson_returns_NULL_when_STRING_construct_for_assembly_name_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x43);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, "assembly.name"))
        .SetReturn("foo.js");
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(DOTNET_LOADER_ENTRYPOINT)));
    STRICT_EXPECTED_CALL(STRING_construct("foo.js"))
        .SetReturn(NULL);

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    void* result = DotnetModuleLoader_ParseEntrypointFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//Tests_SRS_DOTNET_MODULE_LOADER_04_036: [ DotnetModuleLoader_ParseEntrypointFromJson shall retrieve the entry_type by reading the value of the attribute entry.type. ]
//Tests_SRS_DOTNET_MODULE_LOADER_04_013: [ DotnetModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
TEST_FUNCTION(DotnetModuleLoader_ParseEntrypointFromJson_returns_NULL_when_STRING_construct_for_module_entry_class_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x43);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, "assembly.name"))
        .SetReturn("foo.js");
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(DOTNET_LOADER_ENTRYPOINT)));
    STRICT_EXPECTED_CALL(STRING_construct("foo.js"));
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, "entry.type"))
        .SetReturn("foo.js");
    STRICT_EXPECTED_CALL(STRING_construct("foo.js"))
        .SetReturn(NULL);

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    void* result = DotnetModuleLoader_ParseEntrypointFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//Tests_SRS_DOTNET_MODULE_LOADER_04_036: [ DotnetModuleLoader_ParseEntrypointFromJson shall retrieve the entry_type by reading the value of the attribute entry.type. ]
//Tests_SRS_DOTNET_MODULE_LOADER_04_014: [ DotnetModuleLoader_ParseEntrypointFromJson shall retrieve the assembly_name by reading the value of the attribute assembly.name. ]
//Tests_SRS_DOTNET_MODULE_LOADER_04_015: [ DotnetModuleLoader_ParseEntrypointFromJson shall return a non-NULL pointer to the parsed representation of the entrypoint when successful. ]
TEST_FUNCTION(DotnetModuleLoader_ParseEntrypointFromJson_succeeds)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x43);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, "assembly.name"))
        .SetReturn("foo.js");
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(DOTNET_LOADER_ENTRYPOINT)));
    STRICT_EXPECTED_CALL(STRING_construct("foo.js"));
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, "entry.type"))
        .SetReturn("foo.js");
    STRICT_EXPECTED_CALL(STRING_construct("foo.js"));

    // act
    void* result = DotnetModuleLoader_ParseEntrypointFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    DotnetModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, result);
}



//Tests_SRS_DOTNET_MODULE_LOADER_04_016: [ DotnetModuleLoader_FreeEntrypoint shall do nothing if entrypoint is NULL. ]
TEST_FUNCTION(DotnetModuleLoader_FreeEntrypoint_does_nothing_when_entrypoint_is_NULL)
{
    // act
    DotnetModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


//Tests_SRS_DOTNET_MODULE_LOADER_04_017: [ DotnetModuleLoader_FreeEntrypoint shall free resources allocated during DotnetModuleLoader_ParseEntrypointFromJson. ]
TEST_FUNCTION(DotnetModuleLoader_FreeEntrypoint_frees_resources)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x43);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, "assembly.name"))
        .SetReturn("foo.js");
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(DOTNET_LOADER_ENTRYPOINT)));
    STRICT_EXPECTED_CALL(STRING_construct("foo.js"));
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, "entry.type"))
        .SetReturn("foo.js");
    STRICT_EXPECTED_CALL(STRING_construct("foo.js"));

    void* entrypoint = DotnetModuleLoader_ParseEntrypointFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);
    ASSERT_IS_NOT_NULL(entrypoint);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    DotnetModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, entrypoint);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}



//Tests_SRS_DOTNET_MODULE_LOADER_04_018: [ DotnetModuleLoader_ParseConfigurationFromJson shall return NULL if an underlying platform call fails. ]
TEST_FUNCTION(DotnetModuleLoader_ParseConfigurationFromJson_returns_NULL_when_malloc_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(MODULE_LOADER_BASE_CONFIGURATION)))
        .SetReturn(NULL);

    // act
    MODULE_LOADER_BASE_CONFIGURATION* result = DotnetModuleLoader_ParseConfigurationFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


//Tests_SRS_DOTNET_MODULE_LOADER_04_018: [ DotnetModuleLoader_ParseConfigurationFromJson shall return NULL if an underlying platform call fails. ]
//Tests_SRS_DOTNET_MODULE_LOADER_04_019: [ DotnetModuleLoader_ParseConfigurationFromJson shall call ModuleLoader_ParseBaseConfigurationFromJson to parse the loader configuration and return the result. ]
TEST_FUNCTION(DotnetModuleLoader_ParseConfigurationFromJson_returns_NULL_when_ModuleLoader_ParseBaseConfigurationFromJson_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(MODULE_LOADER_BASE_CONFIGURATION)));
    STRICT_EXPECTED_CALL(ModuleLoader_ParseBaseConfigurationFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42))
        .IgnoreArgument(1)
        .SetReturn(MODULE_LOADER_ERROR);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    MODULE_LOADER_BASE_CONFIGURATION* result = DotnetModuleLoader_ParseConfigurationFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


//Tests_SRS_DOTNET_MODULE_LOADER_04_019: [ DotnetModuleLoader_ParseConfigurationFromJson shall call ModuleLoader_ParseBaseConfigurationFromJson to parse the loader configuration and return the result. ]
TEST_FUNCTION(DotnetModuleLoader_ParseConfigurationFromJson_succeeds)
{
    // arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(MODULE_LOADER_BASE_CONFIGURATION)));
    STRICT_EXPECTED_CALL(ModuleLoader_ParseBaseConfigurationFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42))
        .IgnoreArgument(1)
        .SetReturn(MODULE_LOADER_SUCCESS);

    // act
    MODULE_LOADER_BASE_CONFIGURATION* result = DotnetModuleLoader_ParseConfigurationFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    DotnetModuleLoader_FreeConfiguration(IGNORED_PTR_ARG, result);
}

//Tests_SRS_DOTNET_MODULE_LOADER_04_020: [ DotnetModuleLoader_FreeConfiguration shall do nothing if configuration is NULL. ]
TEST_FUNCTION(DotnetModuleLoader_FreeConfiguration_does_nothing_when_configuration_is_NULL)
{
    // act
    DotnetModuleLoader_FreeConfiguration(IGNORED_PTR_ARG, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//Tests_SRS_DOTNET_MODULE_LOADER_04_021: [ DotnetModuleLoader_FreeConfiguration shall call ModuleLoader_FreeBaseConfiguration to free resources allocated by ModuleLoader_ParseBaseConfigurationFromJson. ]
//Tests_SRS_DOTNET_MODULE_LOADER_04_022: [ DotnetModuleLoader_FreeConfiguration shall free resources allocated by DotnetModuleLoader_ParseConfigurationFromJson. ]
TEST_FUNCTION(DotnetModuleLoader_FreeConfiguration_frees_resources)
{
    // arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(MODULE_LOADER_BASE_CONFIGURATION)));
    STRICT_EXPECTED_CALL(ModuleLoader_ParseBaseConfigurationFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42))
        .IgnoreArgument(1)
        .SetReturn(MODULE_LOADER_SUCCESS);

    MODULE_LOADER_BASE_CONFIGURATION* config = DotnetModuleLoader_ParseConfigurationFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(ModuleLoader_FreeBaseConfiguration(config));
    STRICT_EXPECTED_CALL(gballoc_free(config));

    // act
    DotnetModuleLoader_FreeConfiguration(IGNORED_PTR_ARG, config);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}



//Tests_SRS_DOTNET_MODULE_LOADER_04_023: [ DotnetModuleLoader_BuildModuleConfiguration shall return NULL if entrypoint is NULL. ]
TEST_FUNCTION(DotnetModuleLoader_BuildModuleConfiguration_returns_NULL_when_entrypoint_is_NULL)
{
    // act
    void* result = DotnetModuleLoader_BuildModuleConfiguration(NULL, NULL, NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


//Tests_SRS_DOTNET_MODULE_LOADER_04_024: [ DotnetModuleLoader_BuildModuleConfiguration shall return NULL if entrypoint->dotnetModulePath is NULL. ]
TEST_FUNCTION(DotnetModuleLoader_BuildModuleConfiguration_returns_NULL_when_dotnetModulePath_is_NULL)
{
    // arrange
    DOTNET_LOADER_ENTRYPOINT entrypoint = { 0, (STRING_HANDLE)0x42};

    // act
    void* result = DotnetModuleLoader_BuildModuleConfiguration(NULL, &entrypoint, NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//Tests_SRS_DOTNET_MODULE_LOADER_04_037: [ DotnetModuleLoader_BuildModuleConfiguration shall return NULL if entrypoint->dotnetModuleEntryClass is NULL. ]
TEST_FUNCTION(DotnetModuleLoader_BuildModuleConfiguration_returns_NULL_when_dotnetModuleEntryClass_is_NULL)
{
    // arrange
    DOTNET_LOADER_ENTRYPOINT entrypoint = { (STRING_HANDLE)0x42, 0 };

    // act
    void* result = DotnetModuleLoader_BuildModuleConfiguration(NULL, &entrypoint, NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//Tests_SRS_DOTNET_MODULE_LOADER_04_025: [ DotnetModuleLoader_BuildModuleConfiguration shall return NULL if an underlying platform call fails. ]
//Tests_SRS_DOTNET_MODULE_LOADER_04_037: [ DotnetModuleLoader_BuildModuleConfiguration shall return NULL if entrypoint->dotnetModuleEntryClass is NULL. ]
TEST_FUNCTION(DotnetModuleLoader_BuildModuleConfiguration_returns_NULL_when_things_fail)
{
    // arrange
    DOTNET_LOADER_ENTRYPOINT entrypoint = { STRING_construct("foo"), STRING_construct("foo2") };
    char* module_config = "boo";
    umock_c_reset_all_calls();

    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(entrypoint.dotnetModuleEntryClass))
        .SetReturn("foo2");
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "foo2"))
        .IgnoreArgument_destination();
    STRICT_EXPECTED_CALL(STRING_c_str(entrypoint.dotnetModulePath))
        .SetReturn("foo");
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "foo"))
        .IgnoreArgument_destination();
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "boo"))
        .IgnoreArgument_destination();

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (
            (i != 1) /*STRING_c_str*/ &&
            (i != 3) /*STRING_c_str*/
            )
        {

            // arrange
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            void* result = DotnetModuleLoader_BuildModuleConfiguration(NULL, &entrypoint, module_config);

            // assert
            ASSERT_IS_NULL(result);
        }
    }

    // cleanup
    umock_c_negative_tests_deinit();
    STRING_delete(entrypoint.dotnetModulePath);
    STRING_delete(entrypoint.dotnetModuleEntryClass);
}

//Tests_SRS_DOTNET_MODULE_LOADER_04_026: [ DotnetModuleLoader_BuildModuleConfiguration shall build a DOTNET_HOST_CONFIG object by copying information from entrypoint and module_configuration and return a non-NULL pointer. ]
TEST_FUNCTION(DotnetModuleLoader_BuildModuleConfiguration_succeeds)
{
    // arrange
    DOTNET_LOADER_ENTRYPOINT entrypoint = { (STRING_HANDLE)STRING_construct("foo"), (STRING_HANDLE)STRING_construct("foo2") };
    char* module_config = "boo";
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(entrypoint.dotnetModuleEntryClass))
        .SetReturn("foo2");
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "foo2"))
        .IgnoreArgument_destination();
    STRICT_EXPECTED_CALL(STRING_c_str(entrypoint.dotnetModulePath))
        .SetReturn("foo");
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "foo"))
        .IgnoreArgument_destination();
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "boo"))
        .IgnoreArgument_destination();




    ///act
    void* result = DotnetModuleLoader_BuildModuleConfiguration(NULL, &entrypoint, module_config);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    STRING_delete(entrypoint.dotnetModulePath);
    STRING_delete(entrypoint.dotnetModuleEntryClass);
    DotnetModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, result);
}


//Tests_SRS_DOTNET_MODULE_LOADER_04_027: [ DotnetModuleLoader_FreeModuleConfiguration shall do nothing if module_configuration is NULL. ]
TEST_FUNCTION(DotnetModuleLoader_FreeModuleConfiguration_does_nothing_when_module_configuration_is_NULL)
{
    // act
    DotnetModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


//Tests_SRS_DOTNET_MODULE_LOADER_04_028: [ DotnetModuleLoader_FreeModuleConfiguration shall free the DOTNET_HOST_CONFIG object. ]
TEST_FUNCTION(DotnetModuleLoader_FreeModuleConfiguration_frees_resources)
{
    // arrange
    DOTNET_LOADER_ENTRYPOINT entrypoint = { (STRING_HANDLE)STRING_construct("foo"), (STRING_HANDLE)STRING_construct("foo2") };
    STRING_HANDLE module_config = STRING_construct("boo");
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(entrypoint.dotnetModuleEntryClass))
        .SetReturn("foo2");
    
    STRICT_EXPECTED_CALL(STRING_c_str(entrypoint.dotnetModulePath))
        .SetReturn("foo");

    void* result = DotnetModuleLoader_BuildModuleConfiguration(NULL, &entrypoint, module_config);

    ASSERT_IS_NOT_NULL(result);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    DotnetModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, result);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    STRING_delete(entrypoint.dotnetModuleEntryClass);
    STRING_delete(entrypoint.dotnetModulePath);
    STRING_delete(module_config);
}


//Tests_SRS_DOTNET_MODULE_LOADER_04_029: [ DotnetLoader_Get shall return a non-NULL pointer to a MODULE_LOADER struct. ]
//Tests_SRS_DOTNET_MODULE_LOADER_04_038: [ MODULE_LOADER::type shall be DOTNET. ]
//Tests_SRS_DOTNET_MODULE_LOADER_04_039: [ MODULE_LOADER::name shall be the string 'dotnet'. ]
TEST_FUNCTION(DotnetLoader_Get_succeeds)
{
    // act
    const MODULE_LOADER* loader = DotnetLoader_Get(); 

    // assert
    ASSERT_IS_NOT_NULL(loader);
    ASSERT_ARE_EQUAL(MODULE_LOADER_TYPE, DOTNET, DOTNET);
    ASSERT_IS_TRUE(strcmp(loader->name, "dotnet") == 0);
}

END_TEST_SUITE(DotnetLoader_UnitTests);
