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

#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umock_c_negative_tests.h"
#include "umocktypes_charptr.h"
#include "umocktypes_bool.h"
#include "umocktypes_stdint.h"

#define ENABLE_MOCKS

#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/gballoc.h"

#include "parson.h"

#undef ENABLE_MOCKS

#include "module_loader.h"

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

static const size_t g_enabled_loaders[] =
{
    1       // native loader
#ifdef NODE_BINDING_ENABLED
    , 1
#endif
#ifdef JAVA_BINDING_ENABLED
    , 1
#endif
#ifdef DOTNET_BINDING_ENABLED
    , 1
#endif
#ifdef DOTNET_CORE_BINDING_ENABLED
    , 1
#endif
#ifdef OUTPROCESS_ENABLED
	, 1		// outprocess_loader
#endif
};

static const size_t LOADERS_COUNT = sizeof(g_enabled_loaders) / sizeof(g_enabled_loaders[0]);

//=============================================================================
//Globals
//=============================================================================

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error");
}

LOCK_HANDLE my_Lock_Init(void)
{
    return (LOCK_HANDLE)my_gballoc_malloc(1);
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
        my_gballoc_free(handle);
        result = LOCK_OK;
    }
    return result;
}

// Fake module loader
MOCK_FUNCTION_WITH_CODE(, MODULE_LIBRARY_HANDLE, FakeModuleLoader_Load, const MODULE_LOADER*, loader, const void*, entrypoint)
MOCK_FUNCTION_END((MODULE_LIBRARY_HANDLE)my_gballoc_malloc(1))

MOCK_FUNCTION_WITH_CODE(, const MODULE_API*, FakeModuleLoader_GetModuleApi, const MODULE_LOADER*, loader, MODULE_LIBRARY_HANDLE, moduleLibraryHandle)
MOCK_FUNCTION_END((const MODULE_API*)0x42)

MOCK_FUNCTION_WITH_CODE(, void, FakeModuleLoader_Unload, const MODULE_LOADER*, loader, MODULE_LIBRARY_HANDLE, moduleLibraryHandle)
my_gballoc_free(moduleLibraryHandle);
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void*, FakeModuleLoader_ParseEntrypointFromJson, const MODULE_LOADER*, loader, const JSON_Value*, json)
MOCK_FUNCTION_END(my_gballoc_malloc(1))

MOCK_FUNCTION_WITH_CODE(, void, FakeModuleLoader_FreeEntrypoint, const MODULE_LOADER*, loader, void*, entrypoint)
my_gballoc_free(entrypoint);
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, MODULE_LOADER_BASE_CONFIGURATION*, FakeModuleLoader_ParseConfigurationFromJson, const MODULE_LOADER*, loader, const JSON_Value*, json)
MOCK_FUNCTION_END((MODULE_LOADER_BASE_CONFIGURATION*)my_gballoc_malloc(1))

MOCK_FUNCTION_WITH_CODE(, void, FakeModuleLoader_FreeConfiguration, const MODULE_LOADER*, loader, MODULE_LOADER_BASE_CONFIGURATION*, configuration)
my_gballoc_free(configuration);
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void*, FakeModuleLoader_BuildModuleConfiguration,
    const MODULE_LOADER*, loader,
    const void*, entrypoint,
    const void*, module_configuration
)
MOCK_FUNCTION_END(my_gballoc_malloc(1))

MOCK_FUNCTION_WITH_CODE(, void, FakeModuleLoader_FreeModuleConfiguration, const MODULE_LOADER*, loader, const void*, module_configuration)
my_gballoc_free((void*)module_configuration);
MOCK_FUNCTION_END()

// Module loader mocks
static MODULE_LOADER_API Fake_Module_Loader_API =
{
    FakeModuleLoader_Load,
    FakeModuleLoader_Unload,
    FakeModuleLoader_GetModuleApi,

    FakeModuleLoader_ParseEntrypointFromJson,
    FakeModuleLoader_FreeEntrypoint,

    FakeModuleLoader_ParseConfigurationFromJson,
    FakeModuleLoader_FreeConfiguration,

    FakeModuleLoader_BuildModuleConfiguration,
    FakeModuleLoader_FreeModuleConfiguration
};

static MODULE_LOADER Dynamic_Module_Loader =
{
    NATIVE,
    "native",
    NULL,
    &Fake_Module_Loader_API
};

#ifdef __cplusplus
extern "C"
{
#endif
MOCK_FUNCTION_WITH_CODE(, const MODULE_LOADER*, DynamicLoader_Get)
MOCK_FUNCTION_END(&Dynamic_Module_Loader)
#ifdef __cplusplus
}
#endif

static MODULE_LOADER Outprocess_Module_Loader =
{
	OUTPROCESS,
	"outprocess",
	NULL,
	&Fake_Module_Loader_API
};

#ifdef __cplusplus
extern "C"
{
#endif
MOCK_FUNCTION_WITH_CODE(, const MODULE_LOADER*, OutprocessLoader_Get)
MOCK_FUNCTION_END(&Outprocess_Module_Loader)
#ifdef __cplusplus
}
#endif

static MODULE_LOADER Node_Module_Loader =
{
    NODEJS,
    "node",
    NULL,
    &Fake_Module_Loader_API
};

#ifdef __cplusplus
extern "C"
{
#endif
MOCK_FUNCTION_WITH_CODE(, const MODULE_LOADER*, NodeLoader_Get)
MOCK_FUNCTION_END(&Node_Module_Loader)
#ifdef __cplusplus
}
#endif

static MODULE_LOADER Java_Module_Loader =
{
    JAVA,
    "java",
    NULL,
    &Fake_Module_Loader_API
};

#ifdef __cplusplus
extern "C"
{
#endif
MOCK_FUNCTION_WITH_CODE(, const MODULE_LOADER*, JavaLoader_Get)
MOCK_FUNCTION_END(&Java_Module_Loader)
#ifdef __cplusplus
}
#endif

static MODULE_LOADER Dotnet_Module_Loader =
{
    DOTNET,
    "dotnet",
    NULL,
    &Fake_Module_Loader_API
};

static MODULE_LOADER Dotnet_Core_Module_Loader =
{
    DOTNETCORE,
    "dotnetcore",
    NULL,
    &Fake_Module_Loader_API
};

#ifdef __cplusplus
extern "C"
{
#endif
MOCK_FUNCTION_WITH_CODE(, const MODULE_LOADER*, DotnetLoader_Get)
MOCK_FUNCTION_END(&Dotnet_Module_Loader)

MOCK_FUNCTION_WITH_CODE(, const MODULE_LOADER*, DotnetCoreLoader_Get)
MOCK_FUNCTION_END(&Dotnet_Core_Module_Loader)
#ifdef __cplusplus
}
#endif

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

#ifdef __cplusplus
extern "C"
{
#endif

VECTOR_HANDLE real_VECTOR_create(size_t elementSize);
VECTOR_HANDLE real_VECTOR_move(VECTOR_HANDLE handle);
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

STRING_HANDLE real_STRING_construct(const char* psz);
void real_STRING_delete(STRING_HANDLE handle);
const char* real_STRING_c_str(STRING_HANDLE handle);

#ifdef __cplusplus
}
#endif

TEST_DEFINE_ENUM_TYPE(MODULE_LOADER_RESULT, MODULE_LOADER_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(MODULE_LOADER_RESULT, MODULE_LOADER_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(LOCK_RESULT, LOCK_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(LOCK_RESULT, LOCK_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(MODULE_LOADER_TYPE, MODULE_LOADER_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(MODULE_LOADER_TYPE, MODULE_LOADER_TYPE_VALUES);

BEGIN_TEST_SUITE(ModuleLoader_UnitTests)

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
    REGISTER_UMOCK_ALIAS_TYPE(LOCK_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(LOCK_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(VECTOR_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const VECTOR_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PREDICATE_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(JSON_Value_Type, int);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_object, NULL);

    // malloc/free hooks
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    // Lock hooks
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Lock, LOCK_ERROR);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Unlock, LOCK_ERROR);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Lock_Deinit, LOCK_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(Lock_Init, my_Lock_Init);
    REGISTER_GLOBAL_MOCK_HOOK(Lock, my_Lock);
    REGISTER_GLOBAL_MOCK_HOOK(Unlock, my_Unlock);
    REGISTER_GLOBAL_MOCK_HOOK(Lock_Deinit, my_Lock_Deinit);

    // Vector hooks
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_create, real_VECTOR_create);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_move, real_VECTOR_move);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_destroy, real_VECTOR_destroy);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_push_back, real_VECTOR_push_back);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_erase, real_VECTOR_erase);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_clear, real_VECTOR_clear);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_element, real_VECTOR_element);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_front, real_VECTOR_front);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_back, real_VECTOR_back);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_find_if, real_VECTOR_find_if);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_size, real_VECTOR_size);

    // Strings hooks
    REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, real_STRING_construct);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, real_STRING_delete);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_c_str, real_STRING_c_str);
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

// Tests_SRS_MODULE_LOADER_13_002: [ ModuleLoader_Initialize shall return MODULE_LOADER_ERROR if an underlying platform call fails. ]
TEST_FUNCTION(ModuleLoader_Initialize_returns_MODULE_LOADER_ERROR_when_things_fail)
{
    // arrange
    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(Lock_Init())
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(LOCK_ERROR);
    STRICT_EXPECTED_CALL(VECTOR_create(sizeof(MODULE_LOADER*)))
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        // arrange
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        MODULE_LOADER_RESULT result = ModuleLoader_Initialize();

        // assert
        ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
    }

    umock_c_negative_tests_deinit();
}

// Tests_SRS_MODULE_LOADER_13_001: [ ModuleLoader_Initialize shall initialize g_module_loaders.lock. ]
// Tests_SRS_MODULE_LOADER_13_003: [ ModuleLoader_Initialize shall acquire the lock on g_module_loaders.lock. ]
// Tests_SRS_MODULE_LOADER_13_004: [ ModuleLoader_Initialize shall initialize g_module.module_loaders by calling VECTOR_create. ]
// Tests_SRS_MODULE_LOADER_13_005: [ ModuleLoader_Initialize shall add the default support module loaders to g_module.module_loaders. ]
// Tests_SRS_MODULE_LOADER_13_007: [ ModuleLoader_Initialize shall unlock g_module.lock. ]
// Tests_SRS_MODULE_LOADER_13_006: [ ModuleLoader_Initialize shall return MODULE_LOADER_SUCCESS once all the default loaders have been added successfully. ]
TEST_FUNCTION(ModuleLoader_Initialize_succeeds)
{
    // arrange
    STRICT_EXPECTED_CALL(Lock_Init());
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_create(sizeof(MODULE_LOADER*)));
	STRICT_EXPECTED_CALL(DynamicLoader_Get());
#ifdef NODE_BINDING_ENABLED
    STRICT_EXPECTED_CALL(NodeLoader_Get());
#endif
#ifdef JAVA_BINDING_ENABLED
    STRICT_EXPECTED_CALL(JavaLoader_Get());
#endif
#ifdef DOTNET_BINDING_ENABLED
    STRICT_EXPECTED_CALL(DotnetLoader_Get());
#endif
#ifdef DOTNET_CORE_BINDING_ENABLED
    STRICT_EXPECTED_CALL(DotnetCoreLoader_Get());
#endif
#ifdef OUTPROCESS_ENABLED
STRICT_EXPECTED_CALL(OutprocessLoader_Get());
#endif

    for (size_t i = 0; i < LOADERS_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
    }

    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    MODULE_LOADER_RESULT result = ModuleLoader_Initialize();

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_SUCCESS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    ModuleLoader_Destroy();
}

// Tests_SRS_MODULE_LOADER_13_008: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader is NULL. ]
// Tests_SRS_MODULE_LOADER_13_011: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader->api is NULL ]
// Tests_SRS_MODULE_LOADER_13_012: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader->name is NULL ]
// Tests_SRS_MODULE_LOADER_13_013: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader->type is UNKNOWN ]
// Tests_SRS_MODULE_LOADER_13_014: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader->api->BuildModuleConfiguration is NULL ]
// Tests_SRS_MODULE_LOADER_13_015: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader->api->FreeConfiguration is NULL ]
// Tests_SRS_MODULE_LOADER_13_016: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader->api->FreeEntrypoint is NULL ]
// Tests_SRS_MODULE_LOADER_13_017: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader->api->FreeModuleConfiguration is NULL ]
// Tests_SRS_MODULE_LOADER_13_018: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader->api->GetApi is NULL ]
// Tests_SRS_MODULE_LOADER_13_019: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader->api->Load is NULL ]
// Tests_SRS_MODULE_LOADER_13_020: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader->api->ParseConfigurationFromJson is NULL ]
// Tests_SRS_MODULE_LOADER_13_021: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader->api->ParseEntrypointFromJson is NULL ]
// Tests_SRS_MODULE_LOADER_13_022: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if loader->api->Unload is NULL ]
TEST_FUNCTION(ModuleLoader_Add_returns_MODULE_LOADER_ERROR_when_loader_is_invalid)
{
    // arrange
    MODULE_LOADER_API loader_api_inputs[] =
    {
        {
            NULL,
            FakeModuleLoader_Unload,
            FakeModuleLoader_GetModuleApi,

            FakeModuleLoader_ParseEntrypointFromJson,
            FakeModuleLoader_FreeEntrypoint,

            FakeModuleLoader_ParseConfigurationFromJson,
            FakeModuleLoader_FreeConfiguration,

            FakeModuleLoader_BuildModuleConfiguration,
            FakeModuleLoader_FreeModuleConfiguration
        },
        {
            FakeModuleLoader_Load,
            NULL,
            FakeModuleLoader_GetModuleApi,

            FakeModuleLoader_ParseEntrypointFromJson,
            FakeModuleLoader_FreeEntrypoint,

            FakeModuleLoader_ParseConfigurationFromJson,
            FakeModuleLoader_FreeConfiguration,

            FakeModuleLoader_BuildModuleConfiguration,
            FakeModuleLoader_FreeModuleConfiguration
        },
        {
            FakeModuleLoader_Load,
            FakeModuleLoader_Unload,
            NULL,

            FakeModuleLoader_ParseEntrypointFromJson,
            FakeModuleLoader_FreeEntrypoint,

            FakeModuleLoader_ParseConfigurationFromJson,
            FakeModuleLoader_FreeConfiguration,

            FakeModuleLoader_BuildModuleConfiguration,
            FakeModuleLoader_FreeModuleConfiguration
        },
        {
            FakeModuleLoader_Load,
            FakeModuleLoader_Unload,
            FakeModuleLoader_GetModuleApi,

            NULL,
            FakeModuleLoader_FreeEntrypoint,

            FakeModuleLoader_ParseConfigurationFromJson,
            FakeModuleLoader_FreeConfiguration,

            FakeModuleLoader_BuildModuleConfiguration,
            FakeModuleLoader_FreeModuleConfiguration
        },
        {
            FakeModuleLoader_Load,
            FakeModuleLoader_Unload,
            FakeModuleLoader_GetModuleApi,

            FakeModuleLoader_ParseEntrypointFromJson,
            NULL,

            FakeModuleLoader_ParseConfigurationFromJson,
            FakeModuleLoader_FreeConfiguration,

            FakeModuleLoader_BuildModuleConfiguration,
            FakeModuleLoader_FreeModuleConfiguration
        },
        {
            FakeModuleLoader_Load,
            FakeModuleLoader_Unload,
            FakeModuleLoader_GetModuleApi,

            FakeModuleLoader_ParseEntrypointFromJson,
            FakeModuleLoader_FreeEntrypoint,

            NULL,
            FakeModuleLoader_FreeConfiguration,

            FakeModuleLoader_BuildModuleConfiguration,
            FakeModuleLoader_FreeModuleConfiguration
        },
        {
            FakeModuleLoader_Load,
            FakeModuleLoader_Unload,
            FakeModuleLoader_GetModuleApi,

            FakeModuleLoader_ParseEntrypointFromJson,
            FakeModuleLoader_FreeEntrypoint,

            FakeModuleLoader_ParseConfigurationFromJson,
            NULL,

            FakeModuleLoader_BuildModuleConfiguration,
            FakeModuleLoader_FreeModuleConfiguration
        },
        {
            FakeModuleLoader_Load,
            FakeModuleLoader_Unload,
            FakeModuleLoader_GetModuleApi,

            FakeModuleLoader_ParseEntrypointFromJson,
            FakeModuleLoader_FreeEntrypoint,

            FakeModuleLoader_ParseConfigurationFromJson,
            FakeModuleLoader_FreeConfiguration,

            NULL,
            FakeModuleLoader_FreeModuleConfiguration
        },
        {
            FakeModuleLoader_Load,
            FakeModuleLoader_Unload,
            FakeModuleLoader_GetModuleApi,

            FakeModuleLoader_ParseEntrypointFromJson,
            FakeModuleLoader_FreeEntrypoint,

            FakeModuleLoader_ParseConfigurationFromJson,
            FakeModuleLoader_FreeConfiguration,

            FakeModuleLoader_BuildModuleConfiguration,
            NULL
        }
    };
    MODULE_LOADER inputs[] =
    {
        {                               // loader with NULL name
            NATIVE,
            NULL,
            NULL,
            &Fake_Module_Loader_API
        },
        {                               // loader with UNKNOWN type
            UNKNOWN,
            "native",
            NULL,
            &Fake_Module_Loader_API
        },
        {                               // loader with NULL API
            NATIVE,
            "native",
            NULL,
            NULL
        }
    };

    // act
    MODULE_LOADER_RESULT result = ModuleLoader_Add(NULL);
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);

    for (size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++)
    {
        result = ModuleLoader_Add(&(inputs[i]));

        // assert
        ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
    }

    for (size_t i = 0; i < sizeof(loader_api_inputs) / sizeof(loader_api_inputs[0]); i++)
    {
        MODULE_LOADER loader =
        {
            NATIVE,
            "native",
            NULL,
            &(loader_api_inputs[i])
        };
        result = ModuleLoader_Add(&loader);

        // assert
        ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
    }
}

// Tests_SRS_MODULE_LOADER_13_009: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if g_module_loaders.lock is NULL. ]
// Tests_SRS_MODULE_LOADER_13_010: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if g_module_loaders.module_loaders is NULL. ]
TEST_FUNCTION(ModuleLoader_Add_returns_MODULE_LOADER_ERROR_when_not_initialized)
{
    // arrange

    // act
    MODULE_LOADER_RESULT result = ModuleLoader_Add(&Dynamic_Module_Loader);

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
}

// Tests_SRS_MODULE_LOADER_13_023: [ ModuleLoader_Add shall return MODULE_LOADER_ERROR if an underlying platform call fails. ]
TEST_FUNCTION(ModuleLoader_Add_returns_MODULE_LOADER_ERROR_when_things_fail)
{
    // arrange
    MODULE_LOADER_RESULT result = ModuleLoader_Initialize();
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_SUCCESS, result);
    umock_c_reset_all_calls();

    int result2 = 0;
    result2 = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result2);

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(LOCK_ERROR);
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        // arrange
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        result = ModuleLoader_Add(&Dynamic_Module_Loader);

        // assert
        ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    ModuleLoader_Destroy();
}

// Tests_SRS_MODULE_LOADER_13_027: [ ModuleLoader_Add shall lock g_module_loaders.lock. ]
// Tests_SRS_MODULE_LOADER_13_024: [ ModuleLoader_Add shall add the new module to the global module loaders list. ]
// Tests_SRS_MODULE_LOADER_13_025: [ ModuleLoader_Add shall unlock g_module_loaders.lock. ]
// Tests_SRS_MODULE_LOADER_13_026: [ ModuleLoader_Add shall return MODULE_LOADER_SUCCESS if the loader has been added successfully. ]
TEST_FUNCTION(ModuleLoader_Add_succeeds)
{
    // arrange
    MODULE_LOADER_RESULT result = ModuleLoader_Initialize();
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_SUCCESS, result);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    result = ModuleLoader_Add(&Dynamic_Module_Loader);

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_SUCCESS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    ModuleLoader_Destroy();
}

// Tests_SRS_MODULE_LOADER_13_028: [ ModuleLoader_UpdateConfiguration shall return MODULE_LOADER_ERROR if loader is NULL. ]
TEST_FUNCTION(ModuleLoader_UpdateConfiguration_returns_error_when_loader_is_NULL)
{
    // act
    MODULE_LOADER_RESULT result = ModuleLoader_UpdateConfiguration(NULL, NULL);

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
}

// Tests_SRS_MODULE_LOADER_13_029: [ ModuleLoader_UpdateConfiguration shall return MODULE_LOADER_ERROR if g_module_loaders.lock is NULL. ]
// Tests_SRS_MODULE_LOADER_13_030: [ ModuleLoader_UpdateConfiguration shall return MODULE_LOADER_ERROR if g_module_loaders.module_loaders is NULL. ]
TEST_FUNCTION(ModuleLoader_UpdateConfiguration_returns_error_when_not_initialized)
{
    // act
    MODULE_LOADER_RESULT result = ModuleLoader_UpdateConfiguration(&Dynamic_Module_Loader, NULL);

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
}

// Tests_SRS_MODULE_LOADER_13_031: [ ModuleLoader_UpdateConfiguration shall return MODULE_LOADER_ERROR if an underlying platform call fails. ]
TEST_FUNCTION(ModuleLoader_UpdateConfiguration_returns_error_when_things_fail)
{
    // arrange
    MODULE_LOADER_RESULT result = ModuleLoader_Initialize();
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_SUCCESS, result);
    umock_c_reset_all_calls();

    int result2 = 0;
    result2 = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result2);

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(LOCK_ERROR);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        // arrange
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        result = ModuleLoader_UpdateConfiguration(&Dynamic_Module_Loader, (MODULE_LOADER_BASE_CONFIGURATION*)0x42);

        // assert
        ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    ModuleLoader_Destroy();
}

// Tests_SRS_MODULE_LOADER_13_032: [ ModuleLoader_UpdateConfiguration shall lock g_module_loaders.lock ]
// Tests_SRS_MODULE_LOADER_13_033: [ ModuleLoader_UpdateConfiguration shall assign configuration to the module loader. ]
// Tests_SRS_MODULE_LOADER_13_034: [ ModuleLoader_UpdateConfiguration shall unlock g_module_loaders.lock. ]
// Tests_SRS_MODULE_LOADER_13_035: [ ModuleLoader_UpdateConfiguration shall return MODULE_LOADER_SUCCESS if the loader has been updated successfully. ]
// Tests_SRS_MODULE_LOADER_13_074: [ If the existing configuration on the loader is not NULL ModuleLoader_UpdateConfiguration shall call FreeConfiguration on the configuration pointer. ]
TEST_FUNCTION(ModuleLoader_UpdateConfiguration_succeeds)
{
    // arrange
    MODULE_LOADER_RESULT result = ModuleLoader_Initialize();
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_SUCCESS, result);
    umock_c_reset_all_calls();

    MODULE_LOADER loader = Dynamic_Module_Loader;
    loader.configuration = (MODULE_LOADER_BASE_CONFIGURATION*)my_gballoc_malloc(1);

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(FakeModuleLoader_FreeConfiguration(IGNORED_PTR_ARG, loader.configuration))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    result = ModuleLoader_UpdateConfiguration(&loader, (MODULE_LOADER_BASE_CONFIGURATION*)0x42);

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_SUCCESS, result);
    ASSERT_IS_TRUE((MODULE_LOADER_BASE_CONFIGURATION*)0x42 == loader.configuration);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    ModuleLoader_Destroy();
}

// Tests_SRS_MODULE_LOADER_13_037: [ ModuleLoader_FindByName shall return NULL if g_module_loaders.lock is NULL. ]
// Tests_SRS_MODULE_LOADER_13_038: [ ModuleLoader_FindByName shall return NULL if g_module_loaders.module_loaders is NULL.]
TEST_FUNCTION(ModuleLoader_FindByName_returns_NULL_when_not_initialized)
{
    // act
    MODULE_LOADER* result = ModuleLoader_FindByName("native");

    // assert
    ASSERT_IS_NULL(result);
}

// Tests_SRS_MODULE_LOADER_13_036: [ ModuleLoader_FindByName shall return NULL if name is NULL. ]
TEST_FUNCTION(ModuleLoader_FindByName_returns_NULL_when_name_is_NULL)
{
    // act
    MODULE_LOADER* result = ModuleLoader_FindByName(NULL);

    // assert
    ASSERT_IS_NULL(result);
}

// Tests_SRS_MODULE_LOADER_13_039: [ ModuleLoader_FindByName shall return NULL if an underlying platform call fails. ]
TEST_FUNCTION(ModuleLoader_FindByName_returns_NULL_when_things_fail)
{
    // arrange
    MODULE_LOADER_RESULT result = ModuleLoader_Initialize();
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_SUCCESS, result);
    umock_c_reset_all_calls();

    int result2 = 0;
    result2 = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result2);

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(LOCK_ERROR);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        // arrange
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        MODULE_LOADER* result = ModuleLoader_FindByName("native");

        // assert
        ASSERT_IS_NULL(result);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    ModuleLoader_Destroy();
}

// Tests_SRS_MODULE_LOADER_13_042: [ ModuleLoader_FindByName shall return NULL if a matching module loader is not found. ]
TEST_FUNCTION(ModuleLoader_FindByName_returns_NULL_when_loader_cannot_be_found)
{
    // arrange
    MODULE_LOADER_RESULT init_result = ModuleLoader_Initialize();
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_SUCCESS, init_result);
    umock_c_reset_all_calls();

    const char* names[] =
    {
        "no_such_module_loader",    // non-existent loader
        "Native"                    // loader name with different case
    };

    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
    {
        STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, names[i]))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
    }

    // act
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
    {
        MODULE_LOADER* result = ModuleLoader_FindByName(names[i]);

        // assert
        ASSERT_IS_NULL(result);
    }

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    ModuleLoader_Destroy();
}

// Tests_SRS_MODULE_LOADER_13_040: [ ModuleLoader_FindByName shall lock g_module_loaders.lock. ]
// Tests_SRS_MODULE_LOADER_13_041: [ ModuleLoader_FindByName shall search for a module loader whose name is equal to name. The comparison is case sensitive. ]
// Tests_SRS_MODULE_LOADER_13_043: [ ModuleLoader_FindByName shall return a pointer to the MODULE_LOADER if a matching entry is found. ]
// Tests_SRS_MODULE_LOADER_13_044: [ ModuleLoader_FindByName shall unlock g_module_loaders.lock. ]
TEST_FUNCTION(ModuleLoader_FindByName_succeeds)
{
    // arrange
    MODULE_LOADER_RESULT init_result = ModuleLoader_Initialize();
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_SUCCESS, init_result);
    umock_c_reset_all_calls();

    const char* name = "native";

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, name))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    MODULE_LOADER* result = ModuleLoader_FindByName(name);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(MODULE_LOADER_TYPE, result->type, NATIVE);
    ASSERT_IS_TRUE(strcmp(result->name, "native") == 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    ModuleLoader_Destroy();
}

// Tests_SRS_MODULE_LOADER_13_045: [ ModuleLoader_Destroy shall free g_module_loaders.lock if it is not NULL. ]
TEST_FUNCTION(ModuleLoader_Destroy_does_nothing_when_not_initialized)
{
    // act
    ModuleLoader_Destroy();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_MODULE_LOADER_13_046: [ ModuleLoader_Destroy shall invoke FreeConfiguration on every module loader's configuration field. ]
// Tests_SRS_MODULE_LOADER_13_048: [ ModuleLoader_Destroy shall destroy the loaders vector. ]
TEST_FUNCTION(ModuleLoader_Destroy_frees_resources)
{
    // arrange
    MODULE_LOADER_RESULT init_result = ModuleLoader_Initialize();
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_SUCCESS, init_result);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    for (size_t i = 0; i < LOADERS_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, i))
            .IgnoreArgument(1);
    }
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    ModuleLoader_Destroy();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_MODULE_LOADER_13_046: [ ModuleLoader_Destroy shall invoke FreeConfiguration on every module loader's configuration field. ]
// Tests_SRS_MODULE_LOADER_13_047: [ ModuleLoader_Destroy shall free the loader's name and the loader itself if it is not a default loader. ]
// Tests_SRS_MODULE_LOADER_13_048: [ ModuleLoader_Destroy shall destroy the loaders vector. ]
TEST_FUNCTION(ModuleLoader_Destroy_frees_resources_with_non_default_loaders)
{
    // arrange
    MODULE_LOADER_RESULT init_result = ModuleLoader_Initialize();
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_SUCCESS, init_result);

    // add a non-default module loader
    MODULE_LOADER *non_default_loader = (MODULE_LOADER *)my_gballoc_malloc(sizeof(MODULE_LOADER));
    ASSERT_IS_NOT_NULL(non_default_loader);
    memcpy(non_default_loader, &Dynamic_Module_Loader, sizeof(MODULE_LOADER));
    non_default_loader->configuration = (MODULE_LOADER_BASE_CONFIGURATION*)my_gballoc_malloc(1);

    char* loader_name = (char*)my_gballoc_malloc(strlen("non_default_loader") + 1);
    ASSERT_IS_NOT_NULL(loader_name);
    strcpy(loader_name, "non_default_loader");
    non_default_loader->name = loader_name;

    init_result = ModuleLoader_Add(non_default_loader);
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_SUCCESS, init_result);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    for (size_t i = 0; i < LOADERS_COUNT + 1; i++)
    {
        STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, i))
            .IgnoreArgument(1);

        // for the last module loader we will have some additional calls
        if (i == LOADERS_COUNT)
        {
            STRICT_EXPECTED_CALL(FakeModuleLoader_FreeConfiguration(non_default_loader, non_default_loader->configuration));
            STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
            STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
        }
    }
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    ModuleLoader_Destroy();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_MODULE_LOADER_13_049: [ ModuleLoader_ParseBaseConfigurationFromJson shall return MODULE_LOADER_ERROR if configuration is NULL. ]
TEST_FUNCTION(ModuleLoader_ParseBaseConfigurationFromJson_returns_error_when_config_is_NULL)
{
    // act
    MODULE_LOADER_RESULT result = ModuleLoader_ParseBaseConfigurationFromJson(
        NULL, (const JSON_Value*)0x42
    );

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
}

// Tests_SRS_MODULE_LOADER_13_050: [ ModuleLoader_ParseBaseConfigurationFromJson shall return MODULE_LOADER_ERROR if json is NULL. ]
TEST_FUNCTION(ModuleLoader_ParseBaseConfigurationFromJson_returns_error_when_json_is_NULL)
{
    // act
    MODULE_LOADER_RESULT result = ModuleLoader_ParseBaseConfigurationFromJson(
        (MODULE_LOADER_BASE_CONFIGURATION*)0x42, NULL
    );

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
}

// Tests_SRS_MODULE_LOADER_13_051: [ ModuleLoader_ParseBaseConfigurationFromJson shall return MODULE_LOADER_ERROR if json is not a JSON object. ]
TEST_FUNCTION(ModuleLoader_ParseBaseConfigurationFromJson_returns_error_when_json_type_is_not_object)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONArray);

    // act
    MODULE_LOADER_RESULT result = ModuleLoader_ParseBaseConfigurationFromJson(
        (MODULE_LOADER_BASE_CONFIGURATION*)0x42, (const JSON_Value*)0x42
    );

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_MODULE_LOADER_13_052: [ ModuleLoader_ParseBaseConfigurationFromJson shall return MODULE_LOADER_ERROR if an underlying platform call fails. ]
TEST_FUNCTION(ModuleLoader_ParseBaseConfigurationFromJson_returns_error_when_json_value_get_object_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42))
        .SetReturn(NULL);

    // act
    MODULE_LOADER_RESULT result = ModuleLoader_ParseBaseConfigurationFromJson(
        (MODULE_LOADER_BASE_CONFIGURATION*)0x42, (const JSON_Value*)0x42
    );

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_MODULE_LOADER_13_053: [ ModuleLoader_ParseBaseConfigurationFromJson shall read the value of the string attribute binding.path from the JSON object and assign to configuration->binding_path. ]
// Tests_SRS_MODULE_LOADER_13_054: [ ModuleLoader_ParseBaseConfigurationFromJson shall return MODULE_LOADER_SUCCESS if the parsing is successful. ]
TEST_FUNCTION(ModuleLoader_ParseBaseConfigurationFromJson_succeeds)
{
    // arrange
    MODULE_LOADER_BASE_CONFIGURATION config = { 0 };

    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x42));
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x42, "binding.path"))
        .SetReturn("boo");
    STRICT_EXPECTED_CALL(STRING_construct("boo"));

    // act
    MODULE_LOADER_RESULT result = ModuleLoader_ParseBaseConfigurationFromJson(
        &config, (const JSON_Value*)0x42
    );

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_SUCCESS, result);
    ASSERT_IS_NOT_NULL(config.binding_path);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(strcmp(STRING_c_str(config.binding_path), "boo") == 0);

    // cleanup
    ModuleLoader_FreeBaseConfiguration(&config);
}

// Tests_SRS_MODULE_LOADER_13_055: [ ModuleLoader_FreeBaseConfiguration shall do nothing if configuration is NULL. ]
TEST_FUNCTION(ModuleLoader_FreeBaseConfiguration_does_nothing_when_configuration_is_NULL)
{
    // act
    ModuleLoader_FreeBaseConfiguration(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_MODULE_LOADER_13_056: [ ModuleLoader_FreeBaseConfiguration shall free configuration->binding_path. ]
TEST_FUNCTION(ModuleLoader_FreeBaseConfiguration_frees_resources)
{
    // arrange
    MODULE_LOADER_BASE_CONFIGURATION config = { STRING_construct("boo") };
    ASSERT_IS_NOT_NULL(config.binding_path);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_delete(config.binding_path));

    // act
    ModuleLoader_FreeBaseConfiguration(&config);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_MODULE_LOADER_13_057: [ ModuleLoader_GetDefaultLoaderForType shall return NULL if type is not a recongized loader type. ]
TEST_FUNCTION(ModuleLoader_GetDefaultLoaderForType_returns_NULL_for_UNKNOWN_loader_type)
{
    // act
    MODULE_LOADER* result = ModuleLoader_GetDefaultLoaderForType(UNKNOWN);

    // assert
    ASSERT_IS_NULL(result);
}

// Tests_SRS_MODULE_LOADER_13_058: [ ModuleLoader_GetDefaultLoaderForType shall return a non-NULL MODULE_LOADER pointer when the loader type is a recongized type. ]
TEST_FUNCTION(ModuleLoader_GetDefaultLoaderForType_succeeds)
{
    // arrange
    MODULE_LOADER_RESULT init_result = ModuleLoader_Initialize();
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_SUCCESS, init_result);
    umock_c_reset_all_calls();

    MODULE_LOADER_TYPE inputs[] =
    {
        NATIVE
#ifdef JAVA_BINDING_ENABLED
        , JAVA
#endif
#ifdef DOTNET_BINDING_ENABLED
        , DOTNET
#endif
#ifdef NODE_BINDING_ENABLED
        , NODEJS
#endif
#ifdef NODE_CORE_BINDING_ENABLED
        , DOTNETCORE
#endif
#ifdef OUTPROCESS_ENABLED
        , OUTPROCESS
#endif
    };
    for (size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++)
    {
        STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
    }

    for (size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++)
    {
        // act
        MODULE_LOADER* result = ModuleLoader_GetDefaultLoaderForType(inputs[i]);

        // assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(MODULE_LOADER_TYPE, result->type, inputs[i]);
    }

    // cleanup
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ModuleLoader_Destroy();
}

// Tests_SRS_MODULE_LOADER_13_059: [ ModuleLoader_ParseType shall return UNKNOWN if type is not a recognized module loader type string. ]
TEST_FUNCTION(ModuleLoader_ParseType_returns_UNKNOWN_for_unknown_loader_type)
{
    // act
    MODULE_LOADER_TYPE result = ModuleLoader_ParseType("boo");

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_TYPE, UNKNOWN, result);
}

// Tests_SRS_MODULE_LOADER_13_060: [ ModuleLoader_ParseType shall return a valid MODULE_LOADER_TYPE if type is a recognized module loader type string. ]
TEST_FUNCTION(ModuleLoader_ParseType_succeeds)
{
    // arrange
    char* inputs[] = { "native", "node", "java", "dotnet", "dotnetcore", "outprocess" };
    MODULE_LOADER_TYPE expected[] = { NATIVE, NODEJS, JAVA, DOTNET, DOTNETCORE, OUTPROCESS };

    for (size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++)
    {
        // act
        MODULE_LOADER_TYPE result = ModuleLoader_ParseType(inputs[i]);

        // assert
        ASSERT_ARE_EQUAL(MODULE_LOADER_TYPE, expected[i], result);
    }
}

// Tests_SRS_MODULE_LOADER_13_061: [ ModuleLoader_IsDefaultLoader shall return true if name is the name of a default module loader and false otherwise. The default module loader names are 'native', 'node', 'java' , 'dotnet' and 'dotnetcore'. ]
TEST_FUNCTION(ModuleLoader_IsDefaultLoader_succeeds)
{
    // arrange
    char* inputs[] = { "native", "node", "java", "dotnet", "dotnetcore", "outprocess", "boo" };
    bool expected[] = { true, true, true, true, true, true, false };

    for (size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++)
    {
        // act
        bool result = ModuleLoader_IsDefaultLoader(inputs[i]);

        // assert
        ASSERT_IS_TRUE(result == expected[i]);
    }
}

// Tests_SRS_MODULE_LOADER_13_062: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if loaders is NULL. ]
TEST_FUNCTION(ModuleLoader_InitializeFromJson_returns_MODULE_LOADER_ERROR_when_loaders_is_NULL)
{
    // act
    MODULE_LOADER_RESULT result = ModuleLoader_InitializeFromJson(NULL);

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
}

// Tests_SRS_MODULE_LOADER_13_063: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if loaders is not a JSON array. ]
TEST_FUNCTION(ModuleLoader_InitializeFromJson_returns_MODULE_LOADER_ERROR_when_loaders_is_not_a_json_array)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONObject);

    // act
    MODULE_LOADER_RESULT result = ModuleLoader_InitializeFromJson((const JSON_Value*)0x42);

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_MODULE_LOADER_13_064: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if an underlying platform call fails. ]
TEST_FUNCTION(ModuleLoader_InitializeFromJson_returns_MODULE_LOADER_ERROR_when_json_value_get_array_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONArray);
    STRICT_EXPECTED_CALL(json_value_get_array((const JSON_Value*)0x42))
        .SetReturn(NULL);

    // act
    MODULE_LOADER_RESULT result = ModuleLoader_InitializeFromJson((const JSON_Value*)0x42);

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_MODULE_LOADER_13_064: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if an underlying platform call fails. ]
TEST_FUNCTION(ModuleLoader_InitializeFromJson_returns_MODULE_LOADER_ERROR_when_json_array_get_value_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONArray);
    STRICT_EXPECTED_CALL(json_value_get_array((const JSON_Value*)0x42))
        .SetReturn((JSON_Array*)0x42);
    STRICT_EXPECTED_CALL(json_array_get_count((const JSON_Array*)0x42))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(json_array_get_value((const JSON_Array*)0x42, 0))
        .SetReturn(NULL);

    // act
    MODULE_LOADER_RESULT result = ModuleLoader_InitializeFromJson((const JSON_Value*)0x42);

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_MODULE_LOADER_13_065: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if an entry in the loaders array is not a JSON object. ]
TEST_FUNCTION(ModuleLoader_InitializeFromJson_returns_MODULE_LOADER_ERROR_when_array_entry_is_not_object)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONArray);
    STRICT_EXPECTED_CALL(json_value_get_array((const JSON_Value*)0x42))
        .SetReturn((JSON_Array*)0x42);
    STRICT_EXPECTED_CALL(json_array_get_count((const JSON_Array*)0x42))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(json_array_get_value((const JSON_Array*)0x42, 0))
        .SetReturn((JSON_Value*)0x43);
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x43))
        .SetReturn(JSONArray);

    // act
    MODULE_LOADER_RESULT result = ModuleLoader_InitializeFromJson((const JSON_Value*)0x42);

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_MODULE_LOADER_13_064: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if an underlying platform call fails. ]
TEST_FUNCTION(ModuleLoader_InitializeFromJson_returns_MODULE_LOADER_ERROR_when_array_entry_json_value_get_object_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONArray);
    STRICT_EXPECTED_CALL(json_value_get_array((const JSON_Value*)0x42))
        .SetReturn((JSON_Array*)0x42);
    STRICT_EXPECTED_CALL(json_array_get_count((const JSON_Array*)0x42))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(json_array_get_value((const JSON_Array*)0x42, 0))
        .SetReturn((JSON_Value*)0x43);
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x43))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x43))
        .SetReturn(NULL);

    // act
    MODULE_LOADER_RESULT result = ModuleLoader_InitializeFromJson((const JSON_Value*)0x42);

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_MODULE_LOADER_13_066: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if a loader entry's name or type fields are NULL or are empty strings. ]
TEST_FUNCTION(ModuleLoader_InitializeFromJson_returns_MODULE_LOADER_ERROR_when_array_entry_type_is_NULL)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONArray);
    STRICT_EXPECTED_CALL(json_value_get_array((const JSON_Value*)0x42))
        .SetReturn((JSON_Array*)0x42);
    STRICT_EXPECTED_CALL(json_array_get_count((const JSON_Array*)0x42))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(json_array_get_value((const JSON_Array*)0x42, 0))
        .SetReturn((JSON_Value*)0x43);
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x43))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x43))
        .SetReturn((JSON_Object*)0x44);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x44, "type"))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x44, "name"))
        .SetReturn("boo");

    // act
    MODULE_LOADER_RESULT result = ModuleLoader_InitializeFromJson((const JSON_Value*)0x42);

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_MODULE_LOADER_13_066: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if a loader entry's name or type fields are NULL or are empty strings. ]
TEST_FUNCTION(ModuleLoader_InitializeFromJson_returns_MODULE_LOADER_ERROR_when_array_entry_type_is_empty_string)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONArray);
    STRICT_EXPECTED_CALL(json_value_get_array((const JSON_Value*)0x42))
        .SetReturn((JSON_Array*)0x42);
    STRICT_EXPECTED_CALL(json_array_get_count((const JSON_Array*)0x42))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(json_array_get_value((const JSON_Array*)0x42, 0))
        .SetReturn((JSON_Value*)0x43);
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x43))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x43))
        .SetReturn((JSON_Object*)0x44);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x44, "type"))
        .SetReturn("");
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x44, "name"))
        .SetReturn("boo");

    // act
    MODULE_LOADER_RESULT result = ModuleLoader_InitializeFromJson((const JSON_Value*)0x42);

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_MODULE_LOADER_13_066: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if a loader entry's name or type fields are NULL or are empty strings. ]
TEST_FUNCTION(ModuleLoader_InitializeFromJson_returns_MODULE_LOADER_ERROR_when_array_entry_name_is_NULL)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONArray);
    STRICT_EXPECTED_CALL(json_value_get_array((const JSON_Value*)0x42))
        .SetReturn((JSON_Array*)0x42);
    STRICT_EXPECTED_CALL(json_array_get_count((const JSON_Array*)0x42))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(json_array_get_value((const JSON_Array*)0x42, 0))
        .SetReturn((JSON_Value*)0x43);
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x43))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x43))
        .SetReturn((JSON_Object*)0x44);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x44, "type"))
        .SetReturn("native");
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x44, "name"))
        .SetReturn(NULL);

    // act
    MODULE_LOADER_RESULT result = ModuleLoader_InitializeFromJson((const JSON_Value*)0x42);

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_MODULE_LOADER_13_066: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if a loader entry's name or type fields are NULL or are empty strings. ]
TEST_FUNCTION(ModuleLoader_InitializeFromJson_returns_MODULE_LOADER_ERROR_when_array_entry_name_is_empty_string)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONArray);
    STRICT_EXPECTED_CALL(json_value_get_array((const JSON_Value*)0x42))
        .SetReturn((JSON_Array*)0x42);
    STRICT_EXPECTED_CALL(json_array_get_count((const JSON_Array*)0x42))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(json_array_get_value((const JSON_Array*)0x42, 0))
        .SetReturn((JSON_Value*)0x43);
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x43))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x43))
        .SetReturn((JSON_Object*)0x44);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x44, "type"))
        .SetReturn("native");
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x44, "name"))
        .SetReturn("");

    // act
    MODULE_LOADER_RESULT result = ModuleLoader_InitializeFromJson((const JSON_Value*)0x42);

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_MODULE_LOADER_13_067: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if a loader entry's type field has an unknown value. ]
TEST_FUNCTION(ModuleLoader_InitializeFromJson_returns_MODULE_LOADER_ERROR_when_array_entry_type_is_unknown)
{
    // arrange
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONArray);
    STRICT_EXPECTED_CALL(json_value_get_array((const JSON_Value*)0x42))
        .SetReturn((JSON_Array*)0x42);
    STRICT_EXPECTED_CALL(json_array_get_count((const JSON_Array*)0x42))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(json_array_get_value((const JSON_Array*)0x42, 0))
        .SetReturn((JSON_Value*)0x43);
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x43))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x43))
        .SetReturn((JSON_Object*)0x44);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x44, "type"))
        .SetReturn("invalid_loader_type");
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x44, "name"))
        .SetReturn("boo");

    // act
    MODULE_LOADER_RESULT result = ModuleLoader_InitializeFromJson((const JSON_Value*)0x42);

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_MODULE_LOADER_13_068: [ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if no default loader could be located for a loader entry.]
TEST_FUNCTION(ModuleLoader_InitializeFromJson_returns_MODULE_LOADER_ERROR_when_default_loader_cannot_be_found)
{
    // arrange
    // arrange
    MODULE_LOADER_RESULT init_result = ModuleLoader_Initialize();
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_SUCCESS, init_result);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONArray);
    STRICT_EXPECTED_CALL(json_value_get_array((const JSON_Value*)0x42))
        .SetReturn((JSON_Array*)0x42);
    STRICT_EXPECTED_CALL(json_array_get_count((const JSON_Array*)0x42))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(json_array_get_value((const JSON_Array*)0x42, 0))
        .SetReturn((JSON_Value*)0x43);
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x43))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x43))
        .SetReturn((JSON_Object*)0x44);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x44, "type"))
        .SetReturn("native");
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x44, "name"))
        .SetReturn("boo");
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    MODULE_LOADER_RESULT result = ModuleLoader_InitializeFromJson((const JSON_Value*)0x42);

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    ModuleLoader_Destroy();
}

// Tests_SRS_MODULE_LOADER_13_064: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if an underlying platform call fails. ]
TEST_FUNCTION(ModuleLoader_InitializeFromJson_returns_MODULE_LOADER_ERROR_when_malloc_fails)
{
    MODULE_LOADER_BASE_CONFIGURATION* captured_loader = NULL;

    // arrange
    MODULE_LOADER_RESULT init_result = ModuleLoader_Initialize();
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_SUCCESS, init_result);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONArray);
    STRICT_EXPECTED_CALL(json_value_get_array((const JSON_Value*)0x42))
        .SetReturn((JSON_Array*)0x42);
    STRICT_EXPECTED_CALL(json_array_get_count((const JSON_Array*)0x42))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(json_array_get_value((const JSON_Array*)0x42, 0))
        .SetReturn((JSON_Value*)0x43);
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x43))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x43))
        .SetReturn((JSON_Object*)0x44);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x44, "type"))
        .SetReturn("native");
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x44, "name"))
        .SetReturn("boo");
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    // Ideally, we should use the following for this expectation. But for some reason
    // umock_c completely ignores this SetReturn call.
    // STRICT_EXPECTED_CALL(json_object_get_value((const JSON_Object*)0x44, "configuration"))
    //    .SetReturn((JSON_Value*)0x45);
    STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(FakeModuleLoader_ParseConfigurationFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42))
        .IgnoreArgument(1)
        .CaptureReturn(&captured_loader);
    malloc_will_fail = true;
    malloc_fail_count = 3;
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(MODULE_LOADER)));

    // act
    MODULE_LOADER_RESULT result = ModuleLoader_InitializeFromJson((const JSON_Value*)0x42);

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    FakeModuleLoader_FreeConfiguration(IGNORED_PTR_ARG, captured_loader);
    ModuleLoader_Destroy();
}

// Tests_SRS_MODULE_LOADER_13_064: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if an underlying platform call fails. ]
TEST_FUNCTION(ModuleLoader_InitializeFromJson_returns_MODULE_LOADER_ERROR_when_malloc_fails2)
{
    // arrange
    MODULE_LOADER_RESULT init_result = ModuleLoader_Initialize();
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_SUCCESS, init_result);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONArray);
    STRICT_EXPECTED_CALL(json_value_get_array((const JSON_Value*)0x42))
        .SetReturn((JSON_Array*)0x42);
    STRICT_EXPECTED_CALL(json_array_get_count((const JSON_Array*)0x42))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(json_array_get_value((const JSON_Array*)0x42, 0))
        .SetReturn((JSON_Value*)0x43);
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x43))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x43))
        .SetReturn((JSON_Object*)0x44);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x44, "type"))
        .SetReturn("native");
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x44, "name"))
        .SetReturn("boo");
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    // Ideally, we should use the following for this expectation. But for some reason
    // umock_c completely ignores this SetReturn call.
    // STRICT_EXPECTED_CALL(json_object_get_value((const JSON_Object*)0x44, "configuration"))
    //    .SetReturn((JSON_Value*)0x45);
    STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(FakeModuleLoader_ParseConfigurationFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(MODULE_LOADER)));
    malloc_will_fail = true;
    malloc_fail_count = 4;
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(FakeModuleLoader_FreeConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    MODULE_LOADER_RESULT result = ModuleLoader_InitializeFromJson((const JSON_Value*)0x42);

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    ModuleLoader_Destroy();
}

// Tests_SRS_MODULE_LOADER_13_064: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if an underlying platform call fails. ]
// Tests_SRS_MODULE_LOADER_13_071: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_ERROR if ModuleLoader_Add fails. ]
TEST_FUNCTION(ModuleLoader_InitializeFromJson_returns_MODULE_LOADER_ERROR_when_ModuleLoader_Add_fails)
{
    // arrange
    MODULE_LOADER_RESULT init_result = ModuleLoader_Initialize();
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_SUCCESS, init_result);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONArray);
    STRICT_EXPECTED_CALL(json_value_get_array((const JSON_Value*)0x42))
        .SetReturn((JSON_Array*)0x42);
    STRICT_EXPECTED_CALL(json_array_get_count((const JSON_Array*)0x42))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(json_array_get_value((const JSON_Array*)0x42, 0))
        .SetReturn((JSON_Value*)0x43);
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x43))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x43))
        .SetReturn((JSON_Object*)0x44);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x44, "type"))
        .SetReturn("native");
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x44, "name"))
        .SetReturn("boo");
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    // Ideally, we should use the following for this expectation. But for some reason
    // umock_c completely ignores this SetReturn call.
    // STRICT_EXPECTED_CALL(json_object_get_value((const JSON_Object*)0x44, "configuration"))
    //    .SetReturn((JSON_Value*)0x45);
    STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(FakeModuleLoader_ParseConfigurationFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(MODULE_LOADER)));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(LOCK_ERROR);
    STRICT_EXPECTED_CALL(FakeModuleLoader_FreeConfiguration(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    MODULE_LOADER_RESULT result = ModuleLoader_InitializeFromJson((const JSON_Value*)0x42);

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    ModuleLoader_Destroy();
}

// Tests_SRS_MODULE_LOADER_13_069: [ ModuleLoader_InitializeFromJson shall invoke ParseConfigurationFromJson to parse the loader entry's configuration JSON. ]
// Tests_SRS_MODULE_LOADER_13_070: [ ModuleLoader_InitializeFromJson shall allocate a MODULE_LOADER and add the loader to the gateway by calling ModuleLoader_Add if the loader entry is not for a default loader. ]
TEST_FUNCTION(ModuleLoader_InitializeFromJson_succeeds_with_custom_loader)
{
    // arrange
    MODULE_LOADER_RESULT init_result = ModuleLoader_Initialize();
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_SUCCESS, init_result);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONArray);
    STRICT_EXPECTED_CALL(json_value_get_array((const JSON_Value*)0x42))
        .SetReturn((JSON_Array*)0x42);
    STRICT_EXPECTED_CALL(json_array_get_count((const JSON_Array*)0x42))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(json_array_get_value((const JSON_Array*)0x42, 0))
        .SetReturn((JSON_Value*)0x43);
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x43))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x43))
        .SetReturn((JSON_Object*)0x44);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x44, "type"))
        .SetReturn("native");
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x44, "name"))
        .SetReturn("boo");
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    // Ideally, we should use the following for this expectation. But for some reason
    // umock_c completely ignores this SetReturn call.
    // STRICT_EXPECTED_CALL(json_object_get_value((const JSON_Object*)0x44, "configuration"))
    //    .SetReturn((JSON_Value*)0x45);
    STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(FakeModuleLoader_ParseConfigurationFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(MODULE_LOADER)));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    MODULE_LOADER_RESULT result = ModuleLoader_InitializeFromJson((const JSON_Value*)0x42);

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_SUCCESS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    ModuleLoader_Destroy();
}

// Tests_SRS_MODULE_LOADER_13_069: [ ModuleLoader_InitializeFromJson shall invoke ParseConfigurationFromJson to parse the loader entry's configuration JSON. ]
// Tests_SRS_MODULE_LOADER_13_072: [ ModuleLoader_InitializeFromJson shall update the configuration on the default loader if the entry is for a default loader by calling ModuleLoader_UpdateConfiguration. ]
// Tests_SRS_MODULE_LOADER_13_073: [ ModuleLoader_InitializeFromJson shall return MODULE_LOADER_SUCCESS if the the JSON has been processed successfully. ]
TEST_FUNCTION(ModuleLoader_InitializeFromJson_succeeds_with_default_loader)
{
    // arrange
    MODULE_LOADER_RESULT init_result = ModuleLoader_Initialize();
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_SUCCESS, init_result);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x42))
        .SetReturn(JSONArray);
    STRICT_EXPECTED_CALL(json_value_get_array((const JSON_Value*)0x42))
        .SetReturn((JSON_Array*)0x42);
    STRICT_EXPECTED_CALL(json_array_get_count((const JSON_Array*)0x42))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(json_array_get_value((const JSON_Array*)0x42, 0))
        .SetReturn((JSON_Value*)0x43);
    STRICT_EXPECTED_CALL(json_value_get_type((const JSON_Value*)0x43))
        .SetReturn(JSONObject);
    STRICT_EXPECTED_CALL(json_value_get_object((const JSON_Value*)0x43))
        .SetReturn((JSON_Object*)0x44);
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x44, "type"))
        .SetReturn("native");
    STRICT_EXPECTED_CALL(json_object_get_string((const JSON_Object*)0x44, "name"))
        .SetReturn("native");
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    // Ideally, we should use the following for this expectation. But for some reason
    // umock_c completely ignores this SetReturn call.
    // STRICT_EXPECTED_CALL(json_object_get_value((const JSON_Object*)0x44, "configuration"))
    //    .SetReturn((JSON_Value*)0x45);
    STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(FakeModuleLoader_ParseConfigurationFromJson(IGNORED_PTR_ARG, (const JSON_Value*)0x42))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    MODULE_LOADER_RESULT result = ModuleLoader_InitializeFromJson((const JSON_Value*)0x42);

    // assert
    ASSERT_ARE_EQUAL(MODULE_LOADER_RESULT, MODULE_LOADER_SUCCESS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    ModuleLoader_Destroy();
    Dynamic_Module_Loader.configuration = NULL;
}

END_TEST_SUITE(ModuleLoader_UnitTests);
