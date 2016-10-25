// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file       module.h
*    @brief     Interface for modules which communicate with other modules via
*               a message broker.
*
*    @details   Every module associated with the message broker must implement
*               this interface.
*               A module can only belong to one message broker; a message broker
*               may have many modules associated with it.
*
*               Every module library exports a function (Module_GetAPIs) that
*               returns a pointer to the #MODULE_APIS structure.
*/

#ifndef MODULE_H
#define MODULE_H

/** @brief  Encapsulates a module's handle and a cached copy of its interface. */
typedef struct MODULE_TAG MODULE;

/** @brief  Represents a module instance. */
typedef void* MODULE_HANDLE;

/** @brief  Provides access to a module's interface. */
typedef struct MODULE_APIS_TAG MODULE_APIS;

#include "azure_c_shared_utility/macro_utils.h"
#include "broker.h"
#include "message.h"


#ifdef __cplusplus
extern "C"
{
#endif

    struct MODULE_TAG
    {
        const MODULE_APIS* module_apis;
        MODULE_HANDLE module_handle;
    };

    /** @brief      Creates an instance of a module.
    *
    *   @details    This function is optional.
    *
    *   @param      broker          The #BROKER_HANDLE to which this module
    *                               will publish messages.
    *   @param      configuration   A JSON string which describes any needed
    *                               configuration to create this module.
    *
    *   @return     A non-NULL #MODULE_HANDLE upon success, otherwise @c NULL.
    */
    typedef MODULE_HANDLE(*pfModule_CreateFromJson)(BROKER_HANDLE broker, const char* configuration);

    /** @brief      Creates an instance of a module.
    *
    *   @details    This function must be implemented.
    *
    *   @param      broker          The #BROKER_HANDLE to which this module
    *                               will publish messages.
    *   @param      configuration   A pointer to the user-defined configuration 
    *                               structure for this module.
    *
    *   @return     A non-NULL #MODULE_HANDLE upon success, otherwise @c NULL.
    */
    typedef MODULE_HANDLE(*pfModule_Create)(BROKER_HANDLE broker, const void* configuration);

    /** @brief      Disposes of the resources managed by this module.
    *
    *   @details    This function must be implemented.
    *
    *   @param      moduleHandle    The #MODULE_HANDLE of the module to be
    *                               destroyed.
    */
    typedef void(*pfModule_Destroy)(MODULE_HANDLE moduleHandle);

    /** @brief      Receives a message from the broker.
    *
    *   @details    This function must be implemented.
    *
    *   @param      moduleHandle    The #MODULE_HANDLE of the module receiving
    *                               the message.
    *   @param      messageHandle   The #MESSAGE_HANDLE of the message being
    *                               sent to the module.
    */
    typedef void(*pfModule_Receive)(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle);

    /** @brief      Signals to the module that the broker is ready to send and
    *               receive messages.
    *
    *   @details    This function is optional, but recommended. Messaging in
    *               the gateway is more predictable when modules delay
    *               publishing until after this function is called.
    *
    *   @param      moduleHandle    The target module's #MODULE_HANDLE.
    */
    typedef void(*pfModule_Start)(MODULE_HANDLE moduleHandle);

    /** @brief      Structure returned by Module_GetAPIS containing the
    *               function pointers that comprise a module's interface.
    */
    struct MODULE_APIS_TAG
    {
        pfModule_CreateFromJson Module_CreateFromJson;
        pfModule_Create Module_Create;
        pfModule_Destroy Module_Destroy;
        pfModule_Receive Module_Receive;
        pfModule_Start Module_Start;
    };

    /** @brief      The only function exported by a module library. Returns a
    *               module's runtime interface.
    */
    typedef void (*pfModule_GetAPIS)(MODULE_APIS* apis);

/** @brief  Returns the exported name of pfModule_GetAPIS.*/
#define MODULE_GETAPIS_NAME ("Module_GetAPIS")

#ifdef _WIN32
#define MODULE_EXPORT __declspec(dllexport)
#else
#define MODULE_EXPORT
#endif // _WIN32

/** @brief      Returns a unique name for pfModule_GetAPIS that is used when
*               the module is statically linked.
*/
#define MODULE_STATIC_GETAPIS(MODULE_NAME) C2(Module_GetAPIS_, MODULE_NAME)

/** @brief      Static definition for pfModule_GetAPIS. */
MODULE_EXPORT void Module_GetAPIS(MODULE_APIS* apis);

#ifdef __cplusplus
}
#endif

#endif // MODULE_H
