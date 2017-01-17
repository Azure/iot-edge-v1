// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#include <cstddef>
#include <cstdbool>
#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/vector_types_internal.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/uniqueid.h"
#include "azure_c_shared_utility/xlogging.h"
#include "nanomsg/nn.h"
#include "nanomsg/pubsub.h"

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
#include "strings.c"
};

#include "broker.h"
#include "azure_c_shared_utility/lock.h"

DEFINE_MICROMOCK_ENUM_TO_STRING(BROKER_RESULT, BROKER_RESULT_VALUES);

static size_t currentmalloc_call;
static size_t whenShallmalloc_fail;

static size_t currentVECTOR_create_call;
static size_t whenShallVECTOR_create_fail;

static size_t currentVECTOR_push_back_call;
static size_t whenShallVECTOR_push_back_fail;

static size_t currentVECTOR_find_if_call;
static size_t whenShallVECTOR_find_if_fail;

static size_t currentsinglylinkedlist_find_call;
static size_t whenShallsinglylinkedlist_find_fail;

static size_t currentsinglylinkedlist_create_call;
static size_t whenShallsinglylinkedlist_create_fail;

static size_t currentsinglylinkedlist_add_call;
static size_t whenShallsinglylinkedlist_add_fail;


static size_t currentLock_Init_call;
static size_t whenShallLock_Init_fail;

static size_t currentLock_call;
static size_t whenShallLock_fail;

static size_t currentUnlock_call;

static size_t currentCond_Init_call;
static size_t whenShallCond_Init_fail;

static size_t currentCond_Post_call;
static size_t whenShallCond_Post_fail;

static size_t currentThreadAPI_Create_call;
static size_t whenShallThreadAPI_Create_fail;

static size_t nn_current_msg_size;

typedef struct LIST_ITEM_INSTANCE_TAG
{
    const void* item;
    void* next;
} LIST_ITEM_INSTANCE;

typedef struct LIST_INSTANCE_TAG
{
    LIST_ITEM_INSTANCE* head;
} LIST_INSTANCE;

struct ListNode
{
    const void* item;
    ListNode *next, *prev;
};

static int current_nn_socket_index;
static void* nn_socket_memory[10];

static THREAD_START_FUNC thread_func_to_call;
static void* thread_func_args;

struct FakeModule_Receive_Call_Status
{
    MODULE_HANDLE module;
    MESSAGE_HANDLE messageHandle;
    bool was_called;
};
static FakeModule_Receive_Call_Status call_status_for_FakeModule_Receive;

static MODULE_HANDLE fake_module_handle = (MODULE_HANDLE)0x42;

static MODULE_HANDLE FakeModule_Create(BROKER_HANDLE broker, const void* configuration)
{
    return (MODULE_HANDLE)malloc(1);
}
static void FakeModule_Destroy(MODULE_HANDLE module)
{
    free(module);
}

static void FakeModule_Receive(MODULE_HANDLE module, MESSAGE_HANDLE messageHandle)
{
    call_status_for_FakeModule_Receive.was_called = true;
    ASSERT_ARE_EQUAL(void_ptr, module, call_status_for_FakeModule_Receive.module);
}

static MODULE_API_1 fake_module_apis =
{
    { MODULE_API_VERSION_1 },
    NULL,
	NULL,
    FakeModule_Create,
    FakeModule_Destroy,
    FakeModule_Receive,
    NULL
};

MODULE fake_module =
{
    (const MODULE_API *)&fake_module_apis,
    fake_module_handle
};

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

TYPED_MOCK_CLASS(CBrokerMocks, CGlobalMock)
{
public:

    MOCK_STATIC_METHOD_1(, void*, gballoc_malloc, size_t, size)
        void* result2;
        currentmalloc_call++;
        if (currentmalloc_call == whenShallmalloc_fail)
        {
            result2 = NULL;
        }
        else
        {
            result2 = BASEIMPLEMENTATION::gballoc_malloc(size);
        }
    MOCK_METHOD_END(void*, result2);

    MOCK_STATIC_METHOD_1(, void, gballoc_free, void*, ptr)
        BASEIMPLEMENTATION::gballoc_free(ptr);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_0(, LOCK_HANDLE, Lock_Init)
        LOCK_HANDLE result2;
        ++currentLock_Init_call;
        if ((whenShallLock_Init_fail > 0) &&
            (currentLock_Init_call == whenShallLock_Init_fail))
        {
            result2 = NULL;
        }
        else
        {
            result2 = (LOCK_HANDLE)malloc(1);
        }
    MOCK_METHOD_END(LOCK_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, LOCK_RESULT, Lock, LOCK_HANDLE, lock)
        LOCK_RESULT result2;
        ++currentLock_call;
        if ((whenShallLock_fail > 0) &&
            (currentLock_call == whenShallLock_fail))
        {
            result2 = LOCK_ERROR;
        }
        else
        {
            result2 = LOCK_OK;
        }
    MOCK_METHOD_END(LOCK_RESULT, result2)

    MOCK_STATIC_METHOD_1(, LOCK_RESULT, Unlock, LOCK_HANDLE, lock)
        ASSERT_IS_TRUE((currentLock_call - currentUnlock_call) > 0);
        ++currentUnlock_call;
        auto result2 = LOCK_OK;
    MOCK_METHOD_END(LOCK_RESULT, result2)

    MOCK_STATIC_METHOD_1(, LOCK_RESULT, Lock_Deinit, LOCK_HANDLE, lock)
        free(lock);
        auto result2 = LOCK_OK;
    MOCK_METHOD_END(LOCK_RESULT, result2)

    MOCK_STATIC_METHOD_1(, VECTOR_HANDLE, VECTOR_create, size_t, elementSize)
        VECTOR_HANDLE result2;
        ++currentVECTOR_create_call;
        if ((whenShallVECTOR_create_fail > 0) &&
            (currentVECTOR_create_call == whenShallVECTOR_create_fail))
        {
            result2 = NULL;
        }
        else
        {
            result2 = BASEIMPLEMENTATION::VECTOR_create(elementSize);
        }
    MOCK_METHOD_END(VECTOR_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, void, VECTOR_destroy, VECTOR_HANDLE, vector)
        BASEIMPLEMENTATION::VECTOR_destroy(vector);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_3(, int, VECTOR_push_back, VECTOR_HANDLE, vector, const void*, elements, size_t, numElements)
        int result2;
        ++currentVECTOR_push_back_call;
        if ((whenShallVECTOR_push_back_fail > 0) &&
            (currentVECTOR_push_back_call == whenShallVECTOR_push_back_fail))
        {
            result2 = __LINE__;
        }
        else
        {
            result2 = BASEIMPLEMENTATION::VECTOR_push_back(vector, elements, numElements);
        }
    MOCK_METHOD_END(int, result2)

    MOCK_STATIC_METHOD_3(, void, VECTOR_erase, VECTOR_HANDLE, vector, void*, elements, size_t, numElements)
        BASEIMPLEMENTATION::VECTOR_erase(vector, elements, numElements);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_2(, void*, VECTOR_element, VECTOR_HANDLE, vector, size_t, index)
        void* result2 = BASEIMPLEMENTATION::VECTOR_element(vector, index);
    MOCK_METHOD_END(void*, result2)

    MOCK_STATIC_METHOD_1(, void*, VECTOR_front, VECTOR_HANDLE, vector)
        void* result2 = BASEIMPLEMENTATION::VECTOR_front(vector);
    MOCK_METHOD_END(void*, result2)

    MOCK_STATIC_METHOD_1(, void*, VECTOR_back, const VECTOR_HANDLE, vector)
        void* result2 = BASEIMPLEMENTATION::VECTOR_back(vector);
    MOCK_METHOD_END(void*, result2)

    MOCK_STATIC_METHOD_3(, void*, VECTOR_find_if, VECTOR_HANDLE, vector, PREDICATE_FUNCTION, pred, const void*, value)
        void* result2;
        ++currentVECTOR_find_if_call;
        if ((whenShallVECTOR_find_if_fail > 0) &&
            (currentVECTOR_find_if_call == whenShallVECTOR_find_if_fail))
        {
            result2 = NULL;
        }
        else
        {
            result2 = BASEIMPLEMENTATION::VECTOR_find_if(vector, pred, value);
        }
    MOCK_METHOD_END(void*, result2)

    MOCK_STATIC_METHOD_1(, size_t, VECTOR_size, VECTOR_HANDLE, vector)
        size_t result2 = BASEIMPLEMENTATION::VECTOR_size(vector);
    MOCK_METHOD_END(size_t, result2)

    MOCK_STATIC_METHOD_3(, THREADAPI_RESULT, ThreadAPI_Create, THREAD_HANDLE*, threadHandle, THREAD_START_FUNC, func, void*, arg)
        THREADAPI_RESULT result2;
        ++currentThreadAPI_Create_call;
        if ((whenShallThreadAPI_Create_fail > 0) &&
            (currentThreadAPI_Create_call == whenShallThreadAPI_Create_fail))
        {
            result2 = THREADAPI_ERROR;
        }
        else
        {
            *threadHandle = (THREAD_HANDLE*)malloc(3);
            thread_func_to_call = func;
            thread_func_args = arg;

            result2 = THREADAPI_OK;
        }
    MOCK_METHOD_END(THREADAPI_RESULT, result2)

    MOCK_STATIC_METHOD_2(, THREADAPI_RESULT, ThreadAPI_Join, THREAD_HANDLE, threadHandle, int*, res)
        free(threadHandle);
        auto result2 = THREADAPI_OK;
    MOCK_METHOD_END(THREADAPI_RESULT, result2)

    MOCK_STATIC_METHOD_1(, MESSAGE_HANDLE, Message_Create, const MESSAGE_CONFIG*, cfg)
        MESSAGE_HANDLE result2 = (MESSAGE_HANDLE)(new RefCountObject());
    MOCK_METHOD_END(MESSAGE_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, MESSAGE_HANDLE, Message_Clone, MESSAGE_HANDLE, message)
        ((RefCountObject*)message)->inc_ref();
    MOCK_METHOD_END(MESSAGE_HANDLE, message)

    MOCK_STATIC_METHOD_1(, void, Message_Destroy, MESSAGE_HANDLE, message)
        ((RefCountObject*)message)->dec_ref();
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_2(, MESSAGE_HANDLE, Message_CreateFromByteArray, const unsigned char*, source, int32_t, size)
    MOCK_METHOD_END(MESSAGE_HANDLE, (MESSAGE_HANDLE)(new RefCountObject()))

    MOCK_STATIC_METHOD_3(, int32_t, Message_ToByteArray, MESSAGE_HANDLE, messageHandle, unsigned char *, buffer, int32_t, size)
    MOCK_METHOD_END(int32_t, (int32_t)1)

    // list.h

    MOCK_STATIC_METHOD_0(, SINGLYLINKEDLIST_HANDLE, singlylinkedlist_create)
        ++currentsinglylinkedlist_create_call;
        SINGLYLINKEDLIST_HANDLE result1;
        if (currentsinglylinkedlist_create_call == whenShallsinglylinkedlist_create_fail)
        {
            result1 = NULL;
        }
        else
        {
            result1 = (SINGLYLINKEDLIST_HANDLE)new ListNode();
        }
    MOCK_METHOD_END(SINGLYLINKEDLIST_HANDLE, result1)

    MOCK_STATIC_METHOD_1(, void, singlylinkedlist_destroy, SINGLYLINKEDLIST_HANDLE, list)
        ListNode *current = (ListNode*)list;
        while (current != nullptr)
        {
            auto tmp = current;
            current = current->next;
            delete tmp;
        }
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_2(, LIST_ITEM_HANDLE, singlylinkedlist_add, SINGLYLINKEDLIST_HANDLE, list, const void*, item)
        LIST_ITEM_HANDLE result1;
        ++currentsinglylinkedlist_add_call;
        if (currentsinglylinkedlist_add_call == whenShallsinglylinkedlist_add_fail)
        {
            result1 = NULL;
        }
        else
        {
            if ((list == NULL) ||
                (item == NULL))
            {
                result1 = NULL;
            }
            else
            {
                ListNode *new_item = new ListNode();
                new_item->item = item;
                ListNode *real_list = (ListNode*)list;
                ListNode *ptr = real_list;
                while (ptr->next != nullptr)
                    ptr = ptr->next;
                ptr->next = new_item;
                new_item->prev = ptr;
                result1 = (LIST_ITEM_HANDLE)new_item;
            }
        }
    MOCK_METHOD_END(LIST_ITEM_HANDLE, result1)

    MOCK_STATIC_METHOD_2(, int, singlylinkedlist_remove, SINGLYLINKEDLIST_HANDLE, list, LIST_ITEM_HANDLE, item)
        int result2;
        if ((list == NULL) ||
            (item == NULL))
        {
            result2 = __LINE__;
        }
        else
        {
            ListNode *node = (ListNode*)item;
            node->prev->next = node->next;
            if (node->next != nullptr)
                node->next->prev = node->prev;
            delete node;
            result = 0;
        }

    MOCK_METHOD_END(int, result2)

    MOCK_STATIC_METHOD_1(, LIST_ITEM_HANDLE, singlylinkedlist_get_head_item, SINGLYLINKEDLIST_HANDLE, list)
        LIST_ITEM_HANDLE result1;
        if (list == NULL)
        {
            result1 = NULL;
        }
        else
        {
            ListNode* my_result = NULL;
            result1 = (LIST_ITEM_HANDLE)(((ListNode*)list)->next);
        }
    MOCK_METHOD_END(LIST_ITEM_HANDLE, result1)

    MOCK_STATIC_METHOD_1(, LIST_ITEM_HANDLE, singlylinkedlist_get_next_item, LIST_ITEM_HANDLE, item_handle)
        LIST_ITEM_HANDLE result1;
        if (item_handle == NULL)
        {
            result1 = NULL;
        }
        else
        {
            result1 = (LIST_ITEM_HANDLE)(((ListNode*)item_handle)->next);
        }
    MOCK_METHOD_END(LIST_ITEM_HANDLE, result1)

    MOCK_STATIC_METHOD_3(, LIST_ITEM_HANDLE, singlylinkedlist_find, SINGLYLINKEDLIST_HANDLE, list, LIST_MATCH_FUNCTION, match_function, const void*, match_context)
        LIST_ITEM_HANDLE result1;
        currentsinglylinkedlist_find_call++;
        if (currentsinglylinkedlist_find_call == whenShallsinglylinkedlist_find_fail)
        {
            result1 = NULL;
        }
        else
        {
            if ((list == NULL) ||
                (match_function == NULL))
            {
                result1 = NULL;
            }
            else
            {
                result1 = (LIST_ITEM_HANDLE)(((ListNode*)list)->next);
                while (result1)
                {
                    if (match_function(result1, match_context))
                    {
                        break;
                    }
                    result1 = (LIST_ITEM_HANDLE)(((ListNode*)result1)->next);
                }
            }
        }
    MOCK_METHOD_END(LIST_ITEM_HANDLE, result1)

    MOCK_STATIC_METHOD_1(, const void*, singlylinkedlist_item_get_value, LIST_ITEM_HANDLE, item_handle)
        const void* result1;
        if (item_handle == NULL)
        {
            result1 = NULL;
        }
        else
        {
            result1 = (LIST_ITEM_HANDLE)(((ListNode*)item_handle)->item);
        }
    MOCK_METHOD_END(const void*, result1)

    MOCK_STATIC_METHOD_1(, void, STRING_delete, STRING_HANDLE, s)
        BASEIMPLEMENTATION::STRING_delete(s);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, STRING_HANDLE, STRING_construct, const char*, source)
        STRING_HANDLE result2;
        result2 = BASEIMPLEMENTATION::STRING_construct(source);
    MOCK_METHOD_END(STRING_HANDLE, result2)

    MOCK_STATIC_METHOD_2(, int, STRING_concat, STRING_HANDLE, s1, const char*, s2)
        int result1 = BASEIMPLEMENTATION::STRING_concat(s1, s2);
    MOCK_METHOD_END(int, result1);

    MOCK_STATIC_METHOD_1(, const char*, STRING_c_str, STRING_HANDLE, s)
    MOCK_METHOD_END(const char*, BASEIMPLEMENTATION::STRING_c_str(s))

    MOCK_STATIC_METHOD_1(, size_t, STRING_length, STRING_HANDLE, s)
    MOCK_METHOD_END(size_t, BASEIMPLEMENTATION::STRING_length(s))

    MOCK_STATIC_METHOD_2(, UNIQUEID_RESULT, UniqueId_Generate, char*, uid, size_t, bufferSize)
        for (int i = 0; i < (int)bufferSize; i++)
        {
            if (i == (bufferSize - 1))
                uid[i] = '\0';
            else
                uid[i] = 'u';
        }
    MOCK_METHOD_END(UNIQUEID_RESULT, UNIQUEID_OK)


    MOCK_STATIC_METHOD_2(, void *, nn_allocmsg, size_t, size, int, type)
        nn_current_msg_size = size;
    MOCK_METHOD_END(void*, malloc(size))

    MOCK_STATIC_METHOD_1(, int, nn_freemsg, void*, msg)
        free(msg);
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_2(, int, nn_socket, int, domain, int, protocol)
        current_nn_socket_index++;
        nn_socket_memory[current_nn_socket_index] = malloc(1);
    MOCK_METHOD_END(int, current_nn_socket_index)

    MOCK_STATIC_METHOD_1(, int, nn_close, int, s)
        free(nn_socket_memory[s]);
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_2(, int, nn_bind, int, s, const char *, addr)
    MOCK_METHOD_END(int, 1)

    MOCK_STATIC_METHOD_2(, int, nn_connect, int, s, const char *, addr)
    MOCK_METHOD_END(int, 1)

    MOCK_STATIC_METHOD_5(, int, nn_setsockopt, int, s, int, level, int, option, const void *, optval, size_t, optvallen)
    MOCK_METHOD_END(int, 1)


    MOCK_STATIC_METHOD_4(, int, nn_send, int, s, const void*, buf, size_t, len, int, flags)
        int send_length;
        if (len == NN_MSG)
        {
            send_length = (int)nn_current_msg_size;
            free(*(void**)buf); // send is supposed to free auto created buffer on success
        }
        else
        {
            send_length = len;
        }
    MOCK_METHOD_END(int, send_length)

    MOCK_STATIC_METHOD_4(, int, nn_recv, int, s, void*, buf, size_t, len, int, flags)
        int rcv_length;
        if (len == NN_MSG)
        {
            char * text = (char*)"nn_recv";
            (*(void**)buf) = malloc(8);
            memcpy((*(void**)buf), text, 8);
            rcv_length = 8;
        }
        else
        {
            rcv_length = (int)len;
        }
    MOCK_METHOD_END(int, rcv_length)
};

DECLARE_GLOBAL_MOCK_METHOD_1(CBrokerMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CBrokerMocks, , void, gballoc_free, void*, ptr);

DECLARE_GLOBAL_MOCK_METHOD_0(CBrokerMocks, , LOCK_HANDLE, Lock_Init);
DECLARE_GLOBAL_MOCK_METHOD_1(CBrokerMocks, , LOCK_RESULT, Lock, LOCK_HANDLE, lock);
DECLARE_GLOBAL_MOCK_METHOD_1(CBrokerMocks, , LOCK_RESULT, Unlock, LOCK_HANDLE, lock);
DECLARE_GLOBAL_MOCK_METHOD_1(CBrokerMocks, , LOCK_RESULT, Lock_Deinit, LOCK_HANDLE, lock);

DECLARE_GLOBAL_MOCK_METHOD_1(CBrokerMocks, , VECTOR_HANDLE, VECTOR_create, size_t, elementSize);
DECLARE_GLOBAL_MOCK_METHOD_1(CBrokerMocks, , void, VECTOR_destroy, VECTOR_HANDLE, vector);
DECLARE_GLOBAL_MOCK_METHOD_3(CBrokerMocks, , int, VECTOR_push_back, VECTOR_HANDLE, vector, const void*, elements, size_t, numElements);
DECLARE_GLOBAL_MOCK_METHOD_3(CBrokerMocks, , void, VECTOR_erase, VECTOR_HANDLE, vector, void*, elements, size_t, numElements);
DECLARE_GLOBAL_MOCK_METHOD_2(CBrokerMocks, , void*, VECTOR_element, VECTOR_HANDLE, vector, size_t, index);
DECLARE_GLOBAL_MOCK_METHOD_1(CBrokerMocks, , void*, VECTOR_front, VECTOR_HANDLE, vector);
DECLARE_GLOBAL_MOCK_METHOD_1(CBrokerMocks, , void*, VECTOR_back, VECTOR_HANDLE, vector);
DECLARE_GLOBAL_MOCK_METHOD_3(CBrokerMocks, , void*, VECTOR_find_if, VECTOR_HANDLE, vector, PREDICATE_FUNCTION, pred, const void*, value);
DECLARE_GLOBAL_MOCK_METHOD_1(CBrokerMocks, , size_t, VECTOR_size, VECTOR_HANDLE, vector);

DECLARE_GLOBAL_MOCK_METHOD_3(CBrokerMocks, , THREADAPI_RESULT, ThreadAPI_Create, THREAD_HANDLE*, threadHandle, THREAD_START_FUNC, func, void*, arg);
DECLARE_GLOBAL_MOCK_METHOD_2(CBrokerMocks, , THREADAPI_RESULT, ThreadAPI_Join, THREAD_HANDLE, threadHandle, int*, res);

DECLARE_GLOBAL_MOCK_METHOD_1(CBrokerMocks, , MESSAGE_HANDLE, Message_Create, const MESSAGE_CONFIG*, cfg);
DECLARE_GLOBAL_MOCK_METHOD_1(CBrokerMocks, , MESSAGE_HANDLE, Message_Clone, MESSAGE_HANDLE, message);
DECLARE_GLOBAL_MOCK_METHOD_1(CBrokerMocks, , void, Message_Destroy, MESSAGE_HANDLE, message);
DECLARE_GLOBAL_MOCK_METHOD_2(CBrokerMocks, , MESSAGE_HANDLE, Message_CreateFromByteArray, const unsigned char*, source, int32_t, size);
DECLARE_GLOBAL_MOCK_METHOD_3(CBrokerMocks, , int32_t, Message_ToByteArray, MESSAGE_HANDLE, messageHandle, unsigned char *, buffer, int32_t, size);

// singlylinkedlist.h
DECLARE_GLOBAL_MOCK_METHOD_0(CBrokerMocks, , SINGLYLINKEDLIST_HANDLE, singlylinkedlist_create);
DECLARE_GLOBAL_MOCK_METHOD_1(CBrokerMocks, , void, singlylinkedlist_destroy, SINGLYLINKEDLIST_HANDLE, list);
DECLARE_GLOBAL_MOCK_METHOD_2(CBrokerMocks, , LIST_ITEM_HANDLE, singlylinkedlist_add, SINGLYLINKEDLIST_HANDLE, list, const void*, item);
DECLARE_GLOBAL_MOCK_METHOD_2(CBrokerMocks, , int, singlylinkedlist_remove, SINGLYLINKEDLIST_HANDLE, list, LIST_ITEM_HANDLE, item_handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CBrokerMocks, , LIST_ITEM_HANDLE, singlylinkedlist_get_head_item, SINGLYLINKEDLIST_HANDLE, list);
DECLARE_GLOBAL_MOCK_METHOD_1(CBrokerMocks, , LIST_ITEM_HANDLE, singlylinkedlist_get_next_item, LIST_ITEM_HANDLE, item_handle);
DECLARE_GLOBAL_MOCK_METHOD_3(CBrokerMocks, , LIST_ITEM_HANDLE, singlylinkedlist_find, SINGLYLINKEDLIST_HANDLE, list, LIST_MATCH_FUNCTION, match_function, const void*, match_context);
DECLARE_GLOBAL_MOCK_METHOD_1(CBrokerMocks, , const void*, singlylinkedlist_item_get_value, LIST_ITEM_HANDLE, item_handle);

//strings.h
DECLARE_GLOBAL_MOCK_METHOD_1(CBrokerMocks, , void, STRING_delete, STRING_HANDLE, s)
DECLARE_GLOBAL_MOCK_METHOD_1(CBrokerMocks, , STRING_HANDLE, STRING_construct, const char*, source)
DECLARE_GLOBAL_MOCK_METHOD_2(CBrokerMocks, , int, STRING_concat, STRING_HANDLE, s1, const char*, s2)
DECLARE_GLOBAL_MOCK_METHOD_1(CBrokerMocks, , const char*, STRING_c_str, STRING_HANDLE, s)
DECLARE_GLOBAL_MOCK_METHOD_1(CBrokerMocks, , size_t, STRING_length, STRING_HANDLE, s)

// unique id
DECLARE_GLOBAL_MOCK_METHOD_2(CBrokerMocks, , UNIQUEID_RESULT, UniqueId_Generate, char*, uid, size_t, bufferSize)
DECLARE_GLOBAL_MOCK_METHOD_2(CBrokerMocks, , void *, nn_allocmsg, size_t, size, int, type)
// nn.h
DECLARE_GLOBAL_MOCK_METHOD_1(CBrokerMocks, , int, nn_freemsg, void*, msg)
DECLARE_GLOBAL_MOCK_METHOD_2(CBrokerMocks, , int, nn_socket, int, domain, int, protocol)
DECLARE_GLOBAL_MOCK_METHOD_1(CBrokerMocks, , int, nn_close, int, s)
DECLARE_GLOBAL_MOCK_METHOD_2(CBrokerMocks, , int, nn_bind, int, s, const char *, addr)
DECLARE_GLOBAL_MOCK_METHOD_5(CBrokerMocks, , int, nn_setsockopt, int, s, int, level, int, option, const void *, optval, size_t, optvallen)
DECLARE_GLOBAL_MOCK_METHOD_2(CBrokerMocks, , int, nn_connect, int, s, const char *, addr)
DECLARE_GLOBAL_MOCK_METHOD_4(CBrokerMocks, , int, nn_send, int, s, const void*, buf, size_t, len, int, flags)
DECLARE_GLOBAL_MOCK_METHOD_4(CBrokerMocks, , int, nn_recv, int, s, void*, buf, size_t, len, int, flags)

BEGIN_TEST_SUITE(broker_ut)

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

    currentmalloc_call = 0;
    whenShallmalloc_fail = 0;

    currentVECTOR_create_call = 0;
    whenShallVECTOR_create_fail = 0;

    currentVECTOR_push_back_call = 0;
    whenShallVECTOR_push_back_fail = 0;

    currentVECTOR_find_if_call = 0;
    whenShallVECTOR_find_if_fail = 0;

    currentLock_Init_call = 0;
    whenShallLock_Init_fail = 0;

    currentsinglylinkedlist_find_call = 0;
    whenShallsinglylinkedlist_find_fail = 0;

    currentsinglylinkedlist_create_call = 0;
    whenShallsinglylinkedlist_create_fail = 0;

    currentsinglylinkedlist_add_call = 0;
    whenShallsinglylinkedlist_add_fail = 0;

    currentLock_call = 0;
    whenShallLock_fail = 0;

    currentUnlock_call = 0;

    currentCond_Init_call = 0;
    whenShallCond_Init_fail = 0;

    currentCond_Post_call = 0;
    whenShallCond_Post_fail = 0;

    currentThreadAPI_Create_call = 0;
    whenShallThreadAPI_Create_fail = 0;

    current_nn_socket_index = 0;
    for (int l = 0; l < 10; l++)
    {
        nn_socket_memory[l] = NULL;
    }

    nn_current_msg_size = 0;

    thread_func_to_call = NULL;
    thread_func_args = NULL;


    call_status_for_FakeModule_Receive.messageHandle = NULL;
    call_status_for_FakeModule_Receive.module = NULL;
    call_status_for_FakeModule_Receive.was_called = false;
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    if (!MicroMockReleaseMutex(g_testByTest))
    {
        ASSERT_FAIL("failure in test framework at ReleaseMutex");
    }
}

//Tests_SRS_BROKER_13_001: [This API shall yield a BROKER_HANDLE representing the newly created message broker. This handle value shall not be equal to NULL when the API call is successful.]
//Tests_SRS_BROKER_13_007: [Broker_Create shall initialize BROKER_HANDLE_DATA::modules with a valid VECTOR_HANDLE.]
//Tests_SRS_BROKER_13_023: [Broker_Create shall initialize BROKER_HANDLE_DATA::modules_lock with a valid LOCK_HANDLE.]
//Tests_SRS_BROKER_17_001: [ Broker_Create shall initialize a socket for publishing messages. ]
//Tests_SRS_BROKER_17_002: [ Broker_Create shall create a unique id. ]
//Tests_SRS_BROKER_17_003: [ Broker_Create shall initialize a url consisting of "inproc://" + unique id. ]
//Tests_SRS_BROKER_17_004: [ Broker_Create shall bind the socket to the BROKER_HANDLE_DATA::url. ]
TEST_FUNCTION(Broker_Create_succeeds)
{
    ///arrange
    CBrokerMocks mocks;

    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_create());
    STRICT_EXPECTED_CALL(mocks, Lock_Init());
    STRICT_EXPECTED_CALL(mocks, nn_socket(AF_SP, NN_PUB));
    STRICT_EXPECTED_CALL(mocks, UniqueId_Generate(IGNORED_PTR_ARG, 37))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_construct("inproc://"));
    STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, nn_bind(IGNORED_NUM_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    ///act
    auto r = Broker_Create();

    ///assert
    ASSERT_IS_NOT_NULL(r);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_Destroy(r);
}

//Tests_SRS_BROKER_13_003: [This function shall return NULL if an underlying API call to the platform causes an error.]
/*Tests_SRS_BROKER_13_067: [ Broker_Create shall malloc a new instance of BROKER_HANDLE_DATA and return NULL if it fails. ]*/
TEST_FUNCTION(Broker_Create_fails_when_malloc_fails)
{
    ///arrange

    CBrokerMocks mocks;
    whenShallmalloc_fail = 1;
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
        .IgnoreArgument(1);

    ///act
    auto r = Broker_Create();

    ///assert
    ASSERT_IS_NULL(r);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
}

//Tests_SRS_BROKER_13_003: [This function shall return NULL if an underlying API call to the platform causes an error.]
TEST_FUNCTION(Broker_Create_fails_when_singlylinkedlist_create_fails)
{
    ///arrange

    CBrokerMocks mocks;
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    whenShallsinglylinkedlist_create_fail = 1;
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_create());

    ///act
    auto r = Broker_Create();

    ///assert
    ASSERT_IS_NULL(r);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
}

//Tests_SRS_BROKER_13_003: [This function shall return NULL if an underlying API call to the platform causes an error.]
TEST_FUNCTION(Broker_Create_fails_when_Lock_Init_fails)
{
    ///arrange

    CBrokerMocks mocks;
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_create());
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    whenShallLock_Init_fail = 1;
    STRICT_EXPECTED_CALL(mocks, Lock_Init());

    ///act
    auto r = Broker_Create();

    ///assert
    ASSERT_IS_NULL(r);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
}


//Tests_SRS_BROKER_13_003: [This function shall return NULL if an underlying API call to the platform causes an error.]
TEST_FUNCTION(Broker_Create_fails_when_socket_fails)
{
    ///arrange
    CBrokerMocks mocks;

    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_create());
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock_Init());
    STRICT_EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_socket(AF_SP, NN_PUB))
        .SetFailReturn(-1);

    ///act
    auto r = Broker_Create();

    ///assert
    ASSERT_IS_NULL(r);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_Destroy(r);
}


//Tests_SRS_BROKER_13_003: [This function shall return NULL if an underlying API call to the platform causes an error.]
TEST_FUNCTION(Broker_Create_fails_when_uuid_fails)
{
    ///arrange
    CBrokerMocks mocks;

    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_create());
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock_Init());
    STRICT_EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_socket(AF_SP, NN_PUB));
    STRICT_EXPECTED_CALL(mocks, nn_close(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, UniqueId_Generate(IGNORED_PTR_ARG, 37))
        .IgnoreArgument(1)
        .SetFailReturn(UNIQUEID_ERROR);

    ///act
    auto r = Broker_Create();

    ///assert
    ASSERT_IS_NULL(r);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_Destroy(r);
}


//Tests_SRS_BROKER_13_003: [This function shall return NULL if an underlying API call to the platform causes an error.]
TEST_FUNCTION(Broker_Create_fails_when_url_create_fails)
{
    ///arrange
    CBrokerMocks mocks;

    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_create());
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock_Init());
    STRICT_EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_socket(AF_SP, NN_PUB));
    STRICT_EXPECTED_CALL(mocks, nn_close(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, UniqueId_Generate(IGNORED_PTR_ARG, 37))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_construct("inproc://"))
        .SetFailReturn((STRING_HANDLE)NULL);


    ///act
    auto r = Broker_Create();

    ///assert
    ASSERT_IS_NULL(r);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_Destroy(r);
}

//Tests_SRS_BROKER_13_003: [This function shall return NULL if an underlying API call to the platform causes an error.]
TEST_FUNCTION(Broker_Create_fails_when_url_concat_fails)
{
    ///arrange
    CBrokerMocks mocks;

    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_create());
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock_Init());
    STRICT_EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_socket(AF_SP, NN_PUB));
    STRICT_EXPECTED_CALL(mocks, nn_close(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, UniqueId_Generate(IGNORED_PTR_ARG, 37))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_construct("inproc://"));
    STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(1);

    ///act
    auto r = Broker_Create();

    ///assert
    ASSERT_IS_NULL(r);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_Destroy(r);
}

//Tests_SRS_BROKER_13_003: [This function shall return NULL if an underlying API call to the platform causes an error.]
TEST_FUNCTION(Broker_Create_fails_when_bind_fails)
{
    ///arrange
    CBrokerMocks mocks;

    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_create());
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock_Init());
    STRICT_EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_socket(AF_SP, NN_PUB));
    STRICT_EXPECTED_CALL(mocks, nn_close(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, UniqueId_Generate(IGNORED_PTR_ARG, 37))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_construct("inproc://"));
    STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, nn_bind(IGNORED_NUM_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1);
    STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    ///act
    auto r = Broker_Create();

    ///assert
    ASSERT_IS_NULL(r);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_Destroy(r);
}

//Tests_SRS_BROKER_99_013: [ If broker or module is NULL the function shall return BROKER_INVALIDARG. ]
TEST_FUNCTION(Broker_AddModule_fails_with_null_broker)
{
    ///arrange
    CBrokerMocks mocks;

    ///act
    auto result = Broker_AddModule(NULL, (MODULE*)0x1);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_INVALIDARG);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
}

//Tests_SRS_BROKER_99_013: [ If broker or module is NULL the function shall return BROKER_INVALIDARG. ]
TEST_FUNCTION(Broker_AddModule_fails_with_null_module)
{
    ///arrange
    CBrokerMocks mocks;

    ///act
    auto result = Broker_AddModule((BROKER_HANDLE)0x1, (MODULE*)NULL);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_INVALIDARG);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
}

//Tests_SRS_BROKER_99_014: [ If module_handle or module_apis are NULL the function shall return BROKER_INVALIDARG. ]
TEST_FUNCTION(Broker_AddModule_fails_with_null_module_apis)
{
    ///arrange
    CBrokerMocks mocks;

    MODULE module =
    {
        NULL,
        fake_module_handle
    };

    ///act
    auto result = Broker_AddModule((BROKER_HANDLE)0x1, &module);


    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_INVALIDARG);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
}

//Tests_SRS_BROKER_99_014: [ If module_handle or module_apis are NULL the function shall return BROKER_INVALIDARG. ]
TEST_FUNCTION(Broker_AddModule_fails_with_null_module_handle)
{
    ///arrange
    CBrokerMocks mocks;

    MODULE module =
    {
        (const MODULE_API *)&fake_module_apis,
        NULL
    };

    ///act
    auto result = Broker_AddModule((BROKER_HANDLE)0x1, &module);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_INVALIDARG);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
}
//Tests_SRS_BROKER_13_047: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]
TEST_FUNCTION(Broker_AddModule_fails_when_alloc_module_info_fails)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    mocks.ResetAllCalls();



    // this is for the Broker_AddModule call
    whenShallmalloc_fail = currentmalloc_call + 1;
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module_info*/
        .IgnoreArgument(1);

    ///act
    auto result = Broker_AddModule(broker, &fake_module);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_ERROR);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_13_047: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]
TEST_FUNCTION(Broker_AddModule_fails_when_module_struct_alloc_fails)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    mocks.ResetAllCalls();

    // this is for the Broker_AddModule call
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module_info*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module struct*/
        .IgnoreArgument(1)
        .SetFailReturn(nullptr);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(NULL))
        .IgnoreArgument(1);

    ///act
    auto result = Broker_AddModule(broker, &fake_module);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_ERROR);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_13_047: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]
TEST_FUNCTION(Broker_AddModule_fails_when_Lock_Init_fails)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    mocks.ResetAllCalls();

    // this is for the Broker_AddModule call
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module_info*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module struct*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    whenShallLock_Init_fail = currentLock_Init_call + 1;
    STRICT_EXPECTED_CALL(mocks, Lock_Init());

    ///act
    auto result = Broker_AddModule(broker, &fake_module);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_ERROR);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_13_047: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]
TEST_FUNCTION(Broker_AddModule_fails_quit_uuid_fails)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    mocks.ResetAllCalls();

    // this is for the Broker_AddModule call
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module_info*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module struct*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock_Init());
    STRICT_EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, UniqueId_Generate(IGNORED_PTR_ARG, 37))
        .IgnoreArgument(1)
        .SetFailReturn(UNIQUEID_ERROR);

    ///act
    auto result = Broker_AddModule(broker, &fake_module);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_ERROR);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_13_047: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]
TEST_FUNCTION(Broker_AddModule_fails_string_construct_fails)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    mocks.ResetAllCalls();

    // this is for the Broker_AddModule call
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module_info*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module struct*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock_Init());
    STRICT_EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, UniqueId_Generate(IGNORED_PTR_ARG, 37))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn((STRING_HANDLE)NULL);

    ///act
    auto result = Broker_AddModule(broker, &fake_module);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_ERROR);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_13_047: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]
TEST_FUNCTION(Broker_AddModule_fails_Lock_modules_lock_fails)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    mocks.ResetAllCalls();

    // this is for the Broker_AddModule call
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module_info*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module struct*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock_Init());
    STRICT_EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, UniqueId_Generate(IGNORED_PTR_ARG, 37))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(LOCK_ERROR);

    ///act
    auto result = Broker_AddModule(broker, &fake_module);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_ERROR);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_13_047: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]
TEST_FUNCTION(Broker_AddModule_fails_when_singlylinkedlist_add_fails)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    mocks.ResetAllCalls();

    // this is for the Broker_AddModule call
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module_info*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module struct*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock_Init());
    STRICT_EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, UniqueId_Generate(IGNORED_PTR_ARG, 37))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    whenShallsinglylinkedlist_add_fail = 1;
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    ///act
    auto result = Broker_AddModule(broker, &fake_module);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_ERROR);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_Destroy(broker);
}


//Tests_SRS_BROKER_13_047: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]
TEST_FUNCTION(Broker_AddModule_fails_when_nn_socket_fails)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    mocks.ResetAllCalls();

    // this is for the Broker_AddModule call
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module_info*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module struct*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock_Init());
    STRICT_EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, UniqueId_Generate(IGNORED_PTR_ARG, 37))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, nn_socket(AF_SP, NN_SUB))
        .SetFailReturn(-1);

    ///act
    auto result = Broker_AddModule(broker, &fake_module);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_ERROR);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_13_047: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]
TEST_FUNCTION(Broker_AddModule_fails_when_nn_connect_fails)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    mocks.ResetAllCalls();

    // this is for the Broker_AddModule call
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module_info*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module struct*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock_Init());
    STRICT_EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, UniqueId_Generate(IGNORED_PTR_ARG, 37))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, nn_socket(AF_SP, NN_SUB));
    STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_connect(IGNORED_NUM_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetFailReturn(-1);
    STRICT_EXPECTED_CALL(mocks, nn_close(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    ///act
    auto result = Broker_AddModule(broker, &fake_module);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_ERROR);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_13_047: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]
TEST_FUNCTION(Broker_AddModule_fails_when_nn_setSocketOpts_fails)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    mocks.ResetAllCalls();

    // this is for the Broker_AddModule call
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module_info*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module struct*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock_Init());
    STRICT_EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, UniqueId_Generate(IGNORED_PTR_ARG, 37))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, nn_socket(AF_SP, NN_SUB));
    STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_connect(IGNORED_NUM_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_setsockopt(IGNORED_NUM_ARG, NN_SUB, NN_SUB_SUBSCRIBE, IGNORED_PTR_ARG, 36))
        .IgnoreArgument(1)
        .IgnoreArgument(4)
        .SetFailReturn(-1);
    STRICT_EXPECTED_CALL(mocks, nn_close(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    ///act
    auto result = Broker_AddModule(broker, &fake_module);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_ERROR);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_13_047: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]
TEST_FUNCTION(Broker_AddModule_fails_when_ThreadAPI_Create_fails)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    mocks.ResetAllCalls();

    // this is for the Broker_AddModule call
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module_info*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module_info*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock_Init());
    STRICT_EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, UniqueId_Generate(IGNORED_PTR_ARG, 37))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, nn_socket(AF_SP, NN_SUB));
    STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_connect(IGNORED_NUM_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_setsockopt(IGNORED_NUM_ARG, NN_SUB, NN_SUB_SUBSCRIBE, IGNORED_PTR_ARG, 36))
        .IgnoreArgument(1)
        .IgnoreArgument(4);
    STRICT_EXPECTED_CALL(mocks, nn_close(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    whenShallThreadAPI_Create_fail = 1;
    STRICT_EXPECTED_CALL(mocks, ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    ///act
    auto result = Broker_AddModule(broker, &fake_module);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_ERROR);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_Destroy(broker);
}



//Tests_SRS_BROKER_13_107 : [The function shall assign the module handle to BROKER_MODULEINFO::module.]
//Tests_SRS_BROKER_17_013: [ The function shall create a nanomsg socket for reception. ]
//Tests_SRS_BROKER_17_014: [ The function shall bind the socket to the the BROKER_HANDLE_DATA::url. ]
//Tests_SRS_BROKER_13_099: [ The function shall initialize BROKER_MODULEINFO::socket_lock with a valid lock handle. ]
//Tests_SRS_BROKER_17_020: [ The function shall create a unique ID used as a quit signal. ]
//Tests_SRS_BROKER_13_102 : [The function shall create a new thread for the module by calling ThreadAPI_Create using module_publish_worker as the thread callback and using the newly allocated BROKER_MODULEINFO object as the thread context.]
//Tests_SRS_BROKER_13_039 : [This function shall acquire the lock on BROKER_HANDLE_DATA::modules_lock.]
//Tests_SRS_BROKER_13_045 : [Broker_AddModule shall append the new instance of BROKER_MODULEINFO to BROKER_HANDLE_DATA::modules.]
//Tests_SRS_BROKER_13_046 : [This function shall release the lock on BROKER_HANDLE_DATA::modules_lock.]
//Tests_SRS_BROKER_13_047 : [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]
//Tests_SRS_BROKER_17_028: [ The function shall subscribe BROKER_MODULEINFO::receive_socket to the quit signal GUID. ]
TEST_FUNCTION(Broker_AddModule_succeeds)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    mocks.ResetAllCalls();

    // this is for the Broker_AddModule call
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module_info*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the module struct*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, Lock_Init());
    STRICT_EXPECTED_CALL(mocks, UniqueId_Generate(IGNORED_PTR_ARG, 37))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_socket(AF_SP, NN_SUB));
    STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_connect(IGNORED_NUM_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_setsockopt(IGNORED_NUM_ARG, NN_SUB, NN_SUB_SUBSCRIBE, IGNORED_PTR_ARG, 36))
        .IgnoreArgument(1)
        .IgnoreArgument(4);
    STRICT_EXPECTED_CALL(mocks, ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    ///act
    auto result = Broker_AddModule(broker, &fake_module);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_OK);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_RemoveModule(broker, &fake_module);
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_13_026: [ This function shall assign user_data to a local variable called module_info of type BROKER_MODULEINFO*. ]
//Tests_SRS_BROKER_13_089: [ This function shall acquire the lock on module_info->socket_lock. ]
//Tests_SRS_BROKER_13_068: [ This function shall run a loop that keeps running until module_info->quit_message_guid is sent to the thread. ]
//Tests_SRS_BROKER_13_091: [ The function shall unlock module_info->socket_lock. ]
//Tests_SRS_BROKER_17_005: [ For every iteration of the loop, the function shall wait on the receive_socket for messages. ]
//Tests_SRS_BROKER_17_017: [ The function shall deserialize the message received. ]
//Tests_SRS_BROKER_13_092: [ The function shall deliver the message to the module's callback function via module_info->module_apis. ]
//Tests_SRS_BROKER_13_093: [ The function shall destroy the message that was dequeued by calling Message_Destroy. ]
//Tests_SRS_BROKER_17_019: [ The function shall free the buffer received on the receive_socket. ]
//Tests_SRS_BROKER_17_024: [ The function shall strip off the topic from the message. ]
TEST_FUNCTION(module_publish_worker_calls_receive_once_then_exits_on_quit_msg)
{
    CBrokerMocks mocks;
    auto broker = Broker_Create();

    // setup fake module's validation data
    unsigned char fake;
    MESSAGE_CONFIG c = { 1, &fake, (MAP_HANDLE)&fake };
    auto message = Message_Create(&c);
    call_status_for_FakeModule_Receive.module = fake_module.module_handle;
    call_status_for_FakeModule_Receive.messageHandle = message;

    auto add_result = Broker_AddModule(broker, &fake_module);

    mocks.ResetAllCalls();

    //loop 1
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, 0))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, nn_freemsg(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Message_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Message_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //loop 2
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, 0))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(37);
    STRICT_EXPECTED_CALL(mocks, nn_freemsg(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    // buf from nn_recv will always be "nn_recv", so match this here to let it
    // recognize quit message
    STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn("nn_recv");


    auto result = thread_func_to_call(thread_func_args);

    ASSERT_ARE_EQUAL(int, result, 0);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Message_Destroy(message);
    Broker_RemoveModule(broker, &fake_module);
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_02_004: [ If acquiring the lock fails, then module_publish_worker shall return. ]
TEST_FUNCTION(module_publish_worker_exits_on_lock_fail)
{
    CBrokerMocks mocks;
    auto broker = Broker_Create();

    // setup fake module's validation data
    unsigned char fake;
    MESSAGE_CONFIG c = { 1, &fake, (MAP_HANDLE)&fake };
    auto message = Message_Create(&c);
    call_status_for_FakeModule_Receive.module = fake_module.module_handle;
    call_status_for_FakeModule_Receive.messageHandle = message;

    auto add_result = Broker_AddModule(broker, &fake_module);

    mocks.ResetAllCalls();

    //loop 1
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(LOCK_ERROR);


    auto result = thread_func_to_call(thread_func_args);

    ASSERT_ARE_EQUAL(int, result, 0);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Message_Destroy(message);
    Broker_RemoveModule(broker, &fake_module);
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_13_091: [ The function shall unlock module_info->socket_lock. ]
//Tests_SRS_BROKER_17_016: [ If releasing the lock fails, then module_publish_worker shall return. ]
TEST_FUNCTION(module_publish_worker_exits_on_Unlock_fail)
{
    CBrokerMocks mocks;
    auto broker = Broker_Create();

    // setup fake module's validation data
    unsigned char fake;
    MESSAGE_CONFIG c = { 1, &fake, (MAP_HANDLE)&fake };
    auto message = Message_Create(&c);
    call_status_for_FakeModule_Receive.module = fake_module.module_handle;
    call_status_for_FakeModule_Receive.messageHandle = message;

    auto add_result = Broker_AddModule(broker, &fake_module);

    mocks.ResetAllCalls();

    //loop 1
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(LOCK_ERROR);
    STRICT_EXPECTED_CALL(mocks, nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, 0))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, nn_freemsg(IGNORED_PTR_ARG))
        .IgnoreArgument(1);


    auto result = thread_func_to_call(thread_func_args);

    ASSERT_ARE_EQUAL(int, result, 0);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Message_Destroy(message);
    Broker_RemoveModule(broker, &fake_module);
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_17_006: [ An error on receiving a message shall terminate the loop. ]
TEST_FUNCTION(module_publish_worker_exits_on_nn_recv_error)
{
    CBrokerMocks mocks;
    auto broker = Broker_Create();

    // setup fake module's validation data
    unsigned char fake;
    MESSAGE_CONFIG c = { 1, &fake, (MAP_HANDLE)&fake };
    auto message = Message_Create(&c);
    call_status_for_FakeModule_Receive.module = fake_module.module_handle;
    call_status_for_FakeModule_Receive.messageHandle = message;

    auto add_result = Broker_AddModule(broker, &fake_module);

    mocks.ResetAllCalls();

    //loop 1
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, 0))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1);


    auto result = thread_func_to_call(thread_func_args);

    ASSERT_ARE_EQUAL(int, result, 0);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Message_Destroy(message);
    Broker_RemoveModule(broker, &fake_module);
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_13_068: [ This function shall run a loop that keeps running until module_info->quit_message_guid is sent to the thread. ]
TEST_FUNCTION(module_publish_worker_message_size_matches_but_not_quit_for_me)
{
    CBrokerMocks mocks;
    auto broker = Broker_Create();

    // setup fake module's validation data
    unsigned char fake;
    MESSAGE_CONFIG c = { 1, &fake, (MAP_HANDLE)&fake };
    auto message = Message_Create(&c);
    call_status_for_FakeModule_Receive.module = fake_module.module_handle;
    call_status_for_FakeModule_Receive.messageHandle = message;

    auto add_result = Broker_AddModule(broker, &fake_module);

    mocks.ResetAllCalls();

    //loop 1
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, 0))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(37);
    STRICT_EXPECTED_CALL(mocks, nn_freemsg(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    // buf from nn_recv will always be "nn_recv", so force a mismatch
    STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn("nn_send");
    STRICT_EXPECTED_CALL(mocks, Message_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Message_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //loop 2
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, 0))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(37);
    STRICT_EXPECTED_CALL(mocks, nn_freemsg(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    // buf from nn_recv will always be "nn_recv", so match this here to let it
    // recognize quit message
    STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn("nn_recv");


    auto result = thread_func_to_call(thread_func_args);

    ASSERT_ARE_EQUAL(int, result, 0);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Message_Destroy(message);
    Broker_RemoveModule(broker, &fake_module);
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_17_018: [ If the deserialization is not successful, the message loop shall continue. ]
TEST_FUNCTION(module_publish_worker_continue_on_CreateFromByteArray_fails)
{
    CBrokerMocks mocks;
    auto broker = Broker_Create();

    // setup fake module's validation data
    unsigned char fake;
    MESSAGE_CONFIG c = { 1, &fake, (MAP_HANDLE)&fake };
    auto message = Message_Create(&c);
    call_status_for_FakeModule_Receive.module = fake_module.module_handle;
    call_status_for_FakeModule_Receive.messageHandle = message;

    auto add_result = Broker_AddModule(broker, &fake_module);

    mocks.ResetAllCalls();

    //loop 1
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, 0))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, nn_freemsg(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Message_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn((MESSAGE_HANDLE)NULL);

    //loop 2
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, 0))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(37);
    STRICT_EXPECTED_CALL(mocks, nn_freemsg(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    // buf from nn_recv will always be "nn_recv", so match this here to let it
    // recognize quit message
    STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn("nn_recv");


    auto result = thread_func_to_call(thread_func_args);

    ASSERT_ARE_EQUAL(int, result, 0);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Message_Destroy(message);
    Broker_RemoveModule(broker, &fake_module);
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_13_048: [If broker or module is NULL the function shall return BROKER_INVALIDARG.]
TEST_FUNCTION(Broker_RemoveModule_fails_with_null_broker)
{
    ///arrange
    CBrokerMocks mocks;

    ///act
    auto r1 = Broker_RemoveModule(NULL, (MODULE*)0x1);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, r1, BROKER_INVALIDARG);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
}

//Tests_SRS_BROKER_13_048: [If broker or module is NULL the function shall return BROKER_INVALIDARG.]
TEST_FUNCTION(Broker_RemoveModule_fails_with_null_module)
{
    ///arrange
    CBrokerMocks mocks;

    ///act
    auto r1 = Broker_RemoveModule((BROKER_HANDLE)0x1, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, r1, BROKER_INVALIDARG);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
}

//Tests_SRS_BROKER_13_088 : [This function shall acquire the lock on BROKER_HANDLE_DATA::modules_lock.]
//Tests_SRS_BROKER_13_049 : [Broker_RemoveModule shall perform a linear search for module in BROKER_HANDLE_DATA::modules.]
//Tests_SRS_BROKER_13_050 : [Broker_RemoveModule shall unlock BROKER_HANDLE_DATA::modules_lock and return BROKER_ERROR if the module is not found in BROKER_HANDLE_DATA::modules.]
//Tests_SRS_BROKER_13_052 : [The function shall remove the module from BROKER_HANDLE_DATA::modules.]
//Tests_SRS_BROKER_13_054 : [This function shall release the lock on BROKER_HANDLE_DATA::modules_lock.]
//Tests_SRS_BROKER_17_021: [ This function shall send a quit signal to the worker thread by sending BROKER_MODULEINFO::quit_message_guid to the publish_socket. ]
//Tests_SRS_BROKER_02_001: [ Broker_RemoveModule shall lock BROKER_MODULEINFO::socket_lock. ]
//Tests_SRS_BROKER_17_015: [ This function shall close the BROKER_MODULEINFO::receive_socket. ]
//Tests_SRS_BROKER_02_003: [ After closing the socket, Broker_RemoveModule shall unlock BROKER_MODULEINFO::info_lock. ]
//Tests_SRS_BROKER_13_104 : [The function shall wait for the module's thread to exit by joining BROKER_MODULEINFO::thread via ThreadAPI_Join. ]
//Tests_SRS_BROKER_13_057 : [The function shall free all members of the BROKER_MODULEINFO object.]
//Tests_SRS_BROKER_13_053 : [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]
TEST_FUNCTION(Broker_RemoveModule_succeeds)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    auto result = Broker_AddModule(broker, &fake_module);
    mocks.ResetAllCalls();

    // this is for the Broker_RemoveModule call
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, &fake_module))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_send(IGNORED_NUM_ARG, IGNORED_PTR_ARG, 37, 0))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG)) /*this is the lock protecting mq_lock*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_close(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);


    ///act
    result = Broker_RemoveModule(broker, &fake_module);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_OK);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_13_053: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]
TEST_FUNCTION(Broker_RemoveModule_fails_when_Lock_fails)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    auto result = Broker_AddModule(broker, &fake_module);
    mocks.ResetAllCalls();

    // this is for the Broker_RemoveModule call
    whenShallLock_fail = currentLock_call + 1;
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    ///act
    result = Broker_RemoveModule(broker, &fake_module);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_ERROR);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_RemoveModule(broker, &fake_module);
    Broker_Destroy(broker);
}


//Tests_SRS_BROKER_13_050: [Broker_RemoveModule shall unlock BROKER_HANDLE_DATA::modules_lock and return BROKER_ERROR if the module is not found in BROKER_HANDLE_DATA::modules.]
TEST_FUNCTION(Broker_RemoveModule_fails_when_singlylinkedlist_find_fails)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    auto result = Broker_AddModule(broker, &fake_module);
    mocks.ResetAllCalls();

    // this is for the Broker_RemoveModule call
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    whenShallsinglylinkedlist_find_fail = 1;
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, &fake_module))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    ///act
    result = Broker_RemoveModule(broker, &fake_module);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_ERROR);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_RemoveModule(broker, &fake_module);
    Broker_Destroy(broker);
}
//Tests_SRS_BROKER_13_050: [Broker_RemoveModule shall unlock BROKER_HANDLE_DATA::modules_lock and return BROKER_ERROR if the module is not found in BROKER_HANDLE_DATA::modules.]

TEST_FUNCTION(Broker_RemoveModule_succeeds_when_nn_send_fails)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    auto result = Broker_AddModule(broker, &fake_module);
    mocks.ResetAllCalls();

    // this is for the Broker_RemoveModule call
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, &fake_module))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_send(IGNORED_NUM_ARG, IGNORED_PTR_ARG, 37, 0))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1);
    STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_close(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    ///act
    result = Broker_RemoveModule(broker, &fake_module);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_OK);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_13_050: [Broker_RemoveModule shall unlock BROKER_HANDLE_DATA::modules_lock and return BROKER_ERROR if the module is not found in BROKER_HANDLE_DATA::modules.s]
TEST_FUNCTION(Broker_RemoveModule_succeeds_even_when_Lock_fails)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    auto result = Broker_AddModule(broker, &fake_module);
    mocks.ResetAllCalls();

    // this is for the Broker_RemoveModule call
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, &fake_module))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_send(IGNORED_NUM_ARG, IGNORED_PTR_ARG, 37, 0))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG)) /*this is the lock protecting mq_lock*/
        .IgnoreArgument(1)
        .SetFailReturn(LOCK_ERROR);
    STRICT_EXPECTED_CALL(mocks, nn_close(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    ///act
    result = Broker_RemoveModule(broker, &fake_module);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_OK);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_13_050: [Broker_RemoveModule shall unlock BROKER_HANDLE_DATA::modules_lock and return BROKER_ERROR if the module is not found in BROKER_HANDLE_DATA::modules.]
TEST_FUNCTION(Broker_RemoveModule_succeeds_even_when_nn_close_fails)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    auto result = Broker_AddModule(broker, &fake_module);
    mocks.ResetAllCalls();

    // this is for the Broker_RemoveModule call
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, &fake_module))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_send(IGNORED_NUM_ARG, IGNORED_PTR_ARG, 37, 0))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG)) /*this is the lock protecting mq_lock*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_close(IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(mocks, ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    ///act
    result = Broker_RemoveModule(broker, &fake_module);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_OK);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_13_050: [Broker_RemoveModule shall unlock BROKER_HANDLE_DATA::modules_lock and return BROKER_ERROR if the module is not found in BROKER_HANDLE_DATA::modules.]
TEST_FUNCTION(Broker_RemoveModule_succeeds_even_when_unlock_fails)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    auto result = Broker_AddModule(broker, &fake_module);
    mocks.ResetAllCalls();

    // this is for the Broker_RemoveModule call
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(LOCK_ERROR);

    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, &fake_module))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_send(IGNORED_NUM_ARG, IGNORED_PTR_ARG, 37, 0))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG)) /*this is the lock protecting mq_lock*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_close(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    ///act
    result = Broker_RemoveModule(broker, &fake_module);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_OK);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_17_029: [ If broker, link, link->module_source_handle or link->module_sink_handle are NULL, Broker_AddLink shall return BROKER_INVALIDARG. ]
TEST_FUNCTION(Broker_AddLink_null_broker_fails)
{
    ///arrange
    CBrokerMocks mocks;
    BROKER_HANDLE broker = NULL;
    BROKER_LINK_DATA bld =
    {
        fake_module_handle,
        fake_module_handle
    };

    ///act
    auto result = Broker_AddLink(broker, &bld);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_INVALIDARG);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup

}

//Tests_SRS_BROKER_17_029: [ If broker, link, link->module_source_handle or link->module_sink_handle are NULL, Broker_AddLink shall return BROKER_INVALIDARG. ]
TEST_FUNCTION(Broker_AddLink_null_link_fails)
{
    ///arrange
    CBrokerMocks mocks;
    BROKER_HANDLE broker = (BROKER_HANDLE)0x01;
    BROKER_LINK_DATA bld =
    {
        fake_module_handle,
        fake_module_handle
    };

    ///act
    auto result = Broker_AddLink(broker, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_INVALIDARG);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup

}

//Tests_SRS_BROKER_17_029: [ If broker, link, link->module_source_handle or link->module_sink_handle are NULL, Broker_AddLink shall return BROKER_INVALIDARG. ]
TEST_FUNCTION(Broker_AddLink_null_source_fails)
{
    ///arrange
    CBrokerMocks mocks;
    BROKER_HANDLE broker = (BROKER_HANDLE)0x01;
    BROKER_LINK_DATA bld =
    {
        NULL,
        fake_module_handle
    };

    ///act
    auto result = Broker_AddLink(broker, &bld);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_INVALIDARG);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup

}

//Tests_SRS_BROKER_17_029: [ If broker, link, link->module_source_handle or link->module_sink_handle are NULL, Broker_AddLink shall return BROKER_INVALIDARG. ]
TEST_FUNCTION(Broker_AddLink_null_sink_fails)
{
    ///arrange
    CBrokerMocks mocks;
    BROKER_HANDLE broker = (BROKER_HANDLE)0x01;
    BROKER_LINK_DATA bld =
    {
        fake_module_handle,
        NULL
    };

    ///act
    auto result = Broker_AddLink(broker, &bld);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_INVALIDARG);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup

}

//Tests_SRS_BROKER_17_030: [ Broker_AddLink shall lock the modules_lock. ]
//Tests_SRS_BROKER_17_031: [ Broker_AddLink shall find the BROKER_HANDLE_DATA::module_info for link->module_sink_handle. ]
//Tests_SRS_BROKER_17_041: [ Broker_AddLink shall find the BROKER_HANDLE_DATA::module_info for link->module_source_handle. ]
//Tests_SRS_BROKER_17_032: [ Broker_AddLink shall subscribe module_info->receive_socket to the link->module_source_handle module handle. ]
//Tests_SRS_BROKER_17_033: [ Broker_AddLink shall unlock the modules_lock. ]
TEST_FUNCTION(Broker_AddLink_succeeds)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    auto result = Broker_AddModule(broker, &fake_module);
    mocks.ResetAllCalls();

    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_setsockopt(IGNORED_NUM_ARG, NN_SUB, NN_SUB_SUBSCRIBE, IGNORED_PTR_ARG, sizeof(MODULE_HANDLE)))
        .IgnoreArgument(1)
        .IgnoreArgument(4);

    BROKER_LINK_DATA bld =
    {
        fake_module_handle,
        fake_module_handle
    };

    ///act
    result = Broker_AddLink(broker, &bld);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_OK);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_RemoveModule(broker, &fake_module);
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_17_034: [ Upon an error, Broker_AddLink shall return BROKER_ADD_LINK_ERROR ]
TEST_FUNCTION(Broker_AddLink_fails_setsocketoption_fails)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    auto result = Broker_AddModule(broker, &fake_module);
    mocks.ResetAllCalls();

    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_setsockopt(IGNORED_NUM_ARG, NN_SUB, NN_SUB_SUBSCRIBE, IGNORED_PTR_ARG, sizeof(MODULE_HANDLE)))
        .IgnoreArgument(1)
        .IgnoreArgument(4)
        .SetFailReturn(-1);

    BROKER_LINK_DATA bld =
    {
        fake_module_handle,
        fake_module_handle
    };

    ///act
    result = Broker_AddLink(broker, &bld);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_ADD_LINK_ERROR);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_RemoveModule(broker, &fake_module);
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_17_034: [ Upon an error, Broker_AddLink shall return BROKER_ADD_LINK_ERROR ]
TEST_FUNCTION(Broker_AddLink_fails_source_find_fails)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    auto result = Broker_AddModule(broker, &fake_module);
    mocks.ResetAllCalls();

    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .SetFailReturn((LIST_ITEM_HANDLE)NULL);

    BROKER_LINK_DATA bld =
    {
        fake_module_handle,
        fake_module_handle
    };

    ///act
    result = Broker_AddLink(broker, &bld);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_ADD_LINK_ERROR);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_RemoveModule(broker, &fake_module);
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_17_034: [ Upon an error, Broker_AddLink shall return BROKER_ADD_LINK_ERROR ]
TEST_FUNCTION(Broker_AddLink_fails_singlylinkedlist_find_fails)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    auto result = Broker_AddModule(broker, &fake_module);
    mocks.ResetAllCalls();

    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .SetFailReturn((LIST_ITEM_HANDLE)NULL);

    BROKER_LINK_DATA bld =
    {
        fake_module_handle,
        fake_module_handle
    };

    ///act
    result = Broker_AddLink(broker, &bld);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_ADD_LINK_ERROR);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_RemoveModule(broker, &fake_module);
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_17_034: [ Upon an error, Broker_AddLink shall return BROKER_ADD_LINK_ERROR ]
TEST_FUNCTION(Broker_AddLink_fails_lock_fails)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    auto result = Broker_AddModule(broker, &fake_module);
    mocks.ResetAllCalls();

    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(LOCK_ERROR);

    BROKER_LINK_DATA bld =
    {
        fake_module_handle,
        fake_module_handle
    };

    ///act
    result = Broker_AddLink(broker, &bld);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_ADD_LINK_ERROR);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_RemoveModule(broker, &fake_module);
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_17_035: [ If broker, link, link->module_source_handle or link->module_sink_handle are NULL, Broker_RemoveLink shall return BROKER_INVALIDARG. ]
TEST_FUNCTION(Broker_RemoveLink_null_broker_fails)
{
    ///arrange
    CBrokerMocks mocks;
    BROKER_HANDLE broker = NULL;
    BROKER_LINK_DATA bld =
    {
        fake_module_handle,
        fake_module_handle
    };

    ///act
    auto result = Broker_RemoveLink(broker, &bld);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_INVALIDARG);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup

}

//Tests_SRS_BROKER_17_035: [ If broker, link, link->module_source_handle or link->module_sink_handle are NULL, Broker_RemoveLink shall return BROKER_INVALIDARG. ]
TEST_FUNCTION(Broker_RemoveLink_null_link_fails)
{
    ///arrange
    CBrokerMocks mocks;
    BROKER_HANDLE broker = (BROKER_HANDLE)0x01;
    BROKER_LINK_DATA bld =
    {
        fake_module_handle,
        fake_module_handle
    };

    ///act
    auto result = Broker_RemoveLink(broker, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_INVALIDARG);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup

}

//Tests_SRS_BROKER_17_035: [ If broker, link, link->module_source_handle or link->module_sink_handle are NULL, Broker_RemoveLink shall return BROKER_INVALIDARG. ]
TEST_FUNCTION(Broker_RemoveLink_null_source_fails)
{
    ///arrange
    CBrokerMocks mocks;
    BROKER_HANDLE broker = (BROKER_HANDLE)0x01;
    BROKER_LINK_DATA bld =
    {
        NULL,
        fake_module_handle
    };

    ///act
    auto result = Broker_RemoveLink(broker, &bld);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_INVALIDARG);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup

}

//Tests_SRS_BROKER_17_035: [ If broker, link, link->module_source_handle or link->module_sink_handle are NULL, Broker_RemoveLink shall return BROKER_INVALIDARG. ]
TEST_FUNCTION(Broker_RemoveLink_null_sink_fails)
{
    ///arrange
    CBrokerMocks mocks;
    BROKER_HANDLE broker = (BROKER_HANDLE)0x01;
    BROKER_LINK_DATA bld =
    {
        fake_module_handle,
        NULL
    };

    ///act
    auto result = Broker_RemoveLink(broker, &bld);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_INVALIDARG);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup

}

//Tests_SRS_BROKER_17_036: [ Broker_RemoveLink shall lock the modules_lock. ]
//Tests_SRS_BROKER_17_037: [ Broker_RemoveLink shall find the module_info for link->module_sink_handle. ]
//Tests_SRS_BROKER_17_042: [ Broker_RemoveLink shall find the module_info for link->module_source_handle. ]
//Tests_SRS_BROKER_17_038: [ Broker_RemoveLink shall unsubscribe module_info->receive_socket from the link->module_source_handle module handle. ]
//Tests_SRS_BROKER_17_039: [ Broker_RemoveLink shall unlock the modules_lock. ]
TEST_FUNCTION(Broker_RemoveLink_succeeds)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    auto result = Broker_AddModule(broker, &fake_module);
    BROKER_LINK_DATA bld =
    {
        fake_module_handle,
        fake_module_handle
    };
    result = Broker_AddLink(broker, &bld);

    mocks.ResetAllCalls();

    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(mocks, nn_setsockopt(IGNORED_NUM_ARG, NN_SUB, NN_SUB_UNSUBSCRIBE, IGNORED_PTR_ARG, sizeof(MODULE_HANDLE)))
        .IgnoreArgument(1)
        .IgnoreArgument(4);

    ///act
    result = Broker_RemoveLink(broker, &bld);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_OK);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_RemoveModule(broker, &fake_module);
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_17_040: [ Upon an error, Broker_RemoveLink shall return BROKER_REMOVE_LINK_ERROR. ]
TEST_FUNCTION(Broker_RemoveLink_fails_sockopt_fails)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    auto result = Broker_AddModule(broker, &fake_module);
    BROKER_LINK_DATA bld =
    {
        fake_module_handle,
        fake_module_handle
    };
    result = Broker_AddLink(broker, &bld);

    mocks.ResetAllCalls();

    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_setsockopt(IGNORED_NUM_ARG, NN_SUB, NN_SUB_UNSUBSCRIBE, IGNORED_PTR_ARG, sizeof(MODULE_HANDLE)))
        .IgnoreArgument(1)
        .IgnoreArgument(4)
        .SetFailReturn(-1);

    ///act
    result = Broker_RemoveLink(broker, &bld);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_REMOVE_LINK_ERROR);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_RemoveModule(broker, &fake_module);
    Broker_Destroy(broker);
}

TEST_FUNCTION(Broker_RemoveLink_fails_source_find_fails)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    auto result = Broker_AddModule(broker, &fake_module);
    BROKER_LINK_DATA bld =
    {
        fake_module_handle,
        fake_module_handle
    };
    result = Broker_AddLink(broker, &bld);

    mocks.ResetAllCalls();

    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .SetFailReturn(nullptr);

    ///act
    result = Broker_RemoveLink(broker, &bld);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_REMOVE_LINK_ERROR);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_RemoveModule(broker, &fake_module);
    Broker_Destroy(broker);
}


//Tests_SRS_BROKER_17_040: [ Upon an error, Broker_RemoveLink shall return BROKER_REMOVE_LINK_ERROR. ]
TEST_FUNCTION(Broker_RemoveLink_fails_singlylinkedlist_find_fails)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    auto result = Broker_AddModule(broker, &fake_module);
    BROKER_LINK_DATA bld =
    {
        fake_module_handle,
        fake_module_handle
    };
    result = Broker_AddLink(broker, &bld);

    mocks.ResetAllCalls();

    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .SetFailReturn(nullptr);

    ///act
    result = Broker_RemoveLink(broker, &bld);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_REMOVE_LINK_ERROR);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_RemoveModule(broker, &fake_module);
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_17_040: [ Upon an error, Broker_RemoveLink shall return BROKER_REMOVE_LINK_ERROR. ]
TEST_FUNCTION(Broker_RemoveLink_fails_lock_fails)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    auto result = Broker_AddModule(broker, &fake_module);
    BROKER_LINK_DATA bld =
    {
        fake_module_handle,
        fake_module_handle
    };
    result = Broker_AddLink(broker, &bld);

    mocks.ResetAllCalls();

    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(LOCK_ERROR);

    ///act
    result = Broker_RemoveLink(broker, &bld);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_REMOVE_LINK_ERROR);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_RemoveModule(broker, &fake_module);
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_13_108: [If broker is NULL then Broker_IncRef shall do nothing.]
TEST_FUNCTION(Broker_IncRef_does_nothing_with_null_input)
{
    ///arrange
    CBrokerMocks mocks;

    ///act
    Broker_IncRef(NULL);

    ///assert
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
}

//Tests_SRS_BROKER_13_109: [Otherwise, `Broker_IncRef` shall increment the internal ref count.]
TEST_FUNCTION(Broker_IncRef_increments_ref_count_1)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    mocks.ResetAllCalls();

    ///act
    Broker_IncRef(broker);
    Broker_DecRef(broker);

    ///assert
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_13_111: [ Otherwise, Broker_Destroy shall decrement the internal ref count of the message. ]
TEST_FUNCTION(Broker_IncRef_increments_ref_count_1_destroy)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    mocks.ResetAllCalls();

    ///act
    Broker_IncRef(broker);
    Broker_Destroy(broker);

    ///assert
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_13_058: [If broker is NULL the function shall do nothing.]
TEST_FUNCTION(Broker_Destroy_does_nothing_with_null_input)
{
    ///arrange
    CBrokerMocks mocks;

    ///act
    Broker_Destroy(NULL);

    ///assert
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
}

//Tests_SRS_BROKER_13_058: [If broker is NULL the function shall do nothing.]
TEST_FUNCTION(Broker_DecRef_does_nothing_with_null_input)
{
    ///arrange
    CBrokerMocks mocks;

    ///act
    Broker_DecRef(NULL);

    ///assert
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
}

//Tests_SRS_BROKER_13_112: [If the ref count is zero then the allocated resources are freed.]
TEST_FUNCTION(Broker_Destroy_works)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    mocks.ResetAllCalls();

    // these are for Broker_Destroy
    STRICT_EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_close(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    ///act
    Broker_Destroy(broker);

    ///assert
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
}

//Tests_SRS_BROKER_13_112: [If the ref count is zero then the allocated resources are freed.]
//Tests_SRS_BROKER_13_113: [ This function shall implement all the requirements of the Broker_Destroy API. ]
TEST_FUNCTION(Broker_DecRef_works)
{
    ///arrange
    CBrokerMocks mocks;
    auto broker = Broker_Create();
    mocks.ResetAllCalls();

    // these are for Broker_Destroy
    STRICT_EXPECTED_CALL(mocks, Lock_Deinit(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_close(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, singlylinkedlist_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    ///act
    Broker_DecRef(broker);

    ///assert
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
}

//Tests_SRS_BROKER_13_030: [If broker or message is NULL the function shall return BROKER_INVALIDARG.]
TEST_FUNCTION(Broker_Publish_fails_with_null_broker)
{
    ///arrange
    CBrokerMocks mocks;

    ///act
    auto r1 = Broker_Publish(NULL, fake_module_handle, (MESSAGE_HANDLE)0x1);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, r1, BROKER_INVALIDARG);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
}

//Tests_SRS_BROKER_13_030: [If broker or message is NULL the function shall return BROKER_INVALIDARG.]
TEST_FUNCTION(Broker_Publish_fails_with_null_message)
{
    ///arrange
    CBrokerMocks mocks;

    ///act
    auto r1 = Broker_Publish((BROKER_HANDLE)0x1, fake_module_handle, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, r1, BROKER_INVALIDARG);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
}

//Tests_SRS_BROKER_13_030: [If broker or message is NULL the function shall return BROKER_INVALIDARG.]
TEST_FUNCTION(Broker_Publish_fails_with_null_source)
{
    ///arrange
    CBrokerMocks mocks;

    ///act
    auto r1 = Broker_Publish((BROKER_HANDLE)0x1, NULL, (MESSAGE_HANDLE)0x1);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, r1, BROKER_INVALIDARG);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
}
//Tests_SRS_BROKER_13_037: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]
TEST_FUNCTION(Broker_Publish_fails_when_Lock_fails)
{
    ///arrange
    CBrokerMocks mocks;

    auto broker = Broker_Create();

    // create a message to send
    unsigned char fake;
    MESSAGE_CONFIG c = { 1, &fake, (MAP_HANDLE)&fake };
    auto message = Message_Create(&c);

    auto result = Broker_AddModule(broker, &fake_module);

    mocks.ResetAllCalls();

    // this is for Broker_Publish
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(LOCK_ERROR);

    ///act
    result = Broker_Publish(broker, fake_module_handle, message);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_ERROR);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Message_Destroy(message);
    Broker_RemoveModule(broker, &fake_module);
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_13_037: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]
TEST_FUNCTION(Broker_Publish_fails_when_Message_ToByteArray_fails)
{
    ///arrange
    CBrokerMocks mocks;

    auto broker = Broker_Create();

    // create a message to send
    unsigned char fake;
    MESSAGE_CONFIG c = { 1, &fake, (MAP_HANDLE)&fake };
    auto message = Message_Create(&c);

    auto result = Broker_AddModule(broker, &fake_module);

    mocks.ResetAllCalls();

    // this is for Broker_Publish
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Message_Clone(message));
    STRICT_EXPECTED_CALL(mocks, Message_Destroy(message));
    STRICT_EXPECTED_CALL(mocks, Message_ToByteArray(message, NULL, 0))
        .SetFailReturn(-1);

    ///act
    result = Broker_Publish(broker, fake_module_handle, message);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_ERROR);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Message_Destroy(message);
    Broker_RemoveModule(broker, &fake_module);
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_13_037: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]
TEST_FUNCTION(Broker_Publish_fails_when_allocmsg_fails)
{
    ///arrange
    CBrokerMocks mocks;

    auto broker = Broker_Create();

    // create a message to send
    unsigned char fake;
    MESSAGE_CONFIG c = { 1, &fake, (MAP_HANDLE)&fake };
    auto message = Message_Create(&c);

    auto result = Broker_AddModule(broker, &fake_module);

    mocks.ResetAllCalls();

    // this is for Broker_Publish
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Message_Clone(message));
    STRICT_EXPECTED_CALL(mocks, Message_Destroy(message));
    STRICT_EXPECTED_CALL(mocks, Message_ToByteArray(message, NULL, 0));
    STRICT_EXPECTED_CALL(mocks, nn_allocmsg(1 + sizeof(MODULE_HANDLE), 0))
        .SetFailReturn(nullptr);

    ///act
    result = Broker_Publish(broker, fake_module_handle, message);

    ///assert
    ASSERT_ARE_EQUAL(int, result, BROKER_ERROR);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Message_Destroy(message);
    Broker_RemoveModule(broker, &fake_module);
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_13_037: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]
TEST_FUNCTION(Broker_Publish_fails_when_send_fails)
{
    ///arrange
    CBrokerMocks mocks;

    auto broker = Broker_Create();

    // create a message to send
    unsigned char fake;
    MESSAGE_CONFIG c = { 1, &fake, (MAP_HANDLE)&fake };
    auto message = Message_Create(&c);

    auto result = Broker_AddModule(broker, &fake_module);

    mocks.ResetAllCalls();

    // this is for Broker_Publish
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Message_Clone(message));
    STRICT_EXPECTED_CALL(mocks, Message_Destroy(message));
    STRICT_EXPECTED_CALL(mocks, Message_ToByteArray(message, NULL, 0));
    STRICT_EXPECTED_CALL(mocks, nn_allocmsg(1 + sizeof(MODULE_HANDLE), 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, nn_freemsg(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Message_ToByteArray(message, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, nn_send(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, 0))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn((int)-1);

    ///act
    result = Broker_Publish(broker, fake_module_handle, message);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_ERROR);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Message_Destroy(message);
    Broker_RemoveModule(broker, &fake_module);
    Broker_Destroy(broker);
}

//Tests_SRS_BROKER_17_022: [ Broker_Publish shall Lock the modules lock. ]
//Tests_SRS_BROKER_17_007: [Broker_Publish shall clone the message.]
//Tests_SRS_BROKER_17_008: [ Broker_Publish shall serialize the message. ]
//Tests_SRS_BROKER_17_025: [ Broker_Publish shall allocate a nanomsg buffer the size of the serialized message + sizeof(MODULE_HANDLE). ]
//Tests_SRS_BROKER_17_026: [ Broker_Publish shall copy source into the beginning of the nanomsg buffer. ]
//Tests_SRS_BROKER_17_027: [ Broker_Publish shall serialize the message into the remainder of the nanomsg buffer. ]
//Tests_SRS_BROKER_17_010: [ Broker_Publish shall send a message on the publish_socket. ]
//Tests_SRS_BROKER_17_011: [ Broker_Publish shall free the serialized message data. ]
//Tests_SRS_BROKER_17_012: [ Broker_Publish shall free the message. ]
//Tests_SRS_BROKER_17_023: [ Broker_Publish shall Unlock the modules lock. ]
//Tests_SRS_BROKER_13_037 : [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]
TEST_FUNCTION(Broker_Publish_succeeds)
{
    ///arrange
    CBrokerMocks mocks;

    auto broker = Broker_Create();

    // create a message to send
    unsigned char fake;
    MESSAGE_CONFIG c = { 1, &fake, (MAP_HANDLE)&fake };
    auto message = Message_Create(&c);

    auto result = Broker_AddModule(broker, &fake_module);

    mocks.ResetAllCalls();

    // this is for Broker_Publish
    STRICT_EXPECTED_CALL(mocks, Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Message_Clone(message));
    STRICT_EXPECTED_CALL(mocks, Message_Destroy(message));
    STRICT_EXPECTED_CALL(mocks, Message_ToByteArray(message, NULL, 0));
    STRICT_EXPECTED_CALL(mocks, nn_allocmsg(1 + sizeof(MODULE_HANDLE), 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, Message_ToByteArray(message, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mocks, nn_send(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, 0))
        .IgnoreArgument(1)
        .IgnoreArgument(2);


    ///act
    result = Broker_Publish(broker, fake_module_handle, message);

    ///assert
    ASSERT_ARE_EQUAL(BROKER_RESULT, result, BROKER_OK);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    Message_Destroy(message);
    Broker_RemoveModule(broker, &fake_module);
    Broker_Destroy(broker);
}


END_TEST_SUITE(broker_ut)
