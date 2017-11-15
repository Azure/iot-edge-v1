// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#ifdef __cplusplus
  #include <cstdbool>
  #include <cstdlib>
  #include <ctime>
#else
  #include <stdbool.h>
  #include <stdlib.h>
  #include <time.h>
#endif

#ifdef _CRTDBG_MAP_ALLOC
  #include <crtdbg.h>
#endif

// Test framework #includes
#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umocktypes_bool.h"
#include "umocktypes_charptr.h"
#include "umocktypes_stdint.h"
#include "umock_c_negative_tests.h"

#define disableNegativeTest(x) (negative_tests_to_skip |= ((uint64_t)1 << (x)))
#define enableNegativeTest(x) (negative_tests_to_skip &= ~((uint64_t)1 << (x)))
#define skipNegativeTest(x) (negative_tests_to_skip & ((uint64_t)1 << (x)))

#define GATEWAY_EXPORT_H
#define GATEWAY_EXPORT

static size_t negative_test_index;
static uint64_t negative_tests_to_skip;

// External library dependencies
#include <nanomsg/nn.h>
#include <nanomsg/pair.h>

static TEST_MUTEX_HANDLE g_dllByDll;
static TEST_MUTEX_HANDLE g_testByTest;

static
void *
non_mocked_calloc(
    size_t nmemb_,
    size_t size_
) {
    return calloc(nmemb_, size_);
}

static
void *
non_mocked_malloc(
    size_t size_
) {
    return malloc(size_);
}

static
void
non_mocked_free(
    void * block_
) {
    free(block_);
    return;
}

#define ENABLE_MOCKS
  // #include SDK dependencies here
  #include "azure_c_shared_utility/gballoc.h"
  #include "azure_c_shared_utility/lock.h"
  #include "azure_c_shared_utility/threadapi.h"
  #include "control_message.h"
  #include "message.h"
  #include "module.h"
#undef ENABLE_MOCKS

// Under test #includes
#include "broker.h"
#include "gateway.h"

#include "proxy_gateway.h"

#define MOCK_LOCK (LOCK_HANDLE)0x17091979
#define MOCK_MODULE (MODULE_HANDLE)0x09171979
#define MOCK_REMOTE_MODULE (REMOTE_MODULE_HANDLE)0x19790917

#ifdef __cplusplus
extern "C"
{
#endif

extern
int
connect_to_message_channel (
    REMOTE_MODULE_HANDLE remote_module,
    const MESSAGE_URI * channel_uri
);

extern
void
disconnect_from_message_channel (
    REMOTE_MODULE_HANDLE remote_module
);

extern
int
invoke_add_module_procedure (
    REMOTE_MODULE_HANDLE remote_module,
    const char * json_config
);

extern
int
process_module_create_message (
    REMOTE_MODULE_HANDLE remote_module,
    const CONTROL_MESSAGE_MODULE_CREATE * message
);

extern
int
send_control_reply (
    REMOTE_MODULE_HANDLE remote_module,
    uint8_t response
);

extern
int
worker_thread (
    void * thread_arg
);

#ifdef __cplusplus
}
#endif
static
char *
umockvalue_stringify_CONTROL_MESSAGE (
    const CONTROL_MESSAGE ** value_
) {
    char * result;

    if (NULL == value_) {
        result = (char *)NULL;
    } else if (NULL == *value_) {
        result = (char *)NULL;
    } else {
        char buffer[1024];
        size_t len;

        switch ((*value_)->type) {
          case CONTROL_MESSAGE_TYPE_MODULE_CREATE:
          {
            const CONTROL_MESSAGE_MODULE_CREATE * value = (CONTROL_MESSAGE_MODULE_CREATE *)*value_;
            len = sprintf(
                buffer,
                "CONTROL_MESSAGE_MODULE_CREATE {\n\t.base {\n\t\t.type: %u\n\t\t.version: %u\n\t}\n\t.gateway_message_version: %u\n\t.uri {\n\t\t.uri_type: %u\n\t\t.uri_size: %u\n\t\t.uri: %s\n\t}\n\t.args_size: %u\n\t.args: %s\n}\n",
                (uint8_t)value->base.type,
                (uint8_t)value->base.version,
                (uint8_t)value->gateway_message_version,
                (uint8_t)value->uri.uri_type,
                value->uri.uri_size,
                value->uri.uri,
                value->args_size,
                value->args
            );

            result = (char *)non_mocked_malloc(len + 1);
            strcpy(result, buffer);
            break;
          }
          case CONTROL_MESSAGE_TYPE_MODULE_REPLY:
          {
            const CONTROL_MESSAGE_MODULE_REPLY * value = (CONTROL_MESSAGE_MODULE_REPLY *)*value_;
            len = sprintf(
                buffer,
                "CONTROL_MESSAGE_MODULE_REPLY {\n\t.base {\n\t\t.type: %u\n\t\t.version: %u\n\t}\n\t.status: %u\n}\n",
                (uint8_t)value->base.type,
                (uint8_t)value->base.version,
                value->status
            );

            result = (char *)non_mocked_malloc(len + 1);
            strcpy(result, buffer);
            break;
          }
          case CONTROL_MESSAGE_TYPE_MODULE_DESTROY:
            len = sprintf(
                buffer,
                "CONTROL_MESSAGE_MODULE_DESTROY {\n\t.type: %u\n\t.version: %u\n}\n",
                (uint8_t)(*value_)->type,
                (uint8_t)(*value_)->version
            );

            result = (char *)non_mocked_malloc(len + 1);
            strcpy(result, buffer);
            break;
          case CONTROL_MESSAGE_TYPE_MODULE_START:
            len = sprintf(
                buffer,
                "CONTROL_MESSAGE_MODULE_START {\n\t.type: %u\n\t.version: %u\n}\n",
                (uint8_t)(*value_)->type,
                (uint8_t)(*value_)->version
            );

            result = (char *)non_mocked_malloc(len + 1);
            strcpy(result, buffer);
            break;
          default:
            result = (char *)NULL;
            break;
        }
    }

    return result;
}

static
int
umockvalue_are_equal_CONTROL_MESSAGE (
    const CONTROL_MESSAGE ** left_,
    const CONTROL_MESSAGE ** right_
) {
    bool match;

    if (NULL == left_) {
        match = false;
    } else if (NULL == right_) {
        match = false;
    } else if (NULL == *left_) {
        match = false;
    } else if (NULL == *right_) {
        match = false;
    } else {
        switch ((*left_)->type) {
          case CONTROL_MESSAGE_TYPE_MODULE_CREATE:
          {
            const CONTROL_MESSAGE_MODULE_CREATE * left = (CONTROL_MESSAGE_MODULE_CREATE *)*left_;
            const CONTROL_MESSAGE_MODULE_CREATE * right = (CONTROL_MESSAGE_MODULE_CREATE *)*right_;
            match = true;

            match = (match && (left->base.type == right->base.type));
            match = (match && (left->base.version == right->base.version));
            match = (match && (left->gateway_message_version == right->gateway_message_version));
            match = (match && (left->uri.uri_type == right->uri.uri_type));
            match = (match && (left->uri.uri_size == right->uri.uri_size));
            match = (match && (!strcmp(left->uri.uri, right->uri.uri)));
            match = (match && (left->args_size == right->args_size));
            match = (match && (!strcmp(left->args, right->args)));
            break;
          }
          case CONTROL_MESSAGE_TYPE_MODULE_REPLY:
          {
            const CONTROL_MESSAGE_MODULE_REPLY * left = (CONTROL_MESSAGE_MODULE_REPLY *)*left_;
            const CONTROL_MESSAGE_MODULE_REPLY * right = (CONTROL_MESSAGE_MODULE_REPLY *)*right_;
            match = true;

            match = (match && (left->base.type == right->base.type));
            match = (match && (left->base.version == right->base.version));
            match = (match && (left->status == right->status));
            break;
          }
          case CONTROL_MESSAGE_TYPE_MODULE_DESTROY:
          case CONTROL_MESSAGE_TYPE_MODULE_START:
          default:
            match = true;

            match = (match && ((*left_)->type == (*right_)->type));
            match = (match && ((*left_)->version == (*right_)->version));
            break;
        }
    }

    return (int)match;
}

static
int
umockvalue_copy_CONTROL_MESSAGE (
    CONTROL_MESSAGE ** destination_,
    const CONTROL_MESSAGE ** source_
) {
    int result;

    if (NULL == destination_) {
        result = __LINE__;
    } else if (NULL == source_) {
        result = __LINE__;
    } else if (NULL == *source_) {
        (*destination_) = (CONTROL_MESSAGE *)(*source_);
        result = 0;
    } else {
        switch ((*source_)->type) {
          case CONTROL_MESSAGE_TYPE_MODULE_CREATE:
            if (NULL == (*destination_ = (CONTROL_MESSAGE *)non_mocked_malloc(sizeof(CONTROL_MESSAGE_MODULE_CREATE)))) {
                result = __LINE__;
            } else {
                CONTROL_MESSAGE_MODULE_CREATE * destination = (CONTROL_MESSAGE_MODULE_CREATE *)*destination_;
                const CONTROL_MESSAGE_MODULE_CREATE * source = (const CONTROL_MESSAGE_MODULE_CREATE *)*source_;

                if (NULL == destination) {
                    result = __LINE__;
                } else if (NULL == source) {
                    result = __LINE__;
                } else {
                    destination->base.type = source->base.type;
                    destination->base.version = source->base.version;
                    destination->gateway_message_version = source->gateway_message_version;
                    destination->uri.uri_type = source->uri.uri_type;
                    destination->uri.uri_size = source->uri.uri_size;
                    destination->uri.uri = (char *)non_mocked_malloc(source->uri.uri_size);
                    strcpy(destination->uri.uri, source->uri.uri);
                    destination->args_size = source->args_size;
                    destination->args = (char *)non_mocked_malloc(source->args_size);
                    strcpy(destination->args, source->args);
                    result = 0;
                }
            }
            break;
          case CONTROL_MESSAGE_TYPE_MODULE_REPLY:
            if (NULL == (*destination_ = (CONTROL_MESSAGE *)non_mocked_malloc(sizeof(CONTROL_MESSAGE_MODULE_REPLY)))) {
                result = __LINE__;
            } else {
                CONTROL_MESSAGE_MODULE_REPLY * destination = (CONTROL_MESSAGE_MODULE_REPLY *)*destination_;
                const CONTROL_MESSAGE_MODULE_REPLY * source = (const CONTROL_MESSAGE_MODULE_REPLY *)*source_;

                if (NULL == destination) {
                    result = __LINE__;
                } else if (NULL == source) {
                    result = __LINE__;
                } else {
                    destination->base.type = source->base.type;
                    destination->base.version = source->base.version;
                    destination->status = source->status;
                    result = 0;
                }
            }
            break;
          case CONTROL_MESSAGE_TYPE_MODULE_DESTROY:
          case CONTROL_MESSAGE_TYPE_MODULE_START:
          default:
            if (NULL == (*destination_ = (CONTROL_MESSAGE *)non_mocked_malloc(sizeof(CONTROL_MESSAGE)))) {
                result = __LINE__;
            } else {
                if (NULL == (*destination_)) {
                    result = __LINE__;
                } else if (NULL == (*source_)) {
                    result = __LINE__;
                } else {
                    (*destination_)->type = (*source_)->type;
                    (*destination_)->version = (*source_)->version;
                    result = 0;
                }
            }
        }
    }
    return result;
}

static
void
umockvalue_free_CONTROL_MESSAGE (
    CONTROL_MESSAGE ** value_
) {
    if (NULL == value_) {
    } else if (NULL == *value_) {
    } else {
        switch ((*value_)->type) {
          case CONTROL_MESSAGE_TYPE_MODULE_CREATE:
          {
            CONTROL_MESSAGE_MODULE_CREATE * value = (CONTROL_MESSAGE_MODULE_CREATE *)*value_;
            non_mocked_free(value->uri.uri);
            non_mocked_free(value->args);
            non_mocked_free(value);
            break;
          }
          case CONTROL_MESSAGE_TYPE_MODULE_DESTROY:
          case CONTROL_MESSAGE_TYPE_MODULE_REPLY:
          case CONTROL_MESSAGE_TYPE_MODULE_START:
          default:
            non_mocked_free(*value_);
            break;
        }
    }
}

// define free mocked function(s) and enum type(s) (platform, external libraries, etc.)

MOCK_FUNCTION_WITH_CODE(, void *, nn_allocmsg, size_t, size, int, type)
MOCK_FUNCTION_END(NULL)

MOCK_FUNCTION_WITH_CODE(, int, nn_bind, int, s, const char *, addr)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, nn_close, int, s)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, nn_errno)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, nn_freemsg, void *, msg)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, nn_recv, int, s, void *, buf, size_t, len, int, flags)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, nn_send, int, s, const void *, buf, size_t, len, int, flags)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, nn_shutdown, int, s, int, how)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, nn_socket, int, domain, int, protocol)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, MODULE_HANDLE, mock_create, BROKER_HANDLE, broker, const void *, configuration)
MOCK_FUNCTION_END(MOCK_MODULE)

MOCK_FUNCTION_WITH_CODE(, void, mock_destroy, MODULE_HANDLE, moduleHandle)
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void, mock_freeConfiguration, void *, configuration)
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void *, mock_parseConfigurationFromJson, const char *, configuration)
MOCK_FUNCTION_END((void *)configuration)

MOCK_FUNCTION_WITH_CODE(, void, mock_receive, MODULE_HANDLE, moduleHandle, MESSAGE_HANDLE, messageHandle)
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void, mock_start, MODULE_HANDLE, moduleHandle)
MOCK_FUNCTION_END()


static const MODULE_API_1 MOCK_MODULE_APIS = {
    { MODULE_API_VERSION_1 },
    mock_parseConfigurationFromJson,
    mock_freeConfiguration,
    mock_create,
    mock_destroy,
    mock_receive,
    mock_start
};

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static
void
expected_calls_connect_to_message_channel (
    const MESSAGE_URI * message_uri
) {
    static const int COMMAND_ENDPOINT = 917;
    static const int COMMAND_SOCKET = 1979;

    enableNegativeTest(negative_test_index++);
    STRICT_EXPECTED_CALL(nn_socket(AF_SP, message_uri->uri_type))
        .SetFailReturn(-1)
        .SetReturn(COMMAND_SOCKET);
    enableNegativeTest(negative_test_index++);
    STRICT_EXPECTED_CALL(nn_bind(COMMAND_SOCKET, message_uri->uri))
        .SetFailReturn(-1)
        .SetReturn(COMMAND_ENDPOINT);
}

static
void
expected_calls_disconnect_from_message_channel (
    void
) {
    disableNegativeTest(negative_test_index++);
    EXPECTED_CALL(nn_shutdown(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    disableNegativeTest(negative_test_index++);
    EXPECTED_CALL(nn_close(IGNORED_NUM_ARG));
}

static
void
expected_calls_invoke_add_module_procedure (
    REMOTE_MODULE_HANDLE remote_module,
    const CONTROL_MESSAGE_MODULE_CREATE * create_message
) {
    static const void * CREATE_PARAMETERS = (void *)0xEBADF00D;

    disableNegativeTest(negative_test_index++);
    STRICT_EXPECTED_CALL(mock_parseConfigurationFromJson(create_message->args))
        .SetReturn((void *)CREATE_PARAMETERS);
    enableNegativeTest(negative_test_index++);
    STRICT_EXPECTED_CALL(mock_create((BROKER_HANDLE)remote_module, CREATE_PARAMETERS))
        .SetFailReturn(NULL)
        .SetReturn(MOCK_MODULE);
    disableNegativeTest(negative_test_index++);
    STRICT_EXPECTED_CALL(mock_freeConfiguration((void *)CREATE_PARAMETERS));
}

static
void
expected_calls_send_control_reply (
    const CONTROL_MESSAGE_MODULE_REPLY * reply
) {
    static void * ALLOCATED_MEMORY_PTR = (void *)0xEBADF00D;
    static const int32_t MESSAGE_SIZE = 1979;

    enableNegativeTest(negative_test_index++);
    STRICT_EXPECTED_CALL(ControlMessage_ToByteArray((CONTROL_MESSAGE *)reply, NULL, 0))
        .SetFailReturn(-1)
        .SetReturn(MESSAGE_SIZE);
    enableNegativeTest(negative_test_index++);
    STRICT_EXPECTED_CALL(nn_allocmsg(MESSAGE_SIZE, 0))
        .SetFailReturn(NULL)
        .SetReturn(ALLOCATED_MEMORY_PTR);
    enableNegativeTest(negative_test_index++);
    STRICT_EXPECTED_CALL(ControlMessage_ToByteArray((CONTROL_MESSAGE *)reply, (unsigned char *)ALLOCATED_MEMORY_PTR, MESSAGE_SIZE))
        .SetFailReturn(-1)
        .SetReturn(MESSAGE_SIZE);
    enableNegativeTest(negative_test_index++);
    STRICT_EXPECTED_CALL(nn_send(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1)
        .SetReturn(MESSAGE_SIZE);
}

static
void
expected_calls_process_module_create_message (
    REMOTE_MODULE_HANDLE remote_module,
    const CONTROL_MESSAGE_MODULE_CREATE * create_message,
    const CONTROL_MESSAGE_MODULE_REPLY * reply
) {
    expected_calls_connect_to_message_channel((const MESSAGE_URI *)&create_message->uri);
    expected_calls_invoke_add_module_procedure(remote_module, create_message);
    expected_calls_send_control_reply(reply);
}

static
void
on_umock_c_error (
    UMOCK_C_ERROR_CODE error_code
) {
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

BEGIN_TEST_SUITE(proxy_gateway_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    result = umock_c_init(on_umock_c_error);
    ASSERT_ARE_EQUAL(int, 0, result);
    result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);
    result = umocktypes_stdint_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    umocktypes_bool_register_types();

	REGISTER_UMOCK_ALIAS_TYPE(BROKER_HANDLE, void *);
	REGISTER_UMOCK_ALIAS_TYPE(BROKER_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(LOCK_HANDLE, void *);
    REGISTER_UMOCK_ALIAS_TYPE(LOCK_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_HANDLE, void *);
    REGISTER_UMOCK_ALIAS_TYPE(MODULE_HANDLE, void *);
    REGISTER_UMOCK_ALIAS_TYPE(REMOTE_MODULE_HANDLE, void *);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_HANDLE, void *);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_START_FUNC, void *);
    REGISTER_UMOCK_ALIAS_TYPE(THREADAPI_RESULT, int);

    //REGISTER_UMOCKC_PAIRED_CREATE_DESTROY_CALLS(ControlMessage_Create, ControlMessage_Destroy);
    //REGISTER_UMOCKC_PAIRED_CREATE_DESTROY_CALLS(Message_Create, Message_Destroy);
    //REGISTER_UMOCKC_PAIRED_CREATE_DESTROY_CALLS(mock_parseConfigurationFromJson, mock_freeConfiguration);
    //REGISTER_UMOCKC_PAIRED_CREATE_DESTROY_CALLS(ProxyGateway_Attach, ProxyGateway_Detach);

    REGISTER_UMOCK_VALUE_TYPE(CONTROL_MESSAGE *, umockvalue_stringify_CONTROL_MESSAGE, umockvalue_are_equal_CONTROL_MESSAGE, umockvalue_copy_CONTROL_MESSAGE, umockvalue_free_CONTROL_MESSAGE);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_calloc, non_mocked_calloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, non_mocked_free);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, non_mocked_malloc);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
    TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }
    negative_test_index = 0;
    negative_tests_to_skip = 0;
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)

{
    TEST_MUTEX_RELEASE(g_testByTest);
}

/* Tests_SRS_PROXY_GATEWAY_027_000: [Prerequisite Check - If the `module_apis` parameter is `NULL`, then `ProxyGateway_Attach` shall do nothing and return `NULL`] */
TEST_FUNCTION(attach_SCENARIO_NULL_module_apis)
{
    // Arrange
    REMOTE_MODULE_HANDLE remote_module;

    // Expected call listing
    umock_c_reset_all_calls();

    // Act
    remote_module = ProxyGateway_Attach((MODULE_API *)NULL, "proxy_gateway_ut");

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(remote_module);

    // Cleanup
}

/* Tests_SRS_PROXY_GATEWAY_027_001: [Prerequisite Check - If the `module_apis` version is beyond `MODULE_API_VERSION_1`, then `ProxyGateway_Attach` shall do nothing and return `NULL`] */
TEST_FUNCTION(attach_SCENARIO_incompatible_module_apis)
{
    // Arrange
    static const MODULE_API_1 MODULE_APIS = {
        { (MODULE_API_VERSION)(MODULE_API_VERSION_1 + 1) }, // MODULE_API_VERSION_NEXT
        mock_parseConfigurationFromJson,
        mock_freeConfiguration,
        mock_create,
        mock_destroy,
        mock_receive,
        mock_start
    };

    REMOTE_MODULE_HANDLE remote_module;

    // Expected call listing
    umock_c_reset_all_calls();

    // Act
    remote_module = ProxyGateway_Attach((MODULE_API *)&MODULE_APIS, "proxy_gateway_ut");

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(remote_module);

    // Cleanup
}

/* Tests_SRS_PROXY_GATEWAY_027_002: [Prerequisite Check - If the `module_apis` interface fails to provide `Module_Create`, then `ProxyGateway_Attach` shall do nothing and return `NULL`] */
TEST_FUNCTION(attach_SCENARIO_module_apis_NULL_create)
{
    // Arrange
    static const MODULE_API_1 MODULE_APIS = {
        { MODULE_API_VERSION_1 },
        mock_parseConfigurationFromJson,
        mock_freeConfiguration,
        (pfModule_Create)NULL,
        mock_destroy,
        mock_receive,
        mock_start
    };

    REMOTE_MODULE_HANDLE remote_module;

    // Expected call listing
    umock_c_reset_all_calls();

    // Act
    remote_module = ProxyGateway_Attach((MODULE_API *)&MODULE_APIS, "proxy_gateway_ut");

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(remote_module);

    // Cleanup
}

/* Tests_SRS_PROXY_GATEWAY_027_003: [Prerequisite Check - If the `module_apis` interface fails to provide `Module_Destroy`, then `ProxyGateway_Attach` shall do nothing and return `NULL`] */
TEST_FUNCTION(attach_SCENARIO_module_apis_NULL_destroy)
{
    // Arrange
    static const MODULE_API_1 MODULE_APIS = {
        { MODULE_API_VERSION_1 },
        mock_parseConfigurationFromJson,
        mock_freeConfiguration,
        mock_create,
        (pfModule_Destroy)NULL,
        mock_receive,
        mock_start
    };

    REMOTE_MODULE_HANDLE remote_module;

    // Expected call listing
    umock_c_reset_all_calls();

    // Act
    remote_module = ProxyGateway_Attach((MODULE_API *)&MODULE_APIS, "proxy_gateway_ut");

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(remote_module);

    // Cleanup
}

/* Tests_SRS_PROXY_GATEWAY_027_004: [Prerequisite Check - If the `module_apis` interface fails to provide `Module_Receive`, then `ProxyGateway_Attach` shall do nothing and return `NULL`] */
TEST_FUNCTION(attach_SCENARIO_module_apis_NULL_receive)
{
    // Arrange
    static const MODULE_API_1 MODULE_APIS = {
        { MODULE_API_VERSION_1 },
        mock_parseConfigurationFromJson,
        mock_freeConfiguration,
        mock_create,
        mock_destroy,
        (pfModule_Receive)NULL,
        mock_start
    };

    REMOTE_MODULE_HANDLE remote_module;

    // Expected call listing
    umock_c_reset_all_calls();

    // Act
    remote_module = ProxyGateway_Attach((MODULE_API *)&MODULE_APIS, "proxy_gateway_ut");

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(remote_module);

    // Cleanup
}

/* Tests_SRS_PROXY_GATEWAY_027_005: [Prerequisite Check - If the `connection_id` parameter is `NULL`, then `ProxyGateway_Attach` shall do nothing and return `NULL`] */
TEST_FUNCTION(attach_SCENARIO_NULL_connection_id)
{
    // Arrange
    REMOTE_MODULE_HANDLE remote_module;

    // Expected call listing
    umock_c_reset_all_calls();

    // Act
    remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, (const char *)NULL);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(remote_module);

    // Cleanup
}

/* Tests_SRS_PROXY_GATEWAY_027_006: [Prerequisite Check - If the `connection_id` parameter is longer than `GATEWAY_CONNECTION_ID_MAX`, then `ProxyGateway_Attach` shall do nothing and return `NULL`] */
TEST_FUNCTION(attach_SCENARIO_connection_id_too_long)
{
    // Arrange
    REMOTE_MODULE_HANDLE remote_module;

    // Expected call listing
    umock_c_reset_all_calls();

    // Act
    remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "AConnectionStringNeedsToBeARidiculouslyLongStringToSurpassTheAmountOfSpaceSupportedByTheNanomsgLibrarySoIfITryJustALittleBitHarderIShouldBeAbleToExceedTheLengthLimitationQuiteEasily");

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(remote_module);

    // Cleanup
}

/* Tests_SRS_PROXY_GATEWAY_027_007: [`ProxyGateway_Attach` shall allocate the memory required to support its instance data] */
/* Tests_SRS_PROXY_GATEWAY_027_009: [`ProxyGateway_Attach` shall allocate the memory required to formulate the connection string to the Azure IoT Gateway] */
/* Tests_SRS_PROXY_GATEWAY_027_011: [`ProxyGateway_Attach` shall create a socket for the Azure IoT Gateway control channel by calling `int nn_socket(int domain, int protocol)` with `AF_SP` as the `domain` and `NN_PAIR` as the `protocol`] */
/* Tests_SRS_PROXY_GATEWAY_027_013: [`ProxyGateway_Attach` shall bind to the Azure IoT Gateway control channel by calling `int nn_bind(int s, const char * addr)` with the newly created socket as `s` and the newly formulated connection string as `addr`] */
/* Tests_SRS_PROXY_GATEWAY_027_015: [`ProxyGateway_Attach` shall release the memory required to formulate the connection string] */
/* Tests_SRS_PROXY_GATEWAY_027_016: [If no errors are encountered, then `ProxyGateway_Attach` return a handle to the remote module instance] */
TEST_FUNCTION(attach_SCENARIO_success)
{
    // Arrange
    static const int COMMAND_ENDPOINT = 917;
    static const int COMMAND_SOCKET = 1979;
    static const char CONTROL_CHANNEL_URI[] = "ipc://proxy_gateway_ut";
    const MODULE_API_1 module_apis = {
        { MODULE_API_VERSION_1 },
        mock_parseConfigurationFromJson,
        mock_freeConfiguration,
        mock_create,
        mock_destroy,
        mock_receive,
        mock_start
    };

    REMOTE_MODULE_HANDLE remote_module;

    // Expected call listing
    umock_c_reset_all_calls();
    EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(CONTROL_CHANNEL_URI)));
    STRICT_EXPECTED_CALL(nn_socket(AF_SP, NN_PAIR))
        .SetReturn(COMMAND_SOCKET);
    STRICT_EXPECTED_CALL(nn_bind(COMMAND_SOCKET, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(COMMAND_ENDPOINT)
        .ValidateArgumentBuffer(2, CONTROL_CHANNEL_URI, (sizeof(CONTROL_CHANNEL_URI) - 1));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    // Act
    remote_module = ProxyGateway_Attach((MODULE_API *)&module_apis, "proxy_gateway_ut");

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(remote_module);

    // Cleanup
    ProxyGateway_Detach(remote_module);
}

/* Tests_SRS_PROXY_GATEWAY_027_008: [If memory allocation fails for the instance data, then `ProxyGateway_Attach` shall return `NULL`] */
/* Tests_SRS_PROXY_GATEWAY_027_010: [If memory allocation fails for the connection string, then `ProxyGateway_Attach` shall free any previously allocated memory and return `NULL`] */
/* Tests_SRS_PROXY_GATEWAY_027_012: [If the call to `nn_socket` returns -1, then `ProxyGateway_Attach` shall free any previously allocated memory and return `NULL`] */
/* Tests_SRS_PROXY_GATEWAY_027_014: [If the call to `nn_bind` returns a negative value, then `ProxyGateway_Attach` shall close the socket, free any previously allocated memory and return `NULL`] */
TEST_FUNCTION(attach_SCENARIO_negative_tests)
{
    // Arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    static const int COMMAND_ENDPOINT = 917;
    static const int COMMAND_SOCKET = 1979;
    static const char CONTROL_CHANNEL_URI[] = "ipc://proxy_gateway_ut";

    REMOTE_MODULE_HANDLE remote_module;

    // Expected call listing
    umock_c_reset_all_calls();
    enableNegativeTest(negative_test_index++);
    EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG))
        .SetFailReturn(NULL);
    enableNegativeTest(negative_test_index++);
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(CONTROL_CHANNEL_URI)))
        .SetFailReturn(NULL);
    enableNegativeTest(negative_test_index++);
    STRICT_EXPECTED_CALL(nn_socket(AF_SP, NN_PAIR))
        .SetFailReturn(-1)
        .SetReturn(COMMAND_SOCKET);
    enableNegativeTest(negative_test_index++);
    STRICT_EXPECTED_CALL(nn_bind(COMMAND_SOCKET, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetFailReturn(-2)
        .SetReturn(COMMAND_ENDPOINT)
        .ValidateArgumentBuffer(2, CONTROL_CHANNEL_URI, (sizeof(CONTROL_CHANNEL_URI) - 1));
    disableNegativeTest(negative_test_index++);
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    ASSERT_ARE_EQUAL(int, negative_test_index, umock_c_negative_tests_call_count());
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); ++i) {
        if ( skipNegativeTest(i) ) {
            printf("%s: Skipping negative tests: %zx\n", __FUNCTION__, i);
            continue;
        }
        printf("%s: Running negative tests: %zx\n", __FUNCTION__, i);
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // Act
        remote_module = ProxyGateway_Attach(NULL, "proxy_gateway_ut");

        // Assert
        ASSERT_IS_NULL(remote_module);
    }

    // Cleanup
    umock_c_negative_tests_deinit();
}

/* Tests_SRS_PROXY_GATEWAY_027_017: [Prerequisite Check - If the `remote_module` parameter is `NULL`, then `ProxyGateway_StartWorkerThread` shall do nothing and return a non-zero value] */
TEST_FUNCTION(startWorkerThread_SCENARIO_NULL_handle)
{
    // Arrange
    int result;

    // Expected call listing
    umock_c_reset_all_calls();

    // Act
    result = ProxyGateway_StartWorkerThread(NULL);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // Cleanup
}

/* Tests_SRS_PROXY_GATEWAY_027_018: [Prerequisite Check - If a work thread already exist for the given handle, then `ProxyGateway_StartWorkerThread` shall do nothing and return zero] */
TEST_FUNCTION(startWorkerThread_SCENARIO_threadAlreadyStarted)
{
    // Arrange
    int result;
    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(Lock_Init())
        .SetReturn(MOCK_LOCK);
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(THREADAPI_OK);

    // Act
    result = ProxyGateway_StartWorkerThread(remote_module);
    ASSERT_ARE_EQUAL(int, 0, result);
    result = ProxyGateway_StartWorkerThread(remote_module);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // Cleanup
    //TODO: ProxyGateway_HaltWorkerThread(remote_module);
    ProxyGateway_Detach(remote_module);
}


/* Tests_SRS_PROXY_GATEWAY_027_019: [`ProxyGateway_StartWorkerThread` shall allocate the memory required to support the worker thread] */
/* Tests_SRS_PROXY_GATEWAY_027_021: [`ProxyGateway_StartWorkerThread` shall create a mutex by calling `LOCK_HANDLE Lock_Init(void)`] */
/* Tests_SRS_PROXY_GATEWAY_027_023: [`ProxyGateway_StartWorkerThread` shall start a worker thread by calling `THREADAPI_RESULT ThreadAPI_Create(&THREAD_HANDLE threadHandle, THREAD_START_FUNC func, void * arg)` with an empty thread handle for `threadHandle`, a function that loops polling the messages for `func`, and `remote_module` for `arg`] */
/* Tests_SRS_PROXY_GATEWAY_027_025: [If no errors are encountered, then `ProxyGateway_StartWorkerThread` shall return zero] */
TEST_FUNCTION(startWorkerThread_SCENARIO_success)
{
    // Arrange
    int result;
    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(Lock_Init())
        .SetReturn(MOCK_LOCK);
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(THREADAPI_OK);

    // Act
    result = ProxyGateway_StartWorkerThread(remote_module);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // Cleanup
    //TODO: ProxyGateway_HaltWorkerThread(remote_module);
    ProxyGateway_Detach(remote_module);
}

/* Tests_SRS_PROXY_GATEWAY_027_020: [If memory allocation fails for the worker thread data, then `ProxyGateway_StartWorkerThread` shall return a non-zero value] */
/* Tests_SRS_PROXY_GATEWAY_027_022: [If a mutex is unable to be created, then `ProxyGateway_StartWorkerThread` shall free any previously allocated memory and return a non-zero value] */
/* Tests_SRS_PROXY_GATEWAY_027_024: [If the worker thread failed to start, then `ProxyGateway_StartWorkerThread` shall free any previously allocated memory and return a non-zero value] */
TEST_FUNCTION(startWorkerThread_SCENARIO_negative_tests)
{
    // Arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    int result;
    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    enableNegativeTest(negative_test_index++);
    EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG))
        .SetFailReturn(NULL);
    enableNegativeTest(negative_test_index++);
    EXPECTED_CALL(Lock_Init())
        .SetFailReturn(NULL)
        .SetReturn(MOCK_LOCK);
    enableNegativeTest(negative_test_index++);
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetFailReturn(THREADAPI_ERROR)
        .SetReturn(THREADAPI_OK);
    umock_c_negative_tests_snapshot();

    ASSERT_ARE_EQUAL(int, negative_test_index, umock_c_negative_tests_call_count());
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); ++i) {
        if (skipNegativeTest(i)) {
            printf("%s: Skipping negative tests: %zx\n", __FUNCTION__, i);
            continue;
        }
        printf("%s: Running negative tests: %zx\n", __FUNCTION__, i);
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // Act
        result = ProxyGateway_StartWorkerThread(remote_module);

        // Assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
    }

    // Cleanup
    ProxyGateway_Detach(remote_module);
    umock_c_negative_tests_deinit();
}

/* Tests_SRS_PROXY_GATEWAY_027_026: [Prerequisite Check - If the `remote_module` parameter is `NULL`, then `ProxyGateway_DoWork` shall do nothing] */
TEST_FUNCTION(doWork_SCENARIO_NULL_handle)
{
    // Arrange

    // Expected call listing
    umock_c_reset_all_calls();

    // Act
    ProxyGateway_DoWork(NULL);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
}

/* Tests_SRS_PROXY_GATEWAY_027_027: [Control Channel - `ProxyGateway_DoWork` shall poll the gateway control channel by calling `int nn_recv(int s, void * buf, size_t len, int flags)` with the control socket for `s`, `NULL` for `buf`, `NN_MSG` for `len` and NN_DONTWAIT for `flags`] */
/* Tests_SRS_PROXY_GATEWAY_027_029: [Control Channel - If a control message was received, then `ProxyGateway_DoWork` will parse that message by calling `CONTROL_MESSAGE * ControlMessage_CreateFromByteArray(const unsigned char * source, size_t size)` with the buffer received from `nn_recv` as `source` and return value from `nn_recv` as `size`] */
/* Tests_SRS_PROXY_GATEWAY_027_031: [Control Channel - If the message type is CONTROL_MESSAGE_TYPE_MODULE_CREATE, then `ProxyGateway_DoWork` shall process the create message] */
/* Tests_SRS_PROXY_GATEWAY_027_035: [Control Channel - `ProxyGateway_DoWork` shall free the resources held by the parsed control message by calling `void ControlMessage_Destroy(CONTROL_MESSAGE * message)` using the parsed control message as `message`] */
/* Tests_SRS_PROXY_GATEWAY_027_036: [Control Channel - `ProxyGateway_DoWork` shall free the resources held by the gateway message by calling `int nn_freemsg(void * msg)` with the resulting buffer from the previous call to `nn_recv`] */
/* Tests_SRS_PROXY_GATEWAY_027_038: [Message Channel - `ProxyGateway_DoWork` shall poll the gateway message channel by calling `int nn_recv(int s, void * buf, size_t len, int flags)` with each message socket for `s`, `NULL` for `buf`, `NN_MSG` for `len` and NN_DONTWAIT for `flags`] */
/* Tests_SRS_PROXY_GATEWAY_027_040: [Message Channel - If a module message was received, then `ProxyGateway_DoWork` will parse that message by calling `MESSAGE_HANDLE Message_CreateFromByteArray(const unsigned char * source, int32_t size)` with the buffer received from `nn_recv` as `source` and return value from `nn_recv` as `size`] */
/* Tests_SRS_PROXY_GATEWAY_027_042: [Message Channel - `ProxyGateway_DoWork` shall pass the structured message to the module by calling `void Module_Receive(MODULE_HANDLE moduleHandle)` using the parsed message as `moduleHandle`] */
/* Tests_SRS_PROXY_GATEWAY_027_043: [Message Channel - `ProxyGateway_DoWork` shall free the resources held by the parsed module message by calling `void Message_Destroy(MESSAGE_HANDLE * message)` using the parsed module message as `message`] */
/* Tests_SRS_PROXY_GATEWAY_027_044: [Message Channel - `ProxyGateway_DoWork` shall free the resources held by the gateway message by calling `int nn_freemsg(void * msg)` with the resulting buffer from the previous call to `nn_recv`] */
TEST_FUNCTION(doWork_SCENARIO_create_message_success)
{
    // Arrange
	static const int COMMAND_SOCKET = 1979;

    CONTROL_MESSAGE_MODULE_CREATE CREATE_MESSAGE = {
        {
            CONTROL_MESSAGE_VERSION_CURRENT,
            CONTROL_MESSAGE_TYPE_MODULE_CREATE
        },
        GATEWAY_MESSAGE_VERSION_CURRENT,
        {
            sizeof("ipc://message_channel"),
            NN_PAIR,
            "ipc://message_channel"
        },
        sizeof("json_encoded_remote_module_parameters"),
        "json_encoded_remote_module_parameters"
    };
    static const void * NN_MESSAGE_BUFFER = (void *)0xEBADF00D;
    static const int32_t NN_MESSAGE_SIZE = 1979;
    static const CONTROL_MESSAGE_MODULE_REPLY REPLY = {
        {
            CONTROL_MESSAGE_VERSION_1,
            CONTROL_MESSAGE_TYPE_MODULE_REPLY
        },
        0
    };
	EXPECTED_CALL(gballoc_calloc(1, IGNORED_NUM_ARG));
	EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
	EXPECTED_CALL(nn_socket(AF_SP, NN_PAIR)).SetReturn(COMMAND_SOCKET);
	EXPECTED_CALL(nn_bind(IGNORED_NUM_ARG, IGNORED_PTR_ARG)).SetReturn(1);
	EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT))
        .CopyOutArgumentBuffer(2, &NN_MESSAGE_BUFFER, sizeof(void *))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(NN_MESSAGE_SIZE);
    STRICT_EXPECTED_CALL(ControlMessage_CreateFromByteArray((const unsigned char *)NN_MESSAGE_BUFFER, IGNORED_NUM_ARG))
        .IgnoreArgument(2)
        .SetReturn((CONTROL_MESSAGE *)&CREATE_MESSAGE);
    expected_calls_process_module_create_message(remote_module, &CREATE_MESSAGE, &REPLY);
    STRICT_EXPECTED_CALL(ControlMessage_Destroy((CONTROL_MESSAGE *)&CREATE_MESSAGE));
    STRICT_EXPECTED_CALL(nn_freemsg((void *)NN_MESSAGE_BUFFER));
    STRICT_EXPECTED_CALL(nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT))
        .CopyOutArgumentBuffer(2, &NN_MESSAGE_BUFFER, sizeof(void *))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(NN_MESSAGE_SIZE);
    STRICT_EXPECTED_CALL(Message_CreateFromByteArray((const unsigned char *)NN_MESSAGE_BUFFER, IGNORED_NUM_ARG))
        .IgnoreArgument(2)
        .SetReturn((MESSAGE_HANDLE)&CREATE_MESSAGE);
    STRICT_EXPECTED_CALL(mock_receive(MOCK_MODULE, (MESSAGE_HANDLE)&CREATE_MESSAGE));
    STRICT_EXPECTED_CALL(Message_Destroy((MESSAGE_HANDLE)&CREATE_MESSAGE));
    STRICT_EXPECTED_CALL(nn_freemsg((void *)NN_MESSAGE_BUFFER));

    // Act
    ProxyGateway_DoWork(remote_module);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    ProxyGateway_Detach(remote_module);
}

/* Tests_SRS_PROXY_GATEWAY_027_032: [Control Channel - If the message type is CONTROL_MESSAGE_TYPE_MODULE_START and `Module_Start` was provided, then `ProxyGateway_DoWork` shall call `void Module_Start(MODULE_HANDLE moduleHandle)`] */
TEST_FUNCTION(doWork_SCENARIO_start_message_success)
{
    // Arrange
    CONTROL_MESSAGE_MODULE_CREATE CREATE_MESSAGE = {
        {
            CONTROL_MESSAGE_VERSION_CURRENT,
            CONTROL_MESSAGE_TYPE_MODULE_CREATE
        },
        GATEWAY_MESSAGE_VERSION_CURRENT,
        {
            sizeof("ipc://message_channel"),
            NN_PAIR,
            "ipc://message_channel"
        },
        sizeof("json_encoded_remote_module_parameters"),
        "json_encoded_remote_module_parameters"
    };
    static const void * NN_MESSAGE_BUFFER = (void *)0xEBADF00D;
    static const int32_t NN_MESSAGE_SIZE = 1979;
    static const CONTROL_MESSAGE_MODULE_REPLY REPLY = {
        {
            CONTROL_MESSAGE_VERSION_1,
            CONTROL_MESSAGE_TYPE_MODULE_REPLY
        },
        0
    };
    static const CONTROL_MESSAGE START_MESSAGE = {
        CONTROL_MESSAGE_VERSION_CURRENT,
        CONTROL_MESSAGE_TYPE_MODULE_START
    };

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT))
        .CopyOutArgumentBuffer(2, &NN_MESSAGE_BUFFER, sizeof(void *))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(NN_MESSAGE_SIZE);
    STRICT_EXPECTED_CALL(ControlMessage_CreateFromByteArray((const unsigned char *)NN_MESSAGE_BUFFER, IGNORED_NUM_ARG))
        .IgnoreArgument(2)
        .SetReturn((CONTROL_MESSAGE *)&CREATE_MESSAGE);
    expected_calls_process_module_create_message(remote_module, &CREATE_MESSAGE, &REPLY);
    STRICT_EXPECTED_CALL(ControlMessage_Destroy((CONTROL_MESSAGE *)&CREATE_MESSAGE));
    STRICT_EXPECTED_CALL(nn_freemsg((void *)NN_MESSAGE_BUFFER));
    STRICT_EXPECTED_CALL(nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT))
        .CopyOutArgumentBuffer(2, &NN_MESSAGE_BUFFER, sizeof(void *))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(nn_errno())
        .SetReturn(EAGAIN);

    STRICT_EXPECTED_CALL(nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT))
        .CopyOutArgumentBuffer(2, &NN_MESSAGE_BUFFER, sizeof(void *))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(NN_MESSAGE_SIZE);
    STRICT_EXPECTED_CALL(ControlMessage_CreateFromByteArray((const unsigned char *)NN_MESSAGE_BUFFER, IGNORED_NUM_ARG))
        .IgnoreArgument(2)
        .SetReturn((CONTROL_MESSAGE *)&START_MESSAGE);
    EXPECTED_CALL(mock_start(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(ControlMessage_Destroy((CONTROL_MESSAGE *)&START_MESSAGE));
    STRICT_EXPECTED_CALL(nn_freemsg((void *)NN_MESSAGE_BUFFER));
    STRICT_EXPECTED_CALL(nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT))
        .CopyOutArgumentBuffer(2, &NN_MESSAGE_BUFFER, sizeof(void *))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(NN_MESSAGE_SIZE);
    STRICT_EXPECTED_CALL(Message_CreateFromByteArray((const unsigned char *)NN_MESSAGE_BUFFER, IGNORED_NUM_ARG))
        .IgnoreArgument(2)
        .SetReturn((MESSAGE_HANDLE)&START_MESSAGE);
    STRICT_EXPECTED_CALL(mock_receive(IGNORED_PTR_ARG, (MESSAGE_HANDLE)&START_MESSAGE))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Message_Destroy((MESSAGE_HANDLE)&START_MESSAGE));
    STRICT_EXPECTED_CALL(nn_freemsg((void *)NN_MESSAGE_BUFFER));

    // Act
    ProxyGateway_DoWork(remote_module);
    ProxyGateway_DoWork(remote_module);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    ProxyGateway_Detach(remote_module);
}

/* Tests_SRS_PROXY_GATEWAY_027_033: [Control Channel - If the message type is CONTROL_MESSAGE_TYPE_MODULE_DESTROY, then `ProxyGateway_DoWork` shall call `void Module_Destroy(MODULE_HANDLE moduleHandle)`] */
/* Tests_SRS_PROXY_GATEWAY_027_034: [Control Channel - If the message type is CONTROL_MESSAGE_TYPE_MODULE_DESTROY, then `ProxyGateway_DoWork` shall disconnect from the message channel] */
/* Tests_SRS_PROXY_GATEWAY_027_037: [Message Channel - `ProxyGateway_DoWork` shall not check for messages, if the message socket is not available] */
TEST_FUNCTION(doWork_SCENARIO_destroy_message_success)
{
    // Arrange
    static const CONTROL_MESSAGE DESTROY_MESSAGE = {
        CONTROL_MESSAGE_VERSION_CURRENT,
        CONTROL_MESSAGE_TYPE_MODULE_DESTROY
    };
    static const void * NN_MESSAGE_BUFFER = (void *)0xEBADF00D;
    static const int32_t NN_MESSAGE_SIZE = 1979;

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT))
        .CopyOutArgumentBuffer(2, &NN_MESSAGE_BUFFER, sizeof(void *))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(NN_MESSAGE_SIZE);
    STRICT_EXPECTED_CALL(ControlMessage_CreateFromByteArray((const unsigned char *)NN_MESSAGE_BUFFER, IGNORED_NUM_ARG))
        .IgnoreArgument(2)
        .SetReturn((CONTROL_MESSAGE *)&DESTROY_MESSAGE);
    EXPECTED_CALL(mock_destroy(IGNORED_PTR_ARG));
    expected_calls_disconnect_from_message_channel();
    STRICT_EXPECTED_CALL(ControlMessage_Destroy((CONTROL_MESSAGE *)&DESTROY_MESSAGE));
    STRICT_EXPECTED_CALL(nn_freemsg((void *)NN_MESSAGE_BUFFER));

    // Act
    ProxyGateway_DoWork(remote_module);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    ProxyGateway_Detach(remote_module);
}

/* Codes_SRS_PROXY_GATEWAY_027_028: [Control Channel - If no message is available, then `ProxyGateway_DoWork` shall abandon the control channel request] */
TEST_FUNCTION(doWork_SCENARIO_control_message_not_available)
{
    // Arrange
    static const void * NN_MESSAGE_BUFFER = (void *)0xEBADF00D;

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT))
        .CopyOutArgumentBuffer(2, &NN_MESSAGE_BUFFER, sizeof(void *))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(nn_errno())
        .SetReturn(EAGAIN);

    // Act
    ProxyGateway_DoWork(remote_module);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    ProxyGateway_Detach(remote_module);
}

/* Codes_SRS_PROXY_GATEWAY_027_066: [Control Channel - If an error occurred when polling the gateway, then `ProxyGateway_DoWork` shall signal the gateway abandon the control channel request] */
TEST_FUNCTION(doWork_SCENARIO_control_message_error)
{
    // Arrange
    static const CONTROL_MESSAGE_MODULE_REPLY REPLY = {
        {
            CONTROL_MESSAGE_VERSION_1,
            CONTROL_MESSAGE_TYPE_MODULE_REPLY
        },
        1
    };
    static const void * NN_MESSAGE_BUFFER = (void *)0xEBADF00D;

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT))
        .CopyOutArgumentBuffer(2, &NN_MESSAGE_BUFFER, sizeof(void *))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(nn_errno())
        .SetReturn(ETERM);
    expected_calls_send_control_reply(&REPLY);

    // Act
    ProxyGateway_DoWork(remote_module);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    ProxyGateway_Detach(remote_module);
}

/* Tests_SRS_PROXY_GATEWAY_027_030: [Control Channel - If unable to parse the control message, then `ProxyGateway_DoWork` shall signal the gateway, free any previously allocated memory and abandon the control channel request] */
TEST_FUNCTION(doWork_SCENARIO_control_message_bad_parse)
{
    // Arrange
    static const CONTROL_MESSAGE_MODULE_REPLY REPLY = {
        {
            CONTROL_MESSAGE_VERSION_1,
            CONTROL_MESSAGE_TYPE_MODULE_REPLY
        },
        1
    };
    static const void * NN_MESSAGE_BUFFER = (void *)0xEBADF00D;
    static const int32_t NN_MESSAGE_SIZE = 1979;

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT))
        .CopyOutArgumentBuffer(2, &NN_MESSAGE_BUFFER, sizeof(void *))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(NN_MESSAGE_SIZE);
    STRICT_EXPECTED_CALL(ControlMessage_CreateFromByteArray((const unsigned char *)NN_MESSAGE_BUFFER, IGNORED_NUM_ARG))
        .IgnoreArgument(2)
        .SetReturn(NULL);
    expected_calls_send_control_reply(&REPLY);
    STRICT_EXPECTED_CALL(nn_freemsg((void *)NN_MESSAGE_BUFFER));

    // Act
    ProxyGateway_DoWork(remote_module);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    ProxyGateway_Detach(remote_module);
}

/* Tests_SRS_PROXY_GATEWAY_027_039: [Message Channel - If no message is available or an error occurred, then `ProxyGateway_DoWork` shall abandon the message channel request] */
TEST_FUNCTION(doWork_SCENARIO_gateway_message_not_available)
{
    // Arrange
    CONTROL_MESSAGE_MODULE_CREATE CREATE_MESSAGE = {
        {
            CONTROL_MESSAGE_VERSION_CURRENT,
            CONTROL_MESSAGE_TYPE_MODULE_CREATE
        },
        GATEWAY_MESSAGE_VERSION_CURRENT,
        {
            sizeof("ipc://message_channel"),
            NN_PAIR,
            "ipc://message_channel"
        },
        sizeof("json_encoded_remote_module_parameters"),
        "json_encoded_remote_module_parameters"
    };
    static const void * NN_MESSAGE_BUFFER = (void *)0xEBADF00D;
    static const int32_t NN_MESSAGE_SIZE = 1979;
    static const CONTROL_MESSAGE_MODULE_REPLY REPLY = {
        {
            CONTROL_MESSAGE_VERSION_1,
            CONTROL_MESSAGE_TYPE_MODULE_REPLY
        },
        0
    };

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT))
        .CopyOutArgumentBuffer(2, &NN_MESSAGE_BUFFER, sizeof(void *))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(NN_MESSAGE_SIZE);
    STRICT_EXPECTED_CALL(ControlMessage_CreateFromByteArray((const unsigned char *)NN_MESSAGE_BUFFER, IGNORED_NUM_ARG))
        .IgnoreArgument(2)
        .SetReturn((CONTROL_MESSAGE *)&CREATE_MESSAGE);
    expected_calls_process_module_create_message(remote_module, &CREATE_MESSAGE, &REPLY);
    STRICT_EXPECTED_CALL(ControlMessage_Destroy((CONTROL_MESSAGE *)&CREATE_MESSAGE));
    STRICT_EXPECTED_CALL(nn_freemsg((void *)NN_MESSAGE_BUFFER));
    STRICT_EXPECTED_CALL(nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT))
        .CopyOutArgumentBuffer(2, &NN_MESSAGE_BUFFER, sizeof(void *))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(nn_errno())
        .SetReturn(EAGAIN);

    // Act
    ProxyGateway_DoWork(remote_module);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    ProxyGateway_Detach(remote_module);
}

/* Tests_SRS_PROXY_GATEWAY_027_041: [Message Channel - If unable to parse the module message, then `ProxyGateway_DoWork` shall free any previously allocated memory and abandon the message channel request] */
TEST_FUNCTION(doWork_SCENARIO_gateway_message_bad_parse)
{
    // Arrange
    CONTROL_MESSAGE_MODULE_CREATE CREATE_MESSAGE = {
        {
            CONTROL_MESSAGE_VERSION_CURRENT,
            CONTROL_MESSAGE_TYPE_MODULE_CREATE
        },
        GATEWAY_MESSAGE_VERSION_CURRENT,
        {
            sizeof("ipc://message_channel"),
            NN_PAIR,
            "ipc://message_channel"
        },
        sizeof("json_encoded_remote_module_parameters"),
        "json_encoded_remote_module_parameters"
    };
    static const void * NN_MESSAGE_BUFFER = (void *)0xEBADF00D;
    static const int32_t NN_MESSAGE_SIZE = 1979;
    static const CONTROL_MESSAGE_MODULE_REPLY REPLY = {
        {
            CONTROL_MESSAGE_VERSION_1,
            CONTROL_MESSAGE_TYPE_MODULE_REPLY
        },
        0
    };

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT))
        .CopyOutArgumentBuffer(2, &NN_MESSAGE_BUFFER, sizeof(void *))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(NN_MESSAGE_SIZE);
    STRICT_EXPECTED_CALL(ControlMessage_CreateFromByteArray((const unsigned char *)NN_MESSAGE_BUFFER, IGNORED_NUM_ARG))
        .IgnoreArgument(2)
        .SetReturn((CONTROL_MESSAGE *)&CREATE_MESSAGE);
    expected_calls_process_module_create_message(remote_module, &CREATE_MESSAGE, &REPLY);
    STRICT_EXPECTED_CALL(ControlMessage_Destroy((CONTROL_MESSAGE *)&CREATE_MESSAGE));
    STRICT_EXPECTED_CALL(nn_freemsg((void *)NN_MESSAGE_BUFFER));
    STRICT_EXPECTED_CALL(nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT))
        .CopyOutArgumentBuffer(2, &NN_MESSAGE_BUFFER, sizeof(void *))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(NN_MESSAGE_SIZE);
    STRICT_EXPECTED_CALL(Message_CreateFromByteArray((const unsigned char *)NN_MESSAGE_BUFFER, IGNORED_NUM_ARG))
        .IgnoreArgument(2)
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(nn_freemsg((void *)NN_MESSAGE_BUFFER));

    // Act
    ProxyGateway_DoWork(remote_module);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    ProxyGateway_Detach(remote_module);
}

/* Tests_SRS_PROXY_GATEWAY_027_045: [Prerequisite Check - If the `remote_module` parameter is `NULL`, then `ProxyGateway_HaltWorkerThread` shall return a non-zero value] */
TEST_FUNCTION(haltWorkerThread_SCENARIO_NULL_handle)
{
    // Arrange
    int result;

    // Expected call listing
    umock_c_reset_all_calls();

    // Act
    result = ProxyGateway_HaltWorkerThread(NULL);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // Cleanup
}

/* Tests_SRS_PROXY_GATEWAY_027_046: [Prerequisite Check - If a worker thread does not exist, then `ProxyGateway_HaltWorkerThread` shall return a non-zero value] */
TEST_FUNCTION(haltWorkerThread_SCENARIO_no_worker_thread)
{
    // Arrange
    int result;

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();

    // Act
    result = ProxyGateway_HaltWorkerThread(remote_module);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // Cleanup
    ProxyGateway_Detach(remote_module);
}

/* Tests_SRS_PROXY_GATEWAY_027_047: [`ProxyGateway_HaltWorkerThread` shall obtain the thread mutex in order to signal the thread by calling `LOCK_RESULT Lock(LOCK_HANDLE handle)`] */
/* Tests_SRS_PROXY_GATEWAY_027_049: [`ProxyGateway_HaltWorkerThread` shall release the thread mutex upon signalling by calling `LOCK_RESULT Unlock(LOCK_HANDLE handle)`] */
/* Tests_SRS_PROXY_GATEWAY_027_051: [`ProxyGateway_HaltWorkerThread` shall halt the thread by calling `THREADAPI_RESULT ThreadAPI_Join(THREAD_HANDLE handle, int * res)`] */
/* Tests_SRS_PROXY_GATEWAY_027_053: [`ProxyGateway_HaltWorkerThread` shall free the thread mutex by calling `LOCK_RESULT Lock_Deinit(LOCK_HANDLE handle)`] */
/* Tests_SRS_PROXY_GATEWAY_027_055: [`ProxyGateway_HaltWorkerThread` shall free the memory allocated to the thread details] */
/* Tests_SRS_PROXY_GATEWAY_027_057: [If no errors are encountered, then `ProxyGateway_HaltWorkerThread` shall return zero] */
TEST_FUNCTION(haltWorkerThread_SCENARIO_success)
{
    // Arrange
    int result;
    int thread_exit_result = 0;

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(Lock_Init())
        .SetReturn(MOCK_LOCK);
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(THREADAPI_OK);

    STRICT_EXPECTED_CALL(Lock(MOCK_LOCK))
        .SetFailReturn(LOCK_ERROR)
        .SetReturn(LOCK_OK);
    STRICT_EXPECTED_CALL(Unlock(MOCK_LOCK))
        .SetFailReturn(LOCK_ERROR)
        .SetReturn(LOCK_OK);
    STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .CopyOutArgumentBuffer(2, &thread_exit_result, sizeof(thread_exit_result))
        .SetFailReturn(THREADAPI_ERROR)
        .SetReturn(THREADAPI_OK);
    STRICT_EXPECTED_CALL(Lock_Deinit(MOCK_LOCK))
        .SetFailReturn(LOCK_ERROR)
        .SetReturn(LOCK_OK);
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    // Act
    result = ProxyGateway_StartWorkerThread(remote_module);
    ASSERT_ARE_EQUAL(int, 0, result);
    result = ProxyGateway_HaltWorkerThread(remote_module);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // Cleanup
    ProxyGateway_Detach(remote_module);
}

/* Tests_SRS_PROXY_GATEWAY_027_056: [If an error is returned from the worker thread, then `ProxyGateway_HaltWorkerThread` shall return the worker thread's error code] */
TEST_FUNCTION(haltWorkerThread_SCENARIO_thread_error)
{
    // Arrange
    int result;
    int thread_exit_result = __LINE__;

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(Lock_Init())
        .SetReturn(MOCK_LOCK);
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(THREADAPI_OK);

    STRICT_EXPECTED_CALL(Lock(MOCK_LOCK))
        .SetFailReturn(LOCK_ERROR)
        .SetReturn(LOCK_OK);
    STRICT_EXPECTED_CALL(Unlock(MOCK_LOCK))
        .SetFailReturn(LOCK_ERROR)
        .SetReturn(LOCK_OK);
    STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .CopyOutArgumentBuffer(2, &thread_exit_result, sizeof(thread_exit_result))
        .SetFailReturn(THREADAPI_ERROR)
        .SetReturn(THREADAPI_OK);
    STRICT_EXPECTED_CALL(Lock_Deinit(MOCK_LOCK))
        .SetFailReturn(LOCK_ERROR)
        .SetReturn(LOCK_OK);
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    // Act
    result = ProxyGateway_StartWorkerThread(remote_module);
    ASSERT_ARE_EQUAL(int, 0, result);
    result = ProxyGateway_HaltWorkerThread(remote_module);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, thread_exit_result, result);

    // Cleanup
    ProxyGateway_Detach(remote_module);
}

/* Tests_SRS_PROXY_GATEWAY_027_048: [If unable to obtain the mutex, then `ProxyGateway_HaltWorkerThread` shall return a non-zero value] */
TEST_FUNCTION(haltWorkerThread_SCENARIO_can_not_obtain_mutex)
{
    // Arrange
    int result;

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(Lock_Init())
        .SetReturn(MOCK_LOCK);
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(THREADAPI_OK);

    STRICT_EXPECTED_CALL(Lock(MOCK_LOCK))
        .SetReturn(LOCK_ERROR);

    // Act
    result = ProxyGateway_StartWorkerThread(remote_module);
    ASSERT_ARE_EQUAL(int, 0, result);
    result = ProxyGateway_HaltWorkerThread(remote_module);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // Cleanup
    ProxyGateway_Detach(remote_module);
}

/* Tests_SRS_PROXY_GATEWAY_027_050: [If unable to release the mutex, then `ProxyGateway_HaltWorkerThread` shall return a non-zero value] */
TEST_FUNCTION(haltWorkerThread_SCENARIO_can_not_release_mutex)
{
    // Arrange
    int result;

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(Lock_Init())
        .SetReturn(MOCK_LOCK);
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(THREADAPI_OK);

    STRICT_EXPECTED_CALL(Lock(MOCK_LOCK))
        .SetReturn(LOCK_OK);
    STRICT_EXPECTED_CALL(Unlock(MOCK_LOCK))
        .SetReturn(LOCK_ERROR);

    // Act
    result = ProxyGateway_StartWorkerThread(remote_module);
    ASSERT_ARE_EQUAL(int, 0, result);
    result = ProxyGateway_HaltWorkerThread(remote_module);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // Cleanup
    ProxyGateway_Detach(remote_module);
}

/* Tests_SRS_PROXY_GATEWAY_027_052: [If unable to join the thread, then `ProxyGateway_HaltWorkerThread` shall return a non-zero value] */
TEST_FUNCTION(haltWorkerThread_SCENARIO_can_not_join_thread)
{
    // Arrange
    int result;
    int thread_exit_result = 0;

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(Lock_Init())
        .SetReturn(MOCK_LOCK);
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(THREADAPI_OK);

    STRICT_EXPECTED_CALL(Lock(MOCK_LOCK))
        .SetFailReturn(LOCK_ERROR)
        .SetReturn(LOCK_OK);
    STRICT_EXPECTED_CALL(Unlock(MOCK_LOCK))
        .SetFailReturn(LOCK_ERROR)
        .SetReturn(LOCK_OK);
    STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .CopyOutArgumentBuffer(2, &thread_exit_result, sizeof(thread_exit_result))
        .SetReturn(THREADAPI_ERROR);

    // Act
    result = ProxyGateway_StartWorkerThread(remote_module);
    ASSERT_ARE_EQUAL(int, 0, result);
    result = ProxyGateway_HaltWorkerThread(remote_module);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // Cleanup
    ProxyGateway_Detach(remote_module);
}

/* Tests_SRS_PROXY_GATEWAY_027_054: [If unable to free the thread mutex, then `ProxyGateway_HaltWorkerThread` shall ignore the result and continue processing] */
TEST_FUNCTION(haltWorkerThread_SCENARIO_can_not_free_mutex)
{
    // Arrange
    int result;
    int thread_exit_result = 0;

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(Lock_Init())
        .SetReturn(MOCK_LOCK);
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(THREADAPI_OK);

    STRICT_EXPECTED_CALL(Lock(MOCK_LOCK))
        .SetFailReturn(LOCK_ERROR)
        .SetReturn(LOCK_OK);
    STRICT_EXPECTED_CALL(Unlock(MOCK_LOCK))
        .SetFailReturn(LOCK_ERROR)
        .SetReturn(LOCK_OK);
    STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .CopyOutArgumentBuffer(2, &thread_exit_result, sizeof(thread_exit_result))
        .SetFailReturn(THREADAPI_ERROR)
        .SetReturn(THREADAPI_OK);
    STRICT_EXPECTED_CALL(Lock_Deinit(MOCK_LOCK))
        .SetFailReturn(LOCK_ERROR)
        .SetReturn(LOCK_OK);
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    // Act
    result = ProxyGateway_StartWorkerThread(remote_module);
    ASSERT_ARE_EQUAL(int, 0, result);
    result = ProxyGateway_HaltWorkerThread(remote_module);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // Cleanup
    ProxyGateway_Detach(remote_module);
}

/* Tests_SRS_PROXY_GATEWAY_027_058: [Prerequisite Check - If the `remote_module` parameter is `NULL`, then `ProxyGateway_Detach` shall do nothing] */
TEST_FUNCTION(detach_SCENARIO_NULL_handle)
{
    // Arrange

    // Expected call listing
    umock_c_reset_all_calls();

    // Act
    ProxyGateway_Detach(NULL);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
}

/* Tests_SRS_PROXY_GATEWAY_027_059: [If the worker thread is active, then `ProxyGateway_Detach` shall attempt to halt the worker thread] */
TEST_FUNCTION(detach_SCENARIO_halt_not_called)
{
    // Arrange
    static const CONTROL_MESSAGE_MODULE_REPLY REPLY = {
        {
            CONTROL_MESSAGE_VERSION_1,
            CONTROL_MESSAGE_TYPE_MODULE_REPLY
        },
        (uint8_t)-1,
    };

    int result;
    int thread_exit_result = 0;

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(Lock_Init())
        .SetReturn(MOCK_LOCK);
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(THREADAPI_OK);

    STRICT_EXPECTED_CALL(Lock(MOCK_LOCK))
        .SetFailReturn(LOCK_ERROR)
        .SetReturn(LOCK_OK);
    STRICT_EXPECTED_CALL(Unlock(MOCK_LOCK))
        .SetFailReturn(LOCK_ERROR)
        .SetReturn(LOCK_OK);
    STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .CopyOutArgumentBuffer(2, &thread_exit_result, sizeof(thread_exit_result))
        .SetFailReturn(THREADAPI_ERROR)
        .SetReturn(THREADAPI_OK);
    STRICT_EXPECTED_CALL(Lock_Deinit(MOCK_LOCK))
        .SetReturn(LOCK_ERROR);
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    expected_calls_send_control_reply((CONTROL_MESSAGE_MODULE_REPLY *)&REPLY);
	STRICT_EXPECTED_CALL(ThreadAPI_Sleep(1000));
    expected_calls_disconnect_from_message_channel();
    EXPECTED_CALL(nn_shutdown(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(nn_close(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(free(remote_module));

    // Act
    result = ProxyGateway_StartWorkerThread(remote_module);
    ASSERT_ARE_EQUAL(int, 0, result);
    ProxyGateway_Detach(remote_module);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
}

/* Tests_SRS_PROXY_GATEWAY_027_061: [`ProxyGateway_Detach` shall attempt to notify the Azure IoT Gateway of the detachment] */
/* Tests_SRS_PROXY_GATEWAY_027_062: [`ProxyGateway_Detach` shall disconnect from the Azure IoT Gateway message channel] */
/* Tests_SRS_PROXY_GATEWAY_027_063: [`ProxyGateway_Detach` shall shutdown the Azure IoT Gateway control channel by calling `int nn_shutdown(int s, int how)`] */
/* Tests_SRS_PROXY_GATEWAY_027_064: [`ProxyGateway_Detach` shall close the Azure IoT Gateway control socket by calling `int nn_close(int s)`] */
/* Tests_SRS_PROXY_GATEWAY_027_065: [`ProxyGateway_Detach` shall free the remaining memory dedicated to its instance data] */
TEST_FUNCTION(detach_SCENARIO_success)
{
    // Arrange
    static const CONTROL_MESSAGE_MODULE_REPLY REPLY = {
        {
            CONTROL_MESSAGE_VERSION_1,
            CONTROL_MESSAGE_TYPE_MODULE_REPLY
        },
        (uint8_t)-1,
    };

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    expected_calls_send_control_reply((CONTROL_MESSAGE_MODULE_REPLY *)&REPLY);
	STRICT_EXPECTED_CALL(ThreadAPI_Sleep(1000));
    expected_calls_disconnect_from_message_channel();
    EXPECTED_CALL(nn_shutdown(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(nn_close(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(free(remote_module));

    // Act
    ProxyGateway_Detach(remote_module);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
}

/* Tests_SRS_PROXY_GATEWAY_027_060: [If unable to halt the worker thread, `ProxyGateway_Detach` shall forcibly free the memory allocated to the worker thread] */
TEST_FUNCTION(detach_SCENARIO_unable_to_halt_thread)
{
    // Arrange
    static const CONTROL_MESSAGE_MODULE_REPLY REPLY = {
        {
            CONTROL_MESSAGE_VERSION_1,
            CONTROL_MESSAGE_TYPE_MODULE_REPLY
        },
        (uint8_t)-1,
    };

    int result;

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(Lock_Init())
        .SetReturn(MOCK_LOCK);
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(THREADAPI_OK);

    STRICT_EXPECTED_CALL(Lock(MOCK_LOCK))
        .SetFailReturn(LOCK_ERROR)
        .SetReturn(LOCK_OK);
    STRICT_EXPECTED_CALL(Unlock(MOCK_LOCK))
        .SetReturn(LOCK_ERROR);
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    expected_calls_send_control_reply((CONTROL_MESSAGE_MODULE_REPLY *)&REPLY);
	STRICT_EXPECTED_CALL(ThreadAPI_Sleep(1000));
    expected_calls_disconnect_from_message_channel();
    EXPECTED_CALL(nn_shutdown(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(nn_close(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(free(remote_module));

    // Act
    result = ProxyGateway_StartWorkerThread(remote_module);
    ASSERT_ARE_EQUAL(int, 0, result);
    ProxyGateway_Detach(remote_module);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
}


  /***************/
 /** INTERNALS **/
/***************/

/* SRS_PROXY_GATEWAY_027_0xx: [`connect_to_message_channel` shall create a socket for the Azure IoT Gateway message channel by calling `int nn_socket(int domain, int protocol)` with `AF_SP` as `domain` and `MESSAGE_URI::uri_type` as `protocol`] */
/* SRS_PROXY_GATEWAY_027_0xx: [`connect_to_message_channel` shall bind to the Azure IoT Gateway message channel by calling `int nn_bind(int s, const char * addr)` with the newly created socket as `s` and `MESSAGE_URI::uri` as `addr`] */
/* SRS_PROXY_GATEWAY_027_0xx: [If no errors are encountered, then `connect_to_message_channel` shall return zero] */
TEST_FUNCTION(connect_to_message_channel_SCENARIO_success)
{
    // Arrange
    static const MESSAGE_URI MESSAGE = {
        sizeof("ipc://proxy_gateway_ut"),
        NN_PAIR,
        "ipc://proxy_gateway_ut"
    };

    int result;

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    expected_calls_connect_to_message_channel(&MESSAGE);

    // Act
    result = connect_to_message_channel(remote_module, &MESSAGE);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // Cleanup
    ProxyGateway_Detach(remote_module);
}

/* SRS_PROXY_GATEWAY_027_0xx: [If a call to `nn_socket` returns -1, then `connect_to_message_channel` shall free any previously allocated memory, abandon the control message and prepare for the next create message] */
/* SRS_PROXY_GATEWAY_027_0xx: [If a call to `nn_connect` returns a negative value, then `connect_to_message_channel` shall free any previously allocated memory, abandon the control message and prepare for the next create message] */
TEST_FUNCTION(connect_to_message_channel_SCENARIO_negative_tests)
{
    // Arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    static const MESSAGE_URI MESSAGE = {
        sizeof("ipc://proxy_gateway_ut"),
        NN_PAIR,
        "ipc://proxy_gateway_ut"
    };

    int result;

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    expected_calls_connect_to_message_channel(&MESSAGE);
    umock_c_negative_tests_snapshot();

    ASSERT_ARE_EQUAL(int, negative_test_index, umock_c_negative_tests_call_count());
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); ++i) {
        if (skipNegativeTest(i)) {
            printf("%s: Skipping negative tests: %zx\n", __FUNCTION__, i);
            continue;
        }
        printf("%s: Running negative tests: %zx\n", __FUNCTION__, i);
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // Act
        result = connect_to_message_channel(remote_module, &MESSAGE);

        // Assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
    }

    // Cleanup
    ProxyGateway_Detach(remote_module);
    umock_c_negative_tests_deinit();
}

/* SRS_PROXY_GATEWAY_027_0xx: [`disconnect_from_message_channel` shall shutdown the Azure IoT Gateway message channel by calling `int nn_shutdown(int s, int how)`] */
/* SRS_PROXY_GATEWAY_027_0xx: [`disconnect_from_message_channel` shall close the Azure IoT Gateway message socket by calling `int nn_close(int s)`] */
TEST_FUNCTION(disconnect_from_message_channel_SCENARIO_success)
{
    // Arrange
    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    expected_calls_disconnect_from_message_channel();

    // Act
    disconnect_from_message_channel(remote_module);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    ProxyGateway_Detach(remote_module);
}

/* SRS_PROXY_GATEWAY_027_0xx: [Special Handling - If `Module_ParseConfigurationFromJson` was provided, `invoke_add_module_procedure` shall parse the configuration by calling `void * Module_ParseConfigurationFromJson(const char * configuration)` using the `CONTROL_MESSAGE_MODULE_CREATE::args` as `configuration`] */
TEST_FUNCTION(invoke_add_module_procedure_SCENARIO_NULL_Module_ParseConfigurationFromJson)
{
    // Arrange
    static const CONTROL_MESSAGE_MODULE_CREATE CREATE_MESSAGE = {
        {
            CONTROL_MESSAGE_VERSION_CURRENT,
            CONTROL_MESSAGE_TYPE_MODULE_CREATE
        },
        GATEWAY_MESSAGE_VERSION_CURRENT,
        {
            sizeof("ipc://message_channel"),
            NN_PAIR,
            "ipc://message_channel"
        },
        sizeof("json_encoded_remote_module_parameters"),
        "json_encoded_remote_module_parameters"
    };
    static const MODULE_API_1 MODULE_APIS = {
        { MODULE_API_VERSION_1 },
        (pfModule_ParseConfigurationFromJson)NULL,
        mock_freeConfiguration,
        mock_create,
        mock_destroy,
        mock_receive,
        mock_start
    };

    int result;

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mock_create((BROKER_HANDLE)remote_module, CREATE_MESSAGE.args))
        .SetReturn(MOCK_MODULE);

    // Act
    result = invoke_add_module_procedure(remote_module, CREATE_MESSAGE.args);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // Cleanup
    ProxyGateway_Detach(remote_module);
}

/* SRS_PROXY_GATEWAY_027_0xx: [If `Module_ParseConfigurationFromJson` was provided, `invoke_add_module_procedure` shall parse the configuration by calling `void * Module_ParseConfigurationFromJson(const char * configuration)` using the `CONTROL_MESSAGE_MODULE_CREATE::args` as `configuration`] */
/* SRS_PROXY_GATEWAY_027_0xx: [`invoke_add_module_procedure` shall create the remote module by calling `MODULE_HANDLE Module_Create(BROKER_HANDLE broker, const void * configuration)` using the `remote_module_handle` as `broker` and the parsed configuration value or `CONTROL_MESSAGE_MODULE_CREATE::args` as `configuration`] */
/* SRS_PROXY_GATEWAY_027_0xx: [If both `Module_FreeConfiguration` and `Module_ParseConfigurationFromJson` were provided, `invoke_add_module_procedure` shall free the configuration by calling `void Module_FreeConfiguration(void * configuration)` using the value returned from `Module_ParseConfigurationFromJson` as `configuration`] */
/* SRS_PROXY_GATEWAY_027_0xx: [If no errors are encountered, `invoke_add_module_procedure` shall return zero] */
TEST_FUNCTION(invoke_add_module_procedure_SCENARIO_success)
{
    // Arrange
    static const CONTROL_MESSAGE_MODULE_CREATE CREATE_MESSAGE = {
        {
            CONTROL_MESSAGE_VERSION_CURRENT,
            CONTROL_MESSAGE_TYPE_MODULE_CREATE
        },
        GATEWAY_MESSAGE_VERSION_CURRENT,
        {
            sizeof("ipc://message_channel"),
            NN_PAIR,
            "ipc://message_channel"
        },
        sizeof("json_encoded_remote_module_parameters"),
        "json_encoded_remote_module_parameters"
    };

    int result;

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    expected_calls_invoke_add_module_procedure(remote_module, &CREATE_MESSAGE);

    // Act
    result = invoke_add_module_procedure(remote_module, CREATE_MESSAGE.args);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // Cleanup
    ProxyGateway_Detach(remote_module);
}

/* SRS_PROXY_GATEWAY_027_0xx: [If unable to create the remote module, `invoke_add_module_procedure` shall free the configuration (if applicable) and return a non-zero value] */
TEST_FUNCTION(invoke_add_module_procedure_SCENARIO_negative_tests)
{
    // Arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    static const CONTROL_MESSAGE_MODULE_CREATE CREATE_MESSAGE = {
        {
            CONTROL_MESSAGE_VERSION_CURRENT,
            CONTROL_MESSAGE_TYPE_MODULE_CREATE
        },
        GATEWAY_MESSAGE_VERSION_CURRENT,
        {
            sizeof("ipc://message_channel"),
            NN_PAIR,
            "ipc://message_channel"
        },
        sizeof("json_encoded_remote_module_parameters"),
        "json_encoded_remote_module_parameters"
    };

    int result;

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    expected_calls_invoke_add_module_procedure(remote_module, &CREATE_MESSAGE);
    umock_c_negative_tests_snapshot();

    ASSERT_ARE_EQUAL(int, negative_test_index, umock_c_negative_tests_call_count());
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); ++i) {
        if (skipNegativeTest(i)) {
            printf("%s: Skipping negative tests: %zx\n", __FUNCTION__, i);
            continue;
        }
        printf("%s: Running negative tests: %zx\n", __FUNCTION__, i);
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // Act
        result = invoke_add_module_procedure(remote_module, CREATE_MESSAGE.args);

        // Assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
    }

    // Cleanup
    ProxyGateway_Detach(remote_module);
    umock_c_negative_tests_deinit();
}

/* SRS_PROXY_GATEWAY_027_0xx: [Prerequisite Check - If the `gateway_message_version` is greater than 1, then `process_module_create_message` shall do nothing and return a non-zero value] */
TEST_FUNCTION(process_module_create_message_SCENARIO_bad_version)
{
    // Arrange
    static const CONTROL_MESSAGE_MODULE_CREATE CREATE_MESSAGE = {
        {
            CONTROL_MESSAGE_VERSION_CURRENT,
            CONTROL_MESSAGE_TYPE_MODULE_CREATE
        },
        (GATEWAY_MESSAGE_VERSION_CURRENT + 1), // GATEWAY_MESSAGE_VERSION_NEXT
        {
            sizeof("ipc://message_channel"),
            NN_PAIR,
            "ipc://message_channel"
        },
        sizeof("json_encoded_remote_module_parameters"),
        "json_encoded_remote_module_parameters"
    };
    static const CONTROL_MESSAGE_MODULE_REPLY REPLY = {
        {
            CONTROL_MESSAGE_VERSION_1,
            CONTROL_MESSAGE_TYPE_MODULE_REPLY
        },
        1
    };

    int result;

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    expected_calls_send_control_reply(&REPLY);

    // Act
    result = process_module_create_message(remote_module, &CREATE_MESSAGE);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // Cleanup
    ProxyGateway_Detach(remote_module);
}

/* SRS_PROXY_GATEWAY_027_0xx: [Special Condition - If the creation process has already occurred, `process_module_create_message` shall destroy the module and disconnect from the message channel and continue processing the creation message] */
TEST_FUNCTION(process_module_create_message_SCENARIO_already_created)
{
    // Arrange
    static const CONTROL_MESSAGE_MODULE_CREATE CREATE_MESSAGE = {
        {
            CONTROL_MESSAGE_VERSION_CURRENT,
            CONTROL_MESSAGE_TYPE_MODULE_CREATE
        },
        GATEWAY_MESSAGE_VERSION_CURRENT,
        {
            sizeof("ipc://message_channel"),
            NN_PAIR,
            "ipc://message_channel"
        },
        sizeof("json_encoded_remote_module_parameters"),
        "json_encoded_remote_module_parameters"
    };
    static const CONTROL_MESSAGE_MODULE_REPLY REPLY = {
        {
            CONTROL_MESSAGE_VERSION_1,
            CONTROL_MESSAGE_TYPE_MODULE_REPLY
        },
        0
    };

    int result;

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    expected_calls_process_module_create_message(remote_module, &CREATE_MESSAGE, &REPLY);
    STRICT_EXPECTED_CALL(mock_destroy(MOCK_MODULE));
    expected_calls_disconnect_from_message_channel();
    expected_calls_process_module_create_message(remote_module, &CREATE_MESSAGE, &REPLY);

    // Act
    result = process_module_create_message(remote_module, &CREATE_MESSAGE);
    ASSERT_ARE_EQUAL(int, 0, result);
    result = process_module_create_message(remote_module, &CREATE_MESSAGE);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // Cleanup
    ProxyGateway_Detach(remote_module);
}

/* SRS_PROXY_GATEWAY_027_0xx: [`process_module_create_message` shall connect to the message channels] */
/* SRS_PROXY_GATEWAY_027_0xx: [`process_module_create_message` shall invoke the "add module" process] */
/* SRS_PROXY_GATEWAY_027_0xx: [`process_module_create_message` shall reply to the gateway with a success status] */
/* SRS_PROXY_GATEWAY_027_0xx: [If no errors are encountered, `process_module_create_message` shall return zero] */
TEST_FUNCTION(process_module_create_message_SCENARIO_success)
{
    // Arrange
    static const CONTROL_MESSAGE_MODULE_CREATE CREATE_MESSAGE = {
        {
            CONTROL_MESSAGE_VERSION_CURRENT,
            CONTROL_MESSAGE_TYPE_MODULE_CREATE
        },
        GATEWAY_MESSAGE_VERSION_CURRENT,
        {
            sizeof("ipc://message_channel"),
            NN_PAIR,
            "ipc://message_channel"
        },
        sizeof("json_encoded_remote_module_parameters"),
        "json_encoded_remote_module_parameters"
    };
    static const CONTROL_MESSAGE_MODULE_REPLY REPLY = {
        {
            CONTROL_MESSAGE_VERSION_1,
            CONTROL_MESSAGE_TYPE_MODULE_REPLY
        },
        0
    };

    int result;

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    expected_calls_process_module_create_message(remote_module, &CREATE_MESSAGE, &REPLY);

    // Act
    result = process_module_create_message(remote_module, &CREATE_MESSAGE);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // Cleanup
    ProxyGateway_Detach(remote_module);
}

/* SRS_PROXY_GATEWAY_027_0xx: [If unable to connect to the message channels, `process_module_create_message` shall attempt to reply to the gateway with a connection error status and return a non-zero value] */
/* SRS_PROXY_GATEWAY_027_0xx: [If unable to complete the "add module" process, `process_module_create_message` shall disconnect from the message channels, attempt to reply to the gateway with a module creation error status and return a non-zero value] */
/* SRS_PROXY_GATEWAY_027_0xx: [If unable to contact the gateway, `process_module_create_message` disconnect from the message channels and return a non-zero value] */
TEST_FUNCTION(process_module_create_message_SCENARIO_negative_tests)
{
    // Arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    static const CONTROL_MESSAGE_MODULE_CREATE CREATE_MESSAGE = {
        {
            CONTROL_MESSAGE_VERSION_CURRENT,
            CONTROL_MESSAGE_TYPE_MODULE_CREATE
        },
        GATEWAY_MESSAGE_VERSION_CURRENT,
        {
            sizeof("ipc://message_channel"),
            NN_PAIR,
            "ipc://message_channel"
        },
        sizeof("json_encoded_remote_module_parameters"),
        "json_encoded_remote_module_parameters"
    };
    static const CONTROL_MESSAGE_MODULE_REPLY REPLY = {
        {
            CONTROL_MESSAGE_VERSION_1,
            CONTROL_MESSAGE_TYPE_MODULE_REPLY
        },
        0
    };

    int result;

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    expected_calls_process_module_create_message(remote_module, &CREATE_MESSAGE, &REPLY);
    umock_c_negative_tests_snapshot();

    ASSERT_ARE_EQUAL(int, negative_test_index, umock_c_negative_tests_call_count());
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); ++i) {
        if (skipNegativeTest(i)) {
            printf("%s: Skipping negative tests: %zx\n", __FUNCTION__, i);
            continue;
        }
        printf("%s: Running negative tests: %zx\n", __FUNCTION__, i);
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // Act
        result = process_module_create_message(remote_module, &CREATE_MESSAGE);

        // Assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
    }

    // Cleanup
    ProxyGateway_Detach(remote_module);
    umock_c_negative_tests_deinit();
}

/* SRS_PROXY_GATEWAY_027_0xx: [`send_control_reply` shall calculate the serialized message size by calling `size_t ControlMessage_ToByteArray(CONTROL MESSAGE * message, unsigned char * buf, size_t size)`] */
/* SRS_PROXY_GATEWAY_027_0xx: [`send_control_reply` allocate the necessary space for the nano message, by calling `void * nn_allocmsg(size_t size, int type)` using the previously acquired message size for `size` and `0` for `type`] */
/* SRS_PROXY_GATEWAY_027_0xx: [`send_control_reply` shall serialize a creation reply indicating the creation status by calling `size_t ControlMessage_ToByteArray(CONTROL MESSAGE * message, unsigned char * buf, size_t size)`] */
/* SRS_PROXY_GATEWAY_027_0xx: [`send_control_reply` shall send the serialized message by calling `int nn_send(int s, const void * buf, size_t len, int flags)` using the serialized message as the `buf` parameter and the value returned from `ControlMessage_ToByteArray` as `len`] */
/* SRS_PROXY_GATEWAY_027_0xx: [If no errors are encountered, `send_control_reply` shall return zero] */
TEST_FUNCTION(send_control_reply_SCENARIO_success)
{
    // Arrange
    static const CONTROL_MESSAGE_MODULE_REPLY REPLY = {
        {
            CONTROL_MESSAGE_VERSION_1,
            CONTROL_MESSAGE_TYPE_MODULE_REPLY
        },
        0
    };

    int result;

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    expected_calls_send_control_reply(&REPLY);

    // Act
    result = send_control_reply(remote_module, REPLY.status);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // Cleanup
    ProxyGateway_Detach(remote_module);
}

/* SRS_PROXY_GATEWAY_027_0xx: [If unable to calculate the serialized message size, `send_control_reply` shall return a non-zero value] */
/* SRS_PROXY_GATEWAY_027_0xx: [If unable to allocate memory, `send_control_reply` shall return a non-zero value] */
/* SRS_PROXY_GATEWAY_027_0xx: [If unable to serialize the creation message reply, `send_control_reply` shall return a non-zero value] */
/* SRS_PROXY_GATEWAY_027_0xx: [If unable to send the serialized message, `send_control_reply` shall release the nano message by calling `int nn_freemsg(void * msg)` using the previously acquired nano message pointer as `msg` and return a non-zero value] */
TEST_FUNCTION(send_control_reply_SCENARIO_negative_tests)
{
    // Arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    static const CONTROL_MESSAGE_MODULE_REPLY REPLY = {
        {
            CONTROL_MESSAGE_VERSION_1,
            CONTROL_MESSAGE_TYPE_MODULE_REPLY
        },
        0
    };

    int result;

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    expected_calls_send_control_reply(&REPLY);
    umock_c_negative_tests_snapshot();

    ASSERT_ARE_EQUAL(int, negative_test_index, umock_c_negative_tests_call_count());
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); ++i) {
        if (skipNegativeTest(i)) {
            printf("%s: Skipping negative tests: %zx\n", __FUNCTION__, i);
            continue;
        }
        printf("%s: Running negative tests: %zx\n", __FUNCTION__, i);
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // Act
        result = send_control_reply(remote_module, REPLY.status);

        // Assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
    }

    // Cleanup
    ProxyGateway_Detach(remote_module);
    umock_c_negative_tests_deinit();
}

/* Tests_SRS_BROKER_17_007: [ Broker_Publish shall clone the message. ] */
/* Tests_SRS_BROKER_17_008: [ Broker_Publish shall serialize the message. ] */
/* Tests_SRS_BROKER_17_010: [ Broker_Publish shall send a message on the publish_socket. ] */
/* Tests_SRS_BROKER_17_011: [ Broker_Publish shall free the serialized message data. ] */
/* Tests_SRS_BROKER_17_012: [ Broker_Publish shall free the message. ] */
/* Tests_SRS_BROKER_17_022: [ N/A - Broker_Publish shall Lock the modules lock. ] */
/* Tests_SRS_BROKER_17_023: [ N/A - Broker_Publish shall Unlock the modules lock. ] */
/* Tests_SRS_BROKER_17_025: [ Broker_Publish shall allocate a nanomsg buffer the size of the serialized message + sizeof(MODULE_HANDLE). ] */
/* Tests_SRS_BROKER_17_026: [ N/A - Broker_Publish shall copy source into the beginning of the nanomsg buffer. ] */
/* Tests_SRS_BROKER_17_027: [ Broker_Publish shall serialize the message into the remainder of the nanomsg buffer. ] */
/* Tests_SRS_BROKER_13_030: [ If broker or message is NULL the function shall return BROKER_INVALIDARG. ] */
/* Tests_SRS_BROKER_13_037: [ This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise. ] */
TEST_FUNCTION(publish_SCENARIO_create_message_success)
{
    // Arrange
    CONTROL_MESSAGE_MODULE_CREATE CREATE_MESSAGE = {
        {
            CONTROL_MESSAGE_VERSION_CURRENT,
            CONTROL_MESSAGE_TYPE_MODULE_CREATE
        },
        GATEWAY_MESSAGE_VERSION_CURRENT,
        {
            sizeof("ipc://message_channel"),
            NN_PAIR,
            "ipc://message_channel"
        },
        sizeof("json_encoded_remote_module_parameters"),
        "json_encoded_remote_module_parameters"
    };
    static const void * NN_MESSAGE_BUFFER = (void *)0xEBADF00D;
    static const int32_t NN_MESSAGE_SIZE = 1979;
    static const CONTROL_MESSAGE_MODULE_REPLY REPLY = {
        {
            CONTROL_MESSAGE_VERSION_1,
            CONTROL_MESSAGE_TYPE_MODULE_REPLY
        },
        0
    };

    REMOTE_MODULE_HANDLE remote_module = ProxyGateway_Attach((MODULE_API *)&MOCK_MODULE_APIS, "proxy_gateway_ut");
    ASSERT_IS_NOT_NULL(remote_module);

    // Expected call listing
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT))
        .CopyOutArgumentBuffer(2, &NN_MESSAGE_BUFFER, sizeof(void *))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(NN_MESSAGE_SIZE);
    STRICT_EXPECTED_CALL(ControlMessage_CreateFromByteArray((const unsigned char *)NN_MESSAGE_BUFFER, IGNORED_NUM_ARG))
        .IgnoreArgument(2)
        .SetReturn((CONTROL_MESSAGE *)&CREATE_MESSAGE);
    expected_calls_process_module_create_message(remote_module, &CREATE_MESSAGE, &REPLY);
    STRICT_EXPECTED_CALL(ControlMessage_Destroy((CONTROL_MESSAGE *)&CREATE_MESSAGE));
    STRICT_EXPECTED_CALL(nn_freemsg((void *)NN_MESSAGE_BUFFER));
    STRICT_EXPECTED_CALL(nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT))
        .CopyOutArgumentBuffer(2, &NN_MESSAGE_BUFFER, sizeof(void *))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(NN_MESSAGE_SIZE);
    STRICT_EXPECTED_CALL(Message_CreateFromByteArray((const unsigned char *)NN_MESSAGE_BUFFER, IGNORED_NUM_ARG))
        .IgnoreArgument(2)
        .SetReturn((MESSAGE_HANDLE)&CREATE_MESSAGE);
    STRICT_EXPECTED_CALL(mock_receive(MOCK_MODULE, (MESSAGE_HANDLE)&CREATE_MESSAGE));
    STRICT_EXPECTED_CALL(Message_Destroy((MESSAGE_HANDLE)&CREATE_MESSAGE));
    STRICT_EXPECTED_CALL(nn_freemsg((void *)NN_MESSAGE_BUFFER));

    // Act
    ProxyGateway_DoWork(remote_module);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    ProxyGateway_Detach(remote_module);
}


/* SRS_PROXY_GATEWAY_027_0xx: [`worker_thread` shall obtain the thread mutex in order to initialize the thread by calling `LOCK_RESULT Lock(LOCK_HANDLE handle)`] */
/* SRS_PROXY_GATEWAY_027_0xx: [If unable to obtain the mutex, then `worker_thread` shall return a non-zero value] */
/* SRS_PROXY_GATEWAY_027_0xx: [`worker_thread` shall release the thread mutex upon entering the loop by calling `LOCK_RESULT Unlock(LOCK_HANDLE handle)`] */
/* SRS_PROXY_GATEWAY_027_0xx: [If unable to release the mutex, then `worker_thread` shall exit the thread and return a non-zero value] */
/* SRS_PROXY_GATEWAY_027_0xx: [`worker_thread` shall invoke asynchronous processing by calling `void ProxyGateway_DoWork(REMOTE_MODULE_HANDLE remote_module)`] */
/* SRS_PROXY_GATEWAY_027_0xx: [`worker_thread` shall yield its remaining quantum by calling `void THREADAPI_Sleep(unsigned int milliseconds)`] */
/* SRS_PROXY_GATEWAY_027_0xx: [`worker_thread` shall obtain the thread mutex in order to check for a halt signal by calling `LOCK_RESULT Lock(LOCK_HANDLE handle)`] */
/* SRS_PROXY_GATEWAY_027_0xx: [If unable to obtain the mutex, then `worker_thread` shall exit the thread return a non-zero value] */
/* SRS_PROXY_GATEWAY_027_0xx: [If unable to obtain the mutex, then `worker_thread` shall exit the thread return a non-zero value] */
/* SRS_PROXY_GATEWAY_027_0xx: [Once the halt signal is received, `worker_thread` shall release the thread mutex by calling `LOCK_RESULT Unlock(LOCK_HANDLE handle)`] */
/* SRS_PROXY_GATEWAY_027_0xx: [If unable to release the mutex, then `worker_thread` shall return a non-zero value] */
/* SRS_PROXY_GATEWAY_027_0xx: [If no errors are encountered, `worker_thread` shall return zero] */
/* SRS_PROXY_GATEWAY_027_0xx: [`worker_thread` shall pass its return value to the caller by calling `void ThreadAPI_Exit(int res)`] */

END_TEST_SUITE(proxy_gateway_ut)

