// Copyright (c) Microsoft. All rights reserved.
// Licensed under MIT license. See LICENSE file in the project root for full license information.

#ifndef JAVA_MODULE_HOST_HL_H
#define JAVA_MODULE_HOST_HL_H

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

#include "module.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef UNDER_TEST

	extern const MODULE_APIS* MODULE_STATIC_GETAPIS(JAVA_MODULE_HOST)(void);

#endif

MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(JAVA_MODULE_HOST_HL)(void);

#ifdef __cplusplus
}
#endif

#endif /*JAVA_MODULE_HOST_HL_H*/
