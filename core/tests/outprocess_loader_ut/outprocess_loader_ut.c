// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>

#define disableNegativeTest(x) (negative_tests_to_skip |= ((uint64_t)1 << (x)))
#define disableNegativeTestsBeforeIndex(x) for (size_t i = (x) ; i > 0 ; disableNegativeTest(--i))
#define enableNegativeTest(x) (negative_tests_to_skip &= ~((uint64_t)1 << (x)))
#define skipNegativeTest(x) (negative_tests_to_skip & ((uint64_t)1 << (x)))

#define MOCK_UV_LOOP (uv_loop_t *)0x09171979
#define MOCK_UV_PROCESS_VECTOR (VECTOR_HANDLE)0x19790917

static size_t negative_test_index;
static uint64_t negative_tests_to_skip;

#define GATEWAY_EXPORT_H
#define GATEWAY_EXPORT

#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umock_c_negative_tests.h"
#include "umocktypes_charptr.h"
#include "umocktypes_bool.h"
#include "umocktypes_stdint.h"
#include "real_strings.h"

static void ** global_ptrs = NULL;
static size_t global_malloc_count = 0;
static bool global_memory = false;
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
        if (global_memory) {
            ++global_malloc_count;
            global_ptrs = (void **)realloc(global_ptrs, (sizeof(void *) * global_malloc_count));
            ASSERT_IS_NOT_NULL(global_ptrs);
            global_ptrs[(global_malloc_count - 1)] = result;
        }
    }

    return result;
}

void my_gballoc_free(void* ptr)
{
    if (global_memory) {
        --global_malloc_count;
        ptr = global_ptrs[global_malloc_count];
        if (!global_malloc_count) {
            free(global_ptrs);
            global_ptrs = NULL;
        }
    }

    free(ptr);
}

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/uniqueid.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/vector.h"

#include "parson.h"
#include "uv.h"
#include "module_loader.h"

#include  "control_message.h"
#undef ENABLE_MOCKS

#include "module_loaders/outprocess_loader.h"
#include "module_loaders/outprocess_module.h"

static const THREAD_HANDLE MOCK_THREAD_HANDLE = (THREAD_HANDLE *)0x19791709;

#ifdef __cplusplus
  extern "C" {
#endif

int launch_child_process_from_entrypoint(OUTPROCESS_LOADER_ENTRYPOINT * outprocess_entry);
int spawn_child_processes(void * context);
int update_entrypoint_with_launch_object(OUTPROCESS_LOADER_ENTRYPOINT * outprocess_entry, const JSON_Object * launch_object);
int validate_launch_arguments(const JSON_Object * launch_object);

#ifdef __cplusplus
  }
#endif

static pfModuleLoader_Load OutprocessModuleLoader_Load = NULL;
static pfModuleLoader_Unload OutprocessModuleLoader_Unload = NULL;
static pfModuleLoader_GetApi OutprocessModuleLoader_GetModuleApi = NULL;
static pfModuleLoader_ParseEntrypointFromJson OutprocessModuleLoader_ParseEntrypointFromJson = NULL;
static pfModuleLoader_FreeEntrypoint OutprocessModuleLoader_FreeEntrypoint = NULL;
static pfModuleLoader_ParseConfigurationFromJson OutprocessModuleLoader_ParseConfigurationFromJson = NULL;
static pfModuleLoader_FreeConfiguration OutprocessModuleLoader_FreeConfiguration = NULL;
static pfModuleLoader_BuildModuleConfiguration OutprocessModuleLoader_BuildModuleConfiguration = NULL;
static pfModuleLoader_FreeModuleConfiguration OutprocessModuleLoader_FreeModuleConfiguration = NULL;

// ** Mocking parson.h
MOCKABLE_FUNCTION(, void, json_free_serialized_string, char*, string);
MOCKABLE_FUNCTION(, size_t, json_array_get_count, const JSON_Array*, array);
MOCKABLE_FUNCTION(, const char*, json_array_get_string, const JSON_Array*, array, size_t, size);
MOCKABLE_FUNCTION(, char*, json_serialize_to_string, const JSON_Value*, value);
MOCKABLE_FUNCTION(, JSON_Array*, json_object_get_array, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, JSON_Object*, json_object_get_object, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, JSON_Value*, json_object_get_value, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, const char*, json_object_get_string, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, double, json_object_get_number, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, JSON_Object*, json_value_get_object, const JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Value_Type, json_value_get_type, const JSON_Value*, value);

// ** Mocking uv.h (Process handle)
MOCK_FUNCTION_WITH_CODE(, int, uv_process_kill, uv_process_t *, handle, int, signum)
MOCK_FUNCTION_END(0);
MOCK_FUNCTION_WITH_CODE(, int, uv_spawn, uv_loop_t *, loop, uv_process_t *, handle, const uv_process_options_t *, options)
MOCK_FUNCTION_END(0);
MOCK_FUNCTION_WITH_CODE(, void, uv_close, uv_handle_t *, handle, uv_close_cb, close_cb);
MOCK_FUNCTION_END();

// ** Mocking uv.h (Event loop)
MOCK_FUNCTION_WITH_CODE(, uv_loop_t *, uv_default_loop);
MOCK_FUNCTION_END(MOCK_UV_LOOP);
MOCK_FUNCTION_WITH_CODE(, int, uv_loop_alive, const uv_loop_t *, loop);
MOCK_FUNCTION_END(0);
MOCK_FUNCTION_WITH_CODE(, int, uv_run, uv_loop_t *, loop, uv_run_mode, mode);
MOCK_FUNCTION_END(0);

//=============================================================================
//Globals
//=============================================================================

static TEST_MUTEX_HANDLE g_dllByDll;
static TEST_MUTEX_HANDLE g_testByTest;
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
    (void)error_code;
    ASSERT_FAIL("umock_c reported error");
}

MOCK_FUNCTION_WITH_CODE(, MODULE_API*, Fake_GetAPI, MODULE_API_VERSION, gateway_api_version)
MODULE_API* val = (MODULE_API*)0x42;
MOCK_FUNCTION_END(val)

//parson mocks

MOCK_FUNCTION_WITH_CODE(, size_t, json_array_get_count, const JSON_Array*, array)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, const char*, json_array_get_string, const JSON_Array*, array, size_t, size)
MOCK_FUNCTION_END(NULL)

MOCK_FUNCTION_WITH_CODE(, JSON_Array*, json_object_get_array, const JSON_Object*, object, const char*, name)
MOCK_FUNCTION_END(NULL)

MOCK_FUNCTION_WITH_CODE(, JSON_Object*, json_object_get_object, const JSON_Object*, object, const char*, name)
MOCK_FUNCTION_END(NULL)

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

MOCK_FUNCTION_WITH_CODE(, double, json_object_get_number, const JSON_Object*, object, const char*, name)
MOCK_FUNCTION_END(0);

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

TEST_DEFINE_ENUM_TYPE(MODULE_LOADER_TYPE, MODULE_LOADER_TYPE_VALUES);

static inline
void
expected_calls_OutprocessLoader_JoinChildProcesses (
    size_t process_count_,
    bool children_have_exited_,
    size_t grace_period_ms_,
    size_t children_close_ms_
) {
    static const TICK_COUNTER_HANDLE MOCK_TICKCOUNTER = (TICK_COUNTER_HANDLE)0x19790917;
    static uv_process_t * MOCK_UV_PROCESS = (uv_process_t *)0x17091979;

    tickcounter_ms_t injected_ms = 0;
    bool timed_out;

    disableNegativeTest(negative_test_index++);
    STRICT_EXPECTED_CALL(VECTOR_size(MOCK_UV_PROCESS_VECTOR))
        .SetReturn(process_count_);
    disableNegativeTest(negative_test_index++);
    EXPECTED_CALL(uv_default_loop())
        .SetReturn(MOCK_UV_LOOP);
    disableNegativeTest(negative_test_index++);
    if (children_have_exited_) {
        STRICT_EXPECTED_CALL(uv_loop_alive(MOCK_UV_LOOP))
            .SetReturn(0);
    } else {
        STRICT_EXPECTED_CALL(uv_loop_alive(MOCK_UV_LOOP))
            .SetReturn(__LINE__);
        enableNegativeTest(negative_test_index++);
        EXPECTED_CALL(tickcounter_create())
            .SetFailReturn(NULL)
            .SetReturn(MOCK_TICKCOUNTER);
        enableNegativeTest(negative_test_index++);
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(MOCK_TICKCOUNTER, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &injected_ms, sizeof(tickcounter_ms_t))
            .IgnoreArgument(2)
            .SetFailReturn(__LINE__)
            .SetReturn(0);

        for (timed_out = true; injected_ms <= grace_period_ms_; injected_ms += 100) {
            enableNegativeTest(negative_test_index++);
            STRICT_EXPECTED_CALL(tickcounter_get_current_ms(MOCK_TICKCOUNTER, IGNORED_PTR_ARG))
                .CopyOutArgumentBuffer(2, &injected_ms, sizeof(tickcounter_ms_t))
                .IgnoreArgument(2)
                .SetFailReturn(__LINE__)
                .SetReturn(0);
            disableNegativeTest(negative_test_index++);
            EXPECTED_CALL(uv_default_loop())
                .SetReturn(MOCK_UV_LOOP);
            disableNegativeTest(negative_test_index++);
            if (children_close_ms_ <= injected_ms) {
                STRICT_EXPECTED_CALL(uv_loop_alive(MOCK_UV_LOOP))
                    .SetReturn(0);
                timed_out = false;
                break;
            } else {
                STRICT_EXPECTED_CALL(uv_loop_alive(MOCK_UV_LOOP))
                    .SetReturn(__LINE__);
            }
            disableNegativeTest(negative_test_index++);
            STRICT_EXPECTED_CALL(ThreadAPI_Sleep(100));
        }

        if (timed_out) {
            for (size_t i = 0; i < process_count_; ++i) {
                disableNegativeTest(negative_test_index++);
                STRICT_EXPECTED_CALL(VECTOR_element(MOCK_UV_PROCESS_VECTOR, i))
                    .SetReturn(&MOCK_UV_PROCESS);
                disableNegativeTest(negative_test_index++);
                STRICT_EXPECTED_CALL(uv_process_kill(MOCK_UV_PROCESS, SIGTERM))
                    .SetFailReturn(__LINE__)
                    .SetReturn(0);
            }
        }
        disableNegativeTest(negative_test_index++);
        STRICT_EXPECTED_CALL(tickcounter_destroy(MOCK_TICKCOUNTER));
    }

    disableNegativeTest(negative_test_index++);
    STRICT_EXPECTED_CALL(ThreadAPI_Join(MOCK_THREAD_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetFailReturn(THREADAPI_ERROR)
        .SetReturn(THREADAPI_OK);
    for (size_t i = 0; i < process_count_; ++i) {
        disableNegativeTest(negative_test_index++);
        STRICT_EXPECTED_CALL(VECTOR_element(MOCK_UV_PROCESS_VECTOR, i))
            .SetReturn(&MOCK_UV_PROCESS);
        disableNegativeTest(negative_test_index++);
        STRICT_EXPECTED_CALL(gballoc_free(MOCK_UV_PROCESS));
    }
    disableNegativeTest(negative_test_index++);
    STRICT_EXPECTED_CALL(VECTOR_destroy(MOCK_UV_PROCESS_VECTOR));
}

static inline
void
expected_calls_OutprocessLoader_SpawnChildProcesses (
    bool first_call_
) {
    if (first_call_) {
        enableNegativeTest(negative_test_index++);
        STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL))
            .CopyOutArgumentBuffer(1, &MOCK_THREAD_HANDLE, sizeof(THREAD_HANDLE))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .SetFailReturn(THREADAPI_ERROR)
            .SetReturn(THREADAPI_OK);
    }
}

static inline
void
expected_calls_launch_child_process_from_entrypoint (
    bool first_call_
) {
    static uv_process_t * MOCK_UV_PROCESS = (uv_process_t *)0x17091979;

    if (first_call_) {
        enableNegativeTest(negative_test_index++);
        STRICT_EXPECTED_CALL(VECTOR_create(sizeof(uv_process_t *)))
            .SetFailReturn(NULL)
            .SetReturn(MOCK_UV_PROCESS_VECTOR);
    }
    enableNegativeTest(negative_test_index++);
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(uv_process_t)))
        .SetFailReturn(NULL);

    enableNegativeTest(negative_test_index++);
    STRICT_EXPECTED_CALL(VECTOR_push_back(MOCK_UV_PROCESS_VECTOR, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(2)
        .SetFailReturn(__LINE__)
        .SetReturn(0);
    disableNegativeTest(negative_test_index++);
    EXPECTED_CALL(uv_default_loop());
    enableNegativeTest(negative_test_index++);
    STRICT_EXPECTED_CALL(uv_spawn(MOCK_UV_LOOP, MOCK_UV_PROCESS, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .SetFailReturn(__LINE__)
        .SetReturn(0);
}

static inline
void
expected_calls_spawn_child_processes (
    bool failed
) {
    static const int FAIL_RESULT = 1979;
    disableNegativeTest(negative_test_index++);
    EXPECTED_CALL(uv_default_loop());
    enableNegativeTest(negative_test_index++);
    STRICT_EXPECTED_CALL(uv_run(MOCK_UV_LOOP, UV_RUN_DEFAULT))
        .SetFailReturn(FAIL_RESULT)
        .SetReturn(0);
    disableNegativeTest(negative_test_index++);
    if (failed) {
        STRICT_EXPECTED_CALL(ThreadAPI_Exit(FAIL_RESULT));
    } else {
        STRICT_EXPECTED_CALL(ThreadAPI_Exit(0));
    }
}

static inline
void
expected_calls_update_entrypoint_with_launch_object (
    void
) {
    srand((unsigned int)time(NULL));
    static JSON_Array * JSON_PARAMETER_ARRAY = (JSON_Array *)__LINE__;
    const size_t PARAMETER_COUNT = (rand() % 5);

    disableNegativeTest(negative_test_index++);
    EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetFailReturn(NULL)
        .SetReturn("../path/to/program.exe")
        .ValidateArgumentBuffer(2, "path", (sizeof("path") - 1));
    disableNegativeTest(negative_test_index++);
    EXPECTED_CALL(json_object_get_array(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetFailReturn(NULL)
        .SetReturn(JSON_PARAMETER_ARRAY)
        .ValidateArgumentBuffer(2, "args", (sizeof("args") - 1));
    disableNegativeTest(negative_test_index++);
    EXPECTED_CALL(json_array_get_count(IGNORED_PTR_ARG))
        .SetFailReturn(0)
        .SetReturn(PARAMETER_COUNT);
    enableNegativeTest(negative_test_index++);
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(char *) * (PARAMETER_COUNT + 2)))
        .SetFailReturn(NULL);
    enableNegativeTest(negative_test_index++);
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof("../path/to/program.exe")))
        .SetFailReturn(NULL);
    for (size_t i = 0; i < PARAMETER_COUNT; ++i) {
        disableNegativeTest(negative_test_index++);
        // Ensures parameters are not index shifted by one
        if (i % 2) {
            STRICT_EXPECTED_CALL(json_array_get_string(JSON_PARAMETER_ARRAY, i))
                .SetFailReturn(NULL)
                .SetReturn("program_argument");
            enableNegativeTest(negative_test_index++);
            STRICT_EXPECTED_CALL(gballoc_malloc(sizeof("program_argument")))
                .SetFailReturn(NULL);
        } else {
            STRICT_EXPECTED_CALL(json_array_get_string(JSON_PARAMETER_ARRAY, i))
                .SetFailReturn(NULL)
                .SetReturn("other_program_argument");
            enableNegativeTest(negative_test_index++);
            STRICT_EXPECTED_CALL(gballoc_malloc(sizeof("other_program_argument")))
                .SetFailReturn(NULL);
        }
    }
}

static inline
void
expected_calls_validate_launch_arguments (
    const double *grace_period_ms
) {
    enableNegativeTest(negative_test_index++);
    EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetFailReturn(NULL)
        .SetReturn("../path/to/program.exe")
        .ValidateArgumentBuffer(2, "path", (sizeof("path") - 1));
    disableNegativeTest(negative_test_index++);
    EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetFailReturn(NULL)
        .SetReturn("1500")
        .ValidateArgumentBuffer(2, "grace.period.ms", (sizeof("grace.period.ms") - 1));
    if (grace_period_ms) {
        disableNegativeTest(negative_test_index++);
        EXPECTED_CALL(json_object_get_number(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .SetFailReturn(0.0)
            .SetReturn(*grace_period_ms)
            .ValidateArgumentBuffer(2, "grace.period.ms", (sizeof("grace.period.ms") - 1));
    }
}

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
	REGISTER_UMOCK_ALIAS_TYPE(uv_loop_t *, void *);
	REGISTER_UMOCK_ALIAS_TYPE(uv_process_t *, void *);
    REGISTER_UMOCK_ALIAS_TYPE(uv_run_mode, int);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_HANDLE, void *);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_START_FUNC, void *);
    REGISTER_UMOCK_ALIAS_TYPE(THREADAPI_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(TICK_COUNTER_HANDLE, void *);
    REGISTER_UMOCK_ALIAS_TYPE(VECTOR_HANDLE, void *);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_object, NULL);

    // malloc/free hooks
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    // Strings hooks
    REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, real_STRING_construct);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_clone, real_STRING_clone);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, real_STRING_delete);
	REGISTER_GLOBAL_MOCK_HOOK(STRING_c_str, real_STRING_c_str);

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
    negative_test_index = 0;
    negative_tests_to_skip = 0;
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
	OUTPROCESS_LOADER_ENTRYPOINT entrypoint = { OUTPROCESS_LOADER_ACTIVATION_NONE, NULL, (STRING_HANDLE)0x42, 0, NULL, 0 };
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

/*Tests_SRS_OUTPROCESS_LOADER_27_003: [ If the entrypoint's `activation_type` is invalid, then `OutprocessModuleLoader_Load` shall return `NULL`. ]*/
TEST_FUNCTION(OutprocessModuleLoader_Load_returns_NULL_when_activation_type_is_not_NONE)
{
	// arrange
	OUTPROCESS_LOADER_ENTRYPOINT entrypoint = { (OUTPROCESS_LOADER_ACTIVATION_TYPE)0x42, (STRING_HANDLE)0x42, (STRING_HANDLE)0x42, 0, NULL, 0};
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
    char * process_argv[] = {
        "program.exe",
        "control.id"
    };
    OUTPROCESS_LOADER_ENTRYPOINT entrypoint = {
        OUTPROCESS_LOADER_ACTIVATION_NONE,
        (STRING_HANDLE)0x42,
        (STRING_HANDLE)0x42,
        sizeof(process_argv),
        process_argv,
        0
    };
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
/*Tests_SRS_OUTPROCESS_LOADER_27_005: [ Launch - `OutprocessModuleLoader_Load` shall launch the child process identified by the entrypoint. ]*/
/*Tests_SRS_OUTPROCESS_LOADER_27_077: [ Launch - `OutprocessModuleLoader_Load` shall spawn the enqueued child processes. ] */
/*Tests_SRS_OUTPROCESS_LOADER_17_006: [ The loader shall store the Outprocess_Module_API_all in the loader handle. ]*/
/*Tests_SRS_OUTPROCESS_LOADER_17_007: [ Upon success, this function shall return a valid pointer to the loader handle. ]*/
TEST_FUNCTION(OutprocessModuleLoader_Load_succeeds)
{
    // arrange
    global_memory = true;
    char * process_argv[] = {
        "program.exe",
        "control.id"
    };
    OUTPROCESS_LOADER_ENTRYPOINT entrypoint = {
        OUTPROCESS_LOADER_ACTIVATION_LAUNCH,
        (STRING_HANDLE)0x42,
        (STRING_HANDLE)0x42,
        sizeof(process_argv),
        process_argv,
        0
    };
	MODULE_LOADER loader =
	{
		OUTPROCESS,
		NULL, NULL, NULL
	};

    umock_c_reset_all_calls();
    expected_calls_launch_child_process_from_entrypoint(true);
    expected_calls_OutprocessLoader_SpawnChildProcesses(true);

    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    // act
    MODULE_LIBRARY_HANDLE result = OutprocessModuleLoader_Load(&loader, &entrypoint);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    OutprocessModuleLoader_Unload(&loader, result);
    OutprocessLoader_JoinChildProcesses();
    global_memory = false;
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
	OUTPROCESS_LOADER_ENTRYPOINT entrypoint = { OUTPROCESS_LOADER_ACTIVATION_NONE, (STRING_HANDLE)0x42, (STRING_HANDLE)0x42, 0, NULL, 0};
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
	OUTPROCESS_LOADER_ENTRYPOINT entrypoint = { OUTPROCESS_LOADER_ACTIVATION_NONE, (STRING_HANDLE)0x42, (STRING_HANDLE)0x42, 0, NULL, 0};
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
TEST_FUNCTION(OutprocessModuleLoader_ParseEntrypointFromJson_returns_NULL_when_json_object_activation_type_NULL)
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
    STRICT_EXPECTED_CALL(json_object_get_object((JSON_Object*)0x43, "launch"));
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

	STRICT_EXPECTED_CALL(json_value_get_type((JSON_Value*)0x42))
		.SetReturn(JSONObject);
	STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x42))
		.SetReturn((JSON_Object*)0x43);
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "activation.type"))
		.SetReturn(activation_type);
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "control.id"))
		.SetReturn(NULL);
    STRICT_EXPECTED_CALL(json_object_get_object((JSON_Object*)0x43, "launch"));
    STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "message.id"))
		.SetReturn(NULL);

	// act
	void* result = OutprocessModuleLoader_ParseEntrypointFromJson(NULL, (JSON_Value*)0x42);

	// assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_OUTPROCESS_LOADER_27_014: [ This function shall return `NULL` if `activation.type` is `OUTPROCESS_LOADER_ACTIVATION_INVALID`. ]*/
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
    STRICT_EXPECTED_CALL(json_object_get_object((JSON_Object*)0x43, "launch"));
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
    STRICT_EXPECTED_CALL(json_object_get_object((JSON_Object*)0x43, "launch"));
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
/*Tests_SRS_OUTPROCESS_LOADER_17_044: [ If "timeout" is set, the remote_message_wait shall be set to this value, else it will be set to a default of 1000 ms. ]*/
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
    STRICT_EXPECTED_CALL(json_object_get_object((JSON_Object*)0x43, "launch"));
    STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "message.id"))
		.SetReturn(NULL);
	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(OUTPROCESS_LOADER_ENTRYPOINT)));
	STRICT_EXPECTED_CALL(STRING_construct(control_id))
		.SetReturn(NULL);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);


	// act
	void* result = OutprocessModuleLoader_ParseEntrypointFromJson(NULL, (JSON_Value*)0x42);

	// assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_OUTPROCESS_LOADER_27_015: [ Launch - `OutprocessModuleLoader_ParseEntrypointFromJson` shall validate the launch parameters. ]*/
/*Tests_SRS_OUTPROCESS_LOADER_17_016: [ This function shall allocate a OUTPROCESS_LOADER_ENTRYPOINT structure. ]*/
/*Tests_SRS_OUTPROCESS_LOADER_17_017: [ This function shall assign the entrypoint activation_type to NONE. ]*/
/*Tests_SRS_OUTPROCESS_LOADER_17_018: [ This function shall assign the entrypoint control_id to the string value of "control.id" in json. ]*/
/*Tests_SRS_OUTPROCESS_LOADER_17_019: [ This function shall assign the entrypoint message_id to the string value of "message.id" in json, NULL if not present. ]*/
/*Tests_SRS_OUTPROCESS_LOADER_27_020: [ Launch - `OutprocessModuleLoader_ParseEntrypointFromJson` shall update the entry point with the parsed launch parameters. ]*/
/*Tests_SRS_OUTPROCESS_LOADER_17_043: [ This function shall read the "timeout" value. ]*/
/*Tests_SRS_OUTPROCESS_LOADER_17_044: [ If "timeout" is set, the remote_message_wait shall be set to this value, else it will be set to a default of 1000 ms. ]*/
/*Tests_SRS_OUTPROCESS_LOADER_17_022: [ This function shall return a valid pointer to an OUTPROCESS_LOADER_ENTRYPOINT on success. ]*/
TEST_FUNCTION(OutprocessModuleLoader_ParseEntrypointFromJson_succeeds)
{
	// arrange
	char * activation_type = "launch";
	char * control_id = "a url";
    double grace_period_ms = 500;

	STRICT_EXPECTED_CALL(json_value_get_type((JSON_Value*)0x42))
		.SetReturn(JSONObject);
	STRICT_EXPECTED_CALL(json_value_get_object((JSON_Value*)0x42))
		.SetReturn((JSON_Object*)0x43);
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "activation.type"))
		.SetReturn(activation_type);
	STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "control.id"))
		.SetReturn(control_id);
    STRICT_EXPECTED_CALL(json_object_get_object((JSON_Object*)0x43, "launch"))
        .SetReturn((JSON_Object*)0x44);
    STRICT_EXPECTED_CALL(json_object_get_string((JSON_Object*)0x43, "message.id"))
		.SetReturn(NULL);
    expected_calls_validate_launch_arguments(&grace_period_ms);
	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(OUTPROCESS_LOADER_ENTRYPOINT)));
	STRICT_EXPECTED_CALL(STRING_construct(control_id));
    expected_calls_update_entrypoint_with_launch_object();
	STRICT_EXPECTED_CALL(json_object_get_number((JSON_Object*)0x43, "timeout"))
		.SetReturn(2000);
	STRICT_EXPECTED_CALL(STRING_construct(NULL));

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
	STRICT_EXPECTED_CALL(STRING_construct(control_id));
	STRICT_EXPECTED_CALL(STRING_construct(message_id));

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

/*Tests_SRS_OUTPROCESS_LOADER_17_026: [ This function shall return NULL if entrypoint, control_id, or module_configuration is NULL. ]*/
TEST_FUNCTION(OutprocessModuleLoader_BuildModuleConfiguration_returns_null_on_null_entrypoint)
{
    // act
    void* result = OutprocessModuleLoader_BuildModuleConfiguration(NULL, NULL, (void*)0x42);

    // assert
	ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_OUTPROCESS_LOADER_17_026: [ This function shall return NULL if entrypoint, control_id, or module_configuration is NULL. ]*/
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
		STRING_construct("message_id"),
		0,
		NULL,
		0
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
	ASSERT_ARE_EQUAL(char_ptr, STRING_c_str(omc->control_uri), "ipc://control_id");
	ASSERT_ARE_EQUAL(char_ptr, STRING_c_str(omc->message_uri), "ipc://message_id");
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
		0,
		NULL,
		0
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
		0,
		NULL,
		0
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
		0,
		NULL,
		0
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
		0,
		NULL,
		0
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
		0,
		NULL,
		0
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


/* Tests_SRS_OUTPROCESS_LOADER_27_050: [ Prerequisite Check - If no threads are running, then `OutprocessLoader_JoinChildProcesses` shall abandon the effort to join the child processes immediately. ] */
TEST_FUNCTION(OutprocessLoader_JoinChildProcesses_SCENARIO_no_threads_running)
{
    // Arrange
    global_memory = true;

    // Expected call listing
    umock_c_reset_all_calls();
    EXPECTED_CALL(uv_default_loop())
        .SetReturn(MOCK_UV_LOOP);
    STRICT_EXPECTED_CALL(uv_loop_alive(MOCK_UV_LOOP))
        .SetReturn(0);

    // Act
    OutprocessLoader_JoinChildProcesses();

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, global_malloc_count);

    // Cleanup
    global_memory = false;
}


/* Tests_SRS_OUTPROCESS_LOADER_27_063: [ If no processes are running, then `OutprocessLoader_JoinChildProcesses` shall immediately join the child process management thread. ] */
TEST_FUNCTION(OutprocessLoader_JoinChildProcesses_SCENARIO_no_processes_active)
{
    // Arrange
    srand((unsigned int)time(NULL));
    global_memory = true;

    static const bool CHILDREN_HAVE_EXITED = true;
    static const double CHILDREN_EXIT_MS = 600;
    static const double GRACE_PERIOD_MS = 500;
    const size_t PROCESS_COUNT = ((rand() % 5) + 1);

    int result;
    char * process_argv[] = {
        "program.exe",
        "control.id"
    };
    OUTPROCESS_LOADER_ENTRYPOINT entrypoint = {
        OUTPROCESS_LOADER_ACTIVATION_LAUNCH,
        NULL,
        NULL,
        sizeof(process_argv),
        process_argv,
        0
    };

    // Expected call listing
    umock_c_reset_all_calls();

    for (size_t i = 0; i < PROCESS_COUNT; ++i) {
        expected_calls_validate_launch_arguments(&GRACE_PERIOD_MS);
        result = validate_launch_arguments(NULL);
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        expected_calls_launch_child_process_from_entrypoint(0 == i);
        result = launch_child_process_from_entrypoint(&entrypoint);
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        expected_calls_OutprocessLoader_SpawnChildProcesses(0 == i);
        result = OutprocessLoader_SpawnChildProcesses();
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    expected_calls_OutprocessLoader_JoinChildProcesses(PROCESS_COUNT, CHILDREN_HAVE_EXITED, (size_t)GRACE_PERIOD_MS, (size_t)CHILDREN_EXIT_MS);

    // Act
    OutprocessLoader_JoinChildProcesses();

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, global_malloc_count);

    // Cleanup
    global_memory = false;
}


/* Tests_SRS_OUTPROCESS_LOADER_27_059: [** If the child processes are not running, `OutprocessLoader_JoinChildProcesses` shall shall immediately join the child process management thread. ] */
TEST_FUNCTION(OutprocessLoader_JoinChildProcesses_SCENARIO_processes_exit_during_grace_period)
{
    // Arrange
    srand((unsigned int)time(NULL));
    global_memory = true;

    static const bool CHILDREN_STILL_ACTIVE = false;
    static const double CHILDREN_EXIT_MS = 200;
    static const double GRACE_PERIOD_MS = 500;
    const size_t PROCESS_COUNT = ((rand() % 5) + 1);

    int result;
    char * process_argv[] = {
        "program.exe",
        "control.id"
    };
    OUTPROCESS_LOADER_ENTRYPOINT entrypoint = {
        OUTPROCESS_LOADER_ACTIVATION_LAUNCH,
        NULL,
        NULL,
        sizeof(process_argv),
        process_argv,
        0
    };

    // Expected call listing
    umock_c_reset_all_calls();

    for (size_t i = 0; i < PROCESS_COUNT; ++i) {
        expected_calls_validate_launch_arguments(&GRACE_PERIOD_MS);
        result = validate_launch_arguments(NULL);
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        expected_calls_launch_child_process_from_entrypoint(0 == i);
        result = launch_child_process_from_entrypoint(&entrypoint);
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        expected_calls_OutprocessLoader_SpawnChildProcesses(0 == i);
        result = OutprocessLoader_SpawnChildProcesses();
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    expected_calls_OutprocessLoader_JoinChildProcesses(PROCESS_COUNT, CHILDREN_STILL_ACTIVE, (size_t)GRACE_PERIOD_MS, (size_t)CHILDREN_EXIT_MS);

    // Act
    OutprocessLoader_JoinChildProcesses();

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, global_malloc_count);

    // Cleanup
    global_memory = false;
}


/* Tests_SRS_OUTPROCESS_LOADER_27_064: [ `OutprocessLoader_JoinChildProcesses` shall get the count of child processes, by calling `size_t VECTOR_size(VECTOR_HANDLE handle)`. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_051: [ `OutprocessLoader_JoinChildProcesses` shall create a timer to test for timeout, by calling `TICK_COUNTER_HANDLE tickcounter_create(void)`. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_053: [ `OutprocessLoader_JoinChildProcesses` shall mark the begin time, by calling `int tickcounter_get_current_ms(TICK_COUNTER_HANDLE tick_counter, tickcounter_ms_t * current_ms)` using the handle called by the previous call to `tickcounter_create`. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_055: [ While awaiting the grace period, `OutprocessLoader_JoinChildProcesses` shall mark the current time, by calling `int tickcounter_get_current_ms(TICK_COUNTER_HANDLE tick_counter, tickcounter_ms_t * current_ms)` using the handle called by the previous call to `tickcounter_create`. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_057: [ While awaiting the grace period, `OutprocessLoader_JoinChildProcesses` shall check if the processes are running, by calling `int uv_loop_alive(const uv_loop_t * loop)` using the result of `uv_default_loop()` for `loop`. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_058: [ `OutprocessLoader_JoinChildProcesses` shall await the grace period in 100ms increments, by calling `void ThreadAPI_Sleep(unsigned int milliseconds)` passing 100 for `milliseconds`. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_060: [ If the grace period expired, `OutprocessLoader_JoinChildProcesses` shall get the handle of each child processes, by calling `void * VECTOR_element(VECTOR_HANDLE handle, size_t index)`. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_061: [ If the grace period expired, `OutprocessLoader_JoinChildProcesses` shall signal each child, by calling `int uv_process_kill(uv_process_t * process, int signum)` passing `SIGTERM` for `signum`. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_062: [ `OutprocessLoader_JoinChildProcesses` shall join the child process management thread, by calling `THREADAPI_RESULT ThreadAPI_Join(THREAD_HANDLE threadHandle, int * res)`. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_065: [ `OutprocessLoader_JoinChildProcesses` shall get the handle of each child processes, by calling `void * VECTOR_element(VECTOR_HANDLE handle, size_t index)`. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_066: [ `OutprocessLoader_JoinChildProcesses` shall free the resources allocated to each child, by calling `void free(void * _Block)` passing the child handle as `_Block`. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_067: [ `OutprocessLoader_JoinChildProcesses` shall destroy the vector of child processes, by calling `void VECTOR_destroy(VECTOR_HANDLE handle)`. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_068: [ `OutprocessLoader_JoinChildProcesses` shall destroy the timer, by calling `void tickcounter_destroy(TICK_COUNTER_HANDLE tick_counter)`. ] */
TEST_FUNCTION(OutprocessLoader_JoinChildProcesses_SCENARIO_success)
{
    // Arrange
    srand((unsigned int)time(NULL));
    global_memory = true;

    static const bool CHILDREN_STILL_ACTIVE = false;
    static const double CHILDREN_EXIT_MS = 600;
    static const double GRACE_PERIOD_MS = 500;
    const size_t PROCESS_COUNT = ((rand() % 5) + 1);

    int result;
    char * process_argv[] = {
        "program.exe",
        "control.id"
    };
    OUTPROCESS_LOADER_ENTRYPOINT entrypoint = {
        OUTPROCESS_LOADER_ACTIVATION_LAUNCH,
        NULL,
        NULL,
        sizeof(process_argv),
        process_argv,
        0
    };

    // Expected call listing
    umock_c_reset_all_calls();

    for (size_t i = 0; i < PROCESS_COUNT; ++i) {
        expected_calls_validate_launch_arguments(&GRACE_PERIOD_MS);
        result = validate_launch_arguments(NULL);
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        expected_calls_launch_child_process_from_entrypoint(0 == i);
        result = launch_child_process_from_entrypoint(&entrypoint);
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        expected_calls_OutprocessLoader_SpawnChildProcesses(0 == i);
        result = OutprocessLoader_SpawnChildProcesses();
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    expected_calls_OutprocessLoader_JoinChildProcesses(PROCESS_COUNT, CHILDREN_STILL_ACTIVE, (size_t)GRACE_PERIOD_MS, (size_t)CHILDREN_EXIT_MS);

    // Act
    OutprocessLoader_JoinChildProcesses();

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, global_malloc_count);

    // Cleanup
    global_memory = false;
}


/* Tests_SRS_OUTPROCESS_LOADER_27_052: [ If unable to create a timer, `OutprocessLoader_JoinChildProcesses` shall abandon awaiting the grace period. ] */
TEST_FUNCTION(OutprocessLoader_JoinChildProcesses_SCENARIO_unable_to_create_tickcounter)
{
    // Arrange
    srand((unsigned int)time(NULL));
    global_memory = true;

    static const double GRACE_PERIOD_MS = 500;
    static uv_process_t * MOCK_UV_PROCESS = (uv_process_t *)0x17091979;
    const TICK_COUNTER_HANDLE MOCK_TICKCOUNTER = NULL;
    const size_t PROCESS_COUNT = ((rand() % 5) + 1);

    int result;
    char * process_argv[] = {
        "program.exe",
        "control.id"
    };
    OUTPROCESS_LOADER_ENTRYPOINT entrypoint = {
        OUTPROCESS_LOADER_ACTIVATION_LAUNCH,
        NULL,
        NULL,
        sizeof(process_argv),
        process_argv,
        0
    };

    // Expected call listing
    umock_c_reset_all_calls();
    for (size_t i = 0; i < PROCESS_COUNT; ++i) {
        expected_calls_validate_launch_arguments(&GRACE_PERIOD_MS);
        result = validate_launch_arguments(NULL);
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        expected_calls_launch_child_process_from_entrypoint(0 == i);
        result = launch_child_process_from_entrypoint(&entrypoint);
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        expected_calls_OutprocessLoader_SpawnChildProcesses(0 == i);
        result = OutprocessLoader_SpawnChildProcesses();
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    STRICT_EXPECTED_CALL(VECTOR_size(MOCK_UV_PROCESS_VECTOR))
        .SetReturn(PROCESS_COUNT);
    EXPECTED_CALL(uv_default_loop())
        .SetReturn(MOCK_UV_LOOP);
    STRICT_EXPECTED_CALL(uv_loop_alive(MOCK_UV_LOOP))
        .SetReturn(__LINE__);
    EXPECTED_CALL(tickcounter_create())
        .SetReturn(MOCK_TICKCOUNTER);

    for (size_t i = 0; i < PROCESS_COUNT; ++i) {
        STRICT_EXPECTED_CALL(VECTOR_element(MOCK_UV_PROCESS_VECTOR, i))
            .SetReturn(&MOCK_UV_PROCESS);
        STRICT_EXPECTED_CALL(uv_process_kill(MOCK_UV_PROCESS, SIGTERM))
            .SetFailReturn(__LINE__)
            .SetReturn(i);
    }
    STRICT_EXPECTED_CALL(tickcounter_destroy(MOCK_TICKCOUNTER));

    STRICT_EXPECTED_CALL(ThreadAPI_Join(MOCK_THREAD_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetFailReturn(THREADAPI_ERROR)
        .SetReturn(THREADAPI_NO_MEMORY);
    for (size_t i = 0; i < PROCESS_COUNT; ++i) {
        STRICT_EXPECTED_CALL(VECTOR_element(MOCK_UV_PROCESS_VECTOR, i))
            .SetReturn(&MOCK_UV_PROCESS);
        STRICT_EXPECTED_CALL(gballoc_free(MOCK_UV_PROCESS));
    }
    STRICT_EXPECTED_CALL(VECTOR_destroy(MOCK_UV_PROCESS_VECTOR));

    // Act
    OutprocessLoader_JoinChildProcesses();

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, global_malloc_count);

    // Cleanup
    global_memory = false;
}


/* Tests_SRS_OUTPROCESS_LOADER_27_054: [** If unable to mark the begin time, `OutprocessLoader_JoinChildProcesses` shall abandon awaiting the grace period. ] */
TEST_FUNCTION(OutprocessLoader_JoinChildProcesses_SCENARIO_unable_to_mark_start_time)
{
    // Arrange
    srand((unsigned int)time(NULL));
    global_memory = true;

    static const double GRACE_PERIOD_MS = 500;
    static uv_process_t * MOCK_UV_PROCESS = (uv_process_t *)0x17091979;
    const TICK_COUNTER_HANDLE MOCK_TICKCOUNTER = (TICK_COUNTER_HANDLE)0x19790917;
    const size_t PROCESS_COUNT = ((rand() % 5) + 1);

    int result;
    tickcounter_ms_t injected_ms = 0;
    char * process_argv[] = {
        "program.exe",
        "control.id"
    };
    OUTPROCESS_LOADER_ENTRYPOINT entrypoint = {
        OUTPROCESS_LOADER_ACTIVATION_LAUNCH,
        NULL,
        NULL,
        sizeof(process_argv),
        process_argv,
        0
    };

    // Expected call listing
    umock_c_reset_all_calls();
    for (size_t i = 0; i < PROCESS_COUNT; ++i) {
        expected_calls_validate_launch_arguments(&GRACE_PERIOD_MS);
        result = validate_launch_arguments(NULL);
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        expected_calls_launch_child_process_from_entrypoint(0 == i);
        result = launch_child_process_from_entrypoint(&entrypoint);
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        expected_calls_OutprocessLoader_SpawnChildProcesses(0 == i);
        result = OutprocessLoader_SpawnChildProcesses();
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    STRICT_EXPECTED_CALL(VECTOR_size(MOCK_UV_PROCESS_VECTOR))
        .SetReturn(PROCESS_COUNT);
    EXPECTED_CALL(uv_default_loop())
        .SetReturn(MOCK_UV_LOOP);
    STRICT_EXPECTED_CALL(uv_loop_alive(MOCK_UV_LOOP))
        .SetReturn(__LINE__);
    EXPECTED_CALL(tickcounter_create())
        .SetReturn(MOCK_TICKCOUNTER);
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(MOCK_TICKCOUNTER, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &injected_ms, sizeof(tickcounter_ms_t))
        .SetReturn(__LINE__);

    for (size_t i = 0; i < PROCESS_COUNT; ++i) {
        STRICT_EXPECTED_CALL(VECTOR_element(MOCK_UV_PROCESS_VECTOR, i))
            .SetReturn(&MOCK_UV_PROCESS);
        STRICT_EXPECTED_CALL(uv_process_kill(MOCK_UV_PROCESS, SIGTERM))
            .SetFailReturn(__LINE__)
            .SetReturn(i);
    }
    STRICT_EXPECTED_CALL(tickcounter_destroy(MOCK_TICKCOUNTER));

    STRICT_EXPECTED_CALL(ThreadAPI_Join(MOCK_THREAD_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(THREADAPI_NO_MEMORY);
    for (size_t i = 0; i < PROCESS_COUNT; ++i) {
        STRICT_EXPECTED_CALL(VECTOR_element(MOCK_UV_PROCESS_VECTOR, i))
            .SetReturn(&MOCK_UV_PROCESS);
        STRICT_EXPECTED_CALL(gballoc_free(MOCK_UV_PROCESS));
    }
    STRICT_EXPECTED_CALL(VECTOR_destroy(MOCK_UV_PROCESS_VECTOR));

    // Act
    OutprocessLoader_JoinChildProcesses();

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, global_malloc_count);

    // Cleanup
    global_memory = false;
}


/* Tests_SRS_OUTPROCESS_LOADER_27_056: [ If unable to mark the current time, `OutprocessLoader_JoinChildProcesses` shall abandon awaiting the grace period. ] */
TEST_FUNCTION(OutprocessLoader_JoinChildProcesses_SCENARIO_unable_to_mark_current_time)
{
    // Arrange
    srand((unsigned int)time(NULL));
    global_memory = true;

    static const double GRACE_PERIOD_MS = 500;
    static uv_process_t * MOCK_UV_PROCESS = (uv_process_t *)0x17091979;
    const TICK_COUNTER_HANDLE MOCK_TICKCOUNTER = (TICK_COUNTER_HANDLE)0x19790917;
    const size_t PROCESS_COUNT = ((rand() % 5) + 1);

    int result;
    tickcounter_ms_t injected_ms = 0;
    char * process_argv[] = {
        "program.exe",
        "control.id"
    };
    OUTPROCESS_LOADER_ENTRYPOINT entrypoint = {
        OUTPROCESS_LOADER_ACTIVATION_LAUNCH,
        NULL,
        NULL,
        sizeof(process_argv),
        process_argv,
        0
    };

    // Expected call listing
    umock_c_reset_all_calls();
    for (size_t i = 0; i < PROCESS_COUNT; ++i) {
        expected_calls_validate_launch_arguments(&GRACE_PERIOD_MS);
        result = validate_launch_arguments(NULL);
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        expected_calls_launch_child_process_from_entrypoint(0 == i);
        result = launch_child_process_from_entrypoint(&entrypoint);
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        expected_calls_OutprocessLoader_SpawnChildProcesses(0 == i);
        result = OutprocessLoader_SpawnChildProcesses();
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    STRICT_EXPECTED_CALL(VECTOR_size(MOCK_UV_PROCESS_VECTOR))
        .SetReturn(PROCESS_COUNT);
    EXPECTED_CALL(uv_default_loop())
        .SetReturn(MOCK_UV_LOOP);
    STRICT_EXPECTED_CALL(uv_loop_alive(MOCK_UV_LOOP))
        .SetReturn(__LINE__);
    EXPECTED_CALL(tickcounter_create())
        .SetReturn(MOCK_TICKCOUNTER);
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(MOCK_TICKCOUNTER, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &injected_ms, sizeof(tickcounter_ms_t))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(MOCK_TICKCOUNTER, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &injected_ms, sizeof(tickcounter_ms_t))
        .SetReturn(__LINE__);

    for (size_t i = 0; i < PROCESS_COUNT; ++i) {
        STRICT_EXPECTED_CALL(VECTOR_element(MOCK_UV_PROCESS_VECTOR, i))
            .SetReturn(&MOCK_UV_PROCESS);
        STRICT_EXPECTED_CALL(uv_process_kill(MOCK_UV_PROCESS, SIGTERM))
            .SetFailReturn(__LINE__)
            .SetReturn(i);
    }
    STRICT_EXPECTED_CALL(tickcounter_destroy(MOCK_TICKCOUNTER));

    STRICT_EXPECTED_CALL(ThreadAPI_Join(MOCK_THREAD_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(THREADAPI_NO_MEMORY);
    for (size_t i = 0; i < PROCESS_COUNT; ++i) {
        STRICT_EXPECTED_CALL(VECTOR_element(MOCK_UV_PROCESS_VECTOR, i))
            .SetReturn(&MOCK_UV_PROCESS);
        STRICT_EXPECTED_CALL(gballoc_free(MOCK_UV_PROCESS));
    }
    STRICT_EXPECTED_CALL(VECTOR_destroy(MOCK_UV_PROCESS_VECTOR));

    // Act
    OutprocessLoader_JoinChildProcesses();

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, global_malloc_count);

    // Cleanup
    global_memory = false;
}


/* Tests_SRS_OUTPROCESS_LOADER_27_045: [ Prerequisite Check - If no processes have been enqueued, then `OutprocessLoader_SpawnChildProcesses` shall take no action and return zero. ] */
TEST_FUNCTION(OutprocessLoader_SpawnChildProcesses_SCENARIO_no_processes_scheduled)
{
    // Arrange
    int result;

    // Expected call listing
    umock_c_reset_all_calls();

    // Act
    result = OutprocessLoader_SpawnChildProcesses();

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // Cleanup
}


/* Tests_SRS_OUTPROCESS_LOADER_27_046: [ Prerequisite Check - If child processes are already running, then `OutprocessLoader_SpawnChildProcesses` shall take no action and return zero. ] */
TEST_FUNCTION(OutprocessLoader_SpawnChildProcesses_SCENARIO_thread_already_running)
{
    // Arrange
    global_memory = true;
    static const bool FIRST_CALL = true;
    static const bool SUBSEQUENT_CALL = false;

    int result;
    char * process_argv[] = {
        "program.exe",
        "control.id"
    };
    OUTPROCESS_LOADER_ENTRYPOINT entrypoint = {
        OUTPROCESS_LOADER_ACTIVATION_LAUNCH,
        NULL,
        NULL,
        sizeof(process_argv),
        process_argv,
        0
    };

    // Expected call listing
    umock_c_reset_all_calls();

    expected_calls_launch_child_process_from_entrypoint(FIRST_CALL);
    result = launch_child_process_from_entrypoint(&entrypoint);
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    expected_calls_OutprocessLoader_SpawnChildProcesses(FIRST_CALL);
    result = OutprocessLoader_SpawnChildProcesses();
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    expected_calls_OutprocessLoader_SpawnChildProcesses(SUBSEQUENT_CALL);

    // Act
    result = OutprocessLoader_SpawnChildProcesses();

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // Cleanup
    OutprocessLoader_JoinChildProcesses();
    my_gballoc_free(NULL);
    global_memory = false;
    ASSERT_ARE_EQUAL(int, 0, global_malloc_count);
}


/* Tests_SRS_OUTPROCESS_LOADER_27_047: [ `OutprocessLoader_SpawnChildProcesses` shall launch the enqueued child processes by calling `THREADAPI_RESULT ThreadAPI_Create(THREAD_HANDLE * threadHandle, THREAD_START_FUNC func, void * arg)`. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_049: [ If no errors are encountered, then `OutprocessLoader_SpawnChildProcesses` shall return zero. ] */
TEST_FUNCTION(OutprocessLoader_SpawnChildProcesses_SCENARIO_success)
{
    // Arrange
    global_memory = true;
    static const bool FIRST_CALL = true;

    int result;
    char * process_argv[] = {
        "program.exe",
        "control.id"
    };
    OUTPROCESS_LOADER_ENTRYPOINT entrypoint = {
        OUTPROCESS_LOADER_ACTIVATION_LAUNCH,
        NULL,
        NULL,
        sizeof(process_argv),
        process_argv,
        0
    };

    // Expected call listing
    umock_c_reset_all_calls();

    expected_calls_launch_child_process_from_entrypoint(FIRST_CALL);
    result = launch_child_process_from_entrypoint(&entrypoint);
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    expected_calls_OutprocessLoader_SpawnChildProcesses(FIRST_CALL);

    // Act
    result = OutprocessLoader_SpawnChildProcesses();

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // Cleanup
    OutprocessLoader_JoinChildProcesses();
    my_gballoc_free(NULL);
    global_memory = false;
    ASSERT_ARE_EQUAL(int, 0, global_malloc_count);
}

/* Tests_SRS_OUTPROCESS_LOADER_27_048: [ If launching the enqueued child processes fails, then `OutprocessLoader_SpawnChildProcesses` shall return a non-zero value. ] */
TEST_FUNCTION(OutprocessLoader_SpawnChildProcesses_SCENARIO_negative_tests)
{
    // Arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    global_memory = true;
    static const bool FIRST_CALL = true;
    int result;
    char * process_argv[] = {
        "program.exe",
        "control.id"
    };
    OUTPROCESS_LOADER_ENTRYPOINT entrypoint = {
        OUTPROCESS_LOADER_ACTIVATION_LAUNCH,
        NULL,
        NULL,
        sizeof(process_argv),
        process_argv,
        0
    };

    // Expected call listing
    umock_c_reset_all_calls();
    expected_calls_launch_child_process_from_entrypoint(FIRST_CALL);
    result = launch_child_process_from_entrypoint(&entrypoint);
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    disableNegativeTestsBeforeIndex(negative_test_index);
    expected_calls_OutprocessLoader_SpawnChildProcesses(FIRST_CALL);
    umock_c_negative_tests_snapshot();

    ASSERT_ARE_EQUAL(int, negative_test_index, umock_c_negative_tests_call_count());
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); ++i) {
        if (skipNegativeTest(i)) {
            printf("%s: Skipping negative tests: %zx\n", __FUNCTION__, i);
            continue;
        }
        printf("%s: Running negative tests: %zx\n", __FUNCTION__, i);
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // Act
        result = OutprocessLoader_SpawnChildProcesses();

        // Assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);

        // Cleanup
        OutprocessLoader_JoinChildProcesses();
        my_gballoc_free(NULL);
        global_memory = false;
        ASSERT_ARE_EQUAL(int, 0, global_malloc_count);
    }

    // Cleanup
    umock_c_negative_tests_deinit();
}


/* Tests_SRS_OUTPROCESS_LOADER_27_069: [ `launch_child_process_from_entrypoint` shall attempt to create a vector for child processes(unless previously created), by calling `VECTOR_HANDLE VECTOR_create(size_t elementSize)` using `sizeof(uv_process_t *)` as `elementSize`. ]*/
/* Tests_SRS_OUTPROCESS_LOADER_27_071: [ `launch_child_process_from_entrypoint` shall allocate the memory for the child handle, by calling `void * malloc(size_t _Size)` passing `sizeof(uv_process_t)` as `_Size`. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_073: [ `launch_child_process_from_entrypoint` shall store the child's handle, by calling `int VECTOR_push_back(VECTOR_HANDLE handle, const void * elements, size_t numElements)` passing the process vector as `handle` the pointer to the newly allocated memory for the process context as `elements` and 1 as `numElements`. ]*/
/* Tests_SRS_OUTPROCESS_LOADER_27_075: [ `launch_child_process_from_entrypoint` shall enqueue the child process to be spawned, by calling `int uv_spawn(uv_loop_t * loop, uv_process_t * handle, const uv_process_options_t * options)` passing the result of `uv_default_loop()` as `loop`, the newly allocated process handle as `handle`. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_079: [ If no errors are encountered, then `launch_child_process_from_entrypoint` shall return zero. ] */
TEST_FUNCTION(launch_child_process_from_entrypoint_SCENARIO_success)
{
    // Arrange
    global_memory = true;
    static const bool FIRST_CALL = true;

    int result;
    char * process_argv[] = {
        "program.exe",
        "control.id"
    };
    OUTPROCESS_LOADER_ENTRYPOINT entrypoint = {
        OUTPROCESS_LOADER_ACTIVATION_LAUNCH,
        NULL,
        NULL,
        sizeof(process_argv),
        process_argv,
        0
    };

    // Expected call listing
    umock_c_reset_all_calls();
    expected_calls_launch_child_process_from_entrypoint(FIRST_CALL);

    // Act
    result = launch_child_process_from_entrypoint(&entrypoint);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // Cleanup
    OutprocessLoader_JoinChildProcesses();
    my_gballoc_free(NULL);
    global_memory = false;
    ASSERT_ARE_EQUAL(int, 0, global_malloc_count);
}


/* Tests_SRS_OUTPROCESS_LOADER_27_098: [ If a vector for child processes already exists, then `launch_child_process_from_entrypoint` shall not attempt to recreate the vector. ] */
TEST_FUNCTION(launch_child_process_from_entrypoint_SCENARIO_subsequent_call)
{
    // Arrange
    global_memory = true;
    static const bool FIRST_CALL = true;
    static const bool SUBSEQUENT_CALL = false;

    int result;
    char * process_argv[] = {
        "program.exe",
        "control.id"
    };
    OUTPROCESS_LOADER_ENTRYPOINT entrypoint = {
        OUTPROCESS_LOADER_ACTIVATION_LAUNCH,
        NULL,
        NULL,
        sizeof(process_argv),
        process_argv,
        0
    };

    // Expected call listing
    umock_c_reset_all_calls();

    expected_calls_launch_child_process_from_entrypoint(FIRST_CALL);
    result = launch_child_process_from_entrypoint(&entrypoint);
    ASSERT_ARE_EQUAL(int, 0, result);

    expected_calls_launch_child_process_from_entrypoint(SUBSEQUENT_CALL);

    // Act
    result = launch_child_process_from_entrypoint(&entrypoint);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // Cleanup
    OutprocessLoader_JoinChildProcesses();
    my_gballoc_free(NULL);
    my_gballoc_free(NULL);
    global_memory = false;
    ASSERT_ARE_EQUAL(int, 0, global_malloc_count);
}


/* Tests_SRS_OUTPROCESS_LOADER_27_070: [ If a vector for the child processes does not exist, then `launch_child_process_from_entrypoint` shall return a non-zero value. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_072: [ If unable to allocate memory for the child handle, then `launch_child_process_from_entrypoint` shall return a non-zero value. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_074: [ If unable to store the child's handle, then `launch_child_process_from_entrypoint` shall free the memory allocated to the child process handle and return a non-zero value. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_076: [ If unable to enqueue the child process, then `launch_child_process_from_entrypoint` shall remove the stored handle, free the memory allocated to the child process handle and return a non-zero value. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_078: [ If launching the enqueued child processes fails, then `launch_child_process_from_entrypoint` shall return a non - zero value. ] */
TEST_FUNCTION(launch_child_process_from_entrypoint_SCENARIO_negative_tests)
{
    // Arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    global_memory = true;
    static const bool FIRST_CALL = true;

    int result;
    char * process_argv[] = {
        "program.exe",
        "control.id"
    };
    OUTPROCESS_LOADER_ENTRYPOINT entrypoint = {
        OUTPROCESS_LOADER_ACTIVATION_LAUNCH,
        NULL,
        NULL,
        sizeof(process_argv),
        process_argv,
        0
    };

    // Expected call listing
    umock_c_reset_all_calls();
    expected_calls_launch_child_process_from_entrypoint(FIRST_CALL);
    umock_c_negative_tests_snapshot();

    ASSERT_ARE_EQUAL(int, negative_test_index, umock_c_negative_tests_call_count());
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); ++i) {
        if (skipNegativeTest(i)) {
            printf("%s: Skipping negative tests: %zx\n", __FUNCTION__, i);
            continue;
        }
        printf("%s: Running negative tests: %zx\n", __FUNCTION__, i);
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // Act
        result = launch_child_process_from_entrypoint(&entrypoint);

        // Assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);

        // Cleanup
        OutprocessLoader_JoinChildProcesses();
    }

    // Cleanup
    umock_c_negative_tests_deinit();
}

/* Tests_SRS_OUTPROCESS_LOADER_27_080: [ `spawn_child_processes` shall start the child process management thread, by calling `int uv_run(uv_loop_t * loop, uv_run_mode mode)` passing the result of `uv_default_loop()` for `loop` and `UV_RUN_DEFAULT` for `mode`. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_081: [ If no errors are encountered, then `spawn_child_processes` shall return zero. ] */
TEST_FUNCTION(spawn_child_processes_SCENARIO_success)
{
    // Arrange
    int result;

    // Expected call listing
    umock_c_reset_all_calls();
    expected_calls_spawn_child_processes(false);

    // Act
    result = spawn_child_processes(NULL);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // Cleanup
}


/* Tests_SRS_OUTPROCESS_LOADER_27_099: [ `spawn_child_processes` shall return the result of the child process management thread to the parent thread, by calling `void ThreadAPI_Exit(int res)` passing the result of `uv_run()` as `res`. ] */
TEST_FUNCTION(spawn_child_processes_SCENARIO_negative_tests)
{
    // Arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    // Expected call listing
    umock_c_reset_all_calls();
    expected_calls_spawn_child_processes(true);
    umock_c_negative_tests_snapshot();

    ASSERT_ARE_EQUAL(int, negative_test_index, umock_c_negative_tests_call_count());
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); ++i) {
        if (skipNegativeTest(i)) {
            printf("%s: Skipping negative tests: %zx\n", __FUNCTION__, i);
            continue;
        }
        printf("%s: Running negative tests: %zx\n", __FUNCTION__, i);
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // Act
        spawn_child_processes(NULL);

        // Assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // Cleanup
    }

    // Cleanup
    umock_c_negative_tests_deinit();
}


/* Tests_SRS_OUTPROCESS_LOADER_27_082: [ `update_entrypoint_with_launch_object` shall retrieve the file path, by calling `const char * json_object_get_string(const JSON_Object * object, const char * name)` passing `path` as `name`. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_083: [ `update_entrypoint_with_launch_object` shall retrieve the JSON arguments array, by calling `JSON_Array json_object_get_array(const JSON_Object * object, const char * name)` passing `args` as `name`. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_084: [ `update_entrypoint_with_launch_object` shall determine the size of the JSON arguments array, by calling `size_t json_array_get_count(const JSON_Array * array)`. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_085: [ `update_entrypoint_with_launch_object` shall allocate the argument array, by calling `void * malloc(size _Size)` passing the result of `json_array_get_count` plus two, one for passing the file path as the first argument and the second for the NULL terminating pointer required by libUV. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_087: [ `update_entrypoint_with_launch_object` shall allocate the space necessary to copy the file path, by calling `void * malloc(size _Size)`. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_089: [ `update_entrypoint_with_launch_object` shall retrieve each argument from the JSON arguments array, by calling `const char * json_array_get_string(const JSON_Array * array, size_t index)`. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_090: [ `update_entrypoint_with_launch_object` shall allocate the space necessary for each argument, by calling `void * malloc(size _Size)`. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_092: [ If no errors are encountered, then `update_entrypoint_with_launch_object` shall return zero. ] */
TEST_FUNCTION(update_entrypoint_with_launch_object_SCENARIO_success)
{
    // Arrange
    int result;
    char * process_argv[] = {
        "program.exe",
        "control.id"
    };
    OUTPROCESS_LOADER_ENTRYPOINT entrypoint = {
        OUTPROCESS_LOADER_ACTIVATION_LAUNCH,
        NULL,
        NULL,
        sizeof(process_argv),
        process_argv,
        0
    };

    // Expected call listing
    umock_c_reset_all_calls();
    expected_calls_update_entrypoint_with_launch_object();

    // Act
    result = update_entrypoint_with_launch_object(&entrypoint, NULL);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // Cleanup
}


/* Tests_SRS_OUTPROCESS_LOADER_27_086: [ If unable to allocate the array, then `update_entrypoint_with_launch_object` shall return a non-zero value. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_088: [ If unable to allocate space for the file path, then `update_entrypoint_with_launch_object` shall free the argument array and return a non-zero value. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_091: [ If unable to allocate space for the argument, then `update_entrypoint_with_launch_object` shall free the argument array and return a non - zero value. ] */
TEST_FUNCTION(update_entrypoint_with_launch_object_SCENARIO_negative_tests)
{
    // Arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    int result;
    char * process_argv[] = {
        "program.exe",
        "control.id"
    };
    OUTPROCESS_LOADER_ENTRYPOINT entrypoint = {
        OUTPROCESS_LOADER_ACTIVATION_LAUNCH,
        NULL,
        NULL,
        sizeof(process_argv),
        process_argv,
        0
    };

    // Expected call listing
    umock_c_reset_all_calls();
    expected_calls_update_entrypoint_with_launch_object();
    umock_c_negative_tests_snapshot();

    ASSERT_ARE_EQUAL(int, negative_test_index, umock_c_negative_tests_call_count());
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); ++i) {
        if (skipNegativeTest(i)) {
            printf("%s: Skipping negative tests: %zx\n", __FUNCTION__, i);
            continue;
        }
        printf("%s: Running negative tests: %zx\n", __FUNCTION__, i);
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // Act
        result = update_entrypoint_with_launch_object(&entrypoint, NULL);

        // Assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);

        // Cleanup
    }

    // Cleanup
    umock_c_negative_tests_deinit();
}


/* Tests_SRS_OUTPROCESS_LOADER_27_093: [ `validate_launch_arguments` shall retrieve the file path, by calling `const char * json_object_get_string(const JSON_Object * object, const char * name)` passing `path` as `name`. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_095: [ `validate_launch_arguments` shall test for the optional parameter grace period, by calling `const char * json_object_get_string(const JSON_Object * object, const char * name)` passing `grace.period.ms` as `name`. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_096: [ `validate_launch_arguments` shall retrieve the grace period (if provided), by calling `double json_object_get_number(const JSON_Object * object, const char * name)` passing `grace.period.ms` as `name`. ] */
/* Tests_SRS_OUTPROCESS_LOADER_27_097: [ If no errors are encountered, then `validate_launch_arguments` shall return zero. ] */
TEST_FUNCTION(validate_launch_arguments_SCENARIO_success)
{
    // Arrange
    static const double GRACE_PERIOD_MS = 500;

    int result;

    // Expected call listing
    umock_c_reset_all_calls();
    expected_calls_validate_launch_arguments(&GRACE_PERIOD_MS);

    // Act
    result = validate_launch_arguments(NULL);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // Cleanup
}


/* Tests_SRS_OUTPROCESS_LOADER_27_094: [ If unable to retrieve the file path, then `validate_launch_arguments` shall return a non-zero value. ] */
TEST_FUNCTION(validate_launch_arguments_SCENARIO_negative_tests)
{
    // Arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    static const double GRACE_PERIOD_MS = 500;

    int result;

    // Expected call listing
    umock_c_reset_all_calls();
    expected_calls_validate_launch_arguments(&GRACE_PERIOD_MS);
    umock_c_negative_tests_snapshot();

    ASSERT_ARE_EQUAL(int, negative_test_index, umock_c_negative_tests_call_count());
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); ++i) {
        if (skipNegativeTest(i)) {
            printf("%s: Skipping negative tests: %zx\n", __FUNCTION__, i);
            continue;
        }
        printf("%s: Running negative tests: %zx\n", __FUNCTION__, i);
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // Act
        result = validate_launch_arguments(NULL);

        // Assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);

        // Cleanup
    }

    // Cleanup
    umock_c_negative_tests_deinit();
}

END_TEST_SUITE(OutprocessLoader_UnitTests);
