// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstring>
#include <cstdbool>

#include "module.h"
#include "message.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/base64.h"
#include "dynamic_library.h"
#include "dotnetcore.h"

#include <memory>
#include <stdio.h>

#include <parson.h>
#include <string>
#include <fstream>
#include "dotnetcore_common.h"
#include "dotnetcore_utils.h"


using namespace dotnetcore_module;

#define AZUREIOTGATEWAYASSEMBLYNAME L"Microsoft.Azure.Devices.Gateway"


typedef unsigned int(DOTNET_CORE_CALLING_CONVENTION *PGatewayCreateDelegate)(intptr_t broker, intptr_t module, const char* assemblyName, const char* entryType, const char* gatewayConfiguration);

typedef void(DOTNET_CORE_CALLING_CONVENTION *PGatewayReceiveDelegate)(unsigned char* buffer, int32_t bufferSize, unsigned int moduleIdManaged);

typedef void(DOTNET_CORE_CALLING_CONVENTION *PGatewayDestroyDelegate)(unsigned int moduleIdManaged);

typedef void(DOTNET_CORE_CALLING_CONVENTION *PGatewayStartDelegate)(unsigned int moduleIdManaged);

PGatewayCreateDelegate GatewayCreateDelegate = NULL;

PGatewayReceiveDelegate GatewayReceiveDelegate = NULL;

PGatewayDestroyDelegate GatewayDestroyDelegate = NULL;

PGatewayStartDelegate GatewayStartDelegate = NULL;


static DYNAMIC_LIBRARY_HANDLE hCoreCLRModule = NULL;

static size_t m_dotnet_core_modules_counter = 0;

static void* hostHandle = NULL;

static unsigned int domainId = 0;

static coreclr_initialize_ptr m_ptr_coreclr_initialize = NULL;

static coreclr_create_delegate_ptr m_ptr_coreclr_create_delegate = NULL;

static coreclr_shutdown_ptr m_ptr_coreclr_shutdown = NULL;

static MODULE_HANDLE DotNetCore_Create(BROKER_HANDLE broker, const void* configuration)
{
    DOTNET_CORE_HOST_HANDLE_DATA* result = NULL;

    if (
        (broker == NULL) ||
        (configuration == NULL)
        )
    {
        /* Codes_SRS_DOTNET_CORE_04_001: [ DotNetCore_Create shall return NULL if broker is NULL. ] */
        /* Codes_SRS_DOTNET_CORE_04_002: [ DotNetCore_Create shall return NULL if configuration is NULL. ] */
        LogError("invalid arg broker=%p, configuration=%p", broker, configuration);
    }
    else
    {
        DOTNET_CORE_HOST_CONFIG* dotNetCoreConfig = (DOTNET_CORE_HOST_CONFIG*)configuration;
        if (dotNetCoreConfig->assemblyName == NULL)
        {
            /* Codes_SRS_DOTNET_CORE_04_003: [ DotNetCore_Create shall return NULL if configuration->assemblyName is NULL. ] */
            LogError("invalid configuration. assemblyName=%p", dotNetCoreConfig->assemblyName);
        }
        else if (dotNetCoreConfig->entryType == NULL)
        {
            /* Codes_SRS_DOTNET_CORE_04_004: [ DotNetCore_Create shall return NULL if configuration->entryType is NULL. ] */
            LogError("invalid configuration. entryType=%p", dotNetCoreConfig->entryType);
        }
        else if (dotNetCoreConfig->moduleArgs == NULL)
        {
            /* Codes_SRS_DOTNET_CORE_04_005: [ DotNetCore_Create shall return NULL if configuration->moduleArgs is NULL. ] */
            LogError("invalid configuration. moduleArgs=%p", dotNetCoreConfig->moduleArgs);
        }
        else if (dotNetCoreConfig->clrOptions == NULL)
        {
            /* Codes_SRS_DOTNET_CORE_04_035: [ DotNetCore_Create shall return NULL if configuration->clrOptions is NULL. ] */
            LogError("invalid configuration. clrOptions=%p", dotNetCoreConfig->clrOptions);
        }
        else if (dotNetCoreConfig->clrOptions->coreClrPath == NULL)
        {
            /* Codes_SRS_DOTNET_CORE_04_036: [ DotNetCore_Create shall return NULL if configuration->clrOptions->coreClrPath is NULL. ] */
            LogError("invalid configuration. clrOptions->coreClrPath=%p", dotNetCoreConfig->clrOptions->coreClrPath);
        }
        else if (dotNetCoreConfig->clrOptions->trustedPlatformAssembliesLocation == NULL)
        {
            /* Codes_SRS_DOTNET_CORE_04_037: [ DotNetCore_Create shall return NULL if configuration->clrOptions->trustedPlatformAssembliesLocation is NULL. ] */
            LogError("invalid configuration. clrOptions->trustedPlatformAssembliesLocation=%p", dotNetCoreConfig->clrOptions->trustedPlatformAssembliesLocation);
        }
        else
        {
            /* Codes_SRS_DOTNET_CORE_04_010: [ DotNetCore_Create shall load coreclr library, if not loaded yet. ] */
            if (hCoreCLRModule == NULL && GatewayCreateDelegate == NULL)
            {
                hCoreCLRModule = DynamicLibrary_LoadLibrary(dotNetCoreConfig->clrOptions->coreClrPath);

                if (hCoreCLRModule == NULL)
                {
                    /* Codes_SRS_DOTNET_CORE_04_006: [ DotNetCore_Create shall return NULL if an underlying API call fails. ] */
                    LogError("Failed to Load .NET Core CLR Library.");
                }
                else
                {
                    /* Codes_SRS_DOTNET_CORE_04_011: [ DotNetCore_Create shall get address for 3 external methods coreclr_initialize, coreclr_shutdown and coreclr_create_delegate and save it to global reference for Dot Net Core binding. ] */
                    m_ptr_coreclr_initialize = (coreclr_initialize_ptr)DynamicLibrary_FindSymbol(hCoreCLRModule, "coreclr_initialize");
                    if (m_ptr_coreclr_initialize == NULL)
                    {
                        DynamicLibrary_UnloadLibrary(hCoreCLRModule);
                        hCoreCLRModule = NULL;
                        /* Codes_SRS_DOTNET_CORE_04_006: [ DotNetCore_Create shall return NULL if an underlying API call fails. ] */
                        LogError("Failed to find Symbol: coreclr_initialize");
                    }
                    else
                    {
                        m_ptr_coreclr_shutdown = (coreclr_shutdown_ptr)DynamicLibrary_FindSymbol(hCoreCLRModule, "coreclr_shutdown");
                        if (m_ptr_coreclr_shutdown == NULL)
                        {
                            DynamicLibrary_UnloadLibrary(hCoreCLRModule);
                            hCoreCLRModule = NULL;
                            /* Codes_SRS_DOTNET_CORE_04_006: [ DotNetCore_Create shall return NULL if an underlying API call fails. ] */
                            LogError("Failed to find Symbol: coreclr_shutdown");
                        }
                        else
                        {
                            m_ptr_coreclr_create_delegate = (coreclr_create_delegate_ptr)DynamicLibrary_FindSymbol(hCoreCLRModule, "coreclr_create_delegate");
                            if (m_ptr_coreclr_create_delegate == NULL)
                            {
                                DynamicLibrary_UnloadLibrary(hCoreCLRModule);
                                hCoreCLRModule = NULL;
                                /* Codes_SRS_DOTNET_CORE_04_006: [ DotNetCore_Create shall return NULL if an underlying API call fails. ] */
                                LogError("Failed to find Symbol: coreclr_create_delegate");
                            }
                            else
                            {
                                bool resultInitializeDotNetCoreCLR = initializeDotNetCoreCLR(m_ptr_coreclr_initialize, dotNetCoreConfig->clrOptions->trustedPlatformAssembliesLocation, &hostHandle, &domainId);

                                if (!resultInitializeDotNetCoreCLR)
                                {
                                    DynamicLibrary_UnloadLibrary(hCoreCLRModule);
                                    hCoreCLRModule = NULL;
                                    /* Codes_SRS_DOTNET_CORE_04_006: [ DotNetCore_Create shall return NULL if an underlying API call fails. ] */
                                    LogError("Failed to initialize DotNetCoreCLR.");
                                }
                                else
                                {
                                    int status = -1;
                                    try
                                    {
                                        /* Codes_SRS_DOTNET_CORE_04_013: [ DotNetCore_Create shall call coreclr_create_delegate to be able to call Microsoft.Azure.Devices.Gateway.GatewayDelegatesGateway.Delegates_Create ] */
                                        status = m_ptr_coreclr_create_delegate(
                                            hostHandle,
                                            domainId,
                                            "Microsoft.Azure.Devices.Gateway",
                                            "Microsoft.Azure.Devices.Gateway.NetCoreInterop",
                                            "Create",
                                            reinterpret_cast<void**>(&GatewayCreateDelegate)
                                        );
                                    }
                                    catch (const std::exception& msgErr)
                                    {
                                        (void)msgErr;
                                        status = -1;
                                        LogError("Exception Thrown. Create Delegate failed.");
                                    }

                                    if (status < 0)
                                    {
                                        DynamicLibrary_UnloadLibrary(hCoreCLRModule);
                                        hCoreCLRModule = NULL;
                                        GatewayCreateDelegate = NULL;
                                        /* Codes_SRS_DOTNET_CORE_04_006: [ DotNetCore_Create shall return NULL if an underlying API call fails. ] */
                                        LogError("Failed to create Create Delegate.");
                                    }
                                    else
                                    {
                                        int status = -1;
                                        try
                                        {
                                            /* Codes_SRS_DOTNET_CORE_04_021: [ DotNetCore_Create shall call coreclr_create_delegate to be able to call Microsoft.Azure.Devices.Gateway.GatewayDelegatesGateway.Delegates_Receive ] */
                                            status = m_ptr_coreclr_create_delegate(
                                                hostHandle,
                                                domainId,
                                                "Microsoft.Azure.Devices.Gateway",
                                                "Microsoft.Azure.Devices.Gateway.NetCoreInterop",
                                                "Receive",
                                                reinterpret_cast<void**>(&GatewayReceiveDelegate)
                                            );
                                        }
                                        catch (const std::exception& msgErr)
                                        {
                                            (void)msgErr;
                                            status = -1;
                                            LogError("Exception Thrown. Receive delegate failed");
                                        }

                                        if (status < 0)
                                        {
                                            DynamicLibrary_UnloadLibrary(hCoreCLRModule);
                                            hCoreCLRModule = NULL;
                                            GatewayCreateDelegate = NULL;
                                            GatewayReceiveDelegate = NULL;
                                            /* Codes_SRS_DOTNET_CORE_04_006: [ DotNetCore_Create shall return NULL if an underlying API call fails. ] */
                                            LogError("Failed to create Receive Delegate.");
                                        }
                                        else
                                        {
                                            try
                                            {
                                                /* Codes_SRS_DOTNET_CORE_04_024: [ DotNetCore_Destroy shall call coreclr_create_delegate to be able to call Microsoft.Azure.Devices.Gateway.GatewayDelegatesGateway.Delegates_Destroy ] */
                                                status = m_ptr_coreclr_create_delegate(
                                                    hostHandle,
                                                    domainId,
                                                    "Microsoft.Azure.Devices.Gateway",
                                                    "Microsoft.Azure.Devices.Gateway.NetCoreInterop",
                                                    "Destroy",
                                                    reinterpret_cast<void**>(&GatewayDestroyDelegate)
                                                );
                                            }
                                            catch (const std::exception& msgErr)
                                            {
                                                (void)msgErr;
                                                status = -1;
                                                LogError("Exception Thrown. Destroy delegate failed.");
                                            }

                                            if (status < 0)
                                            {
                                                DynamicLibrary_UnloadLibrary(hCoreCLRModule);
                                                hCoreCLRModule = NULL;
                                                GatewayCreateDelegate = NULL;
                                                GatewayDestroyDelegate = NULL;
                                                GatewayReceiveDelegate = NULL;
                                                /* Codes_SRS_DOTNET_CORE_04_006: [ DotNetCore_Create shall return NULL if an underlying API call fails. ] */
                                                LogError("Failed to create Destroy Delegate.");
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            //if hcoreCLRModule is still NULL, then is failed to initialize. 
            /* Codes_SRS_DOTNET_CORE_04_006: [ DotNetCore_Create shall return NULL if an underlying API call fails. ] */
            if (hCoreCLRModule != NULL || GatewayCreateDelegate != NULL)
            {
                /* Codes_SRS_DOTNET_CORE_04_007: [ DotNetCore_Create shall return a non-NULL MODULE_HANDLE when successful. ] */
                try
                {
                    result = new DOTNET_CORE_HOST_HANDLE_DATA(broker, dotNetCoreConfig->assemblyName);
                }
                catch (const std::exception& msgErr)
                {
                    (void)msgErr;
                    LogError("Failed on allocate memory for Handle Data.");
                    result = NULL;
                }


                if (result != NULL)
                {
                    try
                    {
                        /* Codes_SRS_DOTNET_CORE_04_014: [ DotNetCore_Create shall call Microsoft.Azure.Devices.Gateway.GatewayDelegatesGateway.Delegates_Create C# method, implemented on Microsoft.Azure.Devices.Gateway.dll. ] */
                        result->module_id = (*GatewayCreateDelegate)((intptr_t)broker, (intptr_t)result, dotNetCoreConfig->assemblyName, dotNetCoreConfig->entryType, dotNetCoreConfig->moduleArgs);

                        m_dotnet_core_modules_counter++;
                    }
                    catch (const std::exception& msgErr)
                    {
                        (void)msgErr;
                        LogError("Failed on Create Delegate Call.");
                        delete result;
                        result = NULL;
                    }
                }
                else
                {
                    LogError("Failed allocating memory for module data.");
                }
            }
        }
    }

    /* Codes_SRS_DOTNET_CORE_04_007: [ DotNetCore_Create shall return a non-NULL MODULE_HANDLE when successful. ] */
    return result;
}

static void* DotNetCore_ParseConfigurationFromJson(const char* configuration)
{
    return (void*)configuration;
}

void DotNetCore_FreeConfiguration(void* configuration)
{
    (void)configuration;
    //Nothing to be freed here.
}

static void DotNetCore_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
        if (
            (moduleHandle != NULL) &&
            (messageHandle != NULL)
            )
        {
            DOTNET_CORE_HOST_HANDLE_DATA* result = (DOTNET_CORE_HOST_HANDLE_DATA*)moduleHandle;

            /* Codes_SRS_DOTNET_CORE_04_020: [ DotNetCore_Receive shall call Message_ToByteArray to serialize message. ] */
            int32_t size = Message_ToByteArray(messageHandle, NULL, 0); 

            if (size > 0)
            {
                unsigned char* buffer = (unsigned char*)malloc(size);

                if (buffer != NULL)
                {
                    int32_t resultFromConversionToByteArray = Message_ToByteArray(messageHandle, buffer, size);

                    if (resultFromConversionToByteArray > 0)
                    {
                        try
                        {
                            /* Codes_SRS_DOTNET_CORE_04_022: [ DotNetCore_Receive shall call Microsoft.Azure.Devices.Gateway.GatewayDelegatesGateway.Delegates_Receive C# method, implemented on Microsoft.Azure.Devices.Gateway.dll. ] */
                            (*GatewayReceiveDelegate)(buffer, size, result->module_id);
                        }
                        catch (const std::exception& msgErr)
                        {
                            (void)msgErr;
                            LogError("Exception Thrown. Error on calling Receive Delegate.");
                        }
                    }
                    else
                    {
                        LogError("Unable to convert message to Byte Array");
                    }

                    delete(buffer);
                }
            }
        }
        else
        {
            /* Codes_SRS_DOTNET_CORE_04_018: [ DotNetCore_Receive shall do nothing if module is NULL. ] */
            /* Codes_SRS_DOTNET_CORE_04_019: [ DotNetCore_Receive shall do nothing if message is NULL. ] */
            LogError("invalid arg moduleHandle=%p, messageHandle=%p", moduleHandle, messageHandle);
            /*do nothing*/
        }
}


static void DotNetCore_Destroy(MODULE_HANDLE module)
{
    /* Codes_SRS_DOTNET_CORE_04_023: [ DotNetCore_Destroy shall do nothing if module is NULL. ] */
    if (module != NULL)
    {
        DOTNET_CORE_HOST_HANDLE_DATA* handleData = (DOTNET_CORE_HOST_HANDLE_DATA*)module;

        try
        {
            /* Codes_SRS_DOTNET_CORE_04_025: [ DotNetCore_Destroy shall call Microsoft.Azure.Devices.Gateway.GatewayDelegatesGateway.Delegates_Destroy C# method, implemented on Microsoft.Azure.Devices.Gateway.dll. ] */
            (*GatewayDestroyDelegate)(handleData->module_id);
        }
        catch (const std::exception& msgErr)
        {
            (void)msgErr;
            LogError("Exception Thrown. Error on Destroy method. ");
        }

        /* Codes_SRS_DOTNET_CORE_04_038: [ DotNetCore_Destroy shall release all resources allocated by DotNetCore_Create. ] */
        delete(handleData);
        m_dotnet_core_modules_counter--;

        if (m_dotnet_core_modules_counter == 0 && hCoreCLRModule != NULL)
        {
            /* Codes_SRS_DOTNET_CORE_04_039: [ DotNetCore_Destroy shall verify that there is no module and shall shutdown the dotnet core clr. ] */
            m_ptr_coreclr_shutdown(hostHandle, domainId); //Can't call Unload CoreCLR otherwise CLR will crash. 
            hCoreCLRModule = NULL;
        }
    }
}

MODULE_EXPORT bool Module_DotNetCoreHost_PublishMessage(BROKER_HANDLE broker, MODULE_HANDLE sourceModule, const unsigned char* message, int32_t size)
{
    bool returnValue = false;
    MESSAGE_HANDLE messageToPublish = NULL;

    /* Codes_SRS_DOTNET_CORE_04_027: [ Module_DotNetCoreHost_PublishMessage shall return false if broker is NULL. ] */
    /* Codes_SRS_DOTNET_CORE_04_028: [ Module_DotNetCoreHost_PublishMessage shall return false if sourceModule is NULL. ] */
    /* Codes_SRS_DOTNET_CORE_04_029: [ Module_DotNetCoreHost_PublishMessage shall return false if message is NULL, or size if lower than 0. ] */
    if (broker == NULL || sourceModule == NULL || message == NULL || size < 0)
    {
        LogError("invalid arg broker=%p, sourceModule=%p", broker, sourceModule);
    }
    /* Codes_SRS_DOTNET_CORE_04_030: [ Module_DotNetCoreHost_PublishMessage shall create a message from message and size by invoking Message_CreateFromByteArray. ] */
    else if((messageToPublish = Message_CreateFromByteArray(message, size)) == NULL)
    {
        /* Codes_SRS_DOTNET_CORE_04_031: [ If Message_CreateFromByteArray fails, Module_DotNetCoreHost_PublishMessage shall fail. ] */
        LogError("Error trying to create message from Byte Array");
    }
    /* Codes_SRS_DOTNET_CORE_04_032: [ Module_DotNetCoreHost_PublishMessage shall call Broker_Publish passing broker, sourceModule, message and size. ] */
    else if (Broker_Publish(broker, sourceModule, messageToPublish) != BROKER_OK)
    {
        /* Codes_SRS_DOTNET_CORE_04_033: [ If Broker_Publish fails Module_DotNetCoreHost_PublishMessage shall fail. ] */
        LogError("Error trying to publish message on Broker.");
    }
    else
    {
        /* Codes_SRS_DOTNET_CORE_04_034: [ If Broker_Publish succeeds Module_DotNetCoreHost_PublishMessage shall succeed. ] */
        returnValue = true;
    }

    if (messageToPublish != NULL)
    {
        Message_Destroy(messageToPublish);
    }

    return returnValue;
}


MODULE_EXPORT void Module_DotNetCoreHost_SetBindingDelegates(intptr_t createAddress, intptr_t receiveAddress, intptr_t destroyAddress, intptr_t startAddress)
{
    /* Codes_SRS_DOTNET_CORE_04_040: [ Module_DotNetCoreHost_SetBindingDelegates shall just assign createAddress to GatewayCreateDelegate ] */
    GatewayCreateDelegate = (PGatewayCreateDelegate)createAddress;
    /* Codes_SRS_DOTNET_CORE_04_041: [ Module_DotNetCoreHost_SetBindingDelegates shall just assign receiveAddress to GatewayReceiveDelegate ] */
    GatewayReceiveDelegate = (PGatewayReceiveDelegate)receiveAddress;
    /* Codes_SRS_DOTNET_CORE_04_042: [ Module_DotNetCoreHost_SetBindingDelegates shall just assign destroyAddress to GatewayDestroyDelegate ] */
    GatewayDestroyDelegate = (PGatewayDestroyDelegate)destroyAddress;
    /* Codes_SRS_DOTNET_CORE_04_043: [ Module_DotNetCoreHost_SetBindingDelegates shall just assign startAddress to GatewayStartDelegate ] */
    GatewayStartDelegate = (PGatewayStartDelegate)startAddress;
}
static void DotNetCore_Start(MODULE_HANDLE module)
{
    /*Codes_SRS_DOTNET_CORE_004_015: [ DotNetCore_Start shall do nothing if module is NULL. ] */
    if (module != NULL)
    {
        DOTNET_CORE_HOST_HANDLE_DATA* handleData = (DOTNET_CORE_HOST_HANDLE_DATA*)module;
            
        int status = -1;

        if (GatewayStartDelegate == NULL && hCoreCLRModule != NULL)
        {
            /* Codes_SRS_DOTNET_CORE_004_016: [ DotNetCore_Start shall call coreclr_create_delegate to be able to call Microsoft.Azure.Devices.Gateway.GatewayDelegatesGateway.Delegates_Start ] */
            status = m_ptr_coreclr_create_delegate(
                hostHandle,
                domainId,
                "Microsoft.Azure.Devices.Gateway",
                "Microsoft.Azure.Devices.Gateway.NetCoreInterop",
                "Start",
                reinterpret_cast<void**>(&GatewayStartDelegate)
            );
        }
        else
        {
            //If there is already a Start Delegate, set status to 0 so we can call it.
            status = 0;
        }

        if(status == 0)
        {
            try
            {
                /* Codes_SRS_DOTNET_CORE_004_017: [ DotNetCore_Start shall call Microsoft.Azure.Devices.Gateway.GatewayDelegatesGateway.Delegates_Start C# method, implemented on Microsoft.Azure.Devices.Gateway.dll. ] */
                (*GatewayStartDelegate)(handleData->module_id);
            }
            catch (const std::exception& msgErr)
            {
                (void)msgErr;
                LogError("Exception Thrown. Error on Start method.");
            }
        }
        else{
            //Ignore, Start is optional. 
        }

    }
}

static const MODULE_API_1 DOTNET_CORE_APIS_all =
{
    {MODULE_API_VERSION_1},
    DotNetCore_ParseConfigurationFromJson,
    DotNetCore_FreeConfiguration,
    DotNetCore_Create,
    DotNetCore_Destroy,
    DotNetCore_Receive,
    DotNetCore_Start
};

/*Codes_SRS_DOTNET_CORE_04_026: [ Module_GetApi shall return out the provided MODULES_API structure with required module's APIs functions. ]*/
#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(DOTNET_HOST)(MODULE_API_VERSION gateway_api_version)
#else
MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version)
#endif
{
    (void)gateway_api_version;
    return (const MODULE_API *)&DOTNET_CORE_APIS_all;
}
