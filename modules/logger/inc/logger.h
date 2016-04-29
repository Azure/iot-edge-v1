// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef LOGGER_H
#define LOGGER_H

#include "module.h"
#include "logger_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(LOGGER_MODULE)(void);

#ifdef __cplusplus
}
#endif


#endif /*LOGGER_H*/
