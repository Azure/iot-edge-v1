// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umocktypes_charptr.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/constbuffer.h"
#include "azure_c_shared_utility/constmap.h"
#include "azure_c_shared_utility/map.h"
#undef ENABLE_MOCKS

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

#include "message.h"

static size_t currentmalloc_call;
static size_t whenShallmalloc_fail;

static size_t currentConstMap_Create_call;
static size_t whenShallConstMap_Create_fail;

static size_t currentConstMap_Clone_call;
static size_t whenShallConstMap_Clone_fail;

static size_t currentCONSTBUFFER_Create_call;
static size_t whenShallCONSTBUFFER_Create_fail;
static size_t currentCONSTBUFFER_refCount;

static size_t currentCONSTBUFFER_Clone_call;
static size_t whenShallCONSTBUFFER_Clone_fail;

static void* my_gballoc_malloc(size_t size)
{
    void* result;
    currentmalloc_call++;
    if (whenShallmalloc_fail > 0)
    {
        if (currentmalloc_call == whenShallmalloc_fail)
        {
            result = NULL;
        }
        else
        {
            result = malloc(size);
        }
    }
    else
    {
        result = malloc(size);
    }
    return result;
}


static void my_gballoc_free(void* ptr)
{
    free(ptr);
}

static CONSTMAP_HANDLE my_ConstMap_Create(MAP_HANDLE sourceMap)
{
    CONSTMAP_HANDLE result2;
    currentConstMap_Create_call++;
    if (whenShallConstMap_Create_fail == currentConstMap_Create_call)
    {
        result2 = NULL;
    }
    else
    {
        result2 = (CONSTMAP_HANDLE)malloc(1);
        *(unsigned char*)result2 = 1;
    }
    return result2;
}

static CONSTMAP_HANDLE my_ConstMap_Clone(CONSTMAP_HANDLE handle)
{
    CONSTMAP_HANDLE result3;
    currentConstMap_Clone_call++;
    if (whenShallConstMap_Clone_fail == currentConstMap_Clone_call)
    {
        result3 = NULL;
    }
    else
    {
        result3 = handle;
        *(unsigned char*)result3 += 1;
    }
    return result3;
}

static void my_ConstMap_Destroy(CONSTMAP_HANDLE map)
{
    unsigned char refCount = --(*(unsigned char*)map);
    if (refCount == 0)
        free(map);
}

static CONSTBUFFER_HANDLE my_CONSTBUFFER_Create(const unsigned char* source, size_t size)
{
    CONSTBUFFER_HANDLE result1;
    currentCONSTBUFFER_Create_call++;
    if (whenShallCONSTBUFFER_Create_fail == currentCONSTBUFFER_Create_call)
    {
        result1 = NULL;
    }
    else
    {
        result1 = (CONSTBUFFER_HANDLE)malloc(sizeof(CONSTBUFFER));
        (*(CONSTBUFFER*)result1).size = size;
        if (size == 0)
        {
            (*(CONSTBUFFER*)result1).buffer = NULL;
        }
        else
        {
            unsigned char* temp = (unsigned char*)malloc(size);
            memcpy(temp, source, size);
            (*(CONSTBUFFER*)result1).buffer = temp;
        }
        currentCONSTBUFFER_refCount = 1;
    }
    return result1;
}

static CONSTBUFFER_HANDLE my_CONSTBUFFER_Clone(CONSTBUFFER_HANDLE constbufferHandle)
{
    CONSTBUFFER_HANDLE result2;
    currentCONSTBUFFER_Clone_call++;
    if (currentCONSTBUFFER_Clone_call == whenShallCONSTBUFFER_Clone_fail)
    {
        result2 = NULL;
    }
    else
    {
        result2 = constbufferHandle;
        currentCONSTBUFFER_refCount++;
    }
    return result2;
}

static const CONSTBUFFER* my_CONSTBUFFER_GetContent(CONSTBUFFER_HANDLE constbufferHandle)
{
    CONSTBUFFER* result3 = (CONSTBUFFER*)constbufferHandle;
    return result3;
}

static void my_CONSTBUFFER_Destroy(CONSTBUFFER_HANDLE constbufferHandle)
{
    --currentCONSTBUFFER_refCount;
    if (currentCONSTBUFFER_refCount == 0)
    {
        CONSTBUFFER * fakeBuffer = (CONSTBUFFER*)constbufferHandle;
        if (fakeBuffer->buffer != NULL)
        {
            free((void*)fakeBuffer->buffer);
        }
        free(fakeBuffer);
    }
}

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#undef ENABLE_MOCKS

#ifdef _MSC_VER
#pragma warning(disable:4505)
#endif

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error");
}

/*the following buffers explore some cases where the message should be buildable from that buffer*/

static const unsigned char notFail____minimalMessage[] =
{
    0xA1, 0x60,             /*header*/
    0x00, 0x00, 0x00, 14,   /*size of this array*/
    0x00, 0x00, 0x00, 0x00, /*zero properties*/
    0x00, 0x00, 0x00, 0x00  /*zero message content size*/
};

static const unsigned char notFail__1Property_0bytes[] =
{
    0xA1, 0x60,             /*header*/
    0x00, 0x00, 0x00, 18,   /*size of this array*/
    0x00, 0x00, 0x00, 0x01, /*one property*/
    '3', '\0', '3', '\0',
    0x00, 0x00, 0x00, 0x00  /*zero message content size*/
};

static const unsigned char notFail__2Property_0bytes[] =
{
    0xA1, 0x60,             /*header*/
    0x00, 0x00, 0x00, 23,   /*size of this array*/
    0x00, 0x00, 0x00, 0x02, /*two properties*/
    '3', '\0', '3', '\0',
    'a', 'b', '\0', 'a', '\0',
    0x00, 0x00, 0x00, 0x00  /*zero message content size*/
};

static const unsigned char notFail__0Property_1bytes[] =
{
    0xA1, 0x60,             /*header*/
    0x00, 0x00, 0x00, 15,   /*size of this array*/
    0x00, 0x00, 0x00, 0x00, /*zero properties*/
    0x00, 0x00, 0x00, 0x01,  /*1 message content size*/
    '3'
};

static const unsigned char notFail__1Property_1bytes[] =
{
    0xA1, 0x60,             /*header*/
    0x00, 0x00, 0x00, 44,   /*size of this array*/
    0x00, 0x00, 0x00, 0x01, /*one property*/
    'A', 'z','u','r','e',' ','I','o','T',' ','G','a','t','e','w','a','y',' ','i','s','\0','a','w','e','s','o','m','e','\0',
    0x00, 0x00, 0x00, 0x01,  /*1 message content size*/
    '3'
};

static const unsigned char notFail__2Property_1bytes[] =
{
    0xA1, 0x60,             /*header*/
    0x00, 0x00, 0x00, 63,   /*size of this array*/
    0x00, 0x00, 0x00, 0x02, /*two properties*/
    'A', 'z','u','r','e',' ','I','o','T',' ','G','a','t','e','w','a','y',' ','i','s','\0','a','w','e','s','o','m','e','\0',
    'B','l','e','e','d','i','n','g','E','d','g','e','\0','r','o','c','k','s','\0',
    0x00, 0x00, 0x00, 0x01,  /*1 message content size*/
    '3'
};

static const unsigned char notFail__0Property_2bytes[] =
{
    0xA1, 0x60,             /*header*/
    0x00, 0x00, 0x00, 16,   /*size of this array*/
    0x00, 0x00, 0x00, 0x00, /*zero property*/
    0x00, 0x00, 0x00, 0x02,  /*2 message content size*/
    '3', '4'
};

static const unsigned char notFail__1Property_2bytes[] =
{
    0xA1, 0x60,             /*header*/
    0x00, 0x00, 0x00, 45,   /*size of this array*/
    0x00, 0x00, 0x00, 0x01, /*one property*/
    'A', 'z','u','r','e',' ','I','o','T',' ','G','a','t','e','w','a','y',' ','i','s','\0','a','w','e','s','o','m','e','\0',
    0x00, 0x00, 0x00, 0x02,  /*2 message content size*/
    '3', '4'
};

static const unsigned char notFail__2Property_2bytes[] =
{
    0xA1, 0x60,             /*header*/
    0x00, 0x00, 0x00, 64,   /*size of this array*/
    0x00, 0x00, 0x00, 0x02, /*two properties*/
    'B','l','e','e','d','i','n','g','E','d','g','e','\0','r','o','c','k','s','\0',
    'A', 'z','u','r','e',' ','I','o','T',' ','G','a','t','e','w','a','y',' ','i','s','\0','a','w','e','s','o','m','e','\0',
    0x00, 0x00, 0x00, 0x02,  /*2 message content size*/
    '3', '4'
};

static const unsigned char fail_____firstByteNot0xA1[] =
{
    0xA2, 0x60,             /*header - wrong*/
    0x00, 0x00, 0x00, 64,   /*size of this array*/
    0x00, 0x00, 0x00, 0x02, /*two properties*/
    'B','l','e','e','d','i','n','g','E','d','g','e','\0','r','o','c','k','s','\0',
    'A', 'z','u','r','e',' ','I','o','T',' ','G','a','t','e','w','a','y',' ','i','s','\0','a','w','e','s','o','m','e','\0',
    0x00, 0x00, 0x00, 0x02,  /*2 message content size*/
    '3', '4'
};

static const unsigned char fail____secondByteNot0x60[] =
{
    0xA1, 0x61,             /*header - wrong*/
    0x00, 0x00, 0x00, 64,   /*size of this array*/
    0x00, 0x00, 0x00, 0x02, /*two properties*/
    'B','l','e','e','d','i','n','g','E','d','g','e','\0','r','o','c','k','s','\0',
    'A', 'z','u','r','e',' ','I','o','T',' ','G','a','t','e','w','a','y',' ','i','s','\0','a','w','e','s','o','m','e','\0',
    0x00, 0x00, 0x00, 0x02,  /*2 message content size*/
    '3', '4'
};

static const unsigned char fail_firstPropertyNameTooBig[] =
{
    0xA1, 0x60,             /*header*/
    0x00, 0x00, 0x00, 18,   /*size of this array*/
    0x00, 0x00, 0x00, 0x01, /*one property*/
    '3', '3', '3', '3',     /*property name just keeps going...*/
    '3', '3', '3', '3'
};

static const unsigned char fail_firstPropertyValueDoesNotExist[] =
{
    0xA1, 0x60,             /*header*/
    0x00, 0x00, 0x00, 18,   /*size of this array*/
    0x00, 0x00, 0x00, 0x01, /*one property*/
    '3', '3', '3', '3',     
    '3', '3', '3', '\0'     /*property value does not have a start*/
};

static const unsigned char fail_firstPropertyValueDoesNotEnd[] =
{
    0xA1, 0x60,             /*header*/
    0x00, 0x00, 0x00, 18,   /*size of this array*/
    0x00, 0x00, 0x00, 0x01, /*one property*/
    '3', '3', '3', '\0',     
    '3', '3', '3', '3'     /*property value just keeps going...*/
};

static const unsigned char fail_whenThereIsOnly1ByteOfcontentSize[] =
{
    0xA1, 0x60,             /*header*/
    0x00, 0x00, 0x00, 19,   /*size of this array*/
    0x00, 0x00, 0x00, 0x01, /*one property*/
    '3', '3', '3', '\0',
    '3', '3', '3', '\0',
    0x00                    /*not enough bytes for contentSize*/
};



#define TEST_MAP_HANDLE ((MAP_HANDLE)(1))
#define TEST_CONSTBUFFER_HANDLE ((CONSTBUFFER_HANDLE)2)
#define TEST_CONSTMAP_HANDLE ((CONSTMAP_HANDLE)3)
#define TEST_MESSAGE_HANDLE ((MESSAGE_HANDLE)4)
#define TEST_MESSAGE_HANDLE_EMPTY ((MESSAGE_HANDLE)5)
#define TEST_MESSAGE_HANDLE_EMPTY_PROPERTIES ((CONSTMAP_HANDLE)6)

//TEST_DEFINE_ENUM_TYPE(MAP_RESULT, MAP_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(MAP_RESULT, MAP_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(CONSTMAP_RESULT, CONSTMAP_RESULT_VALUES);

BEGIN_TEST_SUITE(gwmessage_unittests)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
        g_testByTest = TEST_MUTEX_CREATE();
        ASSERT_IS_NOT_NULL(g_testByTest);

        umock_c_init(on_umock_c_error);

        int result = umocktypes_charptr_register_types();
        ASSERT_ARE_EQUAL(int, 0, result);

        REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
        REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

        REGISTER_GLOBAL_MOCK_HOOK(ConstMap_Create, my_ConstMap_Create);
        REGISTER_GLOBAL_MOCK_HOOK(ConstMap_Clone, my_ConstMap_Clone);
        REGISTER_GLOBAL_MOCK_HOOK(ConstMap_Destroy, my_ConstMap_Destroy);

        REGISTER_GLOBAL_MOCK_HOOK(CONSTBUFFER_Create, my_CONSTBUFFER_Create);
        REGISTER_GLOBAL_MOCK_HOOK(CONSTBUFFER_Clone, my_CONSTBUFFER_Clone);
        REGISTER_GLOBAL_MOCK_HOOK(CONSTBUFFER_GetContent, my_CONSTBUFFER_GetContent);
        REGISTER_GLOBAL_MOCK_HOOK(CONSTBUFFER_Destroy, my_CONSTBUFFER_Destroy);

        REGISTER_UMOCK_ALIAS_TYPE(MAP_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(MAP_FILTER_CALLBACK, void*);
        REGISTER_UMOCK_ALIAS_TYPE(CONSTMAP_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(CONSTBUFFER_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(const CONSTBUFFER*, void*);
        
        REGISTER_TYPE(MAP_RESULT, MAP_RESULT);
        REGISTER_TYPE(CONSTMAP_RESULT, CONSTMAP_RESULT);
        
        REGISTER_UMOCK_ALIAS_TYPE(const unsigned char*, void*);
        REGISTER_UMOCK_ALIAS_TYPE(const char* const*, void*);
        REGISTER_UMOCK_ALIAS_TYPE(const char* const* *, void*);

        //
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
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

        currentmalloc_call = 0;
        whenShallmalloc_fail = 0;

		currentConstMap_Create_call = 0;
		whenShallConstMap_Create_fail = 0;
		currentConstMap_Clone_call = 0;
		whenShallConstMap_Clone_fail = 0;
		currentCONSTBUFFER_Create_call = 0;
		whenShallCONSTBUFFER_Create_fail = 0;
		currentCONSTBUFFER_refCount = 0;
		currentCONSTBUFFER_Clone_call = 0;
		whenShallCONSTBUFFER_Clone_fail = 0;

    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
        TEST_MUTEX_RELEASE(g_testByTest);
    }

    /*Tests_SRS_MESSAGE_02_002: [If cfg is NULL then Message_Create shall return NULL.]*/
    TEST_FUNCTION(Message_Create_with_NULL_parameter_fails)
    {
        ///arrange

        ///act
        MESSAGE_HANDLE r = Message_Create(NULL);

        ///assert
        ASSERT_IS_NULL(r);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_003: [If field source of cfg is NULL and size is not zero, then Message_Create shall fail and return NULL.]*/
    TEST_FUNCTION(Message_Create_with_NULL_source_and_non_zero_size_fails)
    {
        ///arrange
        MESSAGE_CONFIG c = {1, NULL, (MAP_HANDLE)&c};

        ///act
        MESSAGE_HANDLE r = Message_Create(&c);

        ///assert
        ASSERT_IS_NULL(r);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_006: [Otherwise, Message_Create shall return a non-NULL handle and shall set the internal ref count to "1".]*/
    /*Tests_SRS_MESSAGE_02_019: [Message_Create shall clone the sourceProperties to a readonly CONSTMAP.] */
	/*Tests_SRS_MESSAGE_17_003: [Message_Create shall copy the source to a readonly CONSTBUFFER.]*/
    TEST_FUNCTION(Message_Create_happy_path)
    {
        ///arrange
        unsigned char fake;
        MESSAGE_CONFIG c = { 1, &fake, (MAP_HANDLE)&fake};

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
            .IgnoreArgument(1);

		STRICT_EXPECTED_CALL(CONSTBUFFER_Create(&fake, 1)); /*this is copying the buffer*/

		STRICT_EXPECTED_CALL(ConstMap_Create((MAP_HANDLE)&fake)); /*this is copying the properties*/

        ///act
        MESSAGE_HANDLE r = Message_Create(&c);

        ///assert
        ASSERT_IS_NOT_NULL(r);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        Message_Destroy(r);
    }

    /*Tests_SRS_MESSAGE_02_004: [Mesages shall be allowed to be created from zero-size content.]*/
    TEST_FUNCTION(Message_Create_happy_path_zero_size_1)
    {
        ///arrange
        unsigned char fake;
        MESSAGE_CONFIG c = { 0, &fake, (MAP_HANDLE)&fake };

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
            .IgnoreArgument(1);

		STRICT_EXPECTED_CALL(CONSTBUFFER_Create(&fake, 0)); /* this is copying the (empty buffer)*/

		STRICT_EXPECTED_CALL(ConstMap_Create((MAP_HANDLE)&fake)); /*this is copying the properties*/

        ///act
        MESSAGE_HANDLE r = Message_Create(&c);

        ///assert
        ASSERT_IS_NOT_NULL(r);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        Message_Destroy(r);
    }

    /*Tests_SRS_MESSAGE_02_004: [Mesages shall be allowed to be created from zero-size content.]*/
    TEST_FUNCTION(Message_Create_happy_path_zero_size_2)
    {
        ///arrange
        unsigned char fake;
        MESSAGE_CONFIG c = { 0, NULL, (MAP_HANDLE)&fake }; /*<---- this is NULL , in the testbefore it was non-NULL*/

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
            .IgnoreArgument(1);

		STRICT_EXPECTED_CALL(CONSTBUFFER_Create(NULL, 0)); /* this is copying the (empty buffer)*/

		STRICT_EXPECTED_CALL(ConstMap_Create((MAP_HANDLE)&fake)); /*this is copying the properties*/

        ///act
        MESSAGE_HANDLE r = Message_Create(&c);

        ///assert
        ASSERT_IS_NOT_NULL(r);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        Message_Destroy(r);
    }

    /*Tests_SRS_MESSAGE_02_005: [If Message_Create encounters an error while building the internal structures of the message, then it shall return NULL.]*/
    TEST_FUNCTION(Message_Create_zero_size_fails_when_Map_Clone_fails)
    {
        ///arrange
        unsigned char fake;
        MESSAGE_CONFIG c = { 0, NULL, (MAP_HANDLE)&fake }; /*<---- this is NULL , in the testbefore it was non-NULL*/

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
            .IgnoreArgument(1);
        {
            STRICT_EXPECTED_CALL(CONSTBUFFER_Create(NULL, 0)); /* this is copying the (empty buffer)*/
            {
                whenShallConstMap_Create_fail = 1;
                STRICT_EXPECTED_CALL(ConstMap_Create((MAP_HANDLE)&fake)) /*this is copying the properties*/
                    .IgnoreArgument(1);

                STRICT_EXPECTED_CALL(CONSTBUFFER_Destroy(IGNORED_PTR_ARG)) /* this is copying the (empty buffer)*/
                    .IgnoreArgument(1);
            }
            STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
        }
        ///act
        MESSAGE_HANDLE r = Message_Create(&c);

        ///assert
        ASSERT_IS_NULL(r);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_005: [If Message_Create encounters an error while building the internal structures of the message, then it shall return NULL.]*/
    TEST_FUNCTION(Message_Create_zero_size_fails_when_malloc_fails)
    {
        ///arrange
        unsigned char fake;
        MESSAGE_CONFIG c = { 0, NULL, (MAP_HANDLE)&fake }; /*<---- this is NULL , in the testbefore it was non-NULL*/

        whenShallmalloc_fail = 1;
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
            .IgnoreArgument(1);

        ///act
        MESSAGE_HANDLE r = Message_Create(&c);

        ///assert
        ASSERT_IS_NULL(r);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_005: [If Message_Create encounters an error while building the internal structures of the message, then it shall return NULL.]*/
    TEST_FUNCTION(Message_Create_nonzero_size_fails_when_Map_Clone_fails)
    {
        ///arrange
        unsigned char fake;
        MESSAGE_CONFIG c = { 1, &fake, (MAP_HANDLE)&fake };

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
            .IgnoreArgument(1);
        {
            STRICT_EXPECTED_CALL(CONSTBUFFER_Create(&fake, 1)); /* this is copying the buffer*/
            {

                whenShallConstMap_Create_fail = 1;
                STRICT_EXPECTED_CALL(ConstMap_Create((MAP_HANDLE)&fake)); /*this is copying the properties*/

                STRICT_EXPECTED_CALL(CONSTBUFFER_Destroy(IGNORED_PTR_ARG))
                    .IgnoreArgument(1);
            }
            STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
        }
        ///act
        MESSAGE_HANDLE r = Message_Create(&c);

        ///assert
        ASSERT_IS_NULL(r);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_005: [If Message_Create encounters an error while building the internal structures of the message, then it shall return NULL.]*/
    TEST_FUNCTION(Message_Create_nonzero_size_fails_when_CONSTBUFFER_fails_1)
    {
        ///arrange
        unsigned char fake;
        MESSAGE_CONFIG c = { 1, &fake, (MAP_HANDLE)&fake };

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
            .IgnoreArgument(1);

        whenShallCONSTBUFFER_Create_fail = 1;
        STRICT_EXPECTED_CALL(CONSTBUFFER_Create(&fake, 1)); /* this is copying the buffer*/
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);


        ///act
        MESSAGE_HANDLE r = Message_Create(&c);

        ///assert
        ASSERT_IS_NULL(r);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_005: [If Message_Create encounters an error while building the internal structures of the message, then it shall return NULL.]*/
    TEST_FUNCTION(Message_Create_nonzero_size_fails_when_malloc_fails_2)
    {
        ///arrange
        unsigned char fake;
        MESSAGE_CONFIG c = { 1, &fake, (MAP_HANDLE)&fake };

        whenShallmalloc_fail = 1;
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
            .IgnoreArgument(1);

        ///act
        MESSAGE_HANDLE r = Message_Create(&c);

        ///assert
        ASSERT_IS_NULL(r);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

	/* Tests_SRS_MESSAGE_17_008: [ If cfg is NULL then Message_CreateFromBuffer shall return NULL.]*/
	TEST_FUNCTION(Message_CreateFromBuffer_with_NULL_parameter_fails)
	{
		///arrange

		///act
		MESSAGE_HANDLE r = Message_CreateFromBuffer(NULL);

		///assert
		ASSERT_IS_NULL(r);
		ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

		///cleanup
	}

	/*Tests_SRS_MESSAGE_17_009: [If field sourceContent of cfg is NULL, then Message_CreateFromBuffer shall fail and return NULL.]*/
	TEST_FUNCTION(Message_CreateFromBuffer_with_NULL_content_fails)
	{
		///arrange
		MESSAGE_BUFFER_CONFIG cfg =
		{
			NULL,
			NULL
		};
	/*	typedef struct MESSAGE_BUFFER_CONFIG_TAG
		{
			CONSTBUFFER_HANDLE sourceContent;
			MAP_HANDLE sourceProperties;
		}MESSAGE_BUFFER_CONFIG;*/

		///act
		MESSAGE_HANDLE r = Message_CreateFromBuffer(&cfg);

		///assert
		ASSERT_IS_NULL(r);
		ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

		///cleanup
	}

	/*Tests_SRS_MESSAGE_17_010: [If field sourceProperties of cfg is NULL, then Message_CreateFromBuffer shall fail and return NULL.]*/
	TEST_FUNCTION(Message_CreateFromBuffer_with_NULL_properties_fails)
	{
		///arrange
		CONSTBUFFER_HANDLE buffer = CONSTBUFFER_Create(NULL, 0);

		umock_c_reset_all_calls();

		MESSAGE_BUFFER_CONFIG cfg =
		{
			buffer,
			NULL
		};

		///act
		MESSAGE_HANDLE r = Message_CreateFromBuffer(&cfg);

		///assert
		ASSERT_IS_NULL(r);
		ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

		///cleanup
		CONSTBUFFER_Destroy(buffer);

	}

	/*Tests_SRS_MESSAGE_17_014: [On success, Message_CreateFromBuffer shall return a non-NULL handle and set the internal ref count to "1".]*/
	/*Tests_SRS_MESSAGE_17_012: [Message_CreateFromBuffer shall copy the sourceProperties to a readonly CONSTMAP.]*/
	/*Tests_SRS_MESSAGE_17_013: [Message_CreateFromBuffer shall clone the CONSTBUFFER sourceBuffer.]*/
	TEST_FUNCTION(Message_CreateFromBuffer_Success)
	{
		///arrange
		unsigned char fake;
		CONSTBUFFER_HANDLE buffer = CONSTBUFFER_Create(&fake, 1);
		MESSAGE_BUFFER_CONFIG cfg =
		{
			buffer,
			(MAP_HANDLE)&fake
		};

		umock_c_reset_all_calls();

		STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(CONSTBUFFER_Clone(buffer)); /*this is copying the buffer*/

		STRICT_EXPECTED_CALL(ConstMap_Create((MAP_HANDLE)&fake)); /*this is copying the properties*/


		///act
		MESSAGE_HANDLE r = Message_CreateFromBuffer(&cfg);

		///assert
		ASSERT_IS_NOT_NULL(r);
		ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

		///cleanup
		Message_Destroy(r);
		CONSTBUFFER_Destroy(buffer);

	}

	/*Tests_SRS_MESSAGE_17_011: [If Message_CreateFromBuffer encounters an error while building the internal structures of the message, then it shall return NULL.]*/
	TEST_FUNCTION(Message_CreateFromBuffer_Allocation_Failed)
	{
		///arrange
		unsigned char fake;
		CONSTBUFFER_HANDLE buffer = CONSTBUFFER_Create(&fake, 1);
		MESSAGE_BUFFER_CONFIG cfg =
		{
			buffer,
			(MAP_HANDLE)&fake
		};

		whenShallmalloc_fail = 1;

		umock_c_reset_all_calls();

		STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
			.IgnoreArgument(1);

		///act
		MESSAGE_HANDLE r = Message_CreateFromBuffer(&cfg);

		///assert
		ASSERT_IS_NULL(r);
		ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

		///cleanup
		CONSTBUFFER_Destroy(buffer);

	}

	/*Tests_SRS_MESSAGE_17_011: [If Message_CreateFromBuffer encounters an error while building the internal structures of the message, then it shall return NULL.]*/
	TEST_FUNCTION(Message_CreateFromBuffer_Buffer_Clone_Failed)
	{
		///arrange
		unsigned char fake;
		CONSTBUFFER_HANDLE buffer = CONSTBUFFER_Create(&fake, 1);
		MESSAGE_BUFFER_CONFIG cfg =
		{
			buffer,
			(MAP_HANDLE)&fake
		};

		whenShallCONSTBUFFER_Clone_fail = 1;
		umock_c_reset_all_calls();

		STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(CONSTBUFFER_Clone(buffer)); /*this is copying the buffer*/
		STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

		///act
		MESSAGE_HANDLE r = Message_CreateFromBuffer(&cfg);

		///assert
		ASSERT_IS_NULL(r);
		ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

		//cleanup
		CONSTBUFFER_Destroy(buffer);
	}

	/*Tests_SRS_MESSAGE_17_011: [If Message_CreateFromBuffer encounters an error while building the internal structures of the message, then it shall return NULL.]*/
	TEST_FUNCTION(Message_CreateFromBuffer_Properties_Copy_Failed)
	{
		///arrange
		unsigned char fake;
		CONSTBUFFER_HANDLE buffer = CONSTBUFFER_Create(&fake, 1);
		MESSAGE_BUFFER_CONFIG cfg =
		{
			buffer,
			(MAP_HANDLE)&fake
		};

		whenShallConstMap_Create_fail = 1;
		umock_c_reset_all_calls();

		STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
			.IgnoreArgument(1);
        {
            
            STRICT_EXPECTED_CALL(CONSTBUFFER_Clone(buffer)); /*this is copying the buffer*/
            {
                STRICT_EXPECTED_CALL(ConstMap_Create((MAP_HANDLE)&fake)); /*this is copying the properties*/
                STRICT_EXPECTED_CALL(CONSTBUFFER_Destroy(buffer));
            }

            STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
        }


		///act
		MESSAGE_HANDLE r = Message_CreateFromBuffer(&cfg);

		///assert
		ASSERT_IS_NULL(r);
		ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

		///cleanup
		CONSTBUFFER_Destroy(buffer);
	}

    /*Tests_SRS_MESSAGE_02_007: [If messageHandle is NULL then Message_Clone shall return NULL.] */
    TEST_FUNCTION(Message_Clone_with_NULL_argument_returns_NULL)
    {
        ///arrange

        ///act
        MESSAGE_HANDLE r = Message_Clone(NULL);

        ///assert
        ASSERT_IS_NULL(r);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_010: [Message_Clone shall return messageHandle.]*/
	/*Tests_SRS_MESSAGE_17_001: [Message_Clone shall clone the CONSTMAP handle.]*/
	/*Tests_SRS_MESSAGE_17_004: [Message_Clone shall clone the CONSTBUFFER handle]*/
    TEST_FUNCTION(Message_Clone_increments_ref_count_1)
    {
        ///arrange
        MESSAGE_CONFIG c = {0, NULL, (MAP_HANDLE)&c};
        MESSAGE_HANDLE aMessage = Message_Create(&c);
        umock_c_reset_all_calls();

		STRICT_EXPECTED_CALL(ConstMap_Clone(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(CONSTBUFFER_Clone(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

        ///act
        MESSAGE_HANDLE r = Message_Clone(aMessage);

        ///assert
        ASSERT_ARE_EQUAL(void_ptr, aMessage, r);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        Message_Destroy(r);
        Message_Destroy(aMessage);
    }

    /*Tests_SRS_MESSAGE_02_008: [Otherwise, Message_Clone shall increment the internal ref count.] */
    TEST_FUNCTION(Message_Clone_increments_ref_count_2)
    {
        ///arrange
        MESSAGE_CONFIG c = { 0, NULL, (MAP_HANDLE)&c};
        MESSAGE_HANDLE aMessage = Message_Create(&c);
        MESSAGE_HANDLE r = Message_Clone(aMessage);
        umock_c_reset_all_calls();

		STRICT_EXPECTED_CALL(ConstMap_Destroy(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(CONSTBUFFER_Destroy(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

        ///act
        Message_Destroy(r);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        Message_Destroy(aMessage);
    }

    /*Tests_SRS_MESSAGE_02_008: [Otherwise, Message_Clone shall increment the internal ref count.] */
    TEST_FUNCTION(Message_Clone_increments_ref_count_3)
    {
        ///arrange
        MESSAGE_CONFIG c = { 0, NULL, (MAP_HANDLE)&c};
        MESSAGE_HANDLE aMessage = Message_Create(&c);
        MESSAGE_HANDLE r = Message_Clone(aMessage);
        Message_Destroy(r);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(ConstMap_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(CONSTBUFFER_Destroy(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*only 1 because the message is 0 size*/
			.IgnoreArgument(1);

        ///act
        Message_Destroy(aMessage);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_011: [If message is NULL then Message_GetProperties shall return NULL.] */
    TEST_FUNCTION(Message_GetProperties_with_NULL_messageHandle_returns_NULL)
    {
        ///arrange
        umock_c_reset_all_calls();

        ///act
        CONSTMAP_HANDLE r = Message_GetProperties(NULL);

        ///assert
        ASSERT_IS_NULL(r);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_012: [Otherwise, Message_GetProperties shall shall clone and return the CONSTMAP handle representing the properties of the message.]*/
    TEST_FUNCTION(Message_GetProperties_happy_path)
    {
        ///arrange
        MESSAGE_CONFIG c = { 0, NULL, (MAP_HANDLE)&c };
        MESSAGE_HANDLE aMessage = Message_Create(&c);
        umock_c_reset_all_calls();

		STRICT_EXPECTED_CALL(ConstMap_Clone(IGNORED_PTR_ARG)).IgnoreArgument(1);

        ///act
        CONSTMAP_HANDLE theProperties = Message_GetProperties(aMessage);

        ///assert
        ASSERT_IS_NOT_NULL(theProperties);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        Message_Destroy(aMessage);
		ConstMap_Destroy(theProperties);
    }

    /*Tests_SRS_MESSAGE_02_013: [If message is NULL then Message_GetContent shall return NULL.] */
    TEST_FUNCTION(Message_GetContent_with_NULL_message_returns_NULL)
    {
        ///arrange

        ///act
        const CONSTBUFFER* content = Message_GetContent(NULL);

        ///assert
        ASSERT_IS_NULL(content);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_014: [Otherwise, Message_GetContent shall return a non-NULL const pointer to a structure of type CONSTBUFFER.]*/
    /*Tests_SRS_MESSAGE_02_015: [The MESSAGE_CONTENT's field size shall have the same value as the cfg's field size.]*/
    /*Tests_SRS_MESSAGE_02_016: [The MESSAGE_CONTENT's field data shall compare equal byte-by-byte to the cfg's field source.]*/
    TEST_FUNCTION(Message_GetContent_with_non_NULL_message_zero_size_succeeds)
    {
        ///arrange
        MESSAGE_CONFIG c = { 0, NULL, (MAP_HANDLE)&c};
        MESSAGE_HANDLE msg = Message_Create(&c);
        umock_c_reset_all_calls();

		STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

        ///act
        const CONSTBUFFER* content = Message_GetContent(msg);

        ///assert
        ASSERT_IS_NOT_NULL(content);
        ASSERT_ARE_EQUAL(size_t, 0, content->size);
        ASSERT_IS_NULL(content->buffer);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        Message_Destroy(msg);
    }

    /*Tests_SRS_MESSAGE_02_014: [Otherwise, Message_GetContent shall return a non-NULL const pointer to a structure of type CONSTBUFFER.]*/
    /*Tests_SRS_MESSAGE_02_015: [The MESSAGE_CONTENT's field size shall have the same value as the cfg's field size.]*/
    /*Tests_SRS_MESSAGE_02_016: [The MESSAGE_CONTENT's field data shall compare equal byte-by-byte to the cfg's field source.]*/
    TEST_FUNCTION(Message_GetContent_with_non_NULL_message_nonzero_size_succeeds)
    {
        ///arrange
        char t ='3';
        MESSAGE_CONFIG c = { sizeof(t), (unsigned char*)&t, (MAP_HANDLE)&c};
        MESSAGE_HANDLE msg = Message_Create(&c);
        umock_c_reset_all_calls();

		STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

        ///act
        const CONSTBUFFER* content = Message_GetContent(msg);

        ///assert
        ASSERT_IS_NOT_NULL(content);
        ASSERT_ARE_EQUAL(size_t, 1, content->size);
        ASSERT_ARE_EQUAL(int, 0, memcmp(content->buffer, &t, 1));
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        Message_Destroy(msg);
    }

	/*Tests_SRS_MESSAGE_17_006: [If message is NULL then Message_GetContentHandle shall return NULL.]*/
	TEST_FUNCTION(Message_GetContentHandle_with_NULL_message_returns_NULL)
	{
		///arrange

		///act
		CONSTBUFFER_HANDLE content = Message_GetContentHandle(NULL);

		///assert
		ASSERT_IS_NULL(content);
		ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

		///cleanup
	}

	/*Tests_SRS_MESSAGE_17_007: [Otherwise, Message_GetContentHandle shall shall clone and return the CONSTBUFFER_HANDLE representing the message content.]*/
	TEST_FUNCTION(Message_GetContentHandle_with_non_NULL_message_zero_size_succeeds)
	{
		///arrange
		MESSAGE_CONFIG c = { 0, NULL, (MAP_HANDLE)&c};
		MESSAGE_HANDLE msg = Message_Create(&c);
		umock_c_reset_all_calls();

		STRICT_EXPECTED_CALL(CONSTBUFFER_Clone(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

		///act
        CONSTBUFFER_HANDLE content = Message_GetContentHandle(msg);

		///assert
		ASSERT_IS_NOT_NULL(content);
		ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

		const CONSTBUFFER * contentBuffer = CONSTBUFFER_GetContent(content);
		ASSERT_ARE_EQUAL(size_t, 0, contentBuffer->size);
		ASSERT_IS_NULL(contentBuffer->buffer);

		///cleanup
		Message_Destroy(msg);
		CONSTBUFFER_Destroy(content);

	}

	/*Tests_SRS_MESSAGE_17_007: [Otherwise, Message_GetContentHandle shall shall clone and return the CONSTBUFFER_HANDLE representing the message content.]*/
	TEST_FUNCTION(Message_GetContentHandle_with_non_NULL_message_nonzero_size_succeeds)
	{
		///arrange
		char t = '3';
		MESSAGE_CONFIG c = { sizeof(t), (unsigned char*)&t, (MAP_HANDLE)&c};
		MESSAGE_HANDLE msg = Message_Create(&c);
		umock_c_reset_all_calls();

		STRICT_EXPECTED_CALL(CONSTBUFFER_Clone(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

		///act
        CONSTBUFFER_HANDLE content = Message_GetContentHandle(msg);

		///assert
		ASSERT_IS_NOT_NULL(content);
		ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

		const CONSTBUFFER * contentBuffer = CONSTBUFFER_GetContent(content);
		ASSERT_ARE_EQUAL(size_t, 1, contentBuffer->size);
		ASSERT_ARE_EQUAL(int, 0, memcmp(contentBuffer->buffer, &t, 1));

		///cleanup
		Message_Destroy(msg);
		CONSTBUFFER_Destroy(content);
	}

    /*Tests_SRS_MESSAGE_02_017: [If message is NULL then Message_Destroy shall do nothing.] */
    TEST_FUNCTION(Message_Destroy_with_NULL_argument_does_nothing)
    {
        ///arrange

        ///act
        Message_Destroy(NULL);

        ///assert
        
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_020: [Otherwise, Message_Destroy shall decrement the internal ref count of the message.] 
    /*Tests_SRS_MESSAGE_02_021: [If the ref count is zero then the allocated resources are freed.]*/
	/*Tests_SRS_MESSAGE_17_002: [Message_Destroy shall destroy the CONSTMAP properties.]*/
	/*Tests_SRS_MESSAGE_17_005: [Message_Destroy shall destroy the CONSTBUFFER.]*/
    TEST_FUNCTION(Message_Destroy_happy_path)
    {
        ///arrange
        char t = '3';
        MESSAGE_CONFIG c = { sizeof(t), (unsigned char*)&t, (MAP_HANDLE)&c };
        MESSAGE_HANDLE msg = Message_Create(&c);
        umock_c_reset_all_calls();

		STRICT_EXPECTED_CALL(ConstMap_Destroy(IGNORED_PTR_ARG)) /*this is the map*/
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(CONSTBUFFER_Destroy(IGNORED_PTR_ARG)) /*this is the buffer*/
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*this is the handle*/
            .IgnoreArgument(1);

        ///act
        Message_Destroy(msg);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_022: [ If source is NULL then Message_CreateFromByteArray shall fail and return NULL. ]*/
    TEST_FUNCTION(Message_CreateFromByteArray_with_NULL_source_fails)
    {
        ///arrange

        ///act
        
        MESSAGE_HANDLE handle = Message_CreateFromByteArray(NULL, 1);

        ///assert

        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        
        ///cleanup

    }



    /*Tests_SRS_MESSAGE_02_023: [ If source is not NULL and and size parameter is smaller than 14 then Message_CreateFromByteArray shall fail and return NULL. ]*/
    TEST_FUNCTION(Message_CreateFromByteArray_with_13_size_fails)
    {
        ///arrange

        ///act

        MESSAGE_HANDLE handle = Message_CreateFromByteArray(notFail____minimalMessage, 13);

        ///assert

        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_031: [ Otherwise Message_CreateFromByteArray shall succeed and return a non-NULL handle. ]*/
    /*Tests_SRS_MESSAGE_02_026: [ A MAP_HANDLE shall be created. ]*/
    /*Tests_SRS_MESSAGE_02_028: [ A structure of type MESSAGE_CONFIG shall be populated with the MAP_HANDLE previously constructed and the message content ]*/
    /*Tests_SRS_MESSAGE_02_029: [ A MESSAGE_HANDLE shall be constructed from the MESSAGE_CONFIG. ]*/
    TEST_FUNCTION(Message_CreateFromByteArray_notFail____minimalMessage)
    {

        ///arrange

        STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG))
            .IgnoreArgument_mapFilterFunc()
            .SetReturn(TEST_MAP_HANDLE);
        EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreAllCalls();
        STRICT_EXPECTED_CALL(CONSTBUFFER_Create(notFail____minimalMessage + 14, 0));
        STRICT_EXPECTED_CALL(ConstMap_Create(TEST_MAP_HANDLE));
        STRICT_EXPECTED_CALL(Map_Destroy(TEST_MAP_HANDLE));

        ///act
        MESSAGE_HANDLE handle = Message_CreateFromByteArray(notFail____minimalMessage, sizeof(notFail____minimalMessage));
        
        ///assert
        ASSERT_IS_NOT_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        Message_Destroy(handle);
    }

    /*Tests_SRS_MESSAGE_02_031: [ Otherwise Message_CreateFromByteArray shall succeed and return a non-NULL handle. ]*/
    /*Tests_SRS_MESSAGE_02_027: [ All the properties of the byte array shall be added to the MAP_HANDLE. ]*/
    TEST_FUNCTION(Message_CreateFromByteArray_notFail__1Property_0bytes)
    {

        ///arrange

        STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG))
            .IgnoreArgument_mapFilterFunc()
            .SetReturn(TEST_MAP_HANDLE);
        STRICT_EXPECTED_CALL(Map_Add(TEST_MAP_HANDLE, "3", "3"))
            .SetReturn(MAP_OK);

        EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreAllCalls();
        STRICT_EXPECTED_CALL(CONSTBUFFER_Create(IGNORED_PTR_ARG, 0))
            .IgnoreArgument_source();
        STRICT_EXPECTED_CALL(ConstMap_Create(TEST_MAP_HANDLE));
        STRICT_EXPECTED_CALL(Map_Destroy(TEST_MAP_HANDLE));

        ///act
        MESSAGE_HANDLE handle = Message_CreateFromByteArray(notFail__1Property_0bytes, sizeof(notFail__1Property_0bytes));

        ///assert
        ASSERT_IS_NOT_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        Message_Destroy(handle);
    }

    /*Tests_SRS_MESSAGE_02_031: [ Otherwise Message_CreateFromByteArray shall succeed and return a non-NULL handle. ]*/
    TEST_FUNCTION(Message_CreateFromByteArray_notFail__2Property_0bytes)
    {

        ///arrange

        STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG))
            .IgnoreArgument_mapFilterFunc()
            .SetReturn(TEST_MAP_HANDLE);
        STRICT_EXPECTED_CALL(Map_Add(TEST_MAP_HANDLE, "3", "3"))
            .SetReturn(MAP_OK);
        STRICT_EXPECTED_CALL(Map_Add(TEST_MAP_HANDLE, "ab", "a"))
            .SetReturn(MAP_OK);

        EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreAllCalls();
        STRICT_EXPECTED_CALL(CONSTBUFFER_Create(IGNORED_PTR_ARG, 0))
            .IgnoreArgument_source();
        STRICT_EXPECTED_CALL(ConstMap_Create(TEST_MAP_HANDLE));
        STRICT_EXPECTED_CALL(Map_Destroy(TEST_MAP_HANDLE));

        ///act
        MESSAGE_HANDLE handle = Message_CreateFromByteArray(notFail__2Property_0bytes, sizeof(notFail__2Property_0bytes));

        ///assert
        ASSERT_IS_NOT_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        Message_Destroy(handle);
    }

    /*Tests_SRS_MESSAGE_02_031: [ Otherwise Message_CreateFromByteArray shall succeed and return a non-NULL handle. ]*/
    TEST_FUNCTION(Message_CreateFromByteArray_notFail__0Property_1bytes)
    {

        ///arrange

        STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG))
            .IgnoreArgument_mapFilterFunc()
            .SetReturn(TEST_MAP_HANDLE);

        EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreAllCalls();
        STRICT_EXPECTED_CALL(CONSTBUFFER_Create(IGNORED_PTR_ARG, 1))
            .IgnoreArgument_source();
        STRICT_EXPECTED_CALL(ConstMap_Create(TEST_MAP_HANDLE));
        STRICT_EXPECTED_CALL(Map_Destroy(TEST_MAP_HANDLE));

        ///act
        MESSAGE_HANDLE handle = Message_CreateFromByteArray(notFail__0Property_1bytes, sizeof(notFail__0Property_1bytes));

        ///assert
        ASSERT_IS_NOT_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        Message_Destroy(handle);
    }

    
    /*Tests_SRS_MESSAGE_02_031: [ Otherwise Message_CreateFromByteArray shall succeed and return a non-NULL handle. ]*/
    TEST_FUNCTION(Message_CreateFromByteArray_notFail__1Property_1bytes)
    {

        ///arrange

        STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG))
            .IgnoreArgument_mapFilterFunc()
            .SetReturn(TEST_MAP_HANDLE);
        STRICT_EXPECTED_CALL(Map_Add(TEST_MAP_HANDLE, "Azure IoT Gateway is", "awesome"));

        EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreAllCalls();
        STRICT_EXPECTED_CALL(CONSTBUFFER_Create(IGNORED_PTR_ARG, 1))
            .ValidateArgumentBuffer(1, "3", 1);
        STRICT_EXPECTED_CALL(ConstMap_Create(TEST_MAP_HANDLE));
        STRICT_EXPECTED_CALL(Map_Destroy(TEST_MAP_HANDLE));

        ///act
        MESSAGE_HANDLE handle = Message_CreateFromByteArray(notFail__1Property_1bytes, sizeof(notFail__1Property_1bytes));

        ///assert
        ASSERT_IS_NOT_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        Message_Destroy(handle);
    }

    
    /*Tests_SRS_MESSAGE_02_031: [ Otherwise Message_CreateFromByteArray shall succeed and return a non-NULL handle. ]*/
    TEST_FUNCTION(Message_CreateFromByteArray_notFail__2Property_1bytes)
    {

        ///arrange

        STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG))
            .IgnoreArgument_mapFilterFunc()
            .SetReturn(TEST_MAP_HANDLE);
        STRICT_EXPECTED_CALL(Map_Add(TEST_MAP_HANDLE, "Azure IoT Gateway is", "awesome"));
        STRICT_EXPECTED_CALL(Map_Add(TEST_MAP_HANDLE, "BleedingEdge", "rocks"));

        EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreAllCalls();
        STRICT_EXPECTED_CALL(CONSTBUFFER_Create(IGNORED_PTR_ARG, 1))
            .ValidateArgumentBuffer(1, "3", 1);
        STRICT_EXPECTED_CALL(ConstMap_Create(TEST_MAP_HANDLE));
        STRICT_EXPECTED_CALL(Map_Destroy(TEST_MAP_HANDLE));

        ///act
        MESSAGE_HANDLE handle = Message_CreateFromByteArray(notFail__2Property_1bytes, sizeof(notFail__2Property_1bytes));

        ///assert
        ASSERT_IS_NOT_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        Message_Destroy(handle);
    }

    /*Tests_SRS_MESSAGE_02_031: [ Otherwise Message_CreateFromByteArray shall succeed and return a non-NULL handle. ]*/
    TEST_FUNCTION(Message_CreateFromByteArray_notFail__0Property_2bytes)
    {

        ///arrange

        STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG))
            .IgnoreArgument_mapFilterFunc()
            .SetReturn(TEST_MAP_HANDLE);

        EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreAllCalls();
        STRICT_EXPECTED_CALL(CONSTBUFFER_Create(IGNORED_PTR_ARG, 2))
            .ValidateArgumentBuffer(1, "34", 2);
        STRICT_EXPECTED_CALL(ConstMap_Create(TEST_MAP_HANDLE));
        STRICT_EXPECTED_CALL(Map_Destroy(TEST_MAP_HANDLE));

        ///act
        MESSAGE_HANDLE handle = Message_CreateFromByteArray(notFail__0Property_2bytes, sizeof(notFail__0Property_2bytes));

        ///assert
        ASSERT_IS_NOT_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        Message_Destroy(handle);
    }

    /*Tests_SRS_MESSAGE_02_031: [ Otherwise Message_CreateFromByteArray shall succeed and return a non-NULL handle. ]*/
    TEST_FUNCTION(Message_CreateFromByteArray_notFail__1Property_2bytes)
    {

        ///arrange

        STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG))
            .IgnoreArgument_mapFilterFunc()
            .SetReturn(TEST_MAP_HANDLE);
        STRICT_EXPECTED_CALL(Map_Add(TEST_MAP_HANDLE, "Azure IoT Gateway is", "awesome"));
        EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreAllCalls();
        STRICT_EXPECTED_CALL(CONSTBUFFER_Create(IGNORED_PTR_ARG, 2))
            .ValidateArgumentBuffer(1, "34", 2);
        STRICT_EXPECTED_CALL(ConstMap_Create(TEST_MAP_HANDLE));
        STRICT_EXPECTED_CALL(Map_Destroy(TEST_MAP_HANDLE));

        ///act
        MESSAGE_HANDLE handle = Message_CreateFromByteArray(notFail__1Property_2bytes, sizeof(notFail__1Property_2bytes));

        ///assert
        ASSERT_IS_NOT_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        Message_Destroy(handle);
    }

    /*Tests_SRS_MESSAGE_02_031: [ Otherwise Message_CreateFromByteArray shall succeed and return a non-NULL handle. ]*/
    TEST_FUNCTION(Message_CreateFromByteArray_notFail__2Property_2bytes)
    {

        ///arrange

        STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG))
            .IgnoreArgument_mapFilterFunc()
            .SetReturn(TEST_MAP_HANDLE);
        STRICT_EXPECTED_CALL(Map_Add(TEST_MAP_HANDLE, "BleedingEdge", "rocks"));
        STRICT_EXPECTED_CALL(Map_Add(TEST_MAP_HANDLE, "Azure IoT Gateway is", "awesome"));
        EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreAllCalls();
        STRICT_EXPECTED_CALL(CONSTBUFFER_Create(IGNORED_PTR_ARG, 2))
            .ValidateArgumentBuffer(1, "34", 2);
        STRICT_EXPECTED_CALL(ConstMap_Create(TEST_MAP_HANDLE));
        STRICT_EXPECTED_CALL(Map_Destroy(TEST_MAP_HANDLE));

        ///act
        MESSAGE_HANDLE handle = Message_CreateFromByteArray(notFail__2Property_2bytes, sizeof(notFail__2Property_2bytes));

        ///assert
        ASSERT_IS_NOT_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        Message_Destroy(handle);
    }

    /*Tests_SRS_MESSAGE_02_024: [ If the first two bytes of source are not 0xA1 0x60 then Message_CreateFromByteArray shall fail and return NULL. ]*/
    TEST_FUNCTION(Message_CreateFromByteArray_when_first_byte_is_not_0xA1_fails)
    {

        ///arrange

        ///act
        MESSAGE_HANDLE handle = Message_CreateFromByteArray(fail_____firstByteNot0xA1, sizeof(fail_____firstByteNot0xA1));

        ///assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_024: [ If the first two bytes of source are not 0xA1 0x60 then Message_CreateFromByteArray shall fail and return NULL. ]*/
    TEST_FUNCTION(Message_CreateFromByteArray_when_second_byte_is_not_0x60_fails)
    {

        ///arrange

        ///act
        MESSAGE_HANDLE handle = Message_CreateFromByteArray(fail____secondByteNot0x60, sizeof(fail____secondByteNot0x60));

        ///assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_037: [ If the size embedded in the message is not the same as size parameter then Message_CreateFromByteArray shall fail and return NULL. ]*/
    
    TEST_FUNCTION(Message_CreateFromByteArray_when_message_sizes_not_math_fails)
    {
        ///arrange

        ///act
        MESSAGE_HANDLE handle = Message_CreateFromByteArray(notFail____minimalMessage,1+ sizeof(notFail____minimalMessage));

        ///assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_025: [ If while parsing the message content, a read would occur past the end of the array (as indicated by size) then Message_CreateFromByteArray shall fail and return NULL. ]*/
    TEST_FUNCTION(Message_CreateFromByteArray_with_1_property_when_1st_property_doesnt_end_fails)
    {
        ///arrange
        STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG))
            .IgnoreArgument_mapFilterFunc()
            .SetReturn(TEST_MAP_HANDLE);

        STRICT_EXPECTED_CALL(Map_Destroy(TEST_MAP_HANDLE));

        ///act
        MESSAGE_HANDLE handle = Message_CreateFromByteArray(fail_firstPropertyNameTooBig, sizeof(fail_firstPropertyNameTooBig));

        ///assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_025: [ If while parsing the message content, a read would occur past the end of the array (as indicated by size) then Message_CreateFromByteArray shall fail and return NULL. ]*/
    TEST_FUNCTION(Message_CreateFromByteArray_with_1_property_when_1st_property_value_doesnt_start_fails)
    {
        ///arrange
        STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG))
            .IgnoreArgument_mapFilterFunc()
            .SetReturn(TEST_MAP_HANDLE);

        STRICT_EXPECTED_CALL(Map_Destroy(TEST_MAP_HANDLE));

        ///act
        MESSAGE_HANDLE handle = Message_CreateFromByteArray(fail_firstPropertyValueDoesNotExist, sizeof(fail_firstPropertyValueDoesNotExist));

        ///assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }
    
    /*Tests_SRS_MESSAGE_02_025: [ If while parsing the message content, a read would occur past the end of the array (as indicated by size) then Message_CreateFromByteArray shall fail and return NULL. ]*/
    TEST_FUNCTION(Message_CreateFromByteArray_with_1_property_when_1st_property_value_doesnt_end_fails)
    {
        ///arrange
        STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG))
            .IgnoreArgument_mapFilterFunc()
            .SetReturn(TEST_MAP_HANDLE);

        STRICT_EXPECTED_CALL(Map_Destroy(TEST_MAP_HANDLE));

        ///act
        MESSAGE_HANDLE handle = Message_CreateFromByteArray(fail_firstPropertyValueDoesNotEnd, sizeof(fail_firstPropertyValueDoesNotEnd));

        ///assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_025: [ If while parsing the message content, a read would occur past the end of the array (as indicated by size) then Message_CreateFromByteArray shall fail and return NULL. ]*/
    TEST_FUNCTION(Message_CreateFromByteArray_with_1_byte_of_content_size_fails)
    {
        ///arrange
        STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG))
            .IgnoreArgument_mapFilterFunc()
            .SetReturn(TEST_MAP_HANDLE);
        STRICT_EXPECTED_CALL(Map_Add(TEST_MAP_HANDLE, "333", "333"));
        STRICT_EXPECTED_CALL(Map_Destroy(TEST_MAP_HANDLE));

        ///act
        MESSAGE_HANDLE handle = Message_CreateFromByteArray(fail_whenThereIsOnly1ByteOfcontentSize, sizeof(fail_whenThereIsOnly1ByteOfcontentSize));

        ///assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_025: [ If while parsing the message content, a read would occur past the end of the array (as indicated by size) then Message_CreateFromByteArray shall fail and return NULL. ]*/
    TEST_FUNCTION(Message_CreateFromByteArray_with_2_byte_of_content_size_fails)
    {
        ///arrange

        const unsigned char fail_whenThereIsOnly2ByteOfcontentSize[] =
        {
            0xA1, 0x60,             /*header*/
            0x00, 0x00, 0x00, 20,   /*size of this array*/
            0x00, 0x00, 0x00, 0x01, /*one property*/
            '3', '3', '3', '\0',
            '3', '3', '3', '\0',
            0x00, 0x00              /*not enough bytes for contentSize*/
        };

        STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG))
            .IgnoreArgument_mapFilterFunc()
            .SetReturn(TEST_MAP_HANDLE);
        STRICT_EXPECTED_CALL(Map_Add(TEST_MAP_HANDLE, "333", "333"));
        STRICT_EXPECTED_CALL(Map_Destroy(TEST_MAP_HANDLE));

        ///act
        MESSAGE_HANDLE handle = Message_CreateFromByteArray(fail_whenThereIsOnly2ByteOfcontentSize, sizeof(fail_whenThereIsOnly2ByteOfcontentSize));

        ///assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_025: [ If while parsing the message content, a read would occur past the end of the array (as indicated by size) then Message_CreateFromByteArray shall fail and return NULL. ]*/
    TEST_FUNCTION(Message_CreateFromByteArray_with_3_byte_of_content_size_fails)
    {
        ///arrange

        const unsigned char fail_whenThereIsOnly3ByteOfcontentSize[] =
        {
            0xA1, 0x60,             /*header*/
            0x00, 0x00, 0x00, 21,   /*size of this array*/
            0x00, 0x00, 0x00, 0x01, /*one property*/
            '3', '3', '3', '\0',
            '3', '3', '3', '\0',
            0x00, 0x00, 0x00        /*not enough bytes for contentSize*/
        };

        STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG))
            .IgnoreArgument_mapFilterFunc()
            .SetReturn(TEST_MAP_HANDLE);
        STRICT_EXPECTED_CALL(Map_Add(TEST_MAP_HANDLE, "333", "333"));
        STRICT_EXPECTED_CALL(Map_Destroy(TEST_MAP_HANDLE));

        ///act
        MESSAGE_HANDLE handle = Message_CreateFromByteArray(fail_whenThereIsOnly3ByteOfcontentSize, sizeof(fail_whenThereIsOnly3ByteOfcontentSize));

        ///assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_025: [ If while parsing the message content, a read would occur past the end of the array (as indicated by size) then Message_CreateFromByteArray shall fail and return NULL. ]*/
    TEST_FUNCTION(Message_CreateFromByteArray_with_1_byte_of_content_size_but_no_content_fails)
    {
        ///arrange

        const unsigned char fail_whenThereIsNotEnoughContent[] =
        {
            0xA1, 0x60,             /*header*/
            0x00, 0x00, 0x00, 22,   /*size of this array*/
            0x00, 0x00, 0x00, 0x01, /*one property*/
            '3', '3', '3', '\0',
            '3', '3', '3', '\0',
            0x00, 0x00, 0x00, 0x01  /*no further content*/
        };

        STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG))
            .IgnoreArgument_mapFilterFunc()
            .SetReturn(TEST_MAP_HANDLE);
        STRICT_EXPECTED_CALL(Map_Add(TEST_MAP_HANDLE, "333", "333"));
        STRICT_EXPECTED_CALL(Map_Destroy(TEST_MAP_HANDLE));

        ///act
        MESSAGE_HANDLE handle = Message_CreateFromByteArray(fail_whenThereIsNotEnoughContent, sizeof(fail_whenThereIsNotEnoughContent));

        ///assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_025: [ If while parsing the message content, a read would occur past the end of the array (as indicated by size) then Message_CreateFromByteArray shall fail and return NULL. ]*/
    TEST_FUNCTION(Message_CreateFromByteArray_with_1_byte_of_content_size_but_2_bytes_of_content_fails)
    {
        ///arrange

        const unsigned char fail_whenThereIsTooMuchContent[] =
        {
            0xA1, 0x60,             /*header*/
            0x00, 0x00, 0x00, 24,   /*size of this array*/
            0x00, 0x00, 0x00, 0x01, /*one property*/
            '3', '3', '3', '\0',
            '3', '3', '3', '\0',
            0x00, 0x00, 0x00, 0x01,  /*no further content*/
            '3', '3'
        };

        STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG))
            .IgnoreArgument_mapFilterFunc()
            .SetReturn(TEST_MAP_HANDLE);
        STRICT_EXPECTED_CALL(Map_Add(TEST_MAP_HANDLE, "333", "333"));
        STRICT_EXPECTED_CALL(Map_Destroy(TEST_MAP_HANDLE));

        ///act
        MESSAGE_HANDLE handle = Message_CreateFromByteArray(fail_whenThereIsTooMuchContent, sizeof(fail_whenThereIsTooMuchContent));

        ///assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_030: [ If any of the above steps fails, then Message_CreateFromByteArray shall fail and return NULL. ]*/
    TEST_FUNCTION(Message_CreateFromByteArray_fails_when_Map_Create_fails)
    {
        ///arrange

        const unsigned char notFail____minimalMessage[] =
        {
            0xA1, 0x60,             /*header*/
            0x00, 0x00, 0x00, 14,   /*size of this array*/
            0x00, 0x00, 0x00, 0x00, /*zero properties*/
            0x00, 0x00, 0x00, 0x00  /*zero message content size*/
        };

        STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG))
            .IgnoreArgument_mapFilterFunc()
            .SetReturn(NULL);

        ///act
        MESSAGE_HANDLE handle = Message_CreateFromByteArray(notFail____minimalMessage, sizeof(notFail____minimalMessage));

        ///assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_030: [ If any of the above steps fails, then Message_CreateFromByteArray shall fail and return NULL. ]*/
    TEST_FUNCTION(Message_CreateFromByteArray_fails_when_numberOfProperties_is_negative)
    {
        ///arrange

        const unsigned char notFail____minimalMessage[] =
        {
            0xA1, 0x60,             /*header*/
            0x00, 0x00, 0x00, 14,   /*size of this array*/
            0xFF, 0xFF, 0xFF, 0xFF, /*-1 properties*/
            0x00, 0x00, 0x00, 0x00  /*zero message content size*/
        };

        STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG))
            .IgnoreArgument_mapFilterFunc()
            .SetReturn(TEST_MAP_HANDLE);
        STRICT_EXPECTED_CALL(Map_Destroy(TEST_MAP_HANDLE));

        ///act
        MESSAGE_HANDLE handle = Message_CreateFromByteArray(notFail____minimalMessage, sizeof(notFail____minimalMessage));

        ///assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_030: [ If any of the above steps fails, then Message_CreateFromByteArray shall fail and return NULL. ]*/
    TEST_FUNCTION(Message_CreateFromByteArray_fails_when_numberOfProperties_is_int32_max)
    {
        ///arrange

        const unsigned char notFail____minimalMessage[] =
        {
            0xA1, 0x60,             /*header*/
            0x00, 0x00, 0x00, 14,   /*size of this array*/
            0x7F, 0xFF, 0xFF, 0xFF, /*INT32_MAX properties*/
            0x00, 0x00, 0x00, 0x00  /*zero message content size*/
        };

        STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG))
            .IgnoreArgument_mapFilterFunc()
            .SetReturn(TEST_MAP_HANDLE);

        STRICT_EXPECTED_CALL(Map_Destroy(TEST_MAP_HANDLE));

        ///act
        MESSAGE_HANDLE handle = Message_CreateFromByteArray(notFail____minimalMessage, sizeof(notFail____minimalMessage));

        ///assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_030: [ If any of the above steps fails, then Message_CreateFromByteArray shall fail and return NULL. ]*/
    TEST_FUNCTION(Message_CreateFromByteArray_fails_when_Map_Add_fails)
    {

        ///arrange

        STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG))
            .IgnoreArgument_mapFilterFunc()
            .SetReturn(TEST_MAP_HANDLE);
        STRICT_EXPECTED_CALL(Map_Add(TEST_MAP_HANDLE, "3", "3"))
            .SetReturn(MAP_ERROR);

        STRICT_EXPECTED_CALL(Map_Destroy(TEST_MAP_HANDLE));

        ///act
        MESSAGE_HANDLE handle = Message_CreateFromByteArray(notFail__1Property_0bytes, sizeof(notFail__1Property_0bytes));

        ///assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        Message_Destroy(handle);
    }

    /*Tests_SRS_MESSAGE_02_032: [ If messageHandle is NULL then Message_ToByteArray shall fail and return NULL. ]*/
    TEST_FUNCTION(Message_ToByteArray_fails_with_NULL_messageHandle_parameter)
    {

        ///arrange
        int32_t size;

        ///act
        const unsigned char* byteArray = Message_ToByteArray(NULL, &size);

        ///assert
        ASSERT_IS_NULL(byteArray);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_038: [ If size is NULL then Message_ToByteArray shall fail and return NULL. ]*/
    TEST_FUNCTION(Message_ToByteArray_fails_with_NULL_size_parameter)
    {

        ///arrange

        ///act
        const unsigned char* byteArray = Message_ToByteArray(TEST_MESSAGE_HANDLE, NULL);

        ///assert
        ASSERT_IS_NULL(byteArray);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_MESSAGE_02_033: [ Message_ToByteArray shall precompute the needed memory size and shall pre allocate it. ]*/
    /*Tests_SRS_MESSAGE_02_034: [ Message_ToByteArray shall populate the memory with values as indicated in the implementation details. ]*/
    /*Tests_SRS_MESSAGE_02_036: [ Otherwise Message_ToByteArray shall succeed, write in *size the byte array size and return a non-NULL result. ]*/
    TEST_FUNCTION(Message_ToByteArray_no_properties_no_content_happy_path)
    {

        ///arrange
        int32_t size;

        STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG))
            .IgnoreArgument_mapFilterFunc()
            .SetReturn(TEST_MAP_HANDLE);
        EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreAllCalls();
        STRICT_EXPECTED_CALL(CONSTBUFFER_Create(IGNORED_PTR_ARG, 0))
            .IgnoreArgument_source();
        STRICT_EXPECTED_CALL(ConstMap_Create(TEST_MAP_HANDLE));
        STRICT_EXPECTED_CALL(Map_Destroy(TEST_MAP_HANDLE));

        MESSAGE_HANDLE messageHandle = Message_CreateFromByteArray(notFail____minimalMessage, sizeof(notFail____minimalMessage));

        size_t zero = 0;
        const CONSTBUFFER bufferContent = { NULL, 0 };

        STRICT_EXPECTED_CALL(ConstMap_GetInternals(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_handle()
            .IgnoreArgument_keys()
            .IgnoreArgument_values()
            .CopyOutArgumentBuffer(4, &zero, sizeof(zero));
        STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_PTR_ARG))
            .IgnoreArgument_constbufferHandle()
            .SetReturn(&bufferContent);
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument_size();

        ///act
        const unsigned char* byteArray = Message_ToByteArray(messageHandle, &size);

        ///assert
        ASSERT_IS_NOT_NULL(byteArray);
        ASSERT_ARE_EQUAL(int32_t, sizeof(notFail____minimalMessage), size);
        ASSERT_ARE_EQUAL(int, 0, memcmp(byteArray, notFail____minimalMessage, size));
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        free((void*)byteArray);
        Message_Destroy(messageHandle);
    }

    /*Tests_SRS_MESSAGE_02_033: [ Message_ToByteArray shall precompute the needed memory size and shall pre allocate it. ]*/
    /*Tests_SRS_MESSAGE_02_034: [ Message_ToByteArray shall populate the memory with values as indicated in the implementation details. ]*/
    /*Tests_SRS_MESSAGE_02_036: [ Otherwise Message_ToByteArray shall succeed, write in *size the byte array size and return a non-NULL result. ]*/
    TEST_FUNCTION(Message_ToByteArray_with_properties_and_content_happy_path)
    {

        ///arrange
        int32_t size;

        STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG))
            .IgnoreArgument_mapFilterFunc()
            .SetReturn(TEST_MAP_HANDLE);
        STRICT_EXPECTED_CALL(Map_Add(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(Map_Add(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreAllCalls();
        STRICT_EXPECTED_CALL(CONSTBUFFER_Create(IGNORED_PTR_ARG, 2))
            .IgnoreArgument_source();
        STRICT_EXPECTED_CALL(ConstMap_Create(TEST_MAP_HANDLE));
        STRICT_EXPECTED_CALL(Map_Destroy(TEST_MAP_HANDLE));

        MESSAGE_HANDLE messageHandle = Message_CreateFromByteArray(notFail__2Property_2bytes, sizeof(notFail__2Property_2bytes));

        size_t two = 2;
        const char* keys[] = { "BleedingEdge", "Azure IoT Gateway is" };
        const char* values[] = { "rocks", "awesome" };
        const char* const* *pkeys = (const char* const* *)&keys;
        const char* const* *pvalues = (const char* const* *)&values;

        const CONSTBUFFER bufferContent = { (const unsigned char*)"34", 2 };

        STRICT_EXPECTED_CALL(ConstMap_GetInternals(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_handle()
            .CopyOutArgumentBuffer(2, &pkeys, sizeof(char**))
            .CopyOutArgumentBuffer(3, &pvalues, sizeof(char**))
            .CopyOutArgumentBuffer(4, &two, sizeof(two));
        STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_PTR_ARG))
            .IgnoreArgument_constbufferHandle()
            .SetReturn(&bufferContent);
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument_size();

        ///act
        const unsigned char* byteArray = Message_ToByteArray(messageHandle, &size);

        ///assert
        ASSERT_IS_NOT_NULL(byteArray);
        ASSERT_ARE_EQUAL(int32_t, sizeof(notFail__2Property_2bytes), size);
        ASSERT_ARE_EQUAL(int, 0, memcmp(byteArray, notFail__2Property_2bytes, size));
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        free((void*)byteArray);
        Message_Destroy(messageHandle);
    }

    /*Tests_SRS_MESSAGE_02_035: [ If any of the above steps fails then Message_ToByteArray shall fail and return NULL. ]*/
    TEST_FUNCTION(Message_ToByteArray_with_properties_and_content_fails_when_ConstMap_GetInternals_fails)
    {

        ///arrange
        int32_t size;

        STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG))
            .IgnoreArgument_mapFilterFunc()
            .SetReturn(TEST_MAP_HANDLE);
        STRICT_EXPECTED_CALL(Map_Add(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(Map_Add(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreAllCalls();
        STRICT_EXPECTED_CALL(CONSTBUFFER_Create(IGNORED_PTR_ARG, 2))
            .IgnoreArgument_source();
        STRICT_EXPECTED_CALL(ConstMap_Create(TEST_MAP_HANDLE));
        STRICT_EXPECTED_CALL(Map_Destroy(TEST_MAP_HANDLE));

        MESSAGE_HANDLE messageHandle = Message_CreateFromByteArray(notFail__2Property_2bytes, sizeof(notFail__2Property_2bytes));

        size_t two = 2;
        const char* keys[] = { "BleedingEdge", "Azure IoT Gateway is" };
        const char* values[] = { "rocks", "awesome" };
        const char* const* *pkeys = (const char* const* *)&keys;
        const char* const* *pvalues = (const char* const* *)&values;

        const CONSTBUFFER bufferContent = { (const unsigned char*)"34", 2 };

        STRICT_EXPECTED_CALL(ConstMap_GetInternals(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_handle()
            .CopyOutArgumentBuffer(2, &pkeys, sizeof(char**))
            .CopyOutArgumentBuffer(3, &pvalues, sizeof(char**))
            .CopyOutArgumentBuffer(4, &two, sizeof(two))
            .SetReturn(CONSTMAP_ERROR);

        ///act
        const unsigned char* byteArray = Message_ToByteArray(messageHandle, &size);

        ///assert
        ASSERT_IS_NULL(byteArray);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        free((void*)byteArray);
        Message_Destroy(messageHandle);
    }

    /*Tests_SRS_MESSAGE_02_035: [ If any of the above steps fails then Message_ToByteArray shall fail and return NULL. ]*/
    TEST_FUNCTION(Message_ToByteArray_with_properties_and_content_fails_when_malloc_fails)
    {

        ///arrange
        int32_t size;

        STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG))
            .IgnoreArgument_mapFilterFunc()
            .SetReturn(TEST_MAP_HANDLE);
        STRICT_EXPECTED_CALL(Map_Add(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(Map_Add(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreAllCalls();
        STRICT_EXPECTED_CALL(CONSTBUFFER_Create(IGNORED_PTR_ARG, 2))
            .IgnoreArgument_source();
        STRICT_EXPECTED_CALL(ConstMap_Create(TEST_MAP_HANDLE));
        STRICT_EXPECTED_CALL(Map_Destroy(TEST_MAP_HANDLE));

        MESSAGE_HANDLE messageHandle = Message_CreateFromByteArray(notFail__2Property_2bytes, sizeof(notFail__2Property_2bytes));

        size_t two = 2;
        const char* keys[] = { "BleedingEdge", "Azure IoT Gateway is" };
        const char* values[] = { "rocks", "awesome" };
        const char* const* *pkeys = (const char* const* *)&keys;
        const char* const* *pvalues = (const char* const* *)&values;

        const CONSTBUFFER bufferContent = { (const unsigned char*)"34", 2 };

        STRICT_EXPECTED_CALL(ConstMap_GetInternals(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_handle()
            .CopyOutArgumentBuffer(2, &pkeys, sizeof(char**))
            .CopyOutArgumentBuffer(3, &pvalues, sizeof(char**))
            .CopyOutArgumentBuffer(4, &two, sizeof(two));
        STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_PTR_ARG))
            .IgnoreArgument_constbufferHandle()
            .SetReturn(&bufferContent);
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument_size()
            .SetReturn(NULL);

        ///act
        const unsigned char* byteArray = Message_ToByteArray(messageHandle, &size);

        ///assert
        ASSERT_IS_NULL(byteArray);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        free((void*)byteArray);
        Message_Destroy(messageHandle);
    }

END_TEST_SUITE(gwmessage_unittests)
