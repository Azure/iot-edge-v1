// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/base64.h"
#include "dotnet.h"
#include "azure_c_shared_utility/constmap.h"
#include "azure_c_shared_utility/map.h"

#include <metahost.h>
#include <comutil.h>
#import "mscorlib.tlb" raw_interfaces_only				\
    high_property_prefixes("_get","_put","_putref")		\
    rename("ReportEvent", "InteropServices_ReportEvent")
using namespace mscorlib;


static MICROMOCK_MUTEX_HANDLE g_testByTest;
static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

#define GBALLOC_H

extern "C" int gballoc_init(void);
extern "C" void gballoc_deinit(void);
extern "C" void* gballoc_malloc(size_t size);
extern "C" void* gballoc_calloc(size_t nmemb, size_t size);
extern "C" void* gballoc_realloc(void* ptr, size_t size);
extern "C" void gballoc_free(void* ptr);


#define DefaultCLRVersion L"v4.0.30319"

// {A63CE3E8-D57A-4135-A7DD-C2F878D68D11}
static const GUID GENERIC_GUID =
{ 0xa63ce3e8, 0xd57a, 0x4135,{ 0xa7, 0xdd, 0xc2, 0xf8, 0x78, 0xd6, 0x8d, 0x11 } };

static const GUID NULL_GUID =
{ 0x00000000, 0x0000, 0x0000,{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };


static const GUID CLR_METAHOST_UUID =
{ 0xd332db9e, 0xb9b3, 0x4125,{ 0x82, 0x07, 0xa1, 0x48, 0x84, 0xf5, 0x32, 0x16 } };

static const GUID CLSID_CLRMetaHost_UUID =
{ 0x9280188d, 0xe8e, 0x4867, {0xb3, 0xc, 0x7f, 0xa8, 0x38, 0x84, 0xe8, 0xde} };


static const GUID CLSID_CLRRunTime_UUID =
{ 0xbd39d1d2, 0xba2f, 0x486a,{ 0x89, 0xb0, 0xb4, 0xb0, 0xcb, 0x46, 0x68, 0x91 } };

//EXTERN_GUID(CLSID_CorRuntimeHost, 0xcb2f6723, 0xab3a, 0x11d2, 0x9c, 0x40, 0x00, 0xc0, 0x4f, 0xa3, 0x0a, 0x3e);
static const GUID CLSID_CorRuntimeHost_UUID =
{ 0xcb2f6723, 0xab3a, 0x11d2,{ 0x9c, 0x40, 0x00, 0xc0, 0x4f, 0xa3, 0x0a, 0x3e } };

//MIDL_INTERFACE("CB2F6722-AB3A-11d2-9C40-00C04FA30A3E") - ICorRuntimeHost uuid
static const GUID ICorRuntimeHost_UUID =
{ 0xcb2f6722, 0xab3a, 0x11d2,{ 0x9c, 0x40, 0x00, 0xc0, 0x4f, 0xa3, 0x0a, 0x3e } };

//AppDomain: 05f696dc-2b29-3663-ad8b-c4389cf2a713
static const GUID AppDomainUUID =
{ 0x05f696dc, 0x2b29, 0x3663,{ 0xad, 0x8b, 0xc4, 0x38, 0x9c, 0xf2, 0xa7, 0x13 } };


static size_t gMessageSize;
static const unsigned char * gMessageSource;

static size_t currentnew_call;
static size_t whenShallnew_fail;

static size_t global_IsLoadable;

#define GBALLOC_H

extern "C" int   gballoc_init(void);
extern "C" void  gballoc_deinit(void);
extern "C" void* gballoc_malloc(size_t size);
extern "C" void* gballoc_calloc(size_t nmemb, size_t size);
extern "C" void* gballoc_realloc(void* ptr, size_t size);
extern "C" void  gballoc_free(void* ptr);

namespace BASEIMPLEMENTATION
{
    /*if malloc is defined as gballoc_malloc at this moment, there'd be serious trouble*/
#define Lock(x) (LOCK_OK + gballocState - gballocState) /*compiler warning about constant in if condition*/
#define Unlock(x) (LOCK_OK + gballocState - gballocState)
#define Lock_Init() (LOCK_HANDLE)0x42
#define Lock_Deinit(x) (LOCK_OK + gballocState - gballocState)
#include "gballoc.c"
#undef Lock
#undef Unlock
#undef Lock_Init
#undef Lock_Deinit

#include <stddef.h>   /* size_t */

};

//Comparator to compare a WChar
bool operator==(_In_ const CMockValue<const wchar_t*>& lhs, _In_ const CMockValue<const wchar_t*>& rhs)
{
	const wchar_t* left = lhs.GetValue();
	const wchar_t* right = rhs.GetValue();

	return (wcscmp(left, right) == 0);
}

bool operator==(_In_ const CMockValue<BSTR>& lhs, _In_ const CMockValue<BSTR>& rhs)
{
	const wchar_t* left = lhs.GetValue();
	const wchar_t* right = rhs.GetValue();

	return (wcscmp(left, right) == 0);
}

//Needed to add that for the CLRCreateInstance.
std::ostream& operator<<(std::ostream& left, const IID leIID)
{
	return left;
}

//Needed to add for Invoke member. VARIANT, but not using on this Unit Test. (Not evaluating this as arg)
std::ostream& operator<<(std::ostream& left, const VARIANT rVARIANT)
{
	return left;
}

bool operator==(_In_ const VARIANT lhs, _In_ const VARIANT rhs)
{
	return true;
}

void* operator new  (std::size_t count)
{
	void* result2;
	currentnew_call++;
	if (whenShallnew_fail>0)
	{
		if (currentnew_call == whenShallnew_fail)
		{
			//result2 = NULL;
			throw std::bad_alloc();
		}
		else
		{
			result2 = BASEIMPLEMENTATION::gballoc_malloc(count);
		}
	}
	else
	{
		result2 = BASEIMPLEMENTATION::gballoc_malloc(count);
	}
	

	return result2;
};


class myICLRMetaHost : public ICLRMetaHost
{
public:
	HRESULT STDMETHODCALLTYPE GetRuntime(
		/* [in] */ LPCWSTR pwzVersion,
		/* [in] */ REFIID riid,
		/* [retval][iid_is][out] */ LPVOID *ppRuntime);
	

	HRESULT STDMETHODCALLTYPE GetVersionFromFile(
		/* [in] */ LPCWSTR pwzFilePath,
		/* [annotation][size_is][out] */
		_Out_writes_all_(*pcchBuffer)  LPWSTR pwzBuffer,
		/* [out][in] */ DWORD *pcchBuffer)
	{
		return S_OK;
	};

	HRESULT STDMETHODCALLTYPE EnumerateInstalledRuntimes(
		/* [retval][out] */ IEnumUnknown **ppEnumerator)
	{
		return S_OK;
	};


	HRESULT STDMETHODCALLTYPE EnumerateLoadedRuntimes(
		/* [in] */ HANDLE hndProcess,
		/* [retval][out] */ IEnumUnknown **ppEnumerator)
	{
		return S_OK;
	};

	HRESULT STDMETHODCALLTYPE RequestRuntimeLoadedNotification(
		/* [in] */ RuntimeLoadedCallbackFnPtr pCallbackFunction)
	{
		return S_OK;
	};

	HRESULT STDMETHODCALLTYPE QueryLegacyV2RuntimeBinding(
		/* [in] */ REFIID riid,
		/* [retval][iid_is][out] */ LPVOID *ppUnk)
	{
		return S_OK;
	};

	HRESULT STDMETHODCALLTYPE ExitProcess(
		/* [in] */ INT32 iExitCode)
	{
		return S_OK;
	};

	//From IUnkonwn
	HRESULT STDMETHODCALLTYPE QueryInterface(
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
	{
		return S_OK;
	};

	ULONG STDMETHODCALLTYPE AddRef(void)
	{
		return S_OK;
	};

	ULONG STDMETHODCALLTYPE Release(void)
	{
		return S_OK;
	};

};


class myICLRRuntimeInfo: public ICLRRuntimeInfo
{
public:
	HRESULT STDMETHODCALLTYPE GetVersionString(
		/* [annotation][size_is][out] */
		_Out_writes_all_opt_(*pcchBuffer)  LPWSTR pwzBuffer,
		/* [out][in] */ DWORD *pcchBuffer) 
	{
		return S_OK;
	};

	HRESULT STDMETHODCALLTYPE GetRuntimeDirectory(
		/* [annotation][size_is][out] */
		_Out_writes_all_(*pcchBuffer)  LPWSTR pwzBuffer,
		/* [out][in] */ DWORD *pcchBuffer)
	{
		return S_OK;
	};

	HRESULT STDMETHODCALLTYPE IsLoaded(
		/* [in] */ HANDLE hndProcess,
		/* [retval][out] */ BOOL *pbLoaded)
	{
		return S_OK;
	};

	HRESULT STDMETHODCALLTYPE LoadErrorString(
		/* [in] */ UINT iResourceID,
		/* [annotation][size_is][out] */
		_Out_writes_all_(*pcchBuffer)  LPWSTR pwzBuffer,
		/* [out][in] */ DWORD *pcchBuffer,
		/* [lcid][in] */ LONG iLocaleID)
	{
		return S_OK;
	};

	HRESULT STDMETHODCALLTYPE LoadLibrary(
		/* [in] */ LPCWSTR pwzDllName,
		/* [retval][out] */ HMODULE *phndModule)
	{
		return S_OK;
	};

	HRESULT STDMETHODCALLTYPE GetProcAddress(
		/* [in] */ LPCSTR pszProcName,
		/* [retval][out] */ LPVOID *ppProc)
	{
		return S_OK;
	};

	HRESULT STDMETHODCALLTYPE GetInterface(
		/* [in] */ REFCLSID rclsid,
		/* [in] */ REFIID riid,
		/* [retval][iid_is][out] */ LPVOID *ppUnk);

	HRESULT STDMETHODCALLTYPE IsLoadable(
		/* [retval][out] */ BOOL *pbLoadable);

	HRESULT STDMETHODCALLTYPE SetDefaultStartupFlags(
		/* [in] */ DWORD dwStartupFlags,
		/* [in] */ LPCWSTR pwzHostConfigFile)
	{
		return S_OK;
	};

	HRESULT STDMETHODCALLTYPE GetDefaultStartupFlags(
		/* [out] */ DWORD *pdwStartupFlags,
		/* [annotation][size_is][out] */
		_Out_writes_all_opt_(*pcchHostConfigFile)  LPWSTR pwzHostConfigFile,
		/* [out][in] */ DWORD *pcchHostConfigFile)
	{
		return S_OK;
	};

	HRESULT STDMETHODCALLTYPE BindAsLegacyV2Runtime(void)
	{
		return S_OK;
	};

	HRESULT STDMETHODCALLTYPE IsStarted(
		/* [out] */ BOOL *pbStarted,
		/* [out] */ DWORD *pdwStartupFlags)
	{
		return S_OK;
	};

	//From IUnkonwn
	HRESULT STDMETHODCALLTYPE QueryInterface(
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
	{
		return S_OK;
	};

	ULONG STDMETHODCALLTYPE AddRef(void)
	{
		return S_OK;
	};

	ULONG STDMETHODCALLTYPE Release(void)
	{
		return S_OK;
	};
};



class myICorRuntimeHost : public ICorRuntimeHost
{
public:
	HRESULT STDMETHODCALLTYPE CreateLogicalThreadState(void){  return S_OK; } ;

	HRESULT STDMETHODCALLTYPE DeleteLogicalThreadState(void){  return S_OK; } ;

	HRESULT STDMETHODCALLTYPE SwitchInLogicalThreadState(
		/* [in] */ DWORD *pFiberCookie){  return S_OK; } ;

	HRESULT STDMETHODCALLTYPE SwitchOutLogicalThreadState(
		/* [out] */ DWORD **pFiberCookie){  return S_OK; } ;

	HRESULT STDMETHODCALLTYPE LocksHeldByLogicalThread(
		/* [out] */ DWORD *pCount){  return S_OK; } ;

	HRESULT STDMETHODCALLTYPE MapFile(
		/* [in] */ HANDLE hFile,
		/* [out] */ HMODULE *hMapAddress){  return S_OK; } ;

	HRESULT STDMETHODCALLTYPE GetConfiguration(
		/* [out] */ ICorConfiguration **pConfiguration){  return S_OK; } ;

	HRESULT STDMETHODCALLTYPE Start(void);

	HRESULT STDMETHODCALLTYPE Stop(void){  return S_OK; } ;

	HRESULT STDMETHODCALLTYPE CreateDomain(
		/* [in] */ LPCWSTR pwzFriendlyName,
		/* [in] */ IUnknown *pIdentityArray,
		/* [out] */ IUnknown **pAppDomain){  return S_OK; } ;

	HRESULT STDMETHODCALLTYPE GetDefaultDomain(
		/* [out] */ IUnknown **pAppDomain);

	HRESULT STDMETHODCALLTYPE EnumDomains(
		/* [out] */ HDOMAINENUM *hEnum){  return S_OK; } ;

	HRESULT STDMETHODCALLTYPE NextDomain(
		/* [in] */ HDOMAINENUM hEnum,
		/* [out] */ IUnknown **pAppDomain){  return S_OK; } ;

	HRESULT STDMETHODCALLTYPE CloseEnum(
		/* [in] */ HDOMAINENUM hEnum){  return S_OK; } ;

	HRESULT STDMETHODCALLTYPE CreateDomainEx(
		/* [in] */ LPCWSTR pwzFriendlyName,
		/* [in] */ IUnknown *pSetup,
		/* [in] */ IUnknown *pEvidence,
		/* [out] */ IUnknown **pAppDomain){  return S_OK; } ;

	HRESULT STDMETHODCALLTYPE CreateDomainSetup(
		/* [out] */ IUnknown **pAppDomainSetup){  return S_OK; } ;

	HRESULT STDMETHODCALLTYPE CreateEvidence(
		/* [out] */ IUnknown **pEvidence){  return S_OK; } ;

	HRESULT STDMETHODCALLTYPE UnloadDomain(
		/* [in] */ IUnknown *pAppDomain){  return S_OK; } ;

	HRESULT STDMETHODCALLTYPE CurrentDomain(
		/* [out] */ IUnknown **pAppDomain){  return S_OK; } ;

	//From IUnkonwn
	HRESULT STDMETHODCALLTYPE QueryInterface(
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
	{
		return S_OK;
	};

	ULONG STDMETHODCALLTYPE AddRef(void)
	{
		return S_OK;
	};

	ULONG STDMETHODCALLTYPE Release(void)
	{
		return S_OK;
	};
};

class myIUnkonwnClass : public IUnknown
{
	HRESULT STDMETHODCALLTYPE QueryInterface(
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject);

	ULONG STDMETHODCALLTYPE AddRef(void)
	{
		return S_OK;
	};

	ULONG STDMETHODCALLTYPE Release(void)
	{
		return S_OK;
	};
};

class myAppDomain : public _AppDomain
{
	//
	// Raw methods provided by interface
	//

	 HRESULT __stdcall GetTypeInfoCount(
		/*[out]*/ unsigned long * pcTInfo){  return S_OK; } ;
			 HRESULT __stdcall GetTypeInfo(
				/*[in]*/ unsigned long iTInfo,
				/*[in]*/ unsigned long lcid,
				/*[in]*/ long ppTInfo){  return S_OK; } ;
			 HRESULT __stdcall GetIDsOfNames(
				/*[in]*/ GUID * riid,
				/*[in]*/ long rgszNames,
				/*[in]*/ unsigned long cNames,
				/*[in]*/ unsigned long lcid,
				/*[in]*/ long rgDispId){  return S_OK; } ;
			 HRESULT __stdcall Invoke(
				/*[in]*/ unsigned long dispIdMember,
				/*[in]*/ GUID * riid,
				/*[in]*/ unsigned long lcid,
				/*[in]*/ short wFlags,
				/*[in]*/ long pDispParams,
				/*[in]*/ long pVarResult,
				/*[in]*/ long pExcepInfo,
				/*[in]*/ long puArgErr){  return S_OK; } ;
			 HRESULT __stdcall get_ToString(
				/*[out,retval]*/ BSTR * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall Equals(
				/*[in]*/ VARIANT other,
				/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall GetHashCode(
				/*[out,retval]*/ long * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall GetType(
				/*[out,retval]*/ struct _Type * * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall InitializeLifetimeService(
				/*[out,retval]*/ VARIANT * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall GetLifetimeService(
				/*[out,retval]*/ VARIANT * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall get_Evidence(
				/*[out,retval]*/ struct _Evidence * * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall add_DomainUnload(
				/*[in]*/ struct _EventHandler * value){  return S_OK; } ;
			 HRESULT __stdcall remove_DomainUnload(
				/*[in]*/ struct _EventHandler * value){  return S_OK; } ;
			 HRESULT __stdcall add_AssemblyLoad(
				/*[in]*/ struct _AssemblyLoadEventHandler * value){  return S_OK; } ;
			 HRESULT __stdcall remove_AssemblyLoad(
				/*[in]*/ struct _AssemblyLoadEventHandler * value){  return S_OK; } ;
			 HRESULT __stdcall add_ProcessExit(
				/*[in]*/ struct _EventHandler * value){  return S_OK; } ;
			 HRESULT __stdcall remove_ProcessExit(
				/*[in]*/ struct _EventHandler * value){  return S_OK; } ;
			 HRESULT __stdcall add_TypeResolve(
				/*[in]*/ struct _ResolveEventHandler * value){  return S_OK; } ;
			 HRESULT __stdcall remove_TypeResolve(
				/*[in]*/ struct _ResolveEventHandler * value){  return S_OK; } ;
			 HRESULT __stdcall add_ResourceResolve(
				/*[in]*/ struct _ResolveEventHandler * value){  return S_OK; } ;
			 HRESULT __stdcall remove_ResourceResolve(
				/*[in]*/ struct _ResolveEventHandler * value){  return S_OK; } ;
			 HRESULT __stdcall add_AssemblyResolve(
				/*[in]*/ struct _ResolveEventHandler * value){  return S_OK; } ;
			 HRESULT __stdcall remove_AssemblyResolve(
				/*[in]*/ struct _ResolveEventHandler * value){  return S_OK; } ;
			 HRESULT __stdcall add_UnhandledException(
				/*[in]*/ struct _UnhandledExceptionEventHandler * value){  return S_OK; } ;
			 HRESULT __stdcall remove_UnhandledException(
				/*[in]*/ struct _UnhandledExceptionEventHandler * value){  return S_OK; } ;
			 HRESULT __stdcall DefineDynamicAssembly(
				/*[in]*/ struct _AssemblyName * name,
				/*[in]*/ enum AssemblyBuilderAccess access,
				/*[out,retval]*/ struct _AssemblyBuilder * * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall DefineDynamicAssembly_2(
				/*[in]*/ struct _AssemblyName * name,
				/*[in]*/ enum AssemblyBuilderAccess access,
				/*[in]*/ BSTR dir,
				/*[out,retval]*/ struct _AssemblyBuilder * * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall DefineDynamicAssembly_3(
				/*[in]*/ struct _AssemblyName * name,
				/*[in]*/ enum AssemblyBuilderAccess access,
				/*[in]*/ struct _Evidence * Evidence,
				/*[out,retval]*/ struct _AssemblyBuilder * * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall DefineDynamicAssembly_4(
				/*[in]*/ struct _AssemblyName * name,
				/*[in]*/ enum AssemblyBuilderAccess access,
				/*[in]*/ struct _PermissionSet * requiredPermissions,
				/*[in]*/ struct _PermissionSet * optionalPermissions,
				/*[in]*/ struct _PermissionSet * refusedPermissions,
				/*[out,retval]*/ struct _AssemblyBuilder * * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall DefineDynamicAssembly_5(
				/*[in]*/ struct _AssemblyName * name,
				/*[in]*/ enum AssemblyBuilderAccess access,
				/*[in]*/ BSTR dir,
				/*[in]*/ struct _Evidence * Evidence,
				/*[out,retval]*/ struct _AssemblyBuilder * * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall DefineDynamicAssembly_6(
				/*[in]*/ struct _AssemblyName * name,
				/*[in]*/ enum AssemblyBuilderAccess access,
				/*[in]*/ BSTR dir,
				/*[in]*/ struct _PermissionSet * requiredPermissions,
				/*[in]*/ struct _PermissionSet * optionalPermissions,
				/*[in]*/ struct _PermissionSet * refusedPermissions,
				/*[out,retval]*/ struct _AssemblyBuilder * * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall DefineDynamicAssembly_7(
				/*[in]*/ struct _AssemblyName * name,
				/*[in]*/ enum AssemblyBuilderAccess access,
				/*[in]*/ struct _Evidence * Evidence,
				/*[in]*/ struct _PermissionSet * requiredPermissions,
				/*[in]*/ struct _PermissionSet * optionalPermissions,
				/*[in]*/ struct _PermissionSet * refusedPermissions,
				/*[out,retval]*/ struct _AssemblyBuilder * * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall DefineDynamicAssembly_8(
				/*[in]*/ struct _AssemblyName * name,
				/*[in]*/ enum AssemblyBuilderAccess access,
				/*[in]*/ BSTR dir,
				/*[in]*/ struct _Evidence * Evidence,
				/*[in]*/ struct _PermissionSet * requiredPermissions,
				/*[in]*/ struct _PermissionSet * optionalPermissions,
				/*[in]*/ struct _PermissionSet * refusedPermissions,
				/*[out,retval]*/ struct _AssemblyBuilder * * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall DefineDynamicAssembly_9(
				/*[in]*/ struct _AssemblyName * name,
				/*[in]*/ enum AssemblyBuilderAccess access,
				/*[in]*/ BSTR dir,
				/*[in]*/ struct _Evidence * Evidence,
				/*[in]*/ struct _PermissionSet * requiredPermissions,
				/*[in]*/ struct _PermissionSet * optionalPermissions,
				/*[in]*/ struct _PermissionSet * refusedPermissions,
				/*[in]*/ VARIANT_BOOL IsSynchronized,
				/*[out,retval]*/ struct _AssemblyBuilder * * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall CreateInstance(
				/*[in]*/ BSTR AssemblyName,
				/*[in]*/ BSTR typeName,
				/*[out,retval]*/ struct _ObjectHandle * * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall CreateInstanceFrom(
				/*[in]*/ BSTR assemblyFile,
				/*[in]*/ BSTR typeName,
				/*[out,retval]*/ struct _ObjectHandle * * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall CreateInstance_2(
				/*[in]*/ BSTR AssemblyName,
				/*[in]*/ BSTR typeName,
				/*[in]*/ SAFEARRAY * activationAttributes,
				/*[out,retval]*/ struct _ObjectHandle * * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall CreateInstanceFrom_2(
				/*[in]*/ BSTR assemblyFile,
				/*[in]*/ BSTR typeName,
				/*[in]*/ SAFEARRAY * activationAttributes,
				/*[out,retval]*/ struct _ObjectHandle * * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall CreateInstance_3(
				/*[in]*/ BSTR AssemblyName,
				/*[in]*/ BSTR typeName,
				/*[in]*/ VARIANT_BOOL ignoreCase,
				/*[in]*/ enum BindingFlags bindingAttr,
				/*[in]*/ struct _Binder * Binder,
				/*[in]*/ SAFEARRAY * args,
				/*[in]*/ struct _CultureInfo * culture,
				/*[in]*/ SAFEARRAY * activationAttributes,
				/*[in]*/ struct _Evidence * securityAttributes,
				/*[out,retval]*/ struct _ObjectHandle * * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall CreateInstanceFrom_3(
				/*[in]*/ BSTR assemblyFile,
				/*[in]*/ BSTR typeName,
				/*[in]*/ VARIANT_BOOL ignoreCase,
				/*[in]*/ enum BindingFlags bindingAttr,
				/*[in]*/ struct _Binder * Binder,
				/*[in]*/ SAFEARRAY * args,
				/*[in]*/ struct _CultureInfo * culture,
				/*[in]*/ SAFEARRAY * activationAttributes,
				/*[in]*/ struct _Evidence * securityAttributes,
				/*[out,retval]*/ struct _ObjectHandle * * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall Load(
				/*[in]*/ struct _AssemblyName * assemblyRef,
				/*[out,retval]*/ struct _Assembly * * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall Load_2(
				/*[in]*/ BSTR assemblyString,
				/*[out,retval]*/ struct _Assembly * * pRetVal);
			 HRESULT __stdcall Load_3(
				/*[in]*/ SAFEARRAY * rawAssembly,
				/*[out,retval]*/ struct _Assembly * * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall Load_4(
				/*[in]*/ SAFEARRAY * rawAssembly,
				/*[in]*/ SAFEARRAY * rawSymbolStore,
				/*[out,retval]*/ struct _Assembly * * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall Load_5(
				/*[in]*/ SAFEARRAY * rawAssembly,
				/*[in]*/ SAFEARRAY * rawSymbolStore,
				/*[in]*/ struct _Evidence * securityEvidence,
				/*[out,retval]*/ struct _Assembly * * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall Load_6(
				/*[in]*/ struct _AssemblyName * assemblyRef,
				/*[in]*/ struct _Evidence * assemblySecurity,
				/*[out,retval]*/ struct _Assembly * * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall Load_7(
				/*[in]*/ BSTR assemblyString,
				/*[in]*/ struct _Evidence * assemblySecurity,
				/*[out,retval]*/ struct _Assembly * * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall ExecuteAssembly(
				/*[in]*/ BSTR assemblyFile,
				/*[in]*/ struct _Evidence * assemblySecurity,
				/*[out,retval]*/ long * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall ExecuteAssembly_2(
				/*[in]*/ BSTR assemblyFile,
				/*[out,retval]*/ long * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall ExecuteAssembly_3(
				/*[in]*/ BSTR assemblyFile,
				/*[in]*/ struct _Evidence * assemblySecurity,
				/*[in]*/ SAFEARRAY * args,
				/*[out,retval]*/ long * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall get_FriendlyName(
				/*[out,retval]*/ BSTR * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall get_BaseDirectory(
				/*[out,retval]*/ BSTR * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall get_RelativeSearchPath(
				/*[out,retval]*/ BSTR * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall get_ShadowCopyFiles(
				/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall GetAssemblies(
				/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall AppendPrivatePath(
				/*[in]*/ BSTR Path){  return S_OK; } ;
			 HRESULT __stdcall ClearPrivatePath(){  return S_OK; } ;
			 HRESULT __stdcall SetShadowCopyPath(
				/*[in]*/ BSTR s){  return S_OK; } ;
			 HRESULT __stdcall ClearShadowCopyPath(){  return S_OK; } ;
			 HRESULT __stdcall SetCachePath(
				/*[in]*/ BSTR s){  return S_OK; } ;
			 HRESULT __stdcall SetData(
				/*[in]*/ BSTR name,
				/*[in]*/ VARIANT data){  return S_OK; } ;
			 HRESULT __stdcall GetData(
				/*[in]*/ BSTR name,
				/*[out,retval]*/ VARIANT * pRetVal){  return S_OK; } ;
			 HRESULT __stdcall SetAppDomainPolicy(
				/*[in]*/ struct _PolicyLevel * domainPolicy){  return S_OK; } ;
			 HRESULT __stdcall SetThreadPrincipal(
				/*[in]*/ struct IPrincipal * principal){  return S_OK; } ;
			 HRESULT __stdcall SetPrincipalPolicy(
				/*[in]*/ enum PrincipalPolicy policy){  return S_OK; } ;
			 HRESULT __stdcall DoCallBack(
				/*[in]*/ struct _CrossAppDomainDelegate * theDelegate){  return S_OK; } ;
			 HRESULT __stdcall get_DynamicDirectory(
				/*[out,retval]*/ BSTR * pRetVal){  return S_OK; } ;

			 //From IUnkonwn
			 HRESULT __stdcall QueryInterface(
				 /* [in] */ REFIID riid,
				 /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
			 {
				 return S_OK;
			 };

			 ULONG __stdcall AddRef(void)
			 {
				 return S_OK;
			 };

			 ULONG __stdcall Release(void)
			 {
				 return S_OK;
			 };
};

//struct __declspec(uuid("17156360-2f1a-384a-bc52-fde93c215c5b"))
class myAssemblyClass : public _Assembly 
{
	//
	// Raw methods provided by interface
	//

	HRESULT __stdcall get_ToString(
		/*[out,retval]*/ BSTR * pRetVal){  return S_OK; } ;
	HRESULT __stdcall Equals(
		/*[in]*/ VARIANT other,
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetHashCode(
		/*[out,retval]*/ long * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetType(
		/*[out,retval]*/ struct _Type * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_CodeBase(
		/*[out,retval]*/ BSTR * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_EscapedCodeBase(
		/*[out,retval]*/ BSTR * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetName(
		/*[out,retval]*/ struct _AssemblyName * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetName_2(
		/*[in]*/ VARIANT_BOOL copiedName,
		/*[out,retval]*/ struct _AssemblyName * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_FullName(
		/*[out,retval]*/ BSTR * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_EntryPoint(
		/*[out,retval]*/ struct _MethodInfo * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetType_2(
		/*[in]*/ BSTR name,
		/*[out,retval]*/ struct _Type * * pRetVal);
	HRESULT __stdcall GetType_3(
		/*[in]*/ BSTR name,
		/*[in]*/ VARIANT_BOOL throwOnError,
		/*[out,retval]*/ struct _Type * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetExportedTypes(
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetTypes(
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetManifestResourceStream(
		/*[in]*/ struct _Type * Type,
		/*[in]*/ BSTR name,
		/*[out,retval]*/ struct _Stream * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetManifestResourceStream_2(
		/*[in]*/ BSTR name,
		/*[out,retval]*/ struct _Stream * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetFile(
		/*[in]*/ BSTR name,
		/*[out,retval]*/ struct _FileStream * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetFiles(
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetFiles_2(
		/*[in]*/ VARIANT_BOOL getResourceModules,
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetManifestResourceNames(
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetManifestResourceInfo(
		/*[in]*/ BSTR resourceName,
		/*[out,retval]*/ struct _ManifestResourceInfo * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_Location(
		/*[out,retval]*/ BSTR * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_Evidence(
		/*[out,retval]*/ struct _Evidence * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetCustomAttributes(
		/*[in]*/ struct _Type * attributeType,
		/*[in]*/ VARIANT_BOOL inherit,
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetCustomAttributes_2(
		/*[in]*/ VARIANT_BOOL inherit,
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall IsDefined(
		/*[in]*/ struct _Type * attributeType,
		/*[in]*/ VARIANT_BOOL inherit,
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetObjectData(
		/*[in]*/ struct _SerializationInfo * info,
		/*[in]*/ struct StreamingContext Context){  return S_OK; } ;
	HRESULT __stdcall add_ModuleResolve(
		/*[in]*/ struct _ModuleResolveEventHandler * value){  return S_OK; } ;
	HRESULT __stdcall remove_ModuleResolve(
		/*[in]*/ struct _ModuleResolveEventHandler * value){  return S_OK; } ;
	HRESULT __stdcall GetType_4(
		/*[in]*/ BSTR name,
		/*[in]*/ VARIANT_BOOL throwOnError,
		/*[in]*/ VARIANT_BOOL ignoreCase,
		/*[out,retval]*/ struct _Type * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetSatelliteAssembly(
		/*[in]*/ struct _CultureInfo * culture,
		/*[out,retval]*/ struct _Assembly * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetSatelliteAssembly_2(
		/*[in]*/ struct _CultureInfo * culture,
		/*[in]*/ struct _Version * Version,
		/*[out,retval]*/ struct _Assembly * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall LoadModule(
		/*[in]*/ BSTR moduleName,
		/*[in]*/ SAFEARRAY * rawModule,
		/*[out,retval]*/ struct _Module * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall LoadModule_2(
		/*[in]*/ BSTR moduleName,
		/*[in]*/ SAFEARRAY * rawModule,
		/*[in]*/ SAFEARRAY * rawSymbolStore,
		/*[out,retval]*/ struct _Module * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall CreateInstance(
		/*[in]*/ BSTR typeName,
		/*[out,retval]*/ VARIANT * pRetVal);
	HRESULT __stdcall CreateInstance_2(
		/*[in]*/ BSTR typeName,
		/*[in]*/ VARIANT_BOOL ignoreCase,
		/*[out,retval]*/ VARIANT * pRetVal){  return S_OK; } ;
	HRESULT __stdcall CreateInstance_3(
		/*[in]*/ BSTR typeName,
		/*[in]*/ VARIANT_BOOL ignoreCase,
		/*[in]*/ enum BindingFlags bindingAttr,
		/*[in]*/ struct _Binder * Binder,
		/*[in]*/ SAFEARRAY * args,
		/*[in]*/ struct _CultureInfo * culture,
		/*[in]*/ SAFEARRAY * activationAttributes,
		/*[out,retval]*/ VARIANT * pRetVal);
	HRESULT __stdcall GetLoadedModules(
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetLoadedModules_2(
		/*[in]*/ VARIANT_BOOL getResourceModules,
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetModules(
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetModules_2(
		/*[in]*/ VARIANT_BOOL getResourceModules,
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetModule(
		/*[in]*/ BSTR name,
		/*[out,retval]*/ struct _Module * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetReferencedAssemblies(
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_GlobalAssemblyCache(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;

	//From IDispatch:
	HRESULT STDMETHODCALLTYPE GetTypeInfoCount(
		/* [out] */ __RPC__out UINT *pctinfo){  return S_OK; } ;

	HRESULT STDMETHODCALLTYPE GetTypeInfo(
		/* [in] */ UINT iTInfo,
		/* [in] */ LCID lcid,
		/* [out] */ __RPC__deref_out_opt ITypeInfo **ppTInfo){  return S_OK; } ;

	HRESULT STDMETHODCALLTYPE GetIDsOfNames(
		/* [in] */ __RPC__in REFIID riid,
		/* [size_is][in] */ __RPC__in_ecount_full(cNames) LPOLESTR *rgszNames,
		/* [range][in] */ __RPC__in_range(0, 16384) UINT cNames,
		/* [in] */ LCID lcid,
		/* [size_is][out] */ __RPC__out_ecount_full(cNames) DISPID *rgDispId){  return S_OK; } ;

	/* [local] */ HRESULT STDMETHODCALLTYPE Invoke(
		/* [annotation][in] */
		_In_  DISPID dispIdMember,
		/* [annotation][in] */
		_In_  REFIID riid,
		/* [annotation][in] */
		_In_  LCID lcid,
		/* [annotation][in] */
		_In_  WORD wFlags,
		/* [annotation][out][in] */
		_In_  DISPPARAMS *pDispParams,
		/* [annotation][out] */
		_Out_opt_  VARIANT *pVarResult,
		/* [annotation][out] */
		_Out_opt_  EXCEPINFO *pExcepInfo,
		/* [annotation][out] */
		_Out_opt_  UINT *puArgErr){  return S_OK; } ;

	//From IUnkonwn
	HRESULT __stdcall QueryInterface(
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
	{
		return S_OK;
	};

	ULONG __stdcall AddRef(void)
	{
		return S_OK;
	};

	ULONG __stdcall Release(void)
	{
		return S_OK;
	};
};

class myBinder : _Binder
{
	//
	// Raw methods provided by interface
	//

	HRESULT __stdcall get_ToString(
		/*[out,retval]*/ BSTR * pRetVal){  return S_OK; } ;
	HRESULT __stdcall Equals(
		/*[in]*/ VARIANT obj,
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetHashCode(
		/*[out,retval]*/ long * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetType(
		/*[out,retval]*/ struct _Type * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall BindToMethod(
		/*[in]*/ enum BindingFlags bindingAttr,
		/*[in]*/ SAFEARRAY * match,
		/*[in,out]*/ SAFEARRAY * * args,
		/*[in]*/ SAFEARRAY * modifiers,
		/*[in]*/ struct _CultureInfo * culture,
		/*[in]*/ SAFEARRAY * names,
		/*[out]*/ VARIANT * state,
		/*[out,retval]*/ struct _MethodBase * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall BindToField(
		/*[in]*/ enum BindingFlags bindingAttr,
		/*[in]*/ SAFEARRAY * match,
		/*[in]*/ VARIANT value,
		/*[in]*/ struct _CultureInfo * culture,
		/*[out,retval]*/ struct _FieldInfo * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall SelectMethod(
		/*[in]*/ enum BindingFlags bindingAttr,
		/*[in]*/ SAFEARRAY * match,
		/*[in]*/ SAFEARRAY * types,
		/*[in]*/ SAFEARRAY * modifiers,
		/*[out,retval]*/ struct _MethodBase * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall SelectProperty(
		/*[in]*/ enum BindingFlags bindingAttr,
		/*[in]*/ SAFEARRAY * match,
		/*[in]*/ struct _Type * returnType,
		/*[in]*/ SAFEARRAY * indexes,
		/*[in]*/ SAFEARRAY * modifiers,
		/*[out,retval]*/ struct _PropertyInfo * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall ChangeType(
		/*[in]*/ VARIANT value,
		/*[in]*/ struct _Type * Type,
		/*[in]*/ struct _CultureInfo * culture,
		/*[out,retval]*/ VARIANT * pRetVal){  return S_OK; } ;
	HRESULT __stdcall ReorderArgumentArray(
		/*[in,out]*/ SAFEARRAY * * args,
		/*[in]*/ VARIANT state){  return S_OK; } ;

	//From IDispatch:
	HRESULT STDMETHODCALLTYPE GetTypeInfoCount(
		/* [out] */ __RPC__out UINT *pctinfo) {
		return S_OK;
	};

	HRESULT STDMETHODCALLTYPE GetTypeInfo(
		/* [in] */ UINT iTInfo,
		/* [in] */ LCID lcid,
		/* [out] */ __RPC__deref_out_opt ITypeInfo **ppTInfo) {
		return S_OK;
	};

	HRESULT STDMETHODCALLTYPE GetIDsOfNames(
		/* [in] */ __RPC__in REFIID riid,
		/* [size_is][in] */ __RPC__in_ecount_full(cNames) LPOLESTR *rgszNames,
		/* [range][in] */ __RPC__in_range(0, 16384) UINT cNames,
		/* [in] */ LCID lcid,
		/* [size_is][out] */ __RPC__out_ecount_full(cNames) DISPID *rgDispId) {
		return S_OK;
	};

	/* [local] */ HRESULT STDMETHODCALLTYPE Invoke(
		/* [annotation][in] */
		_In_  DISPID dispIdMember,
		/* [annotation][in] */
		_In_  REFIID riid,
		/* [annotation][in] */
		_In_  LCID lcid,
		/* [annotation][in] */
		_In_  WORD wFlags,
		/* [annotation][out][in] */
		_In_  DISPPARAMS *pDispParams,
		/* [annotation][out] */
		_Out_opt_  VARIANT *pVarResult,
		/* [annotation][out] */
		_Out_opt_  EXCEPINFO *pExcepInfo,
		/* [annotation][out] */
		_Out_opt_  UINT *puArgErr) {
		return S_OK;
	};

	//From IUnkonwn
	HRESULT __stdcall QueryInterface(
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
	{
		return S_OK;
	};

	ULONG __stdcall AddRef(void)
	{
		return S_OK;
	};

	ULONG __stdcall Release(void)
	{
		return S_OK;
	};
};

//struct __declspec(uuid("bca8b44d-aad6-3a86-8ab7-03349f4f2da2"))
class my_Type: public _Type
{
	//
	// Raw methods provided by interface
	//

	HRESULT __stdcall GetTypeInfoCount(
		/*[out]*/ unsigned long * pcTInfo){  return S_OK; } ;
	HRESULT __stdcall GetTypeInfo(
		/*[in]*/ unsigned long iTInfo,
		/*[in]*/ unsigned long lcid,
		/*[in]*/ long ppTInfo){  return S_OK; } ;
	HRESULT __stdcall GetIDsOfNames(
		/*[in]*/ GUID * riid,
		/*[in]*/ long rgszNames,
		/*[in]*/ unsigned long cNames,
		/*[in]*/ unsigned long lcid,
		/*[in]*/ long rgDispId){  return S_OK; } ;
	HRESULT __stdcall Invoke(
		/*[in]*/ unsigned long dispIdMember,
		/*[in]*/ GUID * riid,
		/*[in]*/ unsigned long lcid,
		/*[in]*/ short wFlags,
		/*[in]*/ long pDispParams,
		/*[in]*/ long pVarResult,
		/*[in]*/ long pExcepInfo,
		/*[in]*/ long puArgErr){  return S_OK; } ;
	HRESULT __stdcall get_ToString(
		/*[out,retval]*/ BSTR * pRetVal){  return S_OK; } ;
	HRESULT __stdcall Equals(
		/*[in]*/ VARIANT other,
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetHashCode(
		/*[out,retval]*/ long * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetType(
		/*[out,retval]*/ struct _Type * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_MemberType(
		/*[out,retval]*/ enum MemberTypes * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_name(
		/*[out,retval]*/ BSTR * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_DeclaringType(
		/*[out,retval]*/ struct _Type * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_ReflectedType(
		/*[out,retval]*/ struct _Type * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetCustomAttributes(
		/*[in]*/ struct _Type * attributeType,
		/*[in]*/ VARIANT_BOOL inherit,
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetCustomAttributes_2(
		/*[in]*/ VARIANT_BOOL inherit,
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall IsDefined(
		/*[in]*/ struct _Type * attributeType,
		/*[in]*/ VARIANT_BOOL inherit,
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_Guid(
		/*[out,retval]*/ GUID * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_Module(
		/*[out,retval]*/ struct _Module * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_Assembly(
		/*[out,retval]*/ struct _Assembly * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_TypeHandle(
		/*[out,retval]*/ struct RuntimeTypeHandle * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_FullName(
		/*[out,retval]*/ BSTR * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_Namespace(
		/*[out,retval]*/ BSTR * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_AssemblyQualifiedName(
		/*[out,retval]*/ BSTR * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetArrayRank(
		/*[out,retval]*/ long * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_BaseType(
		/*[out,retval]*/ struct _Type * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetConstructors(
		/*[in]*/ enum BindingFlags bindingAttr,
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetInterface(
		/*[in]*/ BSTR name,
		/*[in]*/ VARIANT_BOOL ignoreCase,
		/*[out,retval]*/ struct _Type * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetInterfaces(
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall FindInterfaces(
		/*[in]*/ struct _TypeFilter * filter,
		/*[in]*/ VARIANT filterCriteria,
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetEvent(
		/*[in]*/ BSTR name,
		/*[in]*/ enum BindingFlags bindingAttr,
		/*[out,retval]*/ struct _EventInfo * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetEvents(
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetEvents_2(
		/*[in]*/ enum BindingFlags bindingAttr,
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetNestedTypes(
		/*[in]*/ enum BindingFlags bindingAttr,
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetNestedType(
		/*[in]*/ BSTR name,
		/*[in]*/ enum BindingFlags bindingAttr,
		/*[out,retval]*/ struct _Type * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetMember(
		/*[in]*/ BSTR name,
		/*[in]*/ enum MemberTypes Type,
		/*[in]*/ enum BindingFlags bindingAttr,
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetDefaultMembers(
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall FindMembers(
		/*[in]*/ enum MemberTypes MemberType,
		/*[in]*/ enum BindingFlags bindingAttr,
		/*[in]*/ struct _MemberFilter * filter,
		/*[in]*/ VARIANT filterCriteria,
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetElementType(
		/*[out,retval]*/ struct _Type * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall IsSubclassOf(
		/*[in]*/ struct _Type * c,
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall IsInstanceOfType(
		/*[in]*/ VARIANT o,
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall IsAssignableFrom(
		/*[in]*/ struct _Type * c,
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetInterfaceMap(
		/*[in]*/ struct _Type * interfaceType,
		/*[out,retval]*/ struct InterfaceMapping * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetMethod(
		/*[in]*/ BSTR name,
		/*[in]*/ enum BindingFlags bindingAttr,
		/*[in]*/ struct _Binder * Binder,
		/*[in]*/ SAFEARRAY * types,
		/*[in]*/ SAFEARRAY * modifiers,
		/*[out,retval]*/ struct _MethodInfo * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetMethod_2(
		/*[in]*/ BSTR name,
		/*[in]*/ enum BindingFlags bindingAttr,
		/*[out,retval]*/ struct _MethodInfo * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetMethods(
		/*[in]*/ enum BindingFlags bindingAttr,
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetField(
		/*[in]*/ BSTR name,
		/*[in]*/ enum BindingFlags bindingAttr,
		/*[out,retval]*/ struct _FieldInfo * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetFields(
		/*[in]*/ enum BindingFlags bindingAttr,
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetProperty(
		/*[in]*/ BSTR name,
		/*[in]*/ enum BindingFlags bindingAttr,
		/*[out,retval]*/ struct _PropertyInfo * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetProperty_2(
		/*[in]*/ BSTR name,
		/*[in]*/ enum BindingFlags bindingAttr,
		/*[in]*/ struct _Binder * Binder,
		/*[in]*/ struct _Type * returnType,
		/*[in]*/ SAFEARRAY * types,
		/*[in]*/ SAFEARRAY * modifiers,
		/*[out,retval]*/ struct _PropertyInfo * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetProperties(
		/*[in]*/ enum BindingFlags bindingAttr,
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetMember_2(
		/*[in]*/ BSTR name,
		/*[in]*/ enum BindingFlags bindingAttr,
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetMembers(
		/*[in]*/ enum BindingFlags bindingAttr,
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall InvokeMember(
		/*[in]*/ BSTR name,
		/*[in]*/ enum BindingFlags invokeAttr,
		/*[in]*/ struct _Binder * Binder,
		/*[in]*/ VARIANT Target,
		/*[in]*/ SAFEARRAY * args,
		/*[in]*/ SAFEARRAY * modifiers,
		/*[in]*/ struct _CultureInfo * culture,
		/*[in]*/ SAFEARRAY * namedParameters,
		/*[out,retval]*/ VARIANT * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_UnderlyingSystemType(
		/*[out,retval]*/ struct _Type * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall InvokeMember_2(
		/*[in]*/ BSTR name,
		/*[in]*/ enum BindingFlags invokeAttr,
		/*[in]*/ struct _Binder * Binder,
		/*[in]*/ VARIANT Target,
		/*[in]*/ SAFEARRAY * args,
		/*[in]*/ struct _CultureInfo * culture,
		/*[out,retval]*/ VARIANT * pRetVal){  return S_OK; } ;
	HRESULT __stdcall InvokeMember_3(
		/*[in]*/ BSTR name,
		/*[in]*/ enum BindingFlags invokeAttr,
		/*[in]*/ struct _Binder * Binder,
		/*[in]*/ VARIANT Target,
		/*[in]*/ SAFEARRAY * args,
		/*[out,retval]*/ VARIANT * pRetVal);
	HRESULT __stdcall GetConstructor(
		/*[in]*/ enum BindingFlags bindingAttr,
		/*[in]*/ struct _Binder * Binder,
		/*[in]*/ enum CallingConventions callConvention,
		/*[in]*/ SAFEARRAY * types,
		/*[in]*/ SAFEARRAY * modifiers,
		/*[out,retval]*/ struct _ConstructorInfo * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetConstructor_2(
		/*[in]*/ enum BindingFlags bindingAttr,
		/*[in]*/ struct _Binder * Binder,
		/*[in]*/ SAFEARRAY * types,
		/*[in]*/ SAFEARRAY * modifiers,
		/*[out,retval]*/ struct _ConstructorInfo * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetConstructor_3(
		/*[in]*/ SAFEARRAY * types,
		/*[out,retval]*/ struct _ConstructorInfo * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetConstructors_2(
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_TypeInitializer(
		/*[out,retval]*/ struct _ConstructorInfo * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetMethod_3(
		/*[in]*/ BSTR name,
		/*[in]*/ enum BindingFlags bindingAttr,
		/*[in]*/ struct _Binder * Binder,
		/*[in]*/ enum CallingConventions callConvention,
		/*[in]*/ SAFEARRAY * types,
		/*[in]*/ SAFEARRAY * modifiers,
		/*[out,retval]*/ struct _MethodInfo * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetMethod_4(
		/*[in]*/ BSTR name,
		/*[in]*/ SAFEARRAY * types,
		/*[in]*/ SAFEARRAY * modifiers,
		/*[out,retval]*/ struct _MethodInfo * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetMethod_5(
		/*[in]*/ BSTR name,
		/*[in]*/ SAFEARRAY * types,
		/*[out,retval]*/ struct _MethodInfo * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetMethod_6(
		/*[in]*/ BSTR name,
		/*[out,retval]*/ struct _MethodInfo * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetMethods_2(
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetField_2(
		/*[in]*/ BSTR name,
		/*[out,retval]*/ struct _FieldInfo * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetFields_2(
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetInterface_2(
		/*[in]*/ BSTR name,
		/*[out,retval]*/ struct _Type * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetEvent_2(
		/*[in]*/ BSTR name,
		/*[out,retval]*/ struct _EventInfo * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetProperty_3(
		/*[in]*/ BSTR name,
		/*[in]*/ struct _Type * returnType,
		/*[in]*/ SAFEARRAY * types,
		/*[in]*/ SAFEARRAY * modifiers,
		/*[out,retval]*/ struct _PropertyInfo * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetProperty_4(
		/*[in]*/ BSTR name,
		/*[in]*/ struct _Type * returnType,
		/*[in]*/ SAFEARRAY * types,
		/*[out,retval]*/ struct _PropertyInfo * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetProperty_5(
		/*[in]*/ BSTR name,
		/*[in]*/ SAFEARRAY * types,
		/*[out,retval]*/ struct _PropertyInfo * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetProperty_6(
		/*[in]*/ BSTR name,
		/*[in]*/ struct _Type * returnType,
		/*[out,retval]*/ struct _PropertyInfo * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetProperty_7(
		/*[in]*/ BSTR name,
		/*[out,retval]*/ struct _PropertyInfo * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetProperties_2(
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetNestedTypes_2(
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetNestedType_2(
		/*[in]*/ BSTR name,
		/*[out,retval]*/ struct _Type * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetMember_3(
		/*[in]*/ BSTR name,
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall GetMembers_2(
		/*[out,retval]*/ SAFEARRAY * * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_Attributes(
		/*[out,retval]*/ enum TypeAttributes * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsNotPublic(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsPublic(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsNestedPublic(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsNestedPrivate(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsNestedFamily(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsNestedAssembly(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsNestedFamANDAssem(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsNestedFamORAssem(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsAutoLayout(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsLayoutSequential(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsExplicitLayout(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsClass(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsInterface(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsValueType(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsAbstract(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsSealed(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsEnum(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsSpecialName(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsImport(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsSerializable(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsAnsiClass(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsUnicodeClass(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsAutoClass(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsArray(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsByRef(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsPointer(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsPrimitive(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsCOMObject(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_HasElementType(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsContextful(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall get_IsMarshalByRef(
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;
	HRESULT __stdcall Equals_2(
		/*[in]*/ struct _Type * o,
		/*[out,retval]*/ VARIANT_BOOL * pRetVal){  return S_OK; } ;

	//From IUnkonwn
	HRESULT __stdcall QueryInterface(
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
	{
		return S_OK;
	};

	ULONG __stdcall AddRef(void)
	{
		return S_OK;
	};

	ULONG __stdcall Release(void)
	{
		return S_OK;
	};
};

TYPED_MOCK_CLASS(CDOTNETMocks, CGlobalMock)
{
public:
    // CLR/COM Mocks
	MOCK_STATIC_METHOD_3(, HRESULT, CLRCreateInstance, REFCLSID, clsid, REFIID, riid, LPVOID *, ppInterface)
		myICLRMetaHost* myCLRInstance = new myICLRMetaHost();
		*ppInterface = myCLRInstance;
	MOCK_METHOD_END(HRESULT, S_OK);

	MOCK_STATIC_METHOD_3(, HRESULT, GetRuntime, LPCWSTR, pwzVersion, REFIID, riid, LPVOID *, ppRuntime)
		myICLRRuntimeInfo* myCLRRunTimeInstance = new myICLRRuntimeInfo();
		*ppRuntime = myCLRRunTimeInstance;
	MOCK_METHOD_END(HRESULT, S_OK);

	MOCK_STATIC_METHOD_1(, HRESULT, IsLoadable, BOOL*, pbLoadable)
		*pbLoadable = global_IsLoadable;
	MOCK_METHOD_END(HRESULT, S_OK);

	MOCK_STATIC_METHOD_3(, HRESULT, GetInterface, REFCLSID, rclsid, REFIID, riid, LPVOID *, ppUnk)
		myICorRuntimeHost* myICorRuntimeHostInstance = new myICorRuntimeHost();
	    *ppUnk = myICorRuntimeHostInstance;
	MOCK_METHOD_END(HRESULT, S_OK);

	MOCK_STATIC_METHOD_0(, HRESULT, Start)
	MOCK_METHOD_END(HRESULT, S_OK);

	MOCK_STATIC_METHOD_1(, HRESULT, GetDefaultDomain, IUnknown **, pAppDomain)
		myIUnkonwnClass* myUnkonwnClassInstance = new myIUnkonwnClass();
		*pAppDomain = myUnkonwnClassInstance;
	MOCK_METHOD_END(HRESULT, S_OK);

	MOCK_STATIC_METHOD_2(, HRESULT, QueryInterface, REFIID, riid, LPVOID *, ppUnk)
		myAppDomain* myAppDomainInstance = new myAppDomain();
     	*ppUnk = myAppDomainInstance;
	MOCK_METHOD_END(HRESULT, S_OK);

	MOCK_STATIC_METHOD_2(, HRESULT, Load_2, BSTR, assemblyString, _Assembly * *, pRetVal)
		myAssemblyClass* myAssemblyClassInstance = new myAssemblyClass();
		*pRetVal = myAssemblyClassInstance;
	MOCK_METHOD_END(HRESULT, S_OK);

	MOCK_STATIC_METHOD_2(, HRESULT, GetType_2, BSTR, name, _Type * *, pRetVal);
		my_Type* my_TypeInstance = new my_Type();
		*pRetVal = my_TypeInstance;
	MOCK_METHOD_END(HRESULT, S_OK);

	MOCK_STATIC_METHOD_8(, HRESULT, CreateInstance_3, BSTR, typeName, VARIANT_BOOL, ignoreCase, enum BindingFlags, bindingAttr, _Binder*, binder, SAFEARRAY*, args_entry, _CultureInfo*, culture, SAFEARRAY *, activationAttributes, VARIANT*, pRetVal)
		MOCK_METHOD_END(HRESULT, S_OK);

	MOCK_STATIC_METHOD_2(, HRESULT, CreateInstance, BSTR, typeName, VARIANT *, pRetVal);
	MOCK_METHOD_END(HRESULT, S_OK);


	MOCK_STATIC_METHOD_6(, HRESULT, InvokeMember_3, BSTR, name, enum BindingFlags, invokeAttr, struct _Binder *, Binder, VARIANT, Target, SAFEARRAY*, args_entry, VARIANT*, pRetVal)
	MOCK_METHOD_END(HRESULT, S_OK);

	//Safe Array mocks
	MOCK_STATIC_METHOD_3(, SAFEARRAY *, myTest_SafeArrayCreateVector, VARTYPE, vt, LONG, lLbound, ULONG, cElements)
	MOCK_METHOD_END(SAFEARRAY *, (SAFEARRAY *)0x42);

	
	MOCK_STATIC_METHOD_3(, HRESULT, myTest_SafeArrayPutElement, SAFEARRAY *, psa, LONG*, rgIndices, void *, pv)
	MOCK_METHOD_END(HRESULT, S_OK);


	MOCK_STATIC_METHOD_3(, SAFEARRAY *, myTest_SafeArrayCreate, VARTYPE, vt, UINT, cDims, SAFEARRAYBOUND *, rgsabound)
		SAFEARRAY * resultSafeArray = SafeArrayCreate(vt, cDims, rgsabound);
	MOCK_METHOD_END(SAFEARRAY *, resultSafeArray);

	MOCK_STATIC_METHOD_1(, HRESULT, myTest_SafeArrayDestroy, SAFEARRAY *, psa)
	MOCK_METHOD_END(HRESULT, S_OK);

	MOCK_STATIC_METHOD_2(, HRESULT, myTest_SafeArrayAccessData, SAFEARRAY *, psa, void**, ppvData)
		*ppvData = psa->pvData;
	MOCK_METHOD_END(HRESULT, S_OK);

	MOCK_STATIC_METHOD_1(, HRESULT, myTest_SafeArrayUnaccessData, SAFEARRAY *, psa)
	MOCK_METHOD_END(HRESULT, S_OK);

	//Message Mocks
	MOCK_STATIC_METHOD_2(, const unsigned char*, Message_ToByteArray, MESSAGE_HANDLE, messageHandle, int32_t *, size)	
	     *size = 11;
	MOCK_METHOD_END(const unsigned char*, (const unsigned char*)"AnyContent");

	
	MOCK_STATIC_METHOD_2(, MESSAGE_HANDLE, Message_CreateFromByteArray, const unsigned char*, source, int32_t, size)
	MOCK_METHOD_END(MESSAGE_HANDLE, (MESSAGE_HANDLE)0x42);

	MOCK_STATIC_METHOD_1(, void, Message_Destroy, MESSAGE_HANDLE, message)
	MOCK_VOID_METHOD_END()

	// memory
	MOCK_STATIC_METHOD_1(, void*, gballoc_malloc, size_t, size)
		void* result2 = BASEIMPLEMENTATION::gballoc_malloc(size);
	MOCK_METHOD_END(void*, result2);

	MOCK_STATIC_METHOD_1(, void, gballoc_free, void*, ptr)
		BASEIMPLEMENTATION::gballoc_free(ptr);
	MOCK_VOID_METHOD_END()

	//MessageBus Mocks
	MOCK_STATIC_METHOD_3(, MESSAGE_BUS_RESULT, MessageBus_Publish, MESSAGE_BUS_HANDLE, bus, MODULE_HANDLE, source, MESSAGE_HANDLE, message)
	MOCK_METHOD_END(MESSAGE_BUS_RESULT, MESSAGE_BUS_OK);
};

extern "C"
{
	// memory
	DECLARE_GLOBAL_MOCK_METHOD_1(CDOTNETMocks, , void*, gballoc_malloc, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_1(CDOTNETMocks, , void, gballoc_free, void*, ptr);

	// CLR/COM Mocks
	DECLARE_GLOBAL_MOCK_METHOD_3(CDOTNETMocks, HRESULT, __stdcall, CLRCreateInstance, REFCLSID, clsid, REFIID, riid, LPVOID *, ppInterface);

	DECLARE_GLOBAL_MOCK_METHOD_3(CDOTNETMocks, HRESULT, __stdcall, GetRuntime, LPCWSTR, pwzVersion, REFIID, riid, LPVOID *, ppRuntime);
	
	DECLARE_GLOBAL_MOCK_METHOD_1(CDOTNETMocks, HRESULT, __stdcall, IsLoadable, BOOL*, pbLoadable);

	DECLARE_GLOBAL_MOCK_METHOD_3(CDOTNETMocks, HRESULT, __stdcall, GetInterface, REFCLSID, rclsid, REFIID, riid, LPVOID *, ppUnk);

	DECLARE_GLOBAL_MOCK_METHOD_0(CDOTNETMocks, HRESULT, __stdcall, Start);

	DECLARE_GLOBAL_MOCK_METHOD_1(CDOTNETMocks, HRESULT, __stdcall, GetDefaultDomain, IUnknown **, pAppDomain);

	DECLARE_GLOBAL_MOCK_METHOD_2(CDOTNETMocks, HRESULT, __stdcall, QueryInterface, REFIID, riid, LPVOID *, ppUnk);

	DECLARE_GLOBAL_MOCK_METHOD_2(CDOTNETMocks, HRESULT, __stdcall, Load_2, BSTR, assemblyString, _Assembly * *, pRetVal);

	DECLARE_GLOBAL_MOCK_METHOD_2(CDOTNETMocks, HRESULT, __stdcall, GetType_2, BSTR, name, _Type * *, pRetVal);
	
	DECLARE_GLOBAL_MOCK_METHOD_8(CDOTNETMocks, HRESULT, __stdcall, CreateInstance_3, BSTR, typeName, VARIANT_BOOL, ignoreCase, enum BindingFlags, bindingAttr, struct _Binder*, Binder, SAFEARRAY*, args, struct _CultureInfo*, culture, SAFEARRAY *, activationAttributes, VARIANT*, pRetVal);

	DECLARE_GLOBAL_MOCK_METHOD_2(CDOTNETMocks, HRESULT, __stdcall, CreateInstance, BSTR, typeName, VARIANT *, pRetVal);
	
	DECLARE_GLOBAL_MOCK_METHOD_6(CDOTNETMocks, HRESULT, __stdcall, InvokeMember_3, BSTR, name, enum BindingFlags, invokeAttr, struct _Binder *, Binder, VARIANT, Target, SAFEARRAY*, args_entry, VARIANT*, pRetVal);

	//Safe Array mocks
	DECLARE_GLOBAL_MOCK_METHOD_3(CDOTNETMocks, SAFEARRAY *, __stdcall, myTest_SafeArrayCreateVector, VARTYPE, vt, LONG, lLbound, ULONG, cElements);
	
	DECLARE_GLOBAL_MOCK_METHOD_3(CDOTNETMocks, HRESULT, __stdcall, myTest_SafeArrayPutElement, SAFEARRAY *, psa, LONG*, rgIndices, void *, pv);

	DECLARE_GLOBAL_MOCK_METHOD_3(CDOTNETMocks, SAFEARRAY *, __stdcall, myTest_SafeArrayCreate, VARTYPE, vt, UINT, cDims, SAFEARRAYBOUND *, rgsabound);

	DECLARE_GLOBAL_MOCK_METHOD_2(CDOTNETMocks, HRESULT, __stdcall, myTest_SafeArrayAccessData, SAFEARRAY *, psa, void**, ppvData);
	
	DECLARE_GLOBAL_MOCK_METHOD_1(CDOTNETMocks, HRESULT, __stdcall, myTest_SafeArrayDestroy, SAFEARRAY *, psa);

	DECLARE_GLOBAL_MOCK_METHOD_1(CDOTNETMocks, HRESULT, __stdcall, myTest_SafeArrayUnaccessData, SAFEARRAY *, psa)
		
	//Message Mocks
	DECLARE_GLOBAL_MOCK_METHOD_2(CDOTNETMocks, , const unsigned char*, Message_ToByteArray, MESSAGE_HANDLE, messageHandle, int32_t *, size);

	DECLARE_GLOBAL_MOCK_METHOD_2(CDOTNETMocks, , MESSAGE_HANDLE, Message_CreateFromByteArray, const unsigned char*, source, int32_t, size);

	DECLARE_GLOBAL_MOCK_METHOD_1(CDOTNETMocks, , void, Message_Destroy, MESSAGE_HANDLE, message);

	//MessageBus Mocks
	DECLARE_GLOBAL_MOCK_METHOD_3(CDOTNETMocks, , MESSAGE_BUS_RESULT, MessageBus_Publish, MESSAGE_BUS_HANDLE, bus, MODULE_HANDLE, source, MESSAGE_HANDLE, message);

}

HRESULT myICLRMetaHost::GetRuntime(
		/* [in] */ LPCWSTR pwzVersion,
		/* [in] */ REFIID riid,
		/* [retval][iid_is][out] */ LPVOID *ppRuntime)
	{
		return ::GetRuntime(pwzVersion, riid, ppRuntime);
	};

HRESULT myICLRRuntimeInfo::IsLoadable(
	/* [retval][out] */ BOOL *pbLoadable)
{
	return ::IsLoadable(pbLoadable);
};

HRESULT myICLRRuntimeInfo::GetInterface(
	/* [in] */ REFCLSID rclsid,
	/* [in] */ REFIID riid,
	/* [retval][iid_is][out] */ LPVOID *ppUnk)
{
	return ::GetInterface(rclsid, riid, ppUnk);
};

HRESULT myICorRuntimeHost::Start(void)
{
	return ::Start();
};

HRESULT myICorRuntimeHost::GetDefaultDomain(
	/* [out] */ IUnknown **pAppDomain)
{
	return ::GetDefaultDomain(pAppDomain);
};

HRESULT myIUnkonwnClass::QueryInterface(
	/* [in] */ REFIID riid,
	/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
{
	return ::QueryInterface(riid, ppvObject);
}

HRESULT myAppDomain::Load_2(
	/*[in]*/ BSTR assemblyString,
	/*[out,retval]*/ struct _Assembly * * pRetVal) {
	return ::Load_2(assemblyString, pRetVal);
};

HRESULT myAssemblyClass::GetType_2(
	/*[in]*/ BSTR name,
	/*[out,retval]*/ struct _Type * * pRetVal) {
	return ::GetType_2(name, pRetVal);
};

HRESULT myAssemblyClass::CreateInstance_3(
	/*[in]*/ BSTR typeName,
	/*[in]*/ VARIANT_BOOL ignoreCase,
	/*[in]*/ enum BindingFlags bindingAttr,
	/*[in]*/ struct _Binder * Binder,
	/*[in]*/ SAFEARRAY * args,
	/*[in]*/ struct _CultureInfo * culture,
	/*[in]*/ SAFEARRAY * activationAttributes,
	/*[out,retval]*/ VARIANT * pRetVal)
{
	return ::CreateInstance_3(typeName, ignoreCase, bindingAttr, Binder, args, culture, activationAttributes, pRetVal);
};

HRESULT myAssemblyClass::CreateInstance(
	/*[in]*/ BSTR typeName,
	/*[out,retval]*/ VARIANT * pRetVal)
{
	return ::CreateInstance(typeName, pRetVal);
};

HRESULT my_Type::InvokeMember_3(
	/*[in]*/ BSTR name,
	/*[in]*/ enum BindingFlags invokeAttr,
	/*[in]*/ struct _Binder * Binder,
	/*[in]*/ VARIANT Target,
	/*[in]*/ SAFEARRAY * args,
	/*[out,retval]*/ VARIANT * pRetVal)
{
	return ::InvokeMember_3(name, invokeAttr, Binder, Target, args, pRetVal);
};

BEGIN_TEST_SUITE(dotnet_unittests)
    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
        g_testByTest = MicroMockCreateMutex();
        ASSERT_IS_NOT_NULL(g_testByTest);
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        MicroMockDestroyMutex(g_testByTest);
        TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
    }

    TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
    {
		currentnew_call = 0;
		whenShallnew_fail = 0;

		global_IsLoadable = true;

        if (!MicroMockAcquireMutex(g_testByTest))
        {
            ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");

			
        }
    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
        if (!MicroMockReleaseMutex(g_testByTest))
        {
            ASSERT_FAIL("failure in test framework at ReleaseMutex");
        }
    }
	
	/* Tests_SRS_DOTNET_04_001: [ DotNET_Create shall return NULL if bus is NULL. ] */
	TEST_FUNCTION(DotNET_Create_returns_NULL_when_bus_is_Null)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		mocks.ResetAllCalls();

		///act
		auto  result = theAPIS->Module_Create(NULL, "Anything");

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_002: [ DotNET_Create shall return NULL if configuration is NULL. ] */
	TEST_FUNCTION(DotNET_Create_returns_NULL_when_configuration_is_Null)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		mocks.ResetAllCalls();

		///act
		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, NULL);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}
	
	/* Tests_SRS_DOTNET_04_003: [ DotNET_Create shall return NULL if configuration->dotnet_module_path is NULL. ] */
	TEST_FUNCTION(DotNET_Create_returns_NULL_when_dotnet_module_path_is_Null)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = NULL;
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";

		mocks.ResetAllCalls();

		///act
		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_004: [ DotNET_Create shall return NULL if configuration->dotnet_module_entry_class is NULL. ] */
	TEST_FUNCTION(DotNET_Create_returns_NULL_when_dotnet_module_entry_class_is_Null)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = NULL;
		dotNetConfig.dotnet_module_args = "module configuration";

		mocks.ResetAllCalls();

		///act
		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_005: [ DotNET_Create shall return NULL if configuration->dotnet_module_args is NULL. ] */
	TEST_FUNCTION(DotNET_Create_returns_NULL_when_dotnet_module_args_is_Null)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = NULL;

		mocks.ResetAllCalls();

		///act
		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
	TEST_FUNCTION(DotNET_Create_returns_NULL_when_new_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";
		bstr_t bstrClientModuleAssemblyName(dotNetConfig.dotnet_module_path);
		bstr_t bstrClientModuleClassName(dotNetConfig.dotnet_module_entry_class);
		bstr_t bstrAzureIoTGatewayAssemblyName(L"Microsoft.Azure.IoT.Gateway");
		bstr_t bstrAzureIoTGatewayMessageBusClassName(L"Microsoft.Azure.IoT.Gateway.MessageBus");
		bstr_t bstrAzureIoTGatewayMessageClassName(L"Microsoft.Azure.IoT.Gateway.Message");
		bstr_t bstrCreateClientMethodName(L"Create");
		variant_t emptyVariant(0);
		whenShallnew_fail = currentnew_call + 4;

		mocks.ResetAllCalls();

		///act
		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
	TEST_FUNCTION(DotNET_Create_returns_NULL_when_CLRCreateInstance_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, CLRCreateInstance(CLSID_CLRMetaHost, CLR_METAHOST_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3)
			.SetFailReturn((HRESULT)E_POINTER);

		///act
		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
	TEST_FUNCTION(DotNET_Create_returns_NULL_when_GetRuntime_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, CLRCreateInstance(CLSID_CLRMetaHost_UUID, CLR_METAHOST_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, GetRuntime(DefaultCLRVersion, CLSID_CLRRunTime_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3)
			.SetFailReturn((HRESULT)E_POINTER);

		///act
		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
	TEST_FUNCTION(DotNET_Create_returns_NULL_when_IsLoadable_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, CLRCreateInstance(CLSID_CLRMetaHost_UUID, CLR_METAHOST_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, GetRuntime(DefaultCLRVersion, CLSID_CLRRunTime_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, IsLoadable(IGNORED_PTR_ARG))
			.IgnoreAllArguments()
			.SetFailReturn((HRESULT)E_POINTER);

		///act
		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
	TEST_FUNCTION(DotNET_Create_returns_NULL_when_IsLoadable_fLoadable_is_False)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";

		global_IsLoadable = false;

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, CLRCreateInstance(CLSID_CLRMetaHost_UUID, CLR_METAHOST_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, GetRuntime(DefaultCLRVersion, CLSID_CLRRunTime_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, IsLoadable(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		///act
		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
	TEST_FUNCTION(DotNET_Create_returns_NULL_when_GetInterface_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, CLRCreateInstance(CLSID_CLRMetaHost_UUID, CLR_METAHOST_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, GetRuntime(DefaultCLRVersion, CLSID_CLRRunTime_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, IsLoadable(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, GetInterface(CLSID_CorRuntimeHost_UUID, ICorRuntimeHost_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3)
			.SetFailReturn((HRESULT)E_POINTER);



		///act
		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
	TEST_FUNCTION(DotNET_Create_returns_NULL_when_CorRunTimeHost_Start_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, CLRCreateInstance(CLSID_CLRMetaHost_UUID, CLR_METAHOST_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, GetRuntime(DefaultCLRVersion, CLSID_CLRRunTime_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, IsLoadable(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, GetInterface(CLSID_CorRuntimeHost_UUID, ICorRuntimeHost_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, Start())
			.SetFailReturn((HRESULT)E_POINTER);

		
		///act
		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}


	/* Tests_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
	TEST_FUNCTION(DotNET_Create_returns_NULL_when_CorRunTimeHost_GetDefaultDomain_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, CLRCreateInstance(CLSID_CLRMetaHost_UUID, CLR_METAHOST_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, GetRuntime(DefaultCLRVersion, CLSID_CLRRunTime_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, IsLoadable(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, GetInterface(CLSID_CorRuntimeHost_UUID, ICorRuntimeHost_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, Start());

		STRICT_EXPECTED_CALL(mocks, GetDefaultDomain(IGNORED_PTR_ARG))
			.IgnoreAllArguments()
			.SetFailReturn((HRESULT)E_POINTER);



		///act
		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
	TEST_FUNCTION(DotNET_Create_returns_NULL_when_QueryInterface_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, CLRCreateInstance(CLSID_CLRMetaHost_UUID, CLR_METAHOST_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, GetRuntime(DefaultCLRVersion, CLSID_CLRRunTime_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, IsLoadable(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, GetInterface(CLSID_CorRuntimeHost_UUID, ICorRuntimeHost_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, Start());

		STRICT_EXPECTED_CALL(mocks, GetDefaultDomain(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, QueryInterface(AppDomainUUID, IGNORED_PTR_ARG))
			.IgnoreArgument(2)
			.SetFailReturn((HRESULT)E_POINTER);



		///act
		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
	TEST_FUNCTION(DotNET_Create_returns_NULL_when_Load_2_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";
		bstr_t bstrClientModuleAssemblyName(dotNetConfig.dotnet_module_path);

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, CLRCreateInstance(CLSID_CLRMetaHost_UUID, CLR_METAHOST_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, GetRuntime(DefaultCLRVersion, CLSID_CLRRunTime_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, IsLoadable(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, GetInterface(CLSID_CorRuntimeHost_UUID, ICorRuntimeHost_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, Start());

		STRICT_EXPECTED_CALL(mocks, GetDefaultDomain(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, QueryInterface(AppDomainUUID, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, Load_2(bstrClientModuleAssemblyName, IGNORED_PTR_ARG))
			.IgnoreArgument(2)
			.SetFailReturn((HRESULT)E_POINTER);



		///act
		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
	TEST_FUNCTION(DotNET_Create_returns_NULL_when_GetType_2_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";
		bstr_t bstrClientModuleAssemblyName(dotNetConfig.dotnet_module_path);
		bstr_t bstrClientModuleClassName(dotNetConfig.dotnet_module_entry_class);

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, CLRCreateInstance(CLSID_CLRMetaHost_UUID, CLR_METAHOST_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, GetRuntime(DefaultCLRVersion, CLSID_CLRRunTime_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, IsLoadable(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, GetInterface(CLSID_CorRuntimeHost_UUID, ICorRuntimeHost_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, Start());

		STRICT_EXPECTED_CALL(mocks, GetDefaultDomain(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, QueryInterface(AppDomainUUID, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, Load_2(bstrClientModuleAssemblyName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);
		

		STRICT_EXPECTED_CALL(mocks, GetType_2(bstrClientModuleClassName, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.SetFailReturn((HRESULT)E_POINTER);

		///act
		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
	TEST_FUNCTION(DotNET_Create_returns_NULL_when_Load_IoTGatewayAssembly_2_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";
		bstr_t bstrClientModuleAssemblyName(dotNetConfig.dotnet_module_path);
		bstr_t bstrClientModuleClassName(dotNetConfig.dotnet_module_entry_class);
		bstr_t bstrAzureIoTGatewayAssemblyName(L"Microsoft.Azure.IoT.Gateway");

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, CLRCreateInstance(CLSID_CLRMetaHost_UUID, CLR_METAHOST_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, GetRuntime(DefaultCLRVersion, CLSID_CLRRunTime_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, IsLoadable(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, GetInterface(CLSID_CorRuntimeHost_UUID, ICorRuntimeHost_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, Start());

		STRICT_EXPECTED_CALL(mocks, GetDefaultDomain(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, QueryInterface(AppDomainUUID, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, Load_2(bstrClientModuleAssemblyName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);


		STRICT_EXPECTED_CALL(mocks, GetType_2(bstrClientModuleClassName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		
		STRICT_EXPECTED_CALL(mocks, Load_2(bstrAzureIoTGatewayAssemblyName, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.SetFailReturn((HRESULT)E_POINTER);

		///act
		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
	TEST_FUNCTION(DotNET_Create_returns_NULL_when_SafeArrayCreateVector_for_MessageBus_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";
		bstr_t bstrClientModuleAssemblyName(dotNetConfig.dotnet_module_path);
		bstr_t bstrClientModuleClassName(dotNetConfig.dotnet_module_entry_class);
		bstr_t bstrAzureIoTGatewayAssemblyName(L"Microsoft.Azure.IoT.Gateway");
		bstr_t bstrAzureIoTGatewayMessageBusClassName(L"Microsoft.Azure.IoT.Gateway.MessageBus");
		bstr_t bstrAzureIoTGatewayMessageClassName(L"Microsoft.Azure.IoT.Gateway.Message");

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, CLRCreateInstance(CLSID_CLRMetaHost_UUID, CLR_METAHOST_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, GetRuntime(DefaultCLRVersion, CLSID_CLRRunTime_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, IsLoadable(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, GetInterface(CLSID_CorRuntimeHost_UUID, ICorRuntimeHost_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, Start());

		STRICT_EXPECTED_CALL(mocks, GetDefaultDomain(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, QueryInterface(AppDomainUUID, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, Load_2(bstrClientModuleAssemblyName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);


		STRICT_EXPECTED_CALL(mocks, GetType_2(bstrClientModuleClassName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);


		STRICT_EXPECTED_CALL(mocks, Load_2(bstrAzureIoTGatewayAssemblyName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreateVector(VT_VARIANT, 0, 2))
			.SetFailReturn((SAFEARRAY*)NULL);

		///act
		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}


	/* Tests_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
	TEST_FUNCTION(DotNET_Create_returns_NULL_when_SafeArrayPutElement_for_MessageBus_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";
		bstr_t bstrClientModuleAssemblyName(dotNetConfig.dotnet_module_path);
		bstr_t bstrClientModuleClassName(dotNetConfig.dotnet_module_entry_class);
		bstr_t bstrAzureIoTGatewayAssemblyName(L"Microsoft.Azure.IoT.Gateway");
		bstr_t bstrAzureIoTGatewayMessageBusClassName(L"Microsoft.Azure.IoT.Gateway.MessageBus");
		bstr_t bstrAzureIoTGatewayMessageClassName(L"Microsoft.Azure.IoT.Gateway.Message");

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, CLRCreateInstance(CLSID_CLRMetaHost_UUID, CLR_METAHOST_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, GetRuntime(DefaultCLRVersion, CLSID_CLRRunTime_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, IsLoadable(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, GetInterface(CLSID_CorRuntimeHost_UUID, ICorRuntimeHost_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, Start());

		STRICT_EXPECTED_CALL(mocks, GetDefaultDomain(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, QueryInterface(AppDomainUUID, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, Load_2(bstrClientModuleAssemblyName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);


		STRICT_EXPECTED_CALL(mocks, GetType_2(bstrClientModuleClassName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);


		STRICT_EXPECTED_CALL(mocks, Load_2(bstrAzureIoTGatewayAssemblyName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreateVector(VT_VARIANT, 0, 2));

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayDestroy(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments()
			.SetFailReturn((HRESULT)E_POINTER);

		///act
		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
	TEST_FUNCTION(DotNET_Create_returns_NULL_when_CreateInstance_3_for_MessageBus_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";
		bstr_t bstrClientModuleAssemblyName(dotNetConfig.dotnet_module_path);
		bstr_t bstrClientModuleClassName(dotNetConfig.dotnet_module_entry_class);
		bstr_t bstrAzureIoTGatewayAssemblyName(L"Microsoft.Azure.IoT.Gateway");
		bstr_t bstrAzureIoTGatewayMessageBusClassName(L"Microsoft.Azure.IoT.Gateway.MessageBus");
		bstr_t bstrAzureIoTGatewayMessageClassName(L"Microsoft.Azure.IoT.Gateway.Message");

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, CLRCreateInstance(CLSID_CLRMetaHost_UUID, CLR_METAHOST_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, GetRuntime(DefaultCLRVersion, CLSID_CLRRunTime_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, IsLoadable(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, GetInterface(CLSID_CorRuntimeHost_UUID, ICorRuntimeHost_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, Start());

		STRICT_EXPECTED_CALL(mocks, GetDefaultDomain(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, QueryInterface(AppDomainUUID, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, Load_2(bstrClientModuleAssemblyName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);


		STRICT_EXPECTED_CALL(mocks, GetType_2(bstrClientModuleClassName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);


		STRICT_EXPECTED_CALL(mocks, Load_2(bstrAzureIoTGatewayAssemblyName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreateVector(VT_VARIANT, 0, 2));

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayDestroy(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, CreateInstance_3(bstrAzureIoTGatewayMessageBusClassName, true, static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public), NULL, IGNORED_PTR_ARG, NULL, NULL, IGNORED_PTR_ARG))
			.IgnoreArgument(5)
			.IgnoreArgument(8)
			.SetFailReturn((HRESULT)E_POINTER);

		///act
		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
	TEST_FUNCTION(DotNET_Create_returns_NULL_when_CreateInstance_for_ClientModuleClass_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";
		bstr_t bstrClientModuleAssemblyName(dotNetConfig.dotnet_module_path);
		bstr_t bstrClientModuleClassName(dotNetConfig.dotnet_module_entry_class);
		bstr_t bstrAzureIoTGatewayAssemblyName(L"Microsoft.Azure.IoT.Gateway");
		bstr_t bstrAzureIoTGatewayMessageBusClassName(L"Microsoft.Azure.IoT.Gateway.MessageBus");
		bstr_t bstrAzureIoTGatewayMessageClassName(L"Microsoft.Azure.IoT.Gateway.Message");

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, CLRCreateInstance(CLSID_CLRMetaHost_UUID, CLR_METAHOST_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, GetRuntime(DefaultCLRVersion, CLSID_CLRRunTime_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, IsLoadable(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, GetInterface(CLSID_CorRuntimeHost_UUID, ICorRuntimeHost_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, Start());

		STRICT_EXPECTED_CALL(mocks, GetDefaultDomain(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, QueryInterface(AppDomainUUID, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, Load_2(bstrClientModuleAssemblyName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);


		STRICT_EXPECTED_CALL(mocks, GetType_2(bstrClientModuleClassName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);


		STRICT_EXPECTED_CALL(mocks, Load_2(bstrAzureIoTGatewayAssemblyName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreateVector(VT_VARIANT, 0, 2));

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayDestroy(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, CreateInstance_3(bstrAzureIoTGatewayMessageBusClassName, true, static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public), NULL, IGNORED_PTR_ARG, NULL, NULL, IGNORED_PTR_ARG))
			.IgnoreArgument(5)
			.IgnoreArgument(8);

		STRICT_EXPECTED_CALL(mocks, CreateInstance(bstrClientModuleClassName, IGNORED_PTR_ARG))
			.IgnoreArgument(2)
			.SetFailReturn((HRESULT)E_POINTER);


		///act
		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}
	
    /* Tests_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
	TEST_FUNCTION(DotNET_Create_returns_NULL_when_SafeArrayCreateVector_ForClientCreate_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";
		bstr_t bstrClientModuleAssemblyName(dotNetConfig.dotnet_module_path);
		bstr_t bstrClientModuleClassName(dotNetConfig.dotnet_module_entry_class);
		bstr_t bstrAzureIoTGatewayAssemblyName(L"Microsoft.Azure.IoT.Gateway");
		bstr_t bstrAzureIoTGatewayMessageBusClassName(L"Microsoft.Azure.IoT.Gateway.MessageBus");
		bstr_t bstrAzureIoTGatewayMessageClassName(L"Microsoft.Azure.IoT.Gateway.Message");

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, CLRCreateInstance(CLSID_CLRMetaHost_UUID, CLR_METAHOST_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, GetRuntime(DefaultCLRVersion, CLSID_CLRRunTime_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, IsLoadable(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, GetInterface(CLSID_CorRuntimeHost_UUID, ICorRuntimeHost_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, Start());

		STRICT_EXPECTED_CALL(mocks, GetDefaultDomain(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, QueryInterface(AppDomainUUID, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, Load_2(bstrClientModuleAssemblyName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);


		STRICT_EXPECTED_CALL(mocks, GetType_2(bstrClientModuleClassName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);


		STRICT_EXPECTED_CALL(mocks, Load_2(bstrAzureIoTGatewayAssemblyName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreateVector(VT_VARIANT, 0, 2));

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayDestroy(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, CreateInstance_3(bstrAzureIoTGatewayMessageBusClassName, true, static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public), NULL, IGNORED_PTR_ARG, NULL, NULL, IGNORED_PTR_ARG))
			.IgnoreArgument(5)
			.IgnoreArgument(8);

		STRICT_EXPECTED_CALL(mocks, CreateInstance(bstrClientModuleClassName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreateVector(VT_VARIANT, 0, 2))
			.SetFailReturn((SAFEARRAY*)NULL);


		///act
		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_006: [ DotNET_Create shall return NULL if an underlying API call fails. ] */
	TEST_FUNCTION(DotNET_Create_returns_NULL_when_SafeArrayPutElement_ARG1_ForClientCreate_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";
		bstr_t bstrClientModuleAssemblyName(dotNetConfig.dotnet_module_path);
		bstr_t bstrClientModuleClassName(dotNetConfig.dotnet_module_entry_class);
		bstr_t bstrAzureIoTGatewayAssemblyName(L"Microsoft.Azure.IoT.Gateway");
		bstr_t bstrAzureIoTGatewayMessageBusClassName(L"Microsoft.Azure.IoT.Gateway.MessageBus");
		bstr_t bstrAzureIoTGatewayMessageClassName(L"Microsoft.Azure.IoT.Gateway.Message");

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, CLRCreateInstance(CLSID_CLRMetaHost_UUID, CLR_METAHOST_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, GetRuntime(DefaultCLRVersion, CLSID_CLRRunTime_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, IsLoadable(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, GetInterface(CLSID_CorRuntimeHost_UUID, ICorRuntimeHost_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, Start());

		STRICT_EXPECTED_CALL(mocks, GetDefaultDomain(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, QueryInterface(AppDomainUUID, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, Load_2(bstrClientModuleAssemblyName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);


		STRICT_EXPECTED_CALL(mocks, GetType_2(bstrClientModuleClassName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);


		STRICT_EXPECTED_CALL(mocks, Load_2(bstrAzureIoTGatewayAssemblyName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreateVector(VT_VARIANT, 0, 2));

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayDestroy(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, CreateInstance_3(bstrAzureIoTGatewayMessageBusClassName, true, static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public), NULL, IGNORED_PTR_ARG, NULL, NULL, IGNORED_PTR_ARG))
			.IgnoreArgument(5)
			.IgnoreArgument(8);

		STRICT_EXPECTED_CALL(mocks, CreateInstance(bstrClientModuleClassName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreateVector(VT_VARIANT, 0, 2));

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayDestroy(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments()
			.SetFailReturn((HRESULT)E_POINTER);




		///act
		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	TEST_FUNCTION(DotNET_Create_returns_NULL_when_SafeArrayPutElement_ARG2_ForClientCreate_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";
		bstr_t bstrClientModuleAssemblyName(dotNetConfig.dotnet_module_path);
		bstr_t bstrClientModuleClassName(dotNetConfig.dotnet_module_entry_class);
		bstr_t bstrAzureIoTGatewayAssemblyName(L"Microsoft.Azure.IoT.Gateway");
		bstr_t bstrAzureIoTGatewayMessageBusClassName(L"Microsoft.Azure.IoT.Gateway.MessageBus");
		bstr_t bstrAzureIoTGatewayMessageClassName(L"Microsoft.Azure.IoT.Gateway.Message");

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, CLRCreateInstance(CLSID_CLRMetaHost_UUID, CLR_METAHOST_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, GetRuntime(DefaultCLRVersion, CLSID_CLRRunTime_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, IsLoadable(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, GetInterface(CLSID_CorRuntimeHost_UUID, ICorRuntimeHost_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, Start());

		STRICT_EXPECTED_CALL(mocks, GetDefaultDomain(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, QueryInterface(AppDomainUUID, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, Load_2(bstrClientModuleAssemblyName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);


		STRICT_EXPECTED_CALL(mocks, GetType_2(bstrClientModuleClassName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);


		STRICT_EXPECTED_CALL(mocks, Load_2(bstrAzureIoTGatewayAssemblyName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreateVector(VT_VARIANT, 0, 2));
		
		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayDestroy(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, CreateInstance_3(bstrAzureIoTGatewayMessageBusClassName, true, static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public), NULL, IGNORED_PTR_ARG, NULL, NULL, IGNORED_PTR_ARG))
			.IgnoreArgument(5)
			.IgnoreArgument(8);

		STRICT_EXPECTED_CALL(mocks, CreateInstance(bstrClientModuleClassName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreateVector(VT_VARIANT, 0, 2));

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayDestroy(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments()
			.SetFailReturn((HRESULT)E_POINTER);

		///act
		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}

	TEST_FUNCTION(DotNET_Create_returns_NULL_when_InvokeMember_3_ForClientCreate_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";
		bstr_t bstrClientModuleAssemblyName(dotNetConfig.dotnet_module_path);
		bstr_t bstrClientModuleClassName(dotNetConfig.dotnet_module_entry_class);
		bstr_t bstrAzureIoTGatewayAssemblyName(L"Microsoft.Azure.IoT.Gateway");
		bstr_t bstrAzureIoTGatewayMessageBusClassName(L"Microsoft.Azure.IoT.Gateway.MessageBus");
		bstr_t bstrAzureIoTGatewayMessageClassName(L"Microsoft.Azure.IoT.Gateway.Message");
		bstr_t bstrCreateClientMethodName(L"Create");
		variant_t emptyVariant(0);

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, CLRCreateInstance(CLSID_CLRMetaHost_UUID, CLR_METAHOST_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, GetRuntime(DefaultCLRVersion, CLSID_CLRRunTime_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, IsLoadable(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, GetInterface(CLSID_CorRuntimeHost_UUID, ICorRuntimeHost_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, Start());

		STRICT_EXPECTED_CALL(mocks, GetDefaultDomain(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, QueryInterface(AppDomainUUID, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, Load_2(bstrClientModuleAssemblyName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);


		STRICT_EXPECTED_CALL(mocks, GetType_2(bstrClientModuleClassName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);


		STRICT_EXPECTED_CALL(mocks, Load_2(bstrAzureIoTGatewayAssemblyName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreateVector(VT_VARIANT, 0, 2));

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayDestroy(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, CreateInstance_3(bstrAzureIoTGatewayMessageBusClassName, true, static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public), NULL, IGNORED_PTR_ARG, NULL, NULL, IGNORED_PTR_ARG))
			.IgnoreArgument(5)
			.IgnoreArgument(8);

		STRICT_EXPECTED_CALL(mocks, CreateInstance(bstrClientModuleClassName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreateVector(VT_VARIANT, 0, 2));

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayDestroy(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, InvokeMember_3(bstrCreateClientMethodName, static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public | BindingFlags_InvokeMethod), NULL, emptyVariant, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreArgument(4)
			.IgnoreArgument(5)
			.IgnoreArgument(6)
			.SetFailReturn((HRESULT)E_POINTER);

		///act
		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NULL(result);

		///cleanup
	}


	/* Tests_SRS_DOTNET_04_007: [ DotNET_Create shall return a non-NULL MODULE_HANDLE when successful. ] */
	/* Tests_SRS_DOTNET_04_008: [ DotNET_Create shall allocate memory for an instance of the DOTNET_HOST_HANDLE_DATA structure and use that as the backing structure for the module handle. ] */
	/* Tests_SRS_DOTNET_04_012: [ DotNET_Create shall get the 3 CLR Host Interfaces (CLRMetaHost, CLRRuntimeInfo and CorRuntimeHost) and save it on DOTNET_HOST_HANDLE_DATA. ] */
	/* Tests_SRS_DOTNET_04_009: [ DotNET_Create shall create an instance of .NET client Module and save it on DOTNET_HOST_HANDLE_DATA. ] */
	/* Tests_SRS_DOTNET_04_010: [ DotNET_Create shall save Client module Type and Azure IoT Gateway Assembly on DOTNET_HOST_HANDLE_DATA. ] */
	/* Tests_SRS_DOTNET_04_013: [ A .NET Object conforming to the MessageBus interface defined shall be created: ] */
	/* Tests_SRS_DOTNET_04_014: [ DotNET_Create shall call Create C# method, implemented from IGatewayModule, passing the MessageBus object created and configuration->dotnet_module_args. ] */
	TEST_FUNCTION(DotNET_Create_succeed)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";
		bstr_t bstrClientModuleAssemblyName(dotNetConfig.dotnet_module_path);
		bstr_t bstrClientModuleClassName(dotNetConfig.dotnet_module_entry_class);
		bstr_t bstrAzureIoTGatewayAssemblyName(L"Microsoft.Azure.IoT.Gateway");
		bstr_t bstrAzureIoTGatewayMessageBusClassName(L"Microsoft.Azure.IoT.Gateway.MessageBus");
		bstr_t bstrAzureIoTGatewayMessageClassName(L"Microsoft.Azure.IoT.Gateway.Message");
		bstr_t bstrCreateClientMethodName(L"Create");
		variant_t emptyVariant(0);

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, CLRCreateInstance(CLSID_CLRMetaHost_UUID, CLR_METAHOST_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, GetRuntime(DefaultCLRVersion, CLSID_CLRRunTime_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, IsLoadable(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, GetInterface(CLSID_CorRuntimeHost_UUID, ICorRuntimeHost_UUID, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, Start());

		STRICT_EXPECTED_CALL(mocks, GetDefaultDomain(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, QueryInterface(AppDomainUUID, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, Load_2(bstrClientModuleAssemblyName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);


		STRICT_EXPECTED_CALL(mocks, GetType_2(bstrClientModuleClassName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);


		STRICT_EXPECTED_CALL(mocks, Load_2(bstrAzureIoTGatewayAssemblyName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreateVector(VT_VARIANT, 0, 2));

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayDestroy(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, CreateInstance_3(bstrAzureIoTGatewayMessageBusClassName, true, static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public), NULL, IGNORED_PTR_ARG, NULL, NULL, IGNORED_PTR_ARG))
			.IgnoreArgument(5)
			.IgnoreArgument(8);

		STRICT_EXPECTED_CALL(mocks, CreateInstance(bstrClientModuleClassName, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreateVector(VT_VARIANT, 0, 2));

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayDestroy(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, InvokeMember_3(bstrCreateClientMethodName, static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public | BindingFlags_InvokeMethod), NULL, emptyVariant, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreArgument(4)
			.IgnoreArgument(5)
			.IgnoreArgument(6);

		///act
		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		///assert
		mocks.AssertActualAndExpectedCalls();
		ASSERT_IS_NOT_NULL(result);

		///cleanup
		theAPIS->Module_Destroy((MODULE_HANDLE)result);
	}

	/* Tests_SRS_DOTNET_04_015: [ DotNET_Receive shall do nothing if module is NULL. ] */
	TEST_FUNCTION(DotNET_Receive_does_nothing_when_modulehandle_is_Null)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		mocks.ResetAllCalls();

		///act
		theAPIS->Module_Receive(NULL,(MESSAGE_HANDLE)0x42);

		///assert
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_016: [ DotNET_Receive shall do nothing if message is NULL. ] */
	TEST_FUNCTION(DotNET_Receive_does_nothing_when_message_is_Null)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		mocks.ResetAllCalls();

		///act
		theAPIS->Module_Receive((MODULE_HANDLE)0x42, NULL);

		///assert
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_017: [ DotNET_Receive shall construct an instance of the Message interface as defined below: ] */
	TEST_FUNCTION(DotNET_Receive__does_nothing_when_Message_ToByteArray_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		bstr_t bstrAzureIoTGatewayMessageClassName(L"Microsoft.Azure.IoT.Gateway.Message");
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";

		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, Message_ToByteArray((MESSAGE_HANDLE)0x42, IGNORED_PTR_ARG))
			.IgnoreArgument(2)
			.SetFailReturn((const unsigned char*)NULL);

		///act
		theAPIS->Module_Receive((MODULE_HANDLE)result, (MESSAGE_HANDLE)0x42);

		///assert
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_017: [ DotNET_Receive shall construct an instance of the Message interface as defined below: ] */
	TEST_FUNCTION(DotNET_Receive__does_nothing_when_CreateSafeArray_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		bstr_t bstrAzureIoTGatewayMessageClassName(L"Microsoft.Azure.IoT.Gateway.Message");
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";

		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, Message_ToByteArray((MESSAGE_HANDLE)0x42, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreate(VT_UI1, 1, IGNORED_PTR_ARG))
			.IgnoreArgument(3)
			.SetFailReturn((SAFEARRAY*)NULL);


		///act
		theAPIS->Module_Receive((MODULE_HANDLE)result, (MESSAGE_HANDLE)0x42);

		///assert
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}

	
	/* Tests_SRS_DOTNET_04_017: [ DotNET_Receive shall construct an instance of the Message interface as defined below: ] */
	TEST_FUNCTION(DotNET_Receive__does_nothing_when_SafeArrayAccessData_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		bstr_t bstrAzureIoTGatewayMessageClassName(L"Microsoft.Azure.IoT.Gateway.Message");
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";

		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, Message_ToByteArray((MESSAGE_HANDLE)0x42, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreate(VT_UI1, 1, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayAccessData(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments()
			.SetFailReturn((HRESULT)E_POINTER);


		///act
		theAPIS->Module_Receive((MODULE_HANDLE)result, (MESSAGE_HANDLE)0x42);

		///assert
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}

	
	/* Tests_SRS_DOTNET_04_017: [ DotNET_Receive shall construct an instance of the Message interface as defined below: ] */
	TEST_FUNCTION(DotNET_Receive__does_nothing_when_SafeArrayUnaccessData_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		bstr_t bstrAzureIoTGatewayMessageClassName(L"Microsoft.Azure.IoT.Gateway.Message");
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";

		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, Message_ToByteArray((MESSAGE_HANDLE)0x42, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreate(VT_UI1, 1, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayAccessData(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayUnaccessData(IGNORED_PTR_ARG))
			.IgnoreAllArguments()
			.SetFailReturn((HRESULT)E_POINTER);



		///act
		theAPIS->Module_Receive((MODULE_HANDLE)result, (MESSAGE_HANDLE)0x42);

		///assert
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_017: [ DotNET_Receive shall construct an instance of the Message interface as defined below: ] */
	TEST_FUNCTION(DotNET_Receive__does_nothing_when_SafeArrayCreateVector_for_Message_Constructor_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		bstr_t bstrAzureIoTGatewayMessageClassName(L"Microsoft.Azure.IoT.Gateway.Message");
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";

		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, Message_ToByteArray((MESSAGE_HANDLE)0x42, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreate(VT_UI1, 1, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayAccessData(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayUnaccessData(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreateVector(VT_VARIANT, 0, 1))
			.SetFailReturn((SAFEARRAY*)NULL);



		///act
		theAPIS->Module_Receive((MODULE_HANDLE)result, (MESSAGE_HANDLE)0x42);

		///assert
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_017: [ DotNET_Receive shall construct an instance of the Message interface as defined below: ] */
	TEST_FUNCTION(DotNET_Receive__does_nothing_when_SafeArrayPutElement_for_Message_Constructor_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		bstr_t bstrAzureIoTGatewayMessageClassName(L"Microsoft.Azure.IoT.Gateway.Message");
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";

		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, Message_ToByteArray((MESSAGE_HANDLE)0x42, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreate(VT_UI1, 1, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayAccessData(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayUnaccessData(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreateVector(VT_VARIANT, 0, 1));

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayDestroy(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments()
			.SetFailReturn((HRESULT)E_POINTER);



		///act
		theAPIS->Module_Receive((MODULE_HANDLE)result, (MESSAGE_HANDLE)0x42);

		///assert
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_017: [ DotNET_Receive shall construct an instance of the Message interface as defined below: ] */
	TEST_FUNCTION(DotNET_Receive__does_nothing_when_Create_Instance_3_for_Message_Constructor_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		bstr_t bstrAzureIoTGatewayMessageClassName(L"Microsoft.Azure.IoT.Gateway.Message");
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";

		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, Message_ToByteArray((MESSAGE_HANDLE)0x42, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreate(VT_UI1, 1, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayAccessData(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayUnaccessData(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreateVector(VT_VARIANT, 0, 1));

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayDestroy(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();
	
		STRICT_EXPECTED_CALL(mocks, CreateInstance_3(bstrAzureIoTGatewayMessageClassName, true, static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public), NULL, IGNORED_PTR_ARG,NULL, NULL, IGNORED_PTR_ARG))
			.IgnoreArgument(5)
			.IgnoreArgument(8)
			.SetFailReturn((HRESULT)E_POINTER);


		///act
		theAPIS->Module_Receive((MODULE_HANDLE)result, (MESSAGE_HANDLE)0x42);

		///assert
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_018: [ DotNET_Create shall call Receive C# method passing the Message object created with the content of message serialized into Message object. ] */
	TEST_FUNCTION(DotNET_Receive__does_nothing_when_SafeArrayCreateVector_For_Receive_Argument_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		bstr_t bstrAzureIoTGatewayMessageClassName(L"Microsoft.Azure.IoT.Gateway.Message");
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";

		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, Message_ToByteArray((MESSAGE_HANDLE)0x42, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreate(VT_UI1, 1, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayAccessData(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayUnaccessData(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreateVector(VT_VARIANT, 0, 1));

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayDestroy(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, CreateInstance_3(bstrAzureIoTGatewayMessageClassName, true, static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public), NULL, IGNORED_PTR_ARG, NULL, NULL, IGNORED_PTR_ARG))
			.IgnoreArgument(5)
			.IgnoreArgument(8);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreateVector(VT_VARIANT, 0, 1))
			.SetFailReturn((SAFEARRAY*)NULL);


		///act
		theAPIS->Module_Receive((MODULE_HANDLE)result, (MESSAGE_HANDLE)0x42);

		///assert
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_018: [ DotNET_Create shall call Receive C# method passing the Message object created with the content of message serialized into Message object. ] */
	TEST_FUNCTION(DotNET_Receive__does_nothing_when_SafeArrayPutElement_For_Receive_Argument_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		bstr_t bstrAzureIoTGatewayMessageClassName(L"Microsoft.Azure.IoT.Gateway.Message");
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";

		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, Message_ToByteArray((MESSAGE_HANDLE)0x42, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreate(VT_UI1, 1, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayAccessData(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayUnaccessData(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreateVector(VT_VARIANT, 0, 1));

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayDestroy(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, CreateInstance_3(bstrAzureIoTGatewayMessageClassName, true, static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public), NULL, IGNORED_PTR_ARG, NULL, NULL, IGNORED_PTR_ARG))
			.IgnoreArgument(5)
			.IgnoreArgument(8);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreateVector(VT_VARIANT, 0, 1));

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayDestroy(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments()
			.SetFailReturn((HRESULT)E_POINTER);


		///act
		theAPIS->Module_Receive((MODULE_HANDLE)result, (MESSAGE_HANDLE)0x42);

		///assert
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_018: [ DotNET_Create shall call Receive C# method passing the Message object created with the content of message serialized into Message object. ] */
	TEST_FUNCTION(DotNET_Receive__does_nothing_when_InvokeMember_3_For_Receive_Argument_fails)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		bstr_t bstrAzureIoTGatewayMessageClassName(L"Microsoft.Azure.IoT.Gateway.Message");
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";
		bstr_t bstrReceiveClientMethodName(L"Receive");
		variant_t emptyVariant(0);

		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, Message_ToByteArray((MESSAGE_HANDLE)0x42, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreate(VT_UI1, 1, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayAccessData(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayUnaccessData(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreateVector(VT_VARIANT, 0, 1));

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayDestroy(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, CreateInstance_3(bstrAzureIoTGatewayMessageClassName, true, static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public), NULL, IGNORED_PTR_ARG, NULL, NULL, IGNORED_PTR_ARG))
			.IgnoreArgument(5)
			.IgnoreArgument(8);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreateVector(VT_VARIANT, 0, 1));

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayDestroy(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, InvokeMember_3(bstrReceiveClientMethodName, static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public | BindingFlags_InvokeMethod), NULL, emptyVariant, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreArgument(4)
			.IgnoreArgument(5)
			.IgnoreArgument(6)
			.SetFailReturn((HRESULT)E_POINTER);


		///act
		theAPIS->Module_Receive((MODULE_HANDLE)result, (MESSAGE_HANDLE)0x42);

		///assert
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_018: [ DotNET_Create shall call Receive C# method passing the Message object created with the content of message serialized into Message object. ] */
	TEST_FUNCTION(DotNET_receive_succeed)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		bstr_t bstrAzureIoTGatewayMessageClassName(L"Microsoft.Azure.IoT.Gateway.Message");
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";
		bstr_t bstrReceiveClientMethodName(L"Receive");
		variant_t emptyVariant(0);

		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		mocks.ResetAllCalls();

		STRICT_EXPECTED_CALL(mocks, Message_ToByteArray((MESSAGE_HANDLE)0x42, IGNORED_PTR_ARG))
			.IgnoreArgument(2);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreate(VT_UI1, 1, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayAccessData(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayUnaccessData(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreateVector(VT_VARIANT, 0, 1));

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayDestroy(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, CreateInstance_3(bstrAzureIoTGatewayMessageClassName, true, static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public), NULL, IGNORED_PTR_ARG, NULL, NULL, IGNORED_PTR_ARG))
			.IgnoreArgument(5)
			.IgnoreArgument(8);

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayCreateVector(VT_VARIANT, 0, 1));

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayDestroy(IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, myTest_SafeArrayPutElement(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreAllArguments();

		STRICT_EXPECTED_CALL(mocks, InvokeMember_3(bstrReceiveClientMethodName, static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public | BindingFlags_InvokeMethod), NULL, emptyVariant, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreArgument(4)
			.IgnoreArgument(5)
			.IgnoreArgument(6);


		///act
		theAPIS->Module_Receive((MODULE_HANDLE)result, (MESSAGE_HANDLE)0x42);

		///assert
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_019: [ DotNET_Destroy shall do nothing if module is NULL. ] */
	TEST_FUNCTION(DotNET_Destroy_does_nothing_when_module_is_Null)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();

		mocks.ResetAllCalls();

		///act
		theAPIS->Module_Destroy((MODULE_HANDLE)NULL);

		///assert
		mocks.AssertActualAndExpectedCalls();
		
		///cleanup
	}

	/* Tests_SRS_DOTNET_04_022: [ DotNET_Destroy shall call the Destroy C# method. ] */
	/* Tests_SRS_DOTNET_04_020: [ DotNET_Destroy shall free all resources associated with the given module.. ] */
	TEST_FUNCTION(DotNET_Destroy_Invokes_DotNet_Destroy)
	{
		///arrage
		CDOTNETMocks mocks;
		const MODULE_APIS* theAPIS = Module_GetAPIS();
		DOTNET_HOST_CONFIG dotNetConfig;
		dotNetConfig.dotnet_module_path = "/path/to/csharp_module.dll";
		dotNetConfig.dotnet_module_entry_class = "mycsharpmodule.classname";
		dotNetConfig.dotnet_module_args = "module configuration";
		bstr_t bstrDestroyClientMethodName(L"Destroy");
		variant_t emptyVariant(0);

		auto  result = theAPIS->Module_Create((MESSAGE_BUS_HANDLE)0x42, &dotNetConfig);

		mocks.ResetAllCalls();
		STRICT_EXPECTED_CALL(mocks, InvokeMember_3(bstrDestroyClientMethodName, static_cast<BindingFlags>(BindingFlags_Instance | BindingFlags_Public | BindingFlags_InvokeMethod), NULL, emptyVariant, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.IgnoreArgument(4)
			.IgnoreArgument(5)
			.IgnoreArgument(6);

		///act
		theAPIS->Module_Destroy(result);

		///assert
		mocks.AssertActualAndExpectedCalls();

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_022: [ Module_DotNetHost_PublishMessage shall return false if bus is NULL. ] */
	TEST_FUNCTION(DotNET_Module_DotNetHost_PublishMessage_if_bus_is_NULL_return_false)
	{
		///arrage
		CDOTNETMocks mocks;

		///act
		auto result = Module_DotNetHost_PublishMessage(NULL,(MODULE_HANDLE)0x42, (const unsigned char *)"AnyContent", 11);

		///assert
		ASSERT_IS_FALSE(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_029: [ Module_DotNetHost_PublishMessage shall return false if sourceModule is NULL. ] */
	TEST_FUNCTION(DotNET_Module_DotNetHost_PublishMessage_if_sourceModule_is_NULL_return_false)
	{
		///arrage
		CDOTNETMocks mocks;

		///act
		auto result = Module_DotNetHost_PublishMessage((MESSAGE_BUS_HANDLE)0x42, (MODULE_HANDLE)NULL, (const unsigned char *)"AnyContent", 11);

		///assert
		ASSERT_IS_FALSE(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_023: [ Module_DotNetHost_PublishMessage shall return false if source is NULL, or size if lower than 0. ] */
	TEST_FUNCTION(DotNET_Module_DotNetHost_PublishMessage_if_source_is_NULL_return_false)
	{
		///arrage
		CDOTNETMocks mocks;

		///act
		auto result = Module_DotNetHost_PublishMessage((MESSAGE_BUS_HANDLE)0x42, (MODULE_HANDLE)0x42, NULL, 11);

		///assert
		ASSERT_IS_FALSE(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_023: [ Module_DotNetHost_PublishMessage shall return false if source is NULL, or size if lower than 0. ] */
	TEST_FUNCTION(DotNET_Module_DotNetHost_PublishMessage_if_size_is_lower_than_zero_return_false)
	{
		///arrage
		CDOTNETMocks mocks;

		///act
		auto result = Module_DotNetHost_PublishMessage((MESSAGE_BUS_HANDLE)0x42, (MODULE_HANDLE)0x42, (const unsigned char *)"AnyContent", -1);

		///assert
		ASSERT_IS_FALSE(result);

		///cleanup
	}

	
	/* Tests_SRS_DOTNET_04_025: [ If Message_CreateFromByteArray fails, Module_DotNetHost_PublishMessage shall fail. ] */
	TEST_FUNCTION(DotNET_Module_DotNetHost_PublishMessage_fails_when_Message_CreateFromByteArray_fail)
	{
		///arrage
		CDOTNETMocks mocks;

		STRICT_EXPECTED_CALL(mocks, Message_CreateFromByteArray((const unsigned char*)"AnyContent", 11))
			.IgnoreArgument(1)
			.SetFailReturn((MESSAGE_HANDLE)NULL);

		///act
		auto result = Module_DotNetHost_PublishMessage((MESSAGE_BUS_HANDLE)0x42, (MODULE_HANDLE)0x42, (const unsigned char *)"AnyContent", 11);

		///assert
		ASSERT_IS_FALSE(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_027: [ If MessageBus_Publish fail Module_DotNetHost_PublishMessage shall fail. ] */
	TEST_FUNCTION(DotNET_Module_DotNetHost_PublishMessage_fails_when_MessageBus_Publish_fail)
	{
		///arrage
		CDOTNETMocks mocks;

		STRICT_EXPECTED_CALL(mocks, Message_CreateFromByteArray((const unsigned char*)"AnyContent", 11))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, MessageBus_Publish((MESSAGE_BUS_HANDLE)0x42, (MODULE_HANDLE)0x42, IGNORED_PTR_ARG))
			.IgnoreArgument(3)
			.SetFailReturn((MESSAGE_BUS_RESULT)MESSAGE_BUS_ERROR);

		STRICT_EXPECTED_CALL(mocks, Message_Destroy(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

		///act
		auto result = Module_DotNetHost_PublishMessage((MESSAGE_BUS_HANDLE)0x42, (MODULE_HANDLE)0x42, (const unsigned char *)"AnyContent", 11);

		///assert
		ASSERT_IS_FALSE(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_024: [ Module_DotNetHost_PublishMessage shall create a message from source and size by invoking Message_CreateFromByteArray. ] */
	/* Tests_SRS_DOTNET_04_026: [ Module_DotNetHost_PublishMessage shall call MessageBus_Publish passing bus, sourceModule, byte array and msgHandle. ] */
	/* Tests_SRS_DOTNET_04_028: [If MessageBus_Publish succeed Module_DotNetHost_PublishMessage shall succeed.] */
	TEST_FUNCTION(DotNET_Module_DotNetHost_PublishMessage_succeed)
	{
		///arrage
		CDOTNETMocks mocks;

		STRICT_EXPECTED_CALL(mocks, Message_CreateFromByteArray((const unsigned char*)"AnyContent", 11))
			.IgnoreArgument(1);

		STRICT_EXPECTED_CALL(mocks, MessageBus_Publish((MESSAGE_BUS_HANDLE)0x42, (MODULE_HANDLE)0x42, IGNORED_PTR_ARG))
			.IgnoreArgument(3);

		STRICT_EXPECTED_CALL(mocks, Message_Destroy(IGNORED_PTR_ARG))
			.IgnoreArgument(1);

		///act
		auto result = Module_DotNetHost_PublishMessage((MESSAGE_BUS_HANDLE)0x42, (MODULE_HANDLE)0x42, (const unsigned char *)"AnyContent", 11);

		///assert
		ASSERT_IS_TRUE(result);

		///cleanup
	}

	/* Tests_SRS_DOTNET_04_021: [ Module_GetAPIS shall return a non-NULL pointer to a structure of type MODULE_APIS that has all fields initialized to non-NULL values. ] */
	TEST_FUNCTION(DotNET_Module_GetAPIS_returns_non_NULL)
	{
		///arrage
		CDOTNETMocks mocks;

		mocks.ResetAllCalls();
		///act
		const MODULE_APIS* apis = Module_GetAPIS();

		///assert
		ASSERT_IS_NOT_NULL(apis);
		ASSERT_IS_NOT_NULL(apis->Module_Destroy);
		ASSERT_IS_NOT_NULL(apis->Module_Create);
		ASSERT_IS_NOT_NULL(apis->Module_Receive);

		///cleanup
	}

END_TEST_SUITE(dotnet_unittests)