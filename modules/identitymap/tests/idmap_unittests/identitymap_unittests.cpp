// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"
#include "azure_c_shared_utility/map.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/vector.h"
#include "messageproperties.h"


static MICROMOCK_MUTEX_HANDLE g_testByTest;
static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

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

#include "identitymap.h"
#include "azure_c_shared_utility/crt_abstractions.h"

static size_t currentmalloc_call;
static size_t whenShallmalloc_fail;

static size_t currentMap_call;
static size_t whenShallMap_fail;

static size_t currentConstMap_Create_call;
static size_t whenShallConstMap_Create_fail;

static size_t currentConstMap_Clone_call;
static size_t whenShallConstMap_Clone_fail;

static size_t currentConstMap_CloneWriteable_call;
static size_t whenShallConstMap_CloneWriteable_fail;

static size_t currentCONSTBUFFER_Create_call;
static size_t whenShallCONSTBUFFER_Create_fail;

static size_t currentCONSTBUFFER_Clone_call;
static size_t whenShallCONSTBUFFER_Clone_fail;

static size_t currentStrdup_call;
static size_t whenShallStrdup_fail;

static size_t currentVectorElement_call;
static size_t whenShallVectorElement_fail;

static size_t currentMessage_call;
static size_t whenShallMessage_fail;
static CONSTBUFFER messageContent;

class RefCountObject
{
private:
	size_t ref_count;

public:
	RefCountObject() : ref_count(1)
	{
	}

	size_t inc_ref()
	{
		return ++ref_count;
	}

	void dec_ref()
	{
		if (--ref_count == 0)
		{
			delete this;
		}
	}
};

#define VALID_MAP_HANDLE	0xDEAF
#define VALID_VALUE			"value"
static MAP_RESULT currentMapResult;

static MESSAGE_BUS_RESULT currentMessageBusResult;

//CONSTMAP GetValue mocks
static const char* macAddressProperties;
static const char* sourceProperties;
static const char* deviceNameProperties;
static const char* deviceKeyProperties;

static VECTOR_HANDLE testVector1;
static VECTOR_HANDLE testVector2;

TYPED_MOCK_CLASS(CIdentitymapMocks, CGlobalMock)
	{
	public:

	// memory
	MOCK_STATIC_METHOD_1(, void*, gballoc_malloc, size_t, size)
		void* result2;
		currentmalloc_call++;
		if (whenShallmalloc_fail>0)
		{
			if (currentmalloc_call == whenShallmalloc_fail)
			{
				result2 = NULL;
			}
			else
			{
				result2 = BASEIMPLEMENTATION::gballoc_malloc(size);
			}
		}
		else
		{
			result2 = BASEIMPLEMENTATION::gballoc_malloc(size);
		}
	MOCK_METHOD_END(void*, result2)

	MOCK_STATIC_METHOD_2(, void*, gballoc_realloc, void*, ptr, size_t, size)
	MOCK_METHOD_END(void*, BASEIMPLEMENTATION::gballoc_realloc(ptr, size));

	MOCK_STATIC_METHOD_1(, void, gballoc_free, void*, ptr)
		BASEIMPLEMENTATION::gballoc_free(ptr);
	MOCK_VOID_METHOD_END()


	// MessageBus mocks
	MOCK_STATIC_METHOD_0(, MESSAGE_BUS_HANDLE, MessageBus_Create)
		MESSAGE_BUS_HANDLE busResult = (MESSAGE_BUS_HANDLE)(new RefCountObject());
	MOCK_METHOD_END(MESSAGE_BUS_HANDLE, busResult)

	MOCK_STATIC_METHOD_1(, void, MessageBus_Destroy, MESSAGE_BUS_HANDLE, bus)
		((RefCountObject*)bus)->dec_ref();
	MOCK_VOID_METHOD_END()

	MOCK_STATIC_METHOD_3(, MESSAGE_BUS_RESULT, MessageBus_Publish, MESSAGE_BUS_HANDLE, bus, MODULE_HANDLE, source, MESSAGE_HANDLE, message)
		MESSAGE_BUS_RESULT busResult = currentMessageBusResult;
	MOCK_METHOD_END(MESSAGE_BUS_RESULT, busResult)

	// ConstMap mocks
	MOCK_STATIC_METHOD_1(, CONSTMAP_HANDLE, ConstMap_Create, MAP_HANDLE, sourceMap)
		CONSTMAP_HANDLE result2;
		currentConstMap_Create_call ++;
		if (whenShallConstMap_Create_fail == currentConstMap_Create_call)
		{
			result2 = NULL;
		}
		else
		{
			result2 = (CONSTMAP_HANDLE)(new RefCountObject());
		}
	MOCK_METHOD_END(CONSTMAP_HANDLE, result2)

	MOCK_STATIC_METHOD_1(, CONSTMAP_HANDLE, ConstMap_Clone, CONSTMAP_HANDLE, handle)
		CONSTMAP_HANDLE result3;
		currentConstMap_Clone_call++;
		if (whenShallConstMap_Clone_fail == currentConstMap_Clone_call)
		{
			result3 = NULL;
		}
		else
		{
			result3 = handle;
			((RefCountObject*)handle)->inc_ref();
		}
		MOCK_METHOD_END(CONSTMAP_HANDLE, result3)

	MOCK_STATIC_METHOD_1(, MAP_HANDLE, ConstMap_CloneWriteable, CONSTMAP_HANDLE, handle)
		MAP_HANDLE result4;
		currentConstMap_CloneWriteable_call++;
		if (currentConstMap_CloneWriteable_call == whenShallConstMap_CloneWriteable_fail)
			result4 = NULL;
		else
			result4 = (MAP_HANDLE)(new RefCountObject());
	MOCK_METHOD_END(MAP_HANDLE, result4)

	MOCK_STATIC_METHOD_1(, void, ConstMap_Destroy, CONSTMAP_HANDLE, map)
		((RefCountObject*)map)->dec_ref();
	MOCK_VOID_METHOD_END()

	MOCK_STATIC_METHOD_2(, const char*, ConstMap_GetValue, CONSTMAP_HANDLE, handle, const char*, key)
		const char * result5 = VALID_VALUE;
		if (strcmp(GW_MAC_ADDRESS_PROPERTY, key) == 0)
		{
			result5 = macAddressProperties;
		}
		else if (strcmp(GW_SOURCE_PROPERTY, key) == 0)
		{
			result5 = sourceProperties;
		}
		else if (strcmp(GW_DEVICENAME_PROPERTY, key) == 0)
		{
			result5 = deviceNameProperties;
		}
		else if (strcmp(GW_DEVICEKEY_PROPERTY, key) == 0)
		{
			result5 = deviceKeyProperties;
		}
	MOCK_METHOD_END(const char *, result5)

	// CONSTBUFFER mocks.
	MOCK_STATIC_METHOD_2(, CONSTBUFFER_HANDLE, CONSTBUFFER_Create, const unsigned char*, source, size_t, size)
		CONSTBUFFER_HANDLE result1;
		currentCONSTBUFFER_Create_call++;
		if (whenShallCONSTBUFFER_Create_fail == currentCONSTBUFFER_Create_call)
		{
			result1 = NULL;
		}
		else
		{
			result1 = (CONSTBUFFER_HANDLE)(new RefCountObject());
		}
	MOCK_METHOD_END(CONSTBUFFER_HANDLE, result1)

	MOCK_STATIC_METHOD_1(, CONSTBUFFER_HANDLE, CONSTBUFFER_Clone, CONSTBUFFER_HANDLE, constbufferHandle)
		CONSTBUFFER_HANDLE result2;
		currentCONSTBUFFER_Clone_call++;
		if (currentCONSTBUFFER_Clone_call == whenShallCONSTBUFFER_Clone_fail)
		{
			result2 = NULL;
		}
		else
		{
			result2 = constbufferHandle;
			((RefCountObject*)constbufferHandle)->inc_ref();
		}
	MOCK_METHOD_END(CONSTBUFFER_HANDLE, result2)

	MOCK_STATIC_METHOD_1(, void, CONSTBUFFER_Destroy, CONSTBUFFER_HANDLE, constbufferHandle)
		((RefCountObject*)constbufferHandle)->dec_ref();
	MOCK_VOID_METHOD_END()


	// Map related

	// Map_Clone
	MOCK_STATIC_METHOD_1(, MAP_HANDLE, Map_Clone, MAP_HANDLE, sourceMap)
		((RefCountObject*)sourceMap)->inc_ref();
	MOCK_METHOD_END(MAP_HANDLE, sourceMap)

	// Map_Destroy
	MOCK_STATIC_METHOD_1(, void, Map_Destroy, MAP_HANDLE, ptr)
		((RefCountObject*)ptr)->dec_ref();
	MOCK_VOID_METHOD_END()

	// Map_AddOrUpdate
	MOCK_STATIC_METHOD_3(, MAP_RESULT, Map_AddOrUpdate, MAP_HANDLE, handle, const char*, key, const char*, value)
		MAP_RESULT result7;
		currentMap_call++;
		if (currentMap_call == whenShallMap_fail)
		{
			result7 = MAP_ERROR;
		}
		else
		{
			result7 = MAP_OK;
		}
	MOCK_METHOD_END(MAP_RESULT, result7)

	// Map_Delete
		MOCK_STATIC_METHOD_2(, MAP_RESULT, Map_Delete, MAP_HANDLE, handle, const char*, key)
		MOCK_METHOD_END(MAP_RESULT, MAP_OK)


	// Message
	MOCK_STATIC_METHOD_1(, MESSAGE_HANDLE, Message_Create, const MESSAGE_CONFIG*, cfg)
		MESSAGE_HANDLE result2 = (MESSAGE_HANDLE)(new RefCountObject());
	MOCK_METHOD_END(MESSAGE_HANDLE, result2)

	MOCK_STATIC_METHOD_1(, MESSAGE_HANDLE, Message_CreateFromBuffer, const MESSAGE_BUFFER_CONFIG*, cfg)
			MESSAGE_HANDLE result1;
			currentMessage_call++;
			if (currentMessage_call == whenShallMessage_fail)
			{
				result1 = NULL;
			}
			else
			{
				result1 = (MESSAGE_HANDLE)(new RefCountObject());
			}
	MOCK_METHOD_END(MESSAGE_HANDLE, result1)

	MOCK_STATIC_METHOD_1(, MESSAGE_HANDLE, Message_Clone, MESSAGE_HANDLE, message)
		((RefCountObject*)message)->inc_ref();
	MOCK_METHOD_END(MESSAGE_HANDLE, message)

	MOCK_STATIC_METHOD_1(, CONSTMAP_HANDLE, Message_GetProperties, MESSAGE_HANDLE, message)
		CONSTMAP_HANDLE result1;
		currentMessage_call++;
		if (currentMessage_call == whenShallMessage_fail)
		{
			result1 = NULL;
		}
		else
		{
			result1 = ConstMap_Create((MAP_HANDLE)VALID_MAP_HANDLE);
		}
	MOCK_METHOD_END(CONSTMAP_HANDLE, result1)

	MOCK_STATIC_METHOD_1(, const CONSTBUFFER*, Message_GetContent, MESSAGE_HANDLE, message)
		CONSTBUFFER* result1 = &messageContent;
	MOCK_METHOD_END(const CONSTBUFFER*, result1)

	MOCK_STATIC_METHOD_1(, CONSTBUFFER_HANDLE, Message_GetContentHandle, MESSAGE_HANDLE, message)
		CONSTBUFFER_HANDLE result1;
		currentMessage_call++;
		if (currentMessage_call == whenShallMessage_fail)
		{
			result1 = NULL;
		}
		else
		{
			result1 = CONSTBUFFER_Create(NULL, 0);
		}
	MOCK_METHOD_END(CONSTBUFFER_HANDLE, result1)

	MOCK_STATIC_METHOD_1(, void, Message_Destroy, MESSAGE_HANDLE, message)
		((RefCountObject*)message)->dec_ref();
	MOCK_VOID_METHOD_END()

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
		currentVectorElement_call++;
		void * result1;
		if (currentVectorElement_call = whenShallVectorElement_fail)
		{
			result1 = NULL;
		}
		else
		{
			result1 = BASEIMPLEMENTATION::VECTOR_element(handle, index);
		}
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

	// crt_abstractions.h

	MOCK_STATIC_METHOD_2(, int, mallocAndStrcpy_s, char**, destination, const char*, source)
		currentStrdup_call++;
		int r;
		if (currentStrdup_call == whenShallStrdup_fail)
		{
			r = 1;
		}
		else
		{
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
		}
	MOCK_METHOD_END(int,r)


	MOCK_STATIC_METHOD_3(, int, unsignedIntToString, char*, destination, size_t, destinationSize, unsigned int, value)
	MOCK_METHOD_END(int, 0)

	MOCK_STATIC_METHOD_3(, int, size_tToString, char*, destination, size_t, destinationSize, size_t, value)
	MOCK_METHOD_END(int, 0)

	};

DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_2(CIdentitymapMocks, , void*, gballoc_realloc, void*, ptr, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapMocks, , void, gballoc_free, void*, ptr);

DECLARE_GLOBAL_MOCK_METHOD_0(CIdentitymapMocks, , MESSAGE_BUS_HANDLE, MessageBus_Create);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapMocks, , void, MessageBus_Destroy, MESSAGE_BUS_HANDLE, bus);
DECLARE_GLOBAL_MOCK_METHOD_3(CIdentitymapMocks, , MESSAGE_BUS_RESULT, MessageBus_Publish, MESSAGE_BUS_HANDLE, bus, MODULE_HANDLE, source, MESSAGE_HANDLE, message);

DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapMocks, , CONSTMAP_HANDLE, ConstMap_Create, MAP_HANDLE, sourceMap);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapMocks, , CONSTMAP_HANDLE, ConstMap_Clone, CONSTMAP_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapMocks, , void, ConstMap_Destroy, CONSTMAP_HANDLE, map);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapMocks, , MAP_HANDLE, ConstMap_CloneWriteable, CONSTMAP_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_2(CIdentitymapMocks, , const char*, ConstMap_GetValue, CONSTMAP_HANDLE, handle, const char *, key);

DECLARE_GLOBAL_MOCK_METHOD_2(CIdentitymapMocks, , CONSTBUFFER_HANDLE, CONSTBUFFER_Create, const unsigned char*, source, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapMocks, , CONSTBUFFER_HANDLE, CONSTBUFFER_Clone, CONSTBUFFER_HANDLE, constbufferHandle);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapMocks, , void, CONSTBUFFER_Destroy, CONSTBUFFER_HANDLE, constbufferHandle);

DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapMocks, , MAP_HANDLE, Map_Clone, MAP_HANDLE, sourceMap);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapMocks, , void, Map_Destroy, MAP_HANDLE, ptr);
DECLARE_GLOBAL_MOCK_METHOD_3(CIdentitymapMocks, , MAP_RESULT, Map_AddOrUpdate, MAP_HANDLE, handle, const char*, key, const char*, value);
DECLARE_GLOBAL_MOCK_METHOD_2(CIdentitymapMocks, , MAP_RESULT, Map_Delete, MAP_HANDLE, handle, const char*, key);

DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapMocks, , MESSAGE_HANDLE, Message_Create, const MESSAGE_CONFIG*, cfg);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapMocks, , MESSAGE_HANDLE, Message_CreateFromBuffer, const MESSAGE_BUFFER_CONFIG*, cfg);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapMocks, , MESSAGE_HANDLE, Message_Clone, MESSAGE_HANDLE, message);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapMocks, , CONSTMAP_HANDLE, Message_GetProperties, MESSAGE_HANDLE, message);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapMocks, , const CONSTBUFFER*, Message_GetContent, MESSAGE_HANDLE, message);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapMocks, , CONSTBUFFER_HANDLE, Message_GetContentHandle, MESSAGE_HANDLE, message);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapMocks, , void, Message_Destroy, MESSAGE_HANDLE, message);

// vector.h
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapMocks, , VECTOR_HANDLE, VECTOR_create, size_t, elementSize);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapMocks, , void, VECTOR_destroy, VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_3(CIdentitymapMocks, , int, VECTOR_push_back, VECTOR_HANDLE, handle, const void*, elements, size_t, numElements);
DECLARE_GLOBAL_MOCK_METHOD_3(CIdentitymapMocks, , void, VECTOR_erase, VECTOR_HANDLE, handle, void*, elements, size_t, numElements);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapMocks, , void, VECTOR_clear, VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_2(CIdentitymapMocks, , void*, VECTOR_element, const VECTOR_HANDLE, handle, size_t, index);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapMocks, , void*, VECTOR_front, const VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapMocks, , void*, VECTOR_back, const VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_3(CIdentitymapMocks, , void*, VECTOR_find_if, const VECTOR_HANDLE, handle, PREDICATE_FUNCTION, pred, const void*, value);
DECLARE_GLOBAL_MOCK_METHOD_1(CIdentitymapMocks, , size_t, VECTOR_size, const VECTOR_HANDLE, handle);

DECLARE_GLOBAL_MOCK_METHOD_2(CIdentitymapMocks, , int, mallocAndStrcpy_s, char**, destination, const char*, source);
DECLARE_GLOBAL_MOCK_METHOD_3(CIdentitymapMocks, , int, unsignedIntToString, char*, destination, size_t, destinationSize, unsigned int, value);
DECLARE_GLOBAL_MOCK_METHOD_3(CIdentitymapMocks, , int, size_tToString, char*, destination, size_t, destinationSize, size_t, value);

BEGIN_TEST_SUITE(identitymap_unittests)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
        g_testByTest = MicroMockCreateMutex();
        ASSERT_IS_NOT_NULL(g_testByTest);
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

		CIdentitymapMocks mocks;

        currentmalloc_call = 0;
        whenShallmalloc_fail = 0;

		currentConstMap_Create_call = 0;
		whenShallConstMap_Create_fail = 0;
		currentConstMap_Clone_call = 0;
		whenShallConstMap_Clone_fail = 0;
		currentCONSTBUFFER_Create_call = 0;
		whenShallCONSTBUFFER_Create_fail = 0;
		currentCONSTBUFFER_Clone_call = 0;
		whenShallCONSTBUFFER_Clone_fail = 0;
		currentStrdup_call = 0;
		whenShallStrdup_fail = 0;
		currentVectorElement_call = 0;
		whenShallVectorElement_fail = 0;
		macAddressProperties = NULL;
		sourceProperties = NULL;
		deviceNameProperties = NULL;
		deviceKeyProperties = NULL;
		currentMessage_call = 0;
		whenShallMessage_fail = 0;
		currentConstMap_CloneWriteable_call = 0;
		whenShallConstMap_CloneWriteable_fail = 0;
		currentMap_call = 0;
		whenShallMap_fail = 0;
		currentMessageBusResult = MESSAGE_BUS_OK;

		testVector1 = VECTOR_create(sizeof(IDENTITY_MAP_CONFIG));
		IDENTITY_MAP_CONFIG c1 =
		{
			"aa:Aa:bb:bB:cc:CC",
			"aNiceDevice",
			"aNiceKey"
		};
		VECTOR_push_back(testVector1, &c1, 1);

		testVector2 = VECTOR_create(sizeof(IDENTITY_MAP_CONFIG));
		IDENTITY_MAP_CONFIG c2 =
		{
			"aa:Aa:bb:bB:cc:BB",
			"a2ndDevice",
			"a2ndKey"
		};
		VECTOR_push_back(testVector2, &c1, 1);
		VECTOR_push_back(testVector2, &c2, 1);
		mocks.ResetAllCalls();

    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
        if (!MicroMockReleaseMutex(g_testByTest))
        {
            ASSERT_FAIL("failure in test framework at ReleaseMutex");
        }
		CIdentitymapMocks mocks;
		VECTOR_destroy(testVector1);
		VECTOR_destroy(testVector2);
		mocks.ResetAllCalls();

    }

	/*Tests_SRS_IDMAP_17_001 [ Module_GetAPIs shall return a non-NULL pointer to a MODULE_APIS structure.] */
	/*Tests_SRS_IDMAP_17_002: [The MODULE_APIS structure shall have non-NULL Module_Create, Module_Destroy, and Module_Receive fields.]*/
	TEST_FUNCTION(IdentityMap_Module_GetAPIs_Success)
	{
		///Arrange
		CIdentitymapMocks mocks;

		///Act
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		///Assert
		ASSERT_IS_NOT_NULL(theAPIS);
		ASSERT_IS_NOT_NULL((void*)theAPIS->Module_Create);
		ASSERT_IS_NOT_NULL((void*)theAPIS->Module_Destroy);
		ASSERT_IS_NOT_NULL((void*)theAPIS->Module_Receive);

		///Ablution
	}

	/*Tests_SRS_IDMAP_17_004: [If the busHandle is NULL, this function shall fail and return NULL.]*/
	TEST_FUNCTION(IdentityMap_Create_MessageBus_Null)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		MESSAGE_BUS_HANDLE bus = NULL;
		unsigned char config;

		///Act
		auto n = theAPIS->Module_Create(bus, &config);

		///Assert
		ASSERT_IS_NULL(n);

		///Ablution
	}

	/*Tests_SRS_IDMAP_17_005: [If the configuration is NULL, this function shall fail and return NULL.]*/
	TEST_FUNCTION(IdentityMap_Create_Config_Null)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		void * config = NULL;

		///Act
		auto n = theAPIS->Module_Create(bus, config);

		///Assert
		ASSERT_IS_NULL(n);

		///Ablution
	}

	/*Tests_SRS_IDMAP_17_003: [Upon success, this function shall return a valid pointer to a MODULE_HANDLE.]*/
	TEST_FUNCTION(IdentityMap_Create_Success_SingleEntry)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;


		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module struct*/
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the d2c internal array*/
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the c2d internal array*/
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the mac address*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device name*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device key*/
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the mac address*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device name*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device key*/
			.IgnoreAllArguments();

		///Act
		auto n = theAPIS->Module_Create(bus, testVector1);

		///Assert
		ASSERT_IS_NOT_NULL(n);
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		theAPIS->Module_Destroy(n);
	}

	/*Tests_SRS_IDMAP_17_041: [If the configuration has no vector elements, this function shall fail and return NULL.]*/
	TEST_FUNCTION(IdentityMap_Create_ValidateConfig_Empty_Vector)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		VECTOR_HANDLE v = VECTOR_create(sizeof(IDENTITY_MAP_CONFIG));

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);


		///Act
		auto n = theAPIS->Module_Create(bus, v);

		///Assert
		ASSERT_IS_NULL(n);
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		VECTOR_destroy(v);
	}

	/*Tests_SRS_IDMAP_17_019: [If any macAddress, deviceId or deviceKey are NULL, this function shall fail and return NULL.]*/
	TEST_FUNCTION(IdentityMap_Create_ValidateConfig_NULL_fields)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		VECTOR_HANDLE v1 = VECTOR_create(sizeof(IDENTITY_MAP_CONFIG));
		IDENTITY_MAP_CONFIG c1 =
		{
			"aa:Aa:bb:bB:cc:CC",
			NULL,
			"aNiceKey"
		};
		VECTOR_push_back(v1, &c1, 1);
		VECTOR_HANDLE v2 = VECTOR_create(sizeof(IDENTITY_MAP_CONFIG));
		IDENTITY_MAP_CONFIG c2 =
		{
			"aa:Aa:bb:bB:cc:CC",
			"aNiceDevice",
			NULL
		};
		VECTOR_push_back(v2, &c2, 1);
		VECTOR_HANDLE v3 = VECTOR_create(sizeof(IDENTITY_MAP_CONFIG));
		IDENTITY_MAP_CONFIG c3 =
		{
			NULL,
			"aNiceDevice",
			"aNiceKey"
		};
		VECTOR_push_back(v3, &c3, 1);
		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0)).IgnoreArgument(1);

		///Act
		auto n1 = theAPIS->Module_Create(bus, v1);
		ASSERT_IS_NULL(n1);
		auto n2 = theAPIS->Module_Create(bus, v2);
		ASSERT_IS_NULL(n2);
		auto n3 = theAPIS->Module_Create(bus, v3);
		ASSERT_IS_NULL(n3);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		VECTOR_destroy(v1);
		VECTOR_destroy(v2);
		VECTOR_destroy(v3);
	}

	/*Tests_SRS_IDMAP_17_006: [If any macAddress string in configuration is not a MAC address in canonical form, this function shall fail and return NULL.]*/
	TEST_FUNCTION(IdentityMap_Create_ValidateConfig_non_Canon_Mac1)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		VECTOR_HANDLE v1 = VECTOR_create(sizeof(IDENTITY_MAP_CONFIG));
		// wrong length
		IDENTITY_MAP_CONFIG c1 =
		{
			"aa-Aa-bb-B-cc-CC",
			"aNiceDevice",
			"aNiceKey"
		};
		VECTOR_push_back(v1, &c1, 1);

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0)).IgnoreArgument(1);


		///Act
		auto n1 = theAPIS->Module_Create(bus, v1);
		ASSERT_IS_NULL(n1);


		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		VECTOR_destroy(v1);

	}

	/*Tests_SRS_IDMAP_17_006: [If any macAddress string in configuration is not a MAC address in canonical form, this function shall fail and return NULL.]*/
	TEST_FUNCTION(IdentityMap_Create_ValidateConfig_non_Canon_Mac2)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;

		VECTOR_HANDLE v2 = VECTOR_create(sizeof(IDENTITY_MAP_CONFIG));
		// incorrect hexadecimal
		IDENTITY_MAP_CONFIG c2 =
		{
			"aa:Aa:bb:bB:cg:CC",
			"aNiceDevice",
			"aNiceKey"
		};
		VECTOR_push_back(v2, &c2, 1);

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0)).IgnoreArgument(1);


		///Act

		auto n2 = theAPIS->Module_Create(bus, v2);
		ASSERT_IS_NULL(n2);


		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution

		VECTOR_destroy(v2);
	}

	/*Tests_SRS_IDMAP_17_006: [If any macAddress string in configuration is not a MAC address in canonical form, this function shall fail and return NULL.]*/
	TEST_FUNCTION(IdentityMap_Create_ValidateConfig_non_Canon_Mac3)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;

		VECTOR_HANDLE v3 = VECTOR_create(sizeof(IDENTITY_MAP_CONFIG));
		// Valid character instead of dash
		IDENTITY_MAP_CONFIG c3 =
		{
			"aa-Aa-bb-bB-cccCC",
			"aNiceDevice",
			"aNiceKey"
		};
		VECTOR_push_back(v3, &c3, 1);
		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0)).IgnoreArgument(1);


		///Act

		auto n3 = theAPIS->Module_Create(bus, v3);
		ASSERT_IS_NULL(n3);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution

		VECTOR_destroy(v3);
	}

	/*Tests_SRS_IDMAP_17_010: [If IdentityMap_Create fails to allocate a new IDENTITY_MAP_DATA structure, then this function shall fail, and return NULL.]*/
	TEST_FUNCTION(IdentityMap_Create_Module_alloc_fails)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;

		whenShallmalloc_fail = 1;
		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module struct*/
			.IgnoreArgument(1);


		///Act
		auto n = theAPIS->Module_Create(bus, testVector1);

		///Assert
		ASSERT_IS_NULL(n);
		mocks.AssertActualAndExpectedCalls();

		///Ablution
	}

	/*Tests_SRS_IDMAP_17_011: [If IdentityMap_Create fails to create memory for the macToDeviceArray, then this function shall fail and return NULL.]*/
	TEST_FUNCTION(IdentityMap_Create_internal_d2c_alloc_fail)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;

		whenShallmalloc_fail = 2;

		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module struct*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);


		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the internal array*/
			.IgnoreArgument(1);


		///Act
		auto n = theAPIS->Module_Create(bus, testVector1);

		///Assert
		ASSERT_IS_NULL(n);
		mocks.AssertActualAndExpectedCalls();

		///Ablution
	}

	/*Tests_SRS_IDMAP_17_042: [ If IdentityMap_Create fails to create memory for the deviceToMacArray, then this function shall fail and return NULL. */
	TEST_FUNCTION(IdentityMap_Create_internal_c2d_alloc_fail)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;

		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module struct*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);


		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the internal array*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the internal array*/
			.IgnoreArgument(1)
			.SetFailReturn((void_ptr)NULL);


		///Act
		auto n = theAPIS->Module_Create(bus, testVector1);

		///Assert
		ASSERT_IS_NULL(n);
		mocks.AssertActualAndExpectedCalls();

		///Ablution
	}
	/*Tests_SRS_IDMAP_17_012: [If IdentityMap_Create fails to add a MAC address triplet to the macToDeviceArray, then this function shall fail, release all resources, and return NULL.]*/
	TEST_FUNCTION(IdentityMap_Create_DeepCopy_fail_mac1)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;

		whenShallStrdup_fail =7;

		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module struct*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the D2C internal array*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the C2D internal array*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1)).IgnoreArgument(1);

		/* 1st vector element */
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the mac address*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device name*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device key*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the mac address*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device name*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device key*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		/* 2nd vector element */
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the mac address*/
			.IgnoreAllArguments();

		///Act
		auto n = theAPIS->Module_Create(bus, testVector2);

		///Assert
		ASSERT_IS_NULL(n);
		mocks.AssertActualAndExpectedCalls();

		///Ablution
	}

	//Tests_SRS_IDMAP_17_043: [ If IdentityMap_Create fails to add a MAC address triplet to the deviceToMacArray, then this function shall fail, release all resources, and return NULL. 
	TEST_FUNCTION(IdentityMap_Create_DeepCopy_fail_mac2)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;

		whenShallStrdup_fail = 10;

		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module struct*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the D2C internal array*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the C2D internal array*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1)).IgnoreArgument(1);

		/* 1st vector element */
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the mac address*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device name*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device key*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the mac address*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device name*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device key*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		/* 2nd vector element */
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the mac address*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device name*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device key*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the mac address*/
			.IgnoreAllArguments();

		///Act
		auto n = theAPIS->Module_Create(bus, testVector2);

		///Assert
		ASSERT_IS_NULL(n);
		mocks.AssertActualAndExpectedCalls();

		///Ablution
	}

	/*Tests_SRS_IDMAP_17_012: [If IdentityMap_Create fails to add a MAC address triplet to the macToDeviceArray, then this function shall fail, release all resources, and return NULL.]*/
	TEST_FUNCTION(IdentityMap_Create_DeepCopy_fail_id1)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;


		whenShallStrdup_fail = 8;

		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module struct*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the d2c internal array*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the c2d internal array*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1)).IgnoreArgument(1);

		//1st vector element
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the mac address*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device name*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device key*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the mac address*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device name*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device key*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		/* 2nd vector element */
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the mac address*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the mac address*/
			.IgnoreAllArguments();

		///Act
		auto n = theAPIS->Module_Create(bus, testVector2);

		///Assert
		ASSERT_IS_NULL(n);
		mocks.AssertActualAndExpectedCalls();

		///Ablution
	}

	TEST_FUNCTION(IdentityMap_Create_DeepCopy_fail_id2)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;


		whenShallStrdup_fail = 11;

		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module struct*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the d2c internal array*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the c2d internal array*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1)).IgnoreArgument(1);

		//1st vector element
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the mac address*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device name*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device key*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the mac address*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device name*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device key*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		/* 2nd vector element */
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the mac address*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device name*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device key*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the mac address*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the mac address*/
			.IgnoreAllArguments();

		///Act
		auto n = theAPIS->Module_Create(bus, testVector2);

		///Assert
		ASSERT_IS_NULL(n);
		mocks.AssertActualAndExpectedCalls();

		///Ablution
	}


	/*Tests_SRS_IDMAP_17_012: [If IdentityMap_Create fails to add a MAC address triplet to the macToDeviceArray, then this function shall fail, release all resources, and return NULL.]*/
	TEST_FUNCTION(IdentityMap_Create_DeepCopy_fail_key1)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;

		whenShallStrdup_fail = 9;

		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module struct*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the D2C internal array*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the C2D internal array*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1)).IgnoreArgument(1);

		//1st vector element
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the mac address*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device name*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device key*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the mac address*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device name*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device key*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		// 2nd vector element
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the mac address*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device name*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device key*/
			.IgnoreAllArguments();



		///Act
		auto n = theAPIS->Module_Create(bus, testVector2);

		///Assert
		ASSERT_IS_NULL(n);
		mocks.AssertActualAndExpectedCalls();

		///Ablution
	}

	TEST_FUNCTION(IdentityMap_Create_DeepCopy_fail_key2)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;

		whenShallStrdup_fail = 12;

		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module struct*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the D2C internal array*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the C2D internal array*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1)).IgnoreArgument(1);

		//1st vector element
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the mac address*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device name*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device key*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the mac address*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device name*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device key*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		// 2nd vector element
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the mac address*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device name*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device key*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the mac address*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device name*/
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is for the device key*/
			.IgnoreAllArguments();



		///Act
		auto n = theAPIS->Module_Create(bus, testVector2);

		///Assert
		ASSERT_IS_NULL(n);
		mocks.AssertActualAndExpectedCalls();

		///Ablution
	}

	/*Tests_SRS_IDMAP_17_018: [If moduleHandle is NULL, IdentityMap_Destroy shall return.]*/
	TEST_FUNCTION(IdentityMap_Destroy_NULL)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		///Act
		theAPIS->Module_Destroy(NULL);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
	}

	/*Tests_SRS_IDMAP_17_015: [IdentityMap_Destroy shall release all resources allocated for the module.]*/
	TEST_FUNCTION(IdentityMap_Destroy_2Element)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		MESSAGE_BUS_HANDLE bus = MessageBus_Create();

		auto n = theAPIS->Module_Create(bus, testVector2);

		mocks.ResetAllCalls();

		//1st vector element
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		//2nd vector element
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		//internal struct and module data
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

		///Act
		theAPIS->Module_Destroy(n);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		MessageBus_Destroy(bus);

	}

	/*Tests_SRS_IDMAP_17_020: [If moduleHandle or messageHandle is NULL, then the function shall return.]*/
	TEST_FUNCTION(IdentityMap_Receive_Null_inputs)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;

		///Act
		theAPIS->Module_Receive((MODULE_HANDLE)&fake, NULL);
		theAPIS->Module_Receive(NULL, (MESSAGE_HANDLE)&fake);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
	}

	TEST_FUNCTION(IdentityMap_Receive_no_source)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = MessageBus_Create();
		auto n = theAPIS->Module_Create(bus, testVector2);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);


		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		theAPIS->Module_Destroy(n);
		MessageBus_Destroy(bus);

	}
	/*Tests_SRS_IDMAP_17_021: [If messageHandle properties does not contain "macAddress" property, then the function shall return.]*/
	TEST_FUNCTION(IdentityMap_Receive_D2C_no_Mac)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = MessageBus_Create();
		auto n = theAPIS->Module_Create(bus, testVector2);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		sourceProperties = GW_SOURCE_BLE_TELEMETRY;

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();


		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		theAPIS->Module_Destroy(n);
		MessageBus_Destroy(bus);

	}

	/*Tests_SRS_IDMAP_17_024: [If messageHandle properties contains properties "deviceName" and "deviceKey", then this function shall return.]*/
	TEST_FUNCTION(IdentityMap_Receive_has_D2C_device_name_and_key)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = MessageBus_Create();
		auto n = theAPIS->Module_Create(bus, testVector2);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		macAddressProperties = "aa:Aa:bb:bB:cc:BB";
		sourceProperties = GW_SOURCE_BLE_TELEMETRY;
		deviceKeyProperties = "APossiblyValidKey";
		deviceNameProperties = "APossiblyValidName";

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICEKEY_PROPERTY))
			.IgnoreArgument(1);


		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		theAPIS->Module_Destroy(n);
		MessageBus_Destroy(bus);

	}

	/*Tests_SRS_IDMAP_17_024: [If messageHandle properties contains properties "deviceName" and "deviceKey", then this function shall return.]*/
	TEST_FUNCTION(IdentityMap_when_received_D2C_message_has_has_device_name_key_and_any_source_then_no_republish_happens)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = MessageBus_Create();
		auto n = theAPIS->Module_Create(bus, testVector2);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		macAddressProperties = "aa:Aa:bb:bB:cc:BB";
		sourceProperties = "anySource";
		deviceKeyProperties = "APossiblyValidKey";
		deviceNameProperties = "APossiblyValidName";

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICEKEY_PROPERTY))
			.IgnoreArgument(1);


		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		theAPIS->Module_Destroy(n);
		MessageBus_Destroy(bus);

	}


	/*Tests_SRS_IDMAP_17_040: [If the macAddress of the message is not in canonical form, then this function shall return.]*/
	TEST_FUNCTION(IdentityMap_Receive_D2C_not_canon_mac)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = MessageBus_Create();
		auto n = theAPIS->Module_Create(bus, testVector2);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		macAddressProperties = GW_SOURCE_BLE_TELEMETRY;
		sourceProperties = GW_SOURCE_BLE_TELEMETRY;
		deviceNameProperties = "SomeDeviceName";

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICEKEY_PROPERTY))
			.IgnoreArgument(1);



		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		theAPIS->Module_Destroy(n);
		MessageBus_Destroy(bus);

	}

	/*Tests_SRS_IDMAP_17_025: [If the macAddress of the message is not found in the macToDeviceArray list, then this function shall return.] */
	TEST_FUNCTION(IdentityMap_Receive_D2C_mac_not_found)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = MessageBus_Create();
		auto n = theAPIS->Module_Create(bus, testVector1);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		macAddressProperties = "00:00:00:00:00:00";
		sourceProperties = GW_SOURCE_BLE_TELEMETRY;
		deviceNameProperties = "SomeDeviceName";

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICEKEY_PROPERTY))
			.IgnoreArgument(1);


		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		theAPIS->Module_Destroy(n);
		MessageBus_Destroy(bus);

	}

	/*Tests_SRS_IDMAP_17_021: [If messageHandle properties does not contain "macAddress" property, then the function shall return.]*/
	TEST_FUNCTION(IdentityMap_Receive_D2C_get_properties_2_fail)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = MessageBus_Create();
		auto n = theAPIS->Module_Create(bus, testVector2);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		macAddressProperties = "aa:aa:bb:bb:cc:cc";
		sourceProperties = GW_SOURCE_BLE_TELEMETRY;

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);

		whenShallMessage_fail = 2;
		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));


		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		theAPIS->Module_Destroy(n);
		MessageBus_Destroy(bus);

	}

	/*Tests_SRS_IDMAP_17_027: [If ConstMap_CloneWriteable fails, IdentityMap_Receive shall deallocate any resources and return.]*/
	TEST_FUNCTION(IdentityMap_Receive_D2C_CloneWriteable_fails)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = MessageBus_Create();
		auto n = theAPIS->Module_Create(bus, testVector2);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		macAddressProperties = "aa:aa:bb:bb:cc:cc";
		sourceProperties = GW_SOURCE_BLE_TELEMETRY;

		mocks.ResetAllCalls();


		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		whenShallConstMap_CloneWriteable_fail = 1;
		STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);


		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		theAPIS->Module_Destroy(n);
		MessageBus_Destroy(bus);

	}

	/*Tests_SRS_IDMAP_17_029: [If adding deviceName fails,IdentityMap_Receive shall deallocate all resources and return.]*/
	TEST_FUNCTION(IdentityMap_Receive_D2C_MapAdd1_fail)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = MessageBus_Create();
		auto n = theAPIS->Module_Create(bus, testVector2);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		macAddressProperties = "aa:aa:bb:bb:cc:cc";
		sourceProperties = GW_SOURCE_BLE_TELEMETRY;

		mocks.ResetAllCalls();


		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));

		STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		whenShallMap_fail = 1;
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY, "aNiceDevice")).IgnoreArgument(1);


		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		theAPIS->Module_Destroy(n);
		MessageBus_Destroy(bus);

	}

	/*Tests_SRS_IDMAP_17_031: [If adding deviceKey fails, IdentityMap_Receive shall deallocate all resources and return.]*/
	TEST_FUNCTION(IdentityMap_Receive_D2C_MapAdd2_fail)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = MessageBus_Create();
		auto n = theAPIS->Module_Create(bus, testVector2);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		macAddressProperties = "aa:aa:bb:bb:cc:cc";
		sourceProperties = GW_SOURCE_BLE_TELEMETRY;

		mocks.ResetAllCalls();


		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		whenShallMap_fail = 2;
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY, "aNiceDevice")).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_DEVICEKEY_PROPERTY, "aNiceKey")).IgnoreArgument(1);


		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		theAPIS->Module_Destroy(n);
		MessageBus_Destroy(bus);

	}

	/*Tests_SRS_IDMAP_17_033: [If adding source fails, IdentityMap_Receive shall deallocate all resources and return.]*/
	TEST_FUNCTION(IdentityMap_Receive_D2C_MapAdd3_fail)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = MessageBus_Create();
		auto n = theAPIS->Module_Create(bus, testVector2);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		macAddressProperties = "aa:aa:bb:bb:cc:cc";
		sourceProperties = GW_SOURCE_BLE_TELEMETRY;

		mocks.ResetAllCalls();


		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		whenShallMap_fail = 3;
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY, "aNiceDevice")).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_DEVICEKEY_PROPERTY, "aNiceKey")).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY, GW_IDMAP_MODULE)).IgnoreArgument(1);


		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		theAPIS->Module_Destroy(n);
		MessageBus_Destroy(bus);

	}

	/*Tests_SRS_IDMAP_17_035: [If cloning message content fails, IdentityMap_Receive shall deallocate all resources and return.]*/
	TEST_FUNCTION(IdentityMap_Receive_D2C_Message_GetContent_fail)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = MessageBus_Create();
		auto n = theAPIS->Module_Create(bus, testVector2);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		macAddressProperties = "aa:aa:bb:bb:cc:cc";
		sourceProperties = GW_SOURCE_BLE_TELEMETRY;

		mocks.ResetAllCalls();


		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY, "aNiceDevice")).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_DEVICEKEY_PROPERTY, "aNiceKey")).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY, GW_IDMAP_MODULE)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Delete(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
			.IgnoreArgument(1);
		whenShallMessage_fail = 3;
		STRICT_EXPECTED_CALL(mocks, Message_GetContentHandle(m));


		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		theAPIS->Module_Destroy(n);
		MessageBus_Destroy(bus);

	}

	/*Tests_SRS_IDMAP_17_037: [If creating new message fails, IdentityMap_Receive shall deallocate all resources and return.]*/
	TEST_FUNCTION(IdentityMap_Receive_D2C_Message_CreateFromBuffer_fail)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = MessageBus_Create();
		auto n = theAPIS->Module_Create(bus, testVector2);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		macAddressProperties = "aa:aa:bb:bb:cc:cc";
		sourceProperties = GW_SOURCE_BLE_TELEMETRY;

		mocks.ResetAllCalls();


		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY, "aNiceDevice"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_DEVICEKEY_PROPERTY, "aNiceKey"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY, GW_IDMAP_MODULE))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Delete(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Message_GetContentHandle(m));
		whenShallMessage_fail = 4;
		STRICT_EXPECTED_CALL(mocks, Message_CreateFromBuffer(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);


		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		theAPIS->Module_Destroy(n);
		MessageBus_Destroy(bus);

	}

	/*Tests_SRS_IDMAP_17_038: [IdentityMap_Receive shall call MessageBus_Publish with busHandle and new message.]*/
	TEST_FUNCTION(IdentityMap_Receive_D2C_MessageBus_Publish_fail)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = MessageBus_Create();
		auto n = theAPIS->Module_Create(bus, testVector2);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		macAddressProperties = "aa:aa:bb:bb:cc:cc";
		sourceProperties = GW_SOURCE_BLE_TELEMETRY;

		mocks.ResetAllCalls();



		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY, "aNiceDevice"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_DEVICEKEY_PROPERTY, "aNiceKey"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY, GW_IDMAP_MODULE))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Delete(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Message_GetContentHandle(m));
		STRICT_EXPECTED_CALL(mocks, Message_CreateFromBuffer(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Message_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		currentMessageBusResult = MESSAGE_BUS_ERROR;
		STRICT_EXPECTED_CALL(mocks, MessageBus_Publish(bus, n, IGNORED_PTR_ARG))
			.IgnoreArgument(3);


		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		theAPIS->Module_Destroy(n);
		MessageBus_Destroy(bus);

	}

	//Tests_SRS_IDMAP_17_054: [ If deleting the MAC Address fails, IdentityMap_Receive shall deallocate all resources and return. ]
	TEST_FUNCTION(IdentityMap_Receive_D2C_macaddress_delete_fails)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		VECTOR_HANDLE v = VECTOR_create(sizeof(IDENTITY_MAP_CONFIG));

		IDENTITY_MAP_CONFIG c1 = { "01:01:01:01:01:01", "Sensor1", "theKeyFor1" };
		IDENTITY_MAP_CONFIG c2 = { "02:02:02:02:02:02", "Sensor2", "theKeyFor2" };
		IDENTITY_MAP_CONFIG c3 = { "03:03:03:03:03:03", "Sensor3", "theKeyFor3" };
		IDENTITY_MAP_CONFIG c4 = { "04:04:04:04:04:04", "Sensor4", "theKeyFor4" };
		IDENTITY_MAP_CONFIG c5 = { "05:05:05:05:05:05", "Sensor5", "theKeyFor5" };
		IDENTITY_MAP_CONFIG c6 = { "06:06:06:06:06:06", "Sensor6", "theKeyFor6" };
		IDENTITY_MAP_CONFIG c7 = { "07:07:07:07:07:07", "Sensor7", "theKeyFor7" };
		IDENTITY_MAP_CONFIG c8 = { "08:08:08:08:08:08",	"Sensor8", "theKeyFor8" };
		IDENTITY_MAP_CONFIG c9 = { "09:09:09:09:09:09", "Sensor9",  "theKeyFor9" };
		VECTOR_push_back(v, &c1, 1);
		VECTOR_push_back(v, &c2, 1);
		VECTOR_push_back(v, &c3, 1);
		VECTOR_push_back(v, &c4, 1);
		VECTOR_push_back(v, &c5, 1);
		VECTOR_push_back(v, &c6, 1);
		VECTOR_push_back(v, &c7, 1);
		VECTOR_push_back(v, &c8, 1);
		VECTOR_push_back(v, &c9, 1);
		auto n = theAPIS->Module_Create(bus, v);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		macAddressProperties = "07:07:07:07:07:07";
		sourceProperties = GW_SOURCE_BLE_TELEMETRY;

		mocks.ResetAllCalls();


		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY, "Sensor7"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_DEVICEKEY_PROPERTY, "theKeyFor7"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY, GW_IDMAP_MODULE))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Delete(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
			.IgnoreArgument(1)
			.SetFailReturn((MAP_RESULT)MAP_KEYNOTFOUND);


		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		VECTOR_destroy(v);
		theAPIS->Module_Destroy(n);

	}

	/*Tests_SRS_IDMAP_17_026: [On a message which passes all checks, IdentityMap_Receive shall call ConstMap_CloneWriteable on the message properties.]*/
	/*Tests_SRS_IDMAP_17_028: [IdentityMap_Receive shall call Map_AddOrUpdate with key of "deviceName" and value of found deviceId.]*/
	/*Tests_SRS_IDMAP_17_032: [IdentityMap_Receive shall call Map_AddOrUpdate with key of "source" and value of "mapping".]*/
	/*Tests_SRS_IDMAP_17_030: [IdentityMap_Receive shall call Map_AddOrUpdate with key of "deviceKey" and value of found deviceKey.]*/
	/*Tests_SRS_IDMAP_17_053: [ IdentityMap_Receive shall call Map_Delete to remove the "macAddress" property. ]*/
	/*Tests_SRS_IDMAP_17_034: [IdentityMap_Receive shall clone message content.]*/
	/*Tests_SRS_IDMAP_17_036: [IdentityMap_Receive shall create a new message by calling Message_Create with new map and cloned content.]*/
	/*Tests_SRS_IDMAP_17_038: [IdentityMap_Receive shall call MessageBus_Publish with busHandle and new message.]*/
	/*Tests_SRS_IDMAP_17_039: [IdentityMap_Receive will destroy all resources it created.]*/
	TEST_FUNCTION(IdentityMap_Receive_D2C_Success)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		VECTOR_HANDLE v = VECTOR_create(sizeof(IDENTITY_MAP_CONFIG));

		IDENTITY_MAP_CONFIG c1 = { "01:01:01:01:01:01", "Sensor1", "theKeyFor1" };
		IDENTITY_MAP_CONFIG c2 = { "02:02:02:02:02:02", "Sensor2", "theKeyFor2" };
		IDENTITY_MAP_CONFIG c3 = { "03:03:03:03:03:03", "Sensor3", "theKeyFor3" };
		IDENTITY_MAP_CONFIG c4 = { "04:04:04:04:04:04", "Sensor4", "theKeyFor4" };
		IDENTITY_MAP_CONFIG c5 = { "05:05:05:05:05:05", "Sensor5", "theKeyFor5" };
		IDENTITY_MAP_CONFIG c6 = { "06:06:06:06:06:06", "Sensor6", "theKeyFor6" };
		IDENTITY_MAP_CONFIG c7 = { "07:07:07:07:07:07", "Sensor7", "theKeyFor7"	};
		IDENTITY_MAP_CONFIG c8 = { "08:08:08:08:08:08",	"Sensor8", "theKeyFor8" };
		IDENTITY_MAP_CONFIG c9 = { "09:09:09:09:09:09", "Sensor9",  "theKeyFor9" };
		VECTOR_push_back(v, &c1, 1);
		VECTOR_push_back(v, &c2, 1);
		VECTOR_push_back(v, &c3, 1);
		VECTOR_push_back(v, &c4, 1);
		VECTOR_push_back(v, &c5, 1);
		VECTOR_push_back(v, &c6, 1);
		VECTOR_push_back(v, &c7, 1);
		VECTOR_push_back(v, &c8, 1);
		VECTOR_push_back(v, &c9, 1);
		auto n = theAPIS->Module_Create(bus, v);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		macAddressProperties = "07:07:07:07:07:07";
		sourceProperties = GW_SOURCE_BLE_TELEMETRY;

		mocks.ResetAllCalls();


		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY, "Sensor7"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_DEVICEKEY_PROPERTY, "theKeyFor7"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY, GW_IDMAP_MODULE))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Delete(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Message_GetContentHandle(m));
		STRICT_EXPECTED_CALL(mocks, Message_CreateFromBuffer(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Message_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, MessageBus_Publish((MESSAGE_BUS_HANDLE)&fake, n, IGNORED_PTR_ARG))
			.IgnoreArgument(3);


		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		VECTOR_destroy(v);
		theAPIS->Module_Destroy(n);

	}

	//Tests_SRS_IDMAP_17_049: [ On a C2D message received, IdentityMap_Receive shall call ConstMap_CloneWriteable on the message properties. ]
	//Tests_SRS_IDMAP_17_051: [ IdentityMap_Receive shall call Map_AddOrUpdate with key of "macAddress" and value of found macAddress. ]
	//Tests_SRS_IDMAP_17_055: [ IdentityMap_Receive shall call Map_Delete to remove the "deviceName" property. ]
	//Tests_SRS_IDMAP_17_057: [ IdentityMap_Receive shall call Map_Delete to remove the "deviceKey" property. ]
	//Tests_SRS_IDMAP_17_032: [IdentityMap_Receive shall call Map_AddOrUpdate with key of "source" and value of "mapping".]
	//Tests_SRS_IDMAP_17_034: [IdentityMap_Receive shall clone message content.]
	//Tests_SRS_IDMAP_17_036: [IdentityMap_Receive shall create a new message by calling Message_CreateFromBuffer with new map and cloned content.]
	//Tests_SRS_IDMAP_17_038: [IdentityMap_Receive shall call MessageBus_Publish with busHandle and new message.]
	TEST_FUNCTION(IdentityMap_Receive_C2D_Success)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		VECTOR_HANDLE v = VECTOR_create(sizeof(IDENTITY_MAP_CONFIG));

		IDENTITY_MAP_CONFIG c1 = { "01:01:01:01:01:01", "Sensor1", "theKeyFor1" };
		IDENTITY_MAP_CONFIG c2 = { "02:02:02:02:02:02", "Sensor2", "theKeyFor2" };
		IDENTITY_MAP_CONFIG c3 = { "03:03:03:03:03:03", "Sensor3", "theKeyFor3" };
		IDENTITY_MAP_CONFIG c4 = { "04:04:04:04:04:04", "Sensor4", "theKeyFor4" };
		IDENTITY_MAP_CONFIG c5 = { "05:05:05:05:05:05", "Sensor5", "theKeyFor5" };
		IDENTITY_MAP_CONFIG c6 = { "06:06:06:06:06:06", "Sensor6", "theKeyFor6" };
		IDENTITY_MAP_CONFIG c7 = { "07:07:07:07:07:07", "Sensor7", "theKeyFor7" };
		IDENTITY_MAP_CONFIG c8 = { "08:08:08:08:08:08",	"Sensor8", "theKeyFor8" };
		IDENTITY_MAP_CONFIG c9 = { "09:09:09:09:09:09", "Sensor9",  "theKeyFor9" };
		VECTOR_push_back(v, &c1, 1);
		VECTOR_push_back(v, &c2, 1);
		VECTOR_push_back(v, &c3, 1);
		VECTOR_push_back(v, &c4, 1);
		VECTOR_push_back(v, &c5, 1);
		VECTOR_push_back(v, &c6, 1);
		VECTOR_push_back(v, &c7, 1);
		VECTOR_push_back(v, &c8, 1);
		VECTOR_push_back(v, &c9, 1);
		auto n = theAPIS->Module_Create(bus, v);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		deviceNameProperties = "Sensor7";
		sourceProperties = GW_IOTHUB_MODULE;

		mocks.ResetAllCalls();


		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);
            
		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY, "07:07:07:07:07:07"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY, GW_IDMAP_MODULE))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Delete(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Delete(IGNORED_PTR_ARG, GW_DEVICEKEY_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Message_GetContentHandle(m));
		STRICT_EXPECTED_CALL(mocks, Message_CreateFromBuffer(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Message_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, MessageBus_Publish((MESSAGE_BUS_HANDLE)&fake, n, IGNORED_PTR_ARG))
			.IgnoreArgument(3);


		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		VECTOR_destroy(v);
		theAPIS->Module_Destroy(n);

	}

	//Tests_SRS_IDMAP_17_057: [ IdentityMap_Receive shall call Map_Delete to remove the "deviceKey" property. ]
	TEST_FUNCTION(IdentityMap_Receive_C2D_no_key_key_delete)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		VECTOR_HANDLE v = VECTOR_create(sizeof(IDENTITY_MAP_CONFIG));

		IDENTITY_MAP_CONFIG c1 = { "01:01:01:01:01:01", "Sensor1", "theKeyFor1" };
		IDENTITY_MAP_CONFIG c2 = { "02:02:02:02:02:02", "Sensor2", "theKeyFor2" };
		IDENTITY_MAP_CONFIG c3 = { "03:03:03:03:03:03", "Sensor3", "theKeyFor3" };
		IDENTITY_MAP_CONFIG c4 = { "04:04:04:04:04:04", "Sensor4", "theKeyFor4" };
		IDENTITY_MAP_CONFIG c5 = { "05:05:05:05:05:05", "Sensor5", "theKeyFor5" };
		IDENTITY_MAP_CONFIG c6 = { "06:06:06:06:06:06", "Sensor6", "theKeyFor6" };
		IDENTITY_MAP_CONFIG c7 = { "07:07:07:07:07:07", "Sensor7", "theKeyFor7" };
		IDENTITY_MAP_CONFIG c8 = { "08:08:08:08:08:08",	"Sensor8", "theKeyFor8" };
		IDENTITY_MAP_CONFIG c9 = { "09:09:09:09:09:09", "Sensor9",  "theKeyFor9" };
		VECTOR_push_back(v, &c1, 1);
		VECTOR_push_back(v, &c2, 1);
		VECTOR_push_back(v, &c3, 1);
		VECTOR_push_back(v, &c4, 1);
		VECTOR_push_back(v, &c5, 1);
		VECTOR_push_back(v, &c6, 1);
		VECTOR_push_back(v, &c7, 1);
		VECTOR_push_back(v, &c8, 1);
		VECTOR_push_back(v, &c9, 1);
		auto n = theAPIS->Module_Create(bus, v);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		deviceNameProperties = "Sensor7";
		sourceProperties = GW_IOTHUB_MODULE;

		mocks.ResetAllCalls();


		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY, "07:07:07:07:07:07"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY, GW_IDMAP_MODULE))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Delete(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Delete(IGNORED_PTR_ARG, GW_DEVICEKEY_PROPERTY))
			.IgnoreArgument(1)
			.SetFailReturn((MAP_RESULT)MAP_KEYNOTFOUND);
		STRICT_EXPECTED_CALL(mocks, Message_GetContentHandle(m));
		STRICT_EXPECTED_CALL(mocks, Message_CreateFromBuffer(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Message_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
			.IgnoreAllArguments();
		STRICT_EXPECTED_CALL(mocks, CONSTBUFFER_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, MessageBus_Publish((MESSAGE_BUS_HANDLE)&fake, n, IGNORED_PTR_ARG))
			.IgnoreArgument(3);


		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		VECTOR_destroy(v);
		theAPIS->Module_Destroy(n);

	}

	//Tests_SRS_IDMAP_17_058: [ If deleting the device key does not return MAP_OK or MAP_KEYNOTFOUND, IdentityMap_Receive shall deallocate all resources and return. ]
	TEST_FUNCTION(IdentityMap_Receive_C2D_key_delete_fails)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		VECTOR_HANDLE v = VECTOR_create(sizeof(IDENTITY_MAP_CONFIG));

		IDENTITY_MAP_CONFIG c1 = { "01:01:01:01:01:01", "Sensor1", "theKeyFor1" };
		IDENTITY_MAP_CONFIG c2 = { "02:02:02:02:02:02", "Sensor2", "theKeyFor2" };
		IDENTITY_MAP_CONFIG c3 = { "03:03:03:03:03:03", "Sensor3", "theKeyFor3" };
		IDENTITY_MAP_CONFIG c4 = { "04:04:04:04:04:04", "Sensor4", "theKeyFor4" };
		IDENTITY_MAP_CONFIG c5 = { "05:05:05:05:05:05", "Sensor5", "theKeyFor5" };
		IDENTITY_MAP_CONFIG c6 = { "06:06:06:06:06:06", "Sensor6", "theKeyFor6" };
		IDENTITY_MAP_CONFIG c7 = { "07:07:07:07:07:07", "Sensor7", "theKeyFor7" };
		IDENTITY_MAP_CONFIG c8 = { "08:08:08:08:08:08",	"Sensor8", "theKeyFor8" };
		IDENTITY_MAP_CONFIG c9 = { "09:09:09:09:09:09", "Sensor9",  "theKeyFor9" };
		VECTOR_push_back(v, &c1, 1);
		VECTOR_push_back(v, &c2, 1);
		VECTOR_push_back(v, &c3, 1);
		VECTOR_push_back(v, &c4, 1);
		VECTOR_push_back(v, &c5, 1);
		VECTOR_push_back(v, &c6, 1);
		VECTOR_push_back(v, &c7, 1);
		VECTOR_push_back(v, &c8, 1);
		VECTOR_push_back(v, &c9, 1);
		auto n = theAPIS->Module_Create(bus, v);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		deviceNameProperties = "Sensor7";
		sourceProperties = GW_IOTHUB_MODULE;

		mocks.ResetAllCalls();


		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY, "07:07:07:07:07:07"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY, GW_IDMAP_MODULE))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Delete(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Delete(IGNORED_PTR_ARG, GW_DEVICEKEY_PROPERTY))
			.IgnoreArgument(1)
			.SetFailReturn((MAP_RESULT)MAP_INVALIDARG);

		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		VECTOR_destroy(v);
		theAPIS->Module_Destroy(n);

	}

	//Tests_SRS_IDMAP_17_056: [ If deleting the device name fails, IdentityMap_Receive shall deallocate all resources and return. ]
	TEST_FUNCTION(IdentityMap_Receive_C2D_devicename_delete_fails)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		VECTOR_HANDLE v = VECTOR_create(sizeof(IDENTITY_MAP_CONFIG));

		IDENTITY_MAP_CONFIG c1 = { "01:01:01:01:01:01", "Sensor1", "theKeyFor1" };
		IDENTITY_MAP_CONFIG c2 = { "02:02:02:02:02:02", "Sensor2", "theKeyFor2" };
		IDENTITY_MAP_CONFIG c3 = { "03:03:03:03:03:03", "Sensor3", "theKeyFor3" };
		IDENTITY_MAP_CONFIG c4 = { "04:04:04:04:04:04", "Sensor4", "theKeyFor4" };
		IDENTITY_MAP_CONFIG c5 = { "05:05:05:05:05:05", "Sensor5", "theKeyFor5" };
		IDENTITY_MAP_CONFIG c6 = { "06:06:06:06:06:06", "Sensor6", "theKeyFor6" };
		IDENTITY_MAP_CONFIG c7 = { "07:07:07:07:07:07", "Sensor7", "theKeyFor7" };
		IDENTITY_MAP_CONFIG c8 = { "08:08:08:08:08:08",	"Sensor8", "theKeyFor8" };
		IDENTITY_MAP_CONFIG c9 = { "09:09:09:09:09:09", "Sensor9",  "theKeyFor9" };
		VECTOR_push_back(v, &c1, 1);
		VECTOR_push_back(v, &c2, 1);
		VECTOR_push_back(v, &c3, 1);
		VECTOR_push_back(v, &c4, 1);
		VECTOR_push_back(v, &c5, 1);
		VECTOR_push_back(v, &c6, 1);
		VECTOR_push_back(v, &c7, 1);
		VECTOR_push_back(v, &c8, 1);
		VECTOR_push_back(v, &c9, 1);
		auto n = theAPIS->Module_Create(bus, v);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		deviceNameProperties = "Sensor7";
		sourceProperties = GW_IOTHUB_MODULE;

		mocks.ResetAllCalls();


		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY, "07:07:07:07:07:07"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY, GW_IDMAP_MODULE))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Delete(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1)
			.SetFailReturn((MAP_RESULT)MAP_INVALIDARG);

		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		VECTOR_destroy(v);
		theAPIS->Module_Destroy(n);

	}

	//Tests_SRS_IDMAP_17_044: [ If messageHandle properties contains a "source" property that is set to "mapping", the message shall not be marked as a D2C message. ] 
	TEST_FUNCTION(IdentityMap_Receive_C2D_MapUpdate_source_fails)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		VECTOR_HANDLE v = VECTOR_create(sizeof(IDENTITY_MAP_CONFIG));

		IDENTITY_MAP_CONFIG c1 = { "01:01:01:01:01:01", "Sensor1", "theKeyFor1" };
		IDENTITY_MAP_CONFIG c2 = { "02:02:02:02:02:02", "Sensor2", "theKeyFor2" };
		IDENTITY_MAP_CONFIG c3 = { "03:03:03:03:03:03", "Sensor3", "theKeyFor3" };
		IDENTITY_MAP_CONFIG c4 = { "04:04:04:04:04:04", "Sensor4", "theKeyFor4" };
		IDENTITY_MAP_CONFIG c5 = { "05:05:05:05:05:05", "Sensor5", "theKeyFor5" };
		IDENTITY_MAP_CONFIG c6 = { "06:06:06:06:06:06", "Sensor6", "theKeyFor6" };
		IDENTITY_MAP_CONFIG c7 = { "07:07:07:07:07:07", "Sensor7", "theKeyFor7" };
		IDENTITY_MAP_CONFIG c8 = { "08:08:08:08:08:08",	"Sensor8", "theKeyFor8" };
		IDENTITY_MAP_CONFIG c9 = { "09:09:09:09:09:09", "Sensor9",  "theKeyFor9" };
		VECTOR_push_back(v, &c1, 1);
		VECTOR_push_back(v, &c2, 1);
		VECTOR_push_back(v, &c3, 1);
		VECTOR_push_back(v, &c4, 1);
		VECTOR_push_back(v, &c5, 1);
		VECTOR_push_back(v, &c6, 1);
		VECTOR_push_back(v, &c7, 1);
		VECTOR_push_back(v, &c8, 1);
		VECTOR_push_back(v, &c9, 1);
		auto n = theAPIS->Module_Create(bus, v);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		deviceNameProperties = "Sensor7";
		sourceProperties = GW_IOTHUB_MODULE;

		mocks.ResetAllCalls();


		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY, "07:07:07:07:07:07"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY, GW_IDMAP_MODULE))
			.IgnoreArgument(1)
			.SetFailReturn(MAP_ERROR);


		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		VECTOR_destroy(v);
		theAPIS->Module_Destroy(n);

	}

	//Tests_SRS_IDMAP_17_052: [ If adding macAddress fails, IdentityMap_Receive shall deallocate all resources and return. ]

	TEST_FUNCTION(IdentityMap_Receive_C2D_MapUpdate_mac_fails)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		VECTOR_HANDLE v = VECTOR_create(sizeof(IDENTITY_MAP_CONFIG));

		IDENTITY_MAP_CONFIG c1 = { "01:01:01:01:01:01", "Sensor1", "theKeyFor1" };
		IDENTITY_MAP_CONFIG c2 = { "02:02:02:02:02:02", "Sensor2", "theKeyFor2" };
		IDENTITY_MAP_CONFIG c3 = { "03:03:03:03:03:03", "Sensor3", "theKeyFor3" };
		IDENTITY_MAP_CONFIG c4 = { "04:04:04:04:04:04", "Sensor4", "theKeyFor4" };
		IDENTITY_MAP_CONFIG c5 = { "05:05:05:05:05:05", "Sensor5", "theKeyFor5" };
		IDENTITY_MAP_CONFIG c6 = { "06:06:06:06:06:06", "Sensor6", "theKeyFor6" };
		IDENTITY_MAP_CONFIG c7 = { "07:07:07:07:07:07", "Sensor7", "theKeyFor7" };
		IDENTITY_MAP_CONFIG c8 = { "08:08:08:08:08:08",	"Sensor8", "theKeyFor8" };
		IDENTITY_MAP_CONFIG c9 = { "09:09:09:09:09:09", "Sensor9",  "theKeyFor9" };
		VECTOR_push_back(v, &c1, 1);
		VECTOR_push_back(v, &c2, 1);
		VECTOR_push_back(v, &c3, 1);
		VECTOR_push_back(v, &c4, 1);
		VECTOR_push_back(v, &c5, 1);
		VECTOR_push_back(v, &c6, 1);
		VECTOR_push_back(v, &c7, 1);
		VECTOR_push_back(v, &c8, 1);
		VECTOR_push_back(v, &c9, 1);
		auto n = theAPIS->Module_Create(bus, v);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		deviceNameProperties = "Sensor7";
		sourceProperties = GW_IOTHUB_MODULE;

		mocks.ResetAllCalls();


		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, Map_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, Map_AddOrUpdate(IGNORED_PTR_ARG, GW_MAC_ADDRESS_PROPERTY, "07:07:07:07:07:07"))
			.IgnoreArgument(1)
			.SetFailReturn(MAP_ERROR);


		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		VECTOR_destroy(v);
		theAPIS->Module_Destroy(n);

	}

	//Tests_SRS_IDMAP_17_050: [ If ConstMap_CloneWriteable fails, IdentityMap_Receive shall deallocate any resources and return. ]
	TEST_FUNCTION(IdentityMap_Receive_C2D_clone_writeable_fails)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		VECTOR_HANDLE v = VECTOR_create(sizeof(IDENTITY_MAP_CONFIG));

		IDENTITY_MAP_CONFIG c1 = { "01:01:01:01:01:01", "Sensor1", "theKeyFor1" };
		IDENTITY_MAP_CONFIG c2 = { "02:02:02:02:02:02", "Sensor2", "theKeyFor2" };
		IDENTITY_MAP_CONFIG c3 = { "03:03:03:03:03:03", "Sensor3", "theKeyFor3" };
		IDENTITY_MAP_CONFIG c4 = { "04:04:04:04:04:04", "Sensor4", "theKeyFor4" };
		IDENTITY_MAP_CONFIG c5 = { "05:05:05:05:05:05", "Sensor5", "theKeyFor5" };
		IDENTITY_MAP_CONFIG c6 = { "06:06:06:06:06:06", "Sensor6", "theKeyFor6" };
		IDENTITY_MAP_CONFIG c7 = { "07:07:07:07:07:07", "Sensor7", "theKeyFor7" };
		IDENTITY_MAP_CONFIG c8 = { "08:08:08:08:08:08",	"Sensor8", "theKeyFor8" };
		IDENTITY_MAP_CONFIG c9 = { "09:09:09:09:09:09", "Sensor9",  "theKeyFor9" };
		VECTOR_push_back(v, &c1, 1);
		VECTOR_push_back(v, &c2, 1);
		VECTOR_push_back(v, &c3, 1);
		VECTOR_push_back(v, &c4, 1);
		VECTOR_push_back(v, &c5, 1);
		VECTOR_push_back(v, &c6, 1);
		VECTOR_push_back(v, &c7, 1);
		VECTOR_push_back(v, &c8, 1);
		VECTOR_push_back(v, &c9, 1);
		auto n = theAPIS->Module_Create(bus, v);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		deviceNameProperties = "Sensor7";
		sourceProperties = GW_IOTHUB_MODULE;

		mocks.ResetAllCalls();


		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, ConstMap_CloneWriteable(IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.SetFailReturn((MAP_HANDLE)NULL);


		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		VECTOR_destroy(v);
		theAPIS->Module_Destroy(n);

	}

	//Tests_SRS_IDMAP_17_046: [ If messageHandle properties does not contain a "source" property, then the message shall not be marked as a C2D message. ]
	TEST_FUNCTION(IdentityMap_Receive_C2D_getproperties_fails)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		VECTOR_HANDLE v = VECTOR_create(sizeof(IDENTITY_MAP_CONFIG));

		IDENTITY_MAP_CONFIG c1 = { "01:01:01:01:01:01", "Sensor1", "theKeyFor1" };
		IDENTITY_MAP_CONFIG c2 = { "02:02:02:02:02:02", "Sensor2", "theKeyFor2" };
		IDENTITY_MAP_CONFIG c3 = { "03:03:03:03:03:03", "Sensor3", "theKeyFor3" };
		IDENTITY_MAP_CONFIG c4 = { "04:04:04:04:04:04", "Sensor4", "theKeyFor4" };
		IDENTITY_MAP_CONFIG c5 = { "05:05:05:05:05:05", "Sensor5", "theKeyFor5" };
		IDENTITY_MAP_CONFIG c6 = { "06:06:06:06:06:06", "Sensor6", "theKeyFor6" };
		IDENTITY_MAP_CONFIG c7 = { "07:07:07:07:07:07", "Sensor7", "theKeyFor7" };
		IDENTITY_MAP_CONFIG c8 = { "08:08:08:08:08:08",	"Sensor8", "theKeyFor8" };
		IDENTITY_MAP_CONFIG c9 = { "09:09:09:09:09:09", "Sensor9",  "theKeyFor9" };
		VECTOR_push_back(v, &c1, 1);
		VECTOR_push_back(v, &c2, 1);
		VECTOR_push_back(v, &c3, 1);
		VECTOR_push_back(v, &c4, 1);
		VECTOR_push_back(v, &c5, 1);
		VECTOR_push_back(v, &c6, 1);
		VECTOR_push_back(v, &c7, 1);
		VECTOR_push_back(v, &c8, 1);
		VECTOR_push_back(v, &c9, 1);
		auto n = theAPIS->Module_Create(bus, v);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		deviceNameProperties = "Sensor7";
		sourceProperties = GW_IOTHUB_MODULE;

		mocks.ResetAllCalls();


		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m))
			.SetFailReturn((CONSTMAP_HANDLE)NULL);

		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		VECTOR_destroy(v);
		theAPIS->Module_Destroy(n);

	}

	//Tests_SRS_IDMAP_17_048: [ If the deviceName of the message is not found in deviceToMacArray, then the message shall not be marked as a C2D message. ]
	TEST_FUNCTION(IdentityMap_Receive_C2D_id_no_match_no_new_msg)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		VECTOR_HANDLE v = VECTOR_create(sizeof(IDENTITY_MAP_CONFIG));

		IDENTITY_MAP_CONFIG c1 = { "01:01:01:01:01:01", "Sensor1", "theKeyFor1" };
		IDENTITY_MAP_CONFIG c2 = { "02:02:02:02:02:02", "Sensor2", "theKeyFor2" };
		IDENTITY_MAP_CONFIG c3 = { "03:03:03:03:03:03", "Sensor3", "theKeyFor3" };
		IDENTITY_MAP_CONFIG c4 = { "04:04:04:04:04:04", "Sensor4", "theKeyFor4" };
		IDENTITY_MAP_CONFIG c5 = { "05:05:05:05:05:05", "Sensor5", "theKeyFor5" };
		IDENTITY_MAP_CONFIG c6 = { "06:06:06:06:06:06", "Sensor6", "theKeyFor6" };
		IDENTITY_MAP_CONFIG c7 = { "07:07:07:07:07:07", "Sensor7", "theKeyFor7" };
		IDENTITY_MAP_CONFIG c8 = { "08:08:08:08:08:08",	"Sensor8", "theKeyFor8" };
		IDENTITY_MAP_CONFIG c9 = { "09:09:09:09:09:09", "Sensor9",  "theKeyFor9" };
		VECTOR_push_back(v, &c1, 1);
		VECTOR_push_back(v, &c2, 1);
		VECTOR_push_back(v, &c3, 1);
		VECTOR_push_back(v, &c4, 1);
		VECTOR_push_back(v, &c5, 1);
		VECTOR_push_back(v, &c6, 1);
		VECTOR_push_back(v, &c7, 1);
		VECTOR_push_back(v, &c8, 1);
		VECTOR_push_back(v, &c9, 1);
		auto n = theAPIS->Module_Create(bus, v);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		deviceNameProperties = "Sensor13";
		sourceProperties = GW_IOTHUB_MODULE;

		mocks.ResetAllCalls();


		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);

		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		VECTOR_destroy(v);
		theAPIS->Module_Destroy(n);

	}

	//Tests_SRS_IDMAP_17_048: [ If the deviceName of the message is not found in deviceToMacArray, then the message shall not be marked as a C2D message. ]
	//Tests_SRS_IDMAP_17_045: [ If messageHandle properties does not contain "deviceName" property, then the message shall not be marked as a C2D message. ]
	TEST_FUNCTION(IdentityMap_Receive_C2D_no_id_no_new_msg)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		VECTOR_HANDLE v = VECTOR_create(sizeof(IDENTITY_MAP_CONFIG));

		IDENTITY_MAP_CONFIG c1 = { "01:01:01:01:01:01", "Sensor1", "theKeyFor1" };
		IDENTITY_MAP_CONFIG c2 = { "02:02:02:02:02:02", "Sensor2", "theKeyFor2" };
		IDENTITY_MAP_CONFIG c3 = { "03:03:03:03:03:03", "Sensor3", "theKeyFor3" };
		IDENTITY_MAP_CONFIG c4 = { "04:04:04:04:04:04", "Sensor4", "theKeyFor4" };
		IDENTITY_MAP_CONFIG c5 = { "05:05:05:05:05:05", "Sensor5", "theKeyFor5" };
		IDENTITY_MAP_CONFIG c6 = { "06:06:06:06:06:06", "Sensor6", "theKeyFor6" };
		IDENTITY_MAP_CONFIG c7 = { "07:07:07:07:07:07", "Sensor7", "theKeyFor7" };
		IDENTITY_MAP_CONFIG c8 = { "08:08:08:08:08:08",	"Sensor8", "theKeyFor8" };
		IDENTITY_MAP_CONFIG c9 = { "09:09:09:09:09:09", "Sensor9",  "theKeyFor9" };
		VECTOR_push_back(v, &c1, 1);
		VECTOR_push_back(v, &c2, 1);
		VECTOR_push_back(v, &c3, 1);
		VECTOR_push_back(v, &c4, 1);
		VECTOR_push_back(v, &c5, 1);
		VECTOR_push_back(v, &c6, 1);
		VECTOR_push_back(v, &c7, 1);
		VECTOR_push_back(v, &c8, 1);
		VECTOR_push_back(v, &c9, 1);
		auto n = theAPIS->Module_Create(bus, v);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		deviceNameProperties = NULL;
		sourceProperties = GW_IOTHUB_MODULE;

		mocks.ResetAllCalls();


		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_DEVICENAME_PROPERTY))
			.IgnoreArgument(1);

		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		VECTOR_destroy(v);
		theAPIS->Module_Destroy(n);

	}

	//Tests_SRS_IDMAP_17_047: [ If messageHandle property "source" is not equal to "IoTHubHttp", then the message shall not be marked as a C2D message. ]
	TEST_FUNCTION(IdentityMap_Receive_no_direction_no_new_msg)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		VECTOR_HANDLE v = VECTOR_create(sizeof(IDENTITY_MAP_CONFIG));

		IDENTITY_MAP_CONFIG c1 = { "01:01:01:01:01:01", "Sensor1", "theKeyFor1" };
		IDENTITY_MAP_CONFIG c2 = { "02:02:02:02:02:02", "Sensor2", "theKeyFor2" };
		IDENTITY_MAP_CONFIG c3 = { "03:03:03:03:03:03", "Sensor3", "theKeyFor3" };
		IDENTITY_MAP_CONFIG c4 = { "04:04:04:04:04:04", "Sensor4", "theKeyFor4" };
		IDENTITY_MAP_CONFIG c5 = { "05:05:05:05:05:05", "Sensor5", "theKeyFor5" };
		IDENTITY_MAP_CONFIG c6 = { "06:06:06:06:06:06", "Sensor6", "theKeyFor6" };
		IDENTITY_MAP_CONFIG c7 = { "07:07:07:07:07:07", "Sensor7", "theKeyFor7" };
		IDENTITY_MAP_CONFIG c8 = { "08:08:08:08:08:08",	"Sensor8", "theKeyFor8" };
		IDENTITY_MAP_CONFIG c9 = { "09:09:09:09:09:09", "Sensor9",  "theKeyFor9" };
		VECTOR_push_back(v, &c1, 1);
		VECTOR_push_back(v, &c2, 1);
		VECTOR_push_back(v, &c3, 1);
		VECTOR_push_back(v, &c4, 1);
		VECTOR_push_back(v, &c5, 1);
		VECTOR_push_back(v, &c6, 1);
		VECTOR_push_back(v, &c7, 1);
		VECTOR_push_back(v, &c8, 1);
		VECTOR_push_back(v, &c9, 1);
		auto n = theAPIS->Module_Create(bus, v);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		deviceNameProperties = "Sensor7";
		sourceProperties = NULL;

		mocks.ResetAllCalls();


		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);


		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		VECTOR_destroy(v);
		theAPIS->Module_Destroy(n);

	}

	//Tests_SRS_IDMAP_17_047: [ If messageHandle property "source" is not equal to "IoTHubHttp", then the message shall not be marked as a C2D message. ]
	TEST_FUNCTION(IdentityMap_Receive_mapping_ignore_mapping_msg)
	{
		///Arrange
		CIdentitymapMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		unsigned char fake;
		MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)&fake;
		VECTOR_HANDLE v = VECTOR_create(sizeof(IDENTITY_MAP_CONFIG));

		IDENTITY_MAP_CONFIG c1 = { "01:01:01:01:01:01", "Sensor1", "theKeyFor1" };
		IDENTITY_MAP_CONFIG c2 = { "02:02:02:02:02:02", "Sensor2", "theKeyFor2" };
		IDENTITY_MAP_CONFIG c3 = { "03:03:03:03:03:03", "Sensor3", "theKeyFor3" };
		IDENTITY_MAP_CONFIG c4 = { "04:04:04:04:04:04", "Sensor4", "theKeyFor4" };
		IDENTITY_MAP_CONFIG c5 = { "05:05:05:05:05:05", "Sensor5", "theKeyFor5" };
		IDENTITY_MAP_CONFIG c6 = { "06:06:06:06:06:06", "Sensor6", "theKeyFor6" };
		IDENTITY_MAP_CONFIG c7 = { "07:07:07:07:07:07", "Sensor7", "theKeyFor7" };
		IDENTITY_MAP_CONFIG c8 = { "08:08:08:08:08:08",	"Sensor8", "theKeyFor8" };
		IDENTITY_MAP_CONFIG c9 = { "09:09:09:09:09:09", "Sensor9",  "theKeyFor9" };
		VECTOR_push_back(v, &c1, 1);
		VECTOR_push_back(v, &c2, 1);
		VECTOR_push_back(v, &c3, 1);
		VECTOR_push_back(v, &c4, 1);
		VECTOR_push_back(v, &c5, 1);
		VECTOR_push_back(v, &c6, 1);
		VECTOR_push_back(v, &c7, 1);
		VECTOR_push_back(v, &c8, 1);
		VECTOR_push_back(v, &c9, 1);
		auto n = theAPIS->Module_Create(bus, v);

		MESSAGE_CONFIG cfg = { 1, &fake, (MAP_HANDLE)&fake };
		auto m = Message_Create(&cfg);

		macAddressProperties = "07:07:07:07:07:07";
		deviceNameProperties = "Sensor7";
		deviceKeyProperties = "theKeyFor7";
		sourceProperties = GW_IDMAP_MODULE;

		mocks.ResetAllCalls();


		STRICT_EXPECTED_CALL(mocks, Message_GetProperties(m));
		STRICT_EXPECTED_CALL(mocks, ConstMap_Create(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(mocks, ConstMap_GetValue(IGNORED_PTR_ARG, GW_SOURCE_PROPERTY))
			.IgnoreArgument(1);


		///Act
		theAPIS->Module_Receive(n, m);

		///Assert
		mocks.AssertActualAndExpectedCalls();

		///Ablution
		Message_Destroy(m);
		VECTOR_destroy(v);
		theAPIS->Module_Destroy(n);

	}
	//

END_TEST_SUITE(identitymap_unittests)
