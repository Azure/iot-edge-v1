// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <stddef.h>
#include <stdint.h>
#include <stddef.h>

#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umock_c_negative_tests.h"
#include "umocktypes_charptr.h"
#include "umocktypes_stdint.h"
#include "umock_c_prod.h"

#define ENABLE_MOCKS

#include "java_module_host_manager.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/strings.h"
#include "java_module_host.h"
#include "java_module_host_common.h"
#include "module_access.h"

#include <parson.h>

#ifdef _WIN32

#define _JAVASOFT_JNI_MD_H_
#define JNIEXPORT
#define JNIIMPORT
#define JNICALL

typedef int32_t jint;
typedef __int64 jlong;
typedef signed char jbyte;

#else

#define _JAVASOFT_JNI_MD_H_

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif
#if (defined(__GNUC__) && ((__GNUC__ > 4) || (__GNUC__ == 4) && (__GNUC_MINOR__ > 2))) || __has_attribute(visibility)
#define JNIEXPORT     __attribute__((visibility("default")))
#define JNIIMPORT     __attribute__((visibility("default")))
#else
#define JNIEXPORT
#define JNIIMPORT
#endif

#define JNICALL

typedef int32_t jint;
#ifdef _LP64 /* 64-bit Solaris */
typedef long jlong;
#else
typedef long long jlong;
#endif

typedef signed char jbyte;

#endif

#include "broker_proxy.h"
#include <jni.h>

//=============================================================================
//Globals
//=============================================================================

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

static bool malloc_will_fail = false;

static int module_manager_count = 0;
static JAVA_MODULE_HOST_MANAGER_HANDLE global_manager = NULL;

static JNIEnv* global_env = NULL;
static JavaVM* global_vm = NULL;

static JAVA_MODULE_HOST_CONFIG config =
{
    "TestClass",
    "{hello}",
    NULL
};

//=============================================================================
//MOCKS
//=============================================================================

//Message mocks
MOCKABLE_FUNCTION(, MESSAGE_HANDLE, Message_CreateFromByteArray, const unsigned char*, source, int32_t, size);
MESSAGE_HANDLE my_Message_CreateFromByteArray(const unsigned char* source, int32_t size)
{
    return (MESSAGE_HANDLE)malloc(1);
}
MOCKABLE_FUNCTION(, int32_t, Message_ToByteArray, MESSAGE_HANDLE, messageHandle, unsigned char*, buf, int32_t, size);
int32_t my_MessageToByteArray(MESSAGE_HANDLE messageHandle, unsigned char* buf, int32_t size)
{
    return 1;
}
MOCKABLE_FUNCTION(, void, Message_Destroy, MESSAGE_HANDLE, message);
void my_Message_Destroy(MESSAGE_HANDLE message)
{
    if (message != NULL)
    {
        free((void*)message);
        message = NULL;
    }
}

//Broker mocks
MOCKABLE_FUNCTION(, BROKER_RESULT, Broker_Publish, BROKER_HANDLE, broker, MODULE_HANDLE, source, MESSAGE_HANDLE, message);

//JEnv function mocks
MOCKABLE_FUNCTION(JNICALL, jclass, FindClass, JNIEnv*, env, const char*, name);
jclass my_FindClass(JNIEnv* env, const char* name)
{
    return (jclass)0x42;
}

MOCKABLE_FUNCTION(JNICALL, jclass, GetObjectClass, JNIEnv*, env, jobject, obj);
jclass my_GetObjectClass(JNIEnv* env, jobject obj)
{
    return (jclass)0x42;
}

MOCKABLE_FUNCTION(JNICALL, jmethodID, GetMethodID, JNIEnv*, env, jclass, clazz, const char*, name, const char*, sig);
jmethodID my_GetMethodID(JNIEnv* env, jclass clazz, const char* name, const char* sig)
{
    return (jmethodID)0x42;
}

MOCKABLE_FUNCTION(JNICALL, jobject, NewObjectV, JNIEnv*, env, jclass, clazz, jmethodID, methodID, va_list, args);
jobject my_NewObject(JNIEnv* env, jclass clazz, jmethodID methodID, va_list args)
{
    return (jobject)0x42;
}

MOCKABLE_FUNCTION(JNICALL, jstring, NewStringUTF, JNIEnv*, env, const char*, utf);
jstring my_NewStringUTF(JNIEnv* env, const char* utf)
{
    return (jstring)0x42;
}

MOCKABLE_FUNCTION(JNICALL, jbyteArray, NewByteArray, JNIEnv*, env, jsize, len);
jbyteArray my_NewByteArray(JNIEnv* env, jsize len)
{
    return (jbyteArray)malloc(1);
}

MOCKABLE_FUNCTION(JNICALL, void, SetByteArrayRegion, JNIEnv*, env, jbyteArray, arr, jsize, start, jsize, len, const jbyte*, buf);

MOCKABLE_FUNCTION(JNICALL, jsize, GetArrayLength, JNIEnv*, env, jarray, arr);
jsize my_GetArrayLength(JNIEnv* env, jarray arr)
{
    return sizeof(arr);
}

MOCKABLE_FUNCTION(JNICALL, void, GetByteArrayRegion, JNIEnv*, env, jbyteArray, arr, jsize, start, jsize, len, jbyte*, buf);

MOCKABLE_FUNCTION(JNICALL, void, DeleteLocalRef, JNIEnv*, env, jobject, obj);
void my_DeleteLocalRef(JNIEnv* env, jobject obj)
{
    free((void*)obj);
}


MOCKABLE_FUNCTION(JNICALL, jobject, NewGlobalRef, JNIEnv*, env, jobject, lobj);
jobject my_NewGlobalRef(JNIEnv* env, jobject lobj)
{
    return (jobject)malloc(1);
}

MOCKABLE_FUNCTION(JNICALL, void, DeleteGlobalRef, JNIEnv*, env, jobject, gref);
void my_DeleteGlobalRef(JNIEnv* env, jobject gref)
{
    free(gref);
}

MOCKABLE_FUNCTION(JNICALL, void, CallVoidMethodV, JNIEnv*, env, jobject, obj, jmethodID, methodID, va_list, args);

MOCKABLE_FUNCTION(JNICALL, jthrowable, ExceptionOccurred, JNIEnv*, env);
jthrowable my_ExceptionOccurred(JNIEnv* env)
{
    return NULL;
}

MOCKABLE_FUNCTION(JNICALL, void, ExceptionClear, JNIEnv*, env);
MOCKABLE_FUNCTION(JNICALL, void, ExceptionDescribe, JNIEnv*, env);

//JVM function mocks
MOCKABLE_FUNCTION(JNICALL, jint, AttachCurrentThread, JavaVM*, vm, void**, penv, void*, args);

MOCKABLE_FUNCTION(JNICALL, jint, DetachCurrentThread, JavaVM*, vm);

MOCKABLE_FUNCTION(JNICALL, jint, DestroyJavaVM, JavaVM*, vm);

jint my_DestroyJavaVM(JavaVM* vm)
{
#ifdef __cplusplus
    delete vm->functions;
    delete vm;
    delete global_env->functions;
    delete global_env;
#else
    free((void*)(*global_env));
    free((void*)global_env);
    free((void*)(*vm));
    free((void*)vm);
#endif

    global_env = NULL;
    global_vm = NULL;
    return JNI_OK;
}

MOCKABLE_FUNCTION(JNICALL, jint, GetEnv, JavaVM*, jvm, void**, penv, jint, version);
jint my_GetEnv(JavaVM* jvm, void** penv, jint version)
{
    *penv = (void*)global_env;

    return 0;
}

//JNI mocks
MOCKABLE_FUNCTION(JNICALL, jint, JNI_GetDefaultJavaVMInitArgs, void*, args);
MOCKABLE_FUNCTION(JNICALL, jint, JNI_CreateJavaVM, JavaVM**, pvm, void**, penv, void*, args);
jint mock_JNI_CreateJavaVM(JavaVM** pvm, void** penv, void* args)
{
    if (!global_env && !global_vm)
    {
        struct JNINativeInterface_ env = {
            0, 0, 0, 0,

            NULL, NULL, FindClass, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
            NULL, ExceptionOccurred, ExceptionDescribe, ExceptionClear, NULL, NULL, NULL, NewGlobalRef, DeleteGlobalRef, DeleteLocalRef,
            NULL, NULL, NULL, NULL, NULL, NewObjectV, NULL, GetObjectClass, NULL, GetMethodID,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, CallVoidMethodV, NULL,
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
            NULL, NULL, NULL, NewStringUTF, NULL, NULL, NULL, GetArrayLength, NULL, NULL,
            NULL, NULL, NewByteArray, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL, NULL, NULL, GetByteArrayRegion, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL, SetByteArrayRegion, NULL, NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
        };

        struct JNIInvokeInterface_ vm = {
            0,
            0,
            0,

            DestroyJavaVM,
            AttachCurrentThread,
            DetachCurrentThread,
            GetEnv,
            NULL
        };

#ifdef __cplusplus
        (*pvm) = new JavaVM();
        (*pvm)->functions = new JNIInvokeInterface_(vm);
        global_vm = *pvm;

        (*penv) = new JNIEnv();
        ((JNIEnv*)(*penv))->functions = new JNINativeInterface_(env);
        global_env = (JNIEnv*)(*penv);

#else
        (*pvm) = (JavaVM*)malloc(sizeof(JavaVM));
        *(*pvm) = (struct JNIInvokeInterface_ *)malloc(sizeof(struct JNIInvokeInterface_));
        *(struct JNIInvokeInterface_ *)**pvm = vm;
        global_vm = *pvm;

        (*penv) = (void*)((JNIEnv*)malloc(sizeof(JNIEnv)));
        *((JNIEnv*)(*penv)) = malloc(sizeof(struct JNINativeInterface_));
        *(struct JNINativeInterface_*)(*((JNIEnv*)(*penv))) = env;
        global_env = *penv;
#endif

        return JNI_OK;
    }
    else
    {
        return JNI_EEXIST;
    }
}
MOCKABLE_FUNCTION(JNICALL, jint, JNI_GetCreatedJavaVMs, JavaVM**, pvm, jsize, count, jsize*, size);
jint mock_JNI_GetCreatedJavaVMs(JavaVM** pvm, jsize num, jsize* vmCount)
{
    *pvm = global_vm;

    return 0;
}

//parson mocks
MOCKABLE_FUNCTION(, JSON_Value*, json_parse_string, const char*, string);
JSON_Value* my_json_parse_string(const char* string)
{
    JSON_Value* value = NULL;
    if (string != NULL)
    {
        value = (JSON_Value*)malloc(1);
}
    return value;
}

MOCK_FUNCTION_WITH_CODE(, JSON_Object*, json_value_get_object, const JSON_Value*, value)
JSON_Object* obj = NULL;
if (value != NULL)
{
    obj = (JSON_Object*)0x42;
}
MOCK_FUNCTION_END(obj)

MOCK_FUNCTION_WITH_CODE(, void, json_free_serialized_string, char*, string)
free(string);
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void, json_value_free, JSON_Value*, value)
free(value);
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, const char*, json_object_get_string, const JSON_Object*, object, const char*, name)
const char* str = NULL;
if (object != NULL && name != NULL)
{
    str = "hello_world";
}
MOCK_FUNCTION_END(str)

MOCK_FUNCTION_WITH_CODE(, char*, json_serialize_to_string, const JSON_Value*, value)
char* serialized_string = NULL;
if (value != NULL)
{
    serialized_string = (char*)malloc(strlen("hello_world") + 1);
    sprintf(serialized_string, "%s", "hello_world");
}
MOCK_FUNCTION_END(serialized_string)

MOCK_FUNCTION_WITH_CODE(, JSON_Object*, json_object_get_object, const JSON_Object*, object, const char*, name)
JSON_Object* obj = NULL;
if (object != NULL && name != NULL)
{
    obj = (JSON_Object*)0x42;
}
MOCK_FUNCTION_END(obj)

MOCK_FUNCTION_WITH_CODE(, double, json_object_get_number, const JSON_Object*, object, const char*, name)
double num = 0;
if (object != NULL && name != NULL)
{
    num = 1;
}
MOCK_FUNCTION_END(num)

MOCK_FUNCTION_WITH_CODE(, int, json_object_get_boolean, const JSON_Object*, object, const char*, name)
int num = -1;
if (object != NULL && name != NULL)
{
    num = 1;
}
MOCK_FUNCTION_END(num)

MOCK_FUNCTION_WITH_CODE(, JSON_Array*, json_object_get_array, const JSON_Object*, object, const char*, name)
JSON_Array* arr = NULL;
if (object != NULL && name != NULL)
{
    arr = (JSON_Array*)0x42;
}
MOCK_FUNCTION_END(arr)

MOCK_FUNCTION_WITH_CODE(, JSON_Value*, json_object_get_value, const JSON_Object*, object, const char*, name)
JSON_Value* value = NULL;
if (object != NULL && name != NULL)
{
    value = (JSON_Value*)0x42;
}
MOCK_FUNCTION_END(value)

MOCK_FUNCTION_WITH_CODE(, size_t, json_array_get_count, const JSON_Array*, arr)
size_t num = -1;
if (arr != NULL)
{
    num = 1;
}
MOCK_FUNCTION_END(num)

MOCK_FUNCTION_WITH_CODE(, const char*, json_array_get_string, const JSON_Array*, arr, size_t, index)
const char* str = NULL;
if (arr != NULL && index >= 0)
{
    str = "hello_world";
}
MOCK_FUNCTION_END(str)

//=============================================================================
//HOOKS
//=============================================================================
#ifdef __cplusplus
extern "C"
{
#endif
    VECTOR_HANDLE real_VECTOR_create(size_t elementSize);
    void real_VECTOR_destroy(VECTOR_HANDLE handle);

    /* insertion */
    int real_VECTOR_push_back(VECTOR_HANDLE handle, const void* elements, size_t numElements);

    /* removal */
    void real_VECTOR_erase(VECTOR_HANDLE handle, void* elements, size_t numElements);
    void real_VECTOR_clear(VECTOR_HANDLE handle);

    /* access */
    void* real_VECTOR_element(const VECTOR_HANDLE handle, size_t index);
    void* real_VECTOR_front(const VECTOR_HANDLE handle);
    void* real_VECTOR_back(const VECTOR_HANDLE handle);
    void* real_VECTOR_find_if(const VECTOR_HANDLE handle, PREDICATE_FUNCTION pred, const void* value);

    /* capacity */
    size_t real_VECTOR_size(const VECTOR_HANDLE handle);

    //String mocks
    STRING_HANDLE real_STRING_construct(const char* psz);
    void real_STRING_delete(STRING_HANDLE handle);
    const char* real_STRING_c_str(STRING_HANDLE handle);
    STRING_HANDLE real_STRING_clone(STRING_HANDLE handle);
    int real_STRING_concat(STRING_HANDLE handle, const char* s2);

    //mallocAndStrcpy_s
    int real_mallocAndStrcpy_s(char** destination, const char* source);
#ifdef __cplusplus
}
#endif
void* my_gballoc_malloc(size_t size)
{
    void* result = NULL;
    if (malloc_will_fail == false)
    {
        result = malloc(size);
    }

    return result;
}

void my_gballoc_free(void* ptr)
{
    free(ptr);
}

//Java Module Host Manager Mocks
JAVA_MODULE_HOST_MANAGER_HANDLE my_JavaModuleHostManager_Create(JAVA_MODULE_HOST_CONFIG* config)
{
    if (config == NULL)
    {
        global_manager = NULL;
    }
    else if (global_manager == NULL)
    {
        global_manager = (JAVA_MODULE_HOST_MANAGER_HANDLE)malloc(0x42);
        module_manager_count = 0;
    }
    return global_manager;
}

void my_JavaModuleHostManager_Destroy(JAVA_MODULE_HOST_MANAGER_HANDLE manager)
{
    if (manager != NULL && manager == global_manager)
    {
        if (module_manager_count == 0)
        {
                free(manager);
                manager = NULL;
                global_manager = NULL;
        }
    }
}

JAVA_MODULE_HOST_MANAGER_RESULT my_JavaModuleHostManager_Add(JAVA_MODULE_HOST_MANAGER_HANDLE manager)
{
    if (manager != NULL)
    {
        module_manager_count++;
        return MANAGER_OK;
    }
    else
    {
        return MANAGER_ERROR;
    }
}

JAVA_MODULE_HOST_MANAGER_RESULT my_JavaModuleHostManager_Remove(JAVA_MODULE_HOST_MANAGER_HANDLE manager)
{
    if (manager != NULL)
    {
        module_manager_count--;
        return MANAGER_OK;
    }
    else
    {
        return MANAGER_ERROR;
    }
}

size_t my_JavaModuleHostManager_Size(JAVA_MODULE_HOST_MANAGER_HANDLE manager)
{
    return manager == NULL ? -1 : module_manager_count;
}

void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error");
}

#include "azure_c_shared_utility/gballoc.h"

#undef ENABLE_MOCKS

/*these are simple cached variables*/
static pfModule_ParseConfigurationFromJson  JavaModuleHost_ParseConfigurationFromJson = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_FreeConfiguration           JavaModuleHost_FreeConfiguration = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Create                      JavaModuleHost_Create = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Destroy                     JavaModuleHost_Destroy = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Receive                     JavaModuleHost_Receive = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Start                       JavaModuleHost_Start = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/

IMPLEMENT_UMOCK_C_ENUM_TYPE(JAVA_MODULE_HOST_MANAGER_RESULT, JAVA_MODULE_HOST_MANAGER_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(BROKER_RESULT, BROKER_RESULT_VALUES);

BEGIN_TEST_SUITE(JavaModuleHost_UnitTests)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
    JavaModuleHost_ParseConfigurationFromJson = MODULE_PARSE_CONFIGURATION_FROM_JSON(apis);
    JavaModuleHost_FreeConfiguration = MODULE_FREE_CONFIGURATION(apis);
    JavaModuleHost_Create = MODULE_CREATE(apis);
    JavaModuleHost_Destroy = MODULE_DESTROY(apis);
    JavaModuleHost_Receive = MODULE_RECEIVE(apis);
    JavaModuleHost_Start = MODULE_START(apis);

    umock_c_init(on_umock_c_error);
    umocktypes_charptr_register_types();
    umocktypes_stdint_register_types();

    //JNI Fail Returns
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FindClass, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(GetObjectClass, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(GetMethodID, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(NewObjectV, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(NewStringUTF, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(NewGlobalRef, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(AttachCurrentThread, JNI_ERR);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(DetachCurrentThread, JNI_ERR);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(JNI_CreateJavaVM, JNI_ERR);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(ExceptionOccurred, (jthrowable)0x42);

    //JNI Hooks
    REGISTER_GLOBAL_MOCK_HOOK(FindClass, my_FindClass);
    REGISTER_GLOBAL_MOCK_HOOK(GetObjectClass, my_GetObjectClass);
    REGISTER_GLOBAL_MOCK_HOOK(GetMethodID, my_GetMethodID);
    REGISTER_GLOBAL_MOCK_HOOK(NewObjectV, my_NewObject);
    REGISTER_GLOBAL_MOCK_HOOK(NewStringUTF, my_NewStringUTF);
    REGISTER_GLOBAL_MOCK_HOOK(NewByteArray, my_NewByteArray);
    REGISTER_GLOBAL_MOCK_HOOK(GetArrayLength, my_GetArrayLength);
    REGISTER_GLOBAL_MOCK_HOOK(DeleteLocalRef, my_DeleteLocalRef);
    REGISTER_GLOBAL_MOCK_HOOK(NewGlobalRef, my_NewGlobalRef);
    REGISTER_GLOBAL_MOCK_HOOK(DeleteGlobalRef, my_DeleteGlobalRef);
    REGISTER_GLOBAL_MOCK_HOOK(ExceptionOccurred, my_ExceptionOccurred);
    REGISTER_GLOBAL_MOCK_HOOK(DestroyJavaVM, my_DestroyJavaVM);
    REGISTER_GLOBAL_MOCK_HOOK(GetEnv, my_GetEnv);

    //gballoc Hooks
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    //Vector Hooks
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_create, real_VECTOR_create);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_push_back, real_VECTOR_push_back);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_size, real_VECTOR_size);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_destroy, real_VECTOR_destroy);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_element, real_VECTOR_element);

    //String Hooks
    REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, real_STRING_construct);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, real_STRING_delete);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_c_str, real_STRING_c_str);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_clone, real_STRING_clone);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_concat, real_STRING_concat);

    //mallocAndStrcpy_s Hook
    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, real_mallocAndStrcpy_s);

    //Message Hooks
    REGISTER_GLOBAL_MOCK_HOOK(Message_CreateFromByteArray, my_Message_CreateFromByteArray);
    REGISTER_GLOBAL_MOCK_HOOK(Message_ToByteArray, my_MessageToByteArray);
    REGISTER_GLOBAL_MOCK_HOOK(Message_Destroy, my_Message_Destroy);

    //JavaModuleHostManager Hooks
    REGISTER_GLOBAL_MOCK_HOOK(JavaModuleHostManager_Create, my_JavaModuleHostManager_Create);
    REGISTER_GLOBAL_MOCK_HOOK(JavaModuleHostManager_Destroy, my_JavaModuleHostManager_Destroy);
    REGISTER_GLOBAL_MOCK_HOOK(JavaModuleHostManager_Add, my_JavaModuleHostManager_Add);
    REGISTER_GLOBAL_MOCK_HOOK(JavaModuleHostManager_Remove, my_JavaModuleHostManager_Remove);
    REGISTER_GLOBAL_MOCK_HOOK(JavaModuleHostManager_Size, my_JavaModuleHostManager_Size);
    
    //JVM Hooks
    REGISTER_GLOBAL_MOCK_HOOK(JNI_CreateJavaVM, mock_JNI_CreateJavaVM);
    REGISTER_GLOBAL_MOCK_HOOK(JNI_GetCreatedJavaVMs, mock_JNI_GetCreatedJavaVMs);

    //parson Hooks
    REGISTER_GLOBAL_MOCK_HOOK(json_parse_string, my_json_parse_string);

    REGISTER_UMOCK_ALIAS_TYPE(MODULE_HANDLE, void*);

    REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_HANDLE, void*);

    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);

    REGISTER_UMOCK_ALIAS_TYPE(VECTOR_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const VECTOR_HANDLE, void*);

    REGISTER_UMOCK_ALIAS_TYPE(JAVA_MODULE_HOST_MANAGER_HANDLE, void*);

    //JNI Alias Types
    REGISTER_UMOCK_ALIAS_TYPE(JavaVM, void*);
    REGISTER_UMOCK_ALIAS_TYPE(JavaVM*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(JavaVM**, void*);
    REGISTER_UMOCK_ALIAS_TYPE(jint, int32_t);
    REGISTER_UMOCK_ALIAS_TYPE(jclass, void*);
    REGISTER_UMOCK_ALIAS_TYPE(jmethodID, void*);
    REGISTER_UMOCK_ALIAS_TYPE(jobject, void*);
    REGISTER_UMOCK_ALIAS_TYPE(jsize, int);
    REGISTER_UMOCK_ALIAS_TYPE(jthrowable, int);
    REGISTER_UMOCK_ALIAS_TYPE(jbyteArray, void*);
    REGISTER_UMOCK_ALIAS_TYPE(jsize, int);
    REGISTER_UMOCK_ALIAS_TYPE(const jbyte*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(jarray, void*);
    REGISTER_UMOCK_ALIAS_TYPE(BROKER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const char*, char*);

    REGISTER_UMOCK_ALIAS_TYPE(void**, void*);
    REGISTER_UMOCK_ALIAS_TYPE(va_list, void*);

    REGISTER_TYPE(JAVA_MODULE_HOST_MANAGER_RESULT, JAVA_MODULE_HOST_MANAGER_RESULT);
    REGISTER_TYPE(BROKER_RESULT, BROKER_RESULT);
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
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    free(global_manager);
    global_manager = NULL;
    global_env = NULL;
    global_vm = NULL;
    TEST_MUTEX_RELEASE(g_testByTest);
}

//=============================================================================
//JavaModuleHost_Create tests
//=============================================================================

/*Tests_SRS_JAVA_MODULE_HOST_14_001: [This function shall return NULL if broker is NULL.]*/
TEST_FUNCTION(JavaModuleHost_Create_with_NULL_broker_fails)
{
    //Arrange

    //Act
    MODULE_HANDLE result = JavaModuleHost_Create(NULL, &config);

    //Assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
}

/*Tests_SRS_JAVA_MODULE_HOST_14_002: [This function shall return NULL if configuration is NULL. ]*/
TEST_FUNCTION(JavaModuleHost_Create_with_NULL_config_fails)
{
    //Arrange

    //Act
    MODULE_HANDLE result = JavaModuleHost_Create((BROKER_HANDLE)0x42, NULL);

    //Assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
}

/*Tests_SRS_JAVA_MODULE_HOST_14_003: [This function shall return NULL if class_name is NULL.]*/
TEST_FUNCTION(JavaModuleHost_Create_with_NULL_class_name_fails)
{
    //Arrange

    JAVA_MODULE_HOST_CONFIG no_class_name_config =
    {
        NULL,
        "{hello}",
        (JVM_OPTIONS*)0x42
    };

    //Act
    MODULE_HANDLE result = JavaModuleHost_Create((BROKER_HANDLE)0x42, &no_class_name_config);

    //Assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JAVA_MODULE_HOST_14_004: [This function shall return NULL upon any underlying API call failure.]*/
TEST_FUNCTION(JavaModuleHost_Create_malloc_failure_fails)
{
    //Arrange
    malloc_will_fail = true;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
        .IgnoreArgument(1);

    //Act
    MODULE_HANDLE result = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);

    //Assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JAVA_MODULE_HOST_14_004: [This function shall return NULL upon any underlying API call failure.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_035: [If any operation fails, the function shall delete the STRING_HANDLE structures, VECTOR_HANDLE and JavaVMOption array.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_013: [This function shall return NULL if a JVM could not be created or found.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_016: [This function shall return NULL if any returned jclass, jmethodID, or jobject is NULL.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_017: [This function shall return NULL if any JNI function fails.]*/
TEST_FUNCTION(JavaModuleHost_Create_API_failure_tests)
{
    VECTOR_HANDLE additional_options = VECTOR_create(sizeof(STRING_HANDLE));
    STRING_HANDLE option1 = STRING_construct("option1");
    STRING_HANDLE option2 = STRING_construct("option2");
    STRING_HANDLE option3 = STRING_construct("option3");
    VECTOR_push_back(additional_options, &option1, 1);
    VECTOR_push_back(additional_options, &option2, 1);
    VECTOR_push_back(additional_options, &option3, 1);

    umock_c_reset_all_calls();

    JVM_OPTIONS options = {
        "class/path",
        "library/path",
        8,
        false,
        -1,
        false,
        additional_options
    };

    JAVA_MODULE_HOST_CONFIG config2 =
    {
        "TestClass",
        "{hello}",
        &options
    };

    //Arrange

    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(JavaModuleHostManager_Create(&config2));

    STRICT_EXPECTED_CALL(VECTOR_create(sizeof(STRING_HANDLE)));
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JavaVMOption)*5));

    STRICT_EXPECTED_CALL(STRING_construct("-Djava.class.path="));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, config2.options->class_path))
        .IgnoreArgument(1)
        .SetFailReturn(-1);
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreAllArguments()
        .SetFailReturn(-1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(STRING_construct("-Djava.library.path="));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, config2.options->library_path))
        .IgnoreArgument(1)
        .SetFailReturn(-1);
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreAllArguments()
        .SetFailReturn(-1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(STRING_clone(option1));
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1).IgnoreArgument(2)
        .SetFailReturn(-1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_clone(option2));
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 2))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_clone(option3));
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn(-1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();


    STRICT_EXPECTED_CALL(JNI_CreateJavaVM(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetFailReturn(JNI_ERR);

    //30
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 2))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 3))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 4))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(JavaModuleHostManager_Add(IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetFailReturn(MANAGER_ERROR);

    STRICT_EXPECTED_CALL(FindClass(IGNORED_PTR_ARG, BROKER_CLASS_NAME))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(GetMethodID(IGNORED_PTR_ARG, IGNORED_PTR_ARG, CONSTRUCTOR_METHOD_NAME, BROKER_CONSTRUCTOR_DESCRIPTOR))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(NewObjectV(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(FindClass(IGNORED_PTR_ARG, config2.class_name))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(GetMethodID(IGNORED_PTR_ARG, IGNORED_PTR_ARG, CONSTRUCTOR_METHOD_NAME, MODULE_CONSTRUCTOR_DESCRIPTOR))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(NewStringUTF(IGNORED_PTR_ARG, config.configuration_json))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(NewObjectV(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(NewGlobalRef(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    umock_c_negative_tests_snapshot();

    //act
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (i != 3 &&
            i != 8 &&
            i != 12 &&
            i != 13 &&
            i != 17 &&
            i != 18 &&
            i != 22 &&
            i != 23 &&
            i != 27 &&
            i != 28 &&
            (i < 30 || i > 47) &&
            i != 50 &&
            i != 52 &&
            i != 54 &&
            i != 56 &&
            i != 58 &&
            i != 60 &&
            i != 62)
        {
            // arrange
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            MODULE_HANDLE result = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config2);

            // assert
            ASSERT_ARE_EQUAL(void_ptr, NULL, result);
        }
    }
    umock_c_negative_tests_deinit();

    //Cleanup
    STRING_delete(option1);
    STRING_delete(option2);
    STRING_delete(option3);
    VECTOR_destroy(additional_options);

}

/*Tests_SRS_JAVA_MODULE_HOST_14_005: [This function shall return a non-NULL MODULE_HANDLE when successful. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_006: [This function shall allocate memory for an instance of a JAVA_MODULE_HANDLE_DATA structure to be used as the backing structure for this module. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_037: [This function shall get a singleton instance of a JavaModuleHostManager.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_007: [This function shall initialize a JavaVMInitArgs structure using the JVM_OPTIONS structure configuration->options. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_009: [This function shall allocate memory for an array of JavaVMOption structures and initialize each with each option provided. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_010: [If this is the first Java module to load, this function shall create the JVM using the JavaVMInitArgs through a call to JNI_CreateJavaVM and save the JavaVM and JNIEnv pointers in the JAVA_MODULE_HANDLE_DATA. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_012: [This function shall increment the count of modules in the JavaModuleHostManager. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_014: [This function shall find the Broker Java class, get the constructor, and create a Broker Java object.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_015 :[This function shall find the user - defined Java module class using configuration->class_name, get the constructor, and create an instance of this module object.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_018: [The function shall save a new global reference to the Java module object in JAVA_MODULE_HANDLE_DATA->module. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_030: [The function shall set the JavaVMInitArgs structures nOptions, version and JavaVMOption* options member variables]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_031: [The function shall create a new VECTOR_HANDLE to store the option strings.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_032: [The function shall construct a new STRING_HANDLE for each option.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_033: [The function shall concatenate the user supplied options to the option key names.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_034: [The function shall push the new STRING_HANDLE onto the newly created vector.]*/
TEST_FUNCTION(JavaModuleHost_Create_initializes_JavaVMInitArgs_structure_no_additional_args_success)
{
    JVM_OPTIONS options = {
        "class/path",
        "library/path",
        8,
        false,
        -1,
        false,
        NULL
    };

    JAVA_MODULE_HOST_CONFIG config2 =
    {
        "TestClass",
        "{hello}",
        &options
    };

    //Arrange

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(JavaModuleHostManager_Create(&config2));

    STRICT_EXPECTED_CALL(VECTOR_create(sizeof(STRING_HANDLE)));
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JavaVMOption) * 2));
    STRICT_EXPECTED_CALL(STRING_construct("-Djava.class.path="));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, config2.options->class_path))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(STRING_construct("-Djava.library.path="));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, config2.options->library_path))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(JNI_CreateJavaVM(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(JavaModuleHostManager_Add(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(FindClass(IGNORED_PTR_ARG, BROKER_CLASS_NAME))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(GetMethodID(IGNORED_PTR_ARG, IGNORED_PTR_ARG, CONSTRUCTOR_METHOD_NAME, BROKER_CONSTRUCTOR_DESCRIPTOR))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(NewObjectV(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(FindClass(IGNORED_PTR_ARG, config2.class_name))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(GetMethodID(IGNORED_PTR_ARG, IGNORED_PTR_ARG, CONSTRUCTOR_METHOD_NAME, MODULE_CONSTRUCTOR_DESCRIPTOR))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(NewStringUTF(IGNORED_PTR_ARG, config.configuration_json))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(NewObjectV(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(NewGlobalRef(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();


    //Act
    MODULE_HANDLE result = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config2);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    JavaModuleHost_Destroy(result);

}

/*Tests_SRS_JAVA_MODULE_HOST_14_005: [This function shall return a non-NULL MODULE_HANDLE when successful. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_006: [This function shall allocate memory for an instance of a JAVA_MODULE_HANDLE_DATA structure to be used as the backing structure for this module. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_037: [This function shall get a singleton instance of a JavaModuleHostManager.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_007: [This function shall initialize a JavaVMInitArgs structure using the JVM_OPTIONS structure configuration->options. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_009: [This function shall allocate memory for an array of JavaVMOption structures and initialize each with each option provided. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_010: [If this is the first Java module to load, this function shall create the JVM using the JavaVMInitArgs through a call to JNI_CreateJavaVM and save the JavaVM and JNIEnv pointers in the JAVA_MODULE_HANDLE_DATA. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_012: [This function shall increment the count of modules in the JavaModuleHostManager. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_014: [This function shall find the Broker Java class, get the constructor, and create a Broker Java object.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_015 :[This function shall find the user - defined Java module class using configuration->class_name, get the constructor, and create an instance of this module object.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_018: [The function shall save a new global reference to the Java module object in JAVA_MODULE_HANDLE_DATA->module. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_030: [The function shall set the JavaVMInitArgs structures nOptions, version and JavaVMOption* options member variables]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_031: [The function shall create a new VECTOR_HANDLE to store the option strings.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_032: [The function shall construct a new STRING_HANDLE for each option.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_036: [The function shall copy any additional user options into a STRING_HANDLE.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_033: [The function shall concatenate the user supplied options to the option key names.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_034: [The function shall push the new STRING_HANDLE onto the newly created vector.]*/
TEST_FUNCTION(JavaModuleHost_Create_initializes_JavaVMInitArgs_structure_additional_args_success)
{
    VECTOR_HANDLE additional_options = VECTOR_create(sizeof(STRING_HANDLE));
    STRING_HANDLE option1 = STRING_construct("option1");
    STRING_HANDLE option2 = STRING_construct("option2");
    STRING_HANDLE option3 = STRING_construct("option3");
    VECTOR_push_back(additional_options, &option1, 1);
    VECTOR_push_back(additional_options, &option2, 1);
    VECTOR_push_back(additional_options, &option3, 1);

    umock_c_reset_all_calls();

    JVM_OPTIONS options = {
        "class/path",
        "library/path",
        8,
        false,
        -1,
        false,
        additional_options
    };

    JAVA_MODULE_HOST_CONFIG config2 =
    {
        "TestClass",
        "{hello}",
        &options
    };

    //Arrange

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(JavaModuleHostManager_Create(&config2));

    STRICT_EXPECTED_CALL(VECTOR_create(sizeof(STRING_HANDLE)));
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(JavaVMOption) * 5));

    STRICT_EXPECTED_CALL(STRING_construct("-Djava.class.path="));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, config2.options->class_path))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(STRING_construct("-Djava.library.path="));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, config2.options->library_path))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_clone(option1));
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_clone(option2));
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 2))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_clone(option3));
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();


    STRICT_EXPECTED_CALL(JNI_CreateJavaVM(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 0))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 2))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 3))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 4))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(JavaModuleHostManager_Add(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(FindClass(IGNORED_PTR_ARG, BROKER_CLASS_NAME))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(GetMethodID(IGNORED_PTR_ARG, IGNORED_PTR_ARG, CONSTRUCTOR_METHOD_NAME, BROKER_CONSTRUCTOR_DESCRIPTOR))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(NewObjectV(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(FindClass(IGNORED_PTR_ARG, config2.class_name))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(GetMethodID(IGNORED_PTR_ARG, IGNORED_PTR_ARG, CONSTRUCTOR_METHOD_NAME, MODULE_CONSTRUCTOR_DESCRIPTOR))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(NewStringUTF(IGNORED_PTR_ARG, config.configuration_json))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(NewObjectV(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(NewGlobalRef(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();


    //Act
    MODULE_HANDLE result = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config2);

    //Assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    JavaModuleHost_Destroy(result);
    STRING_delete(option1);
    STRING_delete(option2);
    STRING_delete(option3);
    VECTOR_destroy(additional_options);
}

/*Tests_SRS_JAVA_MODULE_HOST_14_005: [This function shall return a non-NULL MODULE_HANDLE when successful. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_006: [This function shall allocate memory for an instance of a JAVA_MODULE_HANDLE_DATA structure to be used as the backing structure for this module. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_037: [This function shall get a singleton instance of a JavaModuleHostManager.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_008: [If configuration->options is NULL, JavaVMInitArgs shall be initialized using default values. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_009: [This function shall allocate memory for an array of JavaVMOption structures and initialize each with each option provided. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_010: [If this is the first Java module to load, this function shall create the JVM using the JavaVMInitArgs through a call to JNI_CreateJavaVM and save the JavaVM and JNIEnv pointers in the JAVA_MODULE_HANDLE_DATA. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_012: [This function shall increment the count of modules in the JavaModuleHostManager. ]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_014: [This function shall find the Broker Java class, get the constructor, and create a Broker Java object.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_015 :[This function shall find the user - defined Java module class using configuration->class_name, get the constructor, and create an instance of this module object.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_018: [The function shall save a new global reference to the Java module object in JAVA_MODULE_HANDLE_DATA->module. ]*/
TEST_FUNCTION(JavaModuleHost_Create_allocates_structure_success)
{
    //Arrange

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(JavaModuleHostManager_Create(&config));

    STRICT_EXPECTED_CALL(JNI_GetDefaultJavaVMInitArgs(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(JNI_CreateJavaVM(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(JavaModuleHostManager_Add(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(FindClass(IGNORED_PTR_ARG, BROKER_CLASS_NAME))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(GetMethodID(IGNORED_PTR_ARG, IGNORED_PTR_ARG, CONSTRUCTOR_METHOD_NAME, BROKER_CONSTRUCTOR_DESCRIPTOR))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(NewObjectV(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(FindClass(IGNORED_PTR_ARG, config.class_name))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(GetMethodID(IGNORED_PTR_ARG, IGNORED_PTR_ARG, CONSTRUCTOR_METHOD_NAME, MODULE_CONSTRUCTOR_DESCRIPTOR))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(NewStringUTF(IGNORED_PTR_ARG, config.configuration_json))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(NewObjectV(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(NewGlobalRef(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    //Act
    MODULE_HANDLE result = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);

    //Assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    JavaModuleHost_Destroy(result);
}

/*Tests_SRS_JAVA_MODULE_HOST_14_011: [If the JVM was previously created, the function shall get a pointer to that JavaVM pointer and JNIEnv environment pointer. ]*/
TEST_FUNCTION(JavaModuleHost_Create_gets_previously_created_JVM_success)
{
    //Arrange
    MODULE_HANDLE handle = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    umock_c_reset_all_calls();

    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(JavaModuleHostManager_Create(&config));

    STRICT_EXPECTED_CALL(JNI_GetDefaultJavaVMInitArgs(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(JNI_CreateJavaVM(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .SetFailReturn(JNI_EEXIST);

    STRICT_EXPECTED_CALL(JNI_GetCreatedJavaVMs(IGNORED_PTR_ARG, 1, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(GetEnv(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(JavaModuleHostManager_Add(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(FindClass(IGNORED_PTR_ARG, BROKER_CLASS_NAME))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(GetMethodID(IGNORED_PTR_ARG, IGNORED_PTR_ARG, CONSTRUCTOR_METHOD_NAME, BROKER_CONSTRUCTOR_DESCRIPTOR))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(NewObjectV(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(FindClass(IGNORED_PTR_ARG, config.class_name))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(GetMethodID(IGNORED_PTR_ARG, IGNORED_PTR_ARG, CONSTRUCTOR_METHOD_NAME, MODULE_CONSTRUCTOR_DESCRIPTOR))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(NewStringUTF(IGNORED_PTR_ARG, config.configuration_json))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(NewObjectV(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(NewGlobalRef(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    umock_c_negative_tests_snapshot();

    //Act
    umock_c_negative_tests_fail_call(3);
    MODULE_HANDLE handle2 = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);

    //Assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_IS_NOT_NULL(handle2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    JavaModuleHost_Destroy(handle);
    JavaModuleHost_Destroy(handle2);

    umock_c_negative_tests_deinit();
}

//=============================================================================
//JavaModuleHost_Receive tests
//=============================================================================

/*Tests_SRS_JAVA_MODULE_HOST_14_023: [This function shall serialize message.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_042: [This function shall attach the JVM to the current thread.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_043: [This function shall create a new jbyteArray for the serialized message.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_044: [This function shall set the contents of the jbyteArray to the serialized_message.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_045: [This function shall get the user - defined Java module class using the module parameter and get the receive() method.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_024: [This function shall call the void receive(byte[] source) method of the Java module object passing the serialized message.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_046: [This function shall detach the JVM from the current thread.]*/
TEST_FUNCTION(JavaModuleHost_Receive_success)
{
    //Arrange
    const unsigned char msg[] =
    {
        0xA1, 0x60,             /*header*/
        0x00, 0x00, 0x00, 14,   /*size of this array*/
        0x00, 0x00, 0x00, 0x00, /*zero properties*/
        0x00, 0x00, 0x00, 0x00  /*zero message content size*/
    };

    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    MESSAGE_HANDLE message = Message_CreateFromByteArray(msg, sizeof(msg));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Message_ToByteArray(message, NULL,0));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Message_ToByteArray(message, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    STRICT_EXPECTED_CALL(AttachCurrentThread(IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(NewByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(SetByteArrayRegion(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 0, IGNORED_NUM_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(4)
        .IgnoreArgument(5);

    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(GetObjectClass(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(GetMethodID(IGNORED_PTR_ARG, IGNORED_PTR_ARG, MODULE_RECEIVE_METHOD_NAME, MODULE_RECEIVE_DESCRIPTOR))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(CallVoidMethodV(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(ExceptionOccurred(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(DeleteLocalRef(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(DetachCurrentThread(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //Act
    JavaModuleHost_Receive(module, message);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    Message_Destroy(message);
    JavaModuleHost_Destroy(module);
}

/*Tests_SRS_JAVA_MODULE_HOST_14_022: [ This function shall do nothing if module or message is NULL. ]*/
TEST_FUNCTION(JavaModuleHost_Receive_module_NULL_failure)
{
    //Arrange
    const unsigned char msg[] =
    {
        0xA1, 0x60,             /*header*/
        0x00, 0x00, 0x00, 14,   /*size of this array*/
        0x00, 0x00, 0x00, 0x00, /*zero properties*/
        0x00, 0x00, 0x00, 0x00  /*zero message content size*/
    };

    MESSAGE_HANDLE message = Message_CreateFromByteArray(msg, sizeof(msg));
    umock_c_reset_all_calls();

    //Act
    JavaModuleHost_Receive(NULL, message);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    Message_Destroy(message);
}

/*Tests_SRS_JAVA_MODULE_HOST_14_022: [ This function shall do nothing if module or message is NULL. ]*/
TEST_FUNCTION(JavaModuleHost_Receive_message_NULL_failure)
{
    //Arrange
    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    umock_c_reset_all_calls();
    
    //Act
    JavaModuleHost_Receive(module, NULL);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    JavaModuleHost_Destroy(module);
}

/*Tests_SRS_JAVA_MODULE_HOST_14_047: [This function shall exit if any underlying function fails.]*/
TEST_FUNCTION(JavaModuleHost_Receive_Message_ToByteArray_failure)
{
    //Arrange
    const unsigned char msg[] =
    {
        0xA1, 0x60,             /*header*/
        0x00, 0x00, 0x00, 14,   /*size of this array*/
        0x00, 0x00, 0x00, 0x00, /*zero properties*/
        0x00, 0x00, 0x00, 0x00  /*zero message content size*/
    };

    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    MESSAGE_HANDLE message = Message_CreateFromByteArray(msg, sizeof(msg));
    umock_c_reset_all_calls();

    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(Message_ToByteArray(message, NULL, 0))
        .SetFailReturn(-1);


    umock_c_negative_tests_snapshot();

    //Act
    umock_c_negative_tests_fail_call(0);
    JavaModuleHost_Receive(module, message);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    Message_Destroy(message);
    JavaModuleHost_Destroy(module);

    umock_c_negative_tests_deinit();

}

/*Tests_SRS_JAVA_MODULE_HOST_14_047: [This function shall exit if any underlying function fails.]*/
TEST_FUNCTION(JavaModuleHost_Receive_Message_allocate_failure)
{
    //Arrange
    const unsigned char msg[] =
    {
        0xA1, 0x60,             /*header*/
        0x00, 0x00, 0x00, 14,   /*size of this array*/
        0x00, 0x00, 0x00, 0x00, /*zero properties*/
        0x00, 0x00, 0x00, 0x00  /*zero message content size*/
    };

    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    MESSAGE_HANDLE message = Message_CreateFromByteArray(msg, sizeof(msg));
    umock_c_reset_all_calls();

    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(Message_ToByteArray(message, NULL, 0));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);

    umock_c_negative_tests_snapshot();

    //Act
    umock_c_negative_tests_fail_call(1);
    JavaModuleHost_Receive(module, message);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    Message_Destroy(message);
    JavaModuleHost_Destroy(module);

    umock_c_negative_tests_deinit();

}
/*Tests_SRS_JAVA_MODULE_HOST_14_047: [This function shall exit if any underlying function fails.]*/
TEST_FUNCTION(JavaModuleHost_Receive_AttachCurrentThread_failure)
{
    //Arrange
    const unsigned char msg[] =
    {
        0xA1, 0x60,             /*header*/
        0x00, 0x00, 0x00, 14,   /*size of this array*/
        0x00, 0x00, 0x00, 0x00, /*zero properties*/
        0x00, 0x00, 0x00, 0x00  /*zero message content size*/
    };

    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    MESSAGE_HANDLE message = Message_CreateFromByteArray(msg, sizeof(msg));
    umock_c_reset_all_calls();

    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(Message_ToByteArray(message, NULL, 0));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Message_ToByteArray(message, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(AttachCurrentThread(global_vm, IGNORED_PTR_ARG, NULL))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    umock_c_negative_tests_snapshot();

    //Act
    umock_c_negative_tests_fail_call(3);
    JavaModuleHost_Receive(module, message);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    Message_Destroy(message);
    JavaModuleHost_Destroy(module);

    umock_c_negative_tests_deinit();
}

/*Tests_SRS_JAVA_MODULE_HOST_14_047: [This function shall exit if any underlying function fails.]*/
TEST_FUNCTION(JavaModuleHost_Receive_NewByteArray_failure)
{
    //Arrange
    const unsigned char msg[] =
    {
        0xA1, 0x60,             /*header*/
        0x00, 0x00, 0x00, 14,   /*size of this array*/
        0x00, 0x00, 0x00, 0x00, /*zero properties*/
        0x00, 0x00, 0x00, 0x00  /*zero message content size*/
    };

    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    MESSAGE_HANDLE message = Message_CreateFromByteArray(msg, sizeof(msg));
    umock_c_reset_all_calls();

    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(Message_ToByteArray(message, NULL, 0));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Message_ToByteArray(message, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(AttachCurrentThread(global_vm, IGNORED_PTR_ARG, NULL))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(NewByteArray(global_env, IGNORED_NUM_ARG))
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(DetachCurrentThread(global_vm));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    umock_c_negative_tests_snapshot();

    //Act
    umock_c_negative_tests_fail_call(4);
    JavaModuleHost_Receive(module, message);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    Message_Destroy(message);
    JavaModuleHost_Destroy(module);

    umock_c_negative_tests_deinit();
}

/*Tests_SRS_JAVA_MODULE_HOST_14_047: [This function shall exit if any underlying function fails.]*/
TEST_FUNCTION(JavaModuleHost_Receive_SetByteArrayRegion_failure)
{
    //Arrange
    const unsigned char msg[] =
    {
        0xA1, 0x60,             /*header*/
        0x00, 0x00, 0x00, 14,   /*size of this array*/
        0x00, 0x00, 0x00, 0x00, /*zero properties*/
        0x00, 0x00, 0x00, 0x00  /*zero message content size*/
    };

    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    MESSAGE_HANDLE message = Message_CreateFromByteArray(msg, sizeof(msg));
    umock_c_reset_all_calls();

    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(Message_ToByteArray(message, NULL, 0));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Message_ToByteArray(message, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(AttachCurrentThread(global_vm, IGNORED_PTR_ARG, NULL))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(NewByteArray(global_env, IGNORED_NUM_ARG))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(SetByteArrayRegion(global_env, IGNORED_PTR_ARG, 0, IGNORED_NUM_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(4)
        .IgnoreArgument(5);
    STRICT_EXPECTED_CALL(ExceptionOccurred(global_env));
    STRICT_EXPECTED_CALL(ExceptionDescribe(global_env));
    STRICT_EXPECTED_CALL(ExceptionClear(global_env));
    STRICT_EXPECTED_CALL(DeleteLocalRef(global_env, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(DetachCurrentThread(global_vm));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    umock_c_negative_tests_snapshot();

    //Act
    umock_c_negative_tests_fail_call(6);
    JavaModuleHost_Receive(module, message);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    Message_Destroy(message);
    JavaModuleHost_Destroy(module);

    umock_c_negative_tests_deinit();
}

/*Tests_SRS_JAVA_MODULE_HOST_14_047: [This function shall exit if any underlying function fails.]*/
TEST_FUNCTION(JavaModuleHost_Receive_GetObjectClass_failure)
{
    //Arrange
    const unsigned char msg[] =
    {
        0xA1, 0x60,             /*header*/
        0x00, 0x00, 0x00, 14,   /*size of this array*/
        0x00, 0x00, 0x00, 0x00, /*zero properties*/
        0x00, 0x00, 0x00, 0x00  /*zero message content size*/
    };

    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    MESSAGE_HANDLE message = Message_CreateFromByteArray(msg, sizeof(msg));
    umock_c_reset_all_calls();

    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(Message_ToByteArray(message, NULL, 0));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Message_ToByteArray(message, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(AttachCurrentThread(global_vm, IGNORED_PTR_ARG, NULL))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(NewByteArray(global_env, IGNORED_NUM_ARG))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(SetByteArrayRegion(global_env, IGNORED_PTR_ARG, 0, IGNORED_NUM_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(4)
        .IgnoreArgument(5);
    STRICT_EXPECTED_CALL(ExceptionOccurred(global_env));
    STRICT_EXPECTED_CALL(GetObjectClass(global_env, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(DeleteLocalRef(global_env, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(DetachCurrentThread(global_vm));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    umock_c_negative_tests_snapshot();

    //Act
    umock_c_negative_tests_fail_call(7);
    JavaModuleHost_Receive(module, message);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    Message_Destroy(message);
    JavaModuleHost_Destroy(module);

    umock_c_negative_tests_deinit();
}

/*Tests_SRS_JAVA_MODULE_HOST_14_047: [This function shall exit if any underlying function fails.]*/
TEST_FUNCTION(JavaModuleHost_Receive_GetMethodID_failure)
{
    //Arrange
    const unsigned char msg[] =
    {
        0xA1, 0x60,             /*header*/
        0x00, 0x00, 0x00, 14,   /*size of this array*/
        0x00, 0x00, 0x00, 0x00, /*zero properties*/
        0x00, 0x00, 0x00, 0x00  /*zero message content size*/
    };

    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    MESSAGE_HANDLE message = Message_CreateFromByteArray(msg, sizeof(msg));
    umock_c_reset_all_calls();

    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(Message_ToByteArray(message, NULL, 0));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Message_ToByteArray(message, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(AttachCurrentThread(global_vm, IGNORED_PTR_ARG, NULL))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(NewByteArray(global_env, IGNORED_NUM_ARG))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(SetByteArrayRegion(global_env, IGNORED_PTR_ARG, 0, IGNORED_NUM_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(4)
        .IgnoreArgument(5);;
    STRICT_EXPECTED_CALL(ExceptionOccurred(global_env));
    STRICT_EXPECTED_CALL(GetObjectClass(global_env, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(GetMethodID(global_env, IGNORED_PTR_ARG, MODULE_RECEIVE_METHOD_NAME, MODULE_RECEIVE_DESCRIPTOR))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(ExceptionOccurred(global_env));
    STRICT_EXPECTED_CALL(ExceptionDescribe(global_env));
    STRICT_EXPECTED_CALL(ExceptionClear(global_env));
    STRICT_EXPECTED_CALL(DeleteLocalRef(global_env, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(DetachCurrentThread(global_vm));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    umock_c_negative_tests_snapshot();

    //Act
    umock_c_negative_tests_fail_call(8);
    JavaModuleHost_Receive(module, message);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    Message_Destroy(message);
    JavaModuleHost_Destroy(module);

    umock_c_negative_tests_deinit();
}

/*Tests_SRS_JAVA_MODULE_HOST_14_047: [This function shall exit if any underlying function fails.]*/
TEST_FUNCTION(JavaModuleHost_Receive_CallVoidMethod_failure)
{
    //Arrange
    const unsigned char msg[] =
    {
        0xA1, 0x60,             /*header*/
        0x00, 0x00, 0x00, 14,   /*size of this array*/
        0x00, 0x00, 0x00, 0x00, /*zero properties*/
        0x00, 0x00, 0x00, 0x00  /*zero message content size*/
    };

    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    MESSAGE_HANDLE message = Message_CreateFromByteArray(msg, sizeof(msg));
    umock_c_reset_all_calls();

    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(Message_ToByteArray(message, NULL, 0));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Message_ToByteArray(message, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(AttachCurrentThread(global_vm, IGNORED_PTR_ARG, NULL))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(NewByteArray(global_env, IGNORED_NUM_ARG))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(SetByteArrayRegion(global_env, IGNORED_PTR_ARG, 0, IGNORED_NUM_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(4)
        .IgnoreArgument(5);;
    STRICT_EXPECTED_CALL(ExceptionOccurred(global_env));
    STRICT_EXPECTED_CALL(GetObjectClass(global_env, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(GetMethodID(global_env, IGNORED_PTR_ARG, MODULE_RECEIVE_METHOD_NAME, MODULE_RECEIVE_DESCRIPTOR))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(ExceptionOccurred(global_env));
    STRICT_EXPECTED_CALL(CallVoidMethodV(global_env, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .IgnoreArgument(4);
    STRICT_EXPECTED_CALL(ExceptionOccurred(global_env));
    STRICT_EXPECTED_CALL(ExceptionDescribe(global_env));
    STRICT_EXPECTED_CALL(ExceptionClear(global_env));
    STRICT_EXPECTED_CALL(DeleteLocalRef(global_env, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(DetachCurrentThread(global_vm));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    umock_c_negative_tests_snapshot();

    //Act
    umock_c_negative_tests_fail_call(11);
    JavaModuleHost_Receive(module, message);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    Message_Destroy(message);
    JavaModuleHost_Destroy(module);

    umock_c_negative_tests_deinit();
}

//=============================================================================
//JavaModuleHost_Destroy tests
//=============================================================================

/*Tests_SRS_JAVA_MODULE_HOST_14_039: [This function shall attach the JVM to the current thread.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_038: [This function shall find get the user-defined Java module class using the module parameter and get the destroy().]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_020: [This function shall call the void destroy() method of the Java module object and delete the global reference to this object.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_021: [This function shall free all resources associated with this module.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_029: [This function shall destroy the JVM if it the last module to be disconnected from the gateway.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_040: [This function shall detach the JVM from the current thread.]*/
TEST_FUNCTION(JavaModuleHost_Destroy_success)
{
    //Arrange

    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(AttachCurrentThread(global_vm , IGNORED_PTR_ARG, NULL))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(GetObjectClass(global_env, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(GetMethodID(global_env, IGNORED_PTR_ARG, MODULE_DESTROY_METHOD_NAME, MODULE_DESTROY_DESCRIPTOR))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(ExceptionOccurred(global_env));
    STRICT_EXPECTED_CALL(CallVoidMethodV(global_env, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .IgnoreArgument(4);
    STRICT_EXPECTED_CALL(ExceptionOccurred(global_env));
    STRICT_EXPECTED_CALL(DeleteGlobalRef(global_env, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(DetachCurrentThread(global_vm));

    STRICT_EXPECTED_CALL(JavaModuleHostManager_Remove(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(JavaModuleHostManager_Size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(JavaModuleHostManager_Size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(DestroyJavaVM(global_vm));
    STRICT_EXPECTED_CALL(JavaModuleHostManager_Destroy(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //Act
    JavaModuleHost_Destroy(module);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
}

TEST_FUNCTION(JavaModuleHost_Destroy_multiple_success)
{
    //Arrange

    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    MODULE_HANDLE module2 = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(AttachCurrentThread(global_vm, IGNORED_PTR_ARG, NULL))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(GetObjectClass(global_env, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(GetMethodID(global_env, IGNORED_PTR_ARG, MODULE_DESTROY_METHOD_NAME, MODULE_DESTROY_DESCRIPTOR))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(ExceptionOccurred(global_env));
    STRICT_EXPECTED_CALL(CallVoidMethodV(global_env, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .IgnoreArgument(4);
    STRICT_EXPECTED_CALL(ExceptionOccurred(global_env));
    STRICT_EXPECTED_CALL(DeleteGlobalRef(global_env, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(DetachCurrentThread(global_vm));

    STRICT_EXPECTED_CALL(JavaModuleHostManager_Remove(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(JavaModuleHostManager_Size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(JavaModuleHostManager_Size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(JavaModuleHostManager_Destroy(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(AttachCurrentThread(global_vm, IGNORED_PTR_ARG, NULL))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(GetObjectClass(global_env, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(GetMethodID(global_env, IGNORED_PTR_ARG, MODULE_DESTROY_METHOD_NAME, MODULE_DESTROY_DESCRIPTOR))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(ExceptionOccurred(global_env));
    STRICT_EXPECTED_CALL(CallVoidMethodV(global_env, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .IgnoreArgument(4);
    STRICT_EXPECTED_CALL(ExceptionOccurred(global_env));
    STRICT_EXPECTED_CALL(DeleteGlobalRef(global_env, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(DetachCurrentThread(global_vm));

    STRICT_EXPECTED_CALL(JavaModuleHostManager_Remove(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(JavaModuleHostManager_Size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(JavaModuleHostManager_Size(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(DestroyJavaVM(global_vm));
    STRICT_EXPECTED_CALL(JavaModuleHostManager_Destroy(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //Act
    JavaModuleHost_Destroy(module);
    JavaModuleHost_Destroy(module2);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
}

/*Tests_SRS_JAVA_MODULE_HOST_14_019: [This function shall do nothing if module is NULL.]*/
TEST_FUNCTION(JavaModuleHost_Destroy_module_null_failure)
{
    //Act
    JavaModuleHost_Destroy(NULL);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JAVA_MODULE_HOST_14_041: [This function shall exit if any JNI function fails.]*/
TEST_FUNCTION(JavaModuleHost_Destroy_attach_fails)
{
    //Arrange

    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    umock_c_reset_all_calls();
    
    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(AttachCurrentThread(global_vm, IGNORED_PTR_ARG, NULL))
        .IgnoreArgument(2);

    umock_c_negative_tests_snapshot();

    //Act
    umock_c_negative_tests_fail_call(0);
    JavaModuleHost_Destroy(module);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    umock_c_negative_tests_deinit();

    //Cleanup
    JavaModuleHost_Destroy(module);
}

/*Tests_SRS_JAVA_MODULE_HOST_14_041: [This function shall exit if any JNI function fails.]*/
TEST_FUNCTION(JavaModuleHost_Destroy_GetObjectClass_fails)
{
    //Arrange

    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    umock_c_reset_all_calls();

    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(AttachCurrentThread(global_vm, IGNORED_PTR_ARG, NULL))
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(GetObjectClass(global_env, IGNORED_PTR_ARG))
        .IgnoreArgument(2);

    umock_c_negative_tests_snapshot();

    //Act
    umock_c_negative_tests_fail_call(1);
    JavaModuleHost_Destroy(module);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    umock_c_negative_tests_deinit();

    //Cleanup
    JavaModuleHost_Destroy(module);
}

/*Tests_SRS_JAVA_MODULE_HOST_14_041: [This function shall exit if any JNI function fails.]*/
TEST_FUNCTION(JavaModuleHost_Destroy_GetMethodID_fails)
{
    //Arrange

    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    umock_c_reset_all_calls();

    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(AttachCurrentThread(global_vm, IGNORED_PTR_ARG, NULL))
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(GetObjectClass(global_env, IGNORED_PTR_ARG))
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(GetMethodID(global_env, IGNORED_PTR_ARG, MODULE_DESTROY_METHOD_NAME, MODULE_DESTROY_DESCRIPTOR))
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(ExceptionOccurred(global_env));

    STRICT_EXPECTED_CALL(ExceptionDescribe(global_env));

    STRICT_EXPECTED_CALL(ExceptionClear(global_env));

    umock_c_negative_tests_snapshot();

    //Act
    umock_c_negative_tests_fail_call(2);
    JavaModuleHost_Destroy(module);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    umock_c_negative_tests_deinit();

    //Cleanup
    JavaModuleHost_Destroy(module);
}

/*Tests_SRS_JAVA_MODULE_HOST_14_040: [This function shall detach the JVM from the current thread.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_021: [This function shall free all resources associated with this module.]*/
TEST_FUNCTION(JavaModuleHost_Destroy_CallVoidMethod_fails)
{
    //Arrange

    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    umock_c_reset_all_calls();

    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(AttachCurrentThread(global_vm, IGNORED_PTR_ARG, NULL))
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(GetObjectClass(global_env, IGNORED_PTR_ARG))
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(GetMethodID(global_env, IGNORED_PTR_ARG, MODULE_DESTROY_METHOD_NAME, MODULE_DESTROY_DESCRIPTOR))
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(ExceptionOccurred(global_env));

    STRICT_EXPECTED_CALL(CallVoidMethodV(global_env, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .IgnoreArgument(4);

    STRICT_EXPECTED_CALL(ExceptionOccurred(global_env));

    STRICT_EXPECTED_CALL(ExceptionDescribe(global_env));

    STRICT_EXPECTED_CALL(ExceptionClear(global_env));

    STRICT_EXPECTED_CALL(DeleteGlobalRef(global_env, IGNORED_PTR_ARG))
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(DetachCurrentThread(global_vm));

    STRICT_EXPECTED_CALL(JavaModuleHostManager_Remove(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(JavaModuleHostManager_Size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(JavaModuleHostManager_Size(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(DestroyJavaVM(global_vm));

    STRICT_EXPECTED_CALL(JavaModuleHostManager_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    umock_c_negative_tests_snapshot();

    //Act
    umock_c_negative_tests_fail_call(5);
    JavaModuleHost_Destroy(module);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    umock_c_negative_tests_deinit();
}

//=============================================================================
//JavaModuleHost_Start tests
//=============================================================================

/*Tests_SRS_JAVA_MODULE_HOST_14_050: [This function shall attach the JVM to the current thread.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_051: [This function shall get the user - defined Java module class using the module parameter and get the start() method.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_052: [This function shall call the void start() method of the Java module object.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_053: [This function shall detach the JVM from the current thread.]*/
TEST_FUNCTION(JavaModuleHost_Start_success)
{
    //Arrange

    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(AttachCurrentThread(global_vm, IGNORED_PTR_ARG, NULL))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(GetObjectClass(global_env, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(GetMethodID(global_env, IGNORED_PTR_ARG, MODULE_START_METHOD_NAME, MODULE_START_DESCRIPTOR))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(ExceptionOccurred(global_env));
    STRICT_EXPECTED_CALL(CallVoidMethodV(global_env, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .IgnoreArgument(4);
    STRICT_EXPECTED_CALL(ExceptionOccurred(global_env));
    STRICT_EXPECTED_CALL(DetachCurrentThread(global_vm));

    //Act
    JavaModuleHost_Start(module);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    JavaModuleHost_Destroy(module);
}

/*Tests_SRS_JAVA_MODULE_HOST_14_049: [This function shall do nothing if module is NULL.]*/
TEST_FUNCTION(JavaModuleHost_Start_module_null_failure)
{
    //Arrange
    umock_c_reset_all_calls();

    //Act
    JavaModuleHost_Start(NULL);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JAVA_MODULE_HOST_14_054: [This function shall exit if any JNI function fails.]*/
TEST_FUNCTION(JavaModuleHost_Start_attach_fails)
{
    //Arrange

    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    umock_c_reset_all_calls();

    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(AttachCurrentThread(global_vm, IGNORED_PTR_ARG, NULL))
        .IgnoreArgument(2);

    umock_c_negative_tests_snapshot();

    //Act
    umock_c_negative_tests_fail_call(0);
    JavaModuleHost_Start(module);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    umock_c_negative_tests_deinit();

    //Cleanup
    JavaModuleHost_Destroy(module);
}

/*Tests_SRS_JAVA_MODULE_HOST_14_054: [This function shall exit if any JNI function fails.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_053: [This function shall detach the JVM from the current thread.]*/
TEST_FUNCTION(JavaModuleHost_Start_GetObjectClass_fails)
{
    //Arrange

    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    umock_c_reset_all_calls();

    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(AttachCurrentThread(global_vm, IGNORED_PTR_ARG, NULL))
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(GetObjectClass(global_env, IGNORED_PTR_ARG))
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(DetachCurrentThread(global_vm));

    umock_c_negative_tests_snapshot();

    //Act
    umock_c_negative_tests_fail_call(1);
    JavaModuleHost_Start(module);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    umock_c_negative_tests_deinit();

    //Cleanup
    JavaModuleHost_Destroy(module);
}

/*Tests_SRS_JAVA_MODULE_HOST_14_054: [This function shall exit if any JNI function fails.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_053: [This function shall detach the JVM from the current thread.]*/
TEST_FUNCTION(JavaModuleHost_Start_GetMethodID_fails)
{
    //Arrange

    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    umock_c_reset_all_calls();

    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(AttachCurrentThread(global_vm, IGNORED_PTR_ARG, NULL))
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(GetObjectClass(global_env, IGNORED_PTR_ARG))
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(GetMethodID(global_env, IGNORED_PTR_ARG, MODULE_START_METHOD_NAME, MODULE_START_DESCRIPTOR))
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(ExceptionOccurred(global_env));

    STRICT_EXPECTED_CALL(ExceptionDescribe(global_env));

    STRICT_EXPECTED_CALL(ExceptionClear(global_env));

    STRICT_EXPECTED_CALL(DetachCurrentThread(global_vm));

    umock_c_negative_tests_snapshot();

    //Act
    umock_c_negative_tests_fail_call(2);
    JavaModuleHost_Start(module);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    umock_c_negative_tests_deinit();

    //Cleanup
    JavaModuleHost_Destroy(module);
}

/*Tests_SRS_JAVA_MODULE_HOST_14_054: [This function shall exit if any JNI function fails.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_053: [This function shall detach the JVM from the current thread.]*/
TEST_FUNCTION(JavaModuleHost_Start_CallVoidMethod_fails)
{
    //Arrange

    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    umock_c_reset_all_calls();

    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(AttachCurrentThread(global_vm, IGNORED_PTR_ARG, NULL))
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(GetObjectClass(global_env, IGNORED_PTR_ARG))
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(GetMethodID(global_env, IGNORED_PTR_ARG, MODULE_START_METHOD_NAME, MODULE_START_DESCRIPTOR))
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(ExceptionOccurred(global_env));

    STRICT_EXPECTED_CALL(CallVoidMethodV(global_env, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .IgnoreArgument(4);

    STRICT_EXPECTED_CALL(ExceptionOccurred(global_env));

    STRICT_EXPECTED_CALL(ExceptionDescribe(global_env));

    STRICT_EXPECTED_CALL(ExceptionClear(global_env));

    STRICT_EXPECTED_CALL(DetachCurrentThread(global_vm));

    umock_c_negative_tests_snapshot();

    //Act
    umock_c_negative_tests_fail_call(5);
    JavaModuleHost_Start(module);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    umock_c_negative_tests_deinit();

    //Cleanup
    JavaModuleHost_Destroy(module);
}

//=============================================================================
//JavaModuleHost_ParseConfigurationFromJson tests
//=============================================================================

/*Tests_SRS_JAVA_MODULE_HOST_14_028: [This function shall allocate memory for the configuration parameter and copy it.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_056: [This function shall return a non - NULL const char* configuration string when successful.]*/
TEST_FUNCTION(JavaModuleHost_ParseConfigurationFromJson_success)
{
    //Arrange

    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, config.configuration_json))
        .IgnoreArgument(1);

    //Act
    char* result = (char*)JavaModuleHost_ParseConfigurationFromJson(config.configuration_json);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    //Cleanup
    JavaModuleHost_Destroy(module);
    JavaModuleHost_FreeConfiguration(result);
}

/*Tests_SRS_JAVA_MODULE_HOST_14_055: [This function shall return NULL if any underlying platform call fails.]*/
TEST_FUNCTION(JavaModuleHost_ParseConfigurationFromJson_failure)
{
    //Arrange
    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    umock_c_reset_all_calls();

    int value = 0;
    value = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, value);

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, config.configuration_json))
        .IgnoreArgument(1)
        .SetFailReturn(-1);

    umock_c_negative_tests_snapshot();

    //Act
    umock_c_negative_tests_reset();
    umock_c_negative_tests_fail_call(0);

    char* result = (char*)JavaModuleHost_ParseConfigurationFromJson(config.configuration_json);

    umock_c_negative_tests_deinit();

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(result);

    //Cleanup
    JavaModuleHost_Destroy(module);
    
}

//=============================================================================
//JavaModuleHost_FreeConfiguration tests
//=============================================================================

/*Tests_SRS_JAVA_MODULE_HOST_14_058: [ This function shall free the configuration. ]*/
TEST_FUNCTION(JavaModuleHost_FreeConfiguration_success)
{
    //Arrange
    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    char* configuration_json = (char*)JavaModuleHost_ParseConfigurationFromJson(config.configuration_json);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(configuration_json));

    //Act
    JavaModuleHost_FreeConfiguration(configuration_json);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    JavaModuleHost_Destroy(module);
}

/*Tests_SRS_JAVA_MODULE_HOST_14_057: [ This function shall do nothing if configuration is NULL. ]*/
TEST_FUNCTION(JavaModuleHost_FreeConfiguration_failure)
{
    //Arrange
    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    char* configuration_json = NULL;
    umock_c_reset_all_calls();

    //Act
    JavaModuleHost_FreeConfiguration(configuration_json);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    JavaModuleHost_Destroy(module);
}

//=============================================================================
//Java_com_microsoft_azure_gateway_core_Broker_publishMessage tests
//=============================================================================

/*Tests_SRS_JAVA_MODULE_HOST_14_025: [This function shall convert the jbyteArray message into an unsigned char array.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_026: [This function shall use the serialized message in a call to Message_Create.]*/
/*Tests_SRS_JAVA_MODULE_HOST_14_027: [This function shall publish the message to the BROKER_HANDLE addressed by addr and return the value of this function call.]*/
TEST_FUNCTION(Java_com_microsoft_azure_gateway_core_Broker_publishMessage_success)
{
    //Arrange
    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    umock_c_reset_all_calls();

    jbyteArray serialized_message = (jbyteArray)0x42;
    jobject jBroker = (jobject)0x42;
    jlong broker_address = (jlong)0x42;
    BROKER_HANDLE broker = (BROKER_HANDLE)broker_address;

    STRICT_EXPECTED_CALL(GetArrayLength(global_env, serialized_message));

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(GetByteArrayRegion(global_env, serialized_message, 0, IGNORED_NUM_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(4)
        .IgnoreArgument(5);

    STRICT_EXPECTED_CALL(ExceptionOccurred(global_env));

    STRICT_EXPECTED_CALL(Message_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(Broker_Publish(broker, module, IGNORED_PTR_ARG))
        .IgnoreArgument(3);

    STRICT_EXPECTED_CALL(Message_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //Act

    jint result = Java_com_microsoft_azure_gateway_core_Broker_publishMessage(global_env, jBroker, broker_address, (jlong)module, serialized_message);

    //Assert
    ASSERT_ARE_EQUAL(int32_t, JNI_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //Cleanup
    JavaModuleHost_Destroy(module);
}

/*Tests_SRS_JAVA_MODULE_HOST_14_048: [ This function shall return a non-zero value if any underlying function call fails. ]*/
TEST_FUNCTION(Java_com_microsoft_azure_gateway_core_Broker_publishMessage_failure)
{
    //Arrange
    MODULE_HANDLE module = JavaModuleHost_Create((BROKER_HANDLE)0x42, &config);
    umock_c_reset_all_calls();

    jbyteArray serialized_message = (jbyteArray)0x42;
    jobject jBroker = (jobject)0x42;
    jlong broker_address = (jlong)0x42;
    BROKER_HANDLE broker = (BROKER_HANDLE)broker_address;

    int init_result = 0;
    init_result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, init_result);

    STRICT_EXPECTED_CALL(GetArrayLength(global_env, serialized_message))
        .SetFailReturn(0);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);

    STRICT_EXPECTED_CALL(GetByteArrayRegion(global_env, serialized_message, 0, IGNORED_NUM_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(4)
        .IgnoreArgument(5);

    STRICT_EXPECTED_CALL(ExceptionOccurred(global_env));

    STRICT_EXPECTED_CALL(Message_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreAllArguments()
        .SetFailReturn(NULL);

    STRICT_EXPECTED_CALL(Broker_Publish(broker, module, IGNORED_PTR_ARG))
        .IgnoreArgument(3)
        .SetFailReturn(BROKER_ERROR);

    STRICT_EXPECTED_CALL(Message_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    umock_c_negative_tests_snapshot();

    //act
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (i != 2 &&
            i != 6 &&
            i != 7)
        {
            // arrange
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            jint result = Java_com_microsoft_azure_gateway_core_Broker_publishMessage(global_env, jBroker, broker_address, (jlong)module, serialized_message);

            //Assert
            ASSERT_ARE_NOT_EQUAL(int32_t, JNI_OK, result);
        }
    }
    umock_c_negative_tests_deinit();

    //Act

    //Cleanup
    JavaModuleHost_Destroy(module);
}

/*Tests_SRS_JAVA_MODULE_HOST_26_001: [ `Module_GetApi` shall fill out the provided `MODULES_API` structure with required module's APIs functions. ] */
TEST_FUNCTION(Module_GetApi_returns_non_NULL)
{
    //Arrange

    //Act

    const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

    //Assert
    ASSERT_IS_NOT_NULL(MODULE_PARSE_CONFIGURATION_FROM_JSON(apis));
    ASSERT_IS_NOT_NULL(MODULE_FREE_CONFIGURATION(apis));
    ASSERT_IS_NOT_NULL(MODULE_CREATE(apis));
    ASSERT_IS_NOT_NULL(MODULE_DESTROY(apis));
    ASSERT_IS_NOT_NULL(MODULE_RECEIVE(apis));
    ASSERT_IS_NOT_NULL(MODULE_START(apis));
}

END_TEST_SUITE(JavaModuleHost_UnitTests);
