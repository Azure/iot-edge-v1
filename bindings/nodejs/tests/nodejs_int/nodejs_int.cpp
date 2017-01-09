// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <string>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <memory>

#include "uv.h"
#include "v8.h"
#include "node.h"

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"

#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/map.h"
#include "azure_c_shared_utility/strings.h"
#include "module.h"
#include "module_access.h"
#include "message.h"
#include "broker.h"

#include "nodejs.h"
#include "nodejs_common.h"
#include "nodejs_utils.h"
#include "nodejs_idle.h"
#include "modules_manager.h"

static MICROMOCK_MUTEX_HANDLE g_testByTest;
static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

static pfModule_ParseConfigurationFromJson NODEJS_ParseConfigurationFromJson = NULL;
static pfModule_FreeConfiguration          NODEJS_FreeConfiguration = NULL;
static pfModule_Create                     NODEJS_Create = NULL;
static pfModule_Destroy                    NODEJS_Destroy = NULL;
static pfModule_Receive                    NODEJS_Receive = NULL;
static pfModule_Start                      NODEJS_Start = NULL;

using namespace nodejs_module;

class MockModule
{
private:
    BROKER_HANDLE m_broker;
    bool m_received_message;
    nodejs_module::Lock m_lock;

public:
    MockModule() :
        m_broker(nullptr),
        m_received_message(false)
    {}

    bool get_received_message()
    {
        LockGuard<MockModule> lock_guard(*this);
        return m_received_message;
    }

    void reset()
    {
        LockGuard<MockModule> lock_guard(*this);
        m_received_message = false;
    }

    void create(BROKER_HANDLE broker, const void* configuration)
    {
        LockGuard<MockModule> lock_guard(*this);
        m_broker = broker;
    }

    void receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
    {
        LockGuard<MockModule> lock_guard(*this);
        m_received_message = true;
    }

    void destroy(MODULE_HANDLE moduleHandle)
    {
    }

    void AcquireLock() const
    {
        m_lock.AcquireLock();
    }

    void ReleaseLock() const
    {
        m_lock.ReleaseLock();
    }

    void publish_mock_message()
    {
        LockGuard<MockModule> lock_guard(*this);

        MAP_HANDLE message_properties = Map_Create(NULL);
        Map_Add(message_properties, "p1", "v1");
        Map_Add(message_properties, "p2", "v2");

        MESSAGE_CONFIG config;
        unsigned char buffer[] = { 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };
        config.sourceProperties = message_properties;
        config.size = sizeof(buffer) / sizeof(unsigned char);
        config.source = buffer;

        MESSAGE_HANDLE message = Message_Create(&config);
        Broker_Publish(m_broker, reinterpret_cast<MODULE_HANDLE>(this), message);
        Message_Destroy(message);
        Map_Destroy(message_properties);
    }
};

MockModule g_mock_module;
MODULE_HANDLE g_mock_module_handle = nullptr;
BROKER_HANDLE g_broker = nullptr;

static void* MockModule_ParseConfigurationFromJson(const char* configuration)
{
    return NULL;
}

static void MockModule_FreeConfiguration(void* configuration)
{
}

static MODULE_HANDLE MockModule_Create(BROKER_HANDLE broker, const void* configuration)
{
    g_mock_module.create(broker, configuration);
    return reinterpret_cast<MODULE_HANDLE>(&g_mock_module);
}

static void MockModule_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    g_mock_module.receive(moduleHandle, messageHandle);
}

static void MockModule_Destroy(MODULE_HANDLE moduleHandle)
{
    g_mock_module.destroy(moduleHandle);
}

MODULE_API_1 g_fake_module_apis =
{
    {MODULE_API_VERSION_1},

    MockModule_ParseConfigurationFromJson,
    MockModule_FreeConfiguration,
    MockModule_Create,
    MockModule_Destroy,
    MockModule_Receive,
    NULL
};

MODULE g_module =
{
    NULL,
    NULL
};

class TempFile
{
public:
    std::string file_path;
    std::string js_file_path;

    enum
    {
        COULD_NOT_CREATE_FILE
    };

public:
    TempFile()
    {
        // Use of 'tmpnam' causes the GCC linker (yep, linker - NOT the compiler)
        // to complain like so:
        //      warning: the use of `tmpnam' is dangerous, better use `mkstemp'
        // Couldn't find a way of disabling the warning as I believe the use of this
        // function here to be acceptable in this context. In case of a rare race
        // condition that can happen it's OK for the test to fail. It will pass in a
        // subsequent run.
        auto temp_path = std::tmpnam(nullptr);
        if (temp_path == nullptr)
        {
            throw COULD_NOT_CREATE_FILE;
        }

        file_path = temp_path;
        js_file_path = ReplaceAll(file_path, "\\", "\\\\");
    }

    void Write(std::string contents)
    {
        std::ofstream stream(file_path);
        stream << contents;
    }

    std::string ReplaceAll(std::string str, std::string find_str, std::string replace_str)
    {
        size_t prev_pos = 0;
        auto it = str.find(find_str, prev_pos);
        while (it != std::string::npos)
        {
            str = str.replace(it, find_str.size(), replace_str);
            prev_pos = it + replace_str.size();
            it = str.find(find_str, prev_pos);
        }

        return str;
    }

    ~TempFile()
    {
        std::remove(file_path.c_str());
    }
};

template <typename TCallback>
void wait_for_predicate(uint32_t timeout_in_secs, TCallback pred)
{
    time_t start_time = get_time(nullptr);
    while(
            pred() == false
            &&
            (get_time(nullptr) - start_time) < timeout_in_secs
         )
    {
        ThreadAPI_Sleep(500);
    }
}

class NotifyResult
{
private:
    bool m_notify_result_called;
    bool m_notify_result;
    nodejs_module::Lock m_lock;

public:
    NotifyResult() :
        m_notify_result(false),
        m_notify_result_called(false)
    {}

    void AcquireLock() const
    {
        m_lock.AcquireLock();
    }

    void ReleaseLock() const
    {
        m_lock.ReleaseLock();
    }

    bool WasCalled()
    {
        LockGuard<NotifyResult> lock_guard(*this);
        return m_notify_result_called;
    }

    bool GetResult()
    {
        LockGuard<NotifyResult> lock_guard(*this);
        return m_notify_result;
    }

    void SetCalled(bool called)
    {
        LockGuard<NotifyResult> lock_guard(*this);
        m_notify_result_called = called;
    }

    void SetResult(bool result)
    {
        LockGuard<NotifyResult> lock_guard(*this);
        m_notify_result = result;
    }

    void Reset()
    {
        LockGuard<NotifyResult> lock_guard(*this);
        m_notify_result_called = false;
        m_notify_result = false;
    }
};

NotifyResult g_notify_result;

static void notify_result(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    g_notify_result.SetCalled(true);
    g_notify_result.SetResult(info[0]->ToBoolean()->BooleanValue());
}

static void publish_mock_message(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    g_mock_module.publish_mock_message();
}

static const char * NOOP_JS_MODULE = "module.exports = { "   \
"create: function(broker, configuration) { "       \
"  console.log('create'); "                            \
"  return true; "                                      \
"}, "                                                  \
"receive: function(msg) { "                            \
"  console.log('receive'); "                           \
"}, "                                                  \
"destroy: function() { "                               \
"  console.log('destroy'); "                           \
"} "                                                   \
"};";

BEGIN_TEST_SUITE(nodejs_int)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
        g_testByTest = MicroMockCreateMutex();
        ASSERT_IS_NOT_NULL(g_testByTest);

        const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);
        NODEJS_ParseConfigurationFromJson = MODULE_PARSE_CONFIGURATION_FROM_JSON(apis);
        NODEJS_FreeConfiguration = MODULE_FREE_CONFIGURATION(apis);
        NODEJS_Create = MODULE_CREATE(apis);
        NODEJS_Destroy = MODULE_DESTROY(apis);
        NODEJS_Receive = MODULE_RECEIVE(apis);
        NODEJS_Start = MODULE_START(apis);

        g_broker = Broker_Create();

        g_mock_module_handle = MockModule_Create(g_broker, nullptr);

        g_module.module_handle = g_mock_module_handle;
        g_module.module_apis = reinterpret_cast< const MODULE_API *>(&g_fake_module_apis);
        Broker_AddModule(g_broker, &g_module);
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        MicroMockDestroyMutex(g_testByTest);
        TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);

        // garbage collect v8
        NodeJSIdle::Get()->AddCallback([]() {
            NodeJSUtils::RunWithNodeContext([](v8::Isolate* isolate, v8::Local<v8::Context> context) {
                while (isolate->IdleNotification(1000) == false);

                // exit node thread cleanly
                ModulesManager::Get()->ExitNodeThread();
            });
        });

        ModulesManager::Get()->JoinNodeThread();

        Broker_RemoveModule(g_broker, &g_module);
        MockModule_Destroy(g_mock_module_handle);
        Broker_Destroy(g_broker);
    }

    TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
    {
        if (!MicroMockAcquireMutex(g_testByTest))
        {
            ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
        }

        // reset our fake module
        g_mock_module.reset();
        g_notify_result.Reset();
    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
        if (!MicroMockReleaseMutex(g_testByTest))
        {
            ASSERT_FAIL("failure in test framework at ReleaseMutex");
        }
    }

    /* Tests_SRS_NODEJS_26_001: [ `Module_GetApi` shall fill out the provided `MODULES_API` structure with required module's APIs functions. ] */
    TEST_FUNCTION(Module_GetApi_returns_non_NULL_and_non_NULL_fields)
    {
        ///arrange

        ///act
        const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

        ///assert
        ASSERT_IS_TRUE(MODULE_PARSE_CONFIGURATION_FROM_JSON(apis) != NULL);
        ASSERT_IS_TRUE(MODULE_FREE_CONFIGURATION(apis) != NULL);
        ASSERT_IS_TRUE(MODULE_CREATE(apis) != NULL);
        ASSERT_IS_TRUE(MODULE_START(apis) != NULL);
        ASSERT_IS_TRUE(MODULE_DESTROY(apis) != NULL);
        ASSERT_IS_TRUE(MODULE_RECEIVE(apis) != NULL);
    }

    TEST_FUNCTION(NODEJS_ParseConfigurationFromJson_returns_configuration)
    {
        ///arrange

        ///act
        auto result = NODEJS_ParseConfigurationFromJson("{}");

        ///assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_IS_TRUE(strcmp(STRING_c_str((STRING_HANDLE)result), "{}") == 0);

        ///cleanup
        NODEJS_FreeConfiguration(result);
    }

    TEST_FUNCTION(NODEJS_FreeConfiguration_frees_resources)
    {
        ///arrange
        auto result = NODEJS_ParseConfigurationFromJson("{}");

        // This test is only useful when we run things under a leak
        // detector.

        ///act
        NODEJS_FreeConfiguration(result);

        ///assert
    }

    TEST_FUNCTION(NODEJS_Create_returns_NULL_for_NULL_broker_handle)
    {
        ///arrange

        ///act
        auto result = NODEJS_Create(NULL, (const void*)0x42);

        ///assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(NODEJS_Create_returns_NULL_for_NULL_config)
    {
        ///arrange

        ///act
        auto result = NODEJS_Create((BROKER_HANDLE)0x42, NULL);

        ///assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(NODEJS_Create_returns_NULL_for_NULL_main_path)
    {
        ///arrange
        NODEJS_MODULE_CONFIG config = { 0 };

        ///act
        auto result = NODEJS_Create((BROKER_HANDLE)0x42, &config);

        ///assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(NODEJS_Create_returns_NULL_for_invalid_config_json)
    {
        ///arrange
        NODEJS_MODULE_CONFIG config = {
            STRING_construct("/path/to/mod.js"),
            STRING_construct("not_json")
        };

        ///act
        auto result = NODEJS_Create((BROKER_HANDLE)0x42, &config);

        ///assert
        ASSERT_IS_NULL(result);

        //cleanup
        STRING_delete(config.configuration_json);
        STRING_delete(config.main_path);
    }

    TEST_FUNCTION(NODEJS_Create_returns_NULL_for_invalid_main_file_path)
    {
        ///arrange
        NODEJS_MODULE_CONFIG config = {
            STRING_construct("/path/to/mod.js"),
            STRING_construct("{}")
        };

        ///act
        auto result = NODEJS_Create((BROKER_HANDLE)0x42, &config);

        ///assert
        ASSERT_IS_NULL(result);

        //cleanup
        STRING_delete(config.configuration_json);
        STRING_delete(config.main_path);
    }

    TEST_FUNCTION(NODEJS_Create_returns_handle_for_NULL_config_json)
    {
        ///arrange

        TempFile js_file;
        js_file.Write(NOOP_JS_MODULE);

        NODEJS_MODULE_CONFIG config = {
            STRING_construct(js_file.js_file_path.c_str()),
            NULL
        };

        ///act
        auto result = NODEJS_Create(g_broker, &config);

        ///assert
        ASSERT_IS_NOT_NULL(result);

        // wait for 15 seconds for the create to get done
        NODEJS_MODULE_HANDLE_DATA* handle_data = reinterpret_cast<NODEJS_MODULE_HANDLE_DATA*>(result);
        wait_for_predicate(15, [handle_data]() {
            return handle_data->GetModuleState() == NodeModuleState::initialized;
        });
        ASSERT_IS_TRUE(handle_data->GetModuleState() == NodeModuleState::initialized);
        ASSERT_IS_TRUE(handle_data->module_object.IsEmpty() == false);

        ///cleanup
        NODEJS_Destroy(result);
        STRING_delete(config.configuration_json);
    }

    TEST_FUNCTION(NODEJS_Create_returns_handle_for_valid_main_file_path)
    {
        ///arrange

        TempFile js_file;
        js_file.Write(NOOP_JS_MODULE);

        NODEJS_MODULE_CONFIG config = {
            STRING_construct(js_file.js_file_path.c_str()),
            STRING_construct("{}")
        };

        ///act
        auto result = NODEJS_Create(g_broker, &config);

        ///assert
        ASSERT_IS_NOT_NULL(result);

        // wait for 15 seconds for the create to get done
        NODEJS_MODULE_HANDLE_DATA* handle_data = reinterpret_cast<NODEJS_MODULE_HANDLE_DATA*>(result);
        wait_for_predicate(15, [handle_data]() {
            return handle_data->GetModuleState() == NodeModuleState::initialized;
        });
        ASSERT_IS_TRUE(handle_data->GetModuleState() == NodeModuleState::initialized);
        ASSERT_IS_TRUE(handle_data->module_object.IsEmpty() == false);

        ///cleanup
        NODEJS_Destroy(result);
        STRING_delete(config.configuration_json);
        STRING_delete(config.main_path);
    }

    TEST_FUNCTION(NODEJS_Create_returns_handle_for_adding_same_nodejs_module_twice)
    {
        ///arrange

        TempFile js_file;
        js_file.Write(NOOP_JS_MODULE);

        NODEJS_MODULE_CONFIG config = {
            STRING_construct(js_file.js_file_path.c_str()),
            STRING_construct("{}")
        };

        ///act
        auto result1 = NODEJS_Create(g_broker, &config);
        auto result2 = NODEJS_Create(g_broker, &config);

        ///assert
        ASSERT_IS_NOT_NULL(result1);
        ASSERT_IS_NOT_NULL(result2);
        ASSERT_IS_TRUE(result1 != result2);

        // wait for 15 seconds for the create to get done
        NODEJS_MODULE_HANDLE_DATA* handle_data1 = reinterpret_cast<NODEJS_MODULE_HANDLE_DATA*>(result1);
        NODEJS_MODULE_HANDLE_DATA* handle_data2 = reinterpret_cast<NODEJS_MODULE_HANDLE_DATA*>(result2);
        wait_for_predicate(15, [handle_data1, handle_data2]() {
            return handle_data1->GetModuleState() == NodeModuleState::initialized &&
                   handle_data2->GetModuleState() == NodeModuleState::initialized;
        });
        ASSERT_IS_TRUE(handle_data1->GetModuleState() == NodeModuleState::initialized);
        ASSERT_IS_TRUE(handle_data1->module_object.IsEmpty() == false);
        ASSERT_IS_TRUE(handle_data2->GetModuleState() == NodeModuleState::initialized);
        ASSERT_IS_TRUE(handle_data2->module_object.IsEmpty() == false);

        ///cleanup
        NODEJS_Destroy(result1);
        NODEJS_Destroy(result2);
        STRING_delete(config.configuration_json);
        STRING_delete(config.main_path);
    }

    TEST_FUNCTION(nodejs_module_publishes_message)
    {
        ///arrange
        const char* PUBLISH_JS_MODULE = ""                                      \
            "'use strict';"                                                     \
            "module.exports = {"                                                \
            "    broker: null,"                                             \
            "    configuration: null,"                                          \
            "    create: function (broker, configuration) {"                \
            "        this.broker = broker;"                             \
            "        this.configuration = configuration;"                       \
            "        setTimeout(() => {"                                        \
            "            console.log('NodeJS module is publishing a message');" \
            "            this.broker.publish({"                             \
            "                properties: {"                                     \
            "                    'source': 'sensor'"                            \
            "                },"                                                \
            "                content: new Uint8Array(["                         \
            "                    Math.random() * 50,"                           \
            "                    Math.random() * 50"                            \
            "                ])"                                                \
            "            });"                                                   \
            "        }, 1000);"                                                 \
            "        return true;"                                              \
            "    },"                                                            \
            "    receive: function(message) {"                                  \
            "    },"                                                            \
            "    destroy: function() {"                                         \
            "        console.log('sensor.destroy');"                            \
            "    }"                                                             \
            "};";

        TempFile js_file;
        js_file.Write(PUBLISH_JS_MODULE);

        NODEJS_MODULE_CONFIG config = {
            STRING_construct(js_file.js_file_path.c_str()),
            STRING_construct("{}")
        };

        ///act
        auto result = NODEJS_Create(g_broker, &config);
        const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

        MODULE module = {
            apis,
            result
        };
        Broker_AddModule(g_broker, &module);
        BROKER_LINK_DATA broker_data =
        {
            result,
            g_module.module_handle
        };
        Broker_AddLink(g_broker, &broker_data);

        ///assert
        ASSERT_IS_NOT_NULL(result);

        // wait for 15 seconds for the receive method in
        // our fake module to be called
        wait_for_predicate(15, []() {
            return g_mock_module.get_received_message();
        });
        ASSERT_IS_TRUE(g_mock_module.get_received_message() == true);

        ///cleanup
        Broker_RemoveModule(g_broker, &module);
        NODEJS_Destroy(result);
        STRING_delete(config.configuration_json);
        STRING_delete(config.main_path);
    }

    TEST_FUNCTION(nodejs_module_create_throws)
    {
        ///arrange
        const char* MODULE_CREATE_THROWS = ""                    \
            "'use strict';"                                      \
            "module.exports = {"                                 \
            "    create: function (broker, configuration) {" \
            "        throw 'whoops';"                            \
            "    },"                                             \
            "    receive: function(message) {"                   \
            "    },"                                             \
            "    destroy: function () {"                         \
            "    }"                                              \
            "};";

        TempFile js_file;
        js_file.Write(MODULE_CREATE_THROWS);

        NODEJS_MODULE_CONFIG config = {
            STRING_construct(js_file.js_file_path.c_str()),
            STRING_construct("{}")
        };

        ///act
        auto result = NODEJS_Create(g_broker, &config);

        ///assert
        ASSERT_IS_NOT_NULL(result);

        // wait for 15 seconds for the create to get done
        NODEJS_MODULE_HANDLE_DATA* handle_data = reinterpret_cast<NODEJS_MODULE_HANDLE_DATA*>(result);
        wait_for_predicate(15, [handle_data]() {
            return handle_data->GetModuleState() != NodeModuleState::initializing;
        });
        ASSERT_IS_TRUE(handle_data->GetModuleState() == NodeModuleState::error);
        ASSERT_IS_TRUE(handle_data->module_object.IsEmpty() == false);

        ///cleanup
        NODEJS_Destroy(result);
        STRING_delete(config.configuration_json);
        STRING_delete(config.main_path);
    }

    TEST_FUNCTION(nodejs_module_create_returns_nothing)
    {
        ///arrange
        const char* MODULE_CREATE_RETURNS_NOTHING = ""           \
            "'use strict';"                                      \
            "module.exports = {"                                 \
            "    create: function (broker, configuration) {" \
            "    },"                                             \
            "    receive: function(message) {"                   \
            "    },"                                             \
            "    destroy: function () {"                         \
            "    }"                                              \
            "};";

        TempFile js_file;
        js_file.Write(MODULE_CREATE_RETURNS_NOTHING);

        NODEJS_MODULE_CONFIG config = {
            STRING_construct(js_file.js_file_path.c_str()),
            STRING_construct("{}")
        };

        ///act
        auto result = NODEJS_Create(g_broker, &config);

        ///assert
        ASSERT_IS_NOT_NULL(result);

        // wait for 15 seconds for the create to get done
        NODEJS_MODULE_HANDLE_DATA* handle_data = reinterpret_cast<NODEJS_MODULE_HANDLE_DATA*>(result);
        wait_for_predicate(15, [handle_data]() {
            return handle_data->GetModuleState() != NodeModuleState::initializing;
        });
        ASSERT_IS_TRUE(handle_data->GetModuleState() == NodeModuleState::error);
        ASSERT_IS_TRUE(handle_data->module_object.IsEmpty() == false);

        ///cleanup
        NODEJS_Destroy(result);
        STRING_delete(config.configuration_json);
        STRING_delete(config.main_path);
    }

    TEST_FUNCTION(nodejs_module_create_does_not_return_bool)
    {
        ///arrange
        const char* MODULE_CREATE_RETURNS_NOTHING = ""           \
            "'use strict';"                                      \
            "module.exports = {"                                 \
            "    create: function (broker, configuration) {" \
            "        return {};"                                 \
            "    },"                                             \
            "    receive: function(message) {"                   \
            "    },"                                             \
            "    destroy: function () {"                         \
            "    }"                                              \
            "};";

        TempFile js_file;
        js_file.Write(MODULE_CREATE_RETURNS_NOTHING);

        NODEJS_MODULE_CONFIG config = {
            STRING_construct(js_file.js_file_path.c_str()),
            STRING_construct("{}")
        };

        ///act
        auto result = NODEJS_Create(g_broker, &config);

        ///assert
        ASSERT_IS_NOT_NULL(result);

        // wait for 15 seconds for the create to get done
        NODEJS_MODULE_HANDLE_DATA* handle_data = reinterpret_cast<NODEJS_MODULE_HANDLE_DATA*>(result);
        wait_for_predicate(15, [handle_data]() {
            return handle_data->GetModuleState() != NodeModuleState::initializing;
        });
        ASSERT_IS_TRUE(handle_data->GetModuleState() == NodeModuleState::error);
        ASSERT_IS_TRUE(handle_data->module_object.IsEmpty() == false);

        ///cleanup
        NODEJS_Destroy(result);
        STRING_delete(config.configuration_json);
        STRING_delete(config.main_path);
    }

    TEST_FUNCTION(nodejs_module_create_returns_false)
    {
        ///arrange
        const char* MODULE_CREATE_RETURNS_NOTHING = ""           \
            "'use strict';"                                      \
            "module.exports = {"                                 \
            "    create: function (broker, configuration) {"     \
            "        return false;"                              \
            "    },"                                             \
            "    receive: function(message) {"                   \
            "    },"                                             \
            "    destroy: function () {"                         \
            "    }"                                              \
            "};";

        TempFile js_file;
        js_file.Write(MODULE_CREATE_RETURNS_NOTHING);

        NODEJS_MODULE_CONFIG config = {
            STRING_construct(js_file.js_file_path.c_str()),
            STRING_construct("{}")
        };

        ///act
        auto result = NODEJS_Create(g_broker, &config);

        ///assert
        ASSERT_IS_NOT_NULL(result);

        // wait for 15 seconds for the create to get done
        NODEJS_MODULE_HANDLE_DATA* handle_data = reinterpret_cast<NODEJS_MODULE_HANDLE_DATA*>(result);
        wait_for_predicate(15, [handle_data]() {
            return handle_data->GetModuleState() != NodeModuleState::initializing;
        });
        ASSERT_IS_TRUE(handle_data->GetModuleState() == NodeModuleState::error);
        ASSERT_IS_TRUE(handle_data->module_object.IsEmpty() == false);

        ///cleanup
        NODEJS_Destroy(result);
        STRING_delete(config.configuration_json);
        STRING_delete(config.main_path);
    }

    TEST_FUNCTION(nodejs_module_interface_invalid_fails)
    {
        ///arrange
        const char* MODULE_INTERFACE_INVALID = ""                \
            "'use strict';"                                      \
            "module.exports = {"                                 \
            "    foo: function (broker, configuration) {"        \
            "        throw 'whoops';"                            \
            "    },"                                             \
            "    bar: function(message) {"                       \
            "    },"                                             \
            "    baz: function () {"                             \
            "    }"                                              \
            "};";

        TempFile js_file;
        js_file.Write(MODULE_INTERFACE_INVALID);

        NODEJS_MODULE_CONFIG config = {
            STRING_construct(js_file.js_file_path.c_str()),
            STRING_construct("{}")
        };

        ///act
        auto result = NODEJS_Create(g_broker, &config);

        ///assert
        ASSERT_IS_NOT_NULL(result);

        // wait for 15 seconds for the create to get done
        NODEJS_MODULE_HANDLE_DATA* handle_data = reinterpret_cast<NODEJS_MODULE_HANDLE_DATA*>(result);
        wait_for_predicate(15, [handle_data]() {
            return handle_data->GetModuleState() != NodeModuleState::initializing;
        });
        ASSERT_IS_TRUE(handle_data->GetModuleState() == NodeModuleState::error);

        ///cleanup
        NODEJS_Destroy(result);
        STRING_delete(config.configuration_json);
        STRING_delete(config.main_path);
    }

    TEST_FUNCTION(nodejs_invalid_publish_fails)
    {
        ///arrange
        const char* MODULE_INVALID_PUBLISH = ""                                 \
            "'use strict';"                                                     \
            "module.exports = {"                                                \
            "    broker: null,"                                                 \
            "    configuration: null,"                                          \
            "    create: function (broker, configuration) {"                    \
            "        this.broker = broker;"                                     \
            "        this.configuration = configuration;"                       \
            "        setTimeout(() => {"                                        \
            "            let res = this.broker.publish({});"                    \
            "            _integrationTest1.notify(res);"                        \
            "        }, 1000);"                                                 \
            "        return true;"                                              \
            "    },"                                                            \
            "    receive: function(message) {"                                  \
            "    },"                                                            \
            "    destroy: function() {"                                         \
            "        console.log('sensor.destroy');"                            \
            "    }"                                                             \
            "};";

        TempFile js_file;
        js_file.Write(MODULE_INVALID_PUBLISH);

        NODEJS_MODULE_CONFIG config = {
            STRING_construct(js_file.js_file_path.c_str()),
            STRING_construct("{}")
        };

        // setup a function to be called from the JS test code
        NodeJSIdle::Get()->AddCallback([]() {
            auto notify_result_obj = NodeJSUtils::CreateObjectWithMethod(
                "notify", notify_result
            );
            NodeJSUtils::AddObjectToGlobalContext("_integrationTest1", notify_result_obj);
        });

        ///act
        auto result = NODEJS_Create(g_broker, &config);

        ///assert
        ASSERT_IS_NOT_NULL(result);

        // wait for 15 seconds for the publish to happen
        NODEJS_MODULE_HANDLE_DATA* handle_data = reinterpret_cast<NODEJS_MODULE_HANDLE_DATA*>(result);
        wait_for_predicate(15, [handle_data]() {
            return g_notify_result.WasCalled() == true;
        });
        ASSERT_IS_TRUE(g_notify_result.WasCalled() == true);
        ASSERT_IS_TRUE(g_notify_result.GetResult() == false);

        ///cleanup
        NODEJS_Destroy(result);
        STRING_delete(config.configuration_json);
        STRING_delete(config.main_path);
    }

    TEST_FUNCTION(nodejs_invalid_publish_fails2)
    {
        ///arrange
        const char* MODULE_INVALID_PUBLISH = ""                                 \
            "'use strict';"                                                     \
            "module.exports = {"                                                \
            "    broker: null,"                                                 \
            "    configuration: null,"                                          \
            "    create: function (broker, configuration) {"                    \
            "        this.broker = broker;"                                     \
            "        this.configuration = configuration;"                       \
            "        setTimeout(() => {"                                        \
            "            let res = this.broker.publish({"                       \
            "                properties: 'boo',"                                \
            "                content: new Uint8Array(["                         \
            "                    Math.random() * 50,"                           \
            "                    Math.random() * 50"                            \
            "                ])"                                                \
            "            });"                                                   \
            "            _integrationTest2.notify(res);"                        \
            "        }, 1000);"                                                 \
            "        return true;"                                              \
            "    },"                                                            \
            "    receive: function(message) {"                                  \
            "    },"                                                            \
            "    destroy: function() {"                                         \
            "        console.log('sensor.destroy');"                            \
            "    }"                                                             \
            "};";

        TempFile js_file;
        js_file.Write(MODULE_INVALID_PUBLISH);

        NODEJS_MODULE_CONFIG config = {
            STRING_construct(js_file.js_file_path.c_str()),
            STRING_construct("{}")
        };

        // setup a function to be called from the JS test code
        NodeJSIdle::Get()->AddCallback([]() {
            auto notify_result_obj = NodeJSUtils::CreateObjectWithMethod(
                "notify", notify_result
            );
            NodeJSUtils::AddObjectToGlobalContext("_integrationTest2", notify_result_obj);
        });

        ///act
        auto result = NODEJS_Create(g_broker, &config);

        ///assert
        ASSERT_IS_NOT_NULL(result);

        // wait for 15 seconds for the publish to happen
        NODEJS_MODULE_HANDLE_DATA* handle_data = reinterpret_cast<NODEJS_MODULE_HANDLE_DATA*>(result);
        wait_for_predicate(15, [handle_data]() {
            return g_notify_result.WasCalled() == true;
        });
        ASSERT_IS_TRUE(g_notify_result.WasCalled() == true);
        ASSERT_IS_TRUE(g_notify_result.GetResult() == false);

        ///cleanup
        NODEJS_Destroy(result);
        STRING_delete(config.configuration_json);
        STRING_delete(config.main_path);
    }

    TEST_FUNCTION(nodejs_invalid_publish_fails3)
    {
        ///arrange
        const char* MODULE_INVALID_PUBLISH = ""                                 \
            "'use strict';"                                                     \
            "module.exports = {"                                                \
            "    broker: null,"                                                 \
            "    configuration: null,"                                          \
            "    create: function (broker, configuration) {"                    \
            "        this.broker = broker;"                                     \
            "        this.configuration = configuration;"                       \
            "        setTimeout(() => {"                                        \
            "            let res = this.broker.publish({"                       \
            "                properties: {},"                                   \
            "                content: ''"                                       \
            "            });"                                                   \
            "            _integrationTest3.notify(res);"                        \
            "        }, 1000);"                                                 \
            "        return true;"                                              \
            "    },"                                                            \
            "    receive: function(message) {"                                  \
            "    },"                                                            \
            "    destroy: function() {"                                         \
            "        console.log('sensor.destroy');"                            \
            "    }"                                                             \
            "};";

        TempFile js_file;
        js_file.Write(MODULE_INVALID_PUBLISH);

        NODEJS_MODULE_CONFIG config = {
            STRING_construct(js_file.js_file_path.c_str()),
            STRING_construct("{}")
        };

        // setup a function to be called from the JS test code
        NodeJSIdle::Get()->AddCallback([]() {
            auto notify_result_obj = NodeJSUtils::CreateObjectWithMethod(
                "notify", notify_result
            );
            NodeJSUtils::AddObjectToGlobalContext("_integrationTest3", notify_result_obj);
        });

        ///act
        auto result = NODEJS_Create(g_broker, &config);

        ///assert
        ASSERT_IS_NOT_NULL(result);

        // wait for 15 seconds for the publish to happen
        NODEJS_MODULE_HANDLE_DATA* handle_data = reinterpret_cast<NODEJS_MODULE_HANDLE_DATA*>(result);
        wait_for_predicate(15, [handle_data]() {
            return g_notify_result.WasCalled() == true;
        });
        ASSERT_IS_TRUE(g_notify_result.WasCalled() == true);
        ASSERT_IS_TRUE(g_notify_result.GetResult() == false);

        ///cleanup
        NODEJS_Destroy(result);
        STRING_delete(config.configuration_json);
        STRING_delete(config.main_path);
    }

    TEST_FUNCTION(nodejs_receive_is_called)
    {
        ///arrange
        const char* MODULE_RECEIVE_IS_CALLED = ""                   \
            "'use strict';"                                         \
            "module.exports = {"                                    \
            "    broker: null,"                                     \
            "    configuration: null,"                              \
            "    create: function (broker, configuration) {"        \
            "        this.broker = broker;"                         \
            "        this.configuration = configuration;"           \
            "        setTimeout(() => {"                            \
            "            _mock_module1.publish_mock_message();"     \
            "        }, 10);"                                       \
            "        return true;"                                  \
            "    },"                                                \
            "    receive: function(message) {"                      \
            "        let res = !!message"                           \
            "                  &&"                                  \
            "                  !!(message.properties)"              \
            "                  &&"                                  \
            "                  !!(message.properties['p1'])"        \
            "                  &&"                                  \
            "                  (message.properties['p1'] === 'v1')" \
            "                  &&"                                  \
            "                  !!(message.properties['p2'])"        \
            "                  &&"                                  \
            "                  (message.properties['p2'] === 'v2')" \
            "                  &&"                                  \
            "                  !!(message.content)"                 \
            "                  &&"                                  \
            "                  (message.content.length == 6);"      \
            "        _integrationTest4.notify(res);"                \
            "    },"                                                \
            "    destroy: function() {"                             \
            "    }"                                                 \
            "};";

        TempFile js_file;
        js_file.Write(MODULE_RECEIVE_IS_CALLED);

        NODEJS_MODULE_CONFIG config = {
            STRING_construct(js_file.js_file_path.c_str()),
            STRING_construct("{}")
        };

        // setup a function to be called from the JS test code
        NodeJSIdle::Get()->AddCallback([]() {
            auto notify_result_obj = NodeJSUtils::CreateObjectWithMethod(
                "notify", notify_result
            );
            NodeJSUtils::AddObjectToGlobalContext("_integrationTest4", notify_result_obj);

            auto publish_mock_msg_obj = NodeJSUtils::CreateObjectWithMethod(
                "publish_mock_message", publish_mock_message
            );
            NodeJSUtils::AddObjectToGlobalContext("_mock_module1", publish_mock_msg_obj);
        });

        ///act
        auto result = NODEJS_Create(g_broker, &config);
        const MODULE_API* apis = Module_GetApi(MODULE_API_VERSION_1);

        MODULE module = {
            apis,
            result
        };
        Broker_AddModule(g_broker, &module);
        BROKER_LINK_DATA broker_data =
        {
            g_module.module_handle,
            result
        };
        Broker_AddLink(g_broker, &broker_data);

        ///assert
        ASSERT_IS_NOT_NULL(result);

        // wait for 15 seconds for the publish to happen
        NODEJS_MODULE_HANDLE_DATA* handle_data = reinterpret_cast<NODEJS_MODULE_HANDLE_DATA*>(result);
        wait_for_predicate(15, [handle_data]() {
            return g_notify_result.WasCalled() == true;
        });
        ASSERT_IS_TRUE(g_notify_result.WasCalled() == true);
        ASSERT_IS_TRUE(g_notify_result.GetResult() == true);

        ///cleanup
        Broker_RemoveModule(g_broker, &module);
        NODEJS_Destroy(result);
        STRING_delete(config.configuration_json);
        STRING_delete(config.main_path);
    }

    TEST_FUNCTION(nodejs_destroy_is_called)
    {
        ///arrange
        const char* MODULE_DESTROY_IS_CALLED = ""                \
            "'use strict';"                                      \
            "module.exports = {"                                 \
            "    broker: null,"                                  \
            "    configuration: null,"                           \
            "    create: function (broker, configuration) {"     \
            "        this.broker = broker;"                      \
            "        this.configuration = configuration;"        \
            "        return true;"                               \
            "    },"                                             \
            "    receive: function(message) {"                   \
            "    },"                                             \
            "    destroy: function () {"                         \
            "        _integrationTest5.notify(true);"            \
            "    }"                                              \
            "};";

        TempFile js_file;
        js_file.Write(MODULE_DESTROY_IS_CALLED);

        NODEJS_MODULE_CONFIG config = {
            STRING_construct(js_file.js_file_path.c_str()),
            STRING_construct("{}")
        };

        // setup a function to be called from the JS test code
        NodeJSIdle::Get()->AddCallback([]() {
            auto notify_result_obj = NodeJSUtils::CreateObjectWithMethod(
                "notify", notify_result
            );
            NodeJSUtils::AddObjectToGlobalContext("_integrationTest5", notify_result_obj);
        });

        ///act
        auto result = NODEJS_Create(g_broker, &config);

        // wait for 15 seconds for the create to get done
        NODEJS_MODULE_HANDLE_DATA* handle_data = reinterpret_cast<NODEJS_MODULE_HANDLE_DATA*>(result);
        wait_for_predicate(15, [handle_data]() {
            return handle_data->GetModuleState() != NodeModuleState::initializing;
        });

        NODEJS_Destroy(result);

        ///assert
        wait_for_predicate(15, [handle_data]() {
            return g_notify_result.WasCalled() == true;
        });
        ASSERT_IS_TRUE(g_notify_result.WasCalled() == true);
        ASSERT_IS_TRUE(g_notify_result.GetResult() == true);

        ///cleanup
        STRING_delete(config.configuration_json);
        STRING_delete(config.main_path);
    }

    TEST_FUNCTION(call_Start_before_nodejs_init_completes)
    {
        ///arrange
        const char* MODULE_START_IS_CALLED = ""                  \
            "'use strict';"                                      \
            "module.exports = {"                                 \
            "    create: function (broker, configuration) {"     \
            "        return true;"                               \
            "    },"                                             \
            "    start: function() {"                            \
            "        _integrationTest6.notify(true);"            \
            "    },"                                             \
            "    receive: function(message) {"                   \
            "    },"                                             \
            "    destroy: function () {"                         \
            "    }"                                              \
            "};";

        TempFile js_file;
        js_file.Write(MODULE_START_IS_CALLED);

        NODEJS_MODULE_CONFIG config = {
            STRING_construct(js_file.js_file_path.c_str()),
            STRING_construct("{}")
        };

        // setup a function to be called from the JS test code
        NodeJSIdle::Get()->AddCallback([]() {
            auto notify_result_obj = NodeJSUtils::CreateObjectWithMethod(
                "notify", notify_result
            );
            NodeJSUtils::AddObjectToGlobalContext("_integrationTest6", notify_result_obj);
        });

        auto result = NODEJS_Create(g_broker, &config);

        ///act

        NODEJS_Start(result);

        // wait for 15 seconds for the create to get done
        NODEJS_MODULE_HANDLE_DATA* handle_data = reinterpret_cast<NODEJS_MODULE_HANDLE_DATA*>(result);
        wait_for_predicate(15, [handle_data]() {
            return handle_data->GetModuleState() != NodeModuleState::initializing;
        });

        ///assert
        wait_for_predicate(15, [handle_data]() {
            return g_notify_result.WasCalled() == true;
        });
        ASSERT_IS_TRUE(g_notify_result.WasCalled() == true);
        ASSERT_IS_TRUE(g_notify_result.GetResult() == true);

        ///cleanup
        NODEJS_Destroy(result);
        STRING_delete(config.configuration_json);
        STRING_delete(config.main_path);
    }

    TEST_FUNCTION(call_Start_after_nodejs_init_completes)
    {
        ///arrange
        const char* MODULE_START_IS_CALLED = ""                  \
            "'use strict';"                                      \
            "module.exports = {"                                 \
            "    create: function (broker, configuration) {"     \
            "        return true;"                               \
            "    },"                                             \
            "    start: function() {"                            \
            "        _integrationTest6.notify(true);"            \
            "    },"                                             \
            "    receive: function(message) {"                   \
            "    },"                                             \
            "    destroy: function () {"                         \
            "    }"                                              \
            "};";

        TempFile js_file;
        js_file.Write(MODULE_START_IS_CALLED);

        NODEJS_MODULE_CONFIG config = {
            STRING_construct(js_file.js_file_path.c_str()),
            STRING_construct("{}")
        };

        // setup a function to be called from the JS test code
        NodeJSIdle::Get()->AddCallback([]() {
            auto notify_result_obj = NodeJSUtils::CreateObjectWithMethod(
                "notify", notify_result
            );
            NodeJSUtils::AddObjectToGlobalContext("_integrationTest6", notify_result_obj);
        });

        auto result = NODEJS_Create(g_broker, &config);

        // wait for 15 seconds for the create to get done
        NODEJS_MODULE_HANDLE_DATA* handle_data = reinterpret_cast<NODEJS_MODULE_HANDLE_DATA*>(result);
        wait_for_predicate(15, [handle_data]() {
            return handle_data->GetModuleState() != NodeModuleState::initializing;
        });

        ///act

        NODEJS_Start(result);

        ///assert
        wait_for_predicate(15, [handle_data]() {
            return g_notify_result.WasCalled() == true;
        });
        ASSERT_IS_TRUE(g_notify_result.WasCalled() == true);
        ASSERT_IS_TRUE(g_notify_result.GetResult() == true);

        ///cleanup
        NODEJS_Destroy(result);
        STRING_delete(config.configuration_json);
        STRING_delete(config.main_path);
    }

END_TEST_SUITE(nodejs_int)
