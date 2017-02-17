// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef OOP_MODULE_H
#define OOP_MODULE_H

#include "azure_c_shared_utility/macro_utils.h"

#include "gateway_export.h"
#include "module.h"

#ifdef __cplusplus
  extern "C" {
#endif

typedef struct OOP_MODULE_TAG * OOP_MODULE_HANDLE;

#include "azure_c_shared_utility/umock_c_prod.h"

MOCKABLE_FUNCTION(, GATEWAY_EXPORT OOP_MODULE_HANDLE, OopModule_Attach, const MODULE_API *, module_apis, const char * const, connection_id);
MOCKABLE_FUNCTION(, GATEWAY_EXPORT void, OopModule_Detach, OOP_MODULE_HANDLE, oop_module);
MOCKABLE_FUNCTION(, GATEWAY_EXPORT void, OopModule_DoWork, OOP_MODULE_HANDLE, oop_module);
MOCKABLE_FUNCTION(, GATEWAY_EXPORT int, OopModule_HaltWorkerThread, OOP_MODULE_HANDLE, oop_module);
MOCKABLE_FUNCTION(, GATEWAY_EXPORT int, OopModule_StartWorkerThread, OOP_MODULE_HANDLE, oop_module);

#ifdef __cplusplus
  }
#endif

#endif /* OOP_MODULE_H */
