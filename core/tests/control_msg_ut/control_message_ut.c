// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <stdlib.h>
#include <stddef.h>
#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umocktypes_charptr.h"

#include "control_message.h"

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

static void* my_gballoc_calloc(size_t nmemb, size_t size)
{
    void* result = my_gballoc_malloc(nmemb*size);
	if (result != NULL)
	{
		memset(result, 0, nmemb*size);
	}
    return result;
}


static void my_gballoc_free(void* ptr)
{
    free(ptr);
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

static const unsigned char notFail____minimalMessageCreate[] =
{
    0xA1, 0x6C, 0x01, 1,    /*header, version, type */
    0x00, 0x00, 0x00, 18,   /*size of this array*/
	0x01,				    /*gateway message version*/
	0x00, 0x00, 0x00, 0x00, 0x0, /* type, Size of uri*/
	0x00, 0x00, 0x00, 0x00 /*module args size*/
};

static const unsigned char notFail____minimalMessageCreateReply[] =
{
	0xA1, 0x6C, 0x01, 2,    /*header, version, type */
	0x00, 0x00, 0x00, 9,    /*size of this array*/
	0x00
};
static const unsigned char notFail____minimalMessageStart[] =
{
	0xA1, 0x6C, 0x01, 3,    /*header, version, type */
	0x00, 0x00, 0x00, 8,    /*size of this array*/
};
static const unsigned char notFail____minimalMessageDestroy[] =
{
	0xA1, 0x6C, 0x01, 4,    /*header, version, type */
	0x00, 0x00, 0x00, 8,    /*size of this array*/
};

static const unsigned char notFail__1url_0args[] =
{
	0xA1, 0x6C, 0x01, 1,    /*header, version, type */
	0x00, 0x00, 0x00, 23,   /*size of this array*/
	0x01,				    /*gateway message version*/
	0x00, 0x00, 0x00, 0x00, 0x05, /* type, Size of uri*/
	'm',  's',  'g',  's', '\0', /*uri*/
	0x00, 0x00, 0x00, 0x00, /*module args size*/
};

static const unsigned char notFail__1url_1args[] =
{
	0xA1, 0x6C, 0x01, 1,    /*header, version, type */
	0x00, 0x00, 0x00, 48,   /*size of this array*/
	0x01,				    /*gateway message version*/
	0x00, 0x00, 0x00, 0x00, 0x05, /* type, Size of uri*/
	'm',  's',  'g',  '1', '\0', /*uri*/
	0x00, 0x00, 0x00, 25,   /*module args size*/
	'T','h','i','s',' ','i','s',' ','m','o','d','u','l','e',' ','a','r','g','u','m','e','n','t','s','\0',
};

static const unsigned char fail____headerFirstByteBad[] =
{
	0xA2, 0x6C, 0x01, 3,    /*header, version, type */
	0x00, 0x00, 0x00, 8,    /*size of this array*/
};
static const unsigned char fail____minimalSecondByteBad[] =
{
	0xA1, 0x60, 0x01, 3,    /*header, version, type */
	0x00, 0x00, 0x00, 8,    /*size of this array*/
};
static const unsigned char fail____minimalThirdByteBad[] =
{
	0xA1, 0x6C, 0x11, 3,    /*header, version, type */
	0x00, 0x00, 0x00, 8,    /*size of this array*/
};


TEST_DEFINE_ENUM_TYPE(CONTROL_MESSAGE_TYPE, CONTROL_MESSAGE_TYPE_VALUES)

IMPLEMENT_UMOCK_C_ENUM_TYPE(CONTROL_MESSAGE_TYPE, CONTROL_MESSAGE_TYPE_VALUES)

BEGIN_TEST_SUITE(control_message_ut)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

        umock_c_init(on_umock_c_error);

    int result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_calloc, my_gballoc_calloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

	REGISTER_TYPE(CONTROL_MESSAGE_TYPE, CONTROL_MESSAGE_TYPE);

    REGISTER_UMOCK_ALIAS_TYPE(const unsigned char*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const char* const*, void*);
	REGISTER_UMOCK_ALIAS_TYPE(const char* const* *, void*);
	//

}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    TEST_MUTEX_DESTROY(g_testByTest);
    umock_c_deinit();
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
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

/*Tests_SRS_CONTROL_MESSAGE_17_001: [ If source is NULL, then this function shall return NULL. ]*/
TEST_FUNCTION(ControlMessage_CreateFromByteArray_with_NULL_source_fails)
{
    ///arrange

    ///act
    CONTROL_MESSAGE * r = ControlMessage_CreateFromByteArray(NULL, 15);

    ///assert
    ASSERT_IS_NULL(r);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

/*Tests_SRS_CONTROL_MESSAGE_17_002: [ If size is smaller than 8, then this function shall return NULL. ]*/
TEST_FUNCTION(ControlMessage_CreateFromByteArray_with_source_size_too_small_fails)
{
	///arrange

	///act
	CONTROL_MESSAGE * r = ControlMessage_CreateFromByteArray(notFail____minimalMessageCreate, 7);

	///assert
	ASSERT_IS_NULL(r);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///cleanup
}

/*Tests_SRS_CONTROL_MESSAGE_17_003: [ If the first two bytes of source are not 0xA1 0x6C then this function shall fail and return NULL. ]*/
/*Tests_SRS_CONTROL_MESSAGE_17_004: [ If the version is not equal to CONTROL_MESSAGE_VERSION_CURRENT, then this function shall return NULL. ]*/
TEST_FUNCTION(ControlMessage_CreateFromByteArray_with_header_fail)
{
	///arrange

	///act
	CONTROL_MESSAGE * r1 = ControlMessage_CreateFromByteArray(fail____headerFirstByteBad, 8);
	CONTROL_MESSAGE * r2 = ControlMessage_CreateFromByteArray(fail____minimalSecondByteBad, 8);
	CONTROL_MESSAGE * r3 = ControlMessage_CreateFromByteArray(fail____minimalThirdByteBad, 8);

	///assert
	ASSERT_IS_NULL(r1);
	ASSERT_IS_NULL(r2);
	ASSERT_IS_NULL(r3);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///cleanup
}

/*Tests_SRS_CONTROL_MESSAGE_17_005: [ This function shall read the version, type and size from the byte stream. ]*/
/*Tests_SRS_CONTROL_MESSAGE_17_008: [ This function shall allocate a CONTROL_MESSAGE_MODULE_CREATE structure. ]*/
/*Tests_SRS_CONTROL_MESSAGE_17_014: [ This function shall read the args_size and args from the byte stream. ]*/
/*Tests_SRS_CONTROL_MESSAGE_17_015: [ This function shall allocate args_size bytes for the args array. ]*/
/*Tests_SRS_CONTROL_MESSAGE_17_019: [ This function shall allocate a CONTROL_MESSAGE_MODULE_REPLY structure. ]*/
/*Tests_SRS_CONTROL_MESSAGE_17_021: [ This function shall read the status from the byte stream. ]*/
/*Tests_SRS_CONTROL_MESSAGE_17_023: [ This function shall allocate a CONTROL_MESSAGE structure. ]*/
/*Tests_SRS_CONTROL_MESSAGE_17_024: [ Upon valid reading of the byte stream, this function shall assign the message version and type into the CONTROL_MESSAGE base structure. ]*/
/*Tests_SRS_CONTROL_MESSAGE_17_025: [ Upon success, this function shall return a valid pointer to the CONTROL_MESSAGE base. ]*/
/*Tests_SRS_CONTROL_MESSAGE_17_037: [ This function shall read the gateway_message_version. ]*/
TEST_FUNCTION(ControlMessage_CreateFromByteArray_success)
{
	///arrange
	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(CONTROL_MESSAGE_MODULE_CREATE)));
	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(CONTROL_MESSAGE_MODULE_REPLY)));
	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(CONTROL_MESSAGE)));
	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(CONTROL_MESSAGE)));


	///act
	CONTROL_MESSAGE * r1 = ControlMessage_CreateFromByteArray(notFail____minimalMessageCreate, sizeof(notFail____minimalMessageCreate));
	CONTROL_MESSAGE * r2 = ControlMessage_CreateFromByteArray(notFail____minimalMessageCreateReply, sizeof(notFail____minimalMessageCreateReply));
	CONTROL_MESSAGE * r3 = ControlMessage_CreateFromByteArray(notFail____minimalMessageStart, sizeof(notFail____minimalMessageStart));
	CONTROL_MESSAGE * r4 = ControlMessage_CreateFromByteArray(notFail____minimalMessageDestroy, sizeof(notFail____minimalMessageDestroy));

	CONTROL_MESSAGE_MODULE_CREATE* rc = (CONTROL_MESSAGE_MODULE_CREATE*)r1;
	CONTROL_MESSAGE_MODULE_REPLY * rcr = (CONTROL_MESSAGE_MODULE_REPLY*)r2;

	///assert
	ASSERT_IS_NOT_NULL(r1);
	ASSERT_IS_NOT_NULL(r2);
	ASSERT_IS_NOT_NULL(r3);
	ASSERT_IS_NOT_NULL(r4);
	ASSERT_ARE_EQUAL(CONTROL_MESSAGE_TYPE, r1->type, CONTROL_MESSAGE_TYPE_MODULE_CREATE);
	ASSERT_ARE_EQUAL(CONTROL_MESSAGE_TYPE, r2->type, CONTROL_MESSAGE_TYPE_MODULE_REPLY);
	ASSERT_ARE_EQUAL(CONTROL_MESSAGE_TYPE, r3->type, CONTROL_MESSAGE_TYPE_MODULE_START);
	ASSERT_ARE_EQUAL(CONTROL_MESSAGE_TYPE, r4->type, CONTROL_MESSAGE_TYPE_MODULE_DESTROY);
	ASSERT_ARE_EQUAL(uint8_t, r1->version, 0x01);
	ASSERT_ARE_EQUAL(uint8_t, r2->version, 0x01);
	ASSERT_ARE_EQUAL(uint8_t, r3->version, 0x01);
	ASSERT_ARE_EQUAL(uint8_t, r4->version, 0x01);
	ASSERT_ARE_EQUAL(uint8_t, rc->gateway_message_version, 0x01);
	ASSERT_ARE_EQUAL(int32_t, rc->args_size, 0);
	ASSERT_IS_NULL(rc->args);
	ASSERT_IS_NULL(rc->uri.uri);
	ASSERT_ARE_EQUAL(uint8_t, rcr->status, 0);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///cleanup
	ControlMessage_Destroy(r1);
	ControlMessage_Destroy(r2);
	ControlMessage_Destroy(r3);
	ControlMessage_Destroy(r4);
}

/*Tests_SRS_CONTROL_MESSAGE_17_012: [ This function shall read the uri_type, uri_size, and the uri. ]*/
/*Tests_SRS_CONTROL_MESSAGE_17_013: [ This function shall allocate uri_size bytes for the uri. ]*/
TEST_FUNCTION(ControlMessage_CreateFromByteArray_1url_success)
{
	///arrange
	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(CONTROL_MESSAGE_MODULE_CREATE)));
	STRICT_EXPECTED_CALL(gballoc_malloc(5));


	///act
	CONTROL_MESSAGE * r1 = ControlMessage_CreateFromByteArray(notFail__1url_0args, sizeof(notFail__1url_0args));

	CONTROL_MESSAGE_MODULE_CREATE* rc = (CONTROL_MESSAGE_MODULE_CREATE*)r1;

	///assert
	ASSERT_IS_NOT_NULL(r1);
	ASSERT_ARE_EQUAL(CONTROL_MESSAGE_TYPE, r1->type, CONTROL_MESSAGE_TYPE_MODULE_CREATE);
	ASSERT_ARE_EQUAL(uint8_t, r1->version, 0x01);
	ASSERT_ARE_EQUAL(int32_t, rc->args_size, 0);
	ASSERT_IS_NULL(rc->args);
	ASSERT_IS_NOT_NULL(rc->uri.uri);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///cleanup
	ControlMessage_Destroy(r1);
}

/*Tests_SRS_CONTROL_MESSAGE_17_025: [ Upon success, this function shall return a valid pointer to the CONTROL_MESSAGE base. ]*/
/*Tests_SRS_CONTROL_MESSAGE_17_033: [ This function shall populate the memory with values as indicated in control messages in out process modules. ]*/
TEST_FUNCTION(ControlMessage_CreateFromByteArray_roundabout_success)
{
	///arrange
	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(CONTROL_MESSAGE_MODULE_CREATE)));
	STRICT_EXPECTED_CALL(gballoc_malloc(5));
	STRICT_EXPECTED_CALL(gballoc_malloc(25));
	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(notFail__1url_1args)));

	///act
	CONTROL_MESSAGE * r1 = ControlMessage_CreateFromByteArray(notFail__1url_1args, sizeof(notFail__1url_1args));
	int r2 = ControlMessage_ToByteArray(r1, NULL, 0);
	unsigned char * serialized_again = (unsigned char *)malloc(r2);
	ASSERT_IS_NOT_NULL(serialized_again);
	int32_t r3 = ControlMessage_ToByteArray(r1, serialized_again, r2);
	
	///assert
	ASSERT_IS_NOT_NULL(r1);
	ASSERT_ARE_EQUAL(int, 0, memcmp(notFail__1url_1args, serialized_again, sizeof(notFail__1url_1args)));
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///cleanup
	ControlMessage_Destroy(r1);
	free(serialized_again);
}

/*Tests_SRS_CONTROL_MESSAGE_17_006: [ If the size embedded in the message is not the same as size parameter then this function shall fail and return NULL. ]*/
TEST_FUNCTION(ControlMessage_CreateFromByteArray_incorrect_size_fails)
{
	///arrange

	///act
	CONTROL_MESSAGE * r1 = ControlMessage_CreateFromByteArray(notFail__1url_1args, sizeof(notFail__1url_1args)-1);

	CONTROL_MESSAGE_MODULE_CREATE* rc = (CONTROL_MESSAGE_MODULE_CREATE*)r1;

	///assert
	ASSERT_IS_NULL(r1);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///cleanup
}

/*Tests_SRS_CONTROL_MESSAGE_17_033: [ This function shall populate the memory with values as indicated in control messages in out process modules. ]*/
/*Tests_SRS_CONTROL_MESSAGE_17_018: [ Reading past the end of the byte array shall cause this function to fail and return NULL. ]*/
/*Tests_SRS_CONTROL_MESSAGE_17_022: [ This function shall release all allocated memory upon failure. ]*/
TEST_FUNCTION(ControlMessage_CreateFromByteArray_args_way_too_long)
{
	///arrange
	static const unsigned char fail__1url_1args[] =
	{
		0xA1, 0x6C, 0x01, 1,    /*header, version, type */
		0x00, 0x00, 0x00, 48,   /*size of this array*/
		0x01,				    /*gateway message version*/
		0x00, 0x00, 0x00, 0x00, 0x05, /* type, Size of uri*/
		'm',  's',  'g',  's', '\0', /*uri*/
		0x00, 0x00, 0x00, 60,   /*module args size - wrong*/
		'T','h','i','s',' ','i','s',' ','m','o','d','u','l','e',' ','a','r','g','u','m','e','n','t','s','\0'
	};
	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(CONTROL_MESSAGE_MODULE_CREATE)));
	STRICT_EXPECTED_CALL(gballoc_malloc(5));
	EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
	EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

	///act
	CONTROL_MESSAGE * r1 = ControlMessage_CreateFromByteArray(fail__1url_1args, sizeof(fail__1url_1args));

	CONTROL_MESSAGE_MODULE_CREATE* rc = (CONTROL_MESSAGE_MODULE_CREATE*)r1;

	///assert
	ASSERT_IS_NULL(r1);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///cleanup
}

/*Tests_SRS_CONTROL_MESSAGE_17_036: [ This function shall return NULL upon failure. ]*/
/*Tests_SRS_CONTROL_MESSAGE_17_022: [ This function shall release all allocated memory upon failure. ]*/
TEST_FUNCTION(ControlMessage_CreateFromByteArray_args_malloc_fail)
{
	///arrange

	whenShallmalloc_fail = 3;
	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(CONTROL_MESSAGE_MODULE_CREATE)));
	STRICT_EXPECTED_CALL(gballoc_malloc(5));
	STRICT_EXPECTED_CALL(gballoc_malloc(25));
	EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
	EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

	///act
	CONTROL_MESSAGE * r1 = ControlMessage_CreateFromByteArray(notFail__1url_1args, sizeof(notFail__1url_1args));

	CONTROL_MESSAGE_MODULE_CREATE* rc = (CONTROL_MESSAGE_MODULE_CREATE*)r1;

	///assert
	ASSERT_IS_NULL(r1);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///cleanup
}

/*Tests_SRS_CONTROL_MESSAGE_17_033: [ This function shall populate the memory with values as indicated in control messages in out process modules. ]*/
/*Tests_SRS_CONTROL_MESSAGE_17_018: [ Reading past the end of the byte array shall cause this function to fail and return NULL. ]*/
TEST_FUNCTION(ControlMessage_CreateFromByteArray_url_way_too_long)
{
	///arrange
	static const unsigned char fail__1url_1args[] =
	{
		0xA1, 0x6C, 0x01, 1,    /*header, version, type */
		0x00, 0x00, 0x00, 48,   /*size of this array*/
		0x01,				    /*gateway message version*/
		0x00, 0x00, 0x00, 0x00, 0x61, /* type, Size of uri*/
		'm',  's',  'g',  's', '\0', /*uri*/
		0x00, 0x00, 0x00, 24,   /*module args size - wrong*/
		'T','h','i','s',' ','i','s',' ','m','o','d','u','l','e',' ','a','r','g','u','m','e','n','t','s','\0'
	};
	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(CONTROL_MESSAGE_MODULE_CREATE)));
	EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

	///act
	CONTROL_MESSAGE * r1 = ControlMessage_CreateFromByteArray(fail__1url_1args, sizeof(fail__1url_1args));

	CONTROL_MESSAGE_MODULE_CREATE* rc = (CONTROL_MESSAGE_MODULE_CREATE*)r1;

	///assert
	ASSERT_IS_NULL(r1);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///cleanup
}


/*Tests_SRS_CONTROL_MESSAGE_17_036: [ This function shall return NULL upon failure. ]*/
/*Tests_SRS_CONTROL_MESSAGE_17_022: [ This function shall release all allocated memory upon failure. ]*/
TEST_FUNCTION(ControlMessage_CreateFromByteArray_url_malloc_fail)
{
	///arrange

	whenShallmalloc_fail = 2;
	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(CONTROL_MESSAGE_MODULE_CREATE)));
	STRICT_EXPECTED_CALL(gballoc_malloc(5));
	EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

	///act
	CONTROL_MESSAGE * r1 = ControlMessage_CreateFromByteArray(notFail__1url_1args, sizeof(notFail__1url_1args));

	CONTROL_MESSAGE_MODULE_CREATE* rc = (CONTROL_MESSAGE_MODULE_CREATE*)r1;

	///assert
	ASSERT_IS_NULL(r1);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///cleanup
}

/*Tests_SRS_CONTROL_MESSAGE_17_009: [ If the total message size is not at least 16 bytes, then this function shall return NULL ]*/
TEST_FUNCTION(ControlMessage_CreateFromByteArray_create_struct_size_too_small)
{
	///arrange
	static const unsigned char fail____minimalMessageCreate[] =
	{
		0xA1, 0x6C, 0x01, 1,    /*header, version, type */
		0x00, 0x00, 0x00, 16,   /*size of this array*/
		0x01,				    /*gateway message version*/
		0x00, 0x00, 0x00, 0x00, 0x0, /* type, Size of uri*/
		0x00, 0x00, 0x00, 0x00 /*module args size*/
	};
	///act
	CONTROL_MESSAGE * m1 = ControlMessage_CreateFromByteArray(fail____minimalMessageCreate, 16);

	///assert
	ASSERT_IS_NULL(m1);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///cleanup
}

/*Tests_SRS_CONTROL_MESSAGE_17_020: [ If the total message size is not at least 9 bytes, then this function shall fail and return NULL. ]*/
TEST_FUNCTION(ControlMessage_CreateFromByteArray_reply_struct_size_too_small)
{
	///arrange
	static const unsigned char notFail____minimalMessageCreateReply[] =
	{
		0xA1, 0x6C, 0x01, 2,    /*header, version, type */
		0x00, 0x00, 0x00, 8,    /*size of this array*/
		0x00
	};
	///act
	CONTROL_MESSAGE * r1 = ControlMessage_CreateFromByteArray(notFail____minimalMessageCreateReply, 8);

	///assert
	ASSERT_IS_NULL(r1);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///cleanup
}

/*Tests_SRS_CONTROL_MESSAGE_17_036: [ This function shall return NULL upon failure. ]*/
TEST_FUNCTION(ControlMessage_CreateFromByteArray_reply_struct_malloc_fail)
{
	///arrange

	whenShallmalloc_fail = 1;
	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(CONTROL_MESSAGE_MODULE_REPLY)));

	///act
	CONTROL_MESSAGE * r1 = ControlMessage_CreateFromByteArray(notFail____minimalMessageCreateReply, sizeof(notFail____minimalMessageCreateReply));

	///assert
	ASSERT_IS_NULL(r1);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///cleanup
}

/*Tests_SRS_CONTROL_MESSAGE_17_036: [ This function shall return NULL upon failure. ]*/
TEST_FUNCTION(ControlMessage_CreateFromByteArray_destroy_struct_malloc_fail)
{
	///arrange

	whenShallmalloc_fail = 1;
	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(CONTROL_MESSAGE)));

	///act
	CONTROL_MESSAGE * r1 = ControlMessage_CreateFromByteArray(notFail____minimalMessageDestroy, sizeof(notFail____minimalMessageDestroy));

	///assert
	ASSERT_IS_NULL(r1);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///cleanup
}

/*Tests_SRS_CONTROL_MESSAGE_17_036: [ This function shall return NULL upon failure. ]*/
TEST_FUNCTION(ControlMessage_CreateFromByteArray_start_struct_malloc_fail)
{
	///arrange

	whenShallmalloc_fail = 1;
	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(CONTROL_MESSAGE)));

	///act
	CONTROL_MESSAGE * r1 = ControlMessage_CreateFromByteArray(notFail____minimalMessageStart, sizeof(notFail____minimalMessageStart));

	///assert
	ASSERT_IS_NULL(r1);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///cleanup
}

/*Tests_SRS_CONTROL_MESSAGE_17_007: [ This function shall return NULL if the type is not a valid enum value of CONTROL_MESSAGE_TYPE or CONTROL_MESSAGE_TYPE_ERROR. ]*/
TEST_FUNCTION(ControlMessage_CreateFromByteArray_bad_msg_type_fails)
{
	///arrange
	static const unsigned char notFail____minimalMessageError[] =
	{
		0xA1, 0x6C, 0x01, 0,    /*header, version, type */
		0x00, 0x00, 0x00, 8,    /*size of this array*/
	};

	///act
	CONTROL_MESSAGE * r1 = ControlMessage_CreateFromByteArray(notFail____minimalMessageError, sizeof(notFail____minimalMessageError));

	///assert
	ASSERT_IS_NULL(r1);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///cleanup
}

/*Tests_SRS_CONTROL_MESSAGE_17_026: [ If message is NULL this function shall do nothing. ]*/
TEST_FUNCTION(ControlMessage_Destroy_does_nothing_with_nothing)
{
	///arrange
	///act
	ControlMessage_Destroy(NULL);
	///assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///cleanup
}

/*Tests_SRS_CONTROL_MESSAGE_17_030: [ This function shall release message for all message types. ]*/
TEST_FUNCTION(ControlMessage_Destroy_does_something_with_something)
{
	///arrange
	CONTROL_MESSAGE * r1 = ControlMessage_CreateFromByteArray(notFail____minimalMessageStart, sizeof(notFail____minimalMessageStart));
	umock_c_reset_all_calls();
	EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

	///act
	ControlMessage_Destroy(r1);
	///assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///cleanup
}

/*Tests_SRS_CONTROL_MESSAGE_17_027: [ This function shall release all memory allocated in this structure. ]*/
/*Tests_SRS_CONTROL_MESSAGE_17_028: [ For a CONTROL_MESSAGE_MODULE_CREATE message type, all memory allocated shall be defined as all the memory allocated by ControlMessage_CreateFromByteArray. ]*/
TEST_FUNCTION(ControlMessage_Destroy_does_more_with_create_msgs)
{
	///arrange
	CONTROL_MESSAGE * r1 = ControlMessage_CreateFromByteArray(notFail__1url_1args, sizeof(notFail__1url_1args));
	umock_c_reset_all_calls();
	EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
	EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
	EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

	///act
	ControlMessage_Destroy(r1);
	///assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///cleanup
}

/*Tests_SRS_CONTROL_MESSAGE_17_031: [ If message is NULL, then this function shall return -1. ]*/
TEST_FUNCTION(ControlMessage_ToByteArray_returns_null_given_null_message)
{
	///arrange
	///act
	int32_t r1 = ControlMessage_ToByteArray(NULL,NULL,0);
	///assert
	ASSERT_ARE_EQUAL(int32_t, r1, -1);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///cleanup
}

/*Tests_SRS_CONTROL_MESSAGE_17_032: [ If buf is NULL, but size is not zero, then this function shall return -1. ]*/
TEST_FUNCTION(ControlMessage_ToByteArray_returns_null_with_size_buffer_mismatch)
{
	///arrange
	CONTROL_MESSAGE * msg = ControlMessage_CreateFromByteArray(notFail__1url_1args, sizeof(notFail__1url_1args));
	umock_c_reset_all_calls();

	///act
	int32_t r1 = ControlMessage_ToByteArray(msg, NULL, sizeof(notFail__1url_1args));
	///assert
	ASSERT_ARE_EQUAL(int32_t, r1, -1);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///cleanup
	ControlMessage_Destroy(msg);

}

/*Tests_SRS_CONTROL_MESSAGE_17_033: [ This function shall populate the memory with values as indicated in control messages in out process modules. ]*/
/*Tests_SRS_CONTROL_MESSAGE_17_035: [ Upon success this function shall return the byte array size.*/
TEST_FUNCTION(ControlMessage_ToByteArray_counts_correctly)
{
	///arrange
	int32_t m1_size = sizeof(notFail____minimalMessageCreate);
	int32_t m2_size = sizeof(notFail____minimalMessageCreateReply);
	int32_t m3_size = sizeof(notFail____minimalMessageStart);
	int32_t m4_size = sizeof(notFail____minimalMessageDestroy);
	int32_t m5_size = sizeof(notFail__1url_0args);
	int32_t m8_size = sizeof(notFail__1url_1args);

	CONTROL_MESSAGE * m1 = ControlMessage_CreateFromByteArray(notFail____minimalMessageCreate, m1_size);
	CONTROL_MESSAGE * m2 = ControlMessage_CreateFromByteArray(notFail____minimalMessageCreateReply, m2_size);
	CONTROL_MESSAGE * m3 = ControlMessage_CreateFromByteArray(notFail____minimalMessageStart, m3_size);
	CONTROL_MESSAGE * m4 = ControlMessage_CreateFromByteArray(notFail____minimalMessageDestroy, m4_size);
	CONTROL_MESSAGE * m5 = ControlMessage_CreateFromByteArray(notFail__1url_0args, m5_size);
	CONTROL_MESSAGE * m8 = ControlMessage_CreateFromByteArray(notFail__1url_1args, m8_size);
	umock_c_reset_all_calls();

	///act
	int32_t c1 = ControlMessage_ToByteArray(m1, NULL, 0);
	int32_t c2 = ControlMessage_ToByteArray(m2, NULL, 0);
	int32_t c3 = ControlMessage_ToByteArray(m3, NULL, 0);
	int32_t c4 = ControlMessage_ToByteArray(m4, NULL, 0);
	int32_t c5 = ControlMessage_ToByteArray(m5, NULL, 0);
	int32_t c8 = ControlMessage_ToByteArray(m8, NULL, 0);

	///assert

	ASSERT_ARE_EQUAL(int32_t, c1, m1_size);
	ASSERT_ARE_EQUAL(int32_t, c2, m2_size);
	ASSERT_ARE_EQUAL(int32_t, c3, m3_size);
	ASSERT_ARE_EQUAL(int32_t, c4, m4_size);
	ASSERT_ARE_EQUAL(int32_t, c5, m5_size);
	ASSERT_ARE_EQUAL(int32_t, c8, m8_size);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///cleanup
	ControlMessage_Destroy(m1);
	ControlMessage_Destroy(m2);
	ControlMessage_Destroy(m3);
	ControlMessage_Destroy(m4);
	ControlMessage_Destroy(m5);
	ControlMessage_Destroy(m8);
}

/*Tests_SRS_CONTROL_MESSAGE_17_034: [ If any of the above steps fails then this function shall fail and return -1. ]*/
TEST_FUNCTION(ControlMessage_ToByteArray_error_message_fails)
{
	///arrange
	CONTROL_MESSAGE m1 = {
		0x01,
		CONTROL_MESSAGE_TYPE_ERROR
	};
	///act
	int32_t c1 = ControlMessage_ToByteArray(&m1, NULL, 0);
	///assert
	ASSERT_ARE_EQUAL(int32_t, c1, -1);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///cleanup
}

/*Tests_SRS_CONTROL_MESSAGE_17_034: [ If any of the above steps fails then this function shall fail and return -1. ]*/
TEST_FUNCTION(ControlMessage_ToByteArray_buffer_too_short)
{
	///arrange
	CONTROL_MESSAGE m1 = {
		0x01,
		CONTROL_MESSAGE_TYPE_MODULE_START
	};
	unsigned char buf[7];

	///act
	int32_t c1 = ControlMessage_ToByteArray(&m1, buf, 7);
	///assert
	ASSERT_ARE_EQUAL(int32_t, c1, -1);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///cleanup
}

/*Tests_SRS_CONTROL_MESSAGE_17_033: [ This function shall populate the memory with values as indicated in control messages in out process modules. ]*/
/*Tests_SRS_CONTROL_MESSAGE_17_035: [ Upon success this function shall return the byte array size. ]*/
TEST_FUNCTION(ControlMessage_ToByteArray_create_reply_correct)
{
	///arrange
	CONTROL_MESSAGE_MODULE_REPLY m1 =
	{
		{
			0x01,
			CONTROL_MESSAGE_TYPE_MODULE_REPLY
		},
		1
	};
	unsigned char buf[9];

	///act
	int32_t c1 = ControlMessage_ToByteArray((CONTROL_MESSAGE*)&m1, buf, 9);
	///assert
	ASSERT_ARE_EQUAL(int32_t, c1, 9);
	ASSERT_ARE_EQUAL(uint8_t, buf[8], 1);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///cleanup
}

END_TEST_SUITE(control_message_ut)
