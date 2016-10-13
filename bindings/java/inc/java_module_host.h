// Copyright (c) Microsoft. All rights reserved.
// Licensed under MIT license. See LICENSE file in the project root for full license information.

#ifndef JAVA_MODULE_HOST_H
#define JAVA_MODULE_HOST_H

#include <stdbool.h>
#ifdef _WIN32
#include <cstdbool>
#endif
#include "azure_c_shared_utility/vector.h"
#include "module.h"

#define JAVA_MODULE_CLASS_NAME_KEY "class_name"
#define JAVA_MODULE_ARGS_KEY "args"
#define JAVA_MODULE_CLASS_PATH_KEY "class_path"
#define JAVA_MODULE_LIBRARY_PATH_KEY "library_path"
#define JAVA_MODULE_JVM_OPTIONS_KEY "jvm_options"
#define JAVA_MODULE_JVM_OPTIONS_VERSION_KEY "version"
#define JAVA_MODULE_JVM_OPTIONS_DEBUG_KEY "debug"
#define JAVA_MODULE_JVM_OPTIONS_DEBUG_PORT_KEY "debug_port"
#define JAVA_MODULE_JVM_OPTIONS_VERBOSE_KEY "verbose"
#define JAVA_MODULE_JVM_OPTIONS_ADDITIONAL_OPTIONS_KEY "additional_options"

#ifdef __cplusplus
extern "C"
{
#endif

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

MODULE_EXPORT void MODULE_STATIC_GETAPIS(JAVA_MODULE_HOST)(MODULE_APIS* apis);

#ifdef __cplusplus
}
#endif

#endif /*JAVA_MODULE_HOST_H*/
