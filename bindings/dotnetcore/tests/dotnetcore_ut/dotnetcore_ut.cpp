// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#include <cstdbool>
#include <cstddef>
#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"

#define GATEWAY_EXPORT_H
#define GATEWAY_EXPORT

#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/map.h"
#include "azure_c_shared_utility/strings.h"
#include "dynamic_library.h"


#include "module_access.h"

#include "dotnetcore.h"
#include "dotnetcore_common.h"
#include "dotnetcore_utils.h"

#include <parson.h>


static MICROMOCK_MUTEX_HANDLE g_testByTest;
static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

typedef unsigned int(DOTNET_CORE_CALLING_CONVENTION *PGatewayCreateDelegate)(intptr_t broker, intptr_t module, const char* assemblyName, const char* entryType, const char* gatewayConfiguration);

typedef void(DOTNET_CORE_CALLING_CONVENTION *PGatewayReceiveDelegate)(unsigned char* buffer, int32_t bufferSize, unsigned int moduleIdManaged);

typedef void(DOTNET_CORE_CALLING_CONVENTION *PGatewayDestroyDelegate)(unsigned int moduleIdManaged);

typedef void(DOTNET_CORE_CALLING_CONVENTION *PGatewayStartDelegate)(unsigned int moduleIdManaged);


extern PGatewayCreateDelegate GatewayCreateDelegate;

extern PGatewayReceiveDelegate GatewayReceiveDelegate;

extern PGatewayDestroyDelegate GatewayDestroyDelegate;

extern PGatewayStartDelegate GatewayStartDelegate;

static size_t gMessageSize;
static const unsigned char * gMessageSource;

static size_t currentnew_call;
static size_t whenShallnew_fail;
static size_t currentnewarray_call;
static size_t whenShallnewarray_fail;

#define GBALLOC_H

extern "C" int   gballoc_init(void);
extern "C" void  gballoc_deinit(void);
extern "C" void* gballoc_malloc(size_t size);
extern "C" void* gballoc_calloc(size_t nmemb, size_t size);
extern "C" void* gballoc_realloc(void* ptr, size_t size);
extern "C" void  gballoc_free(void* ptr);

static bool failCreateDelegate = false;
static bool failReceiveDelegate = false;
static bool failDestroyDelegate = false;


static bool calledCreateMethod = false;
static bool calledReceiveMethod = false;
static bool calledDestroyMethod = false;
static bool calledStartMethod = false;

int DOTNET_CORE_CALLING_CONVENTION fakeGatewayCreateMethod(intptr_t broker, intptr_t module, const char* assemblyName, const char* entryType, const char* gatewayConfiguration)
{
    (void)broker;
    (void)module;
    (void)assemblyName;
    (void)entryType;
    (void)gatewayConfiguration;

    calledCreateMethod = true;

    return __LINE__;
};

void DOTNET_CORE_CALLING_CONVENTION fakeGatewayReceiveMethod(unsigned char* buffer, int32_t bufferSize, unsigned int moduleIdManaged)
{
    (void)buffer;
    (void)bufferSize;
    (void)moduleIdManaged;

    calledReceiveMethod = true;
};

void DOTNET_CORE_CALLING_CONVENTION fakeGatewayDestroyMethod(unsigned int moduleIdManaged)
{
    (void)moduleIdManaged;

    calledDestroyMethod = true;
};

void DOTNET_CORE_CALLING_CONVENTION fakeGatewayStartMethod(unsigned int moduleIdManaged)
{
    (void)moduleIdManaged;

    calledStartMethod = true;
}


int DOTNET_CORE_CALLING_CONVENTION fake_create_delegate(void* hostHandle,
    unsigned int domainId,
    const char* entryPointAssemblyName,
    const char* entryPointTypeName,
    const char* entryPointMethodName,
    void** delegate)
{
    (void)hostHandle;
    (void)domainId;
    (void)entryPointAssemblyName;
    (void)entryPointTypeName;

    int returnStatus;

    //assign delegates.
    if (strcmp(entryPointMethodName, "Create") == 0)
    {
        *delegate = (void*)fakeGatewayCreateMethod;
    }
    else if (strcmp(entryPointMethodName, "Receive") == 0)
    {
        *delegate = (void*)fakeGatewayReceiveMethod;
    }
    else if (strcmp(entryPointMethodName, "Destroy") == 0)
    {
        *delegate = (void*)fakeGatewayDestroyMethod;
    }
    else if (strcmp(entryPointMethodName, "Start") == 0)
    {
        *delegate = (void*)fakeGatewayStartMethod;
    }


    //Define return status.
    if (failCreateDelegate == true && strcmp(entryPointMethodName, "Create") == 0)
    {
        returnStatus = -1;
    }
    else if(failReceiveDelegate == true && strcmp(entryPointMethodName, "Receive") == 0)
    {
        returnStatus = -1;
    }
    else if (failDestroyDelegate == true && strcmp(entryPointMethodName, "Destroy") == 0)
    {
        returnStatus = -1;
    }
    else
    {
        returnStatus = 0;
    }

    return returnStatus;
};



int DOTNET_CORE_CALLING_CONVENTION fake_shutdownclr(
    void * hostHandle,
    unsigned int domainId)
{
    (void)hostHandle;
    (void)domainId;
    return 0;
};


namespace BASEIMPLEMENTATION
{
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

    typedef union json_value_value {
        char        *string;
        double       number;
        JSON_Object *object;
        JSON_Array  *array;
        int          boolean;
        int          null;
    } JSON_Value_Value;

    struct json_value_t {
        JSON_Value_Type     type;
        JSON_Value_Value    value;
    };

    typedef struct json_value_t  JSON_Value;
};






TYPED_MOCK_CLASS(CDOTNETCOREMocks, CGlobalMock)
{
public:

	CDOTNETCOREMocks()
	{
	}

	virtual ~CDOTNETCOREMocks()
	{
	}
    //Utilities Mock
    MOCK_STATIC_METHOD_2(,void, AddFilesFromDirectoryToTpaList, const std::string&,  directory, std::string&,  tpaList)
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_4(,bool, initializeDotNetCoreCLR,coreclr_initialize_ptr, coreclrInitialize_ptr, const char*, trustedPlatformAssemblyiesLocation, void**, hostHandle, unsigned int*, domainId)
    MOCK_METHOD_END(bool, true)


    //Dynamic Loca Library
    MOCK_STATIC_METHOD_1(, DYNAMIC_LIBRARY_HANDLE, DynamicLibrary_LoadLibrary, const char*, dynamicLibraryFileName)
    MOCK_METHOD_END(DYNAMIC_LIBRARY_HANDLE, (DYNAMIC_LIBRARY_HANDLE)0x42)

    MOCK_STATIC_METHOD_1(, void, DynamicLibrary_UnloadLibrary, DYNAMIC_LIBRARY_HANDLE, libraryHandle)
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_2(, void*, DynamicLibrary_FindSymbol, DYNAMIC_LIBRARY_HANDLE, libraryHandle, const char*, symbolName)
        void* returnFunctionPointer;
        if (strcmp(symbolName, "coreclr_create_delegate") == 0)
        {
            returnFunctionPointer = (void*)fake_create_delegate;
        }else if (strcmp(symbolName, "coreclr_shutdown") == 0)
        {
            returnFunctionPointer = (void*)fake_shutdownclr;
        }
        else
        {
            returnFunctionPointer = (void*)0x42;
        }
    MOCK_METHOD_END(void*, returnFunctionPointer)

    //String
    MOCK_STATIC_METHOD_1(, STRING_HANDLE, STRING_construct, const char*, psz)
    MOCK_METHOD_END(STRING_HANDLE, STRING_HANDLE(0X42))

    MOCK_STATIC_METHOD_1(, void, STRING_delete, STRING_HANDLE, handle)
    MOCK_VOID_METHOD_END()

    //Message Mocks
    MOCK_STATIC_METHOD_3(, int32_t, Message_ToByteArray, MESSAGE_HANDLE, messageHandle, unsigned char*, buf, int32_t , size)
    MOCK_METHOD_END( int32_t, (int32_t)11);

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

    /*Parson Mocks*/
    MOCK_STATIC_METHOD_1(, JSON_Value*, json_parse_string, const char *, filename)
    JSON_Value* value = NULL;
    if (filename != NULL)
    {
        value = (JSON_Value*)malloc(sizeof(BASEIMPLEMENTATION::JSON_Value));
    }
    MOCK_METHOD_END(JSON_Value*, value);

    MOCK_STATIC_METHOD_1(, JSON_Value*, json_parse_file, const char *, filename)
        JSON_Value* value = NULL;
    if (filename != NULL)
    {
        value = (JSON_Value*)malloc(sizeof(BASEIMPLEMENTATION::JSON_Value));
    }
    MOCK_METHOD_END(JSON_Value*, value);

    MOCK_STATIC_METHOD_1(, JSON_Object*, json_value_get_object, const JSON_Value*, value)
        JSON_Object* object = NULL;
    if (value != NULL)
    {
        object = (JSON_Object*)0x42;
    }
    MOCK_METHOD_END(JSON_Object*, object);

    MOCK_STATIC_METHOD_2(, JSON_Array*, json_object_get_array, const JSON_Object*, object, const char*, name)
        JSON_Array* arr = NULL;
    if (object != NULL && name != NULL)
    {
        arr = (JSON_Array*)0x42;
    }
    MOCK_METHOD_END(JSON_Array*, arr);

    MOCK_STATIC_METHOD_1(, size_t, json_array_get_count, const JSON_Array*, arr)
        size_t size = 4;
    MOCK_METHOD_END(size_t, size);

    MOCK_STATIC_METHOD_2(, double, json_object_get_number, const JSON_Object *, object, const char *, name)
        double result2 = 0;
    MOCK_METHOD_END(double, result2);

    MOCK_STATIC_METHOD_2(, JSON_Object*, json_array_get_object, const JSON_Array*, arr, size_t, index)
        JSON_Object* object = NULL;
    if (arr != NULL)
    {
        object = (JSON_Object*)0x42;
    }
    MOCK_METHOD_END(JSON_Object*, object);

    MOCK_STATIC_METHOD_2(, const char*, json_object_get_string, const JSON_Object*, object, const char*, name)
        const char* result2;
    if (strcmp(name, "assembly.name") == 0)
    {
        result2 = "/path/to/csharp_module.dll";
    }
    else if (strcmp(name, "entry.type") == 0)
    {
        result2 = "mycsharpmodule.classname";
    }
    else if (strcmp(name, "moduleArgs") == 0)
    {
        result2 = "module configuration";
    }
    else
    {
        result2 = NULL;
    }
    MOCK_METHOD_END(const char*, result2);

    MOCK_STATIC_METHOD_2(, JSON_Value*, json_object_get_value, const JSON_Object*, object, const char*, name)
        JSON_Value* value = NULL;
    if (object != NULL && name != NULL)
    {
        value = (JSON_Value*)0x42;
    }
    MOCK_METHOD_END(JSON_Value*, value);

    MOCK_STATIC_METHOD_1(, void, json_value_free, JSON_Value*, value)
        free(value);
    MOCK_VOID_METHOD_END();

    //Broker Mocks
    MOCK_STATIC_METHOD_3(, BROKER_RESULT, Broker_Publish, BROKER_HANDLE, broker, MODULE_HANDLE, source, MESSAGE_HANDLE, message)
    MOCK_METHOD_END(BROKER_RESULT, BROKER_OK);
};

extern "C"
{
    //Utilities
    DECLARE_GLOBAL_MOCK_METHOD_2(CDOTNETCOREMocks, , void, AddFilesFromDirectoryToTpaList, const std::string&, directory, std::string&, tpaList);
    DECLARE_GLOBAL_MOCK_METHOD_4(CDOTNETCOREMocks, , bool, initializeDotNetCoreCLR, coreclr_initialize_ptr, coreclrInitialize_ptr, const char*, trustedPlatformAssemblyiesLocation, void**, hostHandle, unsigned int*, domainId);

    //Dynamic Load Library
    DECLARE_GLOBAL_MOCK_METHOD_1(CDOTNETCOREMocks, , DYNAMIC_LIBRARY_HANDLE, DynamicLibrary_LoadLibrary, const char*, dynamicLibraryFileName);
    DECLARE_GLOBAL_MOCK_METHOD_1(CDOTNETCOREMocks, , void, DynamicLibrary_UnloadLibrary, DYNAMIC_LIBRARY_HANDLE, libraryHandle);
    DECLARE_GLOBAL_MOCK_METHOD_2(CDOTNETCOREMocks, , void*, DynamicLibrary_FindSymbol, DYNAMIC_LIBRARY_HANDLE, libraryHandle, const char*, symbolName);

    // strings
    DECLARE_GLOBAL_MOCK_METHOD_1(CDOTNETCOREMocks, , STRING_HANDLE, STRING_construct, const char*, psz);
    DECLARE_GLOBAL_MOCK_METHOD_1(CDOTNETCOREMocks, , void, STRING_delete, STRING_HANDLE, handle);

    // memory
    DECLARE_GLOBAL_MOCK_METHOD_1(CDOTNETCOREMocks, , void*, gballoc_malloc, size_t, size);
    DECLARE_GLOBAL_MOCK_METHOD_1(CDOTNETCOREMocks, , void, gballoc_free, void*, ptr);

        
    //Message Mocks
    DECLARE_GLOBAL_MOCK_METHOD_3(CDOTNETCOREMocks, , int32_t, Message_ToByteArray, MESSAGE_HANDLE, messageHandle, unsigned char*, buf, int32_t, size);

    DECLARE_GLOBAL_MOCK_METHOD_2(CDOTNETCOREMocks, , MESSAGE_HANDLE, Message_CreateFromByteArray, const unsigned char*, source, int32_t, size);

    DECLARE_GLOBAL_MOCK_METHOD_1(CDOTNETCOREMocks, , void, Message_Destroy, MESSAGE_HANDLE, message);

    DECLARE_GLOBAL_MOCK_METHOD_1(CDOTNETCOREMocks, , JSON_Value*, json_parse_string, const char *, filename);
    DECLARE_GLOBAL_MOCK_METHOD_1(CDOTNETCOREMocks, , JSON_Object*, json_value_get_object, const JSON_Value*, value);
    DECLARE_GLOBAL_MOCK_METHOD_2(CDOTNETCOREMocks, , double, json_object_get_number, const JSON_Object*, value, const char*, name);
    DECLARE_GLOBAL_MOCK_METHOD_2(CDOTNETCOREMocks, , const char*, json_object_get_string, const JSON_Object*, object, const char*, name);
    DECLARE_GLOBAL_MOCK_METHOD_2(CDOTNETCOREMocks, , JSON_Array*, json_object_get_array, const JSON_Object*, object, const char*, name);
    DECLARE_GLOBAL_MOCK_METHOD_1(CDOTNETCOREMocks, , void, json_value_free, JSON_Value*, value);
    DECLARE_GLOBAL_MOCK_METHOD_1(CDOTNETCOREMocks, , size_t, json_array_get_count, const JSON_Array*, arr);
    DECLARE_GLOBAL_MOCK_METHOD_2(CDOTNETCOREMocks, , JSON_Object*, json_array_get_object, const JSON_Array*, arr, size_t, index);

    //Broker Mocks
    DECLARE_GLOBAL_MOCK_METHOD_3(CDOTNETCOREMocks, , BROKER_RESULT, Broker_Publish, BROKER_HANDLE, broker, MODULE_HANDLE, source, MESSAGE_HANDLE, message);

}


BEGIN_TEST_SUITE(dotnetcore_ut)
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
		currentnewarray_call = 0;
		whenShallnewarray_fail = 0;

        failCreateDelegate = false;
        failReceiveDelegate = false;
        failDestroyDelegate = false;

        calledCreateMethod = false;
        calledReceiveMethod = false;
        calledDestroyMethod = false;
        calledStartMethod = false;

        GatewayCreateDelegate = NULL;
        GatewayReceiveDelegate = NULL;
        GatewayDestroyDelegate = NULL;
        GatewayStartDelegate = NULL;

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
    
    /* Tests_SRS_DOTNET_CORE_04_001: [ DotNetCore_Create shall return NULL if broker is NULL. ] */
    TEST_FUNCTION(DotNetCore_Create_returns_NULL_when_broker_is_Null)
    {
        ///arrange
        CDOTNETCOREMocks mocks;
        const MODULE_API* theAPIS = Module_GetApi(MODULE_API_VERSION_1);;
        

        mocks.ResetAllCalls();

        ///act
        auto result = MODULE_CREATE(theAPIS)(NULL, "Anything");

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /* Tests_SRS_DOTNET_CORE_04_002: [ DotNetCore_Create shall return NULL if configuration is NULL. ] */
    TEST_FUNCTION(DotNetCore_Create_returns_NULL_when_configuration_is_Null)
    {
        ///arrange
        CDOTNETCOREMocks mocks;
        const MODULE_API* theAPIS = Module_GetApi(MODULE_API_VERSION_1);;


        mocks.ResetAllCalls();

        ///act
        auto result = MODULE_CREATE(theAPIS)((BROKER_HANDLE)0x42, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /* Tests_SRS_DOTNET_CORE_04_003: [ DotNetCore_Create shall return NULL if configuration->assemblyName is NULL. ] */
    TEST_FUNCTION(DotNetCore_Create_returns_NULL_when_assembly_name_is_Null)
    {
        ///arrange
        CDOTNETCOREMocks mocks;
        const MODULE_API* theAPIS = Module_GetApi(MODULE_API_VERSION_1);;

        DOTNET_CORE_HOST_CONFIG dotNetConfig;
        dotNetConfig.assemblyName = NULL;
        dotNetConfig.entryType = "mycsharpmodule.classname";
        dotNetConfig.moduleArgs = "module configuration";

        mocks.ResetAllCalls();

        ///act
        auto result = MODULE_CREATE(theAPIS)((BROKER_HANDLE)0x42, &dotNetConfig);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /* Tests_SRS_DOTNET_CORE_04_004: [ DotNetCore_Create shall return NULL if configuration->entryType is NULL. ] */
    TEST_FUNCTION(DotNetCore_Create_returns_NULL_when_entry_type_is_Null)
    {
        ///arrange
        CDOTNETCOREMocks mocks;
        const MODULE_API* theAPIS = Module_GetApi(MODULE_API_VERSION_1);;

        DOTNET_CORE_HOST_CONFIG dotNetConfig;
        dotNetConfig.assemblyName = "/path/to/csharp_module.dll";
        dotNetConfig.entryType = NULL;
        dotNetConfig.moduleArgs = "module configuration";

        mocks.ResetAllCalls();

        ///act
        auto result = MODULE_CREATE(theAPIS)((BROKER_HANDLE)0x42, &dotNetConfig);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /* Tests_SRS_DOTNET_CORE_04_005: [ DotNetCore_Create shall return NULL if configuration->moduleArgs is NULL. ] */
    TEST_FUNCTION(DotNetCore_Create_returns_NULL_when_module_args_is_Null)
    {
        ///arrange
        CDOTNETCOREMocks mocks;
        const MODULE_API* theAPIS = Module_GetApi(MODULE_API_VERSION_1);;

        DOTNET_CORE_HOST_CONFIG dotNetConfig;
        dotNetConfig.assemblyName = "/path/to/csharp_module.dll";
        dotNetConfig.entryType = "mycsharpmodule.classname";
        dotNetConfig.moduleArgs = NULL;

        mocks.ResetAllCalls();

        ///act
        auto result = MODULE_CREATE(theAPIS)((BROKER_HANDLE)0x42, &dotNetConfig);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /* Tests_SRS_DOTNET_CORE_04_035: [ DotNetCore_Create shall return NULL if configuration->clrOptions is NULL. ] */
    TEST_FUNCTION(DotNetCore_Create_returns_NULL_when_clr_Options_is_Null)
    {
        ///arrange
        CDOTNETCOREMocks mocks;
        const MODULE_API* theAPIS = Module_GetApi(MODULE_API_VERSION_1);;

        DOTNET_CORE_HOST_CONFIG dotNetConfig;
        dotNetConfig.assemblyName = "/path/to/csharp_module.dll";
        dotNetConfig.entryType = "mycsharpmodule.classname";
        dotNetConfig.moduleArgs = "module configuration";
        dotNetConfig.clrOptions = NULL;

        mocks.ResetAllCalls();

        ///act
        auto result = MODULE_CREATE(theAPIS)((BROKER_HANDLE)0x42, &dotNetConfig);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /* Tests_SRS_DOTNET_CORE_04_036: [ DotNetCore_Create shall return NULL if configuration->clrOptions->coreClrPath is NULL. ] */
    TEST_FUNCTION(DotNetCore_Create_returns_NULL_when_coreClrPath_is_Null)
    {
        ///arrange
        CDOTNETCOREMocks mocks;
        const MODULE_API* theAPIS = Module_GetApi(MODULE_API_VERSION_1);;

        DOTNET_CORE_HOST_CONFIG dotNetConfig;
        dotNetConfig.assemblyName = "/path/to/csharp_module.dll";
        dotNetConfig.entryType = "mycsharpmodule.classname";
        dotNetConfig.moduleArgs = "module configuration";
        DOTNET_CORE_CLR_OPTIONS coreClrOptions;
        dotNetConfig.clrOptions = &coreClrOptions;
        dotNetConfig.clrOptions->coreClrPath = NULL;

        mocks.ResetAllCalls();

        ///act
        auto result = MODULE_CREATE(theAPIS)((BROKER_HANDLE)0x42, &dotNetConfig);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /* Tests_SRS_DOTNET_CORE_04_037: [ DotNetCore_Create shall return NULL if configuration->clrOptions->trustedPlatformAssembliesLocation is NULL. ] */
    TEST_FUNCTION(DotNetCore_Create_returns_NULL_when_trustedplatformassemblieslocation_is_Null)
    {
        ///arrange
        CDOTNETCOREMocks mocks;
        const MODULE_API* theAPIS = Module_GetApi(MODULE_API_VERSION_1);;

        DOTNET_CORE_HOST_CONFIG dotNetConfig;
        dotNetConfig.assemblyName = "/path/to/csharp_module.dll";
        dotNetConfig.entryType = "mycsharpmodule.classname";
        dotNetConfig.moduleArgs = "module configuration";
        DOTNET_CORE_CLR_OPTIONS coreClrOptions;
        dotNetConfig.clrOptions = &coreClrOptions;
        dotNetConfig.clrOptions->coreClrPath = "coreCLRPath";
        dotNetConfig.clrOptions->trustedPlatformAssembliesLocation = NULL;

        mocks.ResetAllCalls();

        ///act
        auto result = MODULE_CREATE(theAPIS)((BROKER_HANDLE)0x42, &dotNetConfig);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /* Tests_SRS_DOTNET_CORE_04_006: [ DotNetCore_Create shall return NULL if an underlying API call fails. ] */
    TEST_FUNCTION(DotNetCore_Create_returns_NULL_when_DynamicLibrary_LoadLibrary_fails)
    {
        ///arrange
        CDOTNETCOREMocks mocks;
        const MODULE_API* theAPIS = Module_GetApi(MODULE_API_VERSION_1);;

        DOTNET_CORE_HOST_CONFIG dotNetConfig;
        dotNetConfig.assemblyName = "/path/to/csharp_module.dll";
        dotNetConfig.entryType = "mycsharpmodule.classname";
        dotNetConfig.moduleArgs = "module configuration";
        DOTNET_CORE_CLR_OPTIONS coreClrOptions;
        dotNetConfig.clrOptions = &coreClrOptions;
        dotNetConfig.clrOptions->coreClrPath = "coreCLRPath";
        dotNetConfig.clrOptions->trustedPlatformAssembliesLocation = "c:\\TrustedPlatformPath";

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_LoadLibrary(dotNetConfig.clrOptions->coreClrPath))
            .SetReturn((DYNAMIC_LIBRARY_HANDLE)NULL);

        ///act
        auto result = MODULE_CREATE(theAPIS)((BROKER_HANDLE)0x42, &dotNetConfig);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /* Tests_SRS_DOTNET_CORE_04_006: [ DotNetCore_Create shall return NULL if an underlying API call fails. ] */
    TEST_FUNCTION(DotNetCore_Create_returns_NULL_when_DynamicLibrary_FindSymbol_for_coreclr_initialize_fails)
    {
        ///arrange
        CDOTNETCOREMocks mocks;
        const MODULE_API* theAPIS = Module_GetApi(MODULE_API_VERSION_1);;

        DOTNET_CORE_HOST_CONFIG dotNetConfig;
        dotNetConfig.assemblyName = "/path/to/csharp_module.dll";
        dotNetConfig.entryType = "mycsharpmodule.classname";
        dotNetConfig.moduleArgs = "module configuration";
        DOTNET_CORE_CLR_OPTIONS coreClrOptions;
        dotNetConfig.clrOptions = &coreClrOptions;
        dotNetConfig.clrOptions->coreClrPath = "coreCLRPath";
        dotNetConfig.clrOptions->trustedPlatformAssembliesLocation = "c:\\TrustedPlatformPath";

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_LoadLibrary(dotNetConfig.clrOptions->coreClrPath));

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol((DYNAMIC_LIBRARY_HANDLE)0x42, "coreclr_initialize"))
            .SetReturn((void*)NULL);

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_UnloadLibrary((DYNAMIC_LIBRARY_HANDLE)0x42));

        ///act
        auto result = MODULE_CREATE(theAPIS)((BROKER_HANDLE)0x42, &dotNetConfig);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /* Tests_SRS_DOTNET_CORE_04_006: [ DotNetCore_Create shall return NULL if an underlying API call fails. ] */
    TEST_FUNCTION(DotNetCore_Create_returns_NULL_when_DynamicLibrary_FindSymbol_for_coreclr_shutdown_fails)
    {
        ///arrange
        CDOTNETCOREMocks mocks;
        const MODULE_API* theAPIS = Module_GetApi(MODULE_API_VERSION_1);;

        DOTNET_CORE_HOST_CONFIG dotNetConfig;
        dotNetConfig.assemblyName = "/path/to/csharp_module.dll";
        dotNetConfig.entryType = "mycsharpmodule.classname";
        dotNetConfig.moduleArgs = "module configuration";
        DOTNET_CORE_CLR_OPTIONS coreClrOptions;
        dotNetConfig.clrOptions = &coreClrOptions;
        dotNetConfig.clrOptions->coreClrPath = "coreCLRPath";
        dotNetConfig.clrOptions->trustedPlatformAssembliesLocation = "c:\\TrustedPlatformPath";

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_LoadLibrary(dotNetConfig.clrOptions->coreClrPath));

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol((DYNAMIC_LIBRARY_HANDLE)0x42, "coreclr_initialize"));

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol((DYNAMIC_LIBRARY_HANDLE)0x42, "coreclr_shutdown"))
            .SetReturn((void*)NULL);

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_UnloadLibrary((DYNAMIC_LIBRARY_HANDLE)0x42));

        ///act
        auto result = MODULE_CREATE(theAPIS)((BROKER_HANDLE)0x42, &dotNetConfig);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /* Tests_SRS_DOTNET_CORE_04_006: [ DotNetCore_Create shall return NULL if an underlying API call fails. ] */
    TEST_FUNCTION(DotNetCore_Create_returns_NULL_when_DynamicLibrary_FindSymbol_for_coreclr_create_delegate_fails)
    {
        ///arrange
        CDOTNETCOREMocks mocks;
        const MODULE_API* theAPIS = Module_GetApi(MODULE_API_VERSION_1);;

        DOTNET_CORE_HOST_CONFIG dotNetConfig;
        dotNetConfig.assemblyName = "/path/to/csharp_module.dll";
        dotNetConfig.entryType = "mycsharpmodule.classname";
        dotNetConfig.moduleArgs = "module configuration";
        DOTNET_CORE_CLR_OPTIONS coreClrOptions;
        dotNetConfig.clrOptions = &coreClrOptions;
        dotNetConfig.clrOptions->coreClrPath = "coreCLRPath";
        dotNetConfig.clrOptions->trustedPlatformAssembliesLocation = "c:\\TrustedPlatformPath";

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_LoadLibrary(dotNetConfig.clrOptions->coreClrPath));

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol((DYNAMIC_LIBRARY_HANDLE)0x42, "coreclr_initialize"));

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol((DYNAMIC_LIBRARY_HANDLE)0x42, "coreclr_shutdown"));

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol((DYNAMIC_LIBRARY_HANDLE)0x42, "coreclr_create_delegate"))
            .SetReturn((void*)NULL);

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_UnloadLibrary((DYNAMIC_LIBRARY_HANDLE)0x42));

        ///act
        auto result = MODULE_CREATE(theAPIS)((BROKER_HANDLE)0x42, &dotNetConfig);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /* Tests_SRS_DOTNET_CORE_04_006: [ DotNetCore_Create shall return NULL if an underlying API call fails. ] */
    TEST_FUNCTION(DotNetCore_Create_returns_NULL_when_initializeDotNetCoreCLR_fails)
    {
        ///arrange
        CDOTNETCOREMocks mocks;
        const MODULE_API* theAPIS = Module_GetApi(MODULE_API_VERSION_1);;

        DOTNET_CORE_HOST_CONFIG dotNetConfig;
        dotNetConfig.assemblyName = "/path/to/csharp_module.dll";
        dotNetConfig.entryType = "mycsharpmodule.classname";
        dotNetConfig.moduleArgs = "module configuration";
        DOTNET_CORE_CLR_OPTIONS coreClrOptions;
        dotNetConfig.clrOptions = &coreClrOptions;
        dotNetConfig.clrOptions->coreClrPath = "coreCLRPath";
        dotNetConfig.clrOptions->trustedPlatformAssembliesLocation = "c:\\TrustedPlatformPath";

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_LoadLibrary(dotNetConfig.clrOptions->coreClrPath));

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol((DYNAMIC_LIBRARY_HANDLE)0x42, "coreclr_initialize"));

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol((DYNAMIC_LIBRARY_HANDLE)0x42, "coreclr_shutdown"));

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol((DYNAMIC_LIBRARY_HANDLE)0x42, "coreclr_create_delegate"));

        STRICT_EXPECTED_CALL(mocks, initializeDotNetCoreCLR((coreclr_initialize_ptr)0x42, "c:\\TrustedPlatformPath", IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .SetReturn(false);

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_UnloadLibrary((DYNAMIC_LIBRARY_HANDLE)0x42));

        ///act
        auto result = MODULE_CREATE(theAPIS)((BROKER_HANDLE)0x42, &dotNetConfig);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /* Tests_SRS_DOTNET_CORE_04_006: [ DotNetCore_Create shall return NULL if an underlying API call fails. ] */
    TEST_FUNCTION(DotNetCore_Create_returns_NULL_when_m_ptr_coreclr_create_delegate_fails)
    {
        ///arrange
        CDOTNETCOREMocks mocks;
        const MODULE_API* theAPIS = Module_GetApi(MODULE_API_VERSION_1);

        failCreateDelegate = true;

        DOTNET_CORE_HOST_CONFIG dotNetConfig;
        dotNetConfig.assemblyName = "/path/to/csharp_module.dll";
        dotNetConfig.entryType = "mycsharpmodule.classname";
        dotNetConfig.moduleArgs = "module configuration";
        DOTNET_CORE_CLR_OPTIONS coreClrOptions;
        dotNetConfig.clrOptions = &coreClrOptions;
        dotNetConfig.clrOptions->coreClrPath = "coreCLRPath";
        dotNetConfig.clrOptions->trustedPlatformAssembliesLocation = "c:\\TrustedPlatformPath";

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_LoadLibrary(dotNetConfig.clrOptions->coreClrPath));

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol((DYNAMIC_LIBRARY_HANDLE)0x42, "coreclr_initialize"));

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol((DYNAMIC_LIBRARY_HANDLE)0x42, "coreclr_shutdown"));

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol((DYNAMIC_LIBRARY_HANDLE)0x42, "coreclr_create_delegate"));

        STRICT_EXPECTED_CALL(mocks, initializeDotNetCoreCLR((coreclr_initialize_ptr)0x42, "c:\\TrustedPlatformPath", IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4);

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_UnloadLibrary((DYNAMIC_LIBRARY_HANDLE)0x42));

        ///act
        auto result = MODULE_CREATE(theAPIS)((BROKER_HANDLE)0x42, &dotNetConfig);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /* Tests_SRS_DOTNET_CORE_04_006: [ DotNetCore_Create shall return NULL if an underlying API call fails. ] */
    TEST_FUNCTION(DotNetCore_Create_returns_NULL_when_m_ptr_coreclr_receive_delegate_fails)
    {
        ///arrange
        CDOTNETCOREMocks mocks;
        const MODULE_API* theAPIS = Module_GetApi(MODULE_API_VERSION_1);

        failReceiveDelegate = true;

        DOTNET_CORE_HOST_CONFIG dotNetConfig;
        dotNetConfig.assemblyName = "/path/to/csharp_module.dll";
        dotNetConfig.entryType = "mycsharpmodule.classname";
        dotNetConfig.moduleArgs = "module configuration";
        DOTNET_CORE_CLR_OPTIONS coreClrOptions;
        dotNetConfig.clrOptions = &coreClrOptions;
        dotNetConfig.clrOptions->coreClrPath = "coreCLRPath";
        dotNetConfig.clrOptions->trustedPlatformAssembliesLocation = "c:\\TrustedPlatformPath";

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_LoadLibrary(dotNetConfig.clrOptions->coreClrPath));

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol((DYNAMIC_LIBRARY_HANDLE)0x42, "coreclr_initialize"));

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol((DYNAMIC_LIBRARY_HANDLE)0x42, "coreclr_shutdown"));

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol((DYNAMIC_LIBRARY_HANDLE)0x42, "coreclr_create_delegate"));

        STRICT_EXPECTED_CALL(mocks, initializeDotNetCoreCLR((coreclr_initialize_ptr)0x42, "c:\\TrustedPlatformPath", IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4);

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_UnloadLibrary((DYNAMIC_LIBRARY_HANDLE)0x42));

        ///act
        auto result = MODULE_CREATE(theAPIS)((BROKER_HANDLE)0x42, &dotNetConfig);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /* Tests_SRS_DOTNET_CORE_04_006: [ DotNetCore_Create shall return NULL if an underlying API call fails. ] */
    TEST_FUNCTION(DotNetCore_Create_returns_NULL_when_m_ptr_coreclr_destroy_delegate_fails)
    {
        ///arrange
        CDOTNETCOREMocks mocks;
        const MODULE_API* theAPIS = Module_GetApi(MODULE_API_VERSION_1);

        failDestroyDelegate = true;

        DOTNET_CORE_HOST_CONFIG dotNetConfig;
        dotNetConfig.assemblyName = "/path/to/csharp_module.dll";
        dotNetConfig.entryType = "mycsharpmodule.classname";
        dotNetConfig.moduleArgs = "module configuration";
        DOTNET_CORE_CLR_OPTIONS coreClrOptions;
        dotNetConfig.clrOptions = &coreClrOptions;
        dotNetConfig.clrOptions->coreClrPath = "coreCLRPath";
        dotNetConfig.clrOptions->trustedPlatformAssembliesLocation = "c:\\TrustedPlatformPath";

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_LoadLibrary(dotNetConfig.clrOptions->coreClrPath));

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol((DYNAMIC_LIBRARY_HANDLE)0x42, "coreclr_initialize"));

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol((DYNAMIC_LIBRARY_HANDLE)0x42, "coreclr_shutdown"));

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol((DYNAMIC_LIBRARY_HANDLE)0x42, "coreclr_create_delegate"));

        STRICT_EXPECTED_CALL(mocks, initializeDotNetCoreCLR((coreclr_initialize_ptr)0x42, "c:\\TrustedPlatformPath", IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4);

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_UnloadLibrary((DYNAMIC_LIBRARY_HANDLE)0x42));

        ///act
        auto result = MODULE_CREATE(theAPIS)((BROKER_HANDLE)0x42, &dotNetConfig);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /* Tests_SRS_DOTNET_CORE_04_007: [ DotNetCore_Create shall return a non-NULL MODULE_HANDLE when successful. ] */
    /* Tests_SRS_DOTNET_CORE_04_010: [ DotNetCore_Create shall load coreclr library, if not loaded yet. ] */
    /* Tests_SRS_DOTNET_CORE_04_011: [ DotNetCore_Create shall get address for 3 external methods coreclr_initialize, coreclr_shutdown and coreclr_create_delegate and save it to global reference for Dot Net Core binding. ] */
    /* Tests_SRS_DOTNET_CORE_04_013: [DotNetCore_Create shall call coreclr_create_delegate to be able to call Microsoft.Azure.Devices.Gateway.GatewayDelegatesGateway.Delegates_Create] */
    /* Tests_SRS_DOTNET_CORE_04_021 : [DotNetCore_Create shall call coreclr_create_delegate to be able to call Microsoft.Azure.Devices.Gateway.GatewayDelegatesGateway.Delegates_Receive]*/
    /* Tests_SRS_DOTNET_CORE_04_024 : [DotNetCore_Destroy shall call coreclr_create_delegate to be able to call Microsoft.Azure.Devices.Gateway.GatewayDelegatesGateway.Delegates_Destroy]*/
    /* Tests_SRS_DOTNET_CORE_04_014: [ DotNetCore_Create shall call Microsoft.Azure.Devices.Gateway.GatewayDelegatesGateway.Delegates_Create C# method, implemented on Microsoft.Azure.Devices.Gateway.dll. ] */
    TEST_FUNCTION(DotNetCore_Create_succeed)
    {
        ///arrange
        CDOTNETCOREMocks mocks;
        const MODULE_API* theAPIS = Module_GetApi(MODULE_API_VERSION_1);


        DOTNET_CORE_HOST_CONFIG dotNetConfig;
        dotNetConfig.assemblyName = "/path/to/csharp_module.dll";
        dotNetConfig.entryType = "mycsharpmodule.classname";
        dotNetConfig.moduleArgs = "module configuration";
        DOTNET_CORE_CLR_OPTIONS coreClrOptions;
        dotNetConfig.clrOptions = &coreClrOptions;
        dotNetConfig.clrOptions->coreClrPath = "coreCLRPath";
        dotNetConfig.clrOptions->trustedPlatformAssembliesLocation = "c:\\TrustedPlatformPath";

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_LoadLibrary(dotNetConfig.clrOptions->coreClrPath));

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol((DYNAMIC_LIBRARY_HANDLE)0x42, "coreclr_initialize"));

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol((DYNAMIC_LIBRARY_HANDLE)0x42, "coreclr_shutdown"));

        STRICT_EXPECTED_CALL(mocks, DynamicLibrary_FindSymbol((DYNAMIC_LIBRARY_HANDLE)0x42, "coreclr_create_delegate"));

        STRICT_EXPECTED_CALL(mocks, initializeDotNetCoreCLR((coreclr_initialize_ptr)0x42, "c:\\TrustedPlatformPath", IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4);

        STRICT_EXPECTED_CALL(mocks, STRING_construct("/path/to/csharp_module.dll"));

        ///act
        auto result = MODULE_CREATE(theAPIS)((BROKER_HANDLE)0x42, &dotNetConfig);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(calledCreateMethod);
        ASSERT_IS_NOT_NULL(result);

        ///cleanup
        MODULE_DESTROY(theAPIS)(result);
    }

    /* Tests_SRS_DOTNET_CORE_004_016: [ DotNetCore_Start shall call coreclr_create_delegate to be able to call Microsoft.Azure.Devices.Gateway.GatewayDelegatesGateway.Delegates_Start ] */
    /* Tests_SRS_DOTNET_CORE_004_017: [ DotNetCore_Start shall call Microsoft.Azure.Devices.Gateway.GatewayDelegatesGateway.Delegates_Start C# method, implemented on Microsoft.Azure.Devices.Gateway.dll. ] */
    TEST_FUNCTION(DotNetCore_Start_succeeds)
    {
        ///arrage
        CDOTNETCOREMocks mocks;
        const MODULE_API* theAPIS = Module_GetApi(MODULE_API_VERSION_1);

        DOTNET_CORE_HOST_CONFIG dotNetConfig;
        dotNetConfig.assemblyName = "/path/to/csharp_module.dll";
        dotNetConfig.entryType = "mycsharpmodule.classname";
        dotNetConfig.moduleArgs = "module configuration";
        DOTNET_CORE_CLR_OPTIONS coreClrOptions;
        dotNetConfig.clrOptions = &coreClrOptions;
        dotNetConfig.clrOptions->coreClrPath = "coreCLRPath";
        dotNetConfig.clrOptions->trustedPlatformAssembliesLocation = "c:\\TrustedPlatformPath";

        auto  result = MODULE_CREATE(theAPIS)((BROKER_HANDLE)0x42, &dotNetConfig);

        mocks.ResetAllCalls();


        ///act
        MODULE_START(theAPIS)((MODULE_HANDLE)result);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(calledStartMethod);

        ///cleanup
        MODULE_DESTROY(theAPIS)((MODULE_HANDLE)result);
    }

    /* Tests_SRS_DOTNET_CORE_004_015: [ DotNetCore_Start shall do nothing if module is NULL. ] */
    TEST_FUNCTION(DotNetCore_Start_shall_do_nothing_if_module_is_NULL)
    {
        ///arrage
        CDOTNETCOREMocks mocks;
        const MODULE_API* theAPIS = Module_GetApi(MODULE_API_VERSION_1);

        ///act
        MODULE_START(theAPIS)(NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_FALSE(calledStartMethod);

        ///cleanup
    }
       
    /* Tests_SRS_DOTNET_CORE_04_018: [ DotNetCore_Receive shall do nothing if module is NULL. ] */  
    TEST_FUNCTION(DotNetCore_Receive_does_nothing_when_modulehandle_is_Null)
    {
        ///arrange
        CDOTNETCOREMocks mocks;
        const MODULE_API* theAPIS = Module_GetApi(MODULE_API_VERSION_1);;


        mocks.ResetAllCalls();

        ///act
        MODULE_RECEIVE(theAPIS)(NULL, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /* Tests_SRS_DOTNET_CORE_04_019: [ DotNetCore_Receive shall do nothing if message is NULL. ] */
    TEST_FUNCTION(DotNetCore_Receive_does_nothing_when_message_is_Null)
    {
        ///arrange
        CDOTNETCOREMocks mocks;
        const MODULE_API* theAPIS = Module_GetApi(MODULE_API_VERSION_1);;


        mocks.ResetAllCalls();

        ///act
        MODULE_RECEIVE(theAPIS)((MODULE_HANDLE)0x42, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /* Tests_SRS_DOTNET_CORE_04_020: [ DotNetCore_Receive shall call Message_ToByteArray to serialize message. ] */
    /* Tests_SRS_DOTNET_CORE_04_022: [ DotNetCore_Receive shall call Microsoft.Azure.Devices.Gateway.GatewayDelegatesGateway.Delegates_Receive C# method, implemented on Microsoft.Azure.Devices.Gateway.dll. ] */
    TEST_FUNCTION(DotNetCore_Receive_succeed)
    {
        ///arrange
        CDOTNETCOREMocks mocks;
        const MODULE_API* theAPIS = Module_GetApi(MODULE_API_VERSION_1);


        DOTNET_CORE_HOST_CONFIG dotNetConfig;
        dotNetConfig.assemblyName = "/path/to/csharp_module.dll";
        dotNetConfig.entryType = "mycsharpmodule.classname";
        dotNetConfig.moduleArgs = "module configuration";
        DOTNET_CORE_CLR_OPTIONS coreClrOptions;
        dotNetConfig.clrOptions = &coreClrOptions;
        dotNetConfig.clrOptions->coreClrPath = "coreCLRPath";
        dotNetConfig.clrOptions->trustedPlatformAssembliesLocation = "c:\\TrustedPlatformPath";

        auto result = MODULE_CREATE(theAPIS)((BROKER_HANDLE)0x42, &dotNetConfig);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Message_ToByteArray((MESSAGE_HANDLE)0x42, NULL, 0));

        STRICT_EXPECTED_CALL(mocks, Message_ToByteArray(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreAllArguments();


        ///act
        MODULE_RECEIVE(theAPIS)(result, (MESSAGE_HANDLE)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(calledReceiveMethod);

        ///cleanup
        MODULE_DESTROY(theAPIS)(result);
    }

    /* Tests_SRS_DOTNET_CORE_04_023: [ DotNetCore_Destroy shall do nothing if module is NULL. ] */
    TEST_FUNCTION(DotNetCore_Destroy_does_nothing_when_modulehandle_is_Null)
    {
        ///arrange
        CDOTNETCOREMocks mocks;
        const MODULE_API* theAPIS = Module_GetApi(MODULE_API_VERSION_1);;


        mocks.ResetAllCalls();

        ///act
        MODULE_DESTROY(theAPIS)(NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /* Tests_SRS_DOTNET_CORE_04_025: [ DotNetCore_Destroy shall call Microsoft.Azure.Devices.Gateway.GatewayDelegatesGateway.Delegates_Destroy C# method, implemented on Microsoft.Azure.Devices.Gateway.dll. ] */
    /* Tests_SRS_DOTNET_CORE_04_038: [ DotNetCore_Destroy shall release all resources allocated by DotNetCore_Create. ] */
    /* Tests_SRS_DOTNET_CORE_04_039: [ DotNetCore_Destroy shall verify that there is no module and shall shutdown the dotnet core clr. ] */
    TEST_FUNCTION(DotNetCore_Destroy_with_2_modules_succeed)
    {
        ///arrange
        CDOTNETCOREMocks mocks;
        const MODULE_API* theAPIS = Module_GetApi(MODULE_API_VERSION_1);


        DOTNET_CORE_HOST_CONFIG dotNetConfig;
        dotNetConfig.assemblyName = "/path/to/csharp_module.dll";
        dotNetConfig.entryType = "mycsharpmodule.classname";
        dotNetConfig.moduleArgs = "module configuration";
        DOTNET_CORE_CLR_OPTIONS coreClrOptions;
        dotNetConfig.clrOptions = &coreClrOptions;
        dotNetConfig.clrOptions->coreClrPath = "coreCLRPath";
        dotNetConfig.clrOptions->trustedPlatformAssembliesLocation = "c:\\TrustedPlatformPath";

        auto result = MODULE_CREATE(theAPIS)((BROKER_HANDLE)0x42, &dotNetConfig);
        auto result2 = MODULE_CREATE(theAPIS)((BROKER_HANDLE)0x42, &dotNetConfig);

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_delete((STRING_HANDLE)0x42));
        STRICT_EXPECTED_CALL(mocks, STRING_delete((STRING_HANDLE)0x42));

        ///act
        MODULE_DESTROY(theAPIS)(result);
        MODULE_DESTROY(theAPIS)(result2);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(calledDestroyMethod);

        ///cleanup
    }

    /* Tests_SRS_DOTNET_CORE_04_025: [ DotNetCore_Destroy shall call Microsoft.Azure.Devices.Gateway.GatewayDelegatesGateway.Delegates_Destroy C# method, implemented on Microsoft.Azure.Devices.Gateway.dll. ] */
    /* Tests_SRS_DOTNET_CORE_04_038: [ DotNetCore_Destroy shall release all resources allocated by DotNetCore_Create. ] */
    /* Tests_SRS_DOTNET_CORE_04_039: [ DotNetCore_Destroy shall verify that there is no module and shall shutdown the dotnet core clr. ] */
    TEST_FUNCTION(DotNetCore_Destroy_succeed)
    {
        ///arrange
        CDOTNETCOREMocks mocks;
        const MODULE_API* theAPIS = Module_GetApi(MODULE_API_VERSION_1);


        DOTNET_CORE_HOST_CONFIG dotNetConfig;
        dotNetConfig.assemblyName = "/path/to/csharp_module.dll";
        dotNetConfig.entryType = "mycsharpmodule.classname";
        dotNetConfig.moduleArgs = "module configuration";
        DOTNET_CORE_CLR_OPTIONS coreClrOptions;
        dotNetConfig.clrOptions = &coreClrOptions;
        dotNetConfig.clrOptions->coreClrPath = "coreCLRPath";
        dotNetConfig.clrOptions->trustedPlatformAssembliesLocation = "c:\\TrustedPlatformPath";

        auto result = MODULE_CREATE(theAPIS)((BROKER_HANDLE)0x42, &dotNetConfig);

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_delete((STRING_HANDLE)0x42));

        ///act
        MODULE_DESTROY(theAPIS)(result);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(calledDestroyMethod);

        ///cleanup
    }


    /* Tests_SRS_DOTNET_CORE_04_027: [ Module_DotNetCoreHost_PublishMessage shall return false if broker is NULL. ] */
    TEST_FUNCTION(DotNetCore_Module_DotNetHost_PublishMessage_if_broker_is_NULL_return_false)
    {
        ///arrange
        CDOTNETCOREMocks mocks;

        ///act
        auto result = Module_DotNetCoreHost_PublishMessage(NULL, (MODULE_HANDLE)0x42, (const unsigned char *)"AnyContent", 11);

        ///assert
        ASSERT_IS_FALSE(result);

        ///cleanup
    }

    /* Tests_SRS_DOTNET_CORE_04_028: [ Module_DotNetCoreHost_PublishMessage shall return false if sourceModule is NULL. ] */
    TEST_FUNCTION(DotNetCore_Module_DotNetHost_PublishMessage_if_sourceModule_is_NULL_return_false)
    {
        ///arrange
        CDOTNETCOREMocks mocks;

        ///act
        auto result = Module_DotNetCoreHost_PublishMessage((BROKER_HANDLE)0x42, (MODULE_HANDLE)NULL, (const unsigned char *)"AnyContent", 11);

        ///assert
        ASSERT_IS_FALSE(result);

        ///cleanup
    }

    /* Tests_SRS_DOTNET_CORE_04_029: [ Module_DotNetCoreHost_PublishMessage shall return false if message is NULL, or size if lower than 0. ] */
    TEST_FUNCTION(DotNetCore_Module_DotNetHost_PublishMessage_if_source_is_NULL_return_false)
    {
        ///arrange
        CDOTNETCOREMocks mocks;

        ///act
        auto result = Module_DotNetCoreHost_PublishMessage((BROKER_HANDLE)0x42, (MODULE_HANDLE)0x42, NULL, 11);

        ///assert
        ASSERT_IS_FALSE(result);

        ///cleanup
    }

    /* Tests_SRS_DOTNET_CORE_04_029: [ Module_DotNetCoreHost_PublishMessage shall return false if message is NULL, or size if lower than 0. ] */
    TEST_FUNCTION(DotNetCore_Module_DotNetHost_PublishMessage_if_size_is_lower_than_zero_return_false)
    {
        ///arrange
        CDOTNETCOREMocks mocks;

        ///act
        auto result = Module_DotNetCoreHost_PublishMessage((BROKER_HANDLE)0x42, (MODULE_HANDLE)0x42, (const unsigned char *)"AnyContent", -1);

        ///assert
        ASSERT_IS_FALSE(result);

        ///cleanup
    }


    /* Tests_SRS_DOTNET_CORE_04_030: [ Module_DotNetCoreHost_PublishMessage shall create a message from message and size by invoking Message_CreateFromByteArray. ] */
    TEST_FUNCTION(DotNetCore_Module_DotNetHost_PublishMessage_fails_when_Message_CreateFromByteArray_fail)
    {
        ///arrange
        CDOTNETCOREMocks mocks;

        STRICT_EXPECTED_CALL(mocks, Message_CreateFromByteArray((const unsigned char*)"AnyContent", 11))
            .IgnoreArgument(1)
            .SetFailReturn((MESSAGE_HANDLE)NULL);

        ///act
        auto result = Module_DotNetCoreHost_PublishMessage((BROKER_HANDLE)0x42, (MODULE_HANDLE)0x42, (const unsigned char *)"AnyContent", 11);

        ///assert
        ASSERT_IS_FALSE(result);

        ///cleanup
    }

    /* Tests_SRS_DOTNET_CORE_04_031: [ If Message_CreateFromByteArray fails, Module_DotNetCoreHost_PublishMessage shall fail. ] */
    TEST_FUNCTION(DotNetCore_Module_DotNetHost_PublishMessage_fails_when_Broker_Publish_fail)
    {
        ///arrange
        CDOTNETCOREMocks mocks;

        STRICT_EXPECTED_CALL(mocks, Message_CreateFromByteArray((const unsigned char*)"AnyContent", 11))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Broker_Publish((BROKER_HANDLE)0x42, (MODULE_HANDLE)0x42, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .SetFailReturn((BROKER_RESULT)BROKER_ERROR);

        STRICT_EXPECTED_CALL(mocks, Message_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        auto result = Module_DotNetCoreHost_PublishMessage((BROKER_HANDLE)0x42, (MODULE_HANDLE)0x42, (const unsigned char *)"AnyContent", 11);

        ///assert
        ASSERT_IS_FALSE(result);

        ///cleanup
    }

    /* Tests_SRS_DOTNET_CORE_04_032: [ Module_DotNetCoreHost_PublishMessage shall call Broker_Publish passing broker, sourceModule, message and size. ] */
    /* Tests_SRS_DOTNET_CORE_04_033: [ If Broker_Publish fails Module_DotNetCoreHost_PublishMessage shall fail. ] */
    /* Tests_SRS_DOTNET_CORE_04_034: [ If Broker_Publish succeeds Module_DotNetCoreHost_PublishMessage shall succeed. ] */
    TEST_FUNCTION(DotNetCore_Module_DotNetHost_PublishMessage_succeed)
    {
        ///arrange
        CDOTNETCOREMocks mocks;

        STRICT_EXPECTED_CALL(mocks, Message_CreateFromByteArray((const unsigned char*)"AnyContent", 11))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, Broker_Publish((BROKER_HANDLE)0x42, (MODULE_HANDLE)0x42, IGNORED_PTR_ARG))
            .IgnoreArgument(3);

        STRICT_EXPECTED_CALL(mocks, Message_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        auto result = Module_DotNetCoreHost_PublishMessage((BROKER_HANDLE)0x42, (MODULE_HANDLE)0x42, (const unsigned char *)"AnyContent", 11);

        ///assert
        ASSERT_IS_TRUE(result);

        ///cleanup
    }


    /* Tests_SRS_DOTNET_CORE_04_040: [Module_DotNetCoreHost_SetBindingDelegates shall just assign createAddress to GatewayCreateDelegate] */
    /* Tests_SRS_DOTNET_CORE_04_041: [Module_DotNetCoreHost_SetBindingDelegates shall just assign receiveAddress to GatewayReceiveDelegate] */
    /* Tests_SRS_DOTNET_CORE_04_042: [Module_DotNetCoreHost_SetBindingDelegates shall just assign destroyAddress to GatewayDestroyDelegate] */
    /* Tests_SRS_DOTNET_CORE_04_043: [Module_DotNetCoreHost_SetBindingDelegates shall just assign startAddress to GatewayStartDelegate] */
    TEST_FUNCTION(Module_DotNetCoreHost_SetBindingDelegates_setting_delegates_succeed)
    {
        ///arrange
        CDOTNETCOREMocks mocks;

        ///act
        Module_DotNetCoreHost_SetBindingDelegates((intptr_t)0x42, (intptr_t)0x43, (intptr_t)0x44, (intptr_t)0x45);

        ///assert
        ASSERT_ARE_EQUAL(long, (long)GatewayCreateDelegate, 0x42);
        ASSERT_ARE_EQUAL(long, (long)GatewayReceiveDelegate, 0x43);
        ASSERT_ARE_EQUAL(long, (long)GatewayDestroyDelegate, 0x44);
        ASSERT_ARE_EQUAL(long, (long)GatewayStartDelegate, 0x45);

        ///cleanup
    }


    /* Tests_SRS_DOTNET_CORE_04_026:: [ Module_GetApi shall return out the provided MODULES_API structure with required module's APIs functions. ] */
    TEST_FUNCTION(DotNetCore_Module_GetApi_returns_non_NULL)
    {
        ///arrange
        CDOTNETCOREMocks mocks;

        mocks.ResetAllCalls();
        ///act
        const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

        ///assert
        ASSERT_IS_TRUE(MODULE_DESTROY(apis) != NULL);
        ASSERT_IS_TRUE(MODULE_CREATE(apis) != NULL);
        ASSERT_IS_TRUE(MODULE_START(apis) != NULL);
        ASSERT_IS_TRUE(MODULE_RECEIVE(apis) != NULL);

        ///cleanup
    }


END_TEST_SUITE(dotnetcore_ut)
