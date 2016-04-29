// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef HELLO_WORLD_HL_H
#define HELLO_WORLD_HL_H

#include "module.h"

#ifdef __cplusplus
extern "C"
{
#endif

MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(HELLOWORLD_HL_MODULE)(void);

#ifdef __cplusplus
}
#endif


#endif /*HELLO_WORLD_HL_H*/
