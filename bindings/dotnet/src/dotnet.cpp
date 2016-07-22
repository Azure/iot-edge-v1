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

#include <metahost.h>

#include <atlbase.h>


#define DEFAULT_CLR_VERSION L"v4.0.30319"
#define AZUREIOTGATEWAYASSEMBLYNAME L"Microsoft.Azure.IoT.Gateway"
#define AZUREIOTGATEWAY_MESSAGEBUS_CLASSNAME L"Microsoft.Azure.IoT.Gateway.MessageBus"
#define AZUREIOTGATEWAY_MESSAGE_CLASSNAME L"Microsoft.Azure.IoT.Gateway.Message"




// Import mscorlib.tlb (Microsoft Common Language Runtime Class Library).
#import "mscorlib.tlb" raw_interfaces_only				\
    high_property_prefixes("_get","_put","_putref")		\
    rename("ReportEvent", "InteropServices_ReportEvent")
using namespace mscorlib;

struct DOTNET_HOST_HANDLE_DATA
{
	DOTNET_HOST_HANDLE_DATA()
	{

	};

    MESSAGE_BUS_HANDLE          bus;

	variant_t                   vtClientModuleObject;
	CComPtr<ICLRMetaHost>       spMetaHost;
	CComPtr<ICLRRuntimeInfo>    spRuntimeInfo;
	CComPtr<ICorRuntimeHost>    spCorRuntimeHost;
	_TypePtr                    spClientModuleType;
	_AssemblyPtr                spAzureIoTGatewayAssembly;

private:
	DOTNET_HOST_HANDLE_DATA& operator=(const DOTNET_HOST_HANDLE_DATA&);
};

bool buildMessageBusObject(long long  busHandle, long long  moduleHandle, _AssemblyPtr spAzureIoTGatewayAssembly, variant_t* vtAzureIoTGatewayMessageBusObject)
{
	SAFEARRAY *psaAzureIoTGatewayMessageBusConstructorArgs = NULL;
	bool returnResult = false;

	try
	{
		LONG index = 0;
		HRESULT hr;
		variant_t msgBus(busHandle);
		variant_t module(moduleHandle);
		bstr_t bstrAzureIoTGatewayMessageBusClassName(AZUREIOTGATEWAY_MESSAGEBUS_CLASSNAME);

		psaAzureIoTGatewayMessageBusConstructorArgs = SafeArrayCreateVector(VT_VARIANT, 0, 2);
		if (psaAzureIoTGatewayMessageBusConstructorArgs == NULL)
		{
			/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
			LogError("Failed to create Safe Array. ");
		}
		else if (FAILED(hr = SafeArrayPutElement(psaAzureIoTGatewayMessageBusConstructorArgs, &index, &msgBus)))
		{
			/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
			LogError("Adding Element on the safe array failed. w/hr 0x%08lx\n", hr);
		}
		else
		{
			index = 1;
			if (FAILED(hr = SafeArrayPutElement(psaAzureIoTGatewayMessageBusConstructorArgs, &index, &module)))
			{
				/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
				LogError("Adding Element on the safe array failed. w/hr 0x%08lx\n", hr);
			}
			/* Codes_SRS_DOTNET_04_013: [ A .NET Object conforming to the MessageBus interface defined shall be created: ] */
			else if (FAILED(hr = spAzureIoTGatewayAssembly->CreateInstance_3(
				bstrAzureIoTGatewayMessageBusClassName,
				true,
				static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public),
				NULL,
				psaAzureIoTGatewayMessageBusConstructorArgs,
				NULL,
				NULL,
				vtAzureIoTGatewayMessageBusObject)))
			{
				/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
				LogError("Creating an instance of Message Bus failed with hr 0x%08lx\n", hr);
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

	if (psaAzureIoTGatewayMessageBusConstructorArgs)
	{
		SafeArrayDestroy(psaAzureIoTGatewayMessageBusConstructorArgs);
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
		/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
		LogError("CLRCreateInstance failed w/hr 0x%08lx\n", hr);
	}
	else if (FAILED(hr = (*pMetaHost)->GetRuntime(DEFAULT_CLR_VERSION, IID_PPV_ARGS(pRuntimeInfo))))
	{
		/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
		LogError("ICLRMetaHost::GetRuntime failed w/hr 0x%08lx", hr);
	}
	else if (FAILED(hr = (*pRuntimeInfo)->IsLoadable(&fLoadable)))
	{
		/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
		LogError("ICLRRuntimeInfo::IsLoadable failed w/hr 0x%08lx\n", hr);
	}
	else if (!fLoadable)
	{
		/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
		LogError(".NET runtime %ls cannot be loaded\n", DEFAULT_CLR_VERSION);
	}
	else if (FAILED(hr = (*pRuntimeInfo)->GetInterface(CLSID_CorRuntimeHost, IID_PPV_ARGS(pCorRuntimeHost))))
	{
		/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
		LogError("ICLRRuntimeInfo::GetInterface failed w/hr 0x%08lx\n", hr);
	}
	else if (FAILED(hr = (*pCorRuntimeHost)->Start()))
	{
		/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
		LogError("CLR failed to start w/hr 0x%08lx\n", hr);
	}
	else if (FAILED(hr = (*pCorRuntimeHost)->GetDefaultDomain(&spAppDomainThunk)))
	{
		/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
		LogError("ICorRuntimeHost::GetDefaultDomain failed w/hr 0x%08lx\n", hr);
	}
	else if (FAILED(hr = spAppDomainThunk->QueryInterface(IID_PPV_ARGS(pDefaultAppDomain))))
	{
		/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
		LogError("Failed to get default AppDomain w/hr 0x%08lx\n", hr);
	}
	else
	{
		returnResult = true;
	}

	return returnResult;
}

bool invokeCreateMethodFromClient(const char* dotnet_module_args, variant_t* vtAzureIoTGatewayMessageBusObject, _Type* pClientModuleType, variant_t* vtClientModuleObject)
{
	SAFEARRAY *psaClientModuleCreateArgs = NULL;
	bool returnResult = false;

	try
	{
		variant_t vtdotNetArgsArg(dotnet_module_args);
		bstr_t bstrCreateClientMethodName(L"Create");
		variant_t vt_Empty;
		long index = 0;
		HRESULT hr;

		if ((psaClientModuleCreateArgs = SafeArrayCreateVector(VT_VARIANT, 0, 2)) == NULL)
		{
			/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
			LogError("Failed to create Safe Array. ");
		}
		else if (FAILED(hr = SafeArrayPutElement(psaClientModuleCreateArgs, &index, vtAzureIoTGatewayMessageBusObject)))
		{
			/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
			LogError("Adding Element on the safe array failed. w/hr 0x%08lx\n", hr);
		}
		else
		{
			index = 1;
			if (FAILED(hr = SafeArrayPutElement(psaClientModuleCreateArgs, &index, &vtdotNetArgsArg)))
			{
				/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
				LogError("Adding Element on the safe array failed. w/hr 0x%08lx\n", hr);
			}
			/* Codes_SRS_DOTNET_04_014: [ DotNET_Create shall call Create C# method, implemented from IGatewayModule, passing the MessageBus object created and configuration->dotnet_module_args. ] */
			else if (FAILED(hr = pClientModuleType->InvokeMember_3(
				bstrCreateClientMethodName,
				static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public | BindingFlags_InvokeMethod),
				NULL, 
				*vtClientModuleObject,
				psaClientModuleCreateArgs,
				&vt_Empty)))
			{
				/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
				LogError("Failed to invoke Create Method with hr 0x%08lx\n", hr);
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

	if (psaClientModuleCreateArgs)
	{
		SafeArrayDestroy(psaClientModuleCreateArgs);
	}

	return returnResult;
}

static MODULE_HANDLE DotNET_Create(MESSAGE_BUS_HANDLE busHandle, const void* configuration)
{
	DOTNET_HOST_HANDLE_DATA* out = NULL;

	try
	{
		if (
			(busHandle == NULL) ||
			(configuration == NULL)
			)
		{
			/* Codes_SRS_DOTNET_04_001: [ DotNET_Create shall return NULL if bus is NULL. ] */
			/* Codes_SRS_DOTNET_04_002: [ DotNET_Create shall return NULL if configuration is NULL. ] */
			LogError("invalid arg busHandle=%p, configuration=%p", busHandle, configuration);
		}
		else
		{
			DOTNET_HOST_CONFIG* dotNetConfig = (DOTNET_HOST_CONFIG*)configuration;
			if (dotNetConfig->dotnet_module_path == NULL)
			{
				/* Codes_SRS_DOTNET_04_003: [ DotNET_Create shall return NULL if configuration->dotnet_module_path is NULL. ] */
				LogError("invalid configuration. dotnet_module_path=%p", dotNetConfig->dotnet_module_path);
			}
			else if (dotNetConfig->dotnet_module_entry_class == NULL)
			{
				/* Codes_SRS_DOTNET_04_004: [ DotNET_Create shall return NULL if configuration->dotnet_module_entry_class is NULL. ] */
				LogError("invalid configuration. dotnet_module_entry_class=%p", dotNetConfig->dotnet_module_entry_class);
			}
			else if (dotNetConfig->dotnet_module_args == NULL)
			{
				/* Codes_SRS_DOTNET_04_005: [ DotNET_Create shall return NULL if configuration->dotnet_module_args is NULL. ] */
				LogError("invalid configuration. dotnet_module_args=%p", dotNetConfig->dotnet_module_args);
			}
			else
			{
				/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
				HRESULT hr;
				bstr_t bstrClientModuleAssemblyName(dotNetConfig->dotnet_module_path);
				_AppDomainPtr spDefaultAppDomain;
				_AssemblyPtr spClientModuleAssembly;
				bstr_t bstrClientModuleClassName(dotNetConfig->dotnet_module_entry_class);
				bstr_t bstrAzureIoTGatewayAssemblyName(AZUREIOTGATEWAYASSEMBLYNAME);
				variant_t vtAzureIoTGatewayMessageBusObject;
				try
				{
					/* Codes_SRS_DOTNET_04_008: [ DotNET_Create shall allocate memory for an instance of the DOTNET_HOST_HANDLE_DATA structure and use that as the backing structure for the module handle. ] */
					out = new DOTNET_HOST_HANDLE_DATA();
				}
				catch (std::bad_alloc)
				{
					//Do not need to do anything, since we are returning NULL below.
					LogError("Memory allocation failed for DOTNET_HOST_HANDLE_DATA.");
				}

				if (out != NULL)
				{
					/* Codes_SRS_DOTNET_04_007: [ DotNET_Create shall return a non-NULL MODULE_HANDLE when successful. ] */
					out->bus = busHandle;

					/* Codes_SRS_DOTNET_04_012: [ DotNET_Create shall get the 3 CLR Host Interfaces (CLRMetaHost, CLRRuntimeInfo and CorRuntimeHost) and save it on DOTNET_HOST_HANDLE_DATA. ] */
					/* Codes_SRS_DOTNET_04_010: [ DotNET_Create shall save Client module Type and Azure IoT Gateway Assembly on DOTNET_HOST_HANDLE_DATA. ] */
					if (!createCLRInstancesGetInterfacesAndStarting(&out->spMetaHost, &out->spRuntimeInfo, &out->spCorRuntimeHost, &spDefaultAppDomain))
					{
						/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
						LogError("Error creating CLR Intance, Getting Interfaces and Starting it.");
						delete out; 
						out = NULL;
					}
					else if (FAILED(hr = spDefaultAppDomain->Load_2(bstrClientModuleAssemblyName, &spClientModuleAssembly)))
					{
						/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
						LogError("Failed to load the assembly w/hr 0x%08lx\n", hr);
						delete out;
						out = NULL;
					}
					/* Codes_SRS_DOTNET_04_010: [ DotNET_Create shall save Client module Type and Azure IoT Gateway Assembly on DOTNET_HOST_HANDLE_DATA. ] */
					else if (FAILED(hr = spClientModuleAssembly->GetType_2(bstrClientModuleClassName, &out->spClientModuleType)))
					{
						/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
						LogError("Failed to get the Type interface w/hr 0x%08lx\n", hr);
						delete out;
						out = NULL;
					}
					else if (FAILED(hr = spDefaultAppDomain->Load_2(bstrAzureIoTGatewayAssemblyName, &(out->spAzureIoTGatewayAssembly))))
					{
						/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
						LogError("Failed to load the assembly w/hr 0x%08lx\n", hr);
						delete out;
						out = NULL;
					}
					else if (!buildMessageBusObject((long long)out->bus, (long long)out, out->spAzureIoTGatewayAssembly, &vtAzureIoTGatewayMessageBusObject))
					{
						LogError("Failed to build Message Bus Object.");
						delete out;
						out = NULL;
					}
					/* Codes_SRS_DOTNET_04_009: [ DotNET_Create shall create an instance of .NET client Module and save it on DOTNET_HOST_HANDLE_DATA. ] */
					/* Codes_SRS_DOTNET_04_010: [ DotNET_Create shall save Client module Type and Azure IoT Gateway Assembly on DOTNET_HOST_HANDLE_DATA. ] */
					else if (FAILED(hr = spClientModuleAssembly->CreateInstance(bstrClientModuleClassName, &out->vtClientModuleObject)))
					{
						/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
						LogError("Creating an instance of Client Class failed with hr 0x%08lx\n", hr);
						delete out;
						out = NULL;
					}
					else if (!invokeCreateMethodFromClient(dotNetConfig->dotnet_module_args, &vtAzureIoTGatewayMessageBusObject, out->spClientModuleType, &out->vtClientModuleObject))
					{
						/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
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


bool buildMessageObject(MESSAGE_HANDLE messageHandle, _AssemblyPtr spAzureIoTGatewayAssembly, variant_t* vtAzureIoTGatewayMessageObject)
{
	SAFEARRAY *psaAzureIoTGatewayMessageConstructorArgs = NULL;
	bool returnResult = false;
	int32_t msg_size;


	try
	{
		const unsigned char*msgByteArray = Message_ToByteArray(messageHandle, &msg_size);
		variant_t msgContentInByteArray;
		V_VT(&msgContentInByteArray) = VT_ARRAY | VT_UI1;
		SAFEARRAYBOUND rgsabound[1];
		rgsabound[0].cElements = msg_size;
		rgsabound[0].lLbound = 0;
		HRESULT hrResult;
		void * pArrayData = NULL;

		if (msgByteArray == NULL)
		{
			LogError("Error getting Message Byte Array.");
		}
		else if ((V_ARRAY(&msgContentInByteArray) = SafeArrayCreate(VT_UI1, 1, rgsabound)) == NULL)
		{
			LogError("Error creating SafeArray.");
		}
		else if (FAILED(hrResult = SafeArrayAccessData(msgContentInByteArray.parray, &pArrayData)))
		{
			LogError("Error Acessing Safe Array Data. w/hr 0x%08lx\n", hrResult);
		}
		else
		{
			memcpy(pArrayData, msgByteArray, msg_size);
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
			/* Codes_SRS_DOTNET_04_017: [ DotNET_Receive shall construct an instance of the Message interface as defined below: ] */
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

static void DotNET_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
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

			/* Codes_SRS_DOTNET_04_017: [ DotNET_Receive shall construct an instance of the Message interface as defined below: ] */
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
			/* Codes_SRS_DOTNET_04_018: [ DotNET_Create shall call Receive C# method passing the Message object created with the content of message serialized into Message object. ] */
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
			/* Codes_SRS_DOTNET_04_015: [ DotNET_Receive shall do nothing if module is NULL. ] */
			/* Codes_SRS_DOTNET_04_016: [ DotNET_Receive shall do nothing if message is NULL. ] */
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


static void DotNET_Destroy(MODULE_HANDLE module)
{
	/* Codes_SRS_DOTNET_04_019: [ DotNET_Destroy shall do nothing if module is NULL. ] */
	if (module != NULL)
	{
		DOTNET_HOST_HANDLE_DATA* handleData = (DOTNET_HOST_HANDLE_DATA*)module;
		/* Codes_SRS_DOTNET_04_022: [ DotNET_Destroy shall call the Destroy C# method. ] */
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
		/* Codes_SRS_DOTNET_04_020: [ DotNET_Destroy shall free all resources associated with the given module.. ] */
		delete(handleData);
	}
}

MODULE_EXPORT bool Module_DotNetHost_PublishMessage(MESSAGE_BUS_HANDLE bus, MODULE_HANDLE sourceModule, const unsigned char* source, int32_t size)
{
	bool returnValue = false;
	MESSAGE_HANDLE messageToPublish = NULL;

	if (bus == NULL || sourceModule == NULL || source == NULL || size < 0)
	{
		/* Codes_SRS_DOTNET_04_022: [ Module_DotNetHost_PublishMessage shall return false if bus is NULL. ] */
		/* Codes_SRS_DOTNET_04_029: [ Module_DotNetHost_PublishMessage shall return false if sourceModule is NULL. ] */
		/* Codes_SRS_DOTNET_04_023: [ Module_DotNetHost_PublishMessage shall return false if source is NULL, or size if lower than 0. ] */
		LogError("invalid arg bus=%p, sourceModule=%p", bus, sourceModule);
	}
	/* Codes_SRS_DOTNET_04_024: [ Module_DotNetHost_PublishMessage shall create a message from source and size by invoking Message_CreateFromByteArray. ] */
	else if((messageToPublish = Message_CreateFromByteArray(source, size)) == NULL)
	{
		/* Codes_SRS_DOTNET_04_025: [ If Message_CreateFromByteArray fails, Module_DotNetHost_PublishMessage shall fail. ] */
		LogError("Error trying to create message from Byte Array");
	}
	/* Codes_SRS_DOTNET_04_026: [ Module_DotNetHost_PublishMessage shall call MessageBus_Publish passing bus, sourceModule, byte array and msgHandle. ] */
	else if (MessageBus_Publish(bus, sourceModule, messageToPublish) != MESSAGE_BUS_OK)
	{
		/* Codes_SRS_DOTNET_04_027: [ If MessageBus_Publish fail Module_DotNetHost_PublishMessage shall fail. ] */
		LogError("Error trying to publish message on MessageBus.");
	}
	else
	{
		/* Codes_SRS_DOTNET_04_028: [If MessageBus_Publish succeed Module_DotNetHost_PublishMessage shall succeed.] */
		returnValue = true;
	}

	if (messageToPublish != NULL)
	{
		Message_Destroy(messageToPublish);
	}

	return returnValue;
}

static const MODULE_APIS DOTNET_APIS_all =
{
	DotNET_Create,
	DotNET_Destroy,
	DotNET_Receive
};



#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(DOTNET_HOST)(void)
#else
MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void)
#endif
{
	/* Codes_SRS_DOTNET_04_021: [ Module_GetAPIS shall return a non-NULL pointer to a structure of type MODULE_APIS that has all fields initialized to non-NULL values. ] */
	return &DOTNET_APIS_all;
}
