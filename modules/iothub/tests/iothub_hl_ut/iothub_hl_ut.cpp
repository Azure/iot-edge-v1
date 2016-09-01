// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"
#include "azure_c_shared_utility/lock.h"

#include "iothub.h"
#include "iothub_hl.h"
#include "parson.h"


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

};


/*forward declarations*/
static MODULE_HANDLE IotHub_Create(BROKER_HANDLE broker, const void* configuration);
/*this destroys (frees resources) of the module parameter*/
static void IotHub_Destroy(MODULE_HANDLE moduleHandle);
/*this is the module's callback function - gets called when a message is to be received by the module*/
static void IotHub_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle);

static const MODULE_APIS moduleInterface =
{
	IotHub_Create,
	IotHub_Destroy,
	IotHub_Receive
};

typedef struct json_value_t
{
	int fake;
} JSON_Value;

typedef struct json_object_t
{
	int fake;
} JSON_Object;

/*these are simple cached variables*/
static pfModule_Create Module_Create = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Destroy Module_Destroy = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Receive Module_Receive = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/

extern "C" const TRANSPORT_PROVIDER* HTTP_Protocol(void) { return NULL; }
extern "C" const TRANSPORT_PROVIDER* AMQP_Protocol(void) { return NULL; }
extern "C" const TRANSPORT_PROVIDER* MQTT_Protocol(void) { return NULL; }

static MICROMOCK_MUTEX_HANDLE g_testByTest;
static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

TYPED_MOCK_CLASS(IotHubHLMocks, CGlobalMock)
{
public:

	/* IotHub module mocks*/
	MOCK_STATIC_METHOD_0(, const MODULE_APIS*, MODULE_STATIC_GETAPIS(IOTHUB_MODULE))
	MOCK_METHOD_END(const MODULE_APIS*, (const MODULE_APIS*)&moduleInterface);

	MOCK_STATIC_METHOD_2(, MODULE_HANDLE, IotHub_Create, BROKER_HANDLE, broker, const void*, configuration)
	MOCK_METHOD_END(MODULE_HANDLE, malloc(1));

	MOCK_STATIC_METHOD_1(, void, IotHub_Destroy, MODULE_HANDLE, moduleHandle)
		free(moduleHandle);
	MOCK_VOID_METHOD_END();

	MOCK_STATIC_METHOD_2(, void, IotHub_Receive, MODULE_HANDLE, moduleHandle, MESSAGE_HANDLE, messageHandle)
	MOCK_VOID_METHOD_END();



	/* Parson Mocks */
	MOCK_STATIC_METHOD_1(, JSON_Value*, json_parse_string, const char *, parseString)
		JSON_Value* value = NULL;
	    if (parseString != NULL)
	    {
		    value = (JSON_Value*)malloc(1);
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
		const char * result2;
	    if (strcmp(name, "IoTHubSuffix") == 0)
	    {
		    result2 = "suffix.name";
	    }
	    else if (strcmp(name, "IoTHubName") == 0)
	    {
		    result2 = "aHubName";
	    }
        else if (strcmp(name, "Transport") == 0)
        {
            result2 = "abc";
        }
        else
	    {
		    result2 = NULL;
	    }
	MOCK_METHOD_END(const char*, result2);

	MOCK_STATIC_METHOD_1(, void, json_value_free, JSON_Value*, value)
		free(value);
	MOCK_VOID_METHOD_END();


	/* malloc */
	MOCK_STATIC_METHOD_1(, void*, gballoc_malloc, size_t, size)
		void* result2 = BASEIMPLEMENTATION::gballoc_malloc(size);
	MOCK_METHOD_END(void*, result2);

	MOCK_STATIC_METHOD_1(, void, gballoc_free, void*, ptr)
		BASEIMPLEMENTATION::gballoc_free(ptr);
	MOCK_VOID_METHOD_END()
};

DECLARE_GLOBAL_MOCK_METHOD_0(IotHubHLMocks, , const MODULE_APIS*, MODULE_STATIC_GETAPIS(IOTHUB_MODULE));

DECLARE_GLOBAL_MOCK_METHOD_2(IotHubHLMocks, , MODULE_HANDLE, IotHub_Create, BROKER_HANDLE, broker, const void*, configuration);
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubHLMocks, , void, IotHub_Destroy, MODULE_HANDLE, moduleHandle);
DECLARE_GLOBAL_MOCK_METHOD_2(IotHubHLMocks, , void, IotHub_Receive, MODULE_HANDLE, moduleHandle, MESSAGE_HANDLE, messageHandle);

DECLARE_GLOBAL_MOCK_METHOD_1(IotHubHLMocks, , JSON_Value*, json_parse_string, const char *, filename);
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubHLMocks, , JSON_Object*, json_value_get_object, const JSON_Value*, value);
DECLARE_GLOBAL_MOCK_METHOD_2(IotHubHLMocks, , const char*, json_object_get_string, const JSON_Object*, object, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubHLMocks, , void, json_value_free, JSON_Value*, value);

DECLARE_GLOBAL_MOCK_METHOD_1(IotHubHLMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(IotHubHLMocks, , void, gballoc_free, void*, ptr);


BEGIN_TEST_SUITE(iothub_hl_ut)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
	g_testByTest = MicroMockCreateMutex();
	ASSERT_IS_NOT_NULL(g_testByTest);

	Module_Create = Module_GetAPIS()->Module_Create;
	Module_Destroy = Module_GetAPIS()->Module_Destroy;
	Module_Receive = Module_GetAPIS()->Module_Receive;
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
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
	if (!MicroMockReleaseMutex(g_testByTest))
	{
		ASSERT_FAIL("failure in test framework at ReleaseMutex");
	}
}


/*Tests_SRS_IOTHUBMODULE_HL_17_013: [ `Module_GetAPIS` shall return a non-NULL pointer. ]*/
TEST_FUNCTION(Module_GetAPIS_returns_non_NULL)
{
	///arrange
	IotHubHLMocks mocks;

	///act
	auto MODULEAPIS = Module_GetAPIS();

	///assert
	ASSERT_IS_NOT_NULL(MODULEAPIS);
	mocks.AssertActualAndExpectedCalls();

	///cleanup
}

/*Tests_SRS_IOTHUBMODULE_HL_17_014: [ The MODULE_APIS structure shall have non-NULL `Module_Create`, `Module_Destroy`, and `Module_Receive` fields. ]*/
TEST_FUNCTION(Module_GetAPIS_returns_non_NULL_fields)
{
	///arrange
	IotHubHLMocks mocks;

	///act
	auto MODULEAPIS = Module_GetAPIS();

	///assert
	ASSERT_IS_NOT_NULL(MODULEAPIS->Module_Create);
	ASSERT_IS_NOT_NULL(MODULEAPIS->Module_Destroy);
	ASSERT_IS_NOT_NULL(MODULEAPIS->Module_Receive);
	mocks.AssertActualAndExpectedCalls();

	///cleanup
}

/*Tests_SRS_IOTHUBMODULE_HL_17_004: [ `IotHub_HL_Create` shall parse `configuration` as a JSON string. ]*/
/*Tests_SRS_IOTHUBMODULE_HL_17_008: [ `IotHub_HL_Create` shall invoke the IotHub module's create function, using the broker, IotHubName, IoTHubSuffix, and Transport. ]*/
/*Tests_SRS_IOTHUBMODULE_HL_17_009: [ When the lower layer IotHub module creation succeeds, `IotHub_HL_Create` shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(IotHub_HL_Create_success)
{
	///arrange
	IotHubHLMocks mocks;
	const char * validJsonString = "calling it valid makes it so";
	BROKER_HANDLE broker = (BROKER_HANDLE)0x42;

	STRICT_EXPECTED_CALL(mocks, json_parse_string(validJsonString));
	STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "IoTHubName"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "IoTHubSuffix"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "Transport"))
        .IgnoreArgument(1)
        .SetReturn("HTTP");
    STRICT_EXPECTED_CALL(mocks, MODULE_STATIC_GETAPIS(IOTHUB_MODULE)());
	STRICT_EXPECTED_CALL(mocks, IotHub_Create(broker, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	///act
	auto result = Module_Create(broker, validJsonString);

    ///assert
	ASSERT_IS_NOT_NULL(result);
	mocks.AssertActualAndExpectedCalls();

	///cleanup
	Module_Destroy(result);
}

/*Tests_SRS_IOTHUBMODULE_HL_17_008: [ `IotHub_HL_Create` shall invoke the IotHub module's create function, using the broker, IotHubName, IoTHubSuffix, and Transport. ]*/
TEST_FUNCTION(IotHub_HL_Create_interprets_HTTP_transport)
{
    ///arrange
    CNiceCallComparer<IotHubHLMocks> mocks;
    BROKER_HANDLE broker = (BROKER_HANDLE)0x42;
    IOTHUB_CLIENT_TRANSPORT_PROVIDER provider = HTTP_Protocol;

    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "Transport"))
        .IgnoreArgument(1)
        .SetReturn("HTTP");
    STRICT_EXPECTED_CALL(mocks, IotHub_Create(broker, IGNORED_PTR_ARG))
        .ValidateArgumentBuffer(2, &provider, sizeof(provider), offsetof(IOTHUB_CONFIG, transportProvider));

    ///act
    auto result = Module_Create(broker, "don't care");

    ///assert
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Module_Destroy(result);
}

/*Tests_SRS_IOTHUBMODULE_HL_17_008: [ `IotHub_HL_Create` shall invoke the IotHub module's create function, using the broker, IotHubName, IoTHubSuffix, and Transport. ]*/
TEST_FUNCTION(IotHub_HL_Create_interprets_AMQP_transport)
{
    ///arrange
    CNiceCallComparer<IotHubHLMocks> mocks;
    BROKER_HANDLE broker = (BROKER_HANDLE)0x42;
    IOTHUB_CLIENT_TRANSPORT_PROVIDER provider = AMQP_Protocol;

    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "Transport"))
        .IgnoreArgument(1)
        .SetReturn("AMQP");
    STRICT_EXPECTED_CALL(mocks, IotHub_Create(broker, IGNORED_PTR_ARG))
        .ValidateArgumentBuffer(2, &provider, sizeof(provider), offsetof(IOTHUB_CONFIG, transportProvider));

    ///act
    auto result = Module_Create(broker, "don't care");

    ///assert
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Module_Destroy(result);
}

/*Tests_SRS_IOTHUBMODULE_HL_17_008: [ `IotHub_HL_Create` shall invoke the IotHub module's create function, using the broker, IotHubName, IoTHubSuffix, and Transport. ]*/
TEST_FUNCTION(IotHub_HL_Create_interprets_MQTT_transport)
{
    ///arrange
    CNiceCallComparer<IotHubHLMocks> mocks;
    BROKER_HANDLE broker = (BROKER_HANDLE)0x42;
    IOTHUB_CLIENT_TRANSPORT_PROVIDER provider = MQTT_Protocol;

    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "Transport"))
        .IgnoreArgument(1)
        .SetReturn("MQTT");
    STRICT_EXPECTED_CALL(mocks, IotHub_Create(broker, IGNORED_PTR_ARG))
        .ValidateArgumentBuffer(2, &provider, sizeof(provider), offsetof(IOTHUB_CONFIG, transportProvider));

    ///act
    auto result = Module_Create(broker, "don't care");

    ///assert
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Module_Destroy(result);
}

/*Tests_SRS_IOTHUBMODULE_HL_05_002: [ If the value of "Transport" is not one of "HTTP", "AMQP", or "MQTT" (case-insensitive) then `IotHub_HL_Create` shall fail and return NULL. ]*/
TEST_FUNCTION(IotHub_HL_Create_interprets_the_transport_string_without_regard_to_case)
{
    CNiceCallComparer<IotHubHLMocks> mocks;
    BROKER_HANDLE broker = (BROKER_HANDLE)0x42;

    const char* transportStrings[] = { "http", "amqp", "mqtt" };
    const IOTHUB_CLIENT_TRANSPORT_PROVIDER providers[] = { HTTP_Protocol, AMQP_Protocol, MQTT_Protocol };
    const size_t length = sizeof(transportStrings) / sizeof(transportStrings[0]);

    for (int i = 0; i < length; ++i)
    {
        mocks.ResetAllCalls();

        ///arrange
        STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "Transport"))
            .IgnoreArgument(1)
            .SetReturn(transportStrings[i]);
        STRICT_EXPECTED_CALL(mocks, IotHub_Create(broker, IGNORED_PTR_ARG))
            .ValidateArgumentBuffer(2, &providers[i], sizeof(IOTHUB_CLIENT_TRANSPORT_PROVIDER), offsetof(IOTHUB_CONFIG, transportProvider));

        ///act
        auto result = Module_Create(broker, "don't care");

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        Module_Destroy(result);
    }
}

/*Tests_SRS_IOTHUBMODULE_HL_05_002: [ If the value of "Transport" is not one of "HTTP", "AMQP", or "MQTT" (case-insensitive) then `IotHub_HL_Create` shall fail and return NULL. ]*/
TEST_FUNCTION(IotHub_HL_Create_returns_null_when_transport_is_unknown)
{
    ///arrange
    CNiceCallComparer<IotHubHLMocks> mocks;
    BROKER_HANDLE broker = (BROKER_HANDLE)0x42;

    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "Transport"))
        .IgnoreArgument(1)
        .SetReturn("ABCD");
    EXPECTED_CALL(mocks, IotHub_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .NeverInvoked();

    ///act
    auto result = Module_Create(broker, "don't care");

    ///assert
    ASSERT_IS_NULL(result);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Module_Destroy(result);
}

/*Tests_SRS_IOTHUBMODULE_HL_17_010: [ If the lower layer IotHub module creation fails, `IotHub_HL_Create` shall fail and return NULL. ]*/
TEST_FUNCTION(IotHub_HL_Create_ll_module_null_returns_null)
{
	///arrange
	IotHubHLMocks mocks;
	const char * validJsonString = "calling it valid makes it so";
	BROKER_HANDLE broker = (BROKER_HANDLE)0x42;

	STRICT_EXPECTED_CALL(mocks, json_parse_string(validJsonString));
	STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "IoTHubName"))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "IoTHubSuffix"))
		.IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "Transport"))
        .IgnoreArgument(1)
        .SetReturn("HTTP");
    STRICT_EXPECTED_CALL(mocks, MODULE_STATIC_GETAPIS(IOTHUB_MODULE)());
	STRICT_EXPECTED_CALL(mocks, IotHub_Create(broker, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.SetFailReturn((MODULE_HANDLE)NULL);
	STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	///act
	auto result = Module_Create(broker, validJsonString);
	///assert

	ASSERT_IS_NULL(result);

	///cleanup
}

/*Tests_SRS_IOTHUBMODULE_HL_05_001: [ If the JSON object does not contain a value named "Transport" then `IotHub_HL_Create` shall fail and return NULL. ]*/
TEST_FUNCTION(IotHub_HL_Create_returns_null_when_Transport_is_missing)
{
    ///arrange
    IotHubHLMocks mocks;
    const char * validJsonString = "calling it valid makes it so";
    BROKER_HANDLE broker = (BROKER_HANDLE)0x42;

    STRICT_EXPECTED_CALL(mocks, json_parse_string(validJsonString));
    STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "IoTHubName"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "IoTHubSuffix"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "Transport"))
        .IgnoreArgument(1)
        .SetFailReturn((const char *)NULL);
    STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    ///act
    auto result = Module_Create(broker, validJsonString);
    ///assert

    ASSERT_IS_NULL(result);

    ///cleanup
}

/*Tests_SRS_IOTHUBMODULE_HL_17_007: [ If the JSON object does not contain a value named "IoTHubSuffix" then `IotHub_HL_Create` shall fail and return NULL. ]*/
TEST_FUNCTION(IotHub_HL_Create_IoTHubSuffix_not_found_returns_null)
{
	///arrange
	IotHubHLMocks mocks;
	const char * validJsonString = "calling it valid makes it so";
	BROKER_HANDLE broker = (BROKER_HANDLE)0x42;

	STRICT_EXPECTED_CALL(mocks, json_parse_string(validJsonString));
	STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "IoTHubName"))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "IoTHubSuffix"))
		.IgnoreArgument(1)
		.SetFailReturn((const char *)NULL);
	STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	///act
	auto result = Module_Create(broker, validJsonString);
	///assert

	ASSERT_IS_NULL(result);

	///cleanup
}

/*Tests_SRS_IOTHUBMODULE_HL_17_006: [ If the JSON object does not contain a value named "IoTHubName" then `IotHub_HL_Create` shall fail and return NULL. ]*/
TEST_FUNCTION(IotHub_HL_Create_IoTHubName_not_found_returns_null)
{
	///arrange
	IotHubHLMocks mocks;
	const char * validJsonString = "calling it valid makes it so";
	BROKER_HANDLE broker = (BROKER_HANDLE)0x42;

    STRICT_EXPECTED_CALL(mocks, json_parse_string(validJsonString));
    STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "IoTHubName"))
        .IgnoreArgument(1)
        .SetFailReturn((const char *)NULL);
    STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

	///act
	auto result = Module_Create(broker, validJsonString);
	///assert

	ASSERT_IS_NULL(result);

	///cleanup
}

/*Tests_SRS_IOTHUBMODULE_HL_17_005: [ If parsing of `configuration` fails, `IotHub_HL_Create` shall fail and return NULL. ]*/
TEST_FUNCTION(IotHub_HL_Create_get_object_null_returns_null)
{
	///arrange
	IotHubHLMocks mocks;
	const char * validJsonString = "calling it valid makes it so";
	BROKER_HANDLE broker = (BROKER_HANDLE)0x42;

	STRICT_EXPECTED_CALL(mocks, json_parse_string(validJsonString));
	STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetFailReturn((JSON_Object*)NULL);
	STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	///act
	auto result = Module_Create(broker, validJsonString);
	///assert

	ASSERT_IS_NULL(result);
}

/*Tests_SRS_IOTHUBMODULE_HL_17_005: [ If parsing of `configuration` fails, `IotHub_HL_Create` shall fail and return NULL. ]*/
/*Tests_SRS_IOTHUBMODULE_HL_17_003: [ If `configuration` is not a JSON string, then `IotHub_HL_Create` shall fail and return NULL. ]*/
TEST_FUNCTION(IotHub_HL_Create_parse_string_null_returns_null)
{
	///arrange
	IotHubHLMocks mocks;
	const char * invalidJsonString = "calling it invalid has the same effect";
	BROKER_HANDLE broker = (BROKER_HANDLE)0x42;

	STRICT_EXPECTED_CALL(mocks, json_parse_string(invalidJsonString))
		.SetFailReturn((JSON_Value*)NULL);

	///act
	auto result = Module_Create(broker, invalidJsonString);
	///assert

	ASSERT_IS_NULL(result);
}

/*Tests_SRS_IOTHUBMODULE_HL_17_001: [If `broker` is NULL then `IotHub_HL_Create` shall fail and return NULL.]*/
TEST_FUNCTION(IotHub_HL_Create_broker_handle_null_returns_null)
{
	///arrange
	IotHubHLMocks mocks;
	const char * validJsonString = "calling it valid makes it so";
	BROKER_HANDLE broker = NULL;

	///act
	auto result = Module_Create(broker, validJsonString);
	///assert

	ASSERT_IS_NULL(result);
}

/*Tests_SRS_IOTHUBMODULE_HL_17_002: [ If `configuration` is NULL then `IotHub_HL_Create` shall fail and return NULL. ]*/
TEST_FUNCTION(IotHub_HL_Create_config_null_returns_null)
{
	///arrange
	IotHubHLMocks mocks;
	const char * nullString = NULL;
	BROKER_HANDLE broker = (BROKER_HANDLE)0x42;

	///act
	auto result = Module_Create(broker, nullString);
	///assert

	ASSERT_IS_NULL(result);
}

/*Tests_SRS_IOTHUBMODULE_HL_17_012: [ IotHub_HL_Destroy shall free all used resources. ]*/
TEST_FUNCTION(IotHub_HL_Destroy_does_everything)
{
	///arrange
	IotHubHLMocks mocks;
	const char * validJsonString = "calling it valid makes it so";
	BROKER_HANDLE broker = (BROKER_HANDLE)0x42;
	auto result = Module_Create(broker, validJsonString);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, MODULE_STATIC_GETAPIS(IOTHUB_MODULE)());
	STRICT_EXPECTED_CALL(mocks, IotHub_Destroy(result));

	///act
	Module_Destroy(result);

	///assert
	///cleanup
}

/*Tests_SRS_IOTHUBMODULE_HL_17_011: [ IotHub_HL_Receive shall pass the received parameters to the underlying IotHub module's receive function. ]*/
TEST_FUNCTION(IotHub_HL_Receive_does_everything)
{
	///arrange
	IotHubHLMocks mocks;
	const char * validJsonString = "calling it valid makes it so";
	BROKER_HANDLE broker = (BROKER_HANDLE)0x42;
	MESSAGE_HANDLE messageHandle = (MESSAGE_HANDLE)0x42;
	auto result = Module_Create(broker, validJsonString);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, MODULE_STATIC_GETAPIS(IOTHUB_MODULE)());
	STRICT_EXPECTED_CALL(mocks, IotHub_Receive(result, messageHandle));

	///act
	Module_Receive(result, messageHandle);

	///assert
	mocks.AssertActualAndExpectedCalls();

	///cleanup
	Module_Destroy(result);
}

END_TEST_SUITE(iothub_hl_ut)
