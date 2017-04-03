#include "java_nanomsg.h"

#include <stdio.h>
#include <nn.h>
#include <jni.h>

JNIEXPORT jint JNICALL Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1errno(JNIEnv *env, jobject obj) {
    (void)env;
    (void)obj;

    return nn_errno();
}

JNIEXPORT jstring JNICALL Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1strerror
(JNIEnv *env, jobject obj, jint errnum) {
    (void)obj;
    const char *error = nn_strerror(errnum);
    if (error == 0)
        error = "";

    jstring error_msg = (*env)->NewStringUTF(env, error);
    jthrowable exception = (*env)->ExceptionOccurred(env);
    if (error_msg == NULL || exception)
    {
        error = "";
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
    }

    return error_msg;
}

JNIEXPORT jint JNICALL Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1socket
(JNIEnv *env, jobject obj, jint domain, jint protocol) {
    (void)env;
    (void)obj;

    return nn_socket(domain, protocol);
}

JNIEXPORT jint JNICALL Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1close
(JNIEnv *env, jobject obj, jint socket) {
    (void)env;
    (void)obj;

    return nn_close(socket);
}

JNIEXPORT jint JNICALL Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1bind
(JNIEnv *env, jobject obj, jint socket, jstring address) {
    (void)obj;
    const char *addr = (*env)->GetStringUTFChars(env, address, NULL);

    return nn_bind(socket, addr);
}

JNIEXPORT jint JNICALL Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1shutdown
(JNIEnv *env, jobject obj, jint socket, jint endpoint) {
    (void)env;
    (void)obj;

    return nn_shutdown(socket, endpoint);
}

JNIEXPORT jint JNICALL Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1send
(JNIEnv *env, jobject obj, jint socket, jbyteArray buffer, jint flags) {
    (void)obj;

    jint result = -1;
    if (buffer != NULL) {
        jsize length = (*env)->GetArrayLength(env, buffer);
        jbyte* cbuffer = (*env)->GetByteArrayElements(env, buffer, 0);

        if (cbuffer == NULL)
        {
            result = -1;
        }
        else {
            result = nn_send(socket, cbuffer, length, flags);
            (*env)->ReleaseByteArrayElements(env, buffer, cbuffer, JNI_ABORT);
        }
    }

    return result;
}


JNIEXPORT jbyteArray JNICALL Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_nn_1recv
(JNIEnv *env, jobject obj, jint socket, jint flags)
{
    (void)obj;
    void *buf = NULL;
    int nbytes = nn_recv(socket, &buf, NN_MSG, flags);
    jbyteArray result = 0;
    if (nbytes <= 0) {
        result = 0;
    }
    else {
        result = (*env)->NewByteArray(env, nbytes);

        if (result == NULL) {
            result = 0;
        }
        else {
            (*env)->SetByteArrayRegion(env, result, 0, nbytes, (const jbyte*)buf);
            jthrowable exception = (*env)->ExceptionOccurred(env);
            if (exception) {
                result = 0;
                (*env)->ExceptionDescribe(env);
                (*env)->ExceptionClear(env);
            }
        }

        nn_freemsg(buf);
    }

    return result;
}

JNIEXPORT jobject JNICALL Java_com_microsoft_azure_gateway_remote_NanomsgLibrary_getSymbols
(JNIEnv *env, jclass clazz) {
    (void)clazz;
    jobject result = NULL;
    jclass jMap_class = (*env)->FindClass(env, "java/util/HashMap");

    jthrowable exception = (*env) -> ExceptionOccurred(env);
    if (jMap_class == NULL || exception)
    {
        result = NULL;
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
    }
    else {
        jmethodID jMap_init = (*env)->GetMethodID(env, jMap_class, "<init>", "()V");
        exception = (*env)->ExceptionOccurred(env);
        if (jMap_init == NULL || exception)
        {
            result = NULL;
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
        }
        else {
            jobject jmap_object = (*env)->NewObject(env, jMap_class, jMap_init, NULL);
            exception = (*env)->ExceptionOccurred(env);
            if (jmap_object == NULL || exception)
            {
                result = NULL;
                (*env)->ExceptionDescribe(env);
                (*env)->ExceptionClear(env);
            }
            else {
                result = jmap_object;
                jmethodID jmap_put = (*env)->GetMethodID(env, jMap_class, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
                exception = (*env)->ExceptionOccurred(env);
                if (jmap_put == NULL || exception)
                {
                    result = NULL;
                    (*env)->ExceptionDescribe(env);
                    (*env)->ExceptionClear(env);
                }
                else {
                    jclass jinteger_class = (*env)->FindClass(env, "java/lang/Integer");
                    exception = (*env)->ExceptionOccurred(env);
                    if (jinteger_class == NULL || exception)
                    {
                        result = NULL;
                        (*env)->ExceptionDescribe(env);
                        (*env)->ExceptionClear(env);
                    }
                    else {
                        jmethodID jinteger_init = (*env)->GetMethodID(env, jinteger_class, "<init>", "(I)V");
                        exception = (*env)->ExceptionOccurred(env);
                        if (jinteger_init == NULL || exception)
                        {
                            result = NULL;
                            (*env)->ExceptionDescribe(env);
                            (*env)->ExceptionClear(env);
                        }
                        else {
                            int value, i;
                            for (i = 0; ; ++i) {
                                const char* name = nn_symbol(i, &value);
                                if (name == NULL) break;

                                jstring jkey = (*env)->NewStringUTF(env, name);
                                exception = (*env)->ExceptionOccurred(env);
                                if (jkey == NULL || exception)
                                {
                                    (*env)->ExceptionDescribe(env);
                                    (*env)->ExceptionClear(env);
                                }
                                else {
                                    jobject jval = (*env)->NewObject(env, jinteger_class, jinteger_init, value);
                                    exception = (*env)->ExceptionOccurred(env);
                                    if (jval == NULL || exception)
                                    {
                                        (*env)->ExceptionDescribe(env);
                                        (*env)->ExceptionClear(env);
                                    }
                                    else {
                                        (*env)->CallObjectMethod(env, jmap_object, jmap_put, jkey, jval);
                                        exception = (*env)->ExceptionOccurred(env);
                                        if (exception)
                                        {
                                            (*env)->ExceptionDescribe(env);
                                            (*env)->ExceptionClear(env);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return result;
}
