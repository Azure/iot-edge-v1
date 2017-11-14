// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#include <cstdbool>
#include <cstdint>
#else
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#endif

#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif


#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umock_c_negative_tests.h"
#include "umocktypes_charptr.h"
#include "umocktypes_stdint.h"
#include "umock_c_prod.h"

#define NN_EXPORT

#include <nn.h>
#include <pair.h>
static TEST_MUTEX_HANDLE g_dllByDll;
static TEST_MUTEX_HANDLE g_testByTest;

#define ENABLE_MOCKS
#include <jni.h>
#include "java_nanomsg.h"


//=============================================================================
//Globals
//=============================================================================

static JNIEnv* global_env = NULL;

//=============================================================================
//MOCKS
//=============================================================================

//JNIEnv function mocks
MOCK_FUNCTION_WITH_CODE(JNICALL, jclass, FindClass, JNIEnv*, env, const char*, name)
jclass clazz = (jclass)0x42;
MOCK_FUNCTION_END(clazz)

MOCK_FUNCTION_WITH_CODE(JNICALL, jclass, GetObjectClass, JNIEnv*, env, jobject, obj);
jclass clazz = (jclass)0x42;
MOCK_FUNCTION_END(clazz)

MOCK_FUNCTION_WITH_CODE(JNICALL, jmethodID, GetMethodID, JNIEnv*, env, jclass, clazz, const char*, name, const char*, sig);
jmethodID methodID = (jmethodID)0x42;
MOCK_FUNCTION_END(methodID)

jobject NewObject(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
    (void)env;
    (void)clazz;
    (void)methodID;
    return (jobject)0x42;
}

jobject CallObjectMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
    (void)env;
    (void)obj;
    (void)methodID;
    return (jobject)0x42;
}

MOCK_FUNCTION_WITH_CODE(JNICALL, jstring, NewStringUTF, JNIEnv*, env, const char*, utf);
    jstring jstr = (jstring)utf;
MOCK_FUNCTION_END(jstr)

MOCK_FUNCTION_WITH_CODE(JNICALL, const char *, GetStringUTFChars, JNIEnv*, env, jstring, str, jboolean*, isCopy);
    char *stringChars = "Test";
MOCK_FUNCTION_END(stringChars)

MOCK_FUNCTION_WITH_CODE(JNICALL, jsize, GetArrayLength, JNIEnv*, env, jarray, arr);
    jsize size = sizeof(arr);
MOCK_FUNCTION_END(size)

MOCK_FUNCTION_WITH_CODE(JNICALL, jbyteArray, NewByteArray, JNIEnv*, env, jsize, len);
    jbyteArray arr =(jbyteArray)malloc(1);
MOCK_FUNCTION_END(arr)

MOCK_FUNCTION_WITH_CODE(JNICALL, void, GetByteArrayRegion, JNIEnv*, env, jbyteArray, arr, jsize, start, jsize, len, jbyte*, buf);
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(JNICALL, jbyte*, GetByteArrayElements, JNIEnv*, env, jbyteArray, arr, jboolean*, isCopy);
MOCK_FUNCTION_END((jbyte*)1)

MOCK_FUNCTION_WITH_CODE(JNICALL, void, ReleaseByteArrayElements, JNIEnv*, env, jbyteArray, arr, jbyte*, elems, jint, mode);
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(JNICALL, void, SetByteArrayRegion, JNIEnv*, env, jbyteArray, arr, jsize, start, jsize, len, const jbyte*, buf);
MOCK_FUNCTION_END()

MOCKABLE_FUNCTION(JNICALL, void, DeleteLocalRef, JNIEnv*, env, jobject, obj);
void my_DeleteLocalRef(JNIEnv* env, jobject obj)
{
    (void)env;
    free((void*)obj);
}

MOCK_FUNCTION_WITH_CODE(JNICALL, jthrowable, ExceptionOccurred, JNIEnv*, env);
MOCK_FUNCTION_END(NULL)

MOCK_FUNCTION_WITH_CODE(JNICALL, void, ExceptionClear, JNIEnv*, env);
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(JNICALL, void, ExceptionDescribe, JNIEnv*, env);
MOCK_FUNCTION_END()

// Structure storing all JNI function pointers. Used to mock the JNIEnv type is a pointer to this structure
struct JNINativeInterface_ env = {
    0, 0, 0, 0,

    NULL, NULL, FindClass, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, ExceptionOccurred, ExceptionDescribe, ExceptionClear, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NewObject, NULL, NULL, NULL, NULL, GetMethodID,
    CallObjectMethod, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NewStringUTF, NULL, GetStringUTFChars, NULL, GetArrayLength, NULL, NULL,
    NULL, NULL, NewByteArray, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    GetByteArrayElements, NULL, NULL, NULL, NULL, NULL, NULL, NULL, ReleaseByteArrayElements, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, SetByteArrayRegion, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

#undef ENABLE_MOCKS

//Nanomsg mocks
MOCK_FUNCTION_WITH_CODE(, int, nn_bind, int, s, const char *, addr)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, nn_close, int, s)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, nn_errno)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, nn_freemsg, void *, msg)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, nn_recv, int, s, void *, buf, size_t, len, int, flags)
MOCK_FUNCTION_END(len)

MOCK_FUNCTION_WITH_CODE(, int, nn_send, int, s, const void *, buf, size_t, len, int, flags)
MOCK_FUNCTION_END(len)

MOCK_FUNCTION_WITH_CODE(, int, nn_shutdown, int, s, int, how)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, nn_socket, int, domain, int, protocol)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, const char *, nn_strerror, int, errnum)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, const char *, nn_symbol, int, i, int*, value)
MOCK_FUNCTION_END(0)

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static
void
on_umock_c_error(
    UMOCK_C_ERROR_CODE error_code
) {
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

BEGIN_TEST_SUITE(JavaNanomsg_UnitTests)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    int result;
    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    g_testByTest = TEST_MUTEX_CREATE();

    result = umock_c_init(on_umock_c_error);
    ASSERT_ARE_EQUAL(int, 0, result);
    umocktypes_charptr_register_types();
    umocktypes_stdint_register_types();

    REGISTER_UMOCK_ALIAS_TYPE(jint, int32_t);
    REGISTER_UMOCK_ALIAS_TYPE(jclass, void*);
    REGISTER_UMOCK_ALIAS_TYPE(jmethodID, void*);
    REGISTER_UMOCK_ALIAS_TYPE(jobject, void*);
    REGISTER_UMOCK_ALIAS_TYPE(jstring, void*);
    REGISTER_UMOCK_ALIAS_TYPE(jsize, int);
    REGISTER_UMOCK_ALIAS_TYPE(jthrowable, int);
    REGISTER_UMOCK_ALIAS_TYPE(jbyteArray, void*);
    REGISTER_UMOCK_ALIAS_TYPE(jsize, int);
    REGISTER_UMOCK_ALIAS_TYPE(const jbyte*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(jarray, void*);
    REGISTER_UMOCK_ALIAS_TYPE(JNIEnv*, void*);

    REGISTER_UMOCK_ALIAS_TYPE(const char*, char*);

    REGISTER_UMOCK_ALIAS_TYPE(void**, void*);
    REGISTER_UMOCK_ALIAS_TYPE(va_list, void*);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
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

#ifdef __cplusplus

    global_env = new JNIEnv();
    ((JNIEnv*)(global_env))->functions = new JNINativeInterface_(env);

#else

    global_env = (void*)((JNIEnv*)malloc(sizeof(JNIEnv)));
    *((JNIEnv*)(global_env)) = malloc(sizeof(struct JNINativeInterface_));
    *(struct JNINativeInterface_*)(*((JNIEnv*)(global_env))) = env;
#endif

    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    global_env = NULL;
    TEST_MUTEX_RELEASE(g_testByTest);
}

TEST_FUNCTION(Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1errno_success)
{
    //Arrange
    umock_c_reset_all_calls();

    jobject jObject = (jobject)0x42;

    STRICT_EXPECTED_CALL(nn_errno())
        .SetReturn(EAGAIN);

    //Act
    jint result = Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1errno(global_env, jObject);

    //Assert
    ASSERT_ARE_EQUAL(int32_t, EAGAIN, result);
}

TEST_FUNCTION(Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1strerror_success)
{
    //Arrange
    umock_c_reset_all_calls();

    jobject jObject = (jobject)0x42;
    jint err = (jint)21;
    const char* error = "Error";

    STRICT_EXPECTED_CALL(nn_strerror(err))
        .IgnoreArgument(1)
        .SetReturn(error);
    STRICT_EXPECTED_CALL(NewStringUTF(IGNORED_PTR_ARG, error))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //Act
    (void)Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1strerror(global_env, jObject, err);
}

TEST_FUNCTION(Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1strerror_returns_empty)
{
    //Arrange
    umock_c_reset_all_calls();

    jobject jObject = (jobject)0x42;
    jint err = (jint)21;
    const char* error = "Error";

    STRICT_EXPECTED_CALL(nn_strerror(err))
        .IgnoreArgument(1)
        .SetReturn(0);
    STRICT_EXPECTED_CALL(NewStringUTF(IGNORED_PTR_ARG, error))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //Act
    (void)Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1strerror(global_env, jObject, err);
}

TEST_FUNCTION(Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1strerror_returns_empty_if_exception)
{
    //Arrange
    umock_c_reset_all_calls();

    jobject jObject = (jobject)0x42;
    jint err = (jint)21;
    const char* error = "Error";
    jthrowable exception = (jthrowable)0x42;

    STRICT_EXPECTED_CALL(nn_strerror(err))
        .IgnoreArgument(1)
        .SetReturn(0);
    STRICT_EXPECTED_CALL(NewStringUTF(IGNORED_PTR_ARG, error))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(exception);
    STRICT_EXPECTED_CALL(ExceptionClear(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //Act
    (void)Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1strerror(global_env, jObject, err);
}

TEST_FUNCTION(Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1socket_success)
{
    //Arrange
    umock_c_reset_all_calls();

    jobject jObject = (jobject)0x42;
    jint domain = (jint)AF_SP;
    jint protocol = (jint)NN_PAIR;
    jint socket = (jint)1;
    STRICT_EXPECTED_CALL(nn_socket(domain, protocol))
        .SetReturn(socket);

    //Act
    jint result = Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1socket(global_env, jObject, domain, protocol);

    //Assert
    ASSERT_ARE_EQUAL(int32_t, socket, result);
}

TEST_FUNCTION(Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1close_success)
{
    //Arrange
    umock_c_reset_all_calls();

    jobject jObject = (jobject)0x42;
    jint socket = (jint)1;
    jint expectedResult = (jint)0;
    STRICT_EXPECTED_CALL(nn_close(socket))
        .SetReturn(expectedResult);

    //Act
    jint result = Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1close(global_env, jObject, socket);

    //Assert
    ASSERT_ARE_EQUAL(int32_t, expectedResult, result);
}

TEST_FUNCTION(Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1bind_success)
{
    //Arrange
    umock_c_reset_all_calls();

    jobject jObject = (jobject)0x42;
    jint socket = (jint)1;
    char* address = "control_id";
    jint endpointId = (jint)1;
    jstring jaddress = (jstring)address;
    STRICT_EXPECTED_CALL(GetStringUTFChars(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(nn_bind(socket, address))
        .IgnoreAllArguments()
        .SetReturn(endpointId);

    //Act
    jint result = Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1bind(global_env, jObject, socket, jaddress);

    //Assert
    ASSERT_ARE_EQUAL(int32_t, endpointId, result);
}

TEST_FUNCTION(Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1bind_success_if_addr_is_NULL)
{
    //Arrange
    umock_c_reset_all_calls();

    jobject jObject = (jobject)0x42;
    jint socket = (jint)1;
    char* address = "control_id";
    jint endpointId = (jint)0;
    jstring jaddress = (jstring)address;
    STRICT_EXPECTED_CALL(GetStringUTFChars(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(nn_bind(socket, address))
        .SetReturn(endpointId);

    //Act
    jint result = Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1bind(global_env, jObject, socket, jaddress);

    //Assert
    ASSERT_ARE_EQUAL(int32_t, endpointId, result);
}

TEST_FUNCTION(Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1shutdown_success)
{
    //Arrange
    umock_c_reset_all_calls();

    jobject jObject = (jobject)0x42;
    jint socket = (jint)1;
    jint endpoint = (jint)0;
    jint expectedResult = (jint)0;
    STRICT_EXPECTED_CALL(nn_shutdown(socket, endpoint))
        .SetReturn(expectedResult);

    //Act
    jint result = Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1shutdown(global_env, jObject, socket, endpoint);

    //Assert
    ASSERT_ARE_EQUAL(int32_t, expectedResult, result);
}

TEST_FUNCTION(Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1send_success)
{
    //Arrange
    umock_c_reset_all_calls();

    jobject jObject = (jobject)0x42;
    jint socket = (jint)1;
    jbyteArray buffer = (jbyteArray)0x42;
    jint flags = (jint)1;
    jint expectedResult = (jint)sizeof(buffer);
    STRICT_EXPECTED_CALL(GetArrayLength(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(GetByteArrayElements(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 0))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(nn_send(socket, buffer, sizeof(buffer), flags))
        .SetReturn(expectedResult);
    STRICT_EXPECTED_CALL(ReleaseByteArrayElements(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, JNI_ABORT));

    //Act
    jint result = Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1send(global_env, jObject, socket, buffer, flags);

    //Assert
    ASSERT_ARE_EQUAL(int32_t, expectedResult, result);
}

TEST_FUNCTION(Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1send_not_sent_if_buffer_is_null)
{
    //Arrange
    umock_c_reset_all_calls();

    jobject jObject = (jobject)0x42;
    jint socket = (jint)1;
    jbyteArray buffer = (jbyteArray)0x42;
    jint flags = (jint)1;
    jint expectedResult = (jint)-1;
    STRICT_EXPECTED_CALL(GetArrayLength(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(GetByteArrayElements(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 0))
        .IgnoreAllArguments()
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(nn_send(socket, buffer, 4, flags));
    STRICT_EXPECTED_CALL(ReleaseByteArrayElements(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, JNI_ABORT));

    //Act
    jint result = Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1send(global_env, jObject, socket, buffer, flags);

    //Assert
    ASSERT_ARE_EQUAL(int32_t, expectedResult, result);
}

TEST_FUNCTION(Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1recv_success)
{
    //Arrange
    umock_c_reset_all_calls();

    jobject jObject = (jobject)0x42;
    jint socket = (jint)1;
    jint flags = (jint)1;
    int expectedResult = 4;
    void *buf = NULL;
    STRICT_EXPECTED_CALL(nn_recv(socket, buf, NN_MSG, flags))
        .SetReturn(expectedResult);
    STRICT_EXPECTED_CALL(NewByteArray(IGNORED_PTR_ARG, expectedResult))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(SetByteArrayRegion(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 0, expectedResult, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //Act
    (void)Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1recv(global_env, jObject, socket, flags);
}

TEST_FUNCTION(Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1recv_return_null_if_no_message)
{
    //Arrange
    umock_c_reset_all_calls();

    jobject jObject = (jobject)0x42;
    jint socket = (jint)1;
    jint flags = (jint)1;

    STRICT_EXPECTED_CALL(nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT))
        .IgnoreAllArguments()
        .SetReturn(-1);

    //Act
    jbyteArray result = Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1recv(global_env, jObject, socket, flags);

    //Assert
    ASSERT_IS_NULL(result);
}

TEST_FUNCTION(Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1recv_return_null_if_bytearray_fails)
{
    //Arrange
    umock_c_reset_all_calls();

    jobject jObject = (jobject)0x42;
    jint socket = (jint)1;
    jint flags = (jint)1;
    int expectedResult = 4;

    STRICT_EXPECTED_CALL(nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT))
        .IgnoreAllArguments()
        .SetReturn(expectedResult);
    STRICT_EXPECTED_CALL(NewByteArray(IGNORED_PTR_ARG, expectedResult))
        .IgnoreAllArguments()
        .SetReturn(NULL);

    //Act
    jbyteArray result = Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1recv(global_env, jObject, socket, flags);

    //Assert
    ASSERT_IS_NULL(result);
}

TEST_FUNCTION(Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1recv_return_null_if_setbytearray_fails)
{
    //Arrange
    umock_c_reset_all_calls();

    jobject jObject = (jobject)0x42;
    jint socket = (jint)1;
    jint flags = (jint)1;
    int expectedResult = 4;
    jthrowable exception = (jthrowable)0x42;

    STRICT_EXPECTED_CALL(nn_recv(IGNORED_NUM_ARG, IGNORED_PTR_ARG, NN_MSG, NN_DONTWAIT))
        .IgnoreAllArguments()
        .SetReturn(expectedResult);
    STRICT_EXPECTED_CALL(NewByteArray(IGNORED_PTR_ARG, expectedResult))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(SetByteArrayRegion(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 0, expectedResult, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(exception);

    //Act
    jbyteArray result = Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1recv(global_env, jObject, socket, flags);

    //Assert
    ASSERT_IS_NULL(result);
}

TEST_FUNCTION(Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_getSymbols_success)
{
    //Arrange
    umock_c_reset_all_calls();

    jclass jClass = (jclass)0x42;
    char* symbol = "NN_PAIR";

    STRICT_EXPECTED_CALL(FindClass(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(nn_symbol(0, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(symbol);
    STRICT_EXPECTED_CALL(nn_symbol(1, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(NULL);

    //Act
    jobject result = Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_getSymbols(global_env, jClass);

    //Assert
    ASSERT_IS_NOT_NULL(result);
}

TEST_FUNCTION(Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_getSymbols_returns_map_if_no_symbols)
{
    //Arrange
    umock_c_reset_all_calls();

    jclass jClass = (jclass)0x42;

    //Act
    jobject result = Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_getSymbols(global_env, jClass);

    //Assert
    ASSERT_IS_NOT_NULL(result);
}

TEST_FUNCTION(Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_getSymbols_returns_map_if_symbol_failure)
{
    //Arrange
    umock_c_reset_all_calls();

    jclass jClass = (jclass)0x42;
    char* symbol = "NN_PAIR";
    jthrowable exception = (jthrowable)0x42;

    umock_c_negative_tests_init();

    STRICT_EXPECTED_CALL(nn_symbol(0, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(symbol);
    STRICT_EXPECTED_CALL(NewStringUTF(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn((jstring)0x42)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(NULL)
        .SetFailReturn(exception);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(NULL)
        .SetFailReturn(exception);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(NULL)
        .SetFailReturn(exception);

    umock_c_negative_tests_snapshot();

    for (size_t i = 1; i < umock_c_negative_tests_call_count(); i++)
    {
        // arrange
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        //Act
        jobject result = Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_getSymbols(global_env, jClass);

        //Assert
        ASSERT_IS_NOT_NULL(result);
    }
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_getSymbols_returns_null_if_failure)
{
    //Arrange
    umock_c_reset_all_calls();

    jclass jClass = (jclass)0x42;
    jmethodID jMethodId = (jmethodID)0x42;
    char* symbol = "NN_PAIR";
    jthrowable exception = (jthrowable)0x42;

    umock_c_negative_tests_init();

    STRICT_EXPECTED_CALL(FindClass(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn(jClass)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(NULL)
        .SetFailReturn(exception);
    STRICT_EXPECTED_CALL(GetMethodID(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn(jMethodId)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(NULL)
        .SetFailReturn(exception);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(NULL)
        .SetFailReturn(exception);
    STRICT_EXPECTED_CALL(GetMethodID(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn(jMethodId)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(NULL)
        .SetFailReturn(exception);
    STRICT_EXPECTED_CALL(FindClass(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn(jClass)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(NULL)
        .SetFailReturn(exception);
    STRICT_EXPECTED_CALL(GetMethodID(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetReturn(jMethodId)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(NULL)
        .SetFailReturn(exception);
    STRICT_EXPECTED_CALL(nn_symbol(0, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(symbol);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (i != 11)
        {
            // arrange
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //Act
            jobject result = Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_getSymbols(global_env, jClass);

            //Assert
            ASSERT_IS_NULL(result);
        }
    }
    umock_c_negative_tests_deinit();
}

END_TEST_SUITE(JavaNanomsg_UnitTests);
