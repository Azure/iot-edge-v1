// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef HELLO_WORLD_H
#define HELLO_WORLD_H

#include "module.h"

#ifdef __cplusplus
extern "C"
{
#endif

MODULE_EXPORT void MODULE_STATIC_GETAPIS(HELLOWORLD_MODULE)(MODULE_APIS* apis);

#ifdef __cplusplus
}
#endif


#endif /*HELLO_WORLD_H*/
