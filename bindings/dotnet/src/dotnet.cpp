// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "azure_c_shared_utility/gballoc.h"

#include "module.h"
#include "message.h"
#include "azure_c_shared_utility/iot_logging.h"
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
	DOTNET_HOST_HANDLE_DATA(const DOTNET_HOST_HANDLE_DATA& other) :
		bus(other.bus),
		vtClientModuleObject(other.vtClientModuleObject),
		spMetaHost(other.spMetaHost),
		spRuntimeInfo(other.spRuntimeInfo),
		spCorRuntimeHost(other.spCorRuntimeHost),
		spClientModuleType(other.spClientModuleType),
		spAzureIoTGatewayMessageClassType(other.spAzureIoTGatewayMessageClassType),
		spAzureIoTGatewayAssembly(other.spAzureIoTGatewayAssembly)
	{

	};

	DOTNET_HOST_HANDLE_DATA()
	{

	};

    MESSAGE_BUS_HANDLE          bus;
	variant_t                   vtClientModuleObject;
	CComPtr<ICLRMetaHost>       spMetaHost;
	CComPtr<ICLRRuntimeInfo>    spRuntimeInfo;
	CComPtr<ICorRuntimeHost>    spCorRuntimeHost;
	_TypePtr                    spClientModuleType;
	_TypePtr                    spAzureIoTGatewayMessageClassType;
	_AssemblyPtr                spAzureIoTGatewayAssembly;

private:
	DOTNET_HOST_HANDLE_DATA& operator=(const DOTNET_HOST_HANDLE_DATA&);
};

static MODULE_HANDLE DotNET_Create(MESSAGE_BUS_HANDLE busHandle, const void* configuration)
{
	DOTNET_HOST_HANDLE_DATA result;
	DOTNET_HOST_HANDLE_DATA* out = NULL;
	SAFEARRAY *psaClientModuleCreateArgs = NULL;
	SAFEARRAY *psaAzureIoTGatewayMessageBusConstructorArgs = NULL;


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
				BOOL fLoadable;
				IUnknownPtr spAppDomainThunk;
				_AppDomainPtr spDefaultAppDomain;
				bstr_t bstrClientModuleAssemblyName(dotNetConfig->dotnet_module_path);
				_AssemblyPtr spClientModuleAssembly;
				bstr_t bstrClientModuleClassName(dotNetConfig->dotnet_module_entry_class);
				bstr_t bstrAzureIoTGatewayAssemblyName(AZUREIOTGATEWAYASSEMBLYNAME);
				bstr_t bstrAzureIoTGatewayMessageBusClassName(AZUREIOTGATEWAY_MESSAGEBUS_CLASSNAME);
				_TypePtr spAzureIoTGatewayMessageBusClassType;
				bstr_t bstrAzureIoTGatewayMessageClassName(AZUREIOTGATEWAY_MESSAGE_CLASSNAME);


				/* Codes_SRS_DOTNET_04_007: [ DotNET_Create shall return a non-NULL MODULE_HANDLE when successful. ] */
				result.bus = busHandle;

				/* Codes_SRS_DOTNET_04_012: [ DotNET_Create shall get the 3 CLR Host Interfaces (CLRMetaHost, CLRRuntimeInfo and CorRuntimeHost) and save it on DOTNET_HOST_HANDLE_DATA. ] */
				/* Codes_SRS_DOTNET_04_010: [ DotNET_Create shall save Client module Type, AzureIoTGateway Message Class and Azure IoT Gateway Assembly and on DOTNET_HOST_HANDLE_DATA. ] */
				hr = CLRCreateInstance(CLSID_CLRMetaHost, IID_PPV_ARGS(&result.spMetaHost));
				if (FAILED(hr))
				{
					/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
					LogError("CLRCreateInstance failed w/hr 0x%08lx\n", hr);
				}
				/* Codes_SRS_DOTNET_04_012: [ DotNET_Create shall get the 3 CLR Host Interfaces (CLRMetaHost, CLRRuntimeInfo and CorRuntimeHost) and save it on DOTNET_HOST_HANDLE_DATA. ] */
				/* Codes_SRS_DOTNET_04_010: [ DotNET_Create shall save Client module Type, AzureIoTGateway Message Class and Azure IoT Gateway Assembly and on DOTNET_HOST_HANDLE_DATA. ] */
				else if (FAILED(hr = (result.spMetaHost)->GetRuntime(DEFAULT_CLR_VERSION, IID_PPV_ARGS(&result.spRuntimeInfo))))
				{
					/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
					LogError("ICLRMetaHost::GetRuntime failed w/hr 0x%08lx", hr);
				}
				/* Codes_SRS_DOTNET_04_012: [ DotNET_Create shall get the 3 CLR Host Interfaces (CLRMetaHost, CLRRuntimeInfo and CorRuntimeHost) and save it on DOTNET_HOST_HANDLE_DATA. ] */
				else if (FAILED(hr = (result.spRuntimeInfo)->IsLoadable(&fLoadable)))
				{
					/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
					LogError("ICLRRuntimeInfo::IsLoadable failed w/hr 0x%08lx\n", hr);
				}
				else if (!fLoadable)
				{
					/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
					LogError(".NET runtime %ls cannot be loaded\n", DEFAULT_CLR_VERSION);
				}
				/* Codes_SRS_DOTNET_04_012: [ DotNET_Create shall get the 3 CLR Host Interfaces (CLRMetaHost, CLRRuntimeInfo and CorRuntimeHost) and save it on DOTNET_HOST_HANDLE_DATA. ] */
				/* Codes_SRS_DOTNET_04_010: [ DotNET_Create shall save Client module Type, AzureIoTGateway Message Class and Azure IoT Gateway Assembly and on DOTNET_HOST_HANDLE_DATA. ] */
				else if (FAILED(hr = (result.spRuntimeInfo)->GetInterface(CLSID_CorRuntimeHost, IID_PPV_ARGS(&(result.spCorRuntimeHost)))))
				{
					/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
					LogError("ICLRRuntimeInfo::GetInterface failed w/hr 0x%08lx\n", hr);
				}
				else if (FAILED(hr = (result.spCorRuntimeHost)->Start()))
				{
					/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
					LogError("CLR failed to start w/hr 0x%08lx\n", hr);
				}
				else if (FAILED(hr = (result.spCorRuntimeHost)->GetDefaultDomain(&spAppDomainThunk)))
				{
					/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
					LogError("ICorRuntimeHost::GetDefaultDomain failed w/hr 0x%08lx\n", hr);
				}
				else if (FAILED(hr = spAppDomainThunk->QueryInterface(IID_PPV_ARGS(&spDefaultAppDomain))))
				{
					/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
					LogError("Failed to get default AppDomain w/hr 0x%08lx\n", hr);
				}
				else if (FAILED(hr = spDefaultAppDomain->Load_2(bstrClientModuleAssemblyName, &spClientModuleAssembly)))
				{
					/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
					LogError("Failed to load the assembly w/hr 0x%08lx\n", hr);
				}
				/* Codes_SRS_DOTNET_04_010: [ DotNET_Create shall save Client module Type, AzureIoTGateway Message Class and Azure IoT Gateway Assembly and on DOTNET_HOST_HANDLE_DATA. ] */
				else if (FAILED(hr = spClientModuleAssembly->GetType_2(bstrClientModuleClassName, &result.spClientModuleType)))
				{
					/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
					LogError("Failed to get the Type interface w/hr 0x%08lx\n", hr);
				}
				else if (FAILED(hr = spDefaultAppDomain->Load_2(bstrAzureIoTGatewayAssemblyName, &(result.spAzureIoTGatewayAssembly))))
				{
					/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
					LogError("Failed to load the assembly w/hr 0x%08lx\n", hr);
				}
				else if (FAILED(hr = result.spAzureIoTGatewayAssembly->GetType_2(bstrAzureIoTGatewayMessageBusClassName, &spAzureIoTGatewayMessageBusClassType)))
				{
					/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
					LogError("Failed to get the Type interface w/hr 0x%08lx\n", hr);
				}
				/* Codes_SRS_DOTNET_04_010: [ DotNET_Create shall save Client module Type, AzureIoTGateway Message Class and Azure IoT Gateway Assembly and on DOTNET_HOST_HANDLE_DATA. ] */
				else if (FAILED(hr = result.spAzureIoTGatewayAssembly->GetType_2(bstrAzureIoTGatewayMessageClassName, &(result.spAzureIoTGatewayMessageClassType))))
				{
					/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
					LogError("Failed to get the Type interface w/hr 0x%08lx\n", hr);
				}
				else
				{
					LONG index = 0;
					variant_t msgBus((long long)result.bus);
					variant_t vtAzureIoTGatewayMessageBusObject;

					psaAzureIoTGatewayMessageBusConstructorArgs = SafeArrayCreateVector(VT_VARIANT, 0, 1);

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
					/* Codes_SRS_DOTNET_04_013: [ A .NET Object conforming to the MessageBus interface defined shall be created: ] */
					else if (FAILED(hr = result.spAzureIoTGatewayAssembly->CreateInstance_3(bstrAzureIoTGatewayMessageBusClassName, true, static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public), NULL, psaAzureIoTGatewayMessageBusConstructorArgs, NULL, NULL, &vtAzureIoTGatewayMessageBusObject)))
					{
						/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
						LogError("Creating an instance of Message Bus failed with hr 0x%08lx\n", hr);
					}
					/* Codes_SRS_DOTNET_04_009: [ DotNET_Create shall create an instance of .NET client Module and save it on DOTNET_HOST_HANDLE_DATA. ] */
					/* Codes_SRS_DOTNET_04_010: [ DotNET_Create shall save Client module Type, AzureIoTGateway Message Class and Azure IoT Gateway Assembly and on DOTNET_HOST_HANDLE_DATA. ] */
					else if (FAILED(hr = spClientModuleAssembly->CreateInstance(bstrClientModuleClassName, &result.vtClientModuleObject)))
					{
						/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
						LogError("Creating an instance of Client Class failed with hr 0x%08lx\n", hr);
					}
					else
					{
						index = 0;
						psaClientModuleCreateArgs = SafeArrayCreateVector(VT_VARIANT, 0, 2);

						if (psaClientModuleCreateArgs == NULL)
						{
							/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
							LogError("Failed to create Safe Array. ");
						}
						else if (FAILED(hr = SafeArrayPutElement(psaClientModuleCreateArgs, &index, &vtAzureIoTGatewayMessageBusObject)))
						{
							/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
							LogError("Adding Element on the safe array failed. w/hr 0x%08lx\n", hr);
						}
						else
						{
							variant_t vtdotNetArgsArg(dotNetConfig->dotnet_module_args);
							bstr_t bstrCreateClientMethodName(L"Create");
							variant_t vt_Empty;
							index = 1;

							if (FAILED(hr = SafeArrayPutElement(psaClientModuleCreateArgs, &index, &vtdotNetArgsArg)))
							{
								/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
								LogError("Adding Element on the safe array failed. w/hr 0x%08lx\n", hr);
							}
							/* Codes_SRS_DOTNET_04_014: [ DotNET_Create shall call Create C# method, implemented from IGatewayModule, passing the MessageBus object created and configuration->dotnet_module_args. ] */
							else if (FAILED(hr = result.spClientModuleType->InvokeMember_3(bstrCreateClientMethodName, static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public | BindingFlags_InvokeMethod), NULL, result.vtClientModuleObject, psaClientModuleCreateArgs, &vt_Empty)))
							{
								/* Codes_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
								LogError("Failed to invoke Create Method with hr 0x%08lx\n", hr);
							}
							else
							{
								try
								{
									/* Codes_SRS_DOTNET_04_008: [ DotNET_Create shall allocate memory for an instance of the DOTNET_HOST_HANDLE_DATA structure and use that as the backing structure for the module handle. ] */
									out = new DOTNET_HOST_HANDLE_DATA(result);
								}
								catch (std::bad_alloc)
								{
									//Do not need to do anything, since we are returning NULL below.
								}
							}
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

	//clean up Safe Arrays.
	if (psaClientModuleCreateArgs)
	{
		SafeArrayDestroy(psaClientModuleCreateArgs);
	}

	if (psaAzureIoTGatewayMessageBusConstructorArgs)
	{
		SafeArrayDestroy(psaAzureIoTGatewayMessageBusConstructorArgs);
	}

    return out;
}

static void DotNET_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
	SAFEARRAY *psaAzureIoTGatewayMessageConstructorArgs = NULL;
	SAFEARRAY *psaAzureIoTGatewayClientReceiveArgs = NULL;

	try
	{
		if (
			(moduleHandle != NULL) &&
			(messageHandle != NULL)
			)
		{
			DOTNET_HOST_HANDLE_DATA* result = (DOTNET_HOST_HANDLE_DATA*)moduleHandle;

			/* Codes_SRS_DOTNET_04_017: [ DotNET_Receive shall construct an instance of the Message interface as defined below: ] */
			int32_t msg_size;
			const unsigned char*msgByteArray = Message_ToByteArray(messageHandle, &msg_size);

			if (msgByteArray != NULL)
			{
				variant_t msgContentInByteArray;
				V_VT(&msgContentInByteArray) = VT_ARRAY | VT_UI1;
				SAFEARRAYBOUND rgsabound[1];
				rgsabound[0].cElements = msg_size;
				rgsabound[0].lLbound = 0;

				V_ARRAY(&msgContentInByteArray) = SafeArrayCreate(VT_UI1, 1, rgsabound);

				if (V_ARRAY(&msgContentInByteArray) != NULL)
				{
					HRESULT hrResult;
					void * pArrayData = NULL;
					hrResult = SafeArrayAccessData(msgContentInByteArray.parray, &pArrayData);
					if (!FAILED(hrResult))
					{
						memcpy(pArrayData, msgByteArray, msg_size);
						hrResult = SafeArrayUnaccessData(msgContentInByteArray.parray);
						if (!FAILED(hrResult))
						{
							psaAzureIoTGatewayMessageConstructorArgs = SafeArrayCreateVector(VT_VARIANT, 0, 1);

							if (psaAzureIoTGatewayMessageConstructorArgs != NULL)
							{
								LONG index = 0;
								hrResult = SafeArrayPutElement(psaAzureIoTGatewayMessageConstructorArgs, &index, &msgContentInByteArray);
								if (!FAILED(hrResult))
								{
									bstr_t bstrAzureIoTGatewayMessageClassName(AZUREIOTGATEWAY_MESSAGE_CLASSNAME);
									variant_t vtAzureIoTGatewayMessageObject;

									/* Codes_SRS_DOTNET_04_017: [ DotNET_Receive shall construct an instance of the Message interface as defined below: ] */
									hrResult = result->spAzureIoTGatewayAssembly->CreateInstance_3
									(
										bstrAzureIoTGatewayMessageClassName,
										true,
										static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public),
										NULL,
										psaAzureIoTGatewayMessageConstructorArgs,
										NULL, NULL,
										&vtAzureIoTGatewayMessageObject
									);

									if (!FAILED(hrResult))
									{
										bstr_t bstrReceiveClientMethodName(L"Receive");
										psaAzureIoTGatewayClientReceiveArgs = SafeArrayCreateVector(VT_VARIANT, 0, 1);

										if (psaAzureIoTGatewayClientReceiveArgs != NULL)
										{
											variant_t vt_Empty;

											index = 0;
											hrResult = SafeArrayPutElement(psaAzureIoTGatewayClientReceiveArgs, &index, &vtAzureIoTGatewayMessageObject);

											if (!FAILED(hrResult))
											{
												/* Codes_SRS_DOTNET_04_018: [ DotNET_Create shall call Receive C# method passing the Message object created with the content of message serialized into Message object. ] */
												hrResult = result->spClientModuleType->InvokeMember_3
												(
													bstrReceiveClientMethodName,
													static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public | BindingFlags_InvokeMethod),
													NULL,
													result->vtClientModuleObject,
													psaAzureIoTGatewayClientReceiveArgs,
													&vt_Empty
												);
												if (FAILED(hrResult))
												{
													LogError("Failed to Invoke Receive method. w/hr 0x%08lx\n", hrResult);
												}
											}
											else
											{
												LogError("Error Adding Argument to the Safe Array.w/hr 0x%08lx\n", hrResult);
											}
										}
										else
										{
											LogError("Error creating arguments Safe Array for Receive.");
										}
									}
									else
									{
										LogError("Error Creating Message Class Instance.w/hr 0x%08lx\n", hrResult);
									}
								}
								else
								{
									LogError("Error Adding Element to the Arguments Safe Array.w/hr 0x%08lx\n", hrResult);
								}
							}
							else
							{
								LogError("Error building SafeArray Vector for arguments.");
							}
						}
						else
						{
							LogError("Error on call for SafeArrayUnaccessData.w/hr 0x%08lx\n", hrResult);
						}
					}
					else
					{
						LogError("Error Acessing Safe Array Data. w/hr 0x%08lx\n", hrResult);
					}
				}
				else
				{
					LogError("Error creating SafeArray.");
				}
			}
			else
			{
				LogError("Error getting message Byte Array.");
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
	if (psaAzureIoTGatewayMessageConstructorArgs)
	{
		SafeArrayDestroy(psaAzureIoTGatewayMessageConstructorArgs);
	}

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
			HRESULT hrResult = handleData->spClientModuleType->InvokeMember_3
			(
				bstrDestroyClientMethodName,
				static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public | BindingFlags_InvokeMethod),
				NULL,
				handleData->vtClientModuleObject,
				NULL,
				&vt_Empty
			);
			if (FAILED(hrResult))
			{
				LogError("Failed to Invoke Receive method. w/hr 0x%08lx\n", hrResult);
			}
		}
		catch (const _com_error& e)
		{
			LogError("Exception Thrown. Message: %s ", e.ErrorMessage());
		}
		/* Codes_SRS_DOTNET_04_020: [ DotNET_Destroy shall free all resources associated with the given module.. ] */
		delete(handleData);
	}
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
