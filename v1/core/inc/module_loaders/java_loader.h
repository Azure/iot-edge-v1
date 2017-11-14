// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef JAVA_LOADER_H
#define JAVA_LOADER_H

#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/umock_c_prod.h"

#include "module.h"
#include "module_loader.h"
#include "java_module_host.h"
#include "gateway_version.h"
#include "gateway_export.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define JAVA_LOADER_NAME            "java"

#define INSTALL_NAME "azure_iot_gateway_sdk"

//=============================================================================
//Set Platform Specific Env Vars
//=============================================================================
#ifdef _WIN64
#define GET_PREFIX                  getenv("PROGRAMFILES")
#else
#ifdef WIN32
#define GET_PREFIX                  getenv("PROGRAMFILES(X86)")
#else
#define GET_PREFIX                  "/usr/local/lib"
#endif
#endif

#if WIN32
#define JAVA_BINDING_MODULE_NAME    "java_module_host.dll"
#define MODULES_INSTALL_LOCATION    "\\lib\\modules"
#define BINDINGS_INSTALL_LOCATION   "\\lib\\bindings\\java\\classes"
#define SEPARATOR                   ";"
#define SLASH                       "\\"
#define PREFIXES                    
#ifdef UNDER_TEST
#define ENV_VARS
#else
#define ENV_VARS ,\
    "PROGRAMFILES", \
    "PROGRAMFILES(X86)"
#endif
#else
#define JAVA_BINDING_MODULE_NAME    "libjava_module_host.so"
#define MODULES_INSTALL_LOCATION    "/modules"
#define BINDINGS_INSTALL_LOCATION   "/bindings/java/classes"
#define SEPARATOR                   ":"
#define SLASH                       "/"
#define PREFIXES                    , "/usr/local/lib"
#define ENV_VARS                    
#endif

#define ENTRYPOINT_CLASSNAME        "class.name"
#define ENTRYPOINT_CLASSPATH        "class.path"
#define JVM_OPTIONS_KEY             "jvm.options"

typedef struct JAVA_LOADER_CONFIGURATION_TAG
{
    MODULE_LOADER_BASE_CONFIGURATION base;
    JVM_OPTIONS* options;
} JAVA_LOADER_CONFIGURATION;

typedef struct JAVA_LOADER_ENTRYPOINT_TAG
{
    STRING_HANDLE className;
    STRING_HANDLE classPath;
} JAVA_LOADER_ENTRYPOINT;

MOCKABLE_FUNCTION(, GATEWAY_EXPORT const MODULE_LOADER*, JavaLoader_Get);

#ifdef __cplusplus
}
#endif

#endif // JAVA_LOADER_H
