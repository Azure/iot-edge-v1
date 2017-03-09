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

/*!
 * \brief Attach to the Proxy Gateway
 * 
 * `ProxyGateway_Attach` will provide the ProxyGateway library with the caller's
 * "Module API" and the connection id required to communicate with the Azure
 * IoT Gateway. When `ProxyGateway_Attach` is called the ProxyGateway library will
 * begin listening for the gateway create message. Once the create message is
 * received the ProxyGateway library will call `Module_ParseConfigurationFromJson`^,
 * `Module_Create` and `Module_FreeConfiguration` (assuming the optional API has
 * been provided).
 *
 * \param module_apis [in] A structure providing the standard Module API set.
 *                         Only Module_Create, Module_Destroy and Module_Receive
 *                         are mandatory.
 * \param connection_id [in] The unique identifier specified in the Azure IoT
 *                           Gateway JSON configuration
 *
 * \return A handle to a remote module
 */
MOCKABLE_FUNCTION(, GATEWAY_EXPORT REMOTE_MODULE_HANDLE, ProxyGateway_Attach, const MODULE_API *, module_apis, const char *, connection_id);

/*!
 * \brief Detach from the Proxy Gateway
 *
 * `ProxyGateway_Detach` will cease to interact with the Azure IoT Gateway and
 * clean up all resources required to maintain the connection with the gateway.
 *
 * \param remote_module [in] The handle of the remote module you wish to detach from
 *                           the Azure IoT Gateway.
 */
MOCKABLE_FUNCTION(, GATEWAY_EXPORT void, ProxyGateway_Detach, REMOTE_MODULE_HANDLE, remote_module);

/*!
 * \brief Process transactions for a given remote module.
 *
 * `ProxyGateway_DoWork` is intended to provide the caller with fine-grain control of work
 * scheduling, and is provided as an alternative to calling `ProxyGateway_StartWorkerThread`.
 * `ProxyGateway_DoWork` is a non-blocking call, wherein each call will check for one message
 * on the command and each message channel connected to the Azure IoT Gateway. In other
 * words, if multiple messages are queued on a single channel, only the first message of
 * each channel will be serviced. If the message is intended for the remote module (as
 * opposed to the ProxyGateway library itself), the ProxyGateway library will pass it along
 * by calling `Module_Receive` on the remote module.
 *
 * \param remote_module [in] The handle of the remote module you wish to detach from
 *                           the Azure IoT Gateway.
 *
 * \note If `ProxyGateway_StartWorkerThread` has been called, then calling `ProxyGateway_DoWork`
 *       will have no observable effect.
 */
MOCKABLE_FUNCTION(, GATEWAY_EXPORT void, ProxyGateway_DoWork, REMOTE_MODULE_HANDLE, remote_module);

/*!
 * \brief Halt the worker thread for a given remote module
 * 
 * `ProxyGateway_HaltWorkerThread` will signal and join the message thread. Once this
 * method has been invoked, no more messages will be received from the Azure IoT Gateway
 * without manually calling `ProxyGateway_DoWork`.
 * 
 * \param remote_module [in] The handle of the remote module you wish to detach from
 *                           the Azure IoT Gateway.
 *
 * \return A result value. 0 indicating success or failure otherwise
 *
 * \note The thread for a given module while be halted when `ProxyGateway_Detach` is called.
 */
MOCKABLE_FUNCTION(, GATEWAY_EXPORT int, ProxyGateway_HaltWorkerThread, REMOTE_MODULE_HANDLE, remote_module);

/*!
 * \brief Start a worker thread for a given remote module
 *
 * `ProxyGateway_StartWorkerThread` is a convenience method which gives control of calling
 * `ProxyGateway_DoWork` over to the ProxyGateway library. If `ProxyGateway_StartWorkerThread`
 * has been invoked, then the ProxyGateway library will create a thread to service and deliver
 * messages from the Azure IoT Gateway to the remote module.
 *
 * \param remote_module [in] The handle of the remote module you wish to detach from
 *                           the Azure IoT Gateway.
 *
 * \return A result value. 0 indicating success or failure otherwise
 */
MOCKABLE_FUNCTION(, GATEWAY_EXPORT int, ProxyGateway_StartWorkerThread, REMOTE_MODULE_HANDLE, remote_module);

#ifdef __cplusplus
  }
#endif

#endif /* REMOTE_MODULE_H */
