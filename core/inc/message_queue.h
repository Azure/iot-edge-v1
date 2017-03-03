// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include "message.h"

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/umock_c_prod.h"

#ifdef __cplusplus
#include <cstddef>
#include <cstdbool>
extern "C"
{
#else
#include <stddef.h>
#include <stdbool.h>
#endif

typedef struct MESSAGE_QUEUE_TAG* MESSAGE_QUEUE_HANDLE;

/* creation */
MOCKABLE_FUNCTION(, MESSAGE_QUEUE_HANDLE, MESSAGE_QUEUE_create);
MOCKABLE_FUNCTION(, void, MESSAGE_QUEUE_destroy, MESSAGE_QUEUE_HANDLE, handle);

/* insertion */
MOCKABLE_FUNCTION(, int, MESSAGE_QUEUE_push, MESSAGE_QUEUE_HANDLE, handle, MESSAGE_HANDLE, element);

/* removal */
MOCKABLE_FUNCTION(, MESSAGE_HANDLE, MESSAGE_QUEUE_pop, MESSAGE_QUEUE_HANDLE, handle);

/* access */
MOCKABLE_FUNCTION(, bool,  MESSAGE_QUEUE_is_empty, MESSAGE_QUEUE_HANDLE, handle);
MOCKABLE_FUNCTION(, MESSAGE_HANDLE, MESSAGE_QUEUE_front, MESSAGE_QUEUE_HANDLE, handle);

#ifdef __cplusplus
}
#endif

#endif /* MESSAGE_QUEUE_H */
