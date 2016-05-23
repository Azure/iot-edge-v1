# Java Module Host Requirements

##Overview

This module enables interop between the native gateway and modules written in
Java. This module will broker communication to and from the native gateway as
well as manage the lifetime of the Java Virtual Machine. Please read the [high
level design](./java_binding_hld.md) document for more design details. This 
module will be built as a dynamic library and loaded by the Java class using
`System.loadLibrary(...)` in order to publish messages to the bus. The path 
to the .dll or .so must be included in `library_path` of the configuration for 
the java module host, or be part of the default library path.

##References

[module.h](../../../../core/devdoc/module.md)

[JNI](http://docs.oracle.com/javase/8/docs/technotes/guides/jni/)

##Relevant Structures
```C
typedef struct JVM_OPTIONS_TAG
{
    const char* class_path;
    const char* library_path;
    int version;
    bool debug;
    int debug_port;
    bool verbose;
    VECTOR_HANDLE additional_options;
} JVM_OPTIONS;

typedef struct JAVA_MODULE_HOST_CONFIG_TAG
{
    const char* class_name;
    const char* configuration_json;
    JVM_OPTIONS* options;
} JAVA_MODULE_HOST_CONFIG;

typedef struct JAVA_MODULE_HANDLE_DATA_TAG
{
    JavaVM *jvm;
    JNIEnv *env;
    jobject module;
    char* moduleName;
}JAVA_MODULE_HANDLE_DATA;
```

##Module_GetAPIs
This is the primary public interface for the module. It returns a pointer to the
`MODULE_APIS` structure containing the implementation functions for this module.
The following functions are the implementation of those APIs.

**SRS_JAVA_MODULE_HOST_14_029: [** This function shall return a
non-`NULL` pointer to a structure of type `MODULE_APIS` that has all
fields non-`NULL`. **]**

##JavaModuleHost_Create
```C
static MODULE_HANDLE JavaModuleHost_Create(MESSAGE_BUS_HANDLE bus, const void* configuration);
```

Creates a new Java Module Host instance. The parameter `configuration` is a
pointer to a `JAVA_MODULE_HOST_CONFIG` structure.

**SRS_JAVA_MODULE_HOST_14_001: [** This function shall return
`NULL` if `bus` is `NULL`. **]**

**SRS_JAVA_MODULE_HOST_14_002: [** This function shall return
`NULL` if `configuration` is `NULL`. **]**

**SRS_JAVA_MODULE_HOST_14_003: [** This function shall return
`NULL` if `configuration->configuration_json` is not valid JSON.
**]**

**SRS_JAVA_MODULE_HOST_14_004: [** This function shall return
`NULL` if `class_name` is `NULL`. **]**

**SRS_JAVA_MODULE_HOST_14_005: [** This function shall return
`NULL` upon any underlying API call failure. **]**

**SRS_JAVA_MODULE_HOST_14_006: [** This function shall return a
non-**`NULL MODULE_HANDLE` **when successful. **]**

**SRS_JAVA_MODULE_HOST_14_007: [** This function shall allocate memory
for an instance of a `JAVA_MODULE_HANDLE` structure to be used as
the backing structure for this module. **]**

**SRS_JAVA_MODULE_HOST_14_008: [** This function shall initize a**
`JavaVMInitArgs` **structure using the `JVM_OPTIONS` **structure**
`configuration->options`. **]**

**SRS_JAVA_MODULE_HOST_14_009: [** If `configuration->options`
is `NULL`**,** `JavaVMInitArgs` shall be initialized using default
values. **]**

**SRS_JAVA_MODULE_HOST_14_010: [** This function shall allocate memory
for an array of `JavaVMOption` structures and initialize each with
each option provided. **]**

**SRS_JAVA_MODULE_HOST_14_011: [** If this is the first Java module to
load, this function shall create the JVM using the `JavaVMInitArgs`
through a call to `JNI_CreateJavaVM` and save the JavaVM and JNIEnv
pointers in the `JAVA_MODULE_HANDLE`. **]**

**SRS_JAVA_MODULE_HOST_14_012: [** If the JVM was previously created, the
function shall get a pointer to that `JavaVM` pointer and `JNIEnv`
environment pointer. **]**

**SRS_JAVA_MODULE_HOST_14_013: [** This function shall increment the
`JAVA_MODULE_COUNT` global variable. **]**

**SRS_JAVA_MODULE_HOST_14_014: [** This function shall return
`NULL` if a JVM could not be created or found. **]**

**SRS_JAVA_MODULE_HOST_14_015: [** This function shall find the
`MessageBus` Java class, get the constructor, and create a `MessageBus`
Java object. **]**

**SRS_JAVA_MODULE_HOST_14_016: [** This function shall find the
user-defined Java module class using `configuration->class_name`,
get the constructor, and create an instance of this module object. **]**

**SRS_JAVA_MODULE_HOST_14_017: [** This function shall return
`NULL` if any returned `jclass`, `jmethodID`, or `jobject` is
`NULL`. **]**

**SRS_JAVA_MODULE_HOST_14_018: [** This function shall return
`NULL` if any JNI function fails. **]**

**SRS_JAVA_MODULE_HOST_14_019: [** The function shall save a new global
reference to the Java module object in
`JAVA_MODULE_HANDLE_DATA->module`. **]**

## JavaModuleHost_Destroy
```C
static void JavaModuleHost_Destroy(MODULE_HANDLE module);
```

**SRS_JAVA_MODULE_HOST_14_020: [** This function shall do nothing if
`module` is `NULL`. **]**

**SRS_JAVA_MODULE_HOST_14_021: [** This function shall call the
`void destroy()` method of the Java module object and delete the global
reference to this object. **]**

**SRS_JAVA_MODULE_HOST_14_022: [** This function shall free all resources
associated with this module. **]**

**SRS_JAVA_MODULE_HOST_14_030: [** This function shall destroy the JVM if 
it the last module to be disconnected from the gateway. **]**

##JavaModuleHost_Receive
```C
static void JavaModuleHost_Receive(MODULE_HANDLE module, MESSAGE_HANDLE message);
```

**SRS_JAVA_MODULE_HOST_14_023: [** This function shall do nothing if
`module` or `message` is `NULL`. **]**

**SRS_JAVA_MODULE_HOST_14_024: [** This function shall serialize
`message`. **]**

**SRS_JAVA_MODULE_HOST_14_025: [** This function shall call the
`void receive(byte[] source)` method of the Java module object passing the
serialized `message`. **]**

##MessageBus_Publish
```C
JNIEXPORT jint JNICALL Java_com_microsoft_azure_gateway_core_MessageBus_publishMessage(JNIEnv *env, jobject MessageBus, jlong addr, jbyteArray message);
```

**SRS_JAVA_MODULE_HOST_14_026: [** This function shall use convert the
`jbyteArray message` into an `unsigned char` array. **]**

**SRS_JAVA_MODULE_HOST_14_027: [** This function shall use the serialized
message in a call to `Message_Create`. **]**

**SRS_JAVA_MODULE_HOST_14_028: [** This function shall publish the
message to the `MESSAGE_BUS_HANDLE` addressed by `addr` and
return the value of this function call. **]**
