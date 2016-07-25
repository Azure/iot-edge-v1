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
#include "module.h"
#include "message.h"
#include "message_bus.h"

#include "nodejs.h"
#include "nodejs_common.h"
#include "nodejs_utils.h"
#include "nodejs_idle.h"
#include "modules_manager.h"

static MICROMOCK_MUTEX_HANDLE g_testByTest;
static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

static pfModule_Create  NODEJS_Create = NULL;  /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Destroy NODEJS_Destroy = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Receive NODEJS_Receive = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/

using namespace nodejs_module;

class MockModule
{
private:
    MESSAGE_BUS_HANDLE m_bus;
    bool m_received_message;
    nodejs_module::Lock m_lock;

public:
    MockModule() :
        m_bus{ nullptr },
        m_received_message{ false }
    {}

    bool get_received_message()
    {
        LockGuard<MockModule> lock_guard{ *this };
        return m_received_message;
    }

    void reset()
    {
        LockGuard<MockModule> lock_guard{ *this };
        m_received_message = false;
    }

    void create(MESSAGE_BUS_HANDLE busHandle, const void* configuration)
    {
        LockGuard<MockModule> lock_guard{ *this };
        m_bus = busHandle;
    }

    void receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
    {
        LockGuard<MockModule> lock_guard{ *this };
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
        LockGuard<MockModule> lock_guard{ *this };

        MAP_HANDLE message_properties = Map_Create(NULL);
        Map_Add(message_properties, "p1", "v1");
        Map_Add(message_properties, "p2", "v2");

        MESSAGE_CONFIG config;
        unsigned char buffer[] = { 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };
        config.sourceProperties = message_properties;
        config.size = sizeof(buffer) / sizeof(unsigned char);
        config.source = buffer;

        MESSAGE_HANDLE message = Message_Create(&config);
        MessageBus_Publish(m_bus, reinterpret_cast<MODULE_HANDLE>(this), message);
        Message_Destroy(message);
        Map_Destroy(message_properties);
    }
};

MockModule g_mock_module;
MODULE_HANDLE g_mock_module_handle = nullptr;
MESSAGE_BUS_HANDLE g_message_bus = nullptr;

MODULE_HANDLE MockModule_Create(MESSAGE_BUS_HANDLE busHandle, const void* configuration)
{
    g_mock_module.create(busHandle, configuration);
    return reinterpret_cast<MODULE_HANDLE>(&g_mock_module);
}

void MockModule_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    g_mock_module.receive(moduleHandle, messageHandle);
}

void MockModule_Destroy(MODULE_HANDLE moduleHandle)
{
    g_mock_module.destroy(moduleHandle);
}

MODULE_APIS g_fake_module_apis = {
    MockModule_Create,
    MockModule_Destroy,
    MockModule_Receive
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
        std::ofstream stream{ file_path };
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
        m_notify_result{ false },
        m_notify_result_called{ false }
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
        LockGuard<NotifyResult> lock_guard{ *this };
        return m_notify_result_called;
    }

    bool GetResult()
    {
        LockGuard<NotifyResult> lock_guard{ *this };
        return m_notify_result;
    }

    void SetCalled(bool called)
    {
        LockGuard<NotifyResult> lock_guard{ *this };
        m_notify_result_called = called;
    }

    void SetResult(bool result)
    {
        LockGuard<NotifyResult> lock_guard{ *this };
        m_notify_result = result;
    }

    void Reset()
    {
        LockGuard<NotifyResult> lock_guard{ *this };
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

BEGIN_TEST_SUITE(nodejs_binding_unittests)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
        g_testByTest = MicroMockCreateMutex();
        ASSERT_IS_NOT_NULL(g_testByTest);

        NODEJS_Create = Module_GetAPIS()->Module_Create;
        NODEJS_Destroy = Module_GetAPIS()->Module_Destroy;
        NODEJS_Receive = Module_GetAPIS()->Module_Receive;

        g_message_bus = MessageBus_Create();

        g_mock_module_handle = MockModule_Create(g_message_bus, nullptr);
        
        g_module.module_handle = g_mock_module_handle;
        g_module.module_apis = &g_fake_module_apis;
        MessageBus_AddModule(g_message_bus, &g_module);
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

        MessageBus_RemoveModule(g_message_bus, &g_module);
        MockModule_Destroy(g_mock_module_handle);
        MessageBus_Destroy(g_message_bus);
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

    TEST_FUNCTION(Module_GetAPIS_returns_non_NULL_and_non_NULL_fields)
    {
        ///arrrange

        ///act
        auto result = Module_GetAPIS();

        ///assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_IS_NOT_NULL(result->Module_Create);
        ASSERT_IS_NOT_NULL(result->Module_Destroy);
        ASSERT_IS_NOT_NULL(result->Module_Receive);
    }

    TEST_FUNCTION(NODEJS_Create_returns_NULL_for_NULL_bus_handle)
    {
        ///arrrange

        ///act
        auto result = NODEJS_Create(NULL, (const void*)0x42);

        ///assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(NODEJS_Create_returns_NULL_for_NULL_config)
    {
        ///arrrange

        ///act
        auto result = NODEJS_Create((MESSAGE_BUS_HANDLE)0x42, NULL);

        ///assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(NODEJS_Create_returns_NULL_for_NULL_main_path)
    {
        ///arrrange
        NODEJS_MODULE_CONFIG config = { 0 };

        ///act
        auto result = NODEJS_Create((MESSAGE_BUS_HANDLE)0x42, &config);

        ///assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(NODEJS_Create_returns_NULL_for_invalid_config_json)
    {
        ///arrrange
        NODEJS_MODULE_CONFIG config = {
            "/path/to/mod.js",
            "not_json"
        };

        ///act
        auto result = NODEJS_Create((MESSAGE_BUS_HANDLE)0x42, &config);

        ///assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(NODEJS_Create_returns_NULL_for_invalid_main_file_path)
    {
        ///arrrange
        NODEJS_MODULE_CONFIG config = {
            "/path/to/mod.js",
            "{}"
        };

        ///act
        auto result = NODEJS_Create((MESSAGE_BUS_HANDLE)0x42, &config);

        ///assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(NODEJS_Create_returns_handle_for_valid_main_file_path)
    {
        ///arrrange
        const char* NOOP_JS_MODULE = "module.exports = { "   \
            "create: function(messageBus, configuration) { "       \
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

        TempFile js_file;
        js_file.Write(NOOP_JS_MODULE);

        NODEJS_MODULE_CONFIG config = {
            js_file.js_file_path.c_str(),
            "{}"
        };

        ///act
        auto result = NODEJS_Create(g_message_bus, &config);

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
    }

    TEST_FUNCTION(nodejs_module_publishes_message)
    {
        ///arrrange
        const char* PUBLISH_JS_MODULE = ""                                      \
            "'use strict';"                                                     \
            "module.exports = {"                                                \
            "    messageBus: null,"                                             \
            "    configuration: null,"                                          \
            "    create: function (messageBus, configuration) {"                \
            "        this.messageBus = messageBus;"                             \
            "        this.configuration = configuration;"                       \
            "        setTimeout(() => {"                                        \
            "            console.log('NodeJS module is publishing a message');" \
            "            this.messageBus.publish({"                             \
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
            js_file.js_file_path.c_str(),
            "{}"
        };

        ///act
        auto result = NODEJS_Create(g_message_bus, &config);
        MODULE module = {
            Module_GetAPIS(),
            result
        };
        MessageBus_AddModule(g_message_bus, &module);

        ///assert
        ASSERT_IS_NOT_NULL(result);

        // wait for 15 seconds for the receive method in
        // our fake module to be called
        wait_for_predicate(15, []() {
            return g_mock_module.get_received_message();
        });
        ASSERT_IS_TRUE(g_mock_module.get_received_message() == true);

        ///cleanup
        MessageBus_RemoveModule(g_message_bus, &module);
        NODEJS_Destroy(result);
    }

    TEST_FUNCTION(nodejs_module_create_throws)
    {
        ///arrrange
        const char* MODULE_CREATE_THROWS = ""                    \
            "'use strict';"                                      \
            "module.exports = {"                                 \
            "    create: function (messageBus, configuration) {" \
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
            js_file.js_file_path.c_str(),
            "{}"
        };

        ///act
        auto result = NODEJS_Create(g_message_bus, &config);

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
    }

    TEST_FUNCTION(nodejs_module_create_returns_nothing)
    {
        ///arrrange
        const char* MODULE_CREATE_RETURNS_NOTHING = ""           \
            "'use strict';"                                      \
            "module.exports = {"                                 \
            "    create: function (messageBus, configuration) {" \
            "    },"                                             \
            "    receive: function(message) {"                   \
            "    },"                                             \
            "    destroy: function () {"                         \
            "    }"                                              \
            "};";

        TempFile js_file;
        js_file.Write(MODULE_CREATE_RETURNS_NOTHING);

        NODEJS_MODULE_CONFIG config = {
            js_file.js_file_path.c_str(),
            "{}"
        };

        ///act
        auto result = NODEJS_Create(g_message_bus, &config);

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
    }

    TEST_FUNCTION(nodejs_module_create_does_not_return_bool)
    {
        ///arrrange
        const char* MODULE_CREATE_RETURNS_NOTHING = ""           \
            "'use strict';"                                      \
            "module.exports = {"                                 \
            "    create: function (messageBus, configuration) {" \
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
            js_file.js_file_path.c_str(),
            "{}"
        };

        ///act
        auto result = NODEJS_Create(g_message_bus, &config);

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
    }

    TEST_FUNCTION(nodejs_module_create_returns_false)
    {
        ///arrrange
        const char* MODULE_CREATE_RETURNS_NOTHING = ""           \
            "'use strict';"                                      \
            "module.exports = {"                                 \
            "    create: function (messageBus, configuration) {" \
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
            js_file.js_file_path.c_str(),
            "{}"
        };

        ///act
        auto result = NODEJS_Create(g_message_bus, &config);

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
    }

    TEST_FUNCTION(nodejs_module_interface_invalid_fails)
    {
        ///arrrange
        const char* MODULE_INTERFACE_INVALID = ""                \
            "'use strict';"                                      \
            "module.exports = {"                                 \
            "    foo: function (messageBus, configuration) {"    \
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
            js_file.js_file_path.c_str(),
            "{}"
        };

        ///act
        auto result = NODEJS_Create(g_message_bus, &config);

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
    }

    TEST_FUNCTION(nodejs_invalid_publish_fails)
    {
        ///arrrange
        const char* MODULE_INVALID_PUBLISH = ""                                 \
            "'use strict';"                                                     \
            "module.exports = {"                                                \
            "    messageBus: null,"                                             \
            "    configuration: null,"                                          \
            "    create: function (messageBus, configuration) {"                \
            "        this.messageBus = messageBus;"                             \
            "        this.configuration = configuration;"                       \
            "        setTimeout(() => {"                                        \
            "            let res = this.messageBus.publish({});"                \
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
            js_file.js_file_path.c_str(),
            "{}"
        };

        // setup a function to be called from the JS test code
        NodeJSIdle::Get()->AddCallback([]() {
            auto notify_result_obj = NodeJSUtils::CreateObjectWithMethod(
                "notify", notify_result
            );
            NodeJSUtils::AddObjectToGlobalContext("_integrationTest1", notify_result_obj);
        });

        ///act
        auto result = NODEJS_Create(g_message_bus, &config);

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
    }

    TEST_FUNCTION(nodejs_invalid_publish_fails2)
    {
        ///arrrange
        const char* MODULE_INVALID_PUBLISH = ""                                 \
            "'use strict';"                                                     \
            "module.exports = {"                                                \
            "    messageBus: null,"                                             \
            "    configuration: null,"                                          \
            "    create: function (messageBus, configuration) {"                \
            "        this.messageBus = messageBus;"                             \
            "        this.configuration = configuration;"                       \
            "        setTimeout(() => {"                                        \
            "            let res = this.messageBus.publish({"                   \
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
            js_file.js_file_path.c_str(),
            "{}"
        };

        // setup a function to be called from the JS test code
        NodeJSIdle::Get()->AddCallback([]() {
            auto notify_result_obj = NodeJSUtils::CreateObjectWithMethod(
                "notify", notify_result
            );
            NodeJSUtils::AddObjectToGlobalContext("_integrationTest2", notify_result_obj);
        });

        ///act
        auto result = NODEJS_Create(g_message_bus, &config);

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
    }

    TEST_FUNCTION(nodejs_invalid_publish_fails3)
    {
        ///arrrange
        const char* MODULE_INVALID_PUBLISH = ""                                 \
            "'use strict';"                                                     \
            "module.exports = {"                                                \
            "    messageBus: null,"                                             \
            "    configuration: null,"                                          \
            "    create: function (messageBus, configuration) {"                \
            "        this.messageBus = messageBus;"                             \
            "        this.configuration = configuration;"                       \
            "        setTimeout(() => {"                                        \
            "            let res = this.messageBus.publish({"                   \
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
            js_file.js_file_path.c_str(),
            "{}"
        };

        // setup a function to be called from the JS test code
        NodeJSIdle::Get()->AddCallback([]() {
            auto notify_result_obj = NodeJSUtils::CreateObjectWithMethod(
                "notify", notify_result
            );
            NodeJSUtils::AddObjectToGlobalContext("_integrationTest3", notify_result_obj);
        });

        ///act
        auto result = NODEJS_Create(g_message_bus, &config);

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
    }

    TEST_FUNCTION(nodejs_receive_is_called)
    {
        ///arrrange
        const char* MODULE_RECEIVE_IS_CALLED = ""                   \
            "'use strict';"                                         \
            "module.exports = {"                                    \
            "    messageBus: null,"                                 \
            "    configuration: null,"                              \
            "    create: function (messageBus, configuration) {"    \
            "        this.messageBus = messageBus;"                 \
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
            js_file.js_file_path.c_str(),
            "{}"
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
        auto result = NODEJS_Create(g_message_bus, &config);
        MODULE module = {
            Module_GetAPIS(),
            result
        };
        MessageBus_AddModule(g_message_bus, &module);

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
        MessageBus_RemoveModule(g_message_bus, &module);
        NODEJS_Destroy(result);
    }

    TEST_FUNCTION(nodejs_destroy_is_called)
    {
        ///arrrange
        const char* MODULE_DESTROY_IS_CALLED = ""                \
            "'use strict';"                                      \
            "module.exports = {"                                 \
            "    messageBus: null,"                              \
            "    configuration: null,"                           \
            "    create: function (messageBus, configuration) {" \
            "        this.messageBus = messageBus;"              \
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
            js_file.js_file_path.c_str(),
            "{}"
        };

        // setup a function to be called from the JS test code
        NodeJSIdle::Get()->AddCallback([]() {
            auto notify_result_obj = NodeJSUtils::CreateObjectWithMethod(
                "notify", notify_result
            );
            NodeJSUtils::AddObjectToGlobalContext("_integrationTest5", notify_result_obj);
        });

        ///act
        auto result = NODEJS_Create(g_message_bus, &config);

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
    }

END_TEST_SUITE(nodejs_binding_unittests)
