// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"

#include <cstdarg>
/*general macros useful for MOCKS*/
#define CURRENT_API_CALL(API) C2(C2(current, API), _call)
#define WHEN_SHALL_API_FAIL(API) C2(C2(whenShall, API), _fail)
#define DEFINE_FAIL_VARIABLES(API) static size_t CURRENT_API_CALL(API); static size_t WHEN_SHALL_API_FAIL(API);
#define MAKE_FAIL(API, WHEN) do{WHEN_SHALL_API_FAIL(API) = CURRENT_API_CALL(API) + WHEN;} while(0)
#define RESET_API_COUNTERS(API) CURRENT_API_CALL(API) = WHEN_SHALL_API_FAIL(API) = 0;

#define LIST_OF_COUNTED_APIS      \
gb_fprintf                        \


FOR_EACH_1(DEFINE_FAIL_VARIABLES, LIST_OF_COUNTED_APIS)

#include "azure_c_shared_utility/lock.h"
#include "module.h"
#include "module_access.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/constmap.h"
#include "azure_c_shared_utility/map.h"
#include "message.h"
#include "azure_c_shared_utility/base64.h"
#include "logger.h"

#include <parson.h>

#ifndef GB_STDIO_INTERCEPT
#error these unit tests require the symbol GB_STDIO_INTERCEPT to be defined
#else
extern "C"
{
    extern FILE* gb_fopen(const char* fileName, const char* mode);
    extern int gb_fclose(FILE *stream);
    extern int gb_fseek(FILE *stream, long int offset, int whence);
    extern long int gb_ftell(FILE *stream);
    extern int gb_fprintf(FILE * stream, const char * format, ...); /*this needs poor man mocks because of ...*/
}
#endif
extern "C" int mallocAndStrcpy_s(char** destination, const char*source);

static char all_fprintfs[100][10000];
/*poor man mock*/
int gb_fprintf(FILE * stream, const char * format, ...) /*cannot be static*/
{
    CURRENT_API_CALL(gb_fprintf)++;
    
    va_list args;
    va_start(args, format);
    (void)vsnprintf(all_fprintfs[CURRENT_API_CALL(gb_fprintf)-1], sizeof(all_fprintfs[0]) / sizeof(all_fprintfs[0][0]), format, args); /*no test here requires this to be 10000... */
    va_end(args);

     int result2 = (WHEN_SHALL_API_FAIL(gb_fprintf) == CURRENT_API_CALL(gb_fprintf)) ? -1 : __LINE__;

    return result2;
}


#ifndef GB_TIME_INTERCEPT
#error these unit tests require the symbol GB_TIME_INTERCEPT to be defined
#else
extern "C"
{
    extern time_t gb_time(time_t *timer);
    extern struct tm* gb_localtime(const time_t *timer);
    extern size_t gb_strftime(char * s, size_t maxsize, const char * format, const struct tm * timeptr); 
}
#endif

#define GBALLOC_H
extern "C" int gballoc_init(void);
extern "C" void gballoc_deinit(void);
extern "C" void* gballoc_malloc(size_t size);
extern "C" void* gballoc_calloc(size_t nmemb, size_t size);
extern "C" void* gballoc_realloc(void* ptr, size_t size);
extern "C" void gballoc_free(void* ptr);

namespace BASEIMPLEMENTATION
{

#define Lock(x) (LOCK_OK + gballocState - gballocState) /*compiler warning about constant in if condition*/
#define Unlock(x) (LOCK_OK + gballocState - gballocState)
#define Lock_Init() (LOCK_HANDLE)0x42
#define Lock_Deinit(x) (LOCK_OK + gballocState - gballocState)
#include "gballoc.c"
#undef Lock
#undef Unlock
#undef Lock_Init
#undef Lock_Deinit

    typedef int JSON_Value_Type;

    typedef union json_value_value {
        char        *string;
        double       number;
        JSON_Object *object;
        JSON_Array  *array;
        int          boolean;
        int          null;
    } JSON_Value_Value;

    struct json_value_t {
        JSON_Value_Type     type;
        JSON_Value_Value    value;
    };

    struct json_object_t {
        char       **names;
        JSON_Value **values;
        size_t       count;
        size_t       capacity;
    };

    typedef struct json_value_t  JSON_Value;
    typedef struct json_object_t JSON_Object;

    JSON_Value* json_parse_string(const char* string);
    JSON_Object* json_value_get_object(const JSON_Value* value);
    const char* json_object_get_string(const JSON_Object* object, const char* name);
    void json_value_free(JSON_Value *value);

};

typedef struct LOGGER_HANDLE_DATA_TAG
{
    FILE* fout;
}LOGGER_HANDLE_DATA;

static MICROMOCK_MUTEX_HANDLE g_testByTest;
static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;


/*these are simple cached variables*/
static pfModule_ParseConfigurationFromJson Logger_ParseConfigurationFromJson = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_FreeConfiguration Logger_FreeConfiguration = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Create  Logger_Create = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Destroy Logger_Destroy = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Receive Logger_Receive = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/

static LOGGER_CONFIG validConfig =
{
	LOGGING_TO_FILE,
	"a.txt"
};
static BROKER_HANDLE validBrokerHandle = (BROKER_HANDLE)0x1;

static LOGGER_CONFIG invalidConfig_fileName =
{
    (LOGGER_TYPE)~LOGGING_TO_FILE,
    "a.txt"
};

static LOGGER_CONFIG invalidConfig_selector =
{
    LOGGING_TO_FILE,
    NULL
};

#define VALID_CONFIG_STRING "{\"filename\":\"log.txt\"}"

#define TIME_IN_STRFTIME "time"
static MESSAGE_HANDLE validMessageHandle = (MESSAGE_HANDLE)0x032;
static unsigned char buffer[3] = { 1,2,3 };
static CONSTBUFFER validBuffer = { buffer, sizeof(buffer)/sizeof(buffer[0]) };


TYPED_MOCK_CLASS(CLoggerMocks, CGlobalMock)
{
public:
    //parson
    MOCK_STATIC_METHOD_1(, JSON_Value*, json_parse_string, const char *, filename)
        JSON_Value* value = NULL;
        if (filename != NULL)
        {
            value = (JSON_Value*)malloc(sizeof(BASEIMPLEMENTATION::JSON_Value));
        }
    MOCK_METHOD_END(JSON_Value*, value);

    MOCK_STATIC_METHOD_1(, JSON_Object*, json_value_get_object, const JSON_Value*, value)
        JSON_Object* object = NULL;
        if (value != NULL)
        {
            object = (JSON_Object*)0x42;
        }
    MOCK_METHOD_END(JSON_Object*, object);

    MOCK_STATIC_METHOD_2(, const char*, json_object_get_string, const JSON_Object*, object, const char*, name)
    MOCK_METHOD_END(const char*, (strcmp(name, "filename") == 0) ? "log.txt" : NULL);

    MOCK_STATIC_METHOD_1(, void, json_value_free, JSON_Value*, value)
        free(value);
    MOCK_VOID_METHOD_END();

    //memory
    MOCK_STATIC_METHOD_1(, void*, gballoc_malloc, size_t, size)
        void* result2 = BASEIMPLEMENTATION::gballoc_malloc(size);
    MOCK_METHOD_END(void*, result2);

    MOCK_STATIC_METHOD_1(, void, gballoc_free, void*, ptr)
        BASEIMPLEMENTATION::gballoc_free(ptr);
    MOCK_VOID_METHOD_END()

	// crt_abstractions.h
	MOCK_STATIC_METHOD_2(, int, mallocAndStrcpy_s, char**, destination, const char*, source)
	int r;
	if (source == NULL)
	{
		*destination = NULL;
		r = 1;
	}
	else
	{
		*destination = (char*)BASEIMPLEMENTATION::gballoc_malloc(strlen(source) + 1);
		strcpy(*destination, source);
		r = 0;
	}
	MOCK_METHOD_END(int, r)

    //string
    MOCK_STATIC_METHOD_1(, STRING_HANDLE, STRING_construct, const char*, source)
        STRING_HANDLE result2 = (STRING_HANDLE)malloc(4);
    MOCK_METHOD_END(STRING_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, void, STRING_delete, STRING_HANDLE, s)
        free(s);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_2(, int, STRING_concat, STRING_HANDLE, s1, const char*, s2)
    MOCK_METHOD_END(int, 0);

    MOCK_STATIC_METHOD_2(, int, STRING_concat_with_STRING, STRING_HANDLE, s1, STRING_HANDLE, s2)
    MOCK_METHOD_END(int, 0);

    MOCK_STATIC_METHOD_1(, const char*, STRING_c_str, STRING_HANDLE, s)
    MOCK_METHOD_END(const char*, "thisIsRandomContent")

    MOCK_STATIC_METHOD_1(, MAP_HANDLE, ConstMap_CloneWriteable, CONSTMAP_HANDLE, handle)
        MAP_HANDLE result2 = (MAP_HANDLE)malloc(5);
    MOCK_METHOD_END(MAP_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, void, Map_Destroy, MAP_HANDLE, ptr)
       free(ptr);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, STRING_HANDLE, Map_ToJSON, MAP_HANDLE, handle)
        STRING_HANDLE result2 = (STRING_HANDLE)malloc(6);
    MOCK_METHOD_END(STRING_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, CONSTMAP_HANDLE, Message_GetProperties, MESSAGE_HANDLE, message)
        CONSTMAP_HANDLE result2 = (CONSTMAP_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1);
    MOCK_METHOD_END(CONSTMAP_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, void, ConstMap_Destroy, CONSTMAP_HANDLE, handle)
        free(handle);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, const CONSTBUFFER *, Message_GetContent, MESSAGE_HANDLE, message)
        const CONSTBUFFER * result2 = &validBuffer;
    MOCK_METHOD_END(const CONSTBUFFER *, result2)

    MOCK_STATIC_METHOD_2(, STRING_HANDLE, Base64_Encode_Bytes, const unsigned char*, source, size_t, size);
        STRING_HANDLE result2 = (STRING_HANDLE)malloc(7);
    MOCK_METHOD_END(STRING_HANDLE, result2);

    MOCK_STATIC_METHOD_2(, FILE*, gb_fopen, const char*, filename, const char*, mode)
        FILE* result2 = (FILE*)malloc(8);
    MOCK_METHOD_END(FILE*, result2);
    
    MOCK_STATIC_METHOD_1(, int, gb_fclose, FILE*, file)
        free(file);
        int result2 = 0;
    MOCK_METHOD_END(int, result2);

    MOCK_STATIC_METHOD_3(, int, gb_fseek, FILE *, stream, long int, offset, int, whence)
        int result2 = 0;
    MOCK_METHOD_END(int, result2);

    MOCK_STATIC_METHOD_1(, long int, gb_ftell, FILE *, stream)
        long int result2 = 0;
    MOCK_METHOD_END(long int, result2);

    MOCK_STATIC_METHOD_1(, time_t, gb_time, time_t *, timer)
        time_t result2 = (time_t)1; /*assume "1" is valid time_t*/
    MOCK_METHOD_END(time_t, result2);
    
    MOCK_STATIC_METHOD_1(, struct tm*, gb_localtime, const time_t *, timer)
        struct tm* result2 = (struct tm*)0x42;
    MOCK_METHOD_END(struct tm*, result2);

    MOCK_STATIC_METHOD_4(, size_t, gb_strftime, char*, s, size_t, maxsize, const char *, format, const struct tm *, timeptr)
        if (maxsize < strlen(TIME_IN_STRFTIME)+1)
        {
            ASSERT_FAIL("what is this puny message size!");
        }
        else
        {
            strcpy(s, TIME_IN_STRFTIME);
        }
    MOCK_METHOD_END(size_t, maxsize);
};

DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , JSON_Value*, json_parse_string, const char *, filename);
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , JSON_Object*, json_value_get_object, const JSON_Value*, value);
DECLARE_GLOBAL_MOCK_METHOD_2(CLoggerMocks, , const char*, json_object_get_string, const JSON_Object*, object, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , void, json_value_free, JSON_Value*, value);

DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , void, gballoc_free, void*, ptr);
DECLARE_GLOBAL_MOCK_METHOD_2(CLoggerMocks, , int, mallocAndStrcpy_s, char**, destination, const char*, source);

DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , STRING_HANDLE, STRING_construct, const char*, s);

DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , void, STRING_delete, STRING_HANDLE, s);
DECLARE_GLOBAL_MOCK_METHOD_2(CLoggerMocks, , int, STRING_concat, STRING_HANDLE, s1, const char*, s2);
DECLARE_GLOBAL_MOCK_METHOD_2(CLoggerMocks, , int, STRING_concat_with_STRING, STRING_HANDLE, s1, STRING_HANDLE, s2);
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , const char*, STRING_c_str, STRING_HANDLE, s);

DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , MAP_HANDLE, ConstMap_CloneWriteable, CONSTMAP_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , STRING_HANDLE, Map_ToJSON, MAP_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , void, Map_Destroy, MAP_HANDLE, ptr);

DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , CONSTMAP_HANDLE, Message_GetProperties, MESSAGE_HANDLE, message);
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , void,  ConstMap_Destroy, CONSTMAP_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , const CONSTBUFFER *, Message_GetContent, MESSAGE_HANDLE, message);

DECLARE_GLOBAL_MOCK_METHOD_2(CLoggerMocks, , STRING_HANDLE, Base64_Encode_Bytes, const unsigned char*, source, size_t, size);

DECLARE_GLOBAL_MOCK_METHOD_2(CLoggerMocks, , FILE*, gb_fopen, const char*, filename, const char*, mode);
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , int, gb_fclose, FILE*, stream);
DECLARE_GLOBAL_MOCK_METHOD_3(CLoggerMocks, , int, gb_fseek, FILE *, stream, long int, offset, int, whence)
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , long int, gb_ftell, FILE*, stream)

DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , time_t, gb_time, time_t*, timer);
DECLARE_GLOBAL_MOCK_METHOD_1(CLoggerMocks, , struct tm*, gb_localtime, const time_t*, timer);
DECLARE_GLOBAL_MOCK_METHOD_4(CLoggerMocks, , size_t, gb_strftime, char*, s, size_t, maxsize, const char *, format, const struct tm *, timeptr);


static void mocks_ResetAllCounters(void)
{
    FOR_EACH_1(RESET_API_COUNTERS, LIST_OF_COUNTED_APIS);
}

BEGIN_TEST_SUITE(logger_ut)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
        g_testByTest = MicroMockCreateMutex();
        ASSERT_IS_NOT_NULL(g_testByTest);

        const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
        Logger_ParseConfigurationFromJson = MODULE_PARSE_CONFIGURATION_FROM_JSON(apis);
		Logger_FreeConfiguration = MODULE_FREE_CONFIGURATION(apis);
        Logger_Create = MODULE_CREATE(apis);
        Logger_Destroy = MODULE_DESTROY(apis);
        Logger_Receive = MODULE_RECEIVE(apis);
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        MicroMockDestroyMutex(g_testByTest);
        TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
    }

    TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
    {
        if (!MicroMockAcquireMutex(g_testByTest))
        {
            ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
        }

        mocks_ResetAllCounters();

    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
        if (!MicroMockReleaseMutex(g_testByTest))
        {
            ASSERT_FAIL("failure in test framework at ReleaseMutex");
        }
    }


    /*Tests_SRS_LOGGER_05_003: [ If configuration is NULL then Logger_ParseConfigurationFromJson shall fail and return NULL. ]*/
    TEST_FUNCTION(Logger_ParseConfigurationFromJson_with_NULL_configuration_fails)
    {
        ///arrange
        CLoggerMocks mocks;

        ///act
        auto result = Logger_ParseConfigurationFromJson(NULL);

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_LOGGER_17_001: [ Logger_ParseConfigurationFromJson shall allocate a new LOGGER_CONFIG structure. ]*/
    /*Tests_SRS_LOGGER_17_002: [ Logger_ParseConfigurationFromJson shall duplicate the filename string into the LOGGER_CONFIG structure. ]*/
    /*Tests_SRS_LOGGER_17_006: [ Logger_ParseConfigurationFromJson shall return a pointer to the created LOGGER_CONFIG structure. ]*/
    /*Tests_SRS_LOGGER_17_007: [ Logger_ParseConfigurationFromJson shall set the selector in LOGGER_CONFIG to LOGGING_TO_FILE. ]*/
    TEST_FUNCTION(Logger_ParseConfigurationFromJson_happy_path_succeeds)
    {
        ///arrange
        CLoggerMocks mocks;

        STRICT_EXPECTED_CALL(mocks, json_parse_string(VALID_CONFIG_STRING)); /*this is creating the JSON from the string*/
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG)) /*this is destroy of the json value created from the string*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG)) /*getting the json object out of the json value*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "filename")) /*this is getting a json string that is what follows "filename": in the json*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(LOGGER_CONFIG)));
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.IgnoreArgument(2);

        ///act
        auto result = Logger_ParseConfigurationFromJson(VALID_CONFIG_STRING);

        ///assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(int, (int)((LOGGER_CONFIG*)result)->selector, (int)LOGGING_TO_FILE);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Logger_FreeConfiguration(result);
    }

    /*Tests_SRS_LOGGER_17_003: [ If any system call fails, Logger_ParseConfigurationFromJson shall fail and return NULL. ]*/
	TEST_FUNCTION(Logger_ParseConfigurationFromJson_string_copy_fails)
	{
		///arrange
		CLoggerMocks mocks;

		STRICT_EXPECTED_CALL(mocks, json_parse_string(VALID_CONFIG_STRING)); /*this is creating the JSON from the string*/
		STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG)) /*this is destroy of the json value created from the string*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG)) /*getting the json object out of the json value*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "filename")) /*this is getting a json string that is what follows "filename": in the json*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(LOGGER_CONFIG)));
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.IgnoreArgument(2)
			.SetFailReturn(1);

		///act
		auto result = Logger_ParseConfigurationFromJson(VALID_CONFIG_STRING);

		///assert
		ASSERT_IS_NULL(result);
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}

    /*Tests_SRS_LOGGER_17_003: [ If any system call fails, Logger_ParseConfigurationFromJson shall fail and return NULL. ]*/
	TEST_FUNCTION(Logger_ParseConfigurationFromJson_config_alloc_fails)
	{
		///arrange
		CLoggerMocks mocks;

		STRICT_EXPECTED_CALL(mocks, json_parse_string(VALID_CONFIG_STRING)); /*this is creating the JSON from the string*/
		STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG)) /*this is destroy of the json value created from the string*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG)) /*getting the json object out of the json value*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "filename")) /*this is getting a json string that is what follows "filename": in the json*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(LOGGER_CONFIG)))
			.SetFailReturn(nullptr);

		///act
		auto result = Logger_ParseConfigurationFromJson(VALID_CONFIG_STRING);

		///assert
		ASSERT_IS_NULL(result);
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}
    
    /*Tests_SRS_LOGGER_05_012: [ If the JSON object does not contain a value named "filename" then Logger_ParseConfigurationFromJson shall fail and return NULL. ]*/
    TEST_FUNCTION(Logger_ParseConfigurationFromJson_fails_when_json_object_get_string_fails)
    {
        ///arrange
        CLoggerMocks mocks;

        STRICT_EXPECTED_CALL(mocks, json_parse_string(VALID_CONFIG_STRING)); /*this is creating the JSON from the string*/
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG)) /*this is destroy of the json value created from the string*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG)) /*getting the json object out of the json value*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "filename")) /*this is getting a json string that is what follows "filename": in the json*/
            .IgnoreArgument(1)
            .SetFailReturn((const char*)NULL);

        ///act
        auto result = Logger_ParseConfigurationFromJson(VALID_CONFIG_STRING);

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_LOGGER_05_012: [ If the JSON object does not contain a value named "filename" then Logger_ParseConfigurationFromJson shall fail and return NULL. ]*/
    TEST_FUNCTION(Logger_ParseConfigurationFromJson_fails_when_json_value_get_object_fails)
    {
        ///arrange
        CLoggerMocks mocks;

        STRICT_EXPECTED_CALL(mocks, json_parse_string(VALID_CONFIG_STRING)); /*this is creating the JSON from the string*/
        STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG)) /*this is destroy of the json value created from the string*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG)) /*getting the json object out of the json value*/
            .IgnoreArgument(1)
            .SetFailReturn((JSON_Object*)NULL);

        ///act
        auto result = Logger_ParseConfigurationFromJson( VALID_CONFIG_STRING);

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_LOGGER_05_011: [ If configuration is not a JSON object, then Logger_ParseConfigurationFromJson shall fail and return NULL. ]*/
    TEST_FUNCTION(Logger_ParseConfigurationFromJson_fails_when_json_parse_string_fails)
    {
        ///arrange
        CLoggerMocks mocks;

        STRICT_EXPECTED_CALL(mocks, json_parse_string(VALID_CONFIG_STRING)) /*this is creating the JSON from the string*/
            .SetFailReturn((JSON_Value*)NULL);

        ///act
        auto result = Logger_ParseConfigurationFromJson( VALID_CONFIG_STRING);

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_LOGGER_17_005: [ Logger_FreeConfiguration shall free all resources created by Logger_ParseConfigurationFromJson. ]*/
	TEST_FUNCTION(Logger_FreeConfiguration_happy_path_succeeds)
	{
		///arrange
		CLoggerMocks mocks;

		STRICT_EXPECTED_CALL(mocks, json_parse_string(VALID_CONFIG_STRING)); /*this is creating the JSON from the string*/
		STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG)) /*this is destroy of the json value created from the string*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG)) /*getting the json object out of the json value*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "filename")) /*this is getting a json string that is what follows "filename": in the json*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(LOGGER_CONFIG)));
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.IgnoreArgument(2);
		auto result = Logger_ParseConfigurationFromJson(VALID_CONFIG_STRING);
		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(result));

		///act
		Logger_FreeConfiguration(result);

		///assert
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}

    /*Tests_SRS_LOGGER_17_004: [ Logger_FreeConfiguration shall do nothing is configuration is NULL. ]*/
	TEST_FUNCTION(Logger_FreeConfiguration_does_nothing_with_null)
	{
		///arrange
		CLoggerMocks mocks;

		///act
		Logger_FreeConfiguration(NULL);

		///assert
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}

    /*Tests_SRS_LOGGER_02_001: [If broker is NULL then Logger_Create shall fail and return NULL.]*/
    TEST_FUNCTION(Logger_Create_with_NULL_broker_fails)
    {
        ///arrange
        CLoggerMocks mocks;

        ///act
        auto result = Logger_Create(NULL, &validConfig);

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_LOGGER_02_002: [If configuration is NULL then Logger_Create shall fail and return NULL.]*/
    TEST_FUNCTION(Logger_Create_with_NULL_config_fails)
    {
        ///arrange
        CLoggerMocks mocks;

        ///act
        auto result = Logger_Create(validBrokerHandle, NULL);

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_LOGGER_02_003: [If configuration->selector has a value different than LOGGING_TO_FILE then Logger_Create shall fail and return NULL.]*/
    TEST_FUNCTION(Logger_Create_with_invalid_selector_fails)
    {
        ///arrange
        CLoggerMocks mocks;

        ///act
        auto result = Logger_Create(validBrokerHandle, &invalidConfig_selector);

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_LOGGER_02_004: [If configuration->selectee.loggerConfigFile.name is NULL then Logger_Create shall fail and return NULL.]*/
    TEST_FUNCTION(Logger_Create_with_invalid_fileName_fails)
    {
        ///arrange
        CLoggerMocks mocks;

        ///act
        auto result = Logger_Create(validBrokerHandle, &invalidConfig_fileName);

        ///assert
        ASSERT_IS_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_LOGGER_02_005: [Logger_Create shall allocate memory for the below structure.]*/
    /*Tests_SRS_LOGGER_02_006: [Logger_Create shall open the file configuration the filename selectee.loggerConfigFile.name in APPEND mode and assign the result of fopen to fout field. ]*/
    /*Tests_SRS_LOGGER_02_017: [Logger_Create shall add the following JSON value to the existing array of JSON values in the file:]*/
    /*Tests_SRS_LOGGER_02_018: [If the file does not contain a JSON array, then it shall create it.]*/
    /*Tests_SRS_LOGGER_02_008: [Otherwise Logger_Create shall return a non-NULL pointer.]*/
    TEST_FUNCTION(Logger_Create_happy_path_non_existent_file_succeeds)
    {
        ///arrange
        CLoggerMocks mocks;

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is the handle*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_fopen(validConfig.selectee.loggerConfigFile.name, "r+b")) /*this is opening the file, and because it doesn't exist, it fails*/
            .SetFailReturn((FILE*)NULL);

        STRICT_EXPECTED_CALL(mocks, gb_fopen(validConfig.selectee.loggerConfigFile.name, "w+b")) /*this is opening the file with create*/;

        STRICT_EXPECTED_CALL(mocks, gb_fseek(IGNORED_PTR_ARG, 0, SEEK_END)) /*this is going to the end of the file*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_ftell(IGNORED_PTR_ARG)) /*this is getting the file size*/
            .IgnoreArgument(1)
            .SetReturn(0); /*zero filesize*/

        /*here a call to fprintf happens ("[]"), it is captured by a weak verification in ASSERT*/

        STRICT_EXPECTED_CALL(mocks, gb_time(NULL)); /*this is getting the time*/

        STRICT_EXPECTED_CALL(mocks, gb_localtime(IGNORED_PTR_ARG)) /*this is transforming the time from time_t to struct tm* */
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_strftime(IGNORED_PTR_ARG, IGNORED_NUM_ARG, "{\"time\":\"%c\",\"content\":\"Log started\"}]", IGNORED_PTR_ARG)) /*this is building a JSON object in timetemp*/
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(4);

        STRICT_EXPECTED_CALL(mocks, gb_fseek(IGNORED_PTR_ARG, -1, SEEK_END)) /*after getting the STRING that is the beginning of the log, it needs to be appended to the file, this essenially eats the "]" at the end*/
            .IgnoreArgument(1); 

        /*here a call to fprintf happens({
    "time":"timeAsPrinted by ctime",
    "content": "Log started"
}," 
        it is captured by a weak verification in ASSERT*/
        
        ///act
        auto handle = Logger_Create(validBrokerHandle, &validConfig);

        ///assert
        ASSERT_IS_NOT_NULL(handle);
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 2, CURRENT_API_CALL(gb_fprintf));
        ASSERT_ARE_EQUAL(char_ptr, "[]", all_fprintfs[0]);
        ASSERT_ARE_EQUAL(char_ptr, TIME_IN_STRFTIME , all_fprintfs[1]);

        ///cleanup
        Logger_Destroy(handle);

    }

    /*Tests_SRS_LOGGER_02_007: [If Logger_Create encounters any errors while creating the LOGGER_HANDLE_DATA then it shall fail and return NULL.]*/
    TEST_FUNCTION(Logger_Create_noneexisting_file_fails_when_fprintf_fails_1)
    {
        ///arrange
        CLoggerMocks mocks;

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is the handle*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_fopen(validConfig.selectee.loggerConfigFile.name, "r+b")) /*this is opening the file, and because it doesn't exist, it fails*/
            .SetFailReturn((FILE*)NULL);

        STRICT_EXPECTED_CALL(mocks, gb_fopen(validConfig.selectee.loggerConfigFile.name, "w+b")) /*this is opening the file with create*/;

        STRICT_EXPECTED_CALL(mocks, gb_fclose(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_fseek(IGNORED_PTR_ARG, 0, SEEK_END)) /*this is going to the end of the file*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_ftell(IGNORED_PTR_ARG)) /*this is getting the file size*/
            .IgnoreArgument(1)
            .SetReturn(0); /*zero filesize*/

        /*here a call to fprintf happens ("[]"), it is captured by a weak verification in ASSERT*/

        STRICT_EXPECTED_CALL(mocks, gb_time(NULL)); /*this is getting the time*/

        STRICT_EXPECTED_CALL(mocks, gb_localtime(IGNORED_PTR_ARG)) /*this is transforming the time from time_t to struct tm* */
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_strftime(IGNORED_PTR_ARG, IGNORED_NUM_ARG, "{\"time\":\"%c\",\"content\":\"Log started\"}]", IGNORED_PTR_ARG)) /*this is building a JSON object in timetemp*/
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(4);

        STRICT_EXPECTED_CALL(mocks, gb_fseek(IGNORED_PTR_ARG, -1, SEEK_END)) /*after getting the STRING that is the beginning of the log, it needs to be appended to the file, this essenially eats the "]" at the end*/
            .IgnoreArgument(1);

        MAKE_FAIL(gb_fprintf, 2);
        /*here a call to fprintf happens({
        "time":"timeAsPrinted by ctime",
        "content": "Log started"
        },"
        it is captured by a weak verification in ASSERT*/

        ///act
        auto handle = Logger_Create(validBrokerHandle, &validConfig);

        ///assert
        ASSERT_IS_NULL(handle);
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 2, CURRENT_API_CALL(gb_fprintf));
        ASSERT_ARE_EQUAL(char_ptr, "[]", all_fprintfs[0]);
        ASSERT_ARE_EQUAL(char_ptr, TIME_IN_STRFTIME, all_fprintfs[1]);

        ///cleanup
        Logger_Destroy(handle);

    }

    /*Tests_SRS_LOGGER_02_007: [If Logger_Create encounters any errors while creating the LOGGER_HANDLE_DATA then it shall fail and return NULL.]*/
    TEST_FUNCTION(Logger_Create_noneexisting_file_fails_when_fseek_fails_1)
    {
        ///arrange
        CLoggerMocks mocks;

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is the handle*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_fopen(validConfig.selectee.loggerConfigFile.name, "r+b")) /*this is opening the file, and because it doesn't exist, it fails*/
            .SetFailReturn((FILE*)NULL);

        STRICT_EXPECTED_CALL(mocks, gb_fopen(validConfig.selectee.loggerConfigFile.name, "w+b")) /*this is opening the file with create*/;

        STRICT_EXPECTED_CALL(mocks, gb_fclose(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_fseek(IGNORED_PTR_ARG, 0, SEEK_END)) /*this is going to the end of the file*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_ftell(IGNORED_PTR_ARG)) /*this is getting the file size*/
            .IgnoreArgument(1)
            .SetReturn(0); /*zero filesize*/

                           /*here a call to fprintf happens ("[]"), it is captured by a weak verification in ASSERT*/

        STRICT_EXPECTED_CALL(mocks, gb_time(NULL)); /*this is getting the time*/

        STRICT_EXPECTED_CALL(mocks, gb_localtime(IGNORED_PTR_ARG)) /*this is transforming the time from time_t to struct tm* */
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_strftime(IGNORED_PTR_ARG, IGNORED_NUM_ARG, "{\"time\":\"%c\",\"content\":\"Log started\"}]", IGNORED_PTR_ARG)) /*this is building a JSON object in timetemp*/
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(4);

        STRICT_EXPECTED_CALL(mocks, gb_fseek(IGNORED_PTR_ARG, -1, SEEK_END)) /*after getting the STRING that is the beginning of the log, it needs to be appended to the file, this essenially eats the "]" at the end*/
            .IgnoreArgument(1)
            .SetReturn(__LINE__); /*The fseek function returns nonzero only for a request that cannot be satisfied.*/

        ///act
        auto handle = Logger_Create(validBrokerHandle, &validConfig);

        ///assert
        ASSERT_IS_NULL(handle);
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 1, CURRENT_API_CALL(gb_fprintf));
        ASSERT_ARE_EQUAL(char_ptr, "[]", all_fprintfs[0]);

        ///cleanup
        Logger_Destroy(handle);

    }

    /*Tests_SRS_LOGGER_02_007: [If Logger_Create encounters any errors while creating the LOGGER_HANDLE_DATA then it shall fail and return NULL.]*/
    TEST_FUNCTION(Logger_Create_noneexisting_file_fails_when_strftime_fails)
    {
        ///arrange
        CLoggerMocks mocks;

		const char * filename = "a.txt";

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is the handle*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_fopen(filename, "r+b")) /*this is opening the file, and because it doesn't exist, it fails*/
            .SetFailReturn((FILE*)NULL);

        STRICT_EXPECTED_CALL(mocks, gb_fopen(filename, "w+b")) /*this is opening the file with create*/;

        STRICT_EXPECTED_CALL(mocks, gb_fclose(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_fseek(IGNORED_PTR_ARG, 0, SEEK_END)) /*this is going to the end of the file*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_ftell(IGNORED_PTR_ARG)) /*this is getting the file size*/
            .IgnoreArgument(1)
            .SetReturn(0); /*zero filesize*/

        /*here a call to fprintf happens ("[]"), it is captured by a weak verification in ASSERT*/

        STRICT_EXPECTED_CALL(mocks, gb_time(NULL)); /*this is getting the time*/

        STRICT_EXPECTED_CALL(mocks, gb_localtime(IGNORED_PTR_ARG)) /*this is transforming the time from time_t to struct tm* */
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_strftime(IGNORED_PTR_ARG, IGNORED_NUM_ARG, "{\"time\":\"%c\",\"content\":\"Log started\"}]", IGNORED_PTR_ARG)) /*this is building a JSON object in timetemp*/
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(4)
            .SetReturn(0);

        ///act
        auto handle = Logger_Create(validBrokerHandle, &validConfig);

        ///assert
        ASSERT_IS_NULL(handle);
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 1, CURRENT_API_CALL(gb_fprintf));
        ASSERT_ARE_EQUAL(char_ptr, "[]", all_fprintfs[0]);

        ///cleanup
        Logger_Destroy(handle);

    }

    /*Tests_SRS_LOGGER_02_007: [If Logger_Create encounters any errors while creating the LOGGER_HANDLE_DATA then it shall fail and return NULL.]*/
    TEST_FUNCTION(Logger_Create_noneexisting_file_fails_when_localtime_fails)
    {
        ///arrange
        CLoggerMocks mocks;

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is the handle*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_fopen(validConfig.selectee.loggerConfigFile.name, "r+b")) /*this is opening the file, and because it doesn't exist, it fails*/
            .SetFailReturn((FILE*)NULL);

        STRICT_EXPECTED_CALL(mocks, gb_fopen(validConfig.selectee.loggerConfigFile.name, "w+b")) /*this is opening the file with create*/;

        STRICT_EXPECTED_CALL(mocks, gb_fclose(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_fseek(IGNORED_PTR_ARG, 0, SEEK_END)) /*this is going to the end of the file*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_ftell(IGNORED_PTR_ARG)) /*this is getting the file size*/
            .IgnoreArgument(1)
            .SetReturn(0); /*zero filesize*/

        /*here a call to fprintf happens ("[]"), it is captured by a weak verification in ASSERT*/

        STRICT_EXPECTED_CALL(mocks, gb_time(NULL)); /*this is getting the time*/

        STRICT_EXPECTED_CALL(mocks, gb_localtime(IGNORED_PTR_ARG)) /*this is transforming the time from time_t to struct tm* */
            .IgnoreArgument(1)
            .SetReturn((struct tm*)NULL); /*null pointer if the specified time cannot be converted to local time.*/

        ///act
        auto handle = Logger_Create(validBrokerHandle, &validConfig);

        ///assert
        ASSERT_IS_NULL(handle);
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 1, CURRENT_API_CALL(gb_fprintf));
        ASSERT_ARE_EQUAL(char_ptr, "[]", all_fprintfs[0]);

        ///cleanup
        Logger_Destroy(handle);

    }

    /*Tests_SRS_LOGGER_02_007: [If Logger_Create encounters any errors while creating the LOGGER_HANDLE_DATA then it shall fail and return NULL.]*/
    TEST_FUNCTION(Logger_Create_noneexisting_file_fails_when_time_fails)
    {
        ///arrange
        CLoggerMocks mocks;

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is the handle*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_fopen(validConfig.selectee.loggerConfigFile.name, "r+b")) /*this is opening the file, and because it doesn't exist, it fails*/
            .SetFailReturn((FILE*)NULL);

        STRICT_EXPECTED_CALL(mocks, gb_fopen(validConfig.selectee.loggerConfigFile.name, "w+b")) /*this is opening the file with create*/;

        STRICT_EXPECTED_CALL(mocks, gb_fclose(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_fseek(IGNORED_PTR_ARG, 0, SEEK_END)) /*this is going to the end of the file*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_ftell(IGNORED_PTR_ARG)) /*this is getting the file size*/
            .IgnoreArgument(1)
            .SetReturn(0); /*zero filesize*/

        /*here a call to fprintf happens ("[]"), it is captured by a weak verification in ASSERT*/

        STRICT_EXPECTED_CALL(mocks, gb_time(NULL)) /*this is getting the time*/
            .SetReturn((time_t)-1);

        ///act
        auto handle = Logger_Create(validBrokerHandle, &validConfig);

        ///assert
        ASSERT_IS_NULL(handle);
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 1, CURRENT_API_CALL(gb_fprintf));
        ASSERT_ARE_EQUAL(char_ptr, "[]", all_fprintfs[0]);

        ///cleanup
        Logger_Destroy(handle);

    }

    /*Tests_SRS_LOGGER_02_007: [If Logger_Create encounters any errors while creating the LOGGER_HANDLE_DATA then it shall fail and return NULL.]*/
    TEST_FUNCTION(Logger_Create_noneexisting_file_fails_when_fprintf_fails_2)
    {
        ///arrange
        CLoggerMocks mocks;

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is the handle*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_fopen(validConfig.selectee.loggerConfigFile.name, "r+b")) /*this is opening the file, and because it doesn't exist, it fails*/
            .SetFailReturn((FILE*)NULL);

        STRICT_EXPECTED_CALL(mocks, gb_fopen(validConfig.selectee.loggerConfigFile.name, "w+b")) /*this is opening the file with create*/;

        STRICT_EXPECTED_CALL(mocks, gb_fclose(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_fseek(IGNORED_PTR_ARG, 0, SEEK_END)) /*this is going to the end of the file*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_ftell(IGNORED_PTR_ARG)) /*this is getting the file size*/
            .IgnoreArgument(1)
            .SetReturn(0); /*zero filesize*/

        MAKE_FAIL(gb_fprintf, 1);
        /*here a call to fprintf happens ("[]"), it is captured by a weak verification in ASSERT*/

        ///act
        auto handle = Logger_Create(validBrokerHandle, &validConfig);

        ///assert
        ASSERT_IS_NULL(handle);
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 1, CURRENT_API_CALL(gb_fprintf));
        ASSERT_ARE_EQUAL(char_ptr, "[]", all_fprintfs[0]);

        ///cleanup
        Logger_Destroy(handle);

    }

    /*Tests_SRS_LOGGER_02_007: [If Logger_Create encounters any errors while creating the LOGGER_HANDLE_DATA then it shall fail and return NULL.]*/
    TEST_FUNCTION(Logger_Create_noneexisting_file_fails_when_ftell_fails)
    {
        ///arrange
        CLoggerMocks mocks;


        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is the handle*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_fopen(validConfig.selectee.loggerConfigFile.name, "r+b")) /*this is opening the file, and because it doesn't exist, it fails*/
            .SetFailReturn((FILE*)NULL);

        STRICT_EXPECTED_CALL(mocks, gb_fopen(validConfig.selectee.loggerConfigFile.name, "w+b")) /*this is opening the file with create*/;

        STRICT_EXPECTED_CALL(mocks, gb_fclose(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_fseek(IGNORED_PTR_ARG, 0, SEEK_END)) /*this is going to the end of the file*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_ftell(IGNORED_PTR_ARG)) /*this is getting the file size*/
            .IgnoreArgument(1)
            .SetReturn(-1L);

        ///act
        auto handle = Logger_Create(validBrokerHandle, &validConfig);

        ///assert
        ASSERT_IS_NULL(handle);
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 0, CURRENT_API_CALL(gb_fprintf));

        ///cleanup
        Logger_Destroy(handle);

    }

    /*Tests_SRS_LOGGER_02_007: [If Logger_Create encounters any errors while creating the LOGGER_HANDLE_DATA then it shall fail and return NULL.]*/
    TEST_FUNCTION(Logger_Create_noneexisting_file_fails_when_fseek_fails_2)
    {
        ///arrange
        CLoggerMocks mocks;


        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is the handle*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_fopen(validConfig.selectee.loggerConfigFile.name, "r+b")) /*this is opening the file, and because it doesn't exist, it fails*/
            .SetFailReturn((FILE*)NULL);

        STRICT_EXPECTED_CALL(mocks, gb_fopen(validConfig.selectee.loggerConfigFile.name, "w+b")) /*this is opening the file with create*/;

        STRICT_EXPECTED_CALL(mocks, gb_fclose(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_fseek(IGNORED_PTR_ARG, 0, SEEK_END)) /*this is going to the end of the file*/
            .IgnoreArgument(1)
            .SetReturn(~0);/*The fseek function returns nonzero only for a request that cannot be satisfied*/

        ///act
        auto handle = Logger_Create(validBrokerHandle, &validConfig);

        ///assert
        ASSERT_IS_NULL(handle);
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 0, CURRENT_API_CALL(gb_fprintf));

        ///cleanup
        Logger_Destroy(handle);

    }

    /*Tests_SRS_LOGGER_02_007: [If Logger_Create encounters any errors while creating the LOGGER_HANDLE_DATA then it shall fail and return NULL.]*/
    /*Tests_SRS_LOGGER_02_020: [If the file selectee.loggerConfigFile.name does not exist, it shall be created.]*/
    /*Tests_SRS_LOGGER_02_021: [If creating selectee.loggerConfigFile.name fails then Logger_Create shall fail and return NULL.]*/
    TEST_FUNCTION(Logger_Create_noneexisting_file_fails_when_fopen_fails)
    {
        ///arrange
        CLoggerMocks mocks;

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is the handle*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_fopen(validConfig.selectee.loggerConfigFile.name, "r+b")) /*this is opening the file*/
            .SetFailReturn((FILE*)NULL);

        STRICT_EXPECTED_CALL(mocks, gb_fopen(validConfig.selectee.loggerConfigFile.name, "w+b")) /*this is trying to create the file*/
            .SetFailReturn((FILE*)NULL);

        ///act
        auto handle = Logger_Create(validBrokerHandle, &validConfig);

        ///assert
        ASSERT_IS_NULL(handle);
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 0, CURRENT_API_CALL(gb_fprintf));

        ///cleanup
        Logger_Destroy(handle);

    }

    /*Tests_SRS_LOGGER_02_007: [If Logger_Create encounters any errors while creating the LOGGER_HANDLE_DATA then it shall fail and return NULL.]*/
    TEST_FUNCTION(Logger_Create_noneexisting_file_fails_when_malloc_fails)
    {
        ///arrange
        CLoggerMocks mocks;

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is the handle*/
            .IgnoreArgument(1)
            .SetFailReturn((void*)NULL);

        ///act
        auto handle = Logger_Create(validBrokerHandle, &validConfig);

        ///assert
        ASSERT_IS_NULL(handle);
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 0, CURRENT_API_CALL(gb_fprintf));

        ///cleanup
        Logger_Destroy(handle);

    }

    /*Tests_SRS_LOGGER_02_005: [Logger_Create shall allocate memory for the below structure.]*/
    /*Tests_SRS_LOGGER_02_006: [Logger_Create shall open the file configuration the filename selectee.loggerConfigFile.name in APPEND mode and assign the result of fopen to fout field. ]*/
    /*Tests_SRS_LOGGER_02_017: [Logger_Create shall add the following JSON value to the existing array of JSON values in the file:]*/
    TEST_FUNCTION(Logger_Create_happy_path_existing_file)
    {
        ///arrange
        CLoggerMocks mocks;

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is the handle*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gb_fopen(validConfig.selectee.loggerConfigFile.name, "r+b")); /*this is opening the file*/

        STRICT_EXPECTED_CALL(mocks, gb_fseek(IGNORED_PTR_ARG, 0, SEEK_END)) /*this is going to the end of the file*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_ftell(IGNORED_PTR_ARG)) /*this is getting the file size*/
            .IgnoreArgument(1)
            .SetReturn(2); /*non-zero filesize*/

        STRICT_EXPECTED_CALL(mocks, gb_time(NULL)); /*this is getting the time*/

        STRICT_EXPECTED_CALL(mocks, gb_localtime(IGNORED_PTR_ARG)) /*this is transforming the time from time_t to struct tm* */
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_strftime(IGNORED_PTR_ARG, IGNORED_NUM_ARG, ",{\"time\":\"%c\",\"content\":\"Log started\"}]", IGNORED_PTR_ARG)) /*this is building a JSON object in timetemp*/
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(4);

        STRICT_EXPECTED_CALL(mocks, gb_fseek(IGNORED_PTR_ARG, -1, SEEK_END)) /*after getting the STRING that is the beginning of the log, it needs to be appended to the file, this essenially eats the "]" at the end*/
            .IgnoreArgument(1);

        /*here a call to fprintf happens({
        "time":"timeAsPrinted by ctime",
        "content": "Log started"
        }]"
        it is captured by a weak verification in ASSERT*/

        ///act
        auto handle = Logger_Create(validBrokerHandle, &validConfig);

        ///assert
        ASSERT_IS_NOT_NULL(handle);
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 1, CURRENT_API_CALL(gb_fprintf));
        ASSERT_ARE_EQUAL(char_ptr, TIME_IN_STRFTIME, all_fprintfs[0]);

        ///cleanup
        Logger_Destroy(handle);

    }

    /*Tests_SRS_LOGGER_02_009: [If moduleHandle is NULL then Logger_Receive shall fail and return.]*/
    TEST_FUNCTION(Logger_Receive_with_NULL_modulehandle_fails)
    {
        ///arrange
        CLoggerMocks mocks;

        ///act
        Logger_Receive(NULL, validMessageHandle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup

    }

    /*Tests_SRS_LOGGER_02_010: [If messageHandle is NULL then Logger_Receive shall fail and return.]*/
    TEST_FUNCTION(Logger_Receive_with_NULL_messageHandle_fails)
    {
        ///arrange
        CLoggerMocks mocks;
        auto moduleHandle = Logger_Create(validBrokerHandle, &validConfig);
        mocks.ResetAllCalls();

        ///act
        Logger_Receive(moduleHandle, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Logger_Destroy(moduleHandle);

    }

    /*Tests_SRS_LOGGER_02_011: [Logger_Receive shall write in the fout FILE the following information in JSON format:]*/
    /*Tests_SRS_LOGGER_02_013: [Logger_Receive shall return.]*/
    TEST_FUNCTION(Logger_Receive_happy_path)
    {
        ///arrange
        CLoggerMocks mocks;
        auto moduleHandle = Logger_Create(validBrokerHandle, &validConfig);
        mocks.ResetAllCalls();
        mocks_ResetAllCounters();

        STRICT_EXPECTED_CALL(mocks, gb_time(NULL)); /*this is getting the time*/

        STRICT_EXPECTED_CALL(mocks, gb_localtime(IGNORED_PTR_ARG)) /*this is transforming the time from time_t to struct tm* */
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_strftime(IGNORED_PTR_ARG, IGNORED_NUM_ARG, "%c", IGNORED_PTR_ARG)) /*this is building a JSON object in timetemp*/
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(4);

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(validMessageHandle)); /*this is getting the properties from the message*/
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)) /*this is getting the properties in a writeable map, because ConstMap doesn't have ToJSON*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Map_ToJSON(IGNORED_PTR_ARG)) /*this is getting a STRING_HANDLE that is the MAP as JSON*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Message_GetContent(validMessageHandle)); /*this is getting the content*/

        STRICT_EXPECTED_CALL(mocks, Base64_Encode_Bytes(buffer, sizeof(buffer) / sizeof(buffer[0]))); /*this is getting a STRING_HANDLE that is the base64 encode of the bytes*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(",{\"time\":\"")); /*this is the actual JSON object building*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is adding the real time to the json*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, "\",\"properties\":")) /*this is adding the "properties":" string*/
            .IgnoreArgument(1); 
        STRICT_EXPECTED_CALL(mocks, STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is adding the result of MapToJSON*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, ",\"content\":\"")) /*this is adding the ,"content":"*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is adding the result of base64_encode*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, "\"}]")) /*this closes the JSON*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG)) /*this is harvesting the const char* of the json string*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_fseek(IGNORED_PTR_ARG, -1, SEEK_END)) /*this is rewinding the file by 1 character*/
            .IgnoreArgument(1);

        /*here a call to fprintf happens({
        "time":"timeAsPrinted by ctime",
        "content": "Log started"
        },"
        it is captured by a weak verification in ASSERT*/

        ///act
        Logger_Receive(moduleHandle, validMessageHandle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 1, CURRENT_API_CALL(gb_fprintf));

        ///cleanup
        Logger_Destroy(moduleHandle);

    }

    /*Tests_SRS_LOGGER_02_012: [If producing the JSON format or writing it to the file fails, then Logger_Receive shall fail and return.]*/
    TEST_FUNCTION(Logger_Receive_fails_when_fprintf_fails)
    {
        ///arrange
        CLoggerMocks mocks;
        auto moduleHandle = Logger_Create(validBrokerHandle, &validConfig);
        mocks.ResetAllCalls();
        mocks_ResetAllCounters();

        STRICT_EXPECTED_CALL(mocks, gb_time(NULL)); /*this is getting the time*/

        STRICT_EXPECTED_CALL(mocks, gb_localtime(IGNORED_PTR_ARG)) /*this is transforming the time from time_t to struct tm* */
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_strftime(IGNORED_PTR_ARG, IGNORED_NUM_ARG, "%c", IGNORED_PTR_ARG)) /*this is building a JSON object in timetemp*/
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(4);

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(validMessageHandle)); /*this is getting the properties from the message*/
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)) /*this is getting the properties in a writeable map, because ConstMap doesn't have ToJSON*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Map_ToJSON(IGNORED_PTR_ARG)) /*this is getting a STRING_HANDLE that is the MAP as JSON*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Message_GetContent(validMessageHandle)); /*this is getting the content*/

        STRICT_EXPECTED_CALL(mocks, Base64_Encode_Bytes(buffer, sizeof(buffer) / sizeof(buffer[0]))); /*this is getting a STRING_HANDLE that is the base64 encode of the bytes*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(",{\"time\":\"")); /*this is the actual JSON object building*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is adding the real time to the json*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, "\",\"properties\":")) /*this is adding the "properties":" string*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is adding the result of MapToJSON*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, ",\"content\":\"")) /*this is adding the ,"content":"*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is adding the result of base64_encode*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, "\"}]")) /*this closes the JSON*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG)) /*this is harvesting the const char* of the json string*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_fseek(IGNORED_PTR_ARG, -1, SEEK_END)) /*this is rewinding the file by 1 character*/
            .IgnoreArgument(1);

        MAKE_FAIL(gb_fprintf, 1);
        /*here a call to fprintf happens({
        "time":"timeAsPrinted by ctime",
        "content": "Log started"
        },"
        it is captured by a weak verification in ASSERT*/

        ///act
        Logger_Receive(moduleHandle, validMessageHandle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 1, CURRENT_API_CALL(gb_fprintf));

        ///cleanup
        Logger_Destroy(moduleHandle);

    }

    /*Tests_SRS_LOGGER_02_012: [If producing the JSON format or writing it to the file fails, then Logger_Receive shall fail and return.]*/
    TEST_FUNCTION(Logger_Receive_fails_when_fseek_fails)
    {
        ///arrange
        CLoggerMocks mocks;
        auto moduleHandle = Logger_Create(validBrokerHandle, &validConfig);
        mocks.ResetAllCalls();
        mocks_ResetAllCounters();

        STRICT_EXPECTED_CALL(mocks, gb_time(NULL)); /*this is getting the time*/

        STRICT_EXPECTED_CALL(mocks, gb_localtime(IGNORED_PTR_ARG)) /*this is transforming the time from time_t to struct tm* */
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_strftime(IGNORED_PTR_ARG, IGNORED_NUM_ARG, "%c", IGNORED_PTR_ARG)) /*this is building a JSON object in timetemp*/
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(4);

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(validMessageHandle)); /*this is getting the properties from the message*/
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)) /*this is getting the properties in a writeable map, because ConstMap doesn't have ToJSON*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Map_ToJSON(IGNORED_PTR_ARG)) /*this is getting a STRING_HANDLE that is the MAP as JSON*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Message_GetContent(validMessageHandle)); /*this is getting the content*/

        STRICT_EXPECTED_CALL(mocks, Base64_Encode_Bytes(buffer, sizeof(buffer) / sizeof(buffer[0]))); /*this is getting a STRING_HANDLE that is the base64 encode of the bytes*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(",{\"time\":\"")); /*this is the actual JSON object building*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is adding the real time to the json*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, "\",\"properties\":")) /*this is adding the "properties":" string*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is adding the result of MapToJSON*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, ",\"content\":\"")) /*this is adding the ,"content":"*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is adding the result of base64_encode*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, "\"}]")) /*this closes the JSON*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG)) /*this is harvesting the const char* of the json string*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_fseek(IGNORED_PTR_ARG, -1, SEEK_END)) /*this is rewinding the file by 1 character*/
            .IgnoreArgument(1)
            .SetReturn(__LINE__);

        ///act
        Logger_Receive(moduleHandle, validMessageHandle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 0, CURRENT_API_CALL(gb_fprintf));

        ///cleanup
        Logger_Destroy(moduleHandle);

    }

    /*Tests_SRS_LOGGER_02_012: [If producing the JSON format or writing it to the file fails, then Logger_Receive shall fail and return.]*/
    TEST_FUNCTION(Logger_Receive_fails_when_STRING_concat_fails_1)
    {
        ///arrange
        CLoggerMocks mocks;
        auto moduleHandle = Logger_Create(validBrokerHandle, &validConfig);
        mocks.ResetAllCalls();
        mocks_ResetAllCounters();

        STRICT_EXPECTED_CALL(mocks, gb_time(NULL)); /*this is getting the time*/

        STRICT_EXPECTED_CALL(mocks, gb_localtime(IGNORED_PTR_ARG)) /*this is transforming the time from time_t to struct tm* */
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_strftime(IGNORED_PTR_ARG, IGNORED_NUM_ARG, "%c", IGNORED_PTR_ARG)) /*this is building a JSON object in timetemp*/
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(4);

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(validMessageHandle)); /*this is getting the properties from the message*/
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)) /*this is getting the properties in a writeable map, because ConstMap doesn't have ToJSON*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Map_ToJSON(IGNORED_PTR_ARG)) /*this is getting a STRING_HANDLE that is the MAP as JSON*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Message_GetContent(validMessageHandle)); /*this is getting the content*/

        STRICT_EXPECTED_CALL(mocks, Base64_Encode_Bytes(buffer, sizeof(buffer) / sizeof(buffer[0]))); /*this is getting a STRING_HANDLE that is the base64 encode of the bytes*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(",{\"time\":\"")); /*this is the actual JSON object building*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is adding the real time to the json*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, "\",\"properties\":")) /*this is adding the "properties":" string*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is adding the result of MapToJSON*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, ",\"content\":\"")) /*this is adding the ,"content":"*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is adding the result of base64_encode*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, "\"}]")) /*this closes the JSON*/
            .IgnoreArgument(1)
            .SetFailReturn(__LINE__);

        ///act
        Logger_Receive(moduleHandle, validMessageHandle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 0, CURRENT_API_CALL(gb_fprintf));

        ///cleanup
        Logger_Destroy(moduleHandle);

    }

    /*Tests_SRS_LOGGER_02_012: [If producing the JSON format or writing it to the file fails, then Logger_Receive shall fail and return.]*/
    TEST_FUNCTION(Logger_Receive_fails_when_STRING_concat_with_STRING_fails_1)
    {
        ///arrange
        CLoggerMocks mocks;
        auto moduleHandle = Logger_Create(validBrokerHandle, &validConfig);
        mocks.ResetAllCalls();
        mocks_ResetAllCounters();

        STRICT_EXPECTED_CALL(mocks, gb_time(NULL)); /*this is getting the time*/

        STRICT_EXPECTED_CALL(mocks, gb_localtime(IGNORED_PTR_ARG)) /*this is transforming the time from time_t to struct tm* */
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_strftime(IGNORED_PTR_ARG, IGNORED_NUM_ARG, "%c", IGNORED_PTR_ARG)) /*this is building a JSON object in timetemp*/
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(4);

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(validMessageHandle)); /*this is getting the properties from the message*/
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)) /*this is getting the properties in a writeable map, because ConstMap doesn't have ToJSON*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Map_ToJSON(IGNORED_PTR_ARG)) /*this is getting a STRING_HANDLE that is the MAP as JSON*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Message_GetContent(validMessageHandle)); /*this is getting the content*/

        STRICT_EXPECTED_CALL(mocks, Base64_Encode_Bytes(buffer, sizeof(buffer) / sizeof(buffer[0]))); /*this is getting a STRING_HANDLE that is the base64 encode of the bytes*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(",{\"time\":\"")); /*this is the actual JSON object building*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is adding the real time to the json*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, "\",\"properties\":")) /*this is adding the "properties":" string*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is adding the result of MapToJSON*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, ",\"content\":\"")) /*this is adding the ,"content":"*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is adding the result of base64_encode*/
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .SetFailReturn(__LINE__);

        ///act
        Logger_Receive(moduleHandle, validMessageHandle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 0, CURRENT_API_CALL(gb_fprintf));

        ///cleanup
        Logger_Destroy(moduleHandle);

    }

    /*Tests_SRS_LOGGER_02_012: [If producing the JSON format or writing it to the file fails, then Logger_Receive shall fail and return.]*/
    TEST_FUNCTION(Logger_Receive_fails_when_STRING_concat_fails_2)
    {
        ///arrange
        CLoggerMocks mocks;
        auto moduleHandle = Logger_Create(validBrokerHandle, &validConfig);
        mocks.ResetAllCalls();
        mocks_ResetAllCounters();

        STRICT_EXPECTED_CALL(mocks, gb_time(NULL)); /*this is getting the time*/

        STRICT_EXPECTED_CALL(mocks, gb_localtime(IGNORED_PTR_ARG)) /*this is transforming the time from time_t to struct tm* */
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_strftime(IGNORED_PTR_ARG, IGNORED_NUM_ARG, "%c", IGNORED_PTR_ARG)) /*this is building a JSON object in timetemp*/
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(4);

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(validMessageHandle)); /*this is getting the properties from the message*/
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)) /*this is getting the properties in a writeable map, because ConstMap doesn't have ToJSON*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Map_ToJSON(IGNORED_PTR_ARG)) /*this is getting a STRING_HANDLE that is the MAP as JSON*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Message_GetContent(validMessageHandle)); /*this is getting the content*/

        STRICT_EXPECTED_CALL(mocks, Base64_Encode_Bytes(buffer, sizeof(buffer) / sizeof(buffer[0]))); /*this is getting a STRING_HANDLE that is the base64 encode of the bytes*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(",{\"time\":\"")); /*this is the actual JSON object building*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is adding the real time to the json*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, "\",\"properties\":")) /*this is adding the "properties":" string*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is adding the result of MapToJSON*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, ",\"content\":\"")) /*this is adding the ,"content":"*/
            .IgnoreArgument(1)
            .SetFailReturn(__LINE__);

        ///act
        Logger_Receive(moduleHandle, validMessageHandle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 0, CURRENT_API_CALL(gb_fprintf));

        ///cleanup
        Logger_Destroy(moduleHandle);

    }

    /*Tests_SRS_LOGGER_02_012: [If producing the JSON format or writing it to the file fails, then Logger_Receive shall fail and return.]*/
    TEST_FUNCTION(Logger_Receive_fails_when_STRING_concat_with_STRING_fails_2)
    {
        ///arrange
        CLoggerMocks mocks;
        auto moduleHandle = Logger_Create(validBrokerHandle, &validConfig);
        mocks.ResetAllCalls();
        mocks_ResetAllCounters();

        STRICT_EXPECTED_CALL(mocks, gb_time(NULL)); /*this is getting the time*/

        STRICT_EXPECTED_CALL(mocks, gb_localtime(IGNORED_PTR_ARG)) /*this is transforming the time from time_t to struct tm* */
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_strftime(IGNORED_PTR_ARG, IGNORED_NUM_ARG, "%c", IGNORED_PTR_ARG)) /*this is building a JSON object in timetemp*/
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(4);

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(validMessageHandle)); /*this is getting the properties from the message*/
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)) /*this is getting the properties in a writeable map, because ConstMap doesn't have ToJSON*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Map_ToJSON(IGNORED_PTR_ARG)) /*this is getting a STRING_HANDLE that is the MAP as JSON*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Message_GetContent(validMessageHandle)); /*this is getting the content*/

        STRICT_EXPECTED_CALL(mocks, Base64_Encode_Bytes(buffer, sizeof(buffer) / sizeof(buffer[0]))); /*this is getting a STRING_HANDLE that is the base64 encode of the bytes*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(",{\"time\":\"")); /*this is the actual JSON object building*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is adding the real time to the json*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, "\",\"properties\":")) /*this is adding the "properties":" string*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is adding the result of MapToJSON*/
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .SetFailReturn(__LINE__);

        ///act
        Logger_Receive(moduleHandle, validMessageHandle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 0, CURRENT_API_CALL(gb_fprintf));

        ///cleanup
        Logger_Destroy(moduleHandle);

    }

    /*Tests_SRS_LOGGER_02_012: [If producing the JSON format or writing it to the file fails, then Logger_Receive shall fail and return.]*/
    TEST_FUNCTION(Logger_Receive_fails_when_STRING_concat_fails_3)
    {
        ///arrange
        CLoggerMocks mocks;
        auto moduleHandle = Logger_Create(validBrokerHandle, &validConfig);
        mocks.ResetAllCalls();
        mocks_ResetAllCounters();

        STRICT_EXPECTED_CALL(mocks, gb_time(NULL)); /*this is getting the time*/

        STRICT_EXPECTED_CALL(mocks, gb_localtime(IGNORED_PTR_ARG)) /*this is transforming the time from time_t to struct tm* */
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_strftime(IGNORED_PTR_ARG, IGNORED_NUM_ARG, "%c", IGNORED_PTR_ARG)) /*this is building a JSON object in timetemp*/
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(4);

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(validMessageHandle)); /*this is getting the properties from the message*/
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)) /*this is getting the properties in a writeable map, because ConstMap doesn't have ToJSON*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Map_ToJSON(IGNORED_PTR_ARG)) /*this is getting a STRING_HANDLE that is the MAP as JSON*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Message_GetContent(validMessageHandle)); /*this is getting the content*/

        STRICT_EXPECTED_CALL(mocks, Base64_Encode_Bytes(buffer, sizeof(buffer) / sizeof(buffer[0]))); /*this is getting a STRING_HANDLE that is the base64 encode of the bytes*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(",{\"time\":\"")); /*this is the actual JSON object building*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is adding the real time to the json*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, "\",\"properties\":")) /*this is adding the "properties":" string*/
            .IgnoreArgument(1)
            .SetFailReturn(__LINE__);

        ///act
        Logger_Receive(moduleHandle, validMessageHandle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 0, CURRENT_API_CALL(gb_fprintf));

        ///cleanup
        Logger_Destroy(moduleHandle);

    }

    /*Tests_SRS_LOGGER_02_012: [If producing the JSON format or writing it to the file fails, then Logger_Receive shall fail and return.]*/
    TEST_FUNCTION(Logger_Receive_fails_when_STRING_concat_fails_4)
    {
        ///arrange
        CLoggerMocks mocks;
        auto moduleHandle = Logger_Create(validBrokerHandle, &validConfig);
        mocks.ResetAllCalls();
        mocks_ResetAllCounters();

        STRICT_EXPECTED_CALL(mocks, gb_time(NULL)); /*this is getting the time*/

        STRICT_EXPECTED_CALL(mocks, gb_localtime(IGNORED_PTR_ARG)) /*this is transforming the time from time_t to struct tm* */
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_strftime(IGNORED_PTR_ARG, IGNORED_NUM_ARG, "%c", IGNORED_PTR_ARG)) /*this is building a JSON object in timetemp*/
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(4);

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(validMessageHandle)); /*this is getting the properties from the message*/
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)) /*this is getting the properties in a writeable map, because ConstMap doesn't have ToJSON*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Map_ToJSON(IGNORED_PTR_ARG)) /*this is getting a STRING_HANDLE that is the MAP as JSON*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Message_GetContent(validMessageHandle)); /*this is getting the content*/

        STRICT_EXPECTED_CALL(mocks, Base64_Encode_Bytes(buffer, sizeof(buffer) / sizeof(buffer[0]))); /*this is getting a STRING_HANDLE that is the base64 encode of the bytes*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(",{\"time\":\"")); /*this is the actual JSON object building*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is adding the real time to the json*/
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .SetFailReturn(__LINE__);

        ///act
        Logger_Receive(moduleHandle, validMessageHandle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 0, CURRENT_API_CALL(gb_fprintf));

        ///cleanup
        Logger_Destroy(moduleHandle);

    }

    /*Tests_SRS_LOGGER_02_012: [If producing the JSON format or writing it to the file fails, then Logger_Receive shall fail and return.]*/
    TEST_FUNCTION(Logger_Receive_fails_when_STRING_construct_fails)
    {
        ///arrange
        CLoggerMocks mocks;
        auto moduleHandle = Logger_Create(validBrokerHandle, &validConfig);
        mocks.ResetAllCalls();
        mocks_ResetAllCounters();

        STRICT_EXPECTED_CALL(mocks, gb_time(NULL)); /*this is getting the time*/

        STRICT_EXPECTED_CALL(mocks, gb_localtime(IGNORED_PTR_ARG)) /*this is transforming the time from time_t to struct tm* */
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_strftime(IGNORED_PTR_ARG, IGNORED_NUM_ARG, "%c", IGNORED_PTR_ARG)) /*this is building a JSON object in timetemp*/
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(4);

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(validMessageHandle)); /*this is getting the properties from the message*/
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)) /*this is getting the properties in a writeable map, because ConstMap doesn't have ToJSON*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Map_ToJSON(IGNORED_PTR_ARG)) /*this is getting a STRING_HANDLE that is the MAP as JSON*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Message_GetContent(validMessageHandle)); /*this is getting the content*/

        STRICT_EXPECTED_CALL(mocks, Base64_Encode_Bytes(buffer, sizeof(buffer) / sizeof(buffer[0]))); /*this is getting a STRING_HANDLE that is the base64 encode of the bytes*/
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(",{\"time\":\"")) /*this is the actual JSON object building*/
            .SetFailReturn((STRING_HANDLE)NULL);
        
        ///act
        Logger_Receive(moduleHandle, validMessageHandle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 0, CURRENT_API_CALL(gb_fprintf));

        ///cleanup
        Logger_Destroy(moduleHandle);

    }

    /*Tests_SRS_LOGGER_02_012: [If producing the JSON format or writing it to the file fails, then Logger_Receive shall fail and return.]*/
    TEST_FUNCTION(Logger_Receive_fails_when_Base64_Encode_Bytes_fails)
    {
        ///arrange
        CLoggerMocks mocks;
        auto moduleHandle = Logger_Create(validBrokerHandle, &validConfig);
        mocks.ResetAllCalls();
        mocks_ResetAllCounters();

        STRICT_EXPECTED_CALL(mocks, gb_time(NULL)); /*this is getting the time*/

        STRICT_EXPECTED_CALL(mocks, gb_localtime(IGNORED_PTR_ARG)) /*this is transforming the time from time_t to struct tm* */
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_strftime(IGNORED_PTR_ARG, IGNORED_NUM_ARG, "%c", IGNORED_PTR_ARG)) /*this is building a JSON object in timetemp*/
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(4);

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(validMessageHandle)); /*this is getting the properties from the message*/
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)) /*this is getting the properties in a writeable map, because ConstMap doesn't have ToJSON*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Map_ToJSON(IGNORED_PTR_ARG)) /*this is getting a STRING_HANDLE that is the MAP as JSON*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Message_GetContent(validMessageHandle)); /*this is getting the content*/

        STRICT_EXPECTED_CALL(mocks, Base64_Encode_Bytes(buffer, sizeof(buffer) / sizeof(buffer[0]))) /*this is getting a STRING_HANDLE that is the base64 encode of the bytes*/
            .SetFailReturn((STRING_HANDLE)NULL);
                                                                        
        ///act
        Logger_Receive(moduleHandle, validMessageHandle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 0, CURRENT_API_CALL(gb_fprintf));

        ///cleanup
        Logger_Destroy(moduleHandle);

    }

    /*Tests_SRS_LOGGER_02_012: [If producing the JSON format or writing it to the file fails, then Logger_Receive shall fail and return.]*/
    TEST_FUNCTION(Logger_Receive_fails_when_Map_ToJSON_fails)
    {
        ///arrange
        CLoggerMocks mocks;
        auto moduleHandle = Logger_Create(validBrokerHandle, &validConfig);
        mocks.ResetAllCalls();
        mocks_ResetAllCounters();

        STRICT_EXPECTED_CALL(mocks, gb_time(NULL)); /*this is getting the time*/

        STRICT_EXPECTED_CALL(mocks, gb_localtime(IGNORED_PTR_ARG)) /*this is transforming the time from time_t to struct tm* */
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_strftime(IGNORED_PTR_ARG, IGNORED_NUM_ARG, "%c", IGNORED_PTR_ARG)) /*this is building a JSON object in timetemp*/
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(4);

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(validMessageHandle)); /*this is getting the properties from the message*/
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)) /*this is getting the properties in a writeable map, because ConstMap doesn't have ToJSON*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Map_ToJSON(IGNORED_PTR_ARG)) /*this is getting a STRING_HANDLE that is the MAP as JSON*/
            .IgnoreArgument(1)
            .SetFailReturn((STRING_HANDLE)NULL);
        
        ///act
        Logger_Receive(moduleHandle, validMessageHandle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 0, CURRENT_API_CALL(gb_fprintf));

        ///cleanup
        Logger_Destroy(moduleHandle);

    }

    /*Tests_SRS_LOGGER_02_012: [If producing the JSON format or writing it to the file fails, then Logger_Receive shall fail and return.]*/
    TEST_FUNCTION(Logger_Receive_fails_when_ConstMap_CloneWriteable_fails)
    {
        ///arrange
        CLoggerMocks mocks;
        auto moduleHandle = Logger_Create(validBrokerHandle, &validConfig);
        mocks.ResetAllCalls();
        mocks_ResetAllCounters();

        STRICT_EXPECTED_CALL(mocks, gb_time(NULL)); /*this is getting the time*/

        STRICT_EXPECTED_CALL(mocks, gb_localtime(IGNORED_PTR_ARG)) /*this is transforming the time from time_t to struct tm* */
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_strftime(IGNORED_PTR_ARG, IGNORED_NUM_ARG, "%c", IGNORED_PTR_ARG)) /*this is building a JSON object in timetemp*/
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(4);

        STRICT_EXPECTED_CALL(mocks, Message_GetProperties(validMessageHandle)); /*this is getting the properties from the message*/
        STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)) /*this is getting the properties in a writeable map, because ConstMap doesn't have ToJSON*/
            .IgnoreArgument(1)
            .SetFailReturn((MAP_HANDLE)NULL);

        ///act
        Logger_Receive(moduleHandle, validMessageHandle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 0, CURRENT_API_CALL(gb_fprintf));

        ///cleanup
        Logger_Destroy(moduleHandle);

    }

    /*Tests_SRS_LOGGER_02_012: [If producing the JSON format or writing it to the file fails, then Logger_Receive shall fail and return.]*/
    TEST_FUNCTION(Logger_Receive_fails_when_gb_strftime_fails)
    {
        ///arrange
        CLoggerMocks mocks;
        auto moduleHandle = Logger_Create(validBrokerHandle, &validConfig);
        mocks.ResetAllCalls();
        mocks_ResetAllCounters();

        STRICT_EXPECTED_CALL(mocks, gb_time(NULL)); /*this is getting the time*/

        STRICT_EXPECTED_CALL(mocks, gb_localtime(IGNORED_PTR_ARG)) /*this is transforming the time from time_t to struct tm* */
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_strftime(IGNORED_PTR_ARG, IGNORED_NUM_ARG, "%c", IGNORED_PTR_ARG)) /*this is building a JSON object in timetemp*/
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(4)
            .SetFailReturn(0);

        ///act
        Logger_Receive(moduleHandle, validMessageHandle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 0, CURRENT_API_CALL(gb_fprintf));

        ///cleanup
        Logger_Destroy(moduleHandle);

    }

    /*Tests_SRS_LOGGER_02_012: [If producing the JSON format or writing it to the file fails, then Logger_Receive shall fail and return.]*/
    TEST_FUNCTION(Logger_Receive_fails_when_gb_localtime_fails)
    {
        ///arrange
        CLoggerMocks mocks;
        auto moduleHandle = Logger_Create(validBrokerHandle, &validConfig);
        mocks.ResetAllCalls();
        mocks_ResetAllCounters();

        STRICT_EXPECTED_CALL(mocks, gb_time(NULL)); /*this is getting the time*/

        STRICT_EXPECTED_CALL(mocks, gb_localtime(IGNORED_PTR_ARG)) /*this is transforming the time from time_t to struct tm* */
            .IgnoreArgument(1)
            .SetFailReturn((struct tm*)NULL);

        ///act
        Logger_Receive(moduleHandle, validMessageHandle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 0, CURRENT_API_CALL(gb_fprintf));

        ///cleanup
        Logger_Destroy(moduleHandle);

    }

    /*Tests_SRS_LOGGER_02_012: [If producing the JSON format or writing it to the file fails, then Logger_Receive shall fail and return.]*/
    TEST_FUNCTION(Logger_Receive_fails_when_gb_time_fails)
    {
        ///arrange
        CLoggerMocks mocks;
        auto moduleHandle = Logger_Create(validBrokerHandle, &validConfig);
        mocks.ResetAllCalls();
        mocks_ResetAllCounters();

        STRICT_EXPECTED_CALL(mocks, gb_time(NULL)) /*this is getting the time*/
            .SetFailReturn((time_t)-1);

        ///act
        Logger_Receive(moduleHandle, validMessageHandle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(size_t, 0, CURRENT_API_CALL(gb_fprintf));

        ///cleanup
        Logger_Destroy(moduleHandle);

    }

    /*Tests_SRS_LOGGER_02_014: [If moduleHandle is NULL then Logger_Destroy shall return.] */
    TEST_FUNCTION(Logger_Destroy_with_NULL_parameter_returns)
    {
        ///arrange
        CLoggerMocks mocks;

        ///act
        Logger_Destroy(NULL);


        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_LOGGER_02_019: [Logger_Destroy shall add to the log file the following end of log JSON object:]*/
    /*Tests_SRS_LOGGER_02_015: [Otherwise Logger_Destroy shall unuse all used resources.]*/
    TEST_FUNCTION(Logger_Destroy_happy_path)
    {
        ///arrange
        CLoggerMocks mocks;
        auto moduleHandle = Logger_Create(validBrokerHandle, &validConfig);
        mocks.ResetAllCalls();
        mocks_ResetAllCounters();

        STRICT_EXPECTED_CALL(mocks, gb_time(NULL)); /*this is getting the time*/

        STRICT_EXPECTED_CALL(mocks, gb_localtime(IGNORED_PTR_ARG)) /*this is transforming the time from time_t to struct tm* */
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gb_strftime(IGNORED_PTR_ARG, IGNORED_NUM_ARG, ",{\"time\":\"%c\",\"content\":\"Log stopped\"}]", IGNORED_PTR_ARG)) /*this is building a JSON object in timetemp*/
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(4);

        STRICT_EXPECTED_CALL(mocks, gb_fseek(IGNORED_PTR_ARG, -1, SEEK_END)) /*after getting the STRING that is the beginning of the log, it needs to be appended to the file, this essenially eats the "," from the last entry*/
            .IgnoreArgument(1);

        /*here a call to fprintf happens({
        "time":"timeAsPrinted by ctime",
        "content": "Log stopped"
        }]"
        it is captured by a weak verification in ASSERT*/

        STRICT_EXPECTED_CALL(mocks, gb_fclose(IGNORED_PTR_ARG)) /*this closes the file opened in _create*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)) /*this frees the memory allocated for the handle data*/
            .IgnoreArgument(1);

        ///act
        Logger_Destroy(moduleHandle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_LOGGER_26_001: [ `Module_GetApi` shall return a pointer to a  `MODULE_API` structure with the required function pointers. ]*/
    TEST_FUNCTION(Module_GetApi_returns_non_NULL_and_non_NULL_fields)
    {
        ///arrrange

        ///act
        const MODULE_API* result = Module_GetApi(MODULE_API_VERSION_1);

        ///assert
        ASSERT_IS_NOT_NULL(result);

		ASSERT_IS_TRUE(MODULE_PARSE_CONFIGURATION_FROM_JSON(result) != NULL);
		ASSERT_IS_TRUE(MODULE_FREE_CONFIGURATION(result) != NULL);
        ASSERT_IS_TRUE(MODULE_CREATE(result) != NULL);
        ASSERT_IS_TRUE(MODULE_DESTROY(result) != NULL);
        ASSERT_IS_TRUE(MODULE_RECEIVE(result) != NULL);
    }
END_TEST_SUITE(logger_ut)
