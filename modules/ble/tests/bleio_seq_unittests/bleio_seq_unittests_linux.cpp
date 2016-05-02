// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"

#include "bluez_device.h"
#include "bluez_characteristic.h"
#include "gio_async_seq.h"
#include "ble_gatt_io.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/strings.h"
#include "bleio_seq.h"

static MICROMOCK_MUTEX_HANDLE g_testByTest;
static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

#define GBALLOC_H

extern "C" int gballoc_init(void);
extern "C" void gballoc_deinit(void);
extern "C" void* gballoc_malloc(size_t size);
extern "C" void* gballoc_calloc(size_t nmemb, size_t size);
extern "C" void* gballoc_realloc(void* ptr, size_t size);
extern "C" void gballoc_free(void* ptr);

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

#include "../../src/gio_async_seq.c"
#include "vector.c"
#include "buffer.c"
#include "strings.c"
};

DEFINE_MICROMOCK_ENUM_TO_STRING(BLEIO_SEQ_RESULT, BLEIO_SEQ_RESULT_VALUES);

struct
{
    BLEIO_GATT_RESULT result;
    const unsigned char* buffer;
    size_t size;
} BLEIO_gatt_read_char_by_uuid_results;

struct
{
    BLEIO_GATT_RESULT result;
} BLEIO_gatt_write_char_by_uuid_results;

gboolean g_expected_timer_return_value = TRUE;
GSourceFunc g_timer_callback = NULL;
gpointer g_timer_data = NULL;

TYPED_MOCK_CLASS(CBLEIOSeqMocks, CGlobalMock)
{
public:

    // memory
    MOCK_STATIC_METHOD_1(, void*, gballoc_malloc, size_t, size)
        void* result2 = BASEIMPLEMENTATION::gballoc_malloc(size);
    MOCK_METHOD_END(void*, result2);

    MOCK_STATIC_METHOD_1(, void, gballoc_free, void*, ptr)
        BASEIMPLEMENTATION::gballoc_free(ptr);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_5(, void, on_write_complete, BLEIO_SEQ_HANDLE, bleio_seq_handle, void*, context, const char*, characteristic_uuid, BLEIO_SEQ_INSTRUCTION_TYPE, type, BLEIO_SEQ_RESULT, result2)
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_6(, void, on_read_complete, BLEIO_SEQ_HANDLE, bleio_seq_handle, void*, context, const char*, characteristic_uuid, BLEIO_SEQ_INSTRUCTION_TYPE, type, BLEIO_SEQ_RESULT, result2, BUFFER_HANDLE, data)
        if (data != NULL)
        {
            BUFFER_delete(data);
        }
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_2(, void, on_destroy_complete, BLEIO_SEQ_HANDLE, bleio_seq_handle, void*, context)
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, VECTOR_HANDLE, VECTOR_create, size_t, elementSize)
        auto result2 = BASEIMPLEMENTATION::VECTOR_create(elementSize);
    MOCK_METHOD_END(VECTOR_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, size_t, VECTOR_size, VECTOR_HANDLE, vector)
        size_t result2 = BASEIMPLEMENTATION::VECTOR_size(vector);
    MOCK_METHOD_END(size_t, result2)

    MOCK_STATIC_METHOD_3(, int, VECTOR_push_back, VECTOR_HANDLE, handle, const void*, elements, size_t, numElements)
        auto result2 = BASEIMPLEMENTATION::VECTOR_push_back(handle, elements, numElements);
    MOCK_METHOD_END(int, result2)

    MOCK_STATIC_METHOD_2(, void*, VECTOR_element, VECTOR_HANDLE, vector, size_t, index)
        void* result2 = BASEIMPLEMENTATION::VECTOR_element(vector, index);
    MOCK_METHOD_END(void*, result2)

    MOCK_STATIC_METHOD_1(, void, VECTOR_destroy, VECTOR_HANDLE, vector)
        BASEIMPLEMENTATION::VECTOR_destroy(vector);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, size_t, STRING_length, STRING_HANDLE, handle)
        auto result2 = BASEIMPLEMENTATION::STRING_length(handle);
    MOCK_METHOD_END(size_t, result2)

    MOCK_STATIC_METHOD_1(, void, STRING_delete, STRING_HANDLE, handle)
        BASEIMPLEMENTATION::STRING_delete(handle);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, const char*, STRING_c_str, STRING_HANDLE, handle)
        auto result2 = BASEIMPLEMENTATION::STRING_c_str(handle);
    MOCK_METHOD_END(const char*, result2)

    MOCK_STATIC_METHOD_1(, STRING_HANDLE, STRING_construct, const char*, psz)
        auto result2 = BASEIMPLEMENTATION::STRING_construct(psz);
    MOCK_METHOD_END(STRING_HANDLE, result2)

    MOCK_STATIC_METHOD_2(, BUFFER_HANDLE, BUFFER_create, const unsigned char*, source, size_t, size)
        BUFFER_HANDLE result2 = BASEIMPLEMENTATION::BUFFER_create(source, size);
    MOCK_METHOD_END(BUFFER_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, void, BUFFER_delete, BUFFER_HANDLE, handle)
        BASEIMPLEMENTATION::BUFFER_delete(handle);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, unsigned char*, BUFFER_u_char, BUFFER_HANDLE, handle)
        auto result2 = BASEIMPLEMENTATION::BUFFER_u_char(handle);
    MOCK_METHOD_END(unsigned char*, result2)

    MOCK_STATIC_METHOD_1(, size_t, BUFFER_length, BUFFER_HANDLE, handle)
        auto result2 = BASEIMPLEMENTATION::BUFFER_length(handle);
    MOCK_METHOD_END(size_t, result2)

    MOCK_STATIC_METHOD_4(, int, BLEIO_gatt_read_char_by_uuid, BLEIO_GATT_HANDLE, bleio_gatt_handle, const char*, ble_uuid, ON_BLEIO_GATT_ATTRIB_READ_COMPLETE, on_bleio_gatt_attrib_read_complete, void*, callback_context)
        int result2 = 0;
        on_bleio_gatt_attrib_read_complete(
            bleio_gatt_handle,
            callback_context,
            BLEIO_gatt_read_char_by_uuid_results.result,
            BLEIO_gatt_read_char_by_uuid_results.buffer,
            BLEIO_gatt_read_char_by_uuid_results.size
        );
    MOCK_METHOD_END(int, result2)

    MOCK_STATIC_METHOD_6(, int, BLEIO_gatt_write_char_by_uuid, BLEIO_GATT_HANDLE, bleio_gatt_handle, const char*, ble_uuid, const unsigned char*, buffer, size_t, size, ON_BLEIO_GATT_ATTRIB_WRITE_COMPLETE, on_bleio_gatt_attrib_write_complete, void*, callback_context)
        int result2 = 0;
        on_bleio_gatt_attrib_write_complete(
            bleio_gatt_handle,
            callback_context,
            BLEIO_gatt_write_char_by_uuid_results.result
        );
    MOCK_METHOD_END(int, result2)

    MOCK_STATIC_METHOD_1(, void, BLEIO_gatt_destroy, BLEIO_GATT_HANDLE, bleio_gatt_handle)
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_3(, guint, g_timeout_add, guint, interval, GSourceFunc, function, gpointer, data)
        g_timer_callback = function;
        g_timer_data = data;
        auto timer_continue = function(data);
        ASSERT_ARE_EQUAL(int, g_expected_timer_return_value, timer_continue);
        guint result2 = 1;
    MOCK_METHOD_END(guint, result2)
};

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEIOSeqMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEIOSeqMocks, , void, gballoc_free, void*, ptr);

DECLARE_GLOBAL_MOCK_METHOD_5(CBLEIOSeqMocks, , void, on_write_complete, BLEIO_SEQ_HANDLE, bleio_seq_handle, void*, context, const char*, characteristic_uuid, BLEIO_SEQ_INSTRUCTION_TYPE, type, BLEIO_SEQ_RESULT, result);
DECLARE_GLOBAL_MOCK_METHOD_6(CBLEIOSeqMocks, , void, on_read_complete, BLEIO_SEQ_HANDLE, bleio_seq_handle, void*, context, const char*, characteristic_uuid, BLEIO_SEQ_INSTRUCTION_TYPE, type, BLEIO_SEQ_RESULT, result, BUFFER_HANDLE, data);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEIOSeqMocks, , void, on_destroy_complete, BLEIO_SEQ_HANDLE, bleio_seq_handle, void*, context);

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEIOSeqMocks, , VECTOR_HANDLE, VECTOR_create, size_t, elementSize);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEIOSeqMocks, , void, VECTOR_destroy, VECTOR_HANDLE, vector);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEIOSeqMocks, , void*, VECTOR_element, VECTOR_HANDLE, vector, size_t, index);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEIOSeqMocks, , size_t, VECTOR_size, VECTOR_HANDLE, vector);
DECLARE_GLOBAL_MOCK_METHOD_3(CBLEIOSeqMocks, , int, VECTOR_push_back, VECTOR_HANDLE, handle, const void*, elements, size_t, numElements);

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEIOSeqMocks, , STRING_HANDLE, STRING_construct, const char*, psz);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEIOSeqMocks, , size_t, STRING_length, STRING_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEIOSeqMocks, , void, STRING_delete, STRING_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEIOSeqMocks, , const char*, STRING_c_str, STRING_HANDLE, handle);

DECLARE_GLOBAL_MOCK_METHOD_2(CBLEIOSeqMocks, , BUFFER_HANDLE, BUFFER_create, const unsigned char*, source, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEIOSeqMocks, , void, BUFFER_delete, BUFFER_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEIOSeqMocks, , unsigned char*, BUFFER_u_char, BUFFER_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEIOSeqMocks, , size_t, BUFFER_length, BUFFER_HANDLE, handle);

DECLARE_GLOBAL_MOCK_METHOD_4(CBLEIOSeqMocks, , int, BLEIO_gatt_read_char_by_uuid, BLEIO_GATT_HANDLE, bleio_gatt_handle, const char*, ble_uuid, ON_BLEIO_GATT_ATTRIB_READ_COMPLETE, on_bleio_gatt_attrib_read_complete, void*, callback_context);
DECLARE_GLOBAL_MOCK_METHOD_6(CBLEIOSeqMocks, , int, BLEIO_gatt_write_char_by_uuid, BLEIO_GATT_HANDLE, bleio_gatt_handle, const char*, ble_uuid, const unsigned char*, buffer, size_t, size, ON_BLEIO_GATT_ATTRIB_WRITE_COMPLETE, on_bleio_gatt_attrib_write_complete, void*, callback_context);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEIOSeqMocks, , void, BLEIO_gatt_destroy, BLEIO_GATT_HANDLE, bleio_gatt_handle);

DECLARE_GLOBAL_MOCK_METHOD_3(CBLEIOSeqMocks, , guint, g_timeout_add, guint, interval, GSourceFunc, function, gpointer, data);

BEGIN_TEST_SUITE(bleio_seq_unittests)
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

        g_timer_callback = NULL;
        g_timer_data = NULL;
    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
        if (!MicroMockReleaseMutex(g_testByTest))
        {
            ASSERT_FAIL("failure in test framework at ReleaseMutex");
        }
    }

    /*Tests_SRS_BLEIO_SEQ_13_001: [ BLEIO_Seq_Create shall return NULL if bleio_gatt_handle is NULL. ]*/
    TEST_FUNCTION(BLEIO_Seq_Create_returns_NULL_for_NULL_input1)
    {
        ///arrange
        CBLEIOSeqMocks mocks;

        ///act
        auto result = BLEIO_Seq_Create(NULL, (VECTOR_HANDLE)0x42, on_read_complete, on_write_complete);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLEIO_SEQ_13_002: [ BLEIO_Seq_Create shall return NULL if instructions is NULL. ]*/
    TEST_FUNCTION(BLEIO_Seq_Create_returns_NULL_for_NULL_input2)
    {
        ///arrange
        CBLEIOSeqMocks mocks;

        ///act
        auto result = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, NULL, on_read_complete, on_write_complete);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLEIO_SEQ_13_003: [ BLEIO_Seq_Create shall return NULL if the vector instructions is empty. ]*/
    TEST_FUNCTION(BLEIO_Seq_Create_returns_NULL_when_instructions_vector_is_empty)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));

        ///act
        auto result = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
        VECTOR_destroy(instructions);
    }

    /*Tests_SRS_BLEIO_SEQ_13_025: [ BLEIO_Seq_Create shall return NULL if the characteristic_uuid field for any instruction is NULL or empty. ]*/
    TEST_FUNCTION(BLEIO_Seq_Create_returns_NULL_when_an_instruction_has_NULL_charid)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            READ_ONCE,
            NULL, // NULL characteristic ID
            NULL,
            { 0 }
        };
        VECTOR_push_back(instructions, &instr1, 1);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions)); // in BLEIO_Seq_Create
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions)); // in validate_instructions
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));

        ///act
        auto result = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
        VECTOR_destroy(instructions);
    }

    /*Tests_SRS_BLEIO_SEQ_13_025: [ BLEIO_Seq_Create shall return NULL if the characteristic_uuid field for any instruction is NULL or empty. ]*/
    TEST_FUNCTION(BLEIO_Seq_Create_returns_NULL_when_an_instruction_has_empty_charid)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            READ_ONCE,
            STRING_construct(""), // empty characteristic ID
            NULL,
            { 0 }
        };
        VECTOR_push_back(instructions, &instr1, 1);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions)); // in BLEIO_Seq_Create
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions)); // in validate_instructions
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));
        STRICT_EXPECTED_CALL(mocks, STRING_length(instr1.characteristic_uuid));

        ///act
        auto result = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
        VECTOR_destroy(instructions);
        STRING_delete(instr1.characteristic_uuid);
    }

    /*Tests_SRS_BLEIO_SEQ_13_023: [ BLEIO_Seq_Create shall return NULL if a READ_PERIODIC instruction's interval_in_ms field is zero. ]*/
    TEST_FUNCTION(BLEIO_Seq_Create_returns_NULL_when_an_read_periodic_instruction_has_zero_interval)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            READ_PERIODIC,
            STRING_construct("fake_char_id"),
            NULL,
            { 0 } // zero interval
        };
        VECTOR_push_back(instructions, &instr1, 1);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions)); // in BLEIO_Seq_Create
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions)); // in validate_instructions
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));
        STRICT_EXPECTED_CALL(mocks, STRING_length(instr1.characteristic_uuid));

        ///act
        auto result = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
        VECTOR_destroy(instructions);
        STRING_delete(instr1.characteristic_uuid);
    }

    /*Tests_SRS_BLEIO_SEQ_13_024: [ BLEIO_Seq_Create shall return NULL if a WRITE_AT_INIT or a WRITE_AT_EXIT or a WRITE_ONCE instruction has the value zero for the size field or has a NULL value in the buffer field. ]*/
    TEST_FUNCTION(BLEIO_Seq_Create_returns_NULL_when_a_write_init_instruction_has_NULL_buffer)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            WRITE_AT_INIT,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = NULL } // NULL buffer
        };
        VECTOR_push_back(instructions, &instr1, 1);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions)); // in BLEIO_Seq_Create
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions)); // in validate_instructions
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));
        STRICT_EXPECTED_CALL(mocks, STRING_length(instr1.characteristic_uuid));

        ///act
        auto result = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
        VECTOR_destroy(instructions);
        STRING_delete(instr1.characteristic_uuid);
    }

    /*Tests_SRS_BLEIO_SEQ_13_024: [ BLEIO_Seq_Create shall return NULL if a WRITE_AT_INIT or a WRITE_AT_EXIT or a WRITE_ONCE instruction has the value zero for the size field or has a NULL value in the buffer field. ]*/
    TEST_FUNCTION(BLEIO_Seq_Create_returns_NULL_when_a_write_at_exit_instruction_has_NULL_buffer)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            WRITE_AT_EXIT,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = NULL } // NULL buffer
        };
        VECTOR_push_back(instructions, &instr1, 1);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions)); // in BLEIO_Seq_Create
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions)); // in validate_instructions
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));
        STRICT_EXPECTED_CALL(mocks, STRING_length(instr1.characteristic_uuid));

        ///act
        auto result = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
        VECTOR_destroy(instructions);
        STRING_delete(instr1.characteristic_uuid);
    }

    /*Tests_SRS_BLEIO_SEQ_13_024: [ BLEIO_Seq_Create shall return NULL if a WRITE_AT_INIT or a WRITE_AT_EXIT or a WRITE_ONCE instruction has the value zero for the size field or has a NULL value in the buffer field. ]*/
    TEST_FUNCTION(BLEIO_Seq_Create_returns_NULL_when_a_write_once_instruction_has_NULL_buffer)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = NULL } // NULL buffer
        };
        VECTOR_push_back(instructions, &instr1, 1);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions)); // in BLEIO_Seq_Create
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions)); // in validate_instructions
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));
        STRICT_EXPECTED_CALL(mocks, STRING_length(instr1.characteristic_uuid));

        ///act
        auto result = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
        VECTOR_destroy(instructions);
        STRING_delete(instr1.characteristic_uuid);
    }

    /*Tests_SRS_BLEIO_SEQ_13_004: [ BLEIO_Seq_Create shall return NULL if any of the underlying platform calls fail. ]*/
    TEST_FUNCTION(BLEIO_Seq_Create_returns_NULL_when_malloc_fails)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            WRITE_AT_EXIT,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instr1, 1);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((void*)NULL);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions)); // in BLEIO_Seq_Create
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions)); // in validate_instructions
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));
        STRICT_EXPECTED_CALL(mocks, STRING_length(instr1.characteristic_uuid));

        ///act
        auto result = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
        VECTOR_destroy(instructions);
        BUFFER_delete(instr1.data.buffer);
        STRING_delete(instr1.characteristic_uuid);
    }

    /*Tests_SRS_BLEIO_SEQ_13_005: [ BLEIO_Seq_Create shall return a non-NULL handle on successful execution. ]*/
    TEST_FUNCTION(BLEIO_Seq_Create_succeeds)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            WRITE_AT_INIT,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instr1, 1);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions)); // in BLEIO_Seq_Create
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions)); // in validate_instructions
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));
        STRICT_EXPECTED_CALL(mocks, STRING_length(instr1.characteristic_uuid));

        ///act
        auto result = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NOT_NULL(result);

        ///cleanup
        BLEIO_Seq_Destroy(result, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_010: [BLEIO_Seq_Run shall return BLEIO_SEQ_ERROR if bleio_seq_handle is NULL.]*/
    TEST_FUNCTION(BLEIO_Seq_Run_returns_error_for_NULL_input)
    {
        ///arrange
        CBLEIOSeqMocks mocks;

        ///act
        auto result = BLEIO_Seq_Run(NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
    }

    /*Tests_SRS_BLEIO_SEQ_13_013: [ BLEIO_Seq_Run shall return BLEIO_SEQ_ERROR if BLEIO_Seq_Run was previously called on this handle. ]*/
    TEST_FUNCTION(BLEIO_Seq_Run_returns_error_when_state_is_not_idle)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            WRITE_AT_INIT,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instr1, 1);

        // cause BLEIO_gatt_write_char_by_uuid to call callback with ok
        BLEIO_gatt_write_char_by_uuid_results.result = BLEIO_GATT_OK;

        auto handle = BLEIO_Seq_Create(
            (BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete
        );
        (void)BLEIO_Seq_Run(handle);
        mocks.ResetAllCalls();

        ///act
        auto result = BLEIO_Seq_Run(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
        BLEIO_Seq_Destroy(handle, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_014: [ BLEIO_Seq_Run shall return BLEIO_SEQ_ERROR if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_Seq_Run_returns_error_when_malloc_fails_when_scheduling_a_read_once_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            READ_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { 0 }
        };
        VECTOR_push_back(instructions, &instr1, 1);
        auto handle = BLEIO_Seq_Create(
            (BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((void*)NULL);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));         // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));   // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));

        ///act
        auto result = BLEIO_Seq_Run(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
        BLEIO_Seq_Destroy(handle, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_014: [ BLEIO_Seq_Run shall return BLEIO_SEQ_ERROR if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_Seq_Run_returns_error_when_BLEIO_gatt_read_char_by_uuid_fails_when_scheduling_a_read_once_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            READ_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { 0 }
        };
        const char* fake_char_id = STRING_c_str(instr1.characteristic_uuid);
        VECTOR_push_back(instructions, &instr1, 1);
        auto handle = BLEIO_Seq_Create(
            (BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));         // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));   // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_read_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .SetFailReturn((int)1);

        ///act
        auto result = BLEIO_Seq_Run(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
        BLEIO_Seq_Destroy(handle, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_018: [ When a READ_ONCE or a READ_PERIODIC instruction completes execution this API shall invoke the on_read_complete callback passing in the data that was read along with the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure. ]*/
    TEST_FUNCTION(BLEIO_Seq_Run_calls_on_read_complete_with_error_when_BLEIO_gatt_read_char_by_uuid_fails_when_scheduling_a_read_once_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            READ_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { 0 }
        };
        const char* fake_char_id = STRING_c_str(instr1.characteristic_uuid);
        VECTOR_push_back(instructions, &instr1, 1);

        // cause BLEIO_gatt_read_char_by_uuid to call callback with error
        BLEIO_gatt_read_char_by_uuid_results.result = BLEIO_GATT_ERROR;
        BLEIO_gatt_read_char_by_uuid_results.buffer = NULL;
        BLEIO_gatt_read_char_by_uuid_results.size = 0;

        auto handle = BLEIO_Seq_Create(
            (BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));         // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));   // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_read_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, on_read_complete(handle, NULL, fake_char_id, READ_ONCE, BLEIO_SEQ_ERROR, NULL));

        ///act
        (void)BLEIO_Seq_Run(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLEIO_Seq_Destroy(handle, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_018: [ When a READ_ONCE or a READ_PERIODIC instruction completes execution this API shall invoke the on_read_complete callback passing in the data that was read along with the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_015: [ BLEIO_Seq_Run shall schedule execution of all READ_ONCE instructions. ]*/
    TEST_FUNCTION(BLEIO_Seq_Run_calls_on_read_complete_with_success_when_BLEIO_gatt_read_char_by_uuid_succeeds)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            READ_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { 0 }
        };
        const char* fake_char_id = STRING_c_str(instr1.characteristic_uuid);
        VECTOR_push_back(instructions, &instr1, 1);

        // cause BLEIO_gatt_read_char_by_uuid to call callback with error
        BLEIO_gatt_read_char_by_uuid_results.result = BLEIO_GATT_OK;
        BLEIO_gatt_read_char_by_uuid_results.buffer = (unsigned char*)"data";
        BLEIO_gatt_read_char_by_uuid_results.size = 4;

        auto handle = BLEIO_Seq_Create(
            (BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));         // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));   // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_read_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, on_read_complete(handle, NULL, fake_char_id, READ_ONCE, BLEIO_SEQ_OK, IGNORED_PTR_ARG))
            .IgnoreArgument(6);
        STRICT_EXPECTED_CALL(mocks, BUFFER_create(IGNORED_PTR_ARG, 4))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        (void)BLEIO_Seq_Run(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLEIO_Seq_Destroy(handle, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_018: [ When a READ_ONCE or a READ_PERIODIC instruction completes execution this API shall invoke the on_read_complete callback passing in the data that was read along with the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure. ]*/
    TEST_FUNCTION(BLEIO_Seq_Run_passes_context_for_read)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            READ_ONCE,
            STRING_construct("fake_char_id"),
            (void*)0x42,
            { 0 }
        };
        const char* fake_char_id = STRING_c_str(instr1.characteristic_uuid);
        VECTOR_push_back(instructions, &instr1, 1);

        // cause BLEIO_gatt_read_char_by_uuid to call callback
        BLEIO_gatt_read_char_by_uuid_results.result = BLEIO_GATT_OK;
        BLEIO_gatt_read_char_by_uuid_results.buffer = (unsigned char*)"data";
        BLEIO_gatt_read_char_by_uuid_results.size = 4;

        auto handle = BLEIO_Seq_Create(
            (BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));         // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));   // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_read_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, on_read_complete(handle, (void*)0x42, fake_char_id, READ_ONCE, BLEIO_SEQ_OK, IGNORED_PTR_ARG))
            .IgnoreArgument(6);
        STRICT_EXPECTED_CALL(mocks, BUFFER_create(IGNORED_PTR_ARG, 4))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        (void)BLEIO_Seq_Run(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLEIO_Seq_Destroy(handle, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_018: [ When a READ_ONCE or a READ_PERIODIC instruction completes execution this API shall invoke the on_read_complete callback passing in the data that was read along with the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure. ]*/
    TEST_FUNCTION(BLEIO_Seq_Run_returns_error_when_malloc_fails_when_scheduling_a_read_periodic_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            READ_PERIODIC,
            STRING_construct("fake_char_id"),
            NULL,
            { 500 }
        };
        VECTOR_push_back(instructions, &instr1, 1);
        auto handle = BLEIO_Seq_Create(
            (BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((void*)NULL);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));         // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));   // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));

        ///act
        auto result = BLEIO_Seq_Run(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
        BLEIO_Seq_Destroy(handle, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_017: [ BLEIO_Seq_Run shall create timers at the specified intervals for scheduling execution of all READ_PERIODIC instructions. ]*/
    TEST_FUNCTION(timer_function_returns_G_SOURCE_CONTINUE_when_running_a_read_periodic_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            READ_PERIODIC,
            STRING_construct("fake_char_id"),
            NULL,
            { 500 }
        };
        const char* fake_char_id = STRING_c_str(instr1.characteristic_uuid);
        VECTOR_push_back(instructions, &instr1, 1);
        auto handle = BLEIO_Seq_Create(
            (BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);                                         // in schedule_periodic
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);                                         // in schedule_read
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);                                         // in on_read_complete in bleio_seq_linux_schedule_read.c
        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_read_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4);                                         // in schedule_read
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));         // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));   // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, on_read_complete(handle, NULL, fake_char_id, READ_PERIODIC, BLEIO_SEQ_OK, IGNORED_PTR_ARG))
            .IgnoreArgument(6);
        STRICT_EXPECTED_CALL(mocks, g_timeout_add(500, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, BUFFER_create(IGNORED_PTR_ARG, 4))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        // cause BLEIO_gatt_read_char_by_uuid to succeed
        BLEIO_gatt_read_char_by_uuid_results.result = BLEIO_GATT_OK;
        BLEIO_gatt_read_char_by_uuid_results.buffer = (const unsigned char*)"data";
        BLEIO_gatt_read_char_by_uuid_results.size = 4;

        // the assertion is in the g_timeout_add mock implementation
        g_expected_timer_return_value = G_SOURCE_CONTINUE;

        ///act
        auto result = BLEIO_Seq_Run(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_OK, result);

        ///cleanup
        BLEIO_Seq_Destroy(handle, NULL, NULL);
        
        // call the timer callback once more to cancel the timer
        g_expected_timer_return_value = G_SOURCE_REMOVE;
        g_timer_callback(g_timer_data);
    }

    /*Tests_SRS_BLEIO_SEQ_13_009: [ If there are active instructions of type READ_PERIODIC in progress then the timers associated with those instructions shall be cancelled. ]*/
    TEST_FUNCTION(timer_function_returns_G_SOURCE_REMOVE_after_shutdown_when_running_a_read_periodic_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            READ_PERIODIC,
            STRING_construct("fake_char_id"),
            NULL,
            { 500 }
        };
        VECTOR_push_back(instructions, &instr1, 1);
        auto handle = BLEIO_Seq_Create(
            (BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete
        );

        // cause BLEIO_gatt_read_char_by_uuid to succeed
        BLEIO_gatt_read_char_by_uuid_results.result = BLEIO_GATT_OK;
        BLEIO_gatt_read_char_by_uuid_results.buffer = (const unsigned char*)"data";
        BLEIO_gatt_read_char_by_uuid_results.size = 4;

        g_expected_timer_return_value = G_SOURCE_CONTINUE;
        (void)BLEIO_Seq_Run(handle);
        BLEIO_Seq_Destroy(handle, NULL, NULL);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);                                         // in dec_ref_handle
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);                                         // in on_timer
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);                                         // in dec_ref_handle
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));
        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_destroy((BLEIO_GATT_HANDLE)0x42));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(instr1.characteristic_uuid));

        ///act
        g_expected_timer_return_value = G_SOURCE_REMOVE;
        g_timer_callback(g_timer_data);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_BLEIO_SEQ_13_016: [ BLEIO_Seq_Run shall schedule execution of all WRITE_AT_INIT instructions. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_014: [ BLEIO_Seq_Run shall return BLEIO_SEQ_ERROR if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_Seq_Run_returns_error_when_malloc_fails_when_scheduling_a_write_init_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            WRITE_AT_INIT,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instr1, 1);
        auto handle = BLEIO_Seq_Create(
            (BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((void*)NULL);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));         // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));   // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));

        ///act
        auto result = BLEIO_Seq_Run(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
        BLEIO_Seq_Destroy(handle, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_033: [ BLEIO_Seq_Run shall schedule execution of all WRITE_ONCE instructions. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_014: [ BLEIO_Seq_Run shall return BLEIO_SEQ_ERROR if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_Seq_Run_returns_error_when_malloc_fails_when_scheduling_a_write_once_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instr1, 1);
        auto handle = BLEIO_Seq_Create(
            (BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((void*)NULL);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));         // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));   // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));

        ///act
        auto result = BLEIO_Seq_Run(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
        BLEIO_Seq_Destroy(handle, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_016: [ BLEIO_Seq_Run shall schedule execution of all WRITE_AT_INIT instructions. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_014: [ BLEIO_Seq_Run shall return BLEIO_SEQ_ERROR if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_Seq_Run_returns_error_when_BLEIO_gatt_write_char_by_uuid_fails_when_scheduling_a_write_init_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            WRITE_AT_INIT,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        const char* fake_char_id = STRING_c_str(instr1.characteristic_uuid);
        VECTOR_push_back(instructions, &instr1, 1);
        auto handle = BLEIO_Seq_Create(
            (BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));         // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));   // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_write_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6)
            .SetFailReturn((int)1);
        STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_length(instr1.data.buffer));

        ///act
        auto result = BLEIO_Seq_Run(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
        BLEIO_Seq_Destroy(handle, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_033: [ BLEIO_Seq_Run shall schedule execution of all WRITE_ONCE instructions. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_014: [ BLEIO_Seq_Run shall return BLEIO_SEQ_ERROR if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_Seq_Run_returns_error_when_BLEIO_gatt_write_char_by_uuid_fails_when_scheduling_a_write_once_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        const char* fake_char_id = STRING_c_str(instr1.characteristic_uuid);
        VECTOR_push_back(instructions, &instr1, 1);
        auto handle = BLEIO_Seq_Create(
            (BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));         // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));   // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_write_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6)
            .SetFailReturn((int)1);
        STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_length(instr1.data.buffer));

        ///act
        auto result = BLEIO_Seq_Run(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
        BLEIO_Seq_Destroy(handle, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_016: [ BLEIO_Seq_Run shall schedule execution of all WRITE_AT_INIT instructions. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_026: [ When the WRITE_AT_INIT instruction completes execution this API shall free the buffer that was passed in via the instruction. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_020: [ When the WRITE_AT_INIT instruction completes execution this API shall invoke the on_write_complete callback passing in the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure. ]*/
    TEST_FUNCTION(BLEIO_Seq_Run_calls_on_write_complete_with_error_when_BLEIO_gatt_write_char_by_uuid_fails_when_scheduling_a_write_init_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            WRITE_AT_INIT,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        const char* fake_char_id = STRING_c_str(instr1.characteristic_uuid);
        VECTOR_push_back(instructions, &instr1, 1);

        // cause BLEIO_gatt_write_char_by_uuid to call callback with ok
        BLEIO_gatt_write_char_by_uuid_results.result = BLEIO_GATT_ERROR;

        auto handle = BLEIO_Seq_Create(
            (BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));         // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));   // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_write_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6);
        STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_length(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, on_write_complete(handle, NULL, fake_char_id, WRITE_AT_INIT, BLEIO_SEQ_ERROR));

        ///act
        (void)BLEIO_Seq_Run(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLEIO_Seq_Destroy(handle, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_033: [ BLEIO_Seq_Run shall schedule execution of all WRITE_ONCE instructions. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_035: [ When the WRITE_ONCE instruction completes execution this API shall free the buffer that was passed in via the instruction. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_034: [ When the WRITE_ONCE instruction completes execution this API shall invoke the on_write_complete callback passing in the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure. ]*/
    TEST_FUNCTION(BLEIO_Seq_Run_calls_on_write_complete_with_error_when_BLEIO_gatt_write_char_by_uuid_fails_when_scheduling_a_write_once_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        const char* fake_char_id = STRING_c_str(instr1.characteristic_uuid);
        VECTOR_push_back(instructions, &instr1, 1);

        // cause BLEIO_gatt_write_char_by_uuid to call callback with ok
        BLEIO_gatt_write_char_by_uuid_results.result = BLEIO_GATT_ERROR;

        auto handle = BLEIO_Seq_Create(
            (BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));         // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));   // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_write_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6);
        STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_length(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, on_write_complete(handle, NULL, fake_char_id, WRITE_ONCE, BLEIO_SEQ_ERROR));

        ///act
        (void)BLEIO_Seq_Run(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLEIO_Seq_Destroy(handle, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_016: [ BLEIO_Seq_Run shall schedule execution of all WRITE_AT_INIT instructions. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_026: [ When the WRITE_AT_INIT instruction completes execution this API shall free the buffer that was passed in via the instruction. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_020: [ When the WRITE_AT_INIT instruction completes execution this API shall invoke the on_write_complete callback passing in the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure. ]*/
    TEST_FUNCTION(BLEIO_Seq_Run_calls_on_write_complete_with_success_when_BLEIO_gatt_write_char_by_uuid_succeeds_for_write_at_init)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            WRITE_AT_INIT,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        const char* fake_char_id = STRING_c_str(instr1.characteristic_uuid);
        VECTOR_push_back(instructions, &instr1, 1);

        // cause BLEIO_gatt_write_char_by_uuid to call callback with ok
        BLEIO_gatt_write_char_by_uuid_results.result = BLEIO_GATT_OK;

        auto handle = BLEIO_Seq_Create(
            (BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));         // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));   // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_write_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6);
        STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_length(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, on_write_complete(handle, NULL, fake_char_id, WRITE_AT_INIT, BLEIO_SEQ_OK));

        ///act
        (void)BLEIO_Seq_Run(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLEIO_Seq_Destroy(handle, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_033: [ BLEIO_Seq_Run shall schedule execution of all WRITE_ONCE instructions. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_035: [ When the WRITE_ONCE instruction completes execution this API shall free the buffer that was passed in via the instruction. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_034: [ When the WRITE_ONCE instruction completes execution this API shall invoke the on_write_complete callback passing in the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure. ]*/
    TEST_FUNCTION(BLEIO_Seq_Run_calls_on_write_complete_with_success_when_BLEIO_gatt_write_char_by_uuid_succeeds_for_write_once)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        const char* fake_char_id = STRING_c_str(instr1.characteristic_uuid);
        VECTOR_push_back(instructions, &instr1, 1);

        // cause BLEIO_gatt_write_char_by_uuid to call callback with ok
        BLEIO_gatt_write_char_by_uuid_results.result = BLEIO_GATT_OK;

        auto handle = BLEIO_Seq_Create(
            (BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));         // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));   // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_write_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6);
        STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_length(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, on_write_complete(handle, NULL, fake_char_id, WRITE_ONCE, BLEIO_SEQ_OK));

        ///act
        (void)BLEIO_Seq_Run(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLEIO_Seq_Destroy(handle, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_016: [ BLEIO_Seq_Run shall schedule execution of all WRITE_AT_INIT instructions. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_026: [ When the WRITE_AT_INIT instruction completes execution this API shall free the buffer that was passed in via the instruction. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_020: [ When the WRITE_AT_INIT instruction completes execution this API shall invoke the on_write_complete callback passing in the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure. ]*/
    TEST_FUNCTION(BLEIO_Seq_Run_passes_context_for_write_at_init)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            WRITE_AT_INIT,
            STRING_construct("fake_char_id"),
            (void*)0x42,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        const char* fake_char_id = STRING_c_str(instr1.characteristic_uuid);
        VECTOR_push_back(instructions, &instr1, 1);

        // cause BLEIO_gatt_write_char_by_uuid to call callback with ok
        BLEIO_gatt_write_char_by_uuid_results.result = BLEIO_GATT_OK;

        auto handle = BLEIO_Seq_Create(
            (BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));         // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));   // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_write_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6);
        STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_length(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, on_write_complete(handle, (void*)0x42, fake_char_id, WRITE_AT_INIT, BLEIO_SEQ_OK));

        ///act
        (void)BLEIO_Seq_Run(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLEIO_Seq_Destroy(handle, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_033: [ BLEIO_Seq_Run shall schedule execution of all WRITE_ONCE instructions. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_035: [ When the WRITE_ONCE instruction completes execution this API shall free the buffer that was passed in via the instruction. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_034: [ When the WRITE_ONCE instruction completes execution this API shall invoke the on_write_complete callback passing in the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure. ]*/
    TEST_FUNCTION(BLEIO_Seq_Run_passes_context_for_write_once)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            (void*)0x42,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        const char* fake_char_id = STRING_c_str(instr1.characteristic_uuid);
        VECTOR_push_back(instructions, &instr1, 1);

        // cause BLEIO_gatt_write_char_by_uuid to call callback with ok
        BLEIO_gatt_write_char_by_uuid_results.result = BLEIO_GATT_OK;

        auto handle = BLEIO_Seq_Create(
            (BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));         // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));   // in BLEIO_Seq_Run
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_write_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6);
        STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_length(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, on_write_complete(handle, (void*)0x42, fake_char_id, WRITE_ONCE, BLEIO_SEQ_OK));

        ///act
        (void)BLEIO_Seq_Run(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLEIO_Seq_Destroy(handle, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_011: [ BLEIO_Seq_Destroy shall schedule the execution of all WRITE_AT_EXIT instructions.*/
    /*Tests_SRS_BLEIO_SEQ_13_006: [ BLEIO_Seq_Destroy shall free all resources associated with the handle once all the pending I/O operations are complete. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_027: [ When the WRITE_AT_EXIT instruction completes execution this API shall free the buffer that was passed in via the instruction. ]*/
    TEST_FUNCTION(BLEIO_Seq_Destroy_schedules_write_at_exit_and_frees_resources)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            WRITE_AT_EXIT,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        const char* fake_char_id = STRING_c_str(instr1.characteristic_uuid);
        VECTOR_push_back(instructions, &instr1, 1);

        // cause BLEIO_gatt_write_char_by_uuid to call callback with ok
        BLEIO_gatt_write_char_by_uuid_results.result = BLEIO_GATT_OK;

        auto handle = BLEIO_Seq_Create(
            (BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));         // in BLEIO_Seq_Destroy
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));         // in dec_ref_handle
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));   // in BLEIO_Seq_Destroy
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));   // in dec_ref_handle
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(instructions));      // in BLEIO_Seq_Destroy
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_write_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6);
        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_destroy((BLEIO_GATT_HANDLE)0x42));
        STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_length(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, on_write_complete(handle, NULL, fake_char_id, WRITE_AT_EXIT, BLEIO_SEQ_OK));

        ///act
        BLEIO_Seq_Destroy(handle, NULL, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_BLEIO_SEQ_13_011: [ BLEIO_Seq_Destroy shall schedule the execution of all WRITE_AT_EXIT instructions.*/
    /*Tests_SRS_BLEIO_SEQ_13_006: [ BLEIO_Seq_Destroy shall free all resources associated with the handle once all the pending I/O operations are complete. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_027: [ When the WRITE_AT_EXIT instruction completes execution this API shall free the buffer that was passed in via the instruction. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_031: [ If on_destroy_complete is not NULL then BLEIO_Seq_Destroy shall invoke on_destroy_complete once all WRITE_AT_EXIT instructions have been executed. ]*/
    TEST_FUNCTION(BLEIO_Seq_Destroy_calls_on_destroy_complete)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            WRITE_AT_EXIT,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        const char* fake_char_id = STRING_c_str(instr1.characteristic_uuid);
        VECTOR_push_back(instructions, &instr1, 1);

        // cause BLEIO_gatt_write_char_by_uuid to call callback with ok
        BLEIO_gatt_write_char_by_uuid_results.result = BLEIO_GATT_OK;

        auto handle = BLEIO_Seq_Create(
            (BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));         // in BLEIO_Seq_Destroy
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));         // in dec_ref_handle
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));   // in BLEIO_Seq_Destroy
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));   // in dec_ref_handle
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(instructions));      // in BLEIO_Seq_Destroy
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_write_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6);
        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_destroy((BLEIO_GATT_HANDLE)0x42));
        STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_length(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, on_write_complete(handle, NULL, fake_char_id, WRITE_AT_EXIT, BLEIO_SEQ_OK));
        STRICT_EXPECTED_CALL(mocks, on_destroy_complete(handle, NULL));

        ///act
        BLEIO_Seq_Destroy(handle, on_destroy_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_BLEIO_SEQ_13_011: [ BLEIO_Seq_Destroy shall schedule the execution of all WRITE_AT_EXIT instructions.*/
    /*Tests_SRS_BLEIO_SEQ_13_006: [ BLEIO_Seq_Destroy shall free all resources associated with the handle once all the pending I/O operations are complete. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_027: [ When the WRITE_AT_EXIT instruction completes execution this API shall free the buffer that was passed in via the instruction. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_031: [ If on_destroy_complete is not NULL then BLEIO_Seq_Destroy shall invoke on_destroy_complete once all WRITE_AT_EXIT instructions have been executed. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_032: [ If on_destroy_complete is not NULL then BLEIO_Seq_Destroy shall pass context as-is to on_destroy_complete. ]*/
    TEST_FUNCTION(BLEIO_Seq_Destroy_calls_on_destroy_complete_and_passes_context)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            WRITE_AT_EXIT,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        const char* fake_char_id = STRING_c_str(instr1.characteristic_uuid);
        VECTOR_push_back(instructions, &instr1, 1);

        // cause BLEIO_gatt_write_char_by_uuid to call callback with ok
        BLEIO_gatt_write_char_by_uuid_results.result = BLEIO_GATT_OK;

        auto handle = BLEIO_Seq_Create(
            (BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));         // in BLEIO_Seq_Destroy
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));         // in dec_ref_handle
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));   // in BLEIO_Seq_Destroy
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));   // in dec_ref_handle
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(instructions));      // in BLEIO_Seq_Destroy
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_write_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6);
        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_destroy((BLEIO_GATT_HANDLE)0x42));
        STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_length(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, on_write_complete(handle, NULL, fake_char_id, WRITE_AT_EXIT, BLEIO_SEQ_OK));
        STRICT_EXPECTED_CALL(mocks, on_destroy_complete(handle, (void*)0x42));

        ///act
        BLEIO_Seq_Destroy(handle, on_destroy_complete, (void*)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_BLEIO_SEQ_13_011: [ BLEIO_Seq_Destroy shall schedule the execution of all WRITE_AT_EXIT instructions.*/
    /*Tests_SRS_BLEIO_SEQ_13_021: [ When the WRITE_AT_EXIT instruction completes execution this API shall invoke the on_write_complete callback passing in the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_027: [ When the WRITE_AT_EXIT instruction completes execution this API shall free the buffer that was passed in via the instruction. ]*/
    TEST_FUNCTION(BLEIO_Seq_Destroy_passes_context_for_write_at_exit)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instr1 =
        {
            WRITE_AT_EXIT,
            STRING_construct("fake_char_id"),
            (void*)0x42,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        const char* fake_char_id = STRING_c_str(instr1.characteristic_uuid);
        VECTOR_push_back(instructions, &instr1, 1);

        // cause BLEIO_gatt_write_char_by_uuid to call callback with ok
        BLEIO_gatt_write_char_by_uuid_results.result = BLEIO_GATT_OK;

        auto handle = BLEIO_Seq_Create(
            (BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete
        );
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));         // in BLEIO_Seq_Destroy
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));         // in dec_ref_handle
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));   // in BLEIO_Seq_Destroy
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));   // in dec_ref_handle
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(instructions));      // in BLEIO_Seq_Destroy
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(instr1.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_write_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6);
        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_destroy((BLEIO_GATT_HANDLE)0x42));
        STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_length(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(instr1.data.buffer));
        STRICT_EXPECTED_CALL(mocks, on_write_complete(handle, (void*)0x42, fake_char_id, WRITE_AT_EXIT, BLEIO_SEQ_OK));

        ///act
        BLEIO_Seq_Destroy(handle, NULL, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_BLEIO_SEQ_13_007: [ If bleio_seq_handle is NULL then BLEIO_Seq_Destroy shall do nothing. ]*/
    TEST_FUNCTION(BLEIO_Seq_Destroy_does_nothing_for_NULL_input)
    {
        ///arrange
        CBLEIOSeqMocks mocks;

        ///act
        BLEIO_Seq_Destroy(NULL, NULL, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_BLEIO_SEQ_13_036: [ BLEIO_Seq_AddInstruction shall return BLEIO_SEQ_ERROR if bleio_seq_handle is NULL. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_returns_error_for_NULL_bleio_seq_handle)
    {
        ///arrange
        CBLEIOSeqMocks mocks;

        ///act
        auto result = BLEIO_Seq_AddInstruction(NULL, (BLEIO_SEQ_INSTRUCTION*)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
    }

    /*Tests_SRS_BLEIO_SEQ_13_046: [ BLEIO_Seq_AddInstruction shall return BLEIO_SEQ_ERROR if instruction is NULL. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_returns_error_for_NULL_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;

        ///act
        auto result = BLEIO_Seq_AddInstruction((BLEIO_SEQ_HANDLE)0x42, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
    }

    /*Tests_SRS_BLEIO_SEQ_13_049: [ BLEIO_Seq_AddInstruction shall return BLEIO_SEQ_ERROR if the characteristic_uuid field for the instruction is NULL or empty. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_returns_error_when_instruction_has_NULL_charid)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            READ_ONCE,
            NULL, // NULL characteristic ID
            NULL,
            { 0 }
        };

        ///act
        auto result = BLEIO_Seq_AddInstruction((BLEIO_SEQ_HANDLE)0x42, &instruction);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
    }

    /*Tests_SRS_BLEIO_SEQ_13_049: [ BLEIO_Seq_AddInstruction shall return BLEIO_SEQ_ERROR if the characteristic_uuid field for the instruction is NULL or empty. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_returns_error_when_instruction_has_empty_charid)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            READ_ONCE,
            STRING_construct(""), // empty characteristic ID
            NULL,
            { 0 }
        };
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction.characteristic_uuid));

        ///act
        auto result = BLEIO_Seq_AddInstruction((BLEIO_SEQ_HANDLE)0x42, &instruction);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
        STRING_delete(instruction.characteristic_uuid);
    }

    /*Tests_SRS_BLEIO_SEQ_13_047: [ BLEIO_Seq_AddInstruction shall return BLEIO_SEQ_ERROR if a READ_PERIODIC instruction's interval_in_ms field is zero. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_returns_error_when_read_periodic_instruction_has_zero_interval)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            READ_PERIODIC,
            STRING_construct("fake_char_id"),
            NULL,
            { 0 } // zero interval
        };
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction.characteristic_uuid));

        ///act
        auto result = BLEIO_Seq_AddInstruction((BLEIO_SEQ_HANDLE)0x42, &instruction);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
        STRING_delete(instruction.characteristic_uuid);
    }

    /*Tests_SRS_BLEIO_SEQ_13_048: [ BLEIO_Seq_AddInstruction shall return BLEIO_SEQ_ERROR if a WRITE_AT_INIT or a WRITE_AT_EXIT or a WRITE_ONCE instruction has a NULL value in the buffer field. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_returns_error_when_a_write_at_init_instruction_has_NULL_buffer)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            WRITE_AT_INIT,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = NULL } // NULL buffer
        };
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction.characteristic_uuid));

        ///act
        auto result = BLEIO_Seq_AddInstruction((BLEIO_SEQ_HANDLE)0x42, &instruction);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
        STRING_delete(instruction.characteristic_uuid);
    }

    /*Tests_SRS_BLEIO_SEQ_13_048: [ BLEIO_Seq_AddInstruction shall return BLEIO_SEQ_ERROR if a WRITE_AT_INIT or a WRITE_AT_EXIT or a WRITE_ONCE instruction has a NULL value in the buffer field. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_returns_error_when_a_write_at_exit_instruction_has_NULL_buffer)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            WRITE_AT_EXIT,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = NULL } // NULL buffer
        };
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction.characteristic_uuid));

        ///act
        auto result = BLEIO_Seq_AddInstruction((BLEIO_SEQ_HANDLE)0x42, &instruction);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
        STRING_delete(instruction.characteristic_uuid);
    }

    /*Tests_SRS_BLEIO_SEQ_13_048: [ BLEIO_Seq_AddInstruction shall return BLEIO_SEQ_ERROR if a WRITE_AT_INIT or a WRITE_AT_EXIT or a WRITE_ONCE instruction has a NULL value in the buffer field. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_returns_error_when_a_write_once_instruction_has_NULL_buffer)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = NULL } // NULL buffer
        };
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction.characteristic_uuid));

        ///act
        auto result = BLEIO_Seq_AddInstruction((BLEIO_SEQ_HANDLE)0x42, &instruction);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
        STRING_delete(instruction.characteristic_uuid);
    }

    /*Tests_SRS_BLEIO_SEQ_13_045: [ BLEIO_Seq_AddInstruction shall return BLEIO_SEQ_ERROR if BLEIO_Seq_Run was NOT called first. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_returns_error_when_seq_is_not_running)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instruction, 1);
        auto sequence = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction.characteristic_uuid));

        ///act
        auto result = BLEIO_Seq_AddInstruction(sequence, &instruction);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
        BLEIO_Seq_Destroy(sequence, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_037: [ BLEIO_Seq_AddInstruction shall return BLEIO_SEQ_ERROR if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_returns_error_when_malloc_fails)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instruction, 1);
        auto sequence = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);
        (void)BLEIO_Seq_Run(sequence);

        BLEIO_SEQ_INSTRUCTION instruction2 =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((void*)NULL);

        ///act
        auto result = BLEIO_Seq_AddInstruction(sequence, &instruction2);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
        BLEIO_Seq_Destroy(sequence, NULL, NULL);
        STRING_delete(instruction2.characteristic_uuid);
        BUFFER_delete(instruction2.data.buffer);
    }

    /*Tests_SRS_BLEIO_SEQ_13_037: [ BLEIO_Seq_AddInstruction shall return BLEIO_SEQ_ERROR if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_returns_error_when_malloc_fails_when_scheduling_a_read_once_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instruction, 1);
        auto sequence = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);
        (void)BLEIO_Seq_Run(sequence);

        BLEIO_SEQ_INSTRUCTION instruction2 =
        {
            READ_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { 0 }
        };

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((void*)NULL);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        auto result = BLEIO_Seq_AddInstruction(sequence, &instruction2);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
        BLEIO_Seq_Destroy(sequence, NULL, NULL);
        STRING_delete(instruction2.characteristic_uuid);
    }

    /*Tests_SRS_BLEIO_SEQ_13_037: [ BLEIO_Seq_AddInstruction shall return BLEIO_SEQ_ERROR if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_returns_error_when_BLEIO_gatt_read_char_by_uuid_fails_when_scheduling_a_read_once_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instruction, 1);
        auto sequence = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);
        (void)BLEIO_Seq_Run(sequence);

        BLEIO_SEQ_INSTRUCTION instruction2 =
        {
            READ_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { 0 }
        };

        const char* fake_char_id = STRING_c_str(instruction2.characteristic_uuid);

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_read_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .SetFailReturn((int)1);

        ///act
        auto result = BLEIO_Seq_AddInstruction(sequence, &instruction2);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
        BLEIO_Seq_Destroy(sequence, NULL, NULL);
        STRING_delete(instruction2.characteristic_uuid);
    }

    /*Tests_SRS_BLEIO_SEQ_13_040: [ When a READ_ONCE or a READ_PERIODIC instruction completes execution this API shall invoke the on_read_complete callback passing in the data that was read along with the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_calls_on_read_complete_with_error_when_BLEIO_gatt_read_char_by_uuid_fails_when_scheduling_a_read_once_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instruction, 1);
        auto sequence = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);
        (void)BLEIO_Seq_Run(sequence);

        // cause BLEIO_gatt_read_char_by_uuid to call callback with error
        BLEIO_gatt_read_char_by_uuid_results.result = BLEIO_GATT_ERROR;
        BLEIO_gatt_read_char_by_uuid_results.buffer = NULL;
        BLEIO_gatt_read_char_by_uuid_results.size = 0;

        BLEIO_SEQ_INSTRUCTION instruction2 =
        {
            READ_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { 0 }
        };

        const char* fake_char_id = STRING_c_str(instruction2.characteristic_uuid);

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(instruction2.characteristic_uuid));

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_read_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, on_read_complete(sequence, NULL, fake_char_id, READ_ONCE, BLEIO_SEQ_ERROR, NULL));

        ///act
        (void)BLEIO_Seq_AddInstruction(sequence, &instruction2);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLEIO_Seq_Destroy(sequence, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_040: [ When a READ_ONCE or a READ_PERIODIC instruction completes execution this API shall invoke the on_read_complete callback passing in the data that was read along with the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_calls_on_read_complete_with_success_when_BLEIO_gatt_read_char_by_uuid_succeeds)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instruction, 1);
        auto sequence = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);
        (void)BLEIO_Seq_Run(sequence);

        // cause BLEIO_gatt_read_char_by_uuid to call callback with error
        BLEIO_gatt_read_char_by_uuid_results.result = BLEIO_GATT_OK;
        BLEIO_gatt_read_char_by_uuid_results.buffer = (unsigned char*)"data";
        BLEIO_gatt_read_char_by_uuid_results.size = 4;

        BLEIO_SEQ_INSTRUCTION instruction2 =
        {
            READ_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { 0 }
        };

        const char* fake_char_id = STRING_c_str(instruction2.characteristic_uuid);

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(instruction2.characteristic_uuid));

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, BUFFER_create(IGNORED_PTR_ARG, 4))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_read_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, on_read_complete(sequence, NULL, fake_char_id, READ_ONCE, BLEIO_SEQ_OK, IGNORED_PTR_ARG))
            .IgnoreArgument(6);

        ///act
        (void)BLEIO_Seq_AddInstruction(sequence, &instruction2);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLEIO_Seq_Destroy(sequence, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_040: [ When a READ_ONCE or a READ_PERIODIC instruction completes execution this API shall invoke the on_read_complete callback passing in the data that was read along with the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_passes_context_for_read)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instruction, 1);
        auto sequence = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);
        (void)BLEIO_Seq_Run(sequence);

        // cause BLEIO_gatt_read_char_by_uuid to call callback with error
        BLEIO_gatt_read_char_by_uuid_results.result = BLEIO_GATT_OK;
        BLEIO_gatt_read_char_by_uuid_results.buffer = (unsigned char*)"data";
        BLEIO_gatt_read_char_by_uuid_results.size = 4;

        BLEIO_SEQ_INSTRUCTION instruction2 =
        {
            READ_ONCE,
            STRING_construct("fake_char_id"),
            (void*)0x42,
            { 0 }
        };

        const char* fake_char_id = STRING_c_str(instruction2.characteristic_uuid);

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(instruction2.characteristic_uuid));

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, BUFFER_create(IGNORED_PTR_ARG, 4))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_read_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, on_read_complete(sequence, (void*)0x42, fake_char_id, READ_ONCE, BLEIO_SEQ_OK, IGNORED_PTR_ARG))
            .IgnoreArgument(6);

        ///act
        (void)BLEIO_Seq_AddInstruction(sequence, &instruction2);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLEIO_Seq_Destroy(sequence, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_037: [ BLEIO_Seq_AddInstruction shall return BLEIO_SEQ_ERROR if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_returns_error_when_malloc_fails_when_scheduling_a_read_periodic_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instruction, 1);
        auto sequence = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);
        (void)BLEIO_Seq_Run(sequence);

        BLEIO_SEQ_INSTRUCTION instruction2 =
        {
            READ_PERIODIC,
            STRING_construct("fake_char_id"),
            NULL,
            { 500 }
        };

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((void*)NULL);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        auto result = BLEIO_Seq_AddInstruction(sequence, &instruction2);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
        BLEIO_Seq_Destroy(sequence, NULL, NULL);
        STRING_delete(instruction2.characteristic_uuid);
    }

    /*Tests_SRS_BLEIO_SEQ_13_039: [ BLEIO_Seq_AddInstruction shall create a timer at the specified interval if the instruction is a READ_PERIODIC instruction. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_timer_function_returns_G_SOURCE_CONTINUE_when_running_a_read_periodic_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instruction, 1);
        auto sequence = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);
        (void)BLEIO_Seq_Run(sequence);

        BLEIO_SEQ_INSTRUCTION instruction2 =
        {
            READ_PERIODIC,
            STRING_construct("fake_char_id"),
            NULL,
            { 500 }
        };

        const char* fake_char_id = STRING_c_str(instruction2.characteristic_uuid);

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_read_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4);                                         // in schedule_read

        STRICT_EXPECTED_CALL(mocks, BUFFER_create(IGNORED_PTR_ARG, 4))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, g_timeout_add(500, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        STRICT_EXPECTED_CALL(mocks, on_read_complete(sequence, NULL, fake_char_id, READ_PERIODIC, BLEIO_SEQ_OK, IGNORED_PTR_ARG))
            .IgnoreArgument(6);

        // cause BLEIO_gatt_read_char_by_uuid to succeed
        BLEIO_gatt_read_char_by_uuid_results.result = BLEIO_GATT_OK;
        BLEIO_gatt_read_char_by_uuid_results.buffer = (const unsigned char*)"data";
        BLEIO_gatt_read_char_by_uuid_results.size = 4;

        // the assertion is in the g_timeout_add mock implementation
        g_expected_timer_return_value = G_SOURCE_CONTINUE;

        ///act
        auto result = BLEIO_Seq_AddInstruction(sequence, &instruction2);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_OK, result);

        ///cleanup
        BLEIO_Seq_Destroy(sequence, NULL, NULL);

        // call the timer callback once more to cancel the timer
        g_expected_timer_return_value = G_SOURCE_REMOVE;
        g_timer_callback(g_timer_data);
    }

    /*Tests_SRS_BLEIO_SEQ_13_009: [ If there are active instructions of type READ_PERIODIC in progress then the timers associated with those instructions shall be cancelled. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_timer_function_returns_G_SOURCE_REMOVE_after_shutdown_when_running_a_read_periodic_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instruction, 1);
        auto sequence = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);
        (void)BLEIO_Seq_Run(sequence);

        BLEIO_SEQ_INSTRUCTION instruction2 =
        {
            READ_PERIODIC,
            STRING_construct("fake_char_id"),
            NULL,
            { 500 }
        };

        const char* fake_char_id = STRING_c_str(instruction2.characteristic_uuid);

        // cause BLEIO_gatt_read_char_by_uuid to succeed
        BLEIO_gatt_read_char_by_uuid_results.result = BLEIO_GATT_OK;
        BLEIO_gatt_read_char_by_uuid_results.buffer = (const unsigned char*)"data";
        BLEIO_gatt_read_char_by_uuid_results.size = 4;

        // the assertion is in the g_timeout_add mock implementation
        g_expected_timer_return_value = G_SOURCE_CONTINUE;

        (void)BLEIO_Seq_AddInstruction(sequence, &instruction2);
        BLEIO_Seq_Destroy(sequence, NULL, NULL);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);                                         // in dec_ref_handle
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);                                         // in on_timer
        
        STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);                                         // in dec_ref_handle
        STRICT_EXPECTED_CALL(mocks, VECTOR_size(instructions));
        STRICT_EXPECTED_CALL(mocks, VECTOR_element(instructions, 0));
        
        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_destroy((BLEIO_GATT_HANDLE)0x42));

        STRICT_EXPECTED_CALL(mocks, STRING_delete(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(instruction.characteristic_uuid));

        ///act
        g_expected_timer_return_value = G_SOURCE_REMOVE;
        g_timer_callback(g_timer_data);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_BLEIO_SEQ_13_037: [ BLEIO_Seq_AddInstruction shall return BLEIO_SEQ_ERROR if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_returns_error_when_malloc_fails_when_scheduling_a_write_init_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instruction, 1);
        auto sequence = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);
        (void)BLEIO_Seq_Run(sequence);

        BLEIO_SEQ_INSTRUCTION instruction2 =
        {
            WRITE_AT_INIT,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((void*)NULL);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        auto result = BLEIO_Seq_AddInstruction(sequence, &instruction2);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
        BLEIO_Seq_Destroy(sequence, NULL, NULL);
        STRING_delete(instruction2.characteristic_uuid);
        BUFFER_delete(instruction2.data.buffer);
    }

    /*Tests_SRS_BLEIO_SEQ_13_037: [ BLEIO_Seq_AddInstruction shall return BLEIO_SEQ_ERROR if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_returns_error_when_malloc_fails_when_scheduling_a_write_once_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instruction, 1);
        auto sequence = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);
        (void)BLEIO_Seq_Run(sequence);

        BLEIO_SEQ_INSTRUCTION instruction2 =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((void*)NULL);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        auto result = BLEIO_Seq_AddInstruction(sequence, &instruction2);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
        BLEIO_Seq_Destroy(sequence, NULL, NULL);
        STRING_delete(instruction2.characteristic_uuid);
        BUFFER_delete(instruction2.data.buffer);
    }

    /*Tests_SRS_BLEIO_SEQ_13_037: [ BLEIO_Seq_AddInstruction shall return BLEIO_SEQ_ERROR if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_returns_error_when_BLEIO_gatt_write_char_by_uuid_fails_when_scheduling_a_write_init_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instruction, 1);
        auto sequence = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);
        (void)BLEIO_Seq_Run(sequence);

        BLEIO_SEQ_INSTRUCTION instruction2 =
        {
            WRITE_AT_INIT,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };

        const char* fake_char_id = STRING_c_str(instruction2.characteristic_uuid);

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_write_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6)
            .SetFailReturn((int)1);
        STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(instruction2.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_length(instruction2.data.buffer));

        ///act
        auto result = BLEIO_Seq_AddInstruction(sequence, &instruction2);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
        BLEIO_Seq_Destroy(sequence, NULL, NULL);
        STRING_delete(instruction2.characteristic_uuid);
        BUFFER_delete(instruction2.data.buffer);
    }

    /*Tests_SRS_BLEIO_SEQ_13_037: [ BLEIO_Seq_AddInstruction shall return BLEIO_SEQ_ERROR if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_returns_error_when_BLEIO_gatt_write_char_by_uuid_fails_when_scheduling_a_write_once_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instruction, 1);
        auto sequence = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);
        (void)BLEIO_Seq_Run(sequence);

        BLEIO_SEQ_INSTRUCTION instruction2 =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };

        const char* fake_char_id = STRING_c_str(instruction2.characteristic_uuid);

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_write_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6)
            .SetFailReturn((int)1);
        STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(instruction2.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_length(instruction2.data.buffer));

        ///act
        auto result = BLEIO_Seq_AddInstruction(sequence, &instruction2);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(BLEIO_SEQ_RESULT, BLEIO_SEQ_ERROR, result);

        ///cleanup
        BLEIO_Seq_Destroy(sequence, NULL, NULL);
        STRING_delete(instruction2.characteristic_uuid);
        BUFFER_delete(instruction2.data.buffer);
    }

    /*Tests_SRS_BLEIO_SEQ_13_042: [ When a WRITE_ONCE or a WRITE_AT_INIT instruction completes execution this API shall invoke the on_write_complete callback passing in the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_041: [ When a WRITE_AT_INIT or a WRITE_ONCE instruction completes execution this API shall free the buffer that was passed in via the instruction. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_calls_on_write_complete_with_error_when_BLEIO_gatt_write_char_by_uuid_fails_when_scheduling_a_write_init_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instruction, 1);
        auto sequence = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);
        (void)BLEIO_Seq_Run(sequence);

        BLEIO_SEQ_INSTRUCTION instruction2 =
        {
            WRITE_AT_INIT,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };

        const char* fake_char_id = STRING_c_str(instruction2.characteristic_uuid);

        // cause BLEIO_gatt_write_char_by_uuid to call callback with error
        BLEIO_gatt_write_char_by_uuid_results.result = BLEIO_GATT_ERROR;

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(instruction2.characteristic_uuid));

        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(instruction2.data.buffer));

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_write_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6);
        STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(instruction2.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_length(instruction2.data.buffer));

        STRICT_EXPECTED_CALL(mocks, on_write_complete(sequence, NULL, fake_char_id, WRITE_AT_INIT, BLEIO_SEQ_ERROR));

        ///act
        (void)BLEIO_Seq_AddInstruction(sequence, &instruction2);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLEIO_Seq_Destroy(sequence, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_042: [ When a WRITE_ONCE or a WRITE_AT_INIT instruction completes execution this API shall invoke the on_write_complete callback passing in the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_041: [ When a WRITE_AT_INIT or a WRITE_ONCE instruction completes execution this API shall free the buffer that was passed in via the instruction. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_calls_on_write_complete_with_error_when_BLEIO_gatt_write_char_by_uuid_fails_when_scheduling_a_write_once_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instruction, 1);
        auto sequence = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);
        (void)BLEIO_Seq_Run(sequence);

        BLEIO_SEQ_INSTRUCTION instruction2 =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };

        const char* fake_char_id = STRING_c_str(instruction2.characteristic_uuid);

        // cause BLEIO_gatt_write_char_by_uuid to call callback with error
        BLEIO_gatt_write_char_by_uuid_results.result = BLEIO_GATT_ERROR;

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(instruction2.characteristic_uuid));

        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(instruction2.data.buffer));

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_write_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6);
        STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(instruction2.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_length(instruction2.data.buffer));

        STRICT_EXPECTED_CALL(mocks, on_write_complete(sequence, NULL, fake_char_id, WRITE_ONCE, BLEIO_SEQ_ERROR));

        ///act
        (void)BLEIO_Seq_AddInstruction(sequence, &instruction2);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLEIO_Seq_Destroy(sequence, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_042: [ When a WRITE_ONCE or a WRITE_AT_INIT instruction completes execution this API shall invoke the on_write_complete callback passing in the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_041: [ When a WRITE_AT_INIT or a WRITE_ONCE instruction completes execution this API shall free the buffer that was passed in via the instruction. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_calls_on_write_complete_with_sucess_when_BLEIO_gatt_write_char_by_uuid_succeeds_when_scheduling_a_write_init_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instruction, 1);
        auto sequence = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);
        (void)BLEIO_Seq_Run(sequence);

        BLEIO_SEQ_INSTRUCTION instruction2 =
        {
            WRITE_AT_INIT,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };

        const char* fake_char_id = STRING_c_str(instruction2.characteristic_uuid);

        // cause BLEIO_gatt_write_char_by_uuid to call callback with ok
        BLEIO_gatt_write_char_by_uuid_results.result = BLEIO_GATT_OK;

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(instruction2.characteristic_uuid));

        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(instruction2.data.buffer));

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_write_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6);
        STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(instruction2.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_length(instruction2.data.buffer));

        STRICT_EXPECTED_CALL(mocks, on_write_complete(sequence, NULL, fake_char_id, WRITE_AT_INIT, BLEIO_SEQ_OK));

        ///act
        (void)BLEIO_Seq_AddInstruction(sequence, &instruction2);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLEIO_Seq_Destroy(sequence, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_042: [ When a WRITE_ONCE or a WRITE_AT_INIT instruction completes execution this API shall invoke the on_write_complete callback passing in the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_041: [ When a WRITE_AT_INIT or a WRITE_ONCE instruction completes execution this API shall free the buffer that was passed in via the instruction. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_calls_on_write_complete_with_sucess_when_BLEIO_gatt_write_char_by_uuid_succeeds_when_scheduling_a_write_once_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instruction, 1);
        auto sequence = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);
        (void)BLEIO_Seq_Run(sequence);

        BLEIO_SEQ_INSTRUCTION instruction2 =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };

        const char* fake_char_id = STRING_c_str(instruction2.characteristic_uuid);

        // cause BLEIO_gatt_write_char_by_uuid to call callback with ok
        BLEIO_gatt_write_char_by_uuid_results.result = BLEIO_GATT_OK;

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(instruction2.characteristic_uuid));

        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(instruction2.data.buffer));

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_write_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6);
        STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(instruction2.data.buffer));
        STRICT_EXPECTED_CALL(mocks, BUFFER_length(instruction2.data.buffer));

        STRICT_EXPECTED_CALL(mocks, on_write_complete(sequence, NULL, fake_char_id, WRITE_ONCE, BLEIO_SEQ_OK));

        ///act
        (void)BLEIO_Seq_AddInstruction(sequence, &instruction2);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLEIO_Seq_Destroy(sequence, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_042: [ When a WRITE_ONCE or a WRITE_AT_INIT instruction completes execution this API shall invoke the on_write_complete callback passing in the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_passes_context_for_write_at_init)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instruction, 1);
        auto sequence = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);
        (void)BLEIO_Seq_Run(sequence);

        // cause BLEIO_gatt_write_char_by_uuid to call callback with ok
        BLEIO_gatt_write_char_by_uuid_results.result = BLEIO_GATT_OK;

        BLEIO_SEQ_INSTRUCTION instruction2 =
        {
            WRITE_AT_INIT,
            STRING_construct("fake_char_id"),
            (void*)0x42,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };

        const char* fake_char_id = STRING_c_str(instruction2.characteristic_uuid);

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(instruction2.characteristic_uuid));

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_write_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6);
        STRICT_EXPECTED_CALL(mocks, on_write_complete(sequence, (void*)0x42, fake_char_id, WRITE_AT_INIT, BLEIO_SEQ_OK));

        ///act
        (void)BLEIO_Seq_AddInstruction(sequence, &instruction2);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLEIO_Seq_Destroy(sequence, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_042: [ When a WRITE_ONCE or a WRITE_AT_INIT instruction completes execution this API shall invoke the on_write_complete callback passing in the status of the operation and the callback context that was passed in via the BLEIO_SEQ_INSTRUCTION structure. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_041: [ When a WRITE_AT_INIT or a WRITE_ONCE instruction completes execution this API shall free the buffer that was passed in via the instruction. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_passes_context_for_write_once)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instruction, 1);
        auto sequence = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);
        (void)BLEIO_Seq_Run(sequence);

        // cause BLEIO_gatt_write_char_by_uuid to call callback with ok
        BLEIO_gatt_write_char_by_uuid_results.result = BLEIO_GATT_OK;

        BLEIO_SEQ_INSTRUCTION instruction2 =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            (void*)0x42,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };

        const char* fake_char_id = STRING_c_str(instruction2.characteristic_uuid);

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(instruction2.characteristic_uuid));

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, BLEIO_gatt_write_char_by_uuid((BLEIO_GATT_HANDLE)0x42, fake_char_id, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6);
        STRICT_EXPECTED_CALL(mocks, on_write_complete(sequence, (void*)0x42, fake_char_id, WRITE_ONCE, BLEIO_SEQ_OK));

        ///act
        (void)BLEIO_Seq_AddInstruction(sequence, &instruction2);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLEIO_Seq_Destroy(sequence, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_038: [ BLEIO_Seq_AddInstruction shall schedule execution of the instruction. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_schedules_write_at_exit_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instruction, 1);
        auto sequence = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);
        (void)BLEIO_Seq_Run(sequence);

        BLEIO_SEQ_INSTRUCTION instruction2 =
        {
            WRITE_AT_EXIT,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };

        const char* fake_char_id = STRING_c_str(instruction2.characteristic_uuid);

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction2.characteristic_uuid));

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        ///act
        (void)BLEIO_Seq_AddInstruction(sequence, &instruction2);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLEIO_Seq_Destroy(sequence, NULL, NULL);
    }

    /*Tests_SRS_BLEIO_SEQ_13_038: [ BLEIO_Seq_AddInstruction shall schedule execution of the instruction. ]*/
    /*Tests_SRS_BLEIO_SEQ_13_037: [ BLEIO_Seq_AddInstruction shall return BLEIO_SEQ_ERROR if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_Seq_AddInstruction_returns_error_when_VECTOR_push_back_fails_for_write_at_exit_instruction)
    {
        ///arrange
        CBLEIOSeqMocks mocks;
        VECTOR_HANDLE instructions = VECTOR_create(sizeof(BLEIO_SEQ_INSTRUCTION));
        BLEIO_SEQ_INSTRUCTION instruction =
        {
            WRITE_ONCE,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };
        VECTOR_push_back(instructions, &instruction, 1);
        auto sequence = BLEIO_Seq_Create((BLEIO_GATT_HANDLE)0x42, instructions, on_read_complete, on_write_complete);
        (void)BLEIO_Seq_Run(sequence);

        BLEIO_SEQ_INSTRUCTION instruction2 =
        {
            WRITE_AT_EXIT,
            STRING_construct("fake_char_id"),
            NULL,
            { .buffer = BUFFER_create((const unsigned char*)"data", 4) }
        };

        const char* fake_char_id = STRING_c_str(instruction2.characteristic_uuid);

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_length(instruction2.characteristic_uuid));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(instruction2.characteristic_uuid));

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .SetFailReturn((int)1);

        ///act
        (void)BLEIO_Seq_AddInstruction(sequence, &instruction2);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLEIO_Seq_Destroy(sequence, NULL, NULL);
        STRING_delete(instruction2.characteristic_uuid);
        BUFFER_delete(instruction2.data.buffer);
    }

END_TEST_SUITE(bleio_seq_unittests)
