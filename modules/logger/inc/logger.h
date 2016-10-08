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

MODULE_EXPORT void MODULE_STATIC_GETAPIS(LOGGER_MODULE)(MODULE_APIS* apis);

#ifdef __cplusplus
}
#endif


#endif /*LOGGER_H*/
