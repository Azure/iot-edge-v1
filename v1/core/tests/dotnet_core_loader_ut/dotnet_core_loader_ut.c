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

#define GATEWAY_EXPORT_H
#define GATEWAY_EXPORT

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

#include "module_loaders/dotnet_core_loader.h"

static pfModuleLoader_Load DotnetCoreModuleLoader_Load = NULL;
static pfModuleLoader_Unload DotnetCoreModuleLoader_Unload = NULL;
static pfModuleLoader_GetApi DotnetCoreModuleLoader_GetModuleApi = NULL;
static pfModuleLoader_ParseEntrypointFromJson DotnetCoreModuleLoader_ParseEntrypointFromJson = NULL;
static pfModuleLoader_FreeEntrypoint DotnetCoreModuleLoader_FreeEntrypoint = NULL;
static pfModuleLoader_ParseConfigurationFromJson DotnetCoreModuleLoader_ParseConfigurationFromJson = NULL;
static pfModuleLoader_FreeConfiguration DotnetCoreModuleLoader_FreeConfiguration = NULL;
static pfModuleLoader_BuildModuleConfiguration DotnetCoreModuleLoader_BuildModuleConfiguration = NULL;
static pfModuleLoader_FreeModuleConfiguration DotnetCoreModuleLoader_FreeModuleConfiguration = NULL;

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

static TEST_MUTEX_HANDLE g_dllByDll;
static TEST_MUTEX_HANDLE g_testByTest;
static const MODULE_LOADER* g_loader;
static MODULE_LOADER_BASE_CONFIGURATION* g_config;

void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    (void)error_code;
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
    if (arr != NULL)
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

TEST_DEFINE_ENUM_TYPE(MODULE_LOADER_TYPE, MODULE_LOADER_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(MODULE_LOADER_TYPE, MODULE_LOADER_TYPE_VALUES);

BEGIN_TEST_SUITE(DotnetCoreLoader_UnitTests)

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

    g_loader = DotnetCoreLoader_Get();
    g_config = g_loader->configuration;
    DotnetCoreModuleLoader_Load = g_loader->api->Load;
    DotnetCoreModuleLoader_Unload = g_loader->api->Unload;
    DotnetCoreModuleLoader_GetModuleApi = g_loader->api->GetApi;
    DotnetCoreModuleLoader_ParseEntrypointFromJson = g_loader->api->ParseEntrypointFromJson;
    DotnetCoreModuleLoader_FreeEntrypoint = g_loader->api->FreeEntrypoint;
    DotnetCoreModuleLoader_ParseConfigurationFromJson = g_loader->api->ParseConfigurationFromJson;
    DotnetCoreModuleLoader_FreeConfiguration = g_loader->api->FreeConfiguration;
    DotnetCoreModuleLoader_BuildModuleConfiguration = g_loader->api->BuildModuleConfiguration;
    DotnetCoreModuleLoader_FreeModuleConfiguration = g_loader->api->FreeModuleConfiguration;
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
    malloc_fail_count = 0;
    malloc_count = 0;
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}


// Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_001: [ DotnetCoreModuleLoader_Load shall return NULL if loader is NULL. ]
TEST_FUNCTION(DotnetCoreModuleLoader_Load_returns_NULL_when_loader_is_NULL)
{
    // act
    MODULE_LIBRARY_HANDLE result = DotnetCoreModuleLoader_Load(NULL, (const void*)0x42);

    // assert
    ASSERT_IS_NULL(result);
} 

// Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_030: [ DotnetCoreModuleLoader_Load shall return NULL if entrypoint is NULL. ]
TEST_FUNCTION(DotnetCoreModuleLoader_Load_returns_NULL_when_entrypoint_is_NULL)
{
    // act
    MODULE_LIBRARY_HANDLE result = DotnetCoreModuleLoader_Load((const MODULE_LOADER*)0x42, NULL);

    // assert
    ASSERT_IS_NULL(result);
}

// Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_001: [ DotnetCoreModuleLoader_Load shall return NULL if loader is NULL. ]
// Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_030: [ DotnetCoreModuleLoader_Load shall return NULL if entrypoint is NULL. ]
TEST_FUNCTION(DotnetCoreModuleLoader_Load_returns_NULL_when_entrypoint_and_loader_is_NULL)
{
    // act
    MODULE_LIBRARY_HANDLE result = DotnetCoreModuleLoader_Load(NULL, NULL);

    // assert
    ASSERT_IS_NULL(result);
}


// Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_002: [ DotnetCoreModuleLoader_Load shall return NULL if loader->type is not DOTNETCORE. ]
TEST_FUNCTION(DotnetCoreModuleLoader_Load_returns_NULL_when_loader_type_is_not_DOTNET)
{
    // arrange
    MODULE_LOADER loader =
    {
        NATIVE,
        NULL, NULL, NULL
    };

    // act
    MODULE_LIBRARY_HANDLE result = DotnetCoreModuleLoader_Load(&loader, NULL);

    // assert
    ASSERT_IS_NULL(result);
}

// Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_031: [ DotnetCoreModuleLoader_Load shall return NULL if entrypoint->dotnetCoreModuleEntryClass is NULL. ]
TEST_FUNCTION(DotnetCoreModuleLoader_Load_returns_NULL_when_entrypoint_dotnetCoreModuleEntryClass_is_null)
{
    // arrange
    MODULE_LOADER loader =
    {
        DOTNETCORE,
        NULL, 
        NULL, 
        NULL
    };

    DOTNET_CORE_LOADER_ENTRYPOINT entrypoint =
    {
        (STRING_HANDLE)0x42, //ModulePath
        NULL //EntryClass
    };

    // act
    MODULE_LIBRARY_HANDLE result = DotnetCoreModuleLoader_Load(&loader, (void*)&entrypoint);

    // assert
    ASSERT_IS_NULL(result);
}

//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_032: [ DotnetCoreModuleLoader_Load shall return NULL if entrypoint->dotnetCoreModulePath is NULL. ]
TEST_FUNCTION(DotnetCoreModuleLoader_Load_returns_NULL_when_entrypoint_dotnetCoreModulePath_is_null)
{
    // arrange
    MODULE_LOADER loader =
    {
        DOTNETCORE,
        NULL,
        NULL,
        NULL
    };

    DOTNET_CORE_LOADER_ENTRYPOINT entrypoint =
    {
        NULL, //ModulePath
        (STRING_HANDLE)0x42 //EntryClass
    };

    // act
    MODULE_LIBRARY_HANDLE result = DotnetCoreModuleLoader_Load(&loader, (void*)&entrypoint);

    // assert
    ASSERT_IS_NULL(result);
}


//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_003: [ DotnetCoreModuleLoader_Load shall return NULL if an underlying platform call fails. ]
TEST_FUNCTION(DotnetCoreModuleLoader_Load_returns_NULL_when_things_fail)
{
    // arrange
    MODULE_LOADER loader =
    {
        DOTNETCORE,
        NULL, NULL, NULL
    };

    DOTNET_CORE_LOADER_ENTRYPOINT entrypoint =
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
        MODULE_LIBRARY_HANDLE result = DotnetCoreModuleLoader_Load(&loader, (void*)&entrypoint);

        // assert
        ASSERT_IS_NULL(result);
    }

    umock_c_negative_tests_deinit();
}

//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_033: [ DotnetCoreModuleLoader_Load shall use the the binding module path given in loader->configuration->binding_path if loader->configuration is not NULL. Otherwise it shall use the default binding path name. ]
//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_034: [ DotnetCoreModuleLoader_Load shall verify that the library has an exported function called Module_GetApi ]
//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_035: [ DotnetCoreModuleLoader_Load shall verify if api version is lower or equal than MODULE_API_VERSION_1 and if MODULE_CREATE, MODULE_DESTROY and MODULE_RECEIVE are implemented, otherwise return NULL ]
//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_005: [ DotnetCoreModuleLoader_Load shall return a non-NULL pointer of type MODULE_LIBRARY_HANDLE when successful. ]
TEST_FUNCTION(DotnetCoreModuleLoader_Load_succeed)
{
    // arrange
    MODULE_LOADER loader =
    {
        DOTNETCORE,
        NULL,
        NULL,
        NULL
    };

    DOTNET_CORE_LOADER_ENTRYPOINT entrypoint =
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
    MODULE_LIBRARY_HANDLE result = DotnetCoreModuleLoader_Load(&loader, (void*)&entrypoint);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    DotnetCoreModuleLoader_Unload(IGNORED_PTR_ARG, result);
}


//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_003: [ DotnetCoreModuleLoader_Load shall return NULL if an underlying platform call fails. ]
//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_034: [ DotnetCoreModuleLoader_Load shall verify that the library has an exported function called Module_GetApi ]
TEST_FUNCTION(DotnetCoreModuleLoader_Load_returns_NULL_when_GetAPI_returns_NULL)
{
    // arrange
    MODULE_LOADER loader =
    {
        DOTNETCORE,
        NULL, NULL, NULL
    };

    DOTNET_CORE_LOADER_ENTRYPOINT entrypoint =
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
    MODULE_LIBRARY_HANDLE result = DotnetCoreModuleLoader_Load(&loader, (void*)&entrypoint);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_003: [ DotnetCoreModuleLoader_Load shall return NULL if an underlying platform call fails. ]
//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_035: [ DotnetCoreModuleLoader_Load shall verify if api version is lower or equal than MODULE_API_VERSION_1 and if MODULE_CREATE, MODULE_DESTROY and MODULE_RECEIVE are implemented, otherwise return NULL ]
TEST_FUNCTION(DotnetCoreModuleLoader_Load_returns_NULL_when_GetAPI_returns_API_with_unsupported_version)
{
    // arrange
    MODULE_LOADER loader =
    {
        DOTNETCORE,
        NULL, NULL, NULL
    };

    DOTNET_CORE_LOADER_ENTRYPOINT entrypoint =
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
    MODULE_LIBRARY_HANDLE result = DotnetCoreModuleLoader_Load(&loader, (void*)&entrypoint);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_003: [ DotnetCoreModuleLoader_Load shall return NULL if an underlying platform call fails. ]
//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_035: [ DotnetCoreModuleLoader_Load shall verify if api version is lower or equal than MODULE_API_VERSION_1 and if MODULE_CREATE, MODULE_DESTROY and MODULE_RECEIVE are implemented, otherwise return NULL ]
TEST_FUNCTION(DotnetCoreModuleLoader_Load_returns_NULL_when_GetAPI_returns_API_with_invalid_callbacks)
{
    // arrange
    MODULE_LOADER loader =
    {
        DOTNETCORE,
        NULL, NULL, NULL
    };

    DOTNET_CORE_LOADER_ENTRYPOINT entrypoint =
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
        MODULE_LIBRARY_HANDLE result = DotnetCoreModuleLoader_Load(&loader, (void*)&entrypoint);

        // assert
        ASSERT_IS_NULL(result);
    }

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_033: [ DotnetCoreModuleLoader_Load shall use the the binding module path given in loader->configuration->binding_path if loader->configuration is not NULL. Otherwise it shall use the default binding path name. ]
TEST_FUNCTION(DotnetCoreModuleLoader_Load_succeeds_with_custom_binding_path)
{
    // arrange
    MODULE_LOADER_BASE_CONFIGURATION config =
    {
        (STRING_HANDLE)0x42
    };
    DOTNET_CORE_LOADER_ENTRYPOINT entrypoint =
    {
        (STRING_HANDLE)0x42, //ModulePath
        (STRING_HANDLE)0x42 //EntryClass
    };
    MODULE_LOADER loader =
    {
        DOTNETCORE,
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
        .SetReturn(DOTNET_CORE_BINDING_MODULE_NAME);
    STRICT_EXPECTED_CALL(DynamicLibrary_LoadLibrary(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DynamicLibrary_FindSymbol(IGNORED_PTR_ARG, MODULE_GETAPI_NAME))
        .IgnoreArgument(1)
        .SetReturn((void*)Fake_GetAPI);
    STRICT_EXPECTED_CALL(Fake_GetAPI((MODULE_API_VERSION)IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .SetReturn((MODULE_API*)&api);

    // act
    MODULE_LIBRARY_HANDLE result = DotnetCoreModuleLoader_Load(&loader, (void*)&entrypoint);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    DotnetCoreModuleLoader_Unload(IGNORED_PTR_ARG, result);
}



//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_006: [ DotnetCoreModuleLoader_GetModuleApi shall return NULL if moduleLibraryHandle is NULL. ]
TEST_FUNCTION(DotnetCoreModuleLoader_GetModuleApi_returns_NULL_when_moduleLibraryHandle_is_NULL)
{
    // act
    const MODULE_API* result = DotnetCoreModuleLoader_GetModuleApi(IGNORED_PTR_ARG, NULL);

    // assert
    ASSERT_IS_NULL(result);
}


//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_007: [ DotnetCoreModuleLoader_GetModuleApi shall return a non-NULL MODULE_API pointer when successful. ]
TEST_FUNCTION(DotnetCoreModuleLoader_GetModuleApi_succeeds)
{
    // arrange
    MODULE_LOADER loader =
    {
        DOTNETCORE,
        NULL, NULL, NULL
    };
    DOTNET_CORE_LOADER_ENTRYPOINT entrypoint =
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

    MODULE_LIBRARY_HANDLE module = DotnetCoreModuleLoader_Load(&loader, (void*)&entrypoint);
    ASSERT_IS_NOT_NULL(module);

    umock_c_reset_all_calls();

    // act
    const MODULE_API* result = DotnetCoreModuleLoader_GetModuleApi(IGNORED_PTR_ARG, module);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    DotnetCoreModuleLoader_Unload(IGNORED_PTR_ARG, module);
}


//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_008: [ DotnetCoreModuleLoader_Unload shall do nothing if moduleLibraryHandle is NULL. ]
TEST_FUNCTION(DotnetCoreModuleLoader_Unload_does_nothing_when_moduleLibraryHandle_is_NULL)
{
    // act
    DotnetCoreModuleLoader_Unload(IGNORED_PTR_ARG, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_009: [ DotnetCoreModuleLoader_Unload shall unload the binding module from memory by calling DynamicLibrary_UnloadLibrary. ]
//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_010: [ DotnetCoreModuleLoader_Unload shall free resources allocated when loading the binding module. ]
TEST_FUNCTION(DotnetCoreModuleLoader_Unload_frees_things)
{
    // arrange
    MODULE_LOADER loader =
    {
        DOTNETCORE,
        NULL, NULL, NULL
    };

    DOTNET_CORE_LOADER_ENTRYPOINT entrypoint =
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

    MODULE_LIBRARY_HANDLE module = DotnetCoreModuleLoader_Load(&loader, (void*)&entrypoint);
    ASSERT_IS_NOT_NULL(module);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(DynamicLibrary_UnloadLibrary((DYNAMIC_LIBRARY_HANDLE)0x42));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    DotnetCoreModuleLoader_Unload(IGNORED_PTR_ARG, module);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_011: [ DotnetCoreModuleLoader_ParseEntrypointFromJson shall return NULL if json is NULL. ]
TEST_FUNCTION(DotnetCoreModuleLoader_ParseEntrypointFromJson_returns_NULL_when_json_is_NULL)
{
    // act
    void* result = DotnetCoreModuleLoader_ParseEntrypointFromJson(IGNORED_PTR_ARG, NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_012: [ DotnetCoreModuleLoader_ParseEntrypointFromJson shall return NULL if the root json entity is not an object. ]
TEST_FUNCTION(DotnetCoreModuleLoader_ParseEntrypointFromJson_returns_NULL_when_json_is_not_an_object)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONArray);

    // act
    void* result = DotnetCoreModuleLoader_ParseEntrypointFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_013: [ DotnetCoreModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
TEST_FUNCTION(DotnetCoreModuleLoader_ParseEntrypointFromJson_returns_NULL_when_json_value_get_object_returns_NULL)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn(NULL);

    // act
    void* result = DotnetCoreModuleLoader_ParseEntrypointFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_014: [ DotnetCoreModuleLoader_ParseEntrypointFromJson shall retrieve the assemblyName by reading the value of the attribute assembly.name. ]
//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_013: [ DotnetCoreModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
TEST_FUNCTION(DotnetCoreModuleLoader_ParseEntrypointFromJson_returns_NULL_when_json_object_get_string_returns_NULL)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x43);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, "assembly.name"))
        .SetReturn(NULL);

    // act
    void* result = DotnetCoreModuleLoader_ParseEntrypointFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_013: [ DotnetCoreModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
TEST_FUNCTION(DotnetCoreModuleLoader_ParseEntrypointFromJson_returns_NULL_when_malloc_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x43);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, "assembly.name"))
        .SetReturn("foo");
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(DOTNET_CORE_LOADER_ENTRYPOINT)))
        .SetReturn(NULL);

    // act
    void* result = DotnetCoreModuleLoader_ParseEntrypointFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_036: [ DotnetCoreModuleLoader_ParseEntrypointFromJson shall retrieve the entryType by reading the value of the attribute entry.type. ]
//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_013: [ DotnetCoreModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
TEST_FUNCTION(DotnetCoreModuleLoader_ParseEntrypointFromJson_returns_NULL_when_json_object_get_string_for_module_entry_class_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x43);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, "assembly.name"))
        .SetReturn("foo");
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(DOTNET_CORE_LOADER_ENTRYPOINT)));
    STRICT_EXPECTED_CALL(STRING_construct("foo"));
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, "entry.type"))
        .SetReturn(NULL);

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);


    // act
    void* result = DotnetCoreModuleLoader_ParseEntrypointFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}



//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_013: [ DotnetCoreModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
TEST_FUNCTION(DotnetCoreModuleLoader_ParseEntrypointFromJson_returns_NULL_when_STRING_construct_for_assembly_name_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x43);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, "assembly.name"))
        .SetReturn("foo.js");
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(DOTNET_CORE_LOADER_ENTRYPOINT)));
    STRICT_EXPECTED_CALL(STRING_construct("foo.js"))
        .SetReturn(NULL);

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    void* result = DotnetCoreModuleLoader_ParseEntrypointFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_036: [ DotnetCoreModuleLoader_ParseEntrypointFromJson shall retrieve the entryType by reading the value of the attribute entry.type. ]
//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_013: [ DotnetCoreModuleLoader_ParseEntrypointFromJson shall return NULL if an underlying platform call fails. ]
TEST_FUNCTION(DotnetCoreModuleLoader_ParseEntrypointFromJson_returns_NULL_when_STRING_construct_for_module_entry_class_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x43);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, "assembly.name"))
        .SetReturn("foo.js");
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(DOTNET_CORE_LOADER_ENTRYPOINT)));
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
    void* result = DotnetCoreModuleLoader_ParseEntrypointFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_036: [ DotnetCoreModuleLoader_ParseEntrypointFromJson shall retrieve the entryType by reading the value of the attribute entry.type. ]
//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_014: [ DotnetCoreModuleLoader_ParseEntrypointFromJson shall retrieve the assemblyName by reading the value of the attribute assembly.name. ]
//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_015: [ DotnetCoreModuleLoader_ParseEntrypointFromJson shall return a non-NULL pointer to the parsed representation of the entrypoint when successful. ]
TEST_FUNCTION(DotnetCoreModuleLoader_ParseEntrypointFromJson_succeeds)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x43);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, "assembly.name"))
        .SetReturn("foo.js");
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(DOTNET_CORE_LOADER_ENTRYPOINT)));
    STRICT_EXPECTED_CALL(STRING_construct("foo.js"));
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, "entry.type"))
        .SetReturn("foo.js");
    STRICT_EXPECTED_CALL(STRING_construct("foo.js"));

    // act
    void* result = DotnetCoreModuleLoader_ParseEntrypointFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    DotnetCoreModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, result);
}



//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_016: [ DotnetCoreModuleLoader_FreeEntrypoint shall do nothing if entrypoint is NULL. ]
TEST_FUNCTION(DotnetCoreModuleLoader_FreeEntrypoint_does_nothing_when_entrypoint_is_NULL)
{
    // act
    DotnetCoreModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_017: [ DotnetCoreModuleLoader_FreeEntrypoint shall free resources allocated during DotnetCoreModuleLoader_ParseEntrypointFromJson. ]
TEST_FUNCTION(DotnetCoreModuleLoader_FreeEntrypoint_frees_resources)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn((JSON_Object*)0x43);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, "assembly.name"))
        .SetReturn("foo.js");
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(DOTNET_CORE_LOADER_ENTRYPOINT)));
    STRICT_EXPECTED_CALL(STRING_construct("foo.js"));
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x43, "entry.type"))
        .SetReturn("foo.js");
    STRICT_EXPECTED_CALL(STRING_construct("foo.js"));

    void* entrypoint = DotnetCoreModuleLoader_ParseEntrypointFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);
    ASSERT_IS_NOT_NULL(entrypoint);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    DotnetCoreModuleLoader_FreeEntrypoint(IGNORED_PTR_ARG, entrypoint);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}



//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_018: [ DotnetCoreModuleLoader_ParseConfigurationFromJson shall return NULL if an underlying platform call fails. ]
TEST_FUNCTION(DotnetCoreModuleLoader_ParseConfigurationFromJson_returns_NULL_when_malloc_for_DOTNET_CORE_LOADER_CONFIGURATION_fails)
{
    // arrange
    malloc_will_fail = true;
    malloc_fail_count = 1;
    
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42));

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(DOTNET_CORE_LOADER_CONFIGURATION)));

    // act
    MODULE_LOADER_BASE_CONFIGURATION* result = DotnetCoreModuleLoader_ParseConfigurationFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_018: [ DotnetCoreModuleLoader_ParseConfigurationFromJson shall return NULL if an underlying platform call fails. ]
TEST_FUNCTION(DotnetCoreModuleLoader_ParseConfigurationFromJson_returns_NULL_when_malloc_for_DOTNET_CORE_CLR_OPTIONS_fails)
{
    // arrange
    malloc_will_fail = true;
    malloc_fail_count = 2;

    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42));
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(DOTNET_CORE_LOADER_CONFIGURATION)));

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(DOTNET_CORE_CLR_OPTIONS)));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    MODULE_LOADER_BASE_CONFIGURATION* result = DotnetCoreModuleLoader_ParseConfigurationFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_018: [ DotnetCoreModuleLoader_ParseConfigurationFromJson shall return NULL if an underlying platform call fails. ]
//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_019: [ DotnetCoreModuleLoader_ParseConfigurationFromJson shall call ModuleLoader_ParseBaseConfigurationFromJson to parse the loader configuration and return the result. ]
TEST_FUNCTION(DotnetCoreModuleLoader_ParseConfigurationFromJson_returns_NULL_when_ModuleLoader_ParseBaseConfigurationFromJson_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42));
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(DOTNET_CORE_LOADER_CONFIGURATION)));
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(DOTNET_CORE_CLR_OPTIONS)));

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, DOTNET_CORE_CLR_PATH_DEFAULT))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, DOTNET_CORE_TRUSTED_PLATFORM_ASSEMBLIES_LOCATION_DEFAULT))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, DOTNET_CORE_CLR_PATH_KEY))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, DOTNET_CORE_TRUSTED_PLATFORM_ASSEMBLIES_LOCATION_KEY))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(ModuleLoader_ParseBaseConfigurationFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42))
        .IgnoreArgument(1)
        .SetReturn(MODULE_LOADER_ERROR);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    MODULE_LOADER_BASE_CONFIGURATION* result = DotnetCoreModuleLoader_ParseConfigurationFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_019: [ DotnetCoreModuleLoader_ParseConfigurationFromJson shall call ModuleLoader_ParseBaseConfigurationFromJson to parse the loader configuration and return the result. ]
/* Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_040: [ DotnetCoreModuleLoader_ParseConfigurationFromJson shall set default paths on clrOptions. ] */
/* Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_041: [ DotnetCoreModuleLoader_ParseConfigurationFromJson shall check if JSON contains DOTNET_CORE_CLR_PATH_KEY, and if it has it shall change the value of clrOptions->coreClrPath. ] */
/* Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_042: [ DotnetCoreModuleLoader_ParseConfigurationFromJson shall check if JSON contains DOTNET_CORE_TRUSTED_PLATFORM_ASSEMBLIES_LOCATION_KEY, and if it has it shall change the value of clrOptions->trustedPlatformAssembliesLocation . ] */
TEST_FUNCTION(DotnetCoreModuleLoader_ParseConfigurationFromJson_succeeds)
{
    // arrange
    
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42));

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(DOTNET_CORE_LOADER_CONFIGURATION)));

    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(DOTNET_CORE_CLR_OPTIONS)));

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, DOTNET_CORE_CLR_PATH_DEFAULT))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, DOTNET_CORE_TRUSTED_PLATFORM_ASSEMBLIES_LOCATION_DEFAULT))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, DOTNET_CORE_CLR_PATH_KEY))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, DOTNET_CORE_TRUSTED_PLATFORM_ASSEMBLIES_LOCATION_KEY))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(ModuleLoader_ParseBaseConfigurationFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42))
        .IgnoreArgument(1)
        .SetReturn(MODULE_LOADER_SUCCESS);

    // act
    MODULE_LOADER_BASE_CONFIGURATION* result = DotnetCoreModuleLoader_ParseConfigurationFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    DotnetCoreModuleLoader_FreeConfiguration(IGNORED_PTR_ARG, result);
}

//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_020: [ DotnetCoreModuleLoader_FreeConfiguration shall do nothing if configuration is NULL. ]
TEST_FUNCTION(DotnetCoreModuleLoader_FreeConfiguration_does_nothing_when_configuration_is_NULL)
{
    // act
    DotnetCoreModuleLoader_FreeConfiguration(IGNORED_PTR_ARG, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_021: [ DotnetCoreModuleLoader_FreeConfiguration shall call ModuleLoader_FreeBaseConfiguration to free resources allocated by ModuleLoader_ParseBaseConfigurationFromJson. ]
//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_022: [ DotnetCoreModuleLoader_FreeConfiguration shall free resources allocated by DotnetCoreModuleLoader_ParseConfigurationFromJson. ]
TEST_FUNCTION(DotnetCoreModuleLoader_FreeConfiguration_frees_resources)
{
    // arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(MODULE_LOADER_BASE_CONFIGURATION)));
    STRICT_EXPECTED_CALL(ModuleLoader_ParseBaseConfigurationFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42))
        .IgnoreArgument(1)
        .SetReturn(MODULE_LOADER_SUCCESS);

    MODULE_LOADER_BASE_CONFIGURATION* config = DotnetCoreModuleLoader_ParseConfigurationFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(ModuleLoader_FreeBaseConfiguration(config));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();

    STRICT_EXPECTED_CALL(gballoc_free(config));

    // act
    DotnetCoreModuleLoader_FreeConfiguration(IGNORED_PTR_ARG, config);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}



//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_023: [ DotnetCoreModuleLoader_BuildModuleConfiguration shall return NULL if entrypoint is NULL. ]
TEST_FUNCTION(DotnetCoreModuleLoader_BuildModuleConfiguration_returns_NULL_when_entrypoint_is_NULL)
{
    // act
    void* result = DotnetCoreModuleLoader_BuildModuleConfiguration(NULL, NULL, NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_024: [ DotnetCoreModuleLoader_BuildModuleConfiguration shall return NULL if entrypoint->dotnetCoreModulePath is NULL. ]
TEST_FUNCTION(DotnetCoreModuleLoader_BuildModuleConfiguration_returns_NULL_when_dotnetCoreModulePath_is_NULL)
{
    // arrange
    DOTNET_CORE_LOADER_ENTRYPOINT entrypoint = { 0, (STRING_HANDLE)0x42};

    // act
    void* result = DotnetCoreModuleLoader_BuildModuleConfiguration(NULL, &entrypoint, NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_037: [ DotnetCoreModuleLoader_BuildModuleConfiguration shall return NULL if entrypoint->dotnetCoreModuleEntryClass is NULL. ]
TEST_FUNCTION(DotnetCoreModuleLoader_BuildModuleConfiguration_returns_NULL_when_dotnetCoreModuleEntryClass_is_NULL)
{
    // arrange
    DOTNET_CORE_LOADER_ENTRYPOINT entrypoint = { (STRING_HANDLE)0x42, 0 };

    // act
    void* result = DotnetCoreModuleLoader_BuildModuleConfiguration(g_loader, &entrypoint, NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_025: [ DotnetCoreModuleLoader_BuildModuleConfiguration shall return NULL if an underlying platform call fails. ]
//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_037: [ DotnetCoreModuleLoader_BuildModuleConfiguration shall return NULL if entrypoint->dotnetCoreModuleEntryClass is NULL. ]
TEST_FUNCTION(DotnetCoreModuleLoader_BuildModuleConfiguration_returns_NULL_when_things_fail)
{
    // arrange
    DOTNET_CORE_LOADER_ENTRYPOINT entrypoint = { STRING_construct("foo"), STRING_construct("foo2") };
    char* module_config = "boo";
    umock_c_reset_all_calls();

    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(entrypoint.dotnetCoreModuleEntryClass))
        .SetReturn("foo2");
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "foo2"))
        .IgnoreArgument_destination();
    STRICT_EXPECTED_CALL(STRING_c_str(entrypoint.dotnetCoreModulePath))
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
            void* result = DotnetCoreModuleLoader_BuildModuleConfiguration(NULL, &entrypoint, module_config);

            // assert
            ASSERT_IS_NULL(result);
        }
    }

    // cleanup
    umock_c_negative_tests_deinit();
    STRING_delete(entrypoint.dotnetCoreModulePath);
    STRING_delete(entrypoint.dotnetCoreModuleEntryClass);
}

//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_026: [ DotnetCoreModuleLoader_BuildModuleConfiguration shall build a DOTNET_HOST_CONFIG object by copying information from entrypoint and module_configuration and return a non-NULL pointer. ]
TEST_FUNCTION(DotnetCoreModuleLoader_BuildModuleConfiguration_succeeds)
{
    // arrange
    DOTNET_CORE_LOADER_ENTRYPOINT entrypoint = { (STRING_HANDLE)STRING_construct("foo"), (STRING_HANDLE)STRING_construct("foo2") };

    char* module_config = "boo";
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(entrypoint.dotnetCoreModuleEntryClass))
        .SetReturn("foo2");
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "foo2"))
        .IgnoreArgument_destination();
    STRICT_EXPECTED_CALL(STRING_c_str(entrypoint.dotnetCoreModulePath))
        .SetReturn("foo");
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "foo"))
        .IgnoreArgument_destination();
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "boo"))
        .IgnoreArgument_destination();




    ///act
    void* result = DotnetCoreModuleLoader_BuildModuleConfiguration(g_loader, &entrypoint, module_config);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    STRING_delete(entrypoint.dotnetCoreModulePath);
    STRING_delete(entrypoint.dotnetCoreModuleEntryClass);
    DotnetCoreModuleLoader_FreeModuleConfiguration(g_loader, result);
}


//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_027: [ DotnetCoreModuleLoader_FreeModuleConfiguration shall do nothing if module_configuration is NULL. ]
TEST_FUNCTION(DotnetCoreModuleLoader_FreeModuleConfiguration_does_nothing_when_module_configuration_is_NULL)
{
    // act
    DotnetCoreModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_028: [ DotnetCoreModuleLoader_FreeModuleConfiguration shall free the DOTNET_HOST_CONFIG object. ]
TEST_FUNCTION(DotnetCoreModuleLoader_FreeModuleConfiguration_frees_resources)
{
    // arrange
    DOTNET_CORE_LOADER_ENTRYPOINT entrypoint = { (STRING_HANDLE)STRING_construct("foo"), (STRING_HANDLE)STRING_construct("foo2") };
    STRING_HANDLE module_config = STRING_construct("boo");
    umock_c_reset_all_calls();

    void* result = DotnetCoreModuleLoader_BuildModuleConfiguration(g_loader, &entrypoint, module_config);


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
    DotnetCoreModuleLoader_FreeModuleConfiguration(IGNORED_PTR_ARG, result);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    STRING_delete(entrypoint.dotnetCoreModuleEntryClass);
    STRING_delete(entrypoint.dotnetCoreModulePath);
    STRING_delete(module_config);
}


//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_029: [ DotnetCoreLoader_Get shall return a non-NULL pointer to a MODULE_LOADER struct. ]
//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_038: [ MODULE_LOADER::type shall be DOTNETCORE. ]
//Tests_SRS_DOTNET_CORE_MODULE_LOADER_04_039: [ MODULE_LOADER::name shall be the string 'DOTNETCORE'. ]
TEST_FUNCTION(DotnetCoreLoader_Get_succeeds)
{
    // act
    const MODULE_LOADER* loader = DotnetCoreLoader_Get(); 

    // assert
    ASSERT_IS_NOT_NULL(loader);
    ASSERT_ARE_EQUAL(MODULE_LOADER_TYPE, DOTNETCORE, DOTNETCORE);
    ASSERT_IS_TRUE(strcmp(loader->name, "dotnetcore") == 0);
}

END_TEST_SUITE(DotnetCoreLoader_UnitTests);
