// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#include <cstdbool>
#include <cstddef>
#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"
#include "azure_c_shared_utility/lock.h"
#include "modules_manager.h"

using namespace dotnetcore_module;

static MICROMOCK_MUTEX_HANDLE g_testByTest;
static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

#define GBALLOC_H

extern "C" int gballoc_init(void);
extern "C" void gballoc_deinit(void);
extern "C" void* gballoc_malloc(size_t size);
extern "C" void* gballoc_calloc(size_t nmemb, size_t size);
extern "C" void* gballoc_realloc(void* ptr, size_t size);
extern "C" void gballoc_free(void* ptr);

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
#include "vector.c"
#include "strings.c"

};

namespace dotnetcore_module
{
    class ModulesManagerForUnitTest : public ModulesManager
    {
    public:
        static void CleanUpMembers()
        {
            if (m_instance != nullptr)
            {
                delete m_instance;
                m_instance = nullptr;
            }

            if (m_lock_handle != nullptr)
            {
                Lock_Deinit(m_lock_handle);
                m_lock_handle = nullptr;
            }
        }
    };
};


TYPED_MOCK_CLASS(CMODULESMANAGER_Mocks, CGlobalMock)
{
public:

    CMODULESMANAGER_Mocks()
    {
    }

    virtual ~CMODULESMANAGER_Mocks()
    {
    }


    // memory
    MOCK_STATIC_METHOD_1(, void*, gballoc_malloc, size_t, size)
        void* result2 = BASEIMPLEMENTATION::gballoc_malloc(size);
    MOCK_METHOD_END(void*, result2);

    MOCK_STATIC_METHOD_1(, void, gballoc_free, void*, ptr)
        BASEIMPLEMENTATION::gballoc_free(ptr);
    MOCK_VOID_METHOD_END()

    // string
    MOCK_STATIC_METHOD_1(, STRING_HANDLE, STRING_clone, STRING_HANDLE, handle)
        STRING_HANDLE result2 = BASEIMPLEMENTATION::STRING_clone(handle);
    MOCK_METHOD_END(STRING_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, STRING_HANDLE, STRING_construct, const char*, psz)
        STRING_HANDLE result2 = BASEIMPLEMENTATION::STRING_construct(psz);
    MOCK_METHOD_END(STRING_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, void, STRING_delete, STRING_HANDLE, handle)
        BASEIMPLEMENTATION::STRING_delete(handle);
    MOCK_VOID_METHOD_END();

    //Lock
    MOCK_STATIC_METHOD_0(, LOCK_HANDLE, Lock_Init)
    MOCK_METHOD_END(LOCK_HANDLE, (LOCK_HANDLE)0x42)


    MOCK_STATIC_METHOD_1(, LOCK_RESULT, Lock, LOCK_HANDLE, handle);
    MOCK_METHOD_END(LOCK_RESULT, LOCK_OK)

    MOCK_STATIC_METHOD_1(, LOCK_RESULT, Unlock, LOCK_HANDLE, handle);
    MOCK_METHOD_END(LOCK_RESULT, LOCK_OK)

    MOCK_STATIC_METHOD_1(, LOCK_RESULT, Lock_Deinit, LOCK_HANDLE, handle);
    MOCK_METHOD_END(LOCK_RESULT, LOCK_OK)
};

extern "C"
{
    // memory
    DECLARE_GLOBAL_MOCK_METHOD_1(CMODULESMANAGER_Mocks, , void*, gballoc_malloc, size_t, size);
    DECLARE_GLOBAL_MOCK_METHOD_1(CMODULESMANAGER_Mocks, , void, gballoc_free, void*, ptr);

    // string
    DECLARE_GLOBAL_MOCK_METHOD_1(CMODULESMANAGER_Mocks, , STRING_HANDLE, STRING_clone, STRING_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_1(CMODULESMANAGER_Mocks, , STRING_HANDLE, STRING_construct, const char*, psz);
    DECLARE_GLOBAL_MOCK_METHOD_1(CMODULESMANAGER_Mocks, , void, STRING_delete, STRING_HANDLE, handle);


    // lock
    DECLARE_GLOBAL_MOCK_METHOD_0(CMODULESMANAGER_Mocks, , LOCK_HANDLE, Lock_Init);
    DECLARE_GLOBAL_MOCK_METHOD_1(CMODULESMANAGER_Mocks, , LOCK_RESULT, Lock, LOCK_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_1(CMODULESMANAGER_Mocks, , LOCK_RESULT, Unlock, LOCK_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_1(CMODULESMANAGER_Mocks, , LOCK_RESULT, Lock_Deinit, LOCK_HANDLE, handle);
}


BEGIN_TEST_SUITE(modules_manager_ut)
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
    
    /* Tests_SRS_DOTNET_CORE_MODULES_MGR_04_001: [ This method shall acquire the lock on a static lock object. ] */
    /* Tests_SRS_DOTNET_CORE_MODULES_MGR_04_003: [ This method shall create an instance of ModulesManager if this is the first call. ] */
    /* Tests_SRS_DOTNET_CORE_MODULES_MGR_04_004: [ This method shall return a non-NULL pointer to a ModulesManager instance when the object has been successfully insantiated. ] */
    /* Tests_SRS_DOTNET_CORE_MODULES_MGR_04_016: [ This method shall release the lock on the static lock object before returning. ] */
    TEST_FUNCTION(ModulesManager_Get_success)
    {
        ///arrange
        CMODULESMANAGER_Mocks mocks;

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Lock((LOCK_HANDLE)0x42));
        STRICT_EXPECTED_CALL(mocks, Lock_Init());  //First LockInit if the for Static Lock on GET method.
        STRICT_EXPECTED_CALL(mocks, Lock_Init());  //Second LockInit if the constructor of Lock Class to be used by Aquire Lock in the future.
        STRICT_EXPECTED_CALL(mocks, Unlock((LOCK_HANDLE)0x42));

        ///act
        auto module_manager = ModulesManagerForUnitTest::Get();

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ASSERT_IS_NOT_NULL(module_manager);

        ///cleanup
        ModulesManagerForUnitTest::CleanUpMembers();


    }

    /* Tests_SRS_DOTNET_CORE_MODULES_MGR_04_002: [ This method shall return NULL if an underlying API call fails. ] */
    TEST_FUNCTION(ModulesManager_LockInit_fail_Get_Fail)
    {
        ///arrange
        CMODULESMANAGER_Mocks mocks;

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Lock_Init())
            .SetReturn((LOCK_HANDLE)NULL);  //First LockInit if the for Static Lock on GET method.

        ///act
        auto module_manager = ModulesManagerForUnitTest::Get();

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ASSERT_IS_NULL(module_manager);

        ///cleanup
        ModulesManagerForUnitTest::CleanUpMembers();

    }

    /* Tests_SRS_DOTNET_CORE_MODULES_MGR_04_002: [ This method shall return NULL if an underlying API call fails. ] */
    TEST_FUNCTION(ModulesManager_Lock_fail_Get_Fail)
    {
        ///arrange
        CMODULESMANAGER_Mocks mocks;

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Lock_Init());  //First LockInit if the for Static Lock on GET method.

        STRICT_EXPECTED_CALL(mocks, Lock((LOCK_HANDLE)0x42))
            .SetReturn((LOCK_RESULT)LOCK_ERROR);
        STRICT_EXPECTED_CALL(mocks, Lock_Deinit((LOCK_HANDLE)0x42));

        ///act
        auto module_manager = ModulesManagerForUnitTest::Get();

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ASSERT_IS_NULL(module_manager);

        ///cleanup
        ModulesManagerForUnitTest::CleanUpMembers();


    }


    /* Tests_SRS_DOTNET_CORE_MODULES_MGR_04_002: [ This method shall return NULL if an underlying API call fails. ] */
    TEST_FUNCTION(ModulesManager_UnLock_fail_Get_Fail)
    {
        ///arrange
        CMODULESMANAGER_Mocks mocks;

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Lock((LOCK_HANDLE)0x42));
        STRICT_EXPECTED_CALL(mocks, Lock_Init());  //First LockInit if the for Static Lock on GET method.
        STRICT_EXPECTED_CALL(mocks, Lock_Init());  //Second LockInit if the constructor of Lock Class to be used by Aquire Lock in the future.
        STRICT_EXPECTED_CALL(mocks, Unlock((LOCK_HANDLE)0x42))
            .SetReturn((LOCK_RESULT)LOCK_ERROR);
        STRICT_EXPECTED_CALL(mocks, Lock_Deinit((LOCK_HANDLE)0x42));

        ///act
        auto module_manager = ModulesManagerForUnitTest::Get();

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ASSERT_IS_NULL(module_manager);

        ///cleanup
        ModulesManagerForUnitTest::CleanUpMembers();

    }


    /* Tests_SRS_DOTNET_CORE_MODULES_MGR_04_005: [ AddModule shall acquire a lock on the ModulesManager object. ] */
    /* Tests_SRS_DOTNET_CORE_MODULES_MGR_04_007: [ AddModule shall add the module to it's internal collection of modules. ] */
    /* Tests_SRS_DOTNET_CORE_MODULES_MGR_04_008: [ AddModule shall release the lock on the ModulesManager object. ] */
    TEST_FUNCTION(ModulesManager_AddModule_success)
    {
        ///arrange
        CMODULESMANAGER_Mocks mocks;
        auto module_manager = ModulesManagerForUnitTest::Get();
        DOTNET_CORE_HOST_HANDLE_DATA handle_data_input((BROKER_HANDLE)0x42, "ModuleName");

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Lock((LOCK_HANDLE)0x42));
        STRICT_EXPECTED_CALL(mocks, STRING_clone(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, STRING_clone(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, STRING_clone(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, Unlock((LOCK_HANDLE)0x42));

        ///act
        auto module_handle = module_manager->AddModule(handle_data_input);
        

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NOT_NULL(module_handle);
        ASSERT_IS_TRUE(module_manager->HasModule(module_handle->module_id));

        ///cleanup
        module_manager->RemoveModule(module_handle->module_id);
        ModulesManagerForUnitTest::CleanUpMembers();

    }

    /* Tests_SRS_DOTNET_CORE_MODULES_MGR_04_006: [ AddModule shall return NULL if an underlying API call fails. ] */
    TEST_FUNCTION(ModulesManager_AddModule_Lock_Fail_AddModule_fail)
    {
        ///arrange
        CMODULESMANAGER_Mocks mocks;
        auto module_manager = ModulesManagerForUnitTest::Get();
        DOTNET_CORE_HOST_HANDLE_DATA handle_data_input((BROKER_HANDLE)0x42, "ModuleName");

        mocks.ResetAllCalls();


        STRICT_EXPECTED_CALL(mocks, Lock((LOCK_HANDLE)0x42))
            .SetReturn((LOCK_RESULT)LOCK_ERROR);
 
        DOTNET_CORE_HOST_HANDLE_DATA* module_handle = nullptr;
        LOCK_RESULT errorCode;
        ///act
        try
        {
            module_handle = module_manager->AddModule(handle_data_input);
        }
        catch (LOCK_RESULT err)
        {
            errorCode = err;
        }
       

        ///assert

        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(module_handle);

        ///cleanup
        ModulesManagerForUnitTest::CleanUpMembers();

    }

  
    /* Tests_SRS_DOTNET_CORE_MODULES_MGR_04_012: [ RemoveModule shall acquire a lock on the ModulesManager object. ] */
    /* Tests_SRS_DOTNET_CORE_MODULES_MGR_04_014: [ RemoveModule shall remove the module from it's internal collection of modules. ] */
    /* Tests_SRS_DOTNET_CORE_MODULES_MGR_04_015: [ RemoveModule shall release the lock on the ModulesManager object. ] */
    TEST_FUNCTION(ModulesManager_RemoveModule_success)
    {
        ///arrange
        CMODULESMANAGER_Mocks mocks;
        auto module_manager = ModulesManagerForUnitTest::Get();
        DOTNET_CORE_HOST_HANDLE_DATA handle_data_input((BROKER_HANDLE)0x42, "ModuleName");

        auto module_handle = module_manager->AddModule(handle_data_input);


        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Lock((LOCK_HANDLE)0x42));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, Unlock((LOCK_HANDLE)0x42));

        ///act
        module_manager->RemoveModule(module_handle->module_id);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_FALSE(module_manager->HasModule(module_handle->module_id));

        ///cleanup
        ModulesManagerForUnitTest::CleanUpMembers();

    }

    /* Tests_SRS_DOTNET_CORE_MODULES_MGR_04_013: [ RemoveModule shall do nothing if an underlying API call fails. ] */
    TEST_FUNCTION(ModulesManager_RemoveModule_LockFail_Remove_does_nothing)
    {
        ///arrange
        CMODULESMANAGER_Mocks mocks;
        auto module_manager = ModulesManagerForUnitTest::Get();
        DOTNET_CORE_HOST_HANDLE_DATA handle_data_input((BROKER_HANDLE)0x42, "ModuleName");

        auto module_handle = module_manager->AddModule(handle_data_input);


        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Lock((LOCK_HANDLE)0x42))
            .SetReturn((LOCK_RESULT)LOCK_ERROR);

        ///act
        LOCK_RESULT errorCode;
        try
        {
            module_manager->RemoveModule(module_handle->module_id);
        }
        catch (LOCK_RESULT err)
        {
            errorCode = err;
        }

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(module_manager->HasModule(module_handle->module_id));

        ///cleanup
        ModulesManagerForUnitTest::CleanUpMembers();

    }

    /* Tests_SRS_DOTNET_CORE_MODULES_MGR_04_017: [ AcquireLock shall acquire a lock by calling AcquireLock on Lock object. ] */
    TEST_FUNCTION(ModulesManager_AquireLock_success)
    {
        ///arrange
        CMODULESMANAGER_Mocks mocks;
        auto module_manager = ModulesManagerForUnitTest::Get();

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Lock((LOCK_HANDLE)0x42));

        ///act
        module_manager->AcquireLock();

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        ModulesManagerForUnitTest::CleanUpMembers();

    }

    /* Tests_SRS_DOTNET_CORE_MODULES_MGR_04_018: [ ReleaseLock shall release the lock by calling ReleaseLock on Lock object. ] */
    TEST_FUNCTION(ModulesManager_ReleaseLock_success)
    {
        ///arrange
        CMODULESMANAGER_Mocks mocks;
        auto module_manager = ModulesManagerForUnitTest::Get();

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Unlock((LOCK_HANDLE)0x42));

        ///act
        module_manager->ReleaseLock();

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        ModulesManagerForUnitTest::CleanUpMembers();

    }

    /* Tests_SRS_DOTNET_CORE_MODULES_MGR_04_019: [ HasModule shall make sure it's thread safe and call lock_guard. ] */
    /* Tests_SRS_DOTNET_CORE_MODULES_MGR_04_020: [ HasModule shall call find passing module id and return result. ] */
    TEST_FUNCTION(ModulesManager_HasModule_success)
    {
        ///arrange
        CMODULESMANAGER_Mocks mocks;
        auto module_manager = ModulesManagerForUnitTest::Get();
        DOTNET_CORE_HOST_HANDLE_DATA handle_data_input((BROKER_HANDLE)0x42, "ModuleName");

        auto module_handle = module_manager->AddModule(handle_data_input);


        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Lock((LOCK_HANDLE)0x42));
        STRICT_EXPECTED_CALL(mocks, Unlock((LOCK_HANDLE)0x42));
        ///act
        bool hasModuleResult = module_manager->HasModule(module_handle->module_id);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(hasModuleResult);

        ///cleanup
        ModulesManagerForUnitTest::CleanUpMembers();

    }

    /* Tests_SRS_DOTNET_CORE_MODULES_MGR_04_021: [ GetModuleFromId shall make sure it's thread safe and call lock_guard. ] */
    /* Tests_SRS_DOTNET_CORE_MODULES_MGR_04_022: [ GetModuleFromId shall return the instance based on module_id. ] */
    TEST_FUNCTION(ModulesManager_GetModuleFromId_success)
    {
        ///arrange
        CMODULESMANAGER_Mocks mocks;
        auto module_manager = ModulesManagerForUnitTest::Get();
        DOTNET_CORE_HOST_HANDLE_DATA handle_data_input((BROKER_HANDLE)0x42, "ModuleName");

        auto module_handle = module_manager->AddModule(handle_data_input);


        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, Lock((LOCK_HANDLE)0x42));
        STRICT_EXPECTED_CALL(mocks, STRING_clone(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, Unlock((LOCK_HANDLE)0x42));

        ///act
        DOTNET_CORE_HOST_HANDLE_DATA module_handle_from_Id = module_manager->GetModuleFromId(module_handle->module_id);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_ARE_EQUAL(int, module_handle->module_id, module_handle_from_Id.module_id);

        ///cleanup
        ModulesManagerForUnitTest::CleanUpMembers();

    }


END_TEST_SUITE(modules_manager_ut)
