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

#define TIME_IN_STRFTIME "time"

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
};

static bool g_call_on_read_complete = false;
static BLEIO_SEQ_RESULT g_read_result = BLEIO_SEQ_OK;

class CBLEIOSequence
{
public:
    BLEIO_GATT_HANDLE _bleio_gatt_handle;
    VECTOR_HANDLE _instructions;
    ON_BLEIO_SEQ_READ_COMPLETE _on_read_complete;
    ON_BLEIO_SEQ_WRITE_COMPLETE _on_write_complete;

public:
    CBLEIOSequence(
        BLEIO_GATT_HANDLE bleio_gatt_handle,
        VECTOR_HANDLE instructions,
        ON_BLEIO_SEQ_READ_COMPLETE on_read_complete,
        ON_BLEIO_SEQ_WRITE_COMPLETE on_write_complete
    ) :
        _bleio_gatt_handle(bleio_gatt_handle),
        _instructions(instructions),
        _on_read_complete(on_read_complete),
        _on_write_complete(on_write_complete)
    {}

    ~CBLEIOSequence()
    {
        if (_bleio_gatt_handle != NULL)
        {
            BLEIO_gatt_destroy(_bleio_gatt_handle);
        }

        if (_instructions != NULL)
        {
            for (size_t i = 0, len = VECTOR_size(_instructions); i < len; i++)
            {
                BLEIO_SEQ_INSTRUCTION *instruction = (BLEIO_SEQ_INSTRUCTION *)VECTOR_element(_instructions, i);
                if (instruction->characteristic_uuid != NULL)
                {
                    STRING_delete(instruction->characteristic_uuid);
                }
            }

            VECTOR_destroy(_instructions);
        }
    }

    BLEIO_SEQ_RESULT run()
    {
        size_t len = VECTOR_size(_instructions);
        if (g_call_on_read_complete && _on_read_complete != NULL && len > 0)
        {
            BLEIO_SEQ_INSTRUCTION* instr = (BLEIO_SEQ_INSTRUCTION*)VECTOR_element(_instructions, 0);
            unsigned char fake_data[] = "data";
            size_t data_size = sizeof(fake_data) / sizeof(fake_data[0]);

            _on_read_complete(
                (BLEIO_SEQ_HANDLE)this,
                instr->context,
                STRING_c_str(instr->characteristic_uuid),
                instr->instruction_type,
                g_read_result,
                BUFFER_create(fake_data, data_size)
            );
        }

        return g_read_result;
    }

    BLEIO_SEQ_RESULT add_instruction(BLEIO_SEQ_INSTRUCTION* instruction)
    {
        return BLEIO_SEQ_OK;
    }
};

TYPED_MOCK_CLASS(CBLEMocks, CGlobalMock)
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
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, time_t, gb_time, time_t *, timer)
        time_t result2 = (time_t)1; /*assume "1" is valid time_t*/
    MOCK_METHOD_END(time_t, result2);

    MOCK_STATIC_METHOD_1(, struct tm*, gb_localtime, const time_t *, timer)
        struct tm* result2 = (struct tm*)0x42;
    MOCK_METHOD_END(struct tm*, result2);

    MOCK_STATIC_METHOD_4(, size_t, gb_strftime, char*, s, size_t, maxsize, const char *, format, const struct tm *, timeptr)
        if (maxsize < strlen(TIME_IN_STRFTIME) + 1)
        {
            ASSERT_FAIL("what is this puny message size!");
        }
        else
        {
            strcpy(s, TIME_IN_STRFTIME);
        }
    MOCK_METHOD_END(size_t, maxsize);

    MOCK_STATIC_METHOD_1(, BLEIO_GATT_HANDLE, BLEIO_gatt_create, const BLE_DEVICE_CONFIG*, config)
        auto result2 = (BLEIO_GATT_HANDLE)malloc(1);
    MOCK_METHOD_END(BLEIO_GATT_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, void, BLEIO_gatt_destroy, BLEIO_GATT_HANDLE, bleio_gatt_handle)
        free(bleio_gatt_handle);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_3(, int, BLEIO_gatt_connect, BLEIO_GATT_HANDLE, bleio_gatt_handle, ON_BLEIO_GATT_CONNECT_COMPLETE, on_bleio_gatt_connect_complete, void*, callback_context)
        on_bleio_gatt_connect_complete(bleio_gatt_handle, callback_context, BLEIO_GATT_CONNECT_OK);
        int result2 = 0;
    MOCK_METHOD_END(int, result2)

    MOCK_STATIC_METHOD_3(, void, BLEIO_gatt_disconnect, BLEIO_GATT_HANDLE, bleio_gatt_handle, ON_BLEIO_GATT_DISCONNECT_COMPLETE, on_bleio_gatt_disconnect_complete, void*, callback_context)
        if (on_bleio_gatt_disconnect_complete != NULL)
        {
            on_bleio_gatt_disconnect_complete(bleio_gatt_handle, callback_context);
        }
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_4(, BLEIO_SEQ_HANDLE, BLEIO_Seq_Create, BLEIO_GATT_HANDLE, bleio_gatt_handle, VECTOR_HANDLE, instructions, ON_BLEIO_SEQ_READ_COMPLETE, on_read_complete, ON_BLEIO_SEQ_WRITE_COMPLETE, on_write_complete)
        auto result2 = (BLEIO_SEQ_HANDLE)new CBLEIOSequence(
            bleio_gatt_handle,
            instructions,
            on_read_complete,
            on_write_complete
        );
    MOCK_METHOD_END(BLEIO_SEQ_HANDLE, result2)

    MOCK_STATIC_METHOD_2(, BLEIO_SEQ_RESULT, BLEIO_Seq_AddInstruction, BLEIO_SEQ_HANDLE, bleio_seq_handle, BLEIO_SEQ_INSTRUCTION*, instruction)
        CBLEIOSequence* seq = (CBLEIOSequence*)bleio_seq_handle;
        auto result2 = seq->add_instruction(instruction);
    MOCK_METHOD_END(BLEIO_SEQ_RESULT, result2)

    MOCK_STATIC_METHOD_3(, void, BLEIO_Seq_Destroy, BLEIO_SEQ_HANDLE, bleio_seq_handle, ON_BLEIO_SEQ_DESTROY_COMPLETE, on_destroy_complete, void*, context)
        if (on_destroy_complete != NULL)
        {
            on_destroy_complete(bleio_seq_handle, context);
        }
        delete (CBLEIOSequence*)bleio_seq_handle;
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, BLEIO_SEQ_RESULT, BLEIO_Seq_Run, BLEIO_SEQ_HANDLE, bleio_seq_handle)
        CBLEIOSequence* seq = (CBLEIOSequence*)bleio_seq_handle;
        auto result2 = seq->run();
    MOCK_METHOD_END(BLEIO_SEQ_RESULT, result2)
};

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEMocks, , void, gballoc_free, void*, ptr);

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEMocks, , time_t, gb_time, time_t*, timer);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEMocks, , struct tm*, gb_localtime, const time_t*, timer);
DECLARE_GLOBAL_MOCK_METHOD_4(CBLEMocks, , size_t, gb_strftime, char*, s, size_t, maxsize, const char *, format, const struct tm *, timeptr);

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEMocks, , BLEIO_GATT_HANDLE, BLEIO_gatt_create, const BLE_DEVICE_CONFIG*, config);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEMocks, , void, BLEIO_gatt_destroy, BLEIO_GATT_HANDLE, bleio_gatt_handle);
DECLARE_GLOBAL_MOCK_METHOD_3(CBLEMocks, , int, BLEIO_gatt_connect, BLEIO_GATT_HANDLE, bleio_gatt_handle, ON_BLEIO_GATT_CONNECT_COMPLETE, on_bleio_gatt_connect_complete, void*, callback_context);
DECLARE_GLOBAL_MOCK_METHOD_3(CBLEMocks, , void, BLEIO_gatt_disconnect, BLEIO_GATT_HANDLE, bleio_gatt_handle, ON_BLEIO_GATT_DISCONNECT_COMPLETE, on_bleio_gatt_disconnect_complete, void*, callback_context);

DECLARE_GLOBAL_MOCK_METHOD_4(CBLEMocks, , BLEIO_SEQ_HANDLE, BLEIO_Seq_Create, BLEIO_GATT_HANDLE, bleio_gatt_handle, VECTOR_HANDLE, instructions, ON_BLEIO_SEQ_READ_COMPLETE, on_read_complete, ON_BLEIO_SEQ_WRITE_COMPLETE, on_write_complete);
DECLARE_GLOBAL_MOCK_METHOD_3(CBLEMocks, , void, BLEIO_Seq_Destroy, BLEIO_SEQ_HANDLE, bleio_seq_handle, ON_BLEIO_SEQ_DESTROY_COMPLETE, on_destroy_complete, void*, context);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEMocks, , BLEIO_SEQ_RESULT, BLEIO_Seq_Run, BLEIO_SEQ_HANDLE, bleio_seq_handle);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEMocks, , BLEIO_SEQ_RESULT, BLEIO_Seq_AddInstruction, BLEIO_SEQ_HANDLE, bleio_seq_handle, BLEIO_SEQ_INSTRUCTION*, instruction);

BEGIN_TEST_SUITE(ble_unittests)
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
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    if (!MicroMockReleaseMutex(g_testByTest))
    {
        ASSERT_FAIL("failure in test framework at ReleaseMutex");
    }
}

END_TEST_SUITE(ble_unittests)