// Copyright (c) Microsoft. All rights reserved.
// Licensed under MIT license. See LICENSE file in the project root for full license information.

#ifndef JAVA_MODULE_HOST_HL_H
#define JAVA_MODULE_HOST_HL_H

#include "java_module_host.h"

#ifdef __cplusplus
extern "C"
{
#endif

MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(JAVA_MODULE_HOST_HL)(void);

#ifdef __cplusplus
}
#endif

#endif /*JAVA_MODULE_HOST_HL_H*/
