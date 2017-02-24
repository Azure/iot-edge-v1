// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REMOTE_MODULE_H
#define REMOTE_MODULE_H

#include "azure_c_shared_utility/macro_utils.h"

#include "gateway_export.h"
#include "module.h"

#ifdef __cplusplus
  extern "C" {
#endif

typedef struct REMOTE_MODULE_TAG * REMOTE_MODULE_HANDLE;

#include "azure_c_shared_utility/umock_c_prod.h"

MOCKABLE_FUNCTION(, GATEWAY_EXPORT REMOTE_MODULE_HANDLE, RemoteModule_Attach, const MODULE_API *, module_apis, const char *, connection_id);
MOCKABLE_FUNCTION(, GATEWAY_EXPORT void, RemoteModule_Detach, REMOTE_MODULE_HANDLE, remote_module);
MOCKABLE_FUNCTION(, GATEWAY_EXPORT void, RemoteModule_DoWork, REMOTE_MODULE_HANDLE, remote_module);
MOCKABLE_FUNCTION(, GATEWAY_EXPORT int, RemoteModule_HaltWorkerThread, REMOTE_MODULE_HANDLE, remote_module);
MOCKABLE_FUNCTION(, GATEWAY_EXPORT int, RemoteModule_StartWorkerThread, REMOTE_MODULE_HANDLE, remote_module);

#ifdef __cplusplus
  }
#endif

#endif /* REMOTE_MODULE_H */
