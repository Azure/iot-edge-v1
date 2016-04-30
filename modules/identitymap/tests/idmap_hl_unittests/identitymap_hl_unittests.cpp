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
#include "azure_c_shared_utility/vector.h"

#include "parson.h"

#include "identitymap.h"
#include "identitymap_hl.h"


static MICROMOCK_MUTEX_HANDLE g_testByTest;
static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

/*these are simple cached variables*/
static pfModule_Create Module_Create = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Destroy Module_Destroy = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Receive Module_Receive = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/

#define GBALLOC_H

extern "C" int gballoc_init(void);
extern "C" void gballoc_deinit(void);
extern "C" void* gballoc_malloc(size_t size);
extern "C" void* gballoc_calloc(size_t nmemb, size_t size);
extern "C" void* gballoc_realloc(void* ptr, size_t size);
extern "C" void gballoc_free(void* ptr);

extern "C" int mallocAndStrcpy_s(char** destination, const char*source);
extern "C" int unsignedIntToString(char* destination, size_t destinationSize, unsigned int value);
extern "C" int size_tToString(char* destination, size_t destinationSize, size_t value);


namespace BASEIMPLEMENTATION
{
    /*if malloc is defined as gballoc_malloc at this moment, there'd be serious trouble*/
#define Lock(x) (LOCK_OK + gballocState - gballocState) /*compiler warning about constant in if condition*/
#define Unlock(x) (LOCK_OK + gballocState - gballocState)
#define Lock_Init() (LOCK_HANDLE)0x42
#define Lock_Deinit(x) (LOCK_OK + gballocState - gballocState)
#include "gballoc.c"
#undef Lock
#undef Unlock
#undef Lock_Init
#undef Lock_Deinit

#include "vector.c"
};


/*forward declarations*/
static MODULE_HANDLE IdentityMap_Create(MESSAGE_BUS_HANDLE busHandle, const void* configuration);
/*this destroys (frees) resources of the module parameter*/
static void IdentityMap_Destroy(MODULE_HANDLE moduleHandle);
/*this is the module's callback function - gets called when a message is to be received by the module*/
static void IdentityMap_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle);

static const MODULE_APIS IoTHubModuleHttp_GetAPIS_Impl =
{
	IdentityMap_Create,
	IdentityMap_Destroy,
	IdentityMap_Receive
};

typedef struct json_value_t
{
	int fake;
} JSON_Value;

typedef struct json_object_t
{
	int fake;
} JSON_Object;


TYPED_MOCK_CLASS(CIdentitymapHlMocks, CGlobalMock)
	{
	public:

		// IdentityMap LL mocks
		MOCK_STATIC_METHOD_0(, const MODULE_APIS*, MODULE_STATIC_GETAPIS(IDENTITYMAP_MODULE))
			MOCK_METHOD_END(const MODULE_APIS*, (const MODULE_APIS*)&IoTHubModuleHttp_GetAPIS_Impl);

		MOCK_STATIC_METHOD_2(, MODULE_HANDLE, IdentityMap_Create, MESSAGE_BUS_HANDLE, busHandle, const void*, configuration)
			MOCK_METHOD_END(MODULE_HANDLE, malloc(1));

		MOCK_STATIC_METHOD_1(, void, IdentityMap_Destroy, MODULE_HANDLE, moduleHandle)
			free(moduleHandle);
		MOCK_VOID_METHOD_END();

		MOCK_STATIC_METHOD_2(, void, IdentityMap_Receive, MODULE_HANDLE, moduleHandle, MESSAGE_HANDLE, messageHandle)
			MOCK_VOID_METHOD_END();

		// Parson Mocks 
		MOCK_STATIC_METHOD_1(, JSON_Value*, json_parse_string, const char *, parseString)
			JSON_Value* value = NULL;
		if (parseString != NULL)
		{
			value = (JSON_Value*)malloc(1);
		}
		MOCK_METHOD_END(JSON_Value*, value);

		MOCK_STATIC_METHOD_2(, JSON_Object *, json_array_get_object, const JSON_Array *, array, size_t, index)
			JSON_Object* object = NULL;
		if (array != NULL)
		{
			object = (JSON_Object*)0x42;
		}
		MOCK_METHOD_END(JSON_Object*, object);

		MOCK_STATIC_METHOD_1(, JSON_Array*, json_value_get_array, const JSON_Value*, value)
			JSON_Array* object = NULL;
		if (value != NULL)
		{
			object = (JSON_Array*)0x43;
		}
		MOCK_METHOD_END(JSON_Array*, object);

		MOCK_STATIC_METHOD_1(, size_t, json_array_get_count, const JSON_Array *, array)
		MOCK_METHOD_END(size_t, (size_t)0);

		MOCK_STATIC_METHOD_2(, const char*, json_object_get_string, const JSON_Object*, object, const char*, name)
			const char * result2;
		if (strcmp(name, "macAddress") == 0)
		{
			result2 = "mac";
		}
		else if (strcmp(name, "deviceId") == 0)
		{
			result2 = "id";
		}
		else if (strcmp(name, "deviceKey") == 0)
		{
			result2 = "key";
		}
		else
		{
			result2 = NULL;
		}
		MOCK_METHOD_END(const char*, result2);

		MOCK_STATIC_METHOD_1(, void, json_value_free, JSON_Value*, value)
			free(value);
		MOCK_VOID_METHOD_END();


		// vector.h
	MOCK_STATIC_METHOD_1(, VECTOR_HANDLE, VECTOR_create, size_t, elementSize)
		auto r = BASEIMPLEMENTATION::VECTOR_create(elementSize);
	MOCK_METHOD_END(VECTOR_HANDLE, r)

	MOCK_STATIC_METHOD_1(, void, VECTOR_destroy, VECTOR_HANDLE, handle)
	BASEIMPLEMENTATION::VECTOR_destroy(handle);
	MOCK_VOID_METHOD_END()

	MOCK_STATIC_METHOD_3(, int, VECTOR_push_back, VECTOR_HANDLE, handle, const void*, elements, size_t, numElements)
		auto r = BASEIMPLEMENTATION::VECTOR_push_back( handle,  elements,  numElements);
	MOCK_METHOD_END(int, r)

		MOCK_STATIC_METHOD_3(, void, VECTOR_erase, VECTOR_HANDLE, handle, void*, elements, size_t, numElements)
		BASEIMPLEMENTATION::VECTOR_erase(handle, elements, numElements);
	MOCK_VOID_METHOD_END()

		MOCK_STATIC_METHOD_1(, void, VECTOR_clear, VECTOR_HANDLE, handle)
		BASEIMPLEMENTATION::VECTOR_clear(handle);
	MOCK_VOID_METHOD_END()

	MOCK_STATIC_METHOD_2(, void*, VECTOR_element, const VECTOR_HANDLE, handle, size_t, index)
		void * result1;
		result1 = BASEIMPLEMENTATION::VECTOR_element(handle, index);
	MOCK_METHOD_END(void*, result1)

	MOCK_STATIC_METHOD_1(, void*, VECTOR_front, const VECTOR_HANDLE, handle)
		auto r = BASEIMPLEMENTATION::VECTOR_front( handle);
	MOCK_METHOD_END(void*, r)

	MOCK_STATIC_METHOD_1(, void*, VECTOR_back, const VECTOR_HANDLE, handle)
		auto r = BASEIMPLEMENTATION::VECTOR_back( handle);
	MOCK_METHOD_END(void*, r)

	MOCK_STATIC_METHOD_3(, void*, VECTOR_find_if, const VECTOR_HANDLE, handle, PREDICATE_FUNCTION, pred, const void*, value)
		auto r = BASEIMPLEMENTATION::VECTOR_find_if( handle,  pred, value);
	MOCK_METHOD_END(void*, r)

	MOCK_STATIC_METHOD_1(, size_t, VECTOR_size, const VECTOR_HANDLE, handle)
		auto r = BASEIMPLEMENTATION::VECTOR_size( handle);
	MOCK_METHOD_END(size_t, r)

	};


DECLARE_GLOBAL_MOCK_METHOD_0(CIdentitymapHlMocks, , const MODULE_APIS*, MODULE_STATIC_GETAPIS(IDENTITYMAP_MODULE));

DECLARE_GLOBAL_MOCK_METHOD_2(CIdentitymapHlMocks, , MODULE_HANDLE, IdentityMap_Create, MESSAGE_BUS_HANDLE, busHandle, const void*, configuration);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapHlMocks, , void, IdentityMap_Destroy, MODULE_HANDLE, moduleHandle);
DECLARE_GLOBAL_MOCK_METHOD_2(CIdentitymapHlMocks, , void, IdentityMap_Receive, MODULE_HANDLE, moduleHandle, MESSAGE_HANDLE, messageHandle);

DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapHlMocks, , JSON_Value*, json_parse_string, const char *, filename);
DECLARE_GLOBAL_MOCK_METHOD_2(CIdentitymapHlMocks, , JSON_Object *, json_array_get_object, const JSON_Array *, array, size_t, index);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapHlMocks, , JSON_Array*, json_value_get_array, const JSON_Value*, value);
DECLARE_GLOBAL_MOCK_METHOD_2(CIdentitymapHlMocks, , const char*, json_object_get_string, const JSON_Object*, object, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapHlMocks, , size_t, json_array_get_count, const JSON_Array *, array);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapHlMocks, , void, json_value_free, JSON_Value*, value);

// vector.h
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapHlMocks, , VECTOR_HANDLE, VECTOR_create, size_t, elementSize);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapHlMocks, , void, VECTOR_destroy, VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_3(CIdentitymapHlMocks, , int, VECTOR_push_back, VECTOR_HANDLE, handle, const void*, elements, size_t, numElements);
DECLARE_GLOBAL_MOCK_METHOD_3(CIdentitymapHlMocks, , void, VECTOR_erase, VECTOR_HANDLE, handle, void*, elements, size_t, numElements);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapHlMocks, , void, VECTOR_clear, VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_2(CIdentitymapHlMocks, , void*, VECTOR_element, const VECTOR_HANDLE, handle, size_t, index);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapHlMocks, , void*, VECTOR_front, const VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapHlMocks, , void*, VECTOR_back, const VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_3(CIdentitymapHlMocks, , void*, VECTOR_find_if, const VECTOR_HANDLE, handle, PREDICATE_FUNCTION, pred, const void*, value);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapHlMocks, , size_t, VECTOR_size, const VECTOR_HANDLE, handle);



BEGIN_TEST_SUITE(identitymap_hl_unittests)

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

	//Tests_SRS_IDMAP_HL_17_001: [Module_GetAPIs shall return a non-NULL pointer to a MODULE_APIS structure.]
	//Tests_SRS_IDMAP_HL_17_002: [ The MODULE_APIS structure shall have non-NULL Module_Create, Module_Destroy, and Module_Receive fields. ]
	TEST_FUNCTION(IdentityMap_HL_GetAPIs_Success)
	{
		///Arrange
		CIdentitymapHlMocks mocks;

		///Act
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		///Assert
		ASSERT_IS_NOT_NULL(theAPIS);
		ASSERT_IS_NOT_NULL((void*)theAPIS->Module_Create);
		ASSERT_IS_NOT_NULL((void*)theAPIS->Module_Destroy);
		ASSERT_IS_NOT_NULL((void*)theAPIS->Module_Receive);

		///Ablution
	}

	//Tests_SRS_IDMAP_HL_17_003: [ If busHandle is NULL then IdentityMap_HL_Create shall fail and return NULL. ]
	TEST_FUNCTION(IdentityMap_HL_Create_Bus_Null)
	{
		///Arrange
		CIdentitymapHlMocks mocks;
		MESSAGE_BUS_HANDLE bus = NULL;
		unsigned char config;

		///Act
		auto n = Module_Create(bus, &config);

		///Assert
		ASSERT_IS_NULL(n);

		///Ablution
	}

	//Tests_SRS_IDMAP_HL_17_004: [ If configuration is NULL then IdentityMap_HL_Create shall fail and return NULL. ]
	TEST_FUNCTION(IdentityMap_HL_Create_Config_Null)
	{
		///Arrange
		CIdentitymapHlMocks mocks;
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		void * config = NULL;

		///Act
		auto n = Module_Create(bus, config);

		///Assert
		ASSERT_IS_NULL(n);

		///Ablution
	}

	//Tests_SRS_IDMAP_HL_17_006: [ IdentityMap_HL_Create shall parse the configuration as a JSON array of objects. ]
	//Tests_SRS_IDMAP_HL_17_007: [ IdentityMap_HL_Create shall call VECTOR_create to make the identity map module input vector. ]
	//Tests_SRS_IDMAP_HL_17_008: [ IdentityMap_HL_Create shall walk through each object of the array. ]
	//Tests_SRS_IDMAP_HL_17_012: [ IdentityMap_HL_Create shall use "macAddress", "deviceId", and "deviceKey" values as the fields for an IDENTITY_MAP_CONFIG structure and call VECTOR_push_back to add this element to the vector. ]
    //Tests_SRS_IDMAP_HL_17_013: [ IdentityMap_HL_Create shall invoke identity map module's create, passing in the message bus handle and the input vector. ]
	//Tests_SRS_IDMAP_HL_17_014: [ When the lower layer identity map module create succeeds, IdentityMap_HL_Create shall succeed and return a non-NULL value. ]
    //Tests_SRS_IDMAP_HL_17_016: [ IdentityMap_HL_Create shall release all data it allocated. ]
	TEST_FUNCTION(IdentityMap_HL_Create_Success)
	{
		///Arrange
		CIdentitymapHlMocks mocks;
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		const char* config = "pretend this is a valid JSON string";

		STRICT_EXPECTED_CALL(mocks, json_parse_string(config));
		STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_value_get_array(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(IDENTITY_MAP_CONFIG)));
		STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.SetReturn((size_t)1);
		STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
			.IgnoreArgument(1)
			.IgnoreArgument(2);
		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "macAddress"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "deviceId"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "deviceKey"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
			.IgnoreArgument(1)
			.IgnoreArgument(2);
		STRICT_EXPECTED_CALL(mocks, MODULE_STATIC_GETAPIS(IDENTITYMAP_MODULE)());
		STRICT_EXPECTED_CALL(mocks, IdentityMap_Create(bus, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		//Act
		auto n = Module_Create(bus, config);

		///Assert
		ASSERT_IS_NOT_NULL(n);
		mocks.AssertActualAndExpectedCalls();

		///Cleanup

		Module_Destroy(n);
	}

	//Tests_SRS_IDMAP_HL_17_008: [ IdentityMap_HL_Create shall walk through each object of the array. ]
	TEST_FUNCTION(IdentityMap_HL_Create_Success_with_2_element_array)
	{
		///Arrange
		CIdentitymapHlMocks mocks;
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		const char* config = "pretend this is a valid JSON string";

		STRICT_EXPECTED_CALL(mocks, json_parse_string(config));
		STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_value_get_array(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(IDENTITY_MAP_CONFIG)));
		STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.SetReturn((size_t)2);

		{
			STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
				.IgnoreArgument(1)
				.IgnoreArgument(2);
			STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "macAddress"))
				.IgnoreArgument(1);
			STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "deviceId"))
				.IgnoreArgument(1);
			STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "deviceKey"))
				.IgnoreArgument(1);
			STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
				.IgnoreArgument(1)
				.IgnoreArgument(2);
		}
		{
			STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 1))
				.IgnoreArgument(1)
				.IgnoreArgument(2);
			STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "macAddress"))
				.IgnoreArgument(1);
			STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "deviceId"))
				.IgnoreArgument(1);
			STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "deviceKey"))
				.IgnoreArgument(1);
			STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
				.IgnoreArgument(1)
				.IgnoreArgument(2);
		}

		STRICT_EXPECTED_CALL(mocks, MODULE_STATIC_GETAPIS(IDENTITYMAP_MODULE)());
		STRICT_EXPECTED_CALL(mocks, IdentityMap_Create(bus, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		//Act
		auto n = Module_Create(bus, config);

		///Assert
		ASSERT_IS_NOT_NULL(n);
		mocks.AssertActualAndExpectedCalls();

		///Cleanup

		Module_Destroy(n);
	}

	//Tests_SRS_IDMAP_HL_17_015: [ If the lower layer identity map module create fails, IdentityMap_HL_Create shall fail and return NULL. ]
	TEST_FUNCTION(IdentityMap_HL_Create_ll_Create_failed_returns_null)
	{
		///Arrange
		CIdentitymapHlMocks mocks;
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		const char* config = "pretend this is a valid JSON string";

		STRICT_EXPECTED_CALL(mocks, json_parse_string(config));
		STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_value_get_array(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(IDENTITY_MAP_CONFIG)));
		STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.SetReturn((size_t)1);
		STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
			.IgnoreArgument(1)
			.IgnoreArgument(2);
		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "macAddress"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "deviceId"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "deviceKey"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
			.IgnoreArgument(1)
			.IgnoreArgument(2);
		STRICT_EXPECTED_CALL(mocks, MODULE_STATIC_GETAPIS(IDENTITYMAP_MODULE)());
		STRICT_EXPECTED_CALL(mocks, IdentityMap_Create(bus, IGNORED_PTR_ARG))
			.IgnoreArgument(2)
			.SetFailReturn((MODULE_HANDLE)NULL);

		//Act
		auto n = Module_Create(bus, config);

		///Assert
		ASSERT_IS_NULL(n);
		mocks.AssertActualAndExpectedCalls();

		///Cleanup

	}

	//Tests_SRS_IDMAP_HL_17_020: [ If pushing into the vector is not successful, then IdentityMap_HL_Create shall fail and return NULL. ]
	TEST_FUNCTION(IdentityMap_HL_Create_push_back_failed_returns_null)
	{
		///Arrange
		CIdentitymapHlMocks mocks;
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		const char* config = "pretend this is a valid JSON string";

		STRICT_EXPECTED_CALL(mocks, json_parse_string(config));
		STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_value_get_array(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(IDENTITY_MAP_CONFIG)));
		STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.SetReturn((size_t)1);
		STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
			.IgnoreArgument(1)
			.IgnoreArgument(2);
		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "macAddress"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "deviceId"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "deviceKey"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
			.IgnoreArgument(1)
			.IgnoreArgument(2)
			.SetFailReturn(__LINE__);

		//Act
		auto n = Module_Create(bus, config);

		///Assert
		ASSERT_IS_NULL(n);
		mocks.AssertActualAndExpectedCalls();

		///Cleanup
	}

	//Tests_SRS_IDMAP_HL_17_009: [ If the array object does not contain a value named "macAddress" then IdentityMap_HL_Create shall fail and return NULL. ]
	TEST_FUNCTION(IdentityMap_HL_Create_no_key_returns_null)
	{
		///Arrange
		CIdentitymapHlMocks mocks;
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		const char* config = "pretend this is a valid JSON string";

		STRICT_EXPECTED_CALL(mocks, json_parse_string(config));
		STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_value_get_array(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(IDENTITY_MAP_CONFIG)));
		STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.SetReturn((size_t)1);
		STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
			.IgnoreArgument(1)
			.IgnoreArgument(2);
		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "macAddress"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "deviceId"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "deviceKey"))
			.IgnoreArgument(1)
			.SetFailReturn((const char*)NULL);

		//Act
		auto n = Module_Create(bus, config);

		///Assert
		ASSERT_IS_NULL(n);
		mocks.AssertActualAndExpectedCalls();

		///Cleanup
	}

	//Tests_SRS_IDMAP_HL_17_010: [ If the array object does not contain a value named "deviceId" then IdentityMap_HL_Create shall fail and return NULL. ]
	TEST_FUNCTION(IdentityMap_HL_Create_no_id_returns_null)
	{
		///Arrange
		CIdentitymapHlMocks mocks;
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		const char* config = "pretend this is a valid JSON string";

		STRICT_EXPECTED_CALL(mocks, json_parse_string(config));
		STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_value_get_array(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(IDENTITY_MAP_CONFIG)));
		STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.SetReturn((size_t)1);
		STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
			.IgnoreArgument(1)
			.IgnoreArgument(2);
		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "macAddress"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "deviceId"))
			.IgnoreArgument(1)
			.SetFailReturn((const char*)NULL);

		//Act
		auto n = Module_Create(bus, config);

		///Assert
		ASSERT_IS_NULL(n);
		mocks.AssertActualAndExpectedCalls();

		///Cleanup
	}

	//Tests_SRS_IDMAP_HL_17_011: [ If the array object does not contain a value named "deviceKey" then IdentityMap_HL_Create shall fail and return NULL. ]
	TEST_FUNCTION(IdentityMap_HL_Create_no_mac_returns_null)
	{
		///Arrange
		CIdentitymapHlMocks mocks;
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		const char* config = "pretend this is a valid JSON string";

		STRICT_EXPECTED_CALL(mocks, json_parse_string(config));
		STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_value_get_array(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(IDENTITY_MAP_CONFIG)));
		STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.SetReturn((size_t)1);
		STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
			.IgnoreArgument(1)
			.IgnoreArgument(2);
		STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "macAddress"))
			.IgnoreArgument(1)
			.SetFailReturn((const char*)NULL);

		//Act
		auto n = Module_Create(bus, config);

		///Assert
		ASSERT_IS_NULL(n);
		mocks.AssertActualAndExpectedCalls();

		///Cleanup
	}

	//Tests_SRS_IDMAP_HL_17_005: [ If configuration is not a JSON array of JSON objects, then IdentityMap_HL_Create shall fail and return NULL. ]
	TEST_FUNCTION(IdentityMap_HL_Create_no_object_returns_null)
	{
		///Arrange
		CIdentitymapHlMocks mocks;
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		const char* config = "pretend this is a valid JSON string";

		STRICT_EXPECTED_CALL(mocks, json_parse_string(config));
		STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_value_get_array(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(IDENTITY_MAP_CONFIG)));
		STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.SetReturn((size_t)1);
		STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
			.IgnoreArgument(1)
			.IgnoreArgument(2)
			.SetFailReturn((JSON_Object*)NULL);

		//Act
		auto n = Module_Create(bus, config);

		///Assert
		ASSERT_IS_NULL(n);
		mocks.AssertActualAndExpectedCalls();

		///Cleanup
	}

	//Tests_SRS_IDMAP_HL_17_019: [ If creating the vector fails, then IdentityMap_HL_Create shall fail and return NULL. ]
	TEST_FUNCTION(IdentityMap_HL_Create_vector_create_returns_null)
	{
		///Arrange
		CIdentitymapHlMocks mocks;
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		const char* config = "pretend this is a valid JSON string";

		STRICT_EXPECTED_CALL(mocks, json_parse_string(config));
		STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_value_get_array(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(IDENTITY_MAP_CONFIG)))
			.SetFailReturn((VECTOR_HANDLE)NULL);

		//Act
		auto n = Module_Create(bus, config);

		///Assert
		ASSERT_IS_NULL(n);
		mocks.AssertActualAndExpectedCalls();

		///Cleanup
	}

	//Tests_SRS_IDMAP_HL_17_005: [ If configuration is not a JSON array of JSON objects, then IdentityMap_HL_Create shall fail and return NULL. ]
	TEST_FUNCTION(IdentityMap_HL_Create_no_array_returns_null)
	{
		///Arrange
		CIdentitymapHlMocks mocks;
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		const char* config = "pretend this is a valid JSON string";

		STRICT_EXPECTED_CALL(mocks, json_parse_string(config));
		STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, json_value_get_array(IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.SetFailReturn((JSON_Array*)NULL);

		//Act
		auto n = Module_Create(bus, config);

		///Assert
		ASSERT_IS_NULL(n);
		mocks.AssertActualAndExpectedCalls();

		///Cleanup
	}

	//Tests_SRS_IDMAP_HL_17_005: [ If configuration is not a JSON array of JSON objects, then IdentityMap_HL_Create shall fail and return NULL. ]
	TEST_FUNCTION(IdentityMap_HL_Create_parse_fails_returns_null)
	{
		///Arrange
		CIdentitymapHlMocks mocks;
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		const char* config = "pretend this is a valid JSON string";

		STRICT_EXPECTED_CALL(mocks, json_parse_string(config))
			.SetFailReturn((JSON_Value*)NULL);

		//Act
		auto n = Module_Create(bus, config);

		///Assert
		ASSERT_IS_NULL(n);
		mocks.AssertActualAndExpectedCalls();

		///Cleanup
	}

	//Tests_SRS_IDMAP_HL_17_017: [ IdentityMap_HL_Destroy shall free all used resources. ]
	TEST_FUNCTION(IdentityMap_HL_Destroy_does_everything)
	{
		///arrange
		CIdentitymapHlMocks mocks;
		const char * validJsonString = "calling it valid makes it so";
		MESSAGE_BUS_HANDLE busHandle = (MESSAGE_BUS_HANDLE)0x42;
		auto result = Module_Create(busHandle, validJsonString);
		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, MODULE_STATIC_GETAPIS(IDENTITYMAP_MODULE)());
		STRICT_EXPECTED_CALL(mocks, IdentityMap_Destroy(result));

		///act
		Module_Destroy(result);

		///assert
		mocks.AssertActualAndExpectedCalls();
		///cleanup
	}

	//Tests_SRS_IDMAP_HL_17_018: [ IdentityMap_HL_Receive shall pass the received parameters to the underlying identity map module receive function. ]
	TEST_FUNCTION(IdentityMap_HL_Receive_does_everything)
	{
		///arrange
		CIdentitymapHlMocks mocks;
		const char * validJsonString = "calling it valid makes it so";
		MESSAGE_BUS_HANDLE busHandle = (MESSAGE_BUS_HANDLE)0x42;
		MESSAGE_HANDLE messageHandle = (MESSAGE_HANDLE)0x42;
		auto result = Module_Create(busHandle, validJsonString);
		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, MODULE_STATIC_GETAPIS(IDENTITYMAP_MODULE)());
		STRICT_EXPECTED_CALL(mocks, IdentityMap_Receive(result, messageHandle));

		///act
		Module_Receive(result, messageHandle);

		///assert
		mocks.AssertActualAndExpectedCalls();

		///cleanup
		Module_Destroy(result);
	}

END_TEST_SUITE(identitymap_hl_unittests)
