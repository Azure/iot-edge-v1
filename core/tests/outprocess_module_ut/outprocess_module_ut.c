// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#define GATEWAY_EXPORT_H
#define GATEWAY_EXPORT

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
    }

    return result;
}

void my_gballoc_free(void* ptr)
{
    free(ptr);
}

#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umock_c_negative_tests.h"
#include "umocktypes_charptr.h"
#include "umocktypes_bool.h"
#include "umocktypes_stdint.h"

// defined to turn off dll linkage warnings
#include <nanomsg/nn.h>
#include <nanomsg/pair.h>
#include <nanomsg/reqrep.h>

#include "message.h"
#include "real_strings.h"


#define ENABLE_MOCKS

#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/lock.h"
#include "broker.h"
#include "module_loader.h"
#include "message_queue.h"

#undef ENABLE_MOCKS
#include "control_message.h"

#include "module_loaders/outprocess_module.h"

//=============================================================================
//Globals
//=============================================================================

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error");
}

static pfModule_ParseConfigurationFromJson Module_ParseConfigurationFromJson = NULL;
static pfModule_FreeConfiguration Module_FreeConfiguration = NULL;
static pfModule_Create Module_Create = NULL;
static pfModule_Destroy Module_Destroy = NULL;
static pfModule_Receive Module_Receive = NULL;
static pfModule_Start Module_Start = NULL;

// nanomsg mocks.
static size_t nn_current_msg_size;
static int current_nn_socket_index;
static int current_nn_bind_index;
static int current_nn_connect_index;

static void* nn_socket_memory[20];
static int when_shall_nn_socket_fail;
static int when_shall_nn_bind_fail;
static int when_shall_nn_connect_fail;

MOCK_FUNCTION_WITH_CODE(, int, nn_socket, int, domain, int, protocol)
	int socket_result = -1;
	current_nn_socket_index++;
	if ((when_shall_nn_socket_fail == 0) || (when_shall_nn_socket_fail != current_nn_socket_index))
	{
		nn_socket_memory[current_nn_socket_index] = my_gballoc_malloc(1);
		socket_result = current_nn_socket_index;
	}
MOCK_FUNCTION_END(socket_result)

MOCK_FUNCTION_WITH_CODE(, int, nn_close, int, s)
	int close_result = 0;
	my_gballoc_free(nn_socket_memory[s]);
MOCK_FUNCTION_END(close_result)

MOCK_FUNCTION_WITH_CODE(, int, nn_errno)
int errno_result = 0;
MOCK_FUNCTION_END(errno_result)

MOCK_FUNCTION_WITH_CODE(, int, nn_bind, int, s, const char *, addr)
	int bind_result = 0;
	current_nn_bind_index++;
	if ((addr == NULL) || 
		((when_shall_nn_bind_fail != 0) && (when_shall_nn_bind_fail == current_nn_bind_index)))
		bind_result = -1;
MOCK_FUNCTION_END(bind_result)

MOCK_FUNCTION_WITH_CODE(, int, nn_connect, int, s, const char *, addr)
	int connect_result = 0;
	current_nn_connect_index++;
	if ((addr == NULL) ||
		((when_shall_nn_connect_fail != 0) && (when_shall_nn_connect_fail == current_nn_connect_index)))
		connect_result = -1;
MOCK_FUNCTION_END(connect_result)

static bool should_nn_send_fail = false;
static int current_nn_send_index;
static int when_shall_nn_send_fail;
MOCK_FUNCTION_WITH_CODE(, int, nn_send, int, s, const void *, buf, size_t, len, int, flags)
	int send_length = 0;
	current_nn_send_index++;
	if (should_nn_send_fail || (current_nn_send_index == when_shall_nn_send_fail))
	{
		send_length = -1;
	}
	else
	{
		if (len == NN_MSG)
		{
			send_length = (int)nn_current_msg_size;
			my_gballoc_free(*(void**)buf); // send is supposed to free auto created buffer on success
		}
		else
		{
			send_length = (int)len;
		}
	}
MOCK_FUNCTION_END(send_length)

static bool should_nn_recv_fail = false;
static int current_nn_recv_index;
static int when_shall_nn_recv_fail;
MOCK_FUNCTION_WITH_CODE(, int, nn_recv, int, s, void *, buf, size_t, len, int, flags)
	int rcv_length;
	current_nn_recv_index++;
	if (should_nn_recv_fail || (current_nn_recv_index == when_shall_nn_recv_fail))
	{
		rcv_length = -1;
	}
	else
	{
		if (len == NN_MSG)
		{
			char * text = (char*)"nn_recv";
			(*(void**)buf) = my_gballoc_malloc(8);
			if ((*(void**)buf) != NULL)
			{
				memcpy((*(void**)buf), text, 8);
				rcv_length = 8;
			}
			else
			{
				rcv_length = -1;
			}
		}
		else
		{
			rcv_length = (int)len;
		}
	}
MOCK_FUNCTION_END(rcv_length)

MOCK_FUNCTION_WITH_CODE(, void*, nn_allocmsg, size_t, size, int, type)
	void * alloc_result = my_gballoc_malloc(size);
	nn_current_msg_size = size;
MOCK_FUNCTION_END(alloc_result)

MOCK_FUNCTION_WITH_CODE(, int, nn_freemsg, void *, msg)
	int free_result = 0;
my_gballoc_free(msg);
MOCK_FUNCTION_END(free_result)

//Thread API mocks
#define NUMMOCKTHREADS 6
static THREAD_START_FUNC thread_func_to_call[NUMMOCKTHREADS];
static void* thread_func_args[NUMMOCKTHREADS];
static size_t currentThreadAPI_Create_call;
static size_t whenShallThreadAPI_Create_fail;

THREADAPI_RESULT myThreadAPI_Create(THREAD_HANDLE* threadHandle, THREAD_START_FUNC func, void* arg)
{
	THREADAPI_RESULT result2;
	++currentThreadAPI_Create_call;
	if ((whenShallThreadAPI_Create_fail > 0) &&
		(currentThreadAPI_Create_call == whenShallThreadAPI_Create_fail))
	{
		result2 = THREADAPI_ERROR;
	}
	else
	{
		*threadHandle = (THREAD_HANDLE*)my_gballoc_malloc(9);
		thread_func_to_call[currentThreadAPI_Create_call] = func;
		thread_func_args[currentThreadAPI_Create_call] = arg;

		result2 = THREADAPI_OK;
	}
	return result2;
}

static THREADAPI_RESULT thread_join_result[NUMMOCKTHREADS];
// set call_thread_function_on_join to the correct create instance above
static int call_thread_function_on_join[NUMMOCKTHREADS];
static size_t currentThreadAPI_join_call;
THREADAPI_RESULT myThreadAPI_Join( THREAD_HANDLE threadHandle, int* res)
{
	currentThreadAPI_join_call++;
	int function_result;
	int thread_number = call_thread_function_on_join[currentThreadAPI_join_call];
	if (call_thread_function_on_join[currentThreadAPI_join_call] > 0)
	{
		function_result = 
			(*thread_func_to_call[thread_number])(thread_func_args[thread_number]);
	}
	else
	{
		function_result = 0;
	}
	if (res != NULL)
	{
		*res = function_result;
	}
	my_gballoc_free(threadHandle);
	return thread_join_result[currentThreadAPI_join_call];
}

/* Lock mocks
 */
LOCK_HANDLE my_Lock_Init(void)
{
	return (LOCK_HANDLE)my_gballoc_malloc(2);
}

LOCK_RESULT my_Lock(LOCK_HANDLE handle)
{
	LOCK_RESULT result = LOCK_ERROR;
	if (handle != NULL)
	{
		result = LOCK_OK;
	}
	return result;
}

LOCK_RESULT my_Unlock(LOCK_HANDLE handle)
{
	LOCK_RESULT result = LOCK_ERROR;
	if (handle != NULL)
	{
		result = LOCK_OK;
	}
	return result;
}

LOCK_RESULT my_Lock_Deinit(LOCK_HANDLE handle)
{
	LOCK_RESULT result = LOCK_ERROR;
	if (handle != NULL)
	{
		my_gballoc_free(handle);
		result = LOCK_OK;
	}
	return result;
}

/*  Control Message Mocks
 */

CONTROL_MESSAGE_MODULE_CREATE global_control_msg;
static int default_serialized_size;

MOCK_FUNCTION_WITH_CODE(, CONTROL_MESSAGE *, ControlMessage_CreateFromByteArray, const unsigned char*, source, size_t, size)
MOCK_FUNCTION_END((CONTROL_MESSAGE*)&global_control_msg)

MOCK_FUNCTION_WITH_CODE(, void, ControlMessage_Destroy, CONTROL_MESSAGE *, message)
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, int32_t, ControlMessage_ToByteArray, CONTROL_MESSAGE *, message, unsigned char*, buf, int32_t, size)
	int32_t carray_size = default_serialized_size;
MOCK_FUNCTION_END(carray_size)

/*  Message mocks 
 */

static int default_message_size;

MOCK_FUNCTION_WITH_CODE(, MESSAGE_HANDLE, Message_Create, const MESSAGE_CONFIG*, cfg)
MESSAGE_HANDLE m1 = (MESSAGE_HANDLE)my_gballoc_malloc(default_message_size);
uint8_t *counter = (uint8_t*)m1;
*counter = 1;
MOCK_FUNCTION_END(m1)

MOCK_FUNCTION_WITH_CODE(, MESSAGE_HANDLE, Message_Clone, MESSAGE_HANDLE, msg)
uint8_t *counter = (uint8_t*)msg;
(*counter)++;
MOCK_FUNCTION_END(msg)

MOCK_FUNCTION_WITH_CODE(, MESSAGE_HANDLE, Message_CreateFromByteArray, const unsigned char*, source, int32_t, size)
MESSAGE_HANDLE m2 = (MESSAGE_HANDLE)my_gballoc_malloc(size);
uint8_t *counter = (uint8_t*)m2;
*counter = 1;
MOCK_FUNCTION_END(m2)

MOCK_FUNCTION_WITH_CODE(, int32_t, Message_ToByteArray, MESSAGE_HANDLE, messageHandle, unsigned char*, buf, int32_t, size)
int32_t array_size = default_serialized_size;
MOCK_FUNCTION_END(array_size)

MOCK_FUNCTION_WITH_CODE(, void, Message_Destroy, MESSAGE_HANDLE, message)
uint8_t *counter = (uint8_t*)message;
--(*counter);
if (*counter == 0)
	my_gballoc_free(message);
MOCK_FUNCTION_END()


MOCK_FUNCTION_WITH_CODE(, BROKER_RESULT, Broker_Publish, BROKER_HANDLE, broker, MODULE_HANDLE, source, MESSAGE_HANDLE, message)
MOCK_FUNCTION_END(BROKER_OK)

BEGIN_TEST_SUITE(OutprocessModule_UnitTests)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
	TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
	g_testByTest = TEST_MUTEX_CREATE();
	ASSERT_IS_NOT_NULL(g_testByTest);

	umock_c_init(on_umock_c_error);
	umocktypes_charptr_register_types();
	umocktypes_stdint_register_types();
	umocktypes_bool_register_types();

	REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
	REGISTER_UMOCK_ALIAS_TYPE(MODULE_HANDLE, void*);
	REGISTER_UMOCK_ALIAS_TYPE(BROKER_HANDLE, void*);
	REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_HANDLE, void*);
	REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_QUEUE_HANDLE, void*);
	REGISTER_UMOCK_ALIAS_TYPE(LOCK_HANDLE, void*);
	REGISTER_UMOCK_ALIAS_TYPE(LOCK_RESULT, int);
	REGISTER_UMOCK_ALIAS_TYPE(THREAD_HANDLE, void*);
	REGISTER_UMOCK_ALIAS_TYPE(THREAD_START_FUNC, void*);
	REGISTER_UMOCK_ALIAS_TYPE(MODULE_API_VERSION, int);
	REGISTER_UMOCK_ALIAS_TYPE(BROKER_RESULT, int);
	REGISTER_UMOCK_ALIAS_TYPE(THREADAPI_RESULT, int);

	// STRING
	REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, real_STRING_construct);
	REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, real_STRING_delete);
	REGISTER_GLOBAL_MOCK_HOOK(STRING_c_str, real_STRING_c_str);
	REGISTER_GLOBAL_MOCK_HOOK(STRING_clone, real_STRING_clone);
	REGISTER_GLOBAL_MOCK_HOOK(STRING_length, real_STRING_length);
	
	// malloc/free hooks
	REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
	REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

	// thread hooks
	REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Create, myThreadAPI_Create);
	REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Join, myThreadAPI_Join);

	//Lock Hooks
	REGISTER_GLOBAL_MOCK_HOOK(Lock_Init, my_Lock_Init);
	REGISTER_GLOBAL_MOCK_HOOK(Lock, my_Lock);
	REGISTER_GLOBAL_MOCK_HOOK(Unlock, my_Unlock);
	REGISTER_GLOBAL_MOCK_HOOK(Lock_Deinit, my_Lock_Deinit);

	// message queue
	REGISTER_GLOBAL_MOCK_RETURNS(MESSAGE_QUEUE_create, (MESSAGE_QUEUE_HANDLE)0x40, NULL);


	Module_ParseConfigurationFromJson = Outprocess_Module_API_all.Module_ParseConfigurationFromJson;
	Module_FreeConfiguration = Outprocess_Module_API_all.Module_FreeConfiguration;
	Module_Create = Outprocess_Module_API_all.Module_Create;
	Module_Destroy = Outprocess_Module_API_all.Module_Destroy;
	Module_Receive = Outprocess_Module_API_all.Module_Receive;
	Module_Start = Outprocess_Module_API_all.Module_Start;
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
    malloc_will_fail = false;
    malloc_fail_count = 0;
    malloc_count = 0;
	should_nn_send_fail = false;
	should_nn_recv_fail = false;
	current_nn_send_index = 0;
	when_shall_nn_send_fail = 0;
	current_nn_recv_index = 0;
	when_shall_nn_recv_fail = 0;

	current_nn_socket_index = 0;
	current_nn_bind_index = 0;
	current_nn_connect_index = 0;
	for (int l = 0; l < 10; l++)
	{
		nn_socket_memory[l] = NULL;
	}

	nn_current_msg_size = 0;
	when_shall_nn_socket_fail =0;
	when_shall_nn_bind_fail=0;
	when_shall_nn_connect_fail=0;

	default_message_size = 1;
	default_serialized_size = 1;

	currentThreadAPI_Create_call = 0;
	whenShallThreadAPI_Create_fail = 0;
	currentThreadAPI_join_call = 0;
	for (int t = 0; t < NUMMOCKTHREADS; t++)
	{
		thread_func_to_call[t] = NULL;
		thread_func_args[t] = NULL;
		thread_join_result[t] = THREADAPI_OK;
		call_thread_function_on_join[t] = 0;
	}

	memset(&global_control_msg, 0, sizeof(CONTROL_MESSAGE_MODULE_CREATE));
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_001: [ This function shall return NULL if configuration is NULL ]*/
TEST_FUNCTION(Outprocess_ParseConfigurationFromJson_returns_null_when_string_returns_null)
{
	// arrange
	STRICT_EXPECTED_CALL(STRING_construct(NULL));

    // act
    void * result = Module_ParseConfigurationFromJson(NULL);

    // assert
    ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

}

/*Tests_SRS_OUTPROCESS_MODULE_17_002: [ This function shall construct a STRING_HANDLE from the given configuration and return the result. ]*/
TEST_FUNCTION(Outprocess_ParseConfigurationFromJson_returns_handle_when_string_is_passed)
{
	// arrange
	STRICT_EXPECTED_CALL(STRING_construct("a valid string"));

	// act
	void * result = Module_ParseConfigurationFromJson("a valid string");

	// assert
	ASSERT_IS_NOT_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution 
	Module_FreeConfiguration(result);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_003: [ If configuration is NULL this function shall do nothing. ]*/
TEST_FUNCTION(Outprocess_FreeConfiguration_does_nothing_when_given_null)
{
	// act
	Module_FreeConfiguration(NULL);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_OUTPROCESS_MODULE_17_004: [ This function shall delete the STRING_HANDLE represented by configuration. ]*/
TEST_FUNCTION(Outprocess_FreeConfiguration_frees_the_stuff)
{
	// arrange
	STRING_HANDLE result = (STRING_HANDLE)Module_ParseConfigurationFromJson("a valid string");

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(STRING_delete(result));

	// act
	Module_FreeConfiguration(result);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

static void setup_create_config(OUTPROCESS_MODULE_CONFIG* config)
{
	OUTPROCESS_MODULE_CONFIG new_config =
	{
		OUTPROCESS_LIFECYCLE_SYNC,
		STRING_construct("control_uri"),
		STRING_construct("message_uri"),
		STRING_construct("outprocess_module_args")
	};
	*config = new_config;
	umock_c_reset_all_calls();
}

static void cleanup_create_config(OUTPROCESS_MODULE_CONFIG* config)
{
	STRING_delete(config->control_uri);
	STRING_delete(config->message_uri);
	STRING_delete(config->outprocess_module_args);
}

static void setup_create_connections(OUTPROCESS_MODULE_CONFIG* config)
{
	const char * real_message_uri = real_STRING_c_str(config->message_uri);
	const char * real_control_uri = real_STRING_c_str(config->control_uri);

	STRICT_EXPECTED_CALL(nn_socket(AF_SP, NN_PAIR));
	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	// assuming the nanomsg mock starts socket at 1
	STRICT_EXPECTED_CALL(nn_connect(1, real_message_uri));
	STRICT_EXPECTED_CALL(nn_socket(AF_SP, NN_PAIR));
	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(nn_connect(2, real_control_uri));
}

static void setup_create_create_message(OUTPROCESS_MODULE_CONFIG* config)
{
	STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	STRICT_EXPECTED_CALL(ControlMessage_ToByteArray(IGNORED_PTR_ARG, NULL, 0))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_allocmsg(default_serialized_size, 0));
	STRICT_EXPECTED_CALL(ControlMessage_ToByteArray(IGNORED_PTR_ARG, IGNORED_PTR_ARG, default_serialized_size))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_005: [ If broker or configuration are NULL, this function shall return NULL. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_with_null_arguments)
{
	///arrange
	///act
	MODULE_HANDLE m1 = Module_Create(NULL, (const void*)0x42);
	MODULE_HANDLE m2 = Module_Create((BROKER_HANDLE)0x42, NULL);

	///asssert
	ASSERT_IS_NULL(m1);
	ASSERT_IS_NULL(m2);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	//ablution
}

/*Tests_SRS_OUTPROCESS_MODULE_17_006: [ This function shall allocate memory for the MODULE_HANDLE. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_007: [ This function shall intialize a lock for thread management. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_008: [ This function shall create a pair socket for sending gateway messages to the module host. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_009: [ This function shall bind and connect the pair socket to the message_uri. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_010: [ This function shall create a request/reply socket for sending control messages to the module host. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_011: [ This function shall connect the request/reply socket to the control_uri. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_012: [ This function shall construct a Create Message from configuration. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_013: [ This function shall send the Create Message on the control channel. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_014: [ This function shall wait for a Create Response on the control channel. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_015: [ This function shall expect a successful result from the Create Response to consider the module creation a success. ]*/
TEST_FUNCTION(Outprocess_Create_success)
{
	// arrange
	global_control_msg.base.type = CONTROL_MESSAGE_TYPE_MODULE_REPLY;
	global_control_msg.base.version = CONTROL_MESSAGE_VERSION_CURRENT;
	((CONTROL_MESSAGE_MODULE_REPLY*)&global_control_msg)->status = 0;

	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());

	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);

	setup_create_connections(&config);

	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());

	STRICT_EXPECTED_CALL(STRING_clone(config.control_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.message_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.outprocess_module_args));

	//create thread
	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	call_thread_function_on_join[1] = 1;
	STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	//join on the create thread.
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	setup_create_create_message(&config);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);

	STRICT_EXPECTED_CALL(nn_send(2, IGNORED_PTR_ARG, NN_MSG, 0))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(nn_recv(2, IGNORED_PTR_ARG, NN_MSG, 0))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(ControlMessage_CreateFromByteArray(IGNORED_PTR_ARG, 8))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_freemsg(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(ControlMessage_Destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NOT_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	Module_Destroy(result);
	cleanup_create_config(&config);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_013: [ This function shall send the Create Message on the control channel. ]*/
TEST_FUNCTION(Outprocess_Create_success_async)
{
	// arrange

	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	config.lifecycle_model = OUTPROCESS_LIFECYCLE_ASYNC;

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);
	setup_create_connections(&config);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(STRING_clone(config.control_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.message_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.outprocess_module_args));
	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NOT_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution

	Module_Destroy(result);
	cleanup_create_config(&config);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_015: [ This function shall expect a successful result from the Create Response to consider the module creation a success. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_wrong_message_type_response)
{
	// arrange
	global_control_msg.base.type = CONTROL_MESSAGE_TYPE_ERROR;
	global_control_msg.base.version = CONTROL_MESSAGE_VERSION_CURRENT;
	((CONTROL_MESSAGE_MODULE_REPLY*)&global_control_msg)->status = 0; 
	
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);
	setup_create_connections(&config);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(STRING_clone(config.control_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.message_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.outprocess_module_args));
	//create thread
	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	call_thread_function_on_join[1] = 1;
	STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	//join on the create thread.
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	setup_create_create_message(&config);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);

	STRICT_EXPECTED_CALL(nn_send(2, IGNORED_PTR_ARG, NN_MSG, 0))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(nn_recv(2, IGNORED_PTR_ARG, NN_MSG, 0))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(ControlMessage_CreateFromByteArray(IGNORED_PTR_ARG, 8))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_freemsg(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(ControlMessage_Destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);

}

/*Tests_SRS_OUTPROCESS_MODULE_17_015: [ This function shall expect a successful result from the Create Response to consider the module creation a success. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_wrong_message_status_response)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	global_control_msg.base.type = CONTROL_MESSAGE_TYPE_MODULE_REPLY;
	global_control_msg.base.version = CONTROL_MESSAGE_VERSION_CURRENT;
	((CONTROL_MESSAGE_MODULE_REPLY*)&global_control_msg)->status = 1;

	setup_create_config(&config);

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);
	setup_create_connections(&config);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(STRING_clone(config.control_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.message_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.outprocess_module_args));
	//create thread
	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	call_thread_function_on_join[1] = 1  ;
	STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	//join on the create thread.
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	setup_create_create_message(&config);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);

	STRICT_EXPECTED_CALL(nn_send(2, IGNORED_PTR_ARG, NN_MSG, 0))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(nn_recv(2, IGNORED_PTR_ARG, NN_MSG, 0))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(ControlMessage_CreateFromByteArray(IGNORED_PTR_ARG, 8))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_freemsg(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(ControlMessage_Destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);


	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);

}

/*Tests_SRS_OUTPROCESS_MODULE_17_015: [ This function shall expect a successful result from the Create Response to consider the module creation a success. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_reponse_not_a_message)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);
	setup_create_connections(&config);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(STRING_clone(config.control_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.message_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.outprocess_module_args));
	//create thread
	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	call_thread_function_on_join[1] = 1  ;
	STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	//join on the create thread.
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	setup_create_create_message(&config);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);

	STRICT_EXPECTED_CALL(nn_send(2, IGNORED_PTR_ARG, NN_MSG, 0))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(nn_recv(2, IGNORED_PTR_ARG, NN_MSG, 0))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(ControlMessage_CreateFromByteArray(IGNORED_PTR_ARG, 8))
		.IgnoreArgument(1)
		.SetReturn(NULL);
	STRICT_EXPECTED_CALL(nn_freemsg(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);

}

/*Tests_SRS_OUTPROCESS_MODULE_17_015: [ This function shall expect a successful result from the Create Response to consider the module creation a success. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_nn_recv_fails)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);
	setup_create_connections(&config);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(STRING_clone(config.control_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.message_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.outprocess_module_args));
	//create thread
	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	call_thread_function_on_join[1] = 1  ;
	STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	//join on the create thread.
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	setup_create_create_message(&config);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);

	STRICT_EXPECTED_CALL(nn_send(2, IGNORED_PTR_ARG, NN_MSG, 0))
		.IgnoreArgument(2);
	malloc_will_fail = true; // we malloc to create the byte array.
	malloc_fail_count = 11;
	STRICT_EXPECTED_CALL(nn_recv(2, IGNORED_PTR_ARG, NN_MSG, 0))
		.IgnoreArgument(2);

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);

}

/*Tests_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_thread_join_fails)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);
	setup_create_connections(&config);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(STRING_clone(config.control_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.message_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.outprocess_module_args));
	//create thread
	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	call_thread_function_on_join[1] = 0;
	thread_join_result[1] = THREADAPI_ERROR;
	STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);

}

/*Tests_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_thread_create_fails)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);
	setup_create_connections(&config);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(STRING_clone(config.control_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.message_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.outprocess_module_args));
	//create thread
	whenShallThreadAPI_Create_fail = 1;
	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);

}

/*Tests_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_nn_send_fails)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);
	setup_create_connections(&config);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(STRING_clone(config.control_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.message_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.outprocess_module_args));
	//create thread
	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	call_thread_function_on_join[1] = 1  ;
	STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	//join on the create thread.
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	setup_create_create_message(&config);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);

	should_nn_send_fail = true;
	STRICT_EXPECTED_CALL(nn_send(2, IGNORED_PTR_ARG, NN_MSG, 0))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(nn_freemsg(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);

}

/*Tests_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_nn_allocmsg_fails)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);
	setup_create_connections(&config);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(STRING_clone(config.control_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.message_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.outprocess_module_args));
	//create thread
	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	call_thread_function_on_join[1] = 1  ;
	STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	//join on the create thread.
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);

	STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	STRICT_EXPECTED_CALL(ControlMessage_ToByteArray(IGNORED_PTR_ARG, NULL, 0))
		.IgnoreArgument(1);
	malloc_will_fail = true; // we malloc to create the message.
	malloc_fail_count = 10;
	STRICT_EXPECTED_CALL(nn_allocmsg(default_serialized_size, 0));	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);

}

/*Tests_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_Message_ToByteArray_fails)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);
	setup_create_connections(&config);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(STRING_clone(config.control_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.message_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.outprocess_module_args));
	//create thread
	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	call_thread_function_on_join[1] = 1  ;
	STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	//join on the create thread.
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);

	STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	STRICT_EXPECTED_CALL(ControlMessage_ToByteArray(IGNORED_PTR_ARG, NULL, 0))
		.IgnoreArgument(1).SetReturn(-1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);

}

/*Tests_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_badargs_null)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);
	setup_create_connections(&config);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(STRING_clone(config.control_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.message_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.outprocess_module_args));
	//create thread
	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	call_thread_function_on_join[1] = 1  ;
	STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	//join on the create thread.
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);

	STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn(NULL);

	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);

}

/*Tests_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_badargs_length)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);
	setup_create_connections(&config);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(STRING_clone(config.control_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.message_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.outprocess_module_args));
	//create thread
	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	call_thread_function_on_join[1] = 1  ;
	STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	//join on the create thread.
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);

	STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn(0);
	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);

}

/*Tests_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_baduri_null)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);
	setup_create_connections(&config);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(STRING_clone(config.control_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.message_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.outprocess_module_args));
	//create thread
	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	call_thread_function_on_join[1] = 1  ;
	STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	//join on the create thread.
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);

	STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn(NULL);
	STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);

}

/*Tests_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_baduri_length)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);
	setup_create_connections(&config);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(STRING_clone(config.control_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.message_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.outprocess_module_args));
	//create thread
	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	call_thread_function_on_join[1] = 1;
	STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	//join on the create thread.
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);

	STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn(0);
	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);


}

/*Tests_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_send_lock_fail)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);
	setup_create_connections(&config);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	malloc_will_fail = true;
	malloc_fail_count = 8;
	STRICT_EXPECTED_CALL(Lock_Init());

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);
}
/*Tests_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_async_lock_fail)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);
	setup_create_connections(&config);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	malloc_will_fail = true;
	malloc_fail_count = 7;
	STRICT_EXPECTED_CALL(Lock_Init());

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);
}
/*Tests_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_control_lock_fail)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);
	setup_create_connections(&config);
	STRICT_EXPECTED_CALL(Lock_Init());
	malloc_will_fail = true;
	malloc_fail_count = 6;
	STRICT_EXPECTED_CALL(Lock_Init());

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);
}
/*Tests_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_receive_lock_fail)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);
	setup_create_connections(&config);
	malloc_will_fail = true;
	malloc_fail_count = 5;
	STRICT_EXPECTED_CALL(Lock_Init());

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_clone_args_fail)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	int test_result = 0;
	test_result = umock_c_negative_tests_init();
	ASSERT_ARE_EQUAL(int, 0, test_result);
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);
	setup_create_connections(&config); 
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(STRING_clone(config.control_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.message_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.outprocess_module_args))
		.SetFailReturn(NULL);

	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
	umock_c_negative_tests_snapshot();

	umock_c_negative_tests_fail_call(15);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);
	umock_c_negative_tests_deinit();
}
/*Tests_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_clone_msg_uri_fails)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	int test_result = 0;
	test_result = umock_c_negative_tests_init();
	ASSERT_ARE_EQUAL(int, 0, test_result);
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);
	setup_create_connections(&config);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(STRING_clone(config.control_uri));
	STRICT_EXPECTED_CALL(STRING_clone(config.message_uri)).SetFailReturn(NULL);

	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
	umock_c_negative_tests_snapshot();

	umock_c_negative_tests_fail_call(14);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);
	umock_c_negative_tests_deinit();
}
/*Tests_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_clone_control_uri_fails)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	int test_result = 0;
	test_result = umock_c_negative_tests_init();
	ASSERT_ARE_EQUAL(int, 0, test_result);
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);
	setup_create_connections(&config);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(STRING_clone(config.control_uri)).SetFailReturn(NULL);

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);
	umock_c_negative_tests_snapshot();

	umock_c_negative_tests_fail_call(13);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);
	umock_c_negative_tests_deinit();
}

/*Tests_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_control_connect_fails)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	const char * real_message_uri = real_STRING_c_str(config.message_uri);
	const char * real_control_uri = real_STRING_c_str(config.control_uri);
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);
	STRICT_EXPECTED_CALL(nn_socket(AF_SP, NN_PAIR));
	STRICT_EXPECTED_CALL(STRING_c_str(config.message_uri));
	// assuming the nanomsg mock starts socket at 1
	STRICT_EXPECTED_CALL(nn_connect(1, real_message_uri));
	STRICT_EXPECTED_CALL(nn_socket(AF_SP, NN_PAIR));
	STRICT_EXPECTED_CALL(STRING_c_str(config.control_uri));
	when_shall_nn_connect_fail = 2;
	STRICT_EXPECTED_CALL(nn_connect(2, real_control_uri));
	STRICT_EXPECTED_CALL(nn_errno());
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// arrange
	umock_c_reset_all_calls();
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);
	STRICT_EXPECTED_CALL(nn_socket(AF_SP, NN_PAIR));
	STRICT_EXPECTED_CALL(STRING_c_str(config.message_uri));
	// assuming the nanomsg mock starts socket at 3
	STRICT_EXPECTED_CALL(nn_connect(3, real_message_uri));
	STRICT_EXPECTED_CALL(nn_socket(AF_SP, NN_PAIR));
	STRICT_EXPECTED_CALL(STRING_c_str(config.control_uri)).SetReturn(NULL);
	when_shall_nn_connect_fail = current_nn_connect_index + 2;
	STRICT_EXPECTED_CALL(nn_connect(4, NULL));
	STRICT_EXPECTED_CALL(nn_errno());

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(3));
	STRICT_EXPECTED_CALL(nn_close(4));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);

}

/*Tests_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_control_socket_fails)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	const char * real_message_uri = real_STRING_c_str(config.message_uri);
	const char * real_control_uri = real_STRING_c_str(config.control_uri);
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);

	STRICT_EXPECTED_CALL(nn_socket(AF_SP, NN_PAIR));
	STRICT_EXPECTED_CALL(STRING_c_str(config.message_uri));
	// assuming the nanomsg mock starts socket at 1
	STRICT_EXPECTED_CALL(nn_connect(1, real_message_uri));
	when_shall_nn_socket_fail = 2;
	STRICT_EXPECTED_CALL(nn_socket(AF_SP, NN_PAIR));

	STRICT_EXPECTED_CALL(nn_errno());
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_message_connect_fails)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	const char * real_message_uri = real_STRING_c_str(config.message_uri);
	const char * real_control_uri = real_STRING_c_str(config.control_uri);
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);

	STRICT_EXPECTED_CALL(nn_socket(AF_SP, NN_PAIR));
	STRICT_EXPECTED_CALL(STRING_c_str(config.message_uri));
	// assuming the nanomsg mock starts socket at 1
	when_shall_nn_connect_fail = 1;
	STRICT_EXPECTED_CALL(nn_connect(1, real_message_uri));

	STRICT_EXPECTED_CALL(nn_errno());
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// arrange
	umock_c_reset_all_calls();
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);

	STRICT_EXPECTED_CALL(nn_socket(AF_SP, NN_PAIR));
	STRICT_EXPECTED_CALL(STRING_c_str(config.message_uri)).SetReturn(NULL);
	// assuming the nanomsg mock starts socket at 1
	when_shall_nn_connect_fail = 2;
	STRICT_EXPECTED_CALL(nn_connect(2, NULL));

	STRICT_EXPECTED_CALL(nn_errno());
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);

}

/*Tests_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_message_socket_fails)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn((MESSAGE_QUEUE_HANDLE)0x40);
	when_shall_nn_socket_fail = 1;
	STRICT_EXPECTED_CALL(nn_socket(AF_SP, NN_PAIR));
	
	STRICT_EXPECTED_CALL(nn_errno());
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_destroy((MESSAGE_QUEUE_HANDLE)0x40));
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_message_queue_fails)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_create())
		.SetReturn(NULL);

	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_lock_init_fails)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);
	malloc_will_fail = true;
	malloc_fail_count = 2;
	STRICT_EXPECTED_CALL(Lock_Init());
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
TEST_FUNCTION(Outprocess_Create_returns_null_malloc_fails)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	malloc_will_fail = true;
	malloc_fail_count = 1;
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);

	// act
	MODULE_HANDLE result = Module_Create((BROKER_HANDLE)0x42, &config);

	// assert

	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);
}

static void setup_start_or_destroy_message()
{
	STRICT_EXPECTED_CALL(ControlMessage_ToByteArray(IGNORED_PTR_ARG, NULL, 0))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_allocmsg(default_serialized_size, 0));
	STRICT_EXPECTED_CALL(ControlMessage_ToByteArray(IGNORED_PTR_ARG, IGNORED_PTR_ARG, default_serialized_size))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_020: [ This function shall do nothing if module is NULL. ]*/
TEST_FUNCTION(Outprocess_Start_does_nothing_with_null)
{
	///arrange
	///act
	Module_Start(NULL);
	///assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	///cleanup
}

/*Tests_SRS_OUTPROCESS_MODULE_17_017: [ This function shall ensure thread safety on execution. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_018: [ This function shall create a thread to handle receiving messages from module host. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_019: [ This function shall send a Start Message on the control channel. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_021: [ This function shall free any resources created. ]*/
TEST_FUNCTION(Outprocess_Start_success)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	global_control_msg.base.type = CONTROL_MESSAGE_TYPE_MODULE_REPLY;
	global_control_msg.base.version = CONTROL_MESSAGE_VERSION_CURRENT;
	((CONTROL_MESSAGE_MODULE_REPLY*)&global_control_msg)->status = 0;

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	setup_start_or_destroy_message();
	STRICT_EXPECTED_CALL(nn_send(2, IGNORED_PTR_ARG, NN_MSG, 0)).IgnoreArgument(2);

	///act
	Module_Start(module);

	///assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_021: [ This function shall free any resources created. ]*/
TEST_FUNCTION(Outprocess_Start_nn_send_fail)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	global_control_msg.base.type = CONTROL_MESSAGE_TYPE_MODULE_REPLY;
	global_control_msg.base.version = CONTROL_MESSAGE_VERSION_CURRENT;
	((CONTROL_MESSAGE_MODULE_REPLY*)&global_control_msg)->status = 0;

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	setup_start_or_destroy_message();
	should_nn_send_fail = true;
	STRICT_EXPECTED_CALL(nn_send(2, IGNORED_PTR_ARG, NN_MSG, 0))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(nn_freemsg(IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	///act
	Module_Start(module);

	///assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_021: [ This function shall free any resources created. ]*/
TEST_FUNCTION(Outprocess_Start_thread3_create_fails)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	global_control_msg.base.type = CONTROL_MESSAGE_TYPE_MODULE_REPLY;
	global_control_msg.base.version = CONTROL_MESSAGE_VERSION_CURRENT;
	((CONTROL_MESSAGE_MODULE_REPLY*)&global_control_msg)->status = 0;

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	currentThreadAPI_Create_call = 0;
	whenShallThreadAPI_Create_fail = 3;
	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	///act
	Module_Start(module);

	///assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}
/*Tests_SRS_OUTPROCESS_MODULE_17_021: [ This function shall free any resources created. ]*/
TEST_FUNCTION(Outprocess_Start_thread2_create_fails)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	global_control_msg.base.type = CONTROL_MESSAGE_TYPE_MODULE_REPLY;
	global_control_msg.base.version = CONTROL_MESSAGE_VERSION_CURRENT;
	((CONTROL_MESSAGE_MODULE_REPLY*)&global_control_msg)->status = 0;

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	currentThreadAPI_Create_call = 0;
	whenShallThreadAPI_Create_fail = 2;
	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	///act
	Module_Start(module);

	///assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_021: [ This function shall free any resources created. ]*/
TEST_FUNCTION(Outprocess_Start_thread1_create_fails)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	global_control_msg.base.type = CONTROL_MESSAGE_TYPE_MODULE_REPLY;
	global_control_msg.base.version = CONTROL_MESSAGE_VERSION_CURRENT;
	((CONTROL_MESSAGE_MODULE_REPLY*)&global_control_msg)->status = 0;

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	umock_c_reset_all_calls();

	currentThreadAPI_Create_call = 0;
	whenShallThreadAPI_Create_fail = 1;
	STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	///act
	Module_Start(module);

	///assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	///ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_026: [ If module is NULL, this function shall do nothing. ]*/
TEST_FUNCTION(Outprocess_Destroy_does_nothing_with_nothing)
{
	// act
	Module_Destroy(NULL);

	// arrange
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

static void teardown_a_thread(bool needs_join, bool lock_fail)
{
	if (lock_fail)
	{
		STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1).SetReturn(LOCK_ERROR);
	}
	else
	{
		STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
		STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	}
	if (needs_join)
	{
		STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.IgnoreArgument(2);
	}	
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_027: [ This function shall ensure thread safety on execution. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_028: [ This function shall construct a Destroy Message. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_029: [ This function shall send the Destroy Message on the control channel. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_030: [ This function shall close the message channel socket. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_031: [ This function shall close the control channel socket. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_032: [ This function shall signal the messaging thread to close. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_033: [ This function shall wait for the messaging thread to complete. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_034: [ This function shall release all resources created by this module. ]*/
TEST_FUNCTION(Outprocess_Destroy_success)
{
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	global_control_msg.base.type = CONTROL_MESSAGE_TYPE_MODULE_REPLY;
	global_control_msg.base.version = CONTROL_MESSAGE_VERSION_CURRENT;
	((CONTROL_MESSAGE_MODULE_REPLY*)&global_control_msg)->status = 0;

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);

	Module_Start(module);

	umock_c_reset_all_calls();

	// arrange
	setup_start_or_destroy_message();
	STRICT_EXPECTED_CALL(nn_send(2, IGNORED_PTR_ARG, NN_MSG, 1)).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	//thread_create order: async, msg_rec, msg_send, control
	//thread join order: async, msg_rec, msg_send, control, async
	call_thread_function_on_join[2] = 2;
	call_thread_function_on_join[3] = 3;
	call_thread_function_on_join[4] = 4;
	//teardown_a_thread(true, false);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	//teardown_a_thread(true, false);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	//teardown_a_thread(true, false);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	teardown_a_thread(false, false); //async should be closed and NULL
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	Module_Destroy(module);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);
}


/*Tests_SRS_OUTPROCESS_MODULE_17_027: [ This function shall ensure thread safety on execution. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_032: [ This function shall signal the messaging thread to close. ]*/
TEST_FUNCTION(Outprocess_Destroy_send_lock_join_fails)
{
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	config.lifecycle_model = OUTPROCESS_LIFECYCLE_ASYNC;
	global_control_msg.base.type = CONTROL_MESSAGE_TYPE_MODULE_REPLY;
	global_control_msg.base.version = CONTROL_MESSAGE_VERSION_CURRENT;
	((CONTROL_MESSAGE_MODULE_REPLY*)&global_control_msg)->status = 0;

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);

	umock_c_reset_all_calls();

	// arrange
	setup_start_or_destroy_message();

	should_nn_send_fail = true;
	STRICT_EXPECTED_CALL(nn_send(2, IGNORED_PTR_ARG, NN_MSG, 1)).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(nn_send(2, IGNORED_PTR_ARG, NN_MSG, 1)).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(nn_send(2, IGNORED_PTR_ARG, NN_MSG, 1)).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(nn_send(2, IGNORED_PTR_ARG, NN_MSG, 1)).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(nn_send(2, IGNORED_PTR_ARG, NN_MSG, 1)).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(nn_send(2, IGNORED_PTR_ARG, NN_MSG, 1)).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(nn_send(2, IGNORED_PTR_ARG, NN_MSG, 1)).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(nn_send(2, IGNORED_PTR_ARG, NN_MSG, 1)).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(nn_send(2, IGNORED_PTR_ARG, NN_MSG, 1)).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(nn_send(2, IGNORED_PTR_ARG, NN_MSG, 1)).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(nn_send(2, IGNORED_PTR_ARG, NN_MSG, 1)).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(nn_freemsg(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	thread_join_result[1] = THREADAPI_ERROR;
	thread_join_result[2] = THREADAPI_ERROR;
	thread_join_result[3] = THREADAPI_ERROR;
	teardown_a_thread(true, true);
	teardown_a_thread(true, true);
	teardown_a_thread(true, true);
	teardown_a_thread(true, true); //async won't be closed.
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	Module_Destroy(module);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_028: [ This function shall construct a Destroy Message. ]*/
TEST_FUNCTION(Outprocess_Destroy_destroy_message_alloc_fails)
{
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	global_control_msg.base.type = CONTROL_MESSAGE_TYPE_MODULE_REPLY;
	global_control_msg.base.version = CONTROL_MESSAGE_VERSION_CURRENT;
	((CONTROL_MESSAGE_MODULE_REPLY*)&global_control_msg)->status = 0;

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);

	umock_c_reset_all_calls();

	// arrange
	STRICT_EXPECTED_CALL(ControlMessage_ToByteArray(IGNORED_PTR_ARG, NULL, 0))
		.IgnoreArgument(1);
	malloc_will_fail = true;
	malloc_fail_count = 1; //nn_allocmsg
	malloc_count = 0;
	STRICT_EXPECTED_CALL(nn_allocmsg(default_serialized_size, 0));
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	teardown_a_thread(true, false);
	teardown_a_thread(true, false);
	teardown_a_thread(true, false);
	teardown_a_thread(false, false);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	Module_Destroy(module);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_028: [ This function shall construct a Destroy Message. ]*/
TEST_FUNCTION(Outprocess_Destroy_destroy_message_to_array_fails)
{
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	global_control_msg.base.type = CONTROL_MESSAGE_TYPE_MODULE_REPLY;
	global_control_msg.base.version = CONTROL_MESSAGE_VERSION_CURRENT;
	((CONTROL_MESSAGE_MODULE_REPLY*)&global_control_msg)->status = 0;

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);

	umock_c_reset_all_calls();

	// arrange
	STRICT_EXPECTED_CALL(ControlMessage_ToByteArray(IGNORED_PTR_ARG, NULL, 0))
		.IgnoreArgument(1)
		.SetReturn(-1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(1));
	STRICT_EXPECTED_CALL(nn_close(2));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	teardown_a_thread(true, false);
	teardown_a_thread(true, false);
	teardown_a_thread(true, false);
	teardown_a_thread(false, false);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	Module_Destroy(module);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	cleanup_create_config(&config);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_022: [ If module or message_handle is NULL, this function shall do nothing. ]*/
TEST_FUNCTION(Outprocess_Receive_does_nothing_with_nothing)
{
	// act
	Module_Receive(NULL, (MESSAGE_HANDLE)0x42);
	Module_Receive((MODULE_HANDLE)0x42, NULL);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_OUTPROCESS_MODULE_17_023: [ This function shall serialize the message for transmission on the message channel. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_024: [ This function shall send the message on the message channel. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_025: [ This function shall free any resources created. ]*/
TEST_FUNCTION(Outprocess_Receive_success)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);
	MESSAGE_HANDLE msg = Message_Create((const MESSAGE_CONFIG*)(0x42));
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Message_Clone(msg));
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_push(IGNORED_PTR_ARG, msg)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	Module_Receive(module, msg);

	// assert 
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//ablution
	Message_Destroy(msg);
	Message_Destroy(msg);
	Module_Destroy(module);
	cleanup_create_config(&config);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_024: [ This function shall send the message on the message channel. ]*/
TEST_FUNCTION(Outprocess_Receive_push_queue_fails)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);
	MESSAGE_HANDLE msg = Message_Create((const MESSAGE_CONFIG*)(0x42));
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Message_Clone(msg));
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_push(IGNORED_PTR_ARG, msg)).IgnoreArgument(1).SetReturn(2620);
	STRICT_EXPECTED_CALL(Message_Destroy(msg));
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	Module_Receive(module, msg);

	// assert 
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//ablution
	Message_Destroy(msg);
	Module_Destroy(module);
	cleanup_create_config(&config);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_024: [ This function shall send the message on the message channel. ]*/
TEST_FUNCTION(Outprocess_Receive_Lock_fails)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);
	MESSAGE_HANDLE msg = Message_Create((const MESSAGE_CONFIG*)(0x42));
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Message_Clone(msg));
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1).SetReturn(LOCK_ERROR);
	STRICT_EXPECTED_CALL(Message_Destroy(msg));

	// act
	Module_Receive(module, msg);

	// assert 
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//ablution
	Message_Destroy(msg);
	Module_Destroy(module);
	cleanup_create_config(&config);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_024: [ This function shall send the message on the message channel. ]*/
TEST_FUNCTION(Outprocess_Receive_Message_Clone_fails)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);
	MESSAGE_HANDLE msg = Message_Create((const MESSAGE_CONFIG*)(0x42));
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Message_Clone(msg)).SetReturn(NULL);

	// act
	Module_Receive(module, msg);

	// assert 
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//ablution
	Message_Destroy(msg);
	Message_Destroy(msg);
	Module_Destroy(module);
	cleanup_create_config(&config);
}

TEST_FUNCTION(Outprocess_Receive_thread_does_nothing_with_nothing)
{
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);
	umock_c_reset_all_calls();

	// act
	thread_func_to_call[3](NULL);


	// assert 
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

TEST_FUNCTION(Outprocess_Receive_thread_success)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);
	MESSAGE_HANDLE msg = Message_Create((const MESSAGE_CONFIG*)(0x42));
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_is_empty(IGNORED_PTR_ARG)).IgnoreArgument(1)
		.SetReturn(false);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_pop(IGNORED_PTR_ARG)).IgnoreArgument(1)
		.SetReturn(msg);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Message_ToByteArray(msg, NULL, 0));
	STRICT_EXPECTED_CALL(nn_allocmsg(default_serialized_size, 0));
	STRICT_EXPECTED_CALL(Message_ToByteArray(msg, IGNORED_PTR_ARG, default_serialized_size))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(nn_send(1, IGNORED_PTR_ARG, NN_MSG, 0)).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(Message_Destroy(msg));
	STRICT_EXPECTED_CALL(ThreadAPI_Sleep(1));
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1)
		.SetReturn(LOCK_ERROR);

	// act
	//third thread created is outgoing message thread
	thread_func_to_call[3](thread_func_args[3]);

	// assert 
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

TEST_FUNCTION(Outprocess_Receive_thread_nn_send_1st_unlock_fails)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);
	MESSAGE_HANDLE msg = Message_Create((const MESSAGE_CONFIG*)(0x42));
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_is_empty(IGNORED_PTR_ARG)).IgnoreArgument(1)
		.SetReturn(false);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_pop(IGNORED_PTR_ARG)).IgnoreArgument(1)
		.SetReturn(msg);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Message_ToByteArray(msg, NULL, 0));
	STRICT_EXPECTED_CALL(nn_allocmsg(default_serialized_size, 0));
	STRICT_EXPECTED_CALL(Message_ToByteArray(msg, IGNORED_PTR_ARG, default_serialized_size))
		.IgnoreArgument(2);
	should_nn_send_fail = true;
	current_nn_send_index = 0;
	when_shall_nn_send_fail = 1;
	STRICT_EXPECTED_CALL(nn_send(1, IGNORED_PTR_ARG, NN_MSG, 0)).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(nn_freemsg(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Message_Destroy(msg));
	STRICT_EXPECTED_CALL(ThreadAPI_Sleep(1));
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1)
		.SetReturn(LOCK_ERROR);

	// act
	//third thread created is outgoing message thread
	thread_func_to_call[3](thread_func_args[3]);

	// assert 
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

TEST_FUNCTION(Outprocess_Receive_thread_nn_alloc_2nd_lock_fails)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);
	MESSAGE_HANDLE msg = Message_Create((const MESSAGE_CONFIG*)(0x42));
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_is_empty(IGNORED_PTR_ARG)).IgnoreArgument(1)
		.SetReturn(false);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_pop(IGNORED_PTR_ARG)).IgnoreArgument(1)
		.SetReturn(msg);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Message_ToByteArray(msg, NULL, 0));
	malloc_will_fail = true;
	malloc_fail_count = malloc_count + 1;
	STRICT_EXPECTED_CALL(nn_allocmsg(default_serialized_size, 0));
	STRICT_EXPECTED_CALL(Message_Destroy(msg));
	STRICT_EXPECTED_CALL(ThreadAPI_Sleep(1));
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1)
		.SetReturn(LOCK_ERROR);

	// act
	//third thread created is outgoing message thread
	thread_func_to_call[3](thread_func_args[3]);

	// assert 
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

TEST_FUNCTION(Outprocess_Receive_thread_serialize_2nd_unlock_fails)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);
	MESSAGE_HANDLE msg = Message_Create((const MESSAGE_CONFIG*)(0x42));
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_is_empty(IGNORED_PTR_ARG)).IgnoreArgument(1)
		.SetReturn(false);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_pop(IGNORED_PTR_ARG)).IgnoreArgument(1)
		.SetReturn(msg);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Message_ToByteArray(msg, NULL, 0)).SetReturn(-1);
	STRICT_EXPECTED_CALL(Message_Destroy(msg));
	STRICT_EXPECTED_CALL(ThreadAPI_Sleep(1));
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_is_empty(IGNORED_PTR_ARG)).IgnoreArgument(1)
		.SetReturn(true);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1)
		.SetReturn(LOCK_ERROR);

	// act
	//third thread created is outgoing message thread
	thread_func_to_call[3](thread_func_args[3]);

	// assert 
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

TEST_FUNCTION(Outprocess_Receive_thread_queue_has_null_msg_fails)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);
	MESSAGE_HANDLE msg = Message_Create((const MESSAGE_CONFIG*)(0x42));
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_is_empty(IGNORED_PTR_ARG)).IgnoreArgument(1)
		.SetReturn(false);
	STRICT_EXPECTED_CALL(MESSAGE_QUEUE_pop(IGNORED_PTR_ARG)).IgnoreArgument(1)
		.SetReturn(NULL);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);

	// act
	//third thread created is outgoing message thread
	thread_func_to_call[3](thread_func_args[3]);

	// assert 
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//ablution
	Message_Destroy(msg);
	Module_Destroy(module);
	cleanup_create_config(&config);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_037: [ This function shall receive the module handle data as the thread parameter. ]*/
TEST_FUNCTION(Outprocess_messaging_thread_does_nothing_null_input)
{
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);

	umock_c_reset_all_calls();

	int function_result = (*thread_func_to_call[2])(NULL);

	// assert
	ASSERT_ARE_EQUAL(int, function_result, 0);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_036: [ This function shall ensure thread safety on execution. ]*/
TEST_FUNCTION(Outprocess_messaging_thread_ends_lock_fails)
{
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1).SetReturn(LOCK_ERROR);

	int function_result = (*thread_func_to_call[2])(thread_func_args[2]);

	// assert
	ASSERT_ARE_EQUAL(int, function_result, 0);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_036: [ This function shall ensure thread safety on execution. ]*/
TEST_FUNCTION(Outprocess_messaging_thread_ends_unlock_fails)
{
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1).SetReturn(LOCK_ERROR);

	int function_result = (*thread_func_to_call[2])(thread_func_args[2]);

	// assert
	ASSERT_ARE_EQUAL(int, function_result, 0);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_036: [ This function shall ensure thread safety on execution. ]*/
TEST_FUNCTION(Outprocess_messaging_thread_ends_lock2_fails)
{
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1).SetReturn(LOCK_ERROR);

	int function_result = (*thread_func_to_call[2])(thread_func_args[2]);

	// assert
	ASSERT_ARE_EQUAL(int, function_result, 0);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_036: [ This function shall ensure thread safety on execution. ]*/
TEST_FUNCTION(Outprocess_messaging_thread_ends_unlock2_fails)
{
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1).SetReturn(LOCK_ERROR);

	int function_result = (*thread_func_to_call[2])(thread_func_args[2]);

	// assert
	ASSERT_ARE_EQUAL(int, function_result, 0);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_038: [ This function shall read from the message channel for gateway messages from the module host. ]*/
TEST_FUNCTION(Outprocess_messaging_thread_ends_nn_recv_fails)
{
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	should_nn_recv_fail = true;
	STRICT_EXPECTED_CALL(nn_recv(1, IGNORED_PTR_ARG, NN_MSG, 0)).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(nn_errno()).SetReturn(37);
	STRICT_EXPECTED_CALL(ThreadAPI_Sleep(1));

	int function_result = (*thread_func_to_call[2])(thread_func_args[2]);

	// assert
	ASSERT_ARE_EQUAL(int, function_result, 0);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

/*Tests_SRS_OUTPROCESS_MODULE_17_036: [ This function shall ensure thread safety on execution. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_037: [ This function shall receive the module handle data as the thread parameter. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_038: [ This function shall read from the message channel for gateway messages from the module host. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_039: [ Upon successful receiving a gateway message, this function shall deserialize the message. ]*/
/*Tests_SRS_OUTPROCESS_MODULE_17_040: [This function shall publish any successfully created gateway message to the broker.]*/
TEST_FUNCTION(Outprocess_messaging_thread_ends_one_loop_then_fails)
{
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_recv(1, IGNORED_PTR_ARG, NN_MSG, 0)).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(Message_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(Broker_Publish(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(Message_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_freemsg(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(ThreadAPI_Sleep(1));
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1).SetReturn(LOCK_ERROR);

	int function_result = (*thread_func_to_call[2])(thread_func_args[2]);

	// assert
	ASSERT_ARE_EQUAL(int, function_result, 0);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

TEST_FUNCTION(Outprocess_control_thread_does_nothing_with_nothing)
{
	// arrange
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);
	umock_c_reset_all_calls();


	// act
	//fourth thread created is control thread
	thread_func_to_call[4](NULL);

	// assert 
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//ablution
	Module_Destroy(module);
	cleanup_create_config(&config);

}

TEST_FUNCTION(Outprocess_control_thread_success)
{
	// arrange
	global_control_msg.base.type = CONTROL_MESSAGE_TYPE_MODULE_REPLY;
	global_control_msg.base.version = CONTROL_MESSAGE_VERSION_CURRENT;
	((CONTROL_MESSAGE_MODULE_REPLY*)&global_control_msg)->status = 0;
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_recv(2, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT)).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(ControlMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.IgnoreAllArguments();
	STRICT_EXPECTED_CALL(nn_freemsg(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(ControlMessage_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(ThreadAPI_Sleep(250));
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1)
		.SetReturn(LOCK_ERROR);

	// act
	//fourth thread created is control message thread
	thread_func_to_call[4](thread_func_args[4]);

	// assert 
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

TEST_FUNCTION(Outprocess_control_thread_restart_success)
{
	// arrange
	CONTROL_MESSAGE_MODULE_REPLY remote_died =
	{
		{ CONTROL_MESSAGE_VERSION_CURRENT,  CONTROL_MESSAGE_TYPE_MODULE_REPLY },
		(uint8_t)-1
	};
	global_control_msg.base.type = CONTROL_MESSAGE_TYPE_MODULE_REPLY;
	global_control_msg.base.version = CONTROL_MESSAGE_VERSION_CURRENT;
	((CONTROL_MESSAGE_MODULE_REPLY*)&global_control_msg)->status = 0;
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);
	umock_c_reset_all_calls();

	//1st pass: status is bad
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_recv(2, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT)).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(ControlMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.IgnoreAllArguments()
		.SetReturn((CONTROL_MESSAGE*)&remote_died);
	STRICT_EXPECTED_CALL(nn_freemsg(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(ControlMessage_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(ThreadAPI_Sleep(250));
	// 2nd pass:needs_to_attach is set.
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	//restart control channel
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(IGNORED_NUM_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_socket(AF_SP, NN_PAIR));
	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_connect(IGNORED_NUM_ARG, IGNORED_PTR_ARG)).IgnoreAllArguments();
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	// resend create message
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	setup_create_create_message(&config);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);

	STRICT_EXPECTED_CALL(nn_send(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, 0))
		.IgnoreArgument(1).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, 0))
		.IgnoreArgument(1).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(ControlMessage_CreateFromByteArray(IGNORED_PTR_ARG, 8))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_freemsg(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(ControlMessage_Destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	//bail out
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1)
		.SetReturn(LOCK_ERROR);


	// act
	//fourth thread created is control message thread
	thread_func_to_call[4](thread_func_args[4]);

	// assert 
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

TEST_FUNCTION(Outprocess_control_thread_restart_fails_bad_msg_then_reset_control)
{
	// arrange
	CONTROL_MESSAGE_MODULE_REPLY remote_died =
	{
		{ CONTROL_MESSAGE_VERSION_CURRENT,  CONTROL_MESSAGE_TYPE_MODULE_REPLY },
		(uint8_t)-1
	};
	global_control_msg.base.type = CONTROL_MESSAGE_TYPE_MODULE_REPLY;
	global_control_msg.base.version = CONTROL_MESSAGE_VERSION_CURRENT;
	((CONTROL_MESSAGE_MODULE_REPLY*)&global_control_msg)->status = 0;
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);
	umock_c_reset_all_calls();

	//1st pass: status is bad
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_recv(2, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT)).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(ControlMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.IgnoreAllArguments()
		.SetReturn((CONTROL_MESSAGE*)&remote_died);
	STRICT_EXPECTED_CALL(nn_freemsg(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(ControlMessage_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(ThreadAPI_Sleep(250));
	// 2nd pass:needs_to_attach is set, get bad message.
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	//restart control channel
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(IGNORED_NUM_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_socket(AF_SP, NN_PAIR));
	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_connect(IGNORED_NUM_ARG, IGNORED_PTR_ARG)).IgnoreAllArguments();
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	// resend create message
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	setup_create_create_message(&config);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_send(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, 0))
		.IgnoreArgument(1).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, 0))
		.IgnoreArgument(1).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(ControlMessage_CreateFromByteArray(IGNORED_PTR_ARG, 8))
		.IgnoreArgument(1).SetReturn((CONTROL_MESSAGE*)&remote_died);
	STRICT_EXPECTED_CALL(nn_freemsg(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(ControlMessage_Destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	when_shall_nn_recv_fail = current_nn_recv_index +3;
	STRICT_EXPECTED_CALL(nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT))
		.IgnoreArgument(1).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(nn_errno()).SetReturn(EAGAIN);
	STRICT_EXPECTED_CALL(ThreadAPI_Sleep(250));
	//3rd pass: reset_channel fails (needs attach is still 1)
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	//restart control channel gets called and we bail right after
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1).SetReturn(LOCK_ERROR);

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1).SetReturn(LOCK_ERROR);



	// act
	//fourth thread created is control message thread
	thread_func_to_call[4](thread_func_args[4]);

	// assert 
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

TEST_FUNCTION(Outprocess_control_thread_restart_fails_bad_msg_then_reset_socket_fails)
{
	// arrange
	CONTROL_MESSAGE_MODULE_REPLY remote_died =
	{
		{ CONTROL_MESSAGE_VERSION_CURRENT,  CONTROL_MESSAGE_TYPE_MODULE_REPLY },
		(uint8_t)-1
	};
	global_control_msg.base.type = CONTROL_MESSAGE_TYPE_MODULE_REPLY;
	global_control_msg.base.version = CONTROL_MESSAGE_VERSION_CURRENT;
	((CONTROL_MESSAGE_MODULE_REPLY*)&global_control_msg)->status = 0;
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);
	umock_c_reset_all_calls();

	//1st pass: status is bad
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_recv(2, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT)).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(ControlMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.IgnoreAllArguments()
		.SetReturn((CONTROL_MESSAGE*)&remote_died);
	STRICT_EXPECTED_CALL(nn_freemsg(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(ControlMessage_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(ThreadAPI_Sleep(250));
	// 2nd pass:needs_to_attach is set, control socket fails.
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	//restart control channel
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(IGNORED_NUM_ARG)).IgnoreArgument(1);
	when_shall_nn_socket_fail = 1 + current_nn_socket_index;
	STRICT_EXPECTED_CALL(nn_socket(AF_SP, NN_PAIR));
	STRICT_EXPECTED_CALL(nn_errno());
	//then just bail
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1).SetReturn(LOCK_ERROR);

	// act
	//fourth thread created is control message thread
	thread_func_to_call[4](thread_func_args[4]);

	// assert 
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

TEST_FUNCTION(Outprocess_control_thread_restart_fails_bad_msg_then_reset_connect_fails)
{
	// arrange
	CONTROL_MESSAGE_MODULE_REPLY remote_died =
	{
		{ CONTROL_MESSAGE_VERSION_CURRENT,  CONTROL_MESSAGE_TYPE_MODULE_REPLY },
		(uint8_t)-1
	};
	global_control_msg.base.type = CONTROL_MESSAGE_TYPE_MODULE_REPLY;
	global_control_msg.base.version = CONTROL_MESSAGE_VERSION_CURRENT;
	((CONTROL_MESSAGE_MODULE_REPLY*)&global_control_msg)->status = 0;
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);
	umock_c_reset_all_calls();

	//1st pass: status is bad
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_recv(2, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT)).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(ControlMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.IgnoreAllArguments()
		.SetReturn((CONTROL_MESSAGE*)&remote_died);
	STRICT_EXPECTED_CALL(nn_freemsg(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(ControlMessage_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(ThreadAPI_Sleep(250));
	// 2nd pass:needs_to_attach is set, control socket fails.
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	//restart control channel
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(IGNORED_NUM_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_socket(AF_SP, NN_PAIR));
	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument(1);
	when_shall_nn_connect_fail = current_nn_connect_index + 1;
	STRICT_EXPECTED_CALL(nn_connect(IGNORED_NUM_ARG, IGNORED_PTR_ARG)).IgnoreAllArguments();
	STRICT_EXPECTED_CALL(nn_errno());
	STRICT_EXPECTED_CALL(nn_close(IGNORED_NUM_ARG)).IgnoreArgument(1);
	//then just bail
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1).SetReturn(LOCK_ERROR);

	// act
	//fourth thread created is control message thread
	thread_func_to_call[4](thread_func_args[4]);

	// assert 
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

TEST_FUNCTION(Outprocess_control_thread_restart_fails_bad_msg_then_reset_2nd_lock_fails)
{
	// arrange
	CONTROL_MESSAGE_MODULE_REPLY remote_died =
	{
		{ CONTROL_MESSAGE_VERSION_CURRENT,  CONTROL_MESSAGE_TYPE_MODULE_REPLY },
		(uint8_t)-1
	};
	global_control_msg.base.type = CONTROL_MESSAGE_TYPE_MODULE_REPLY;
	global_control_msg.base.version = CONTROL_MESSAGE_VERSION_CURRENT;
	((CONTROL_MESSAGE_MODULE_REPLY*)&global_control_msg)->status = 0;
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);
	umock_c_reset_all_calls();

	//1st pass: status is bad
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_recv(2, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT)).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(ControlMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.IgnoreAllArguments()
		.SetReturn((CONTROL_MESSAGE*)&remote_died);
	STRICT_EXPECTED_CALL(nn_freemsg(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(ControlMessage_Destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(ThreadAPI_Sleep(250));
	// 2nd pass:needs_to_attach is set, control socket fails.
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	//restart control channel
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_close(IGNORED_NUM_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_socket(AF_SP, NN_PAIR));
	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(nn_connect(IGNORED_NUM_ARG, IGNORED_PTR_ARG)).IgnoreAllArguments();
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1).SetReturn(LOCK_ERROR);
	STRICT_EXPECTED_CALL(nn_close(IGNORED_NUM_ARG)).IgnoreArgument(1);
	//then just bail
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1).SetReturn(LOCK_ERROR);

	// act
	//fourth thread created is control message thread
	thread_func_to_call[4](thread_func_args[4]);

	// assert 
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

TEST_FUNCTION(Outprocess_control_thread_dies_on_1st_unlock)
{
	// arrange
	global_control_msg.base.type = CONTROL_MESSAGE_TYPE_MODULE_REPLY;
	global_control_msg.base.version = CONTROL_MESSAGE_VERSION_CURRENT;
	((CONTROL_MESSAGE_MODULE_REPLY*)&global_control_msg)->status = 0;
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1).SetReturn(LOCK_ERROR);

	// act
	//fourth thread created is control message thread
	thread_func_to_call[4](thread_func_args[4]);

	// assert 
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

TEST_FUNCTION(Outprocess_control_thread_dies_on_2nd_unlock)
{
	// arrange
	global_control_msg.base.type = CONTROL_MESSAGE_TYPE_MODULE_REPLY;
	global_control_msg.base.version = CONTROL_MESSAGE_VERSION_CURRENT;
	((CONTROL_MESSAGE_MODULE_REPLY*)&global_control_msg)->status = 0;
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1).SetReturn(LOCK_ERROR);

	// act
	//fourth thread created is control message thread
	thread_func_to_call[4](thread_func_args[4]);

	// assert 
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

TEST_FUNCTION(Outprocess_control_thread_dies_nn_recv_unexpected_error)
{
	// arrange
	global_control_msg.base.type = CONTROL_MESSAGE_TYPE_MODULE_REPLY;
	global_control_msg.base.version = CONTROL_MESSAGE_VERSION_CURRENT;
	((CONTROL_MESSAGE_MODULE_REPLY*)&global_control_msg)->status = 0;
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	Module_Start(module);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).IgnoreArgument(1);
	should_nn_recv_fail = true;
	when_shall_nn_recv_fail = current_nn_recv_index +1;
	STRICT_EXPECTED_CALL(nn_recv(2, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT)).IgnoreArgument(2);
	STRICT_EXPECTED_CALL(nn_errno()).SetReturn(100);
	STRICT_EXPECTED_CALL(ThreadAPI_Sleep(250));

	// act
	//fourth thread created is control message thread
	thread_func_to_call[4](thread_func_args[4]);

	// assert 
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

TEST_FUNCTION(OutProcess_async_thread_null_input)
{
	// arrange
	global_control_msg.base.type = CONTROL_MESSAGE_TYPE_MODULE_REPLY;
	global_control_msg.base.version = CONTROL_MESSAGE_VERSION_CURRENT;
	((CONTROL_MESSAGE_MODULE_REPLY*)&global_control_msg)->status = 0;
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	config.lifecycle_model = OUTPROCESS_LIFECYCLE_ASYNC;

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	umock_c_reset_all_calls();

	// act
	thread_func_to_call[1](NULL);

	// assert 
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

TEST_FUNCTION(OutProcess_async_thread_1st_lock_fails)
{
	// arrange
	global_control_msg.base.type = CONTROL_MESSAGE_TYPE_MODULE_REPLY;
	global_control_msg.base.version = CONTROL_MESSAGE_VERSION_CURRENT;
	((CONTROL_MESSAGE_MODULE_REPLY*)&global_control_msg)->status = 0;
	OUTPROCESS_MODULE_CONFIG config;
	setup_create_config(&config);
	config.lifecycle_model = OUTPROCESS_LIFECYCLE_ASYNC;

	MODULE_HANDLE module = Module_Create((BROKER_HANDLE)0x42, &config);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).
		IgnoreArgument(1)
		.SetReturn(LOCK_ERROR);

	// act
	thread_func_to_call[1](thread_func_args[1]);

	// assert 
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//ablution
	Module_Destroy(module);
	cleanup_create_config(&config);
}

END_TEST_SUITE(OutprocessModule_UnitTests);
