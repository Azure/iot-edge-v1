// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef NODEJS_HL_H
#define NODEJS_HL_H

#include "module.h"

#ifdef __cplusplus
extern "C"
{
#endif

MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(NODEJS_MODULE_HL)(void);

#ifdef __cplusplus
}
#endif

#endif /*NODEJS_HL_H*/
