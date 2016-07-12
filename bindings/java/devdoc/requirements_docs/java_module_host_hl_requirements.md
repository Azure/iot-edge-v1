# Java Module Host HL Requirements

##Overview

##References

[Java Module Host](./java_module_host_requirements.md)

[JSON](http://www.json.org)

[JNI](http://docs.oracle.com/javase/8/docs/technotes/guides/jni/)

## Module_GetAPIs
```C
extern const MODULE_APIS* Module_GetAPIS(void);
```
This is the primary public interface for the module. It returns a pointer to
the `MODULE_APIS` structure containing the implementation functions for this module.
The following functions are the implementation of those APIs.

**SRS_JAVA_MODULE_HOST_HL_14_001: [** This function shall return a non-`NULL` pointer to a structure of type `MODULE_APIS` that has all fields non-`NULL`. **]**

##JavaModuleHost_HL_Create
```C
static MODULE_HANDLE JavaModuleHost_HL_Create(MESSAGE_BUS_HANDLE bus, const void* configuration);
```

Creates a new Java Module Host instance. The parameter `configuration` is a pointer to a `const char*` that contains a JSON object supplied by `Gateway_Create_From_JSON`.
This JSON object will be the `"args"` section of the Gateway JSON configuration file and should be formatted as follows:
```json
{
    "modules": [
        {
            "module name": "java_poller",
            "module path": "/path/to/java_module_host_hl.so|.dll",
            "args": {
                "class_path": "/path/to/relevant/class/files",
                "library_path": "/path/to/dir/with/java_module_host.so|.dll",
                "class_name": "Poller",
                "args": {
                    "frequency": 30
                },
                "jvm_options": {
                    "version": 8,
                    "debug": true,
                    "debug_port": 9876,
                    "verbose": false,
                    "additional_options": [
                        "-Djava.version=1.8"
                    ]
                }
            }
        }
    ]
}
```

###"class_path" (optional)
Path to the relevant Java class files.

###"library_path" (optional)
Path to the Java module host native library.

###"class_name" (required)
The name of the main module class.

###"args" (optional)
The user-defined arguments for the Java module (Java).

###"jvm_options" (optional)
The JVM options to be used when creating the JVM. Specify the version, debug options, verbosity, and any other additional options. The defaults are:
* version: 4
* debug: false
* debug_port: 9876
* verbose: false
* additional_options: NONE

**SRS_JAVA_MODULE_HOST_HL_14_002: [** This function shall return `NULL` if `bus` is `NULL` or `configuration` is `NULL`. **]**

**SRS_JAVA_MODULE_HOST_HL_14_003: [** This function shall return `NULL` if `configuration` is not a valid JSON object. **]**

**SRS_JAVA_MODULE_HOST_HL_14_004: [** This function shall return `NULL` if `configuration.args` does not contain a field named `class_name`. **]**

**SRS_JAVA_MODULE_HOST_HL_14_005: [** This function shall parse the `configuration.args` JSON object and initialize a new `JAVA_MODULE_HOST_CONFIG` setting default values to all missing fields. **]**

**SRS_JAVA_MODULE_HOST_HL_14_006: [** This function shall pass `bus` and the newly created `JAVA_MODULE_HOST_CONFIG` structure to `JavaModuleHost_Create`. **]**

**SRS_JAVA_MODULE_HOST_HL_14_007: [** This function shall fail or succeed after this function call and return the value from this function call. **]**

**SRS_JAVA_MODULE_HOST_HL_14_010: [** This function shall return `NULL` if any underlying API call fails. **]**

##JavaModuleHost_HL_Destroy
```C
static void JavaModuleHost_HL_Destroy(MODULE_HANDLE module);
```
**SRS_JAVA_MODULE_HOST_HL_14_008: [** This function shall call the underlying Java Module Host's `_Destroy` function using this `module` `MODULE_HANDLE`. **]**

##JavaModuleHost_HL_Receive
```C
static void JavaModuleHost_HL_Receive(MODULE_HANDLE module, MESSAGE_HANDLE message);
```

**SRS_JAVA_MODULE_HOST_HL_14_009: [** This function shall call the underlying Java Module Host's `_Receive` function using this `module` `MODULE_HANDLE` and `message` `MESSAGE_HANDLE`. **]**