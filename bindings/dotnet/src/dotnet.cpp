// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "azure_c_shared_utility/gballoc.h"

#include "module.h"
#include "message.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/base64.h"

#ifdef UNDER_TEST
#define SafeArrayCreateVector myTest_SafeArrayCreateVector
#define SafeArrayPutElement   myTest_SafeArrayPutElement
#define SafeArrayDestroy      myTest_SafeArrayDestroy
#define SafeArrayCreate       myTest_SafeArrayCreate
#define SafeArrayAccessData   myTest_SafeArrayAccessData
#define SafeArrayUnaccessData myTest_SafeArrayUnaccessData
#endif

#include "dotnet.h"

#include <new>
#include <memory>
#include <metahost.h>
#include <atlbase.h>
#include <stdio.h>

#include <parson.h>


#define DEFAULT_CLR_VERSION L"v4.0.30319"
#define AZUREIOTGATEWAYASSEMBLYNAME L"Microsoft.Azure.IoT.Gateway"
#define AZUREIOTGATEWAY_BROKER_CLASSNAME L"Microsoft.Azure.IoT.Gateway.Broker"
#define AZUREIOTGATEWAY_MESSAGE_CLASSNAME L"Microsoft.Azure.IoT.Gateway.Message"




// Import mscorlib.tlb (Microsoft Common Language Runtime Class Library).
#import "mscorlib.tlb" raw_interfaces_only                 \
    high_property_prefixes("_get","_put","_putref")        \
    rename("ReportEvent", "InteropServices_ReportEvent")
using namespace mscorlib;

struct DOTNET_HOST_HANDLE_DATA
{
    DOTNET_HOST_HANDLE_DATA()
    {

    };

    BROKER_HANDLE               broker;

    variant_t                   vtClientModuleObject;
    CComPtr<ICLRMetaHost>       spMetaHost;
    CComPtr<ICLRRuntimeInfo>    spRuntimeInfo;
    CComPtr<ICorRuntimeHost>    spCorRuntimeHost;
    _TypePtr                    spClientModuleType;
    _AssemblyPtr                spAzureIoTGatewayAssembly;

private:
    DOTNET_HOST_HANDLE_DATA& operator=(const DOTNET_HOST_HANDLE_DATA&);
};

bool buildBrokerObject(long long  brokerHandle, long long  moduleHandle, _AssemblyPtr spAzureIoTGatewayAssembly, variant_t* vtAzureIoTGatewayBrokerObject)
{
    SAFEARRAY *psaAzureIoTGatewayBrokerConstructorArgs = NULL;
    bool returnResult = false;

    try
    {
        LONG index = 0;
        HRESULT hr;
        variant_t broker(brokerHandle);
        variant_t module(moduleHandle);
        bstr_t bstrAzureIoTGatewayBrokerClassName(AZUREIOTGATEWAY_BROKER_CLASSNAME);

        psaAzureIoTGatewayBrokerConstructorArgs = SafeArrayCreateVector(VT_VARIANT, 0, 2);
        if (psaAzureIoTGatewayBrokerConstructorArgs == NULL)
        {
            /* Codes_SRS_DOTNET_04_006: [ DotNet_Create shall return NULL if an underlying API call fails. ] */
            LogError("Failed to create Safe Array. ");
        }
        else if (FAILED(hr = SafeArrayPutElement(psaAzureIoTGatewayBrokerConstructorArgs, &index, &broker)))
        {
            /* Codes_SRS_DOTNET_04_006: [ DotNet_Create shall return NULL if an underlying API call fails. ] */
            LogError("Adding Element on the safe array failed. w/hr 0x%08lx\n", hr);
        }
        else
        {
            index = 1;
            if (FAILED(hr = SafeArrayPutElement(psaAzureIoTGatewayBrokerConstructorArgs, &index, &module)))
            {
                /* Codes_SRS_DOTNET_04_006: [ DotNet_Create shall return NULL if an underlying API call fails. ] */
                LogError("Adding Element on the safe array failed. w/hr 0x%08lx\n", hr);
            }
            /* Codes_SRS_DOTNET_04_013: [ A .NET Object conforming to the Broker interface defined shall be created: ] */
            else if (FAILED(hr = spAzureIoTGatewayAssembly->CreateInstance_3(
                bstrAzureIoTGatewayBrokerClassName,
                true,
                static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public),
                NULL,
                psaAzureIoTGatewayBrokerConstructorArgs,
                NULL,
                NULL,
                vtAzureIoTGatewayBrokerObject)))
            {
                /* Codes_SRS_DOTNET_04_006: [ DotNet_Create shall return NULL if an underlying API call fails. ] */
                LogError("Creating an instance of the message broker failed with hr 0x%08lx\n", hr);
            }
            else
            {
                returnResult = true;
            }
        }
    }
    catch (const _com_error& e)
    {
        LogError("Exception Thrown. Message: %s ", e.ErrorMessage());
    }

    if (psaAzureIoTGatewayBrokerConstructorArgs)
    {
        SafeArrayDestroy(psaAzureIoTGatewayBrokerConstructorArgs);
    }

    return returnResult;
}

bool createCLRInstancesGetInterfacesAndStarting(ICLRMetaHost** pMetaHost, ICLRRuntimeInfo** pRuntimeInfo, ICorRuntimeHost** pCorRuntimeHost, _AppDomain** pDefaultAppDomain)
{
    bool returnResult = false;
    HRESULT hr;
    BOOL fLoadable;
    IUnknownPtr spAppDomainThunk;
                

    if (FAILED(hr = CLRCreateInstance(CLSID_CLRMetaHost, IID_PPV_ARGS(pMetaHost))))
    {
        /* Codes_SRS_DOTNET_04_006: [ DotNet_Create shall return NULL if an underlying API call fails. ] */
        LogError("CLRCreateInstance failed w/hr 0x%08lx\n", hr);
    }
    else if (FAILED(hr = (*pMetaHost)->GetRuntime(DEFAULT_CLR_VERSION, IID_PPV_ARGS(pRuntimeInfo))))
    {
        /* Codes_SRS_DOTNET_04_006: [ DotNet_Create shall return NULL if an underlying API call fails. ] */
        LogError("ICLRMetaHost::GetRuntime failed w/hr 0x%08lx", hr);
    }
    else if (FAILED(hr = (*pRuntimeInfo)->IsLoadable(&fLoadable)))
    {
        /* Codes_SRS_DOTNET_04_006: [ DotNet_Create shall return NULL if an underlying API call fails. ] */
        LogError("ICLRRuntimeInfo::IsLoadable failed w/hr 0x%08lx\n", hr);
    }
    else if (!fLoadable)
    {
        /* Codes_SRS_DOTNET_04_006: [ DotNet_Create shall return NULL if an underlying API call fails. ] */
        LogError(".NET runtime %ls cannot be loaded\n", DEFAULT_CLR_VERSION);
    }
    else if (FAILED(hr = (*pRuntimeInfo)->GetInterface(CLSID_CorRuntimeHost, IID_PPV_ARGS(pCorRuntimeHost))))
    {
        /* Codes_SRS_DOTNET_04_006: [ DotNet_Create shall return NULL if an underlying API call fails. ] */
        LogError("ICLRRuntimeInfo::GetInterface failed w/hr 0x%08lx\n", hr);
    }
    else if (FAILED(hr = (*pCorRuntimeHost)->Start()))
    {
        /* Codes_SRS_DOTNET_04_006: [ DotNet_Create shall return NULL if an underlying API call fails. ] */
        LogError("CLR failed to start w/hr 0x%08lx\n", hr);
    }
    else if (FAILED(hr = (*pCorRuntimeHost)->GetDefaultDomain(&spAppDomainThunk)))
    {
        /* Codes_SRS_DOTNET_04_006: [ DotNet_Create shall return NULL if an underlying API call fails. ] */
        LogError("ICorRuntimeHost::GetDefaultDomain failed w/hr 0x%08lx\n", hr);
    }
    else if (FAILED(hr = spAppDomainThunk->QueryInterface(IID_PPV_ARGS(pDefaultAppDomain))))
    {
        /* Codes_SRS_DOTNET_04_006: [ DotNet_Create shall return NULL if an underlying API call fails. ] */
        LogError("Failed to get default AppDomain w/hr 0x%08lx\n", hr);
    }
    else
    {
        returnResult = true;
    }

    return returnResult;
}

bool invokeCreateMethodFromClient(const char* module_args, variant_t* vtAzureIoTGatewayBrokerObject, _Type* pClientModuleType, variant_t* vtClientModuleObject)
{
    SAFEARRAY *psaClientModuleCreateArgs = NULL;
    bool returnResult = false;

    try
    {
        SAFEARRAYBOUND rgsabound[1];
        HRESULT hrResult;
        variant_t configurationInByteArray;
        variant_t vt_Empty;
        V_VT(&configurationInByteArray) = VT_ARRAY | VT_UI1;
        void * pArrayData = NULL;

        if (module_args == NULL)
        {
            LogError("Missing DotNetModule Configuration.");
        }
        else
        {
            int32_t msg_size = strlen(module_args);
            if (msg_size < 0)
            {
                LogError("Error getting size of module configuration.");
            }
            else
            {
                rgsabound[0].cElements = msg_size;
                rgsabound[0].lLbound = 0;

                if ((V_ARRAY(&configurationInByteArray) = SafeArrayCreate(VT_UI1, 1, rgsabound)) == NULL)
                {
                    LogError("Error creating SafeArray.");
                }
                else if (FAILED(hrResult = SafeArrayAccessData(configurationInByteArray.parray, &pArrayData)))
                {
                    LogError("Error Acessing Safe Array Data. w/hr 0x%08lx\n", hrResult);
                }
                else
                {
                    memcpy(pArrayData, module_args, msg_size);
                    LONG index = 0;

                    bstr_t bstrCreateClientMethodName(L"Create");

                    if (FAILED(hrResult = SafeArrayUnaccessData(configurationInByteArray.parray)))
                    {
                        LogError("Error on call for SafeArrayUnaccessData.w/hr 0x%08lx\n", hrResult);
                    }
                    else if ((psaClientModuleCreateArgs = SafeArrayCreateVector(VT_VARIANT, 0, 2)) == NULL)
                    {
                        LogError("Error building SafeArray Vector for arguments.");
                    }
                    else if (FAILED(hrResult = SafeArrayPutElement(psaClientModuleCreateArgs, &index, vtAzureIoTGatewayBrokerObject)))
                    {
                        LogError("Adding Element on the safe array failed. w/hr 0x%08lx\n", hrResult);
                    }
                    else
                    {
                        index = 1;
                        if (FAILED(hrResult = SafeArrayPutElement(psaClientModuleCreateArgs, &index, &configurationInByteArray)))
                        {
                            LogError("Error Adding Element to the Arguments Safe Array.w/hr 0x%08lx\n", hrResult);
                        }
                        /*Codes_SRS_DOTNET_04_014: [ DotNet_Create shall call Create C# method, implemented from IGatewayModule, passing the Broker object created and configuration->module_args. ] */
                        else if (FAILED(hrResult = pClientModuleType->InvokeMember_3(
                            bstrCreateClientMethodName,
                            static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public | BindingFlags_InvokeMethod),
                            NULL,
                            *vtClientModuleObject,
                            psaClientModuleCreateArgs,
                            &vt_Empty)))
                        {
                            /* Codes_SRS_DOTNET_04_006: [ DotNet_Create shall return NULL if an underlying API call fails. ] */
                            LogError("Failed to invoke Create Method with hr 0x%08lx\n", hrResult);
                        }
                        else
                        {
                            returnResult = true;
                        }
                    }
                }
            }
        }
    }
    catch (const _com_error& e)
    {
        LogError("Exception Thrown. Message: %s ", e.ErrorMessage());
    }

    if (psaClientModuleCreateArgs)
    {
        SafeArrayDestroy(psaClientModuleCreateArgs);
    }

    return returnResult;
}

static MODULE_HANDLE DotNet_Create(BROKER_HANDLE broker, const void* configuration)
{
    DOTNET_HOST_HANDLE_DATA* out = NULL;

    try
    {
        if (
            (broker == NULL) ||
            (configuration == NULL)
            )
        {
            /* Codes_SRS_DOTNET_04_001: [ DotNet_Create shall return NULL if broker is NULL. ] */
            /* Codes_SRS_DOTNET_04_002: [ DotNet_Create shall return NULL if configuration is NULL. ] */
            LogError("invalid arg broker=%p, configuration=%p", broker, configuration);
        }
        else
        {
            DOTNET_HOST_CONFIG* dotNetConfig = (DOTNET_HOST_CONFIG*)configuration;
            if (dotNetConfig->assembly_name == NULL)
            {
                /* Codes_SRS_DOTNET_04_003: [ DotNet_Create shall return NULL if configuration->assembly_name is NULL. ] */
                LogError("invalid configuration. assembly_name=%p", dotNetConfig->assembly_name);
            }
            else if (dotNetConfig->entry_type == NULL)
            {
                /* Codes_SRS_DOTNET_04_004: [ DotNet_Create shall return NULL if configuration->entry_type is NULL. ] */
                LogError("invalid configuration. entry_type=%p", dotNetConfig->entry_type);
            }
            else if (dotNetConfig->module_args == NULL)
            {
                /* Codes_SRS_DOTNET_04_005: [ DotNet_Create shall return NULL if configuration->module_args is NULL. ] */
                LogError("invalid configuration. module_args=%p", dotNetConfig->module_args);
            }
            else
            {
                /* Codes_SRS_DOTNET_04_006: [ DotNet_Create shall return NULL if an underlying API call fails. ] */
                HRESULT hr;
                bstr_t bstrClientModuleAssemblyName(dotNetConfig->assembly_name);
                _AppDomainPtr spDefaultAppDomain;
                _AssemblyPtr spClientModuleAssembly;
                bstr_t bstrClientModuleClassName(dotNetConfig->entry_type);
                bstr_t bstrAzureIoTGatewayAssemblyName(AZUREIOTGATEWAYASSEMBLYNAME);
                variant_t vtAzureIoTGatewayBrokerObject;
                try
                {
                    /* Codes_SRS_DOTNET_04_008: [ DotNet_Create shall allocate memory for an instance of the DOTNET_HOST_HANDLE_DATA structure and use that as the backing structure for the module handle. ] */
                    out = new DOTNET_HOST_HANDLE_DATA();
                }
                catch (std::bad_alloc)
                {
                    //Do not need to do anything, since we are returning NULL below.
                    LogError("Memory allocation failed for DOTNET_HOST_HANDLE_DATA.");
                }

                if (out != NULL)
                {
                    /* Codes_SRS_DOTNET_04_007: [ DotNet_Create shall return a non-NULL MODULE_HANDLE when successful. ] */
                    out->broker = broker;

                    /* Codes_SRS_DOTNET_04_012: [ DotNet_Create shall get the 3 CLR Host Interfaces (CLRMetaHost, CLRRuntimeInfo and CorRuntimeHost) and save it on DOTNET_HOST_HANDLE_DATA. ] */
                    /* Codes_SRS_DOTNET_04_010: [ DotNet_Create shall save Client module Type and Azure IoT Gateway Assembly on DOTNET_HOST_HANDLE_DATA. ] */
                    if (!createCLRInstancesGetInterfacesAndStarting(&out->spMetaHost, &out->spRuntimeInfo, &out->spCorRuntimeHost, &spDefaultAppDomain))
                    {
                        /* Codes_SRS_DOTNET_04_006: [ DotNet_Create shall return NULL if an underlying API call fails. ] */
                        LogError("Error creating CLR Intance, Getting Interfaces and Starting it.");
                        delete out; 
                        out = NULL;
                    }
                    else if (FAILED(hr = spDefaultAppDomain->Load_2(bstrClientModuleAssemblyName, &spClientModuleAssembly)))
                    {
                        /* Codes_SRS_DOTNET_04_006: [ DotNet_Create shall return NULL if an underlying API call fails. ] */
                        LogError("Failed to load the assembly w/hr 0x%08lx\n", hr);
                        delete out;
                        out = NULL;
                    }
                    /* Codes_SRS_DOTNET_04_010: [ DotNet_Create shall save Client module Type and Azure IoT Gateway Assembly on DOTNET_HOST_HANDLE_DATA. ] */
                    else if (FAILED(hr = spClientModuleAssembly->GetType_2(bstrClientModuleClassName, &out->spClientModuleType)))
                    {
                        /* Codes_SRS_DOTNET_04_006: [ DotNet_Create shall return NULL if an underlying API call fails. ] */
                        LogError("Failed to get the Type interface w/hr 0x%08lx\n", hr);
                        delete out;
                        out = NULL;
                    }
                    else if (FAILED(hr = spDefaultAppDomain->Load_2(bstrAzureIoTGatewayAssemblyName, &(out->spAzureIoTGatewayAssembly))))
                    {
                        /* Codes_SRS_DOTNET_04_006: [ DotNet_Create shall return NULL if an underlying API call fails. ] */
                        LogError("Failed to load the assembly w/hr 0x%08lx\n", hr);
                        delete out;
                        out = NULL;
                    }
                    else if (!buildBrokerObject((long long)out->broker, (long long)out, out->spAzureIoTGatewayAssembly, &vtAzureIoTGatewayBrokerObject))
                    {
                        LogError("Failed to build the message broker object.");
                        delete out;
                        out = NULL;
                    }
                    /* Codes_SRS_DOTNET_04_009: [ DotNet_Create shall create an instance of .NET client Module and save it on DOTNET_HOST_HANDLE_DATA. ] */
                    /* Codes_SRS_DOTNET_04_010: [ DotNet_Create shall save Client module Type and Azure IoT Gateway Assembly on DOTNET_HOST_HANDLE_DATA. ] */
                    else if (FAILED(hr = spClientModuleAssembly->CreateInstance(bstrClientModuleClassName, &out->vtClientModuleObject)))
                    {
                        /* Codes_SRS_DOTNET_04_006: [ DotNet_Create shall return NULL if an underlying API call fails. ] */
                        LogError("Creating an instance of Client Class failed with hr 0x%08lx\n", hr);
                        delete out;
                        out = NULL;
                    }
                    else if (!invokeCreateMethodFromClient(dotNetConfig->module_args, &vtAzureIoTGatewayBrokerObject, out->spClientModuleType, &out->vtClientModuleObject))
                    {
                        /* Codes_SRS_DOTNET_04_006: [ DotNet_Create shall return NULL if an underlying API call fails. ] */
                        LogError("Failed to invoke Create Method");
                        delete out;
                        out = NULL;
                    }
                }
            }
        }
    }
    catch (const _com_error& e)
    {
        LogError("Exception Thrown. Message: %s ", e.ErrorMessage());
    }

    return out;
}

static void* DotNet_ParseConfigurationFromJson(const char* configuration)
{
    return (void*)configuration;
}

bool buildMessageObject(MESSAGE_HANDLE messageHandle, _AssemblyPtr spAzureIoTGatewayAssembly, variant_t* vtAzureIoTGatewayMessageObject)
{
    SAFEARRAY *psaAzureIoTGatewayMessageConstructorArgs = NULL;
    bool returnResult = false;
    int32_t msg_size;

    try
    {
        msg_size = Message_ToByteArray(messageHandle, NULL, 0);
        SAFEARRAYBOUND rgsabound[1];

        if (msg_size > 0)
        {
			auto msgByteArray = std::make_unique<unsigned char[]>(msg_size);
			if (msgByteArray)
			{
				Message_ToByteArray(messageHandle, msgByteArray.get(), msg_size);
				rgsabound[0].cElements = msg_size;
				rgsabound[0].lLbound = 0;

				variant_t msgContentInByteArray;
				V_VT(&msgContentInByteArray) = VT_ARRAY | VT_UI1;

				HRESULT hrResult;
				void * pArrayData = NULL;

				if ((V_ARRAY(&msgContentInByteArray) = SafeArrayCreate(VT_UI1, 1, rgsabound)) == NULL)
				{
					LogError("Error creating SafeArray.");
				}
				else if (FAILED(hrResult = SafeArrayAccessData(msgContentInByteArray.parray, &pArrayData)))
				{
					LogError("Error Acessing Safe Array Data. w/hr 0x%08lx\n", hrResult);
				}
				else
				{
					memcpy(pArrayData, msgByteArray.get(), msg_size);
					LONG index = 0;
					bstr_t bstrAzureIoTGatewayMessageClassName(AZUREIOTGATEWAY_MESSAGE_CLASSNAME);

					if (FAILED(hrResult = SafeArrayUnaccessData(msgContentInByteArray.parray)))
					{
						LogError("Error on call for SafeArrayUnaccessData.w/hr 0x%08lx\n", hrResult);
					}
					else if ((psaAzureIoTGatewayMessageConstructorArgs = SafeArrayCreateVector(VT_VARIANT, 0, 1)) == NULL)
					{
						LogError("Error building SafeArray Vector for arguments.");
					}
					else if (FAILED(hrResult = SafeArrayPutElement(psaAzureIoTGatewayMessageConstructorArgs, &index, &msgContentInByteArray)))
					{
						LogError("Error Adding Element to the Arguments Safe Array.w/hr 0x%08lx\n", hrResult);
					}
					/* Codes_SRS_DOTNET_04_017: [ DotNet_Receive shall construct an instance of the Message interface as defined below: ] */
					else if (FAILED(hrResult = spAzureIoTGatewayAssembly->CreateInstance_3
					(
						bstrAzureIoTGatewayMessageClassName,
						true,
						static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public),
						NULL,
						psaAzureIoTGatewayMessageConstructorArgs,
						NULL, NULL,
						vtAzureIoTGatewayMessageObject)))
					{
						LogError("Error creating message instance.w/hr 0x%08lx\n", hrResult);
					}
					else
					{
						returnResult = true;
					}
				}
			}
		}
    }
	catch (const std::bad_alloc & msgErr)
	{
		LogError("Exception Thrown. Allocate message failed");
	}
    catch (const _com_error& e)
    {
        LogError("Exception Thrown. Message: %s ", e.ErrorMessage());
    }

    if (psaAzureIoTGatewayMessageConstructorArgs)
    {
        SafeArrayDestroy(psaAzureIoTGatewayMessageConstructorArgs);
    }
    return returnResult;
}


void DotNet_FreeConfiguration(void* configuration)
{
    //Nothing to be freed here.
}

static void DotNet_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    SAFEARRAY *psaAzureIoTGatewayClientReceiveArgs = NULL;
    try
    {
        if (
            (moduleHandle != NULL) &&
            (messageHandle != NULL)
            )
        {
            DOTNET_HOST_HANDLE_DATA* result = (DOTNET_HOST_HANDLE_DATA*)moduleHandle;
            variant_t vtAzureIoTGatewayMessageObject;
            bstr_t bstrReceiveClientMethodName(L"Receive");
            HRESULT hrResult;
            variant_t vt_Empty;
            LONG index = 0;

            /* Codes_SRS_DOTNET_04_017: [ DotNet_Receive shall construct an instance of the Message interface as defined below: ] */
            if (!buildMessageObject(messageHandle, result->spAzureIoTGatewayAssembly, &vtAzureIoTGatewayMessageObject))
            {
                LogError("Error building Message Object.");
            }
            else if ((psaAzureIoTGatewayClientReceiveArgs = SafeArrayCreateVector(VT_VARIANT, 0, 1)) == NULL)
            {
                LogError("Error creating arguments Safe Array for Receive.");
            }
            else if (FAILED(hrResult = SafeArrayPutElement(
                psaAzureIoTGatewayClientReceiveArgs,
                &index,
                &vtAzureIoTGatewayMessageObject)))
            {
                LogError("Error Adding Argument to the Safe Array.w/hr 0x%08lx\n", hrResult);
            }
            /* Codes_SRS_DOTNET_04_018: [ DotNet_Create shall call Receive C# method passing the Message object created with the content of message serialized into Message object. ] */
            else if (FAILED(hrResult = result->spClientModuleType->InvokeMember_3
            (
                bstrReceiveClientMethodName,
                static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public | BindingFlags_InvokeMethod),
                NULL,
                result->vtClientModuleObject,
                psaAzureIoTGatewayClientReceiveArgs,
                &vt_Empty
            )))
            {
                LogError("Failed to Invoke Receive method. w/hr 0x%08lx\n", hrResult);
            }
        }
        else
        {
            /* Codes_SRS_DOTNET_04_015: [ DotNet_Receive shall do nothing if module is NULL. ] */
            /* Codes_SRS_DOTNET_04_016: [ DotNet_Receive shall do nothing if message is NULL. ] */
            LogError("invalid arg moduleHandle=%p, messageHandle=%p", moduleHandle, messageHandle);
            /*do nothing*/
        }
    }
    catch (const _com_error& e)
    {
        LogError("Exception Thrown. Message: %s ", e.ErrorMessage());
    }

    //Clean Up SafeArray
    if (psaAzureIoTGatewayClientReceiveArgs)
    {
        SafeArrayDestroy(psaAzureIoTGatewayClientReceiveArgs);
    }
}


static void DotNet_Destroy(MODULE_HANDLE module)
{
    /* Codes_SRS_DOTNET_04_019: [ DotNet_Destroy shall do nothing if module is NULL. ] */
    if (module != NULL)
    {
        DOTNET_HOST_HANDLE_DATA* handleData = (DOTNET_HOST_HANDLE_DATA*)module;
        /* Codes_SRS_DOTNET_04_022: [ DotNet_Destroy shall call the Destroy C# method. ] */
        try
        {
            bstr_t bstrDestroyClientMethodName(L"Destroy");
            variant_t vt_Empty;
            HRESULT hrResult;
            if (FAILED(hrResult = handleData->spClientModuleType->InvokeMember_3
            (
                bstrDestroyClientMethodName,
                static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public | BindingFlags_InvokeMethod),
                NULL,
                handleData->vtClientModuleObject,
                NULL,
                &vt_Empty
            )))
            {
                LogError("Failed to Invoke Destroy method. w/hr 0x%08lx\n", hrResult);
            }
        }
        catch (const _com_error& e)
        {
            LogError("Exception Thrown. Message: %s ", e.ErrorMessage());
        };
        /* Codes_SRS_DOTNET_04_020: [ DotNet_Destroy shall free all resources associated with the given module.. ] */
        delete(handleData);
    }
}

MODULE_EXPORT bool Module_DotNetHost_PublishMessage(BROKER_HANDLE broker, MODULE_HANDLE sourceModule, const unsigned char* message, int32_t size)
{
    bool returnValue = false;
    MESSAGE_HANDLE messageToPublish = NULL;

    if (broker == NULL || sourceModule == NULL || message == NULL || size < 0)
    {
        /* Codes_SRS_DOTNET_04_022: [ Module_DotNetHost_PublishMessage shall return false if broker is NULL. ] */
        /* Codes_SRS_DOTNET_04_029: [ Module_DotNetHost_PublishMessage shall return false if sourceModule is NULL. ] */
        /* Codes_SRS_DOTNET_04_023: [ Module_DotNetHost_PublishMessage shall return false if message is NULL, or size if lower than 0. ] */
        LogError("invalid arg broker=%p, sourceModule=%p", broker, sourceModule);
    }
    /* Codes_SRS_DOTNET_04_024: [ Module_DotNetHost_PublishMessage shall create a message from message and size by invoking Message_CreateFromByteArray. ] */
    else if((messageToPublish = Message_CreateFromByteArray(message, size)) == NULL)
    {
        /* Codes_SRS_DOTNET_04_025: [ If Message_CreateFromByteArray fails, Module_DotNetHost_PublishMessage shall fail. ] */
        LogError("Error trying to create message from Byte Array");
    }
    /* Codes_SRS_DOTNET_04_026: [ Module_DotNetHost_PublishMessage shall call Broker_Publish passing broker, sourceModule, message and size. ] */
    else if (Broker_Publish(broker, sourceModule, messageToPublish) != BROKER_OK)
    {
        /* Codes_SRS_DOTNET_04_027: [ If Broker_Publish fails Module_DotNetHost_PublishMessage shall fail. ] */
        LogError("Error trying to publish message on Broker.");
    }
    else
    {
        /* Codes_SRS_DOTNET_04_028: [If Broker_Publish succeeds Module_DotNetHost_PublishMessage shall succeed.] */
        returnValue = true;
    }

    if (messageToPublish != NULL)
    {
        Message_Destroy(messageToPublish);
    }

    return returnValue;
}

static void DotNet_Start(MODULE_HANDLE module)
{
    /*Codes_SRS_DOTNET_17_001: [ DotNet_Start shall do nothing if module is NULL. ]*/
    if (module != NULL)
    {
        DOTNET_HOST_HANDLE_DATA* handleData = (DOTNET_HOST_HANDLE_DATA*)module;
        try
        {
            bstr_t bstrStartClientIFName(L"IGatewayModuleStart");
            _Type * if_type = NULL;
            HRESULT hrResult;
            
            /*Codes_SRS_DOTNET_17_002: [ DotNet_Start shall attempt to get the "IGatewayModuleStart" type interface. ]*/
            if (FAILED(hrResult = handleData->spClientModuleType->GetInterface(bstrStartClientIFName, VARIANT_TRUE, &if_type)))
            {
                LogError("Failed to discover type");
            }
            else
            {
                if (if_type)
                {
                    bstr_t bstrStartClientMethodName(L"Start");
                    variant_t vt_Empty;

                    /*Codes_SRS_DOTNET_17_003: [ If the "IGatewayModuleStart" type interface exists, DotNet_Start shall call theStart C# method. ]*/
                    if (FAILED(hrResult = handleData->spClientModuleType->InvokeMember_3
                    (
                        bstrStartClientMethodName,
                        static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public | BindingFlags_InvokeMethod),
                        NULL,
                        handleData->vtClientModuleObject,
                        NULL,
                        &vt_Empty
                    )))
                    {
                        LogError("Failed to Invoke Start method. w/hr 0x%08lx\n", hrResult);
                    }
                }
            }    
        }
        catch (const _com_error& e)
        {
            LogError("Exception Thrown. Message: %s ", e.ErrorMessage());
        };
    }
}

static const MODULE_API_1 DOTNET_APIS_all =
{
    {MODULE_API_VERSION_1},
    DotNet_ParseConfigurationFromJson,
    DotNet_FreeConfiguration,
    DotNet_Create,
    DotNet_Destroy,
    DotNet_Receive,
    DotNet_Start
};

/*Codes_SRS_DOTNET_26_001: [ Module_GetApi shall return out the provided MODULES_API structure with required module's APIs functions. ]*/
#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(DOTNET_HOST)(MODULE_API_VERSION gateway_api_version)
#else
MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version)
#endif
{
    (void)gateway_api_version;
    return (const MODULE_API *)&DOTNET_APIS_all;
}
