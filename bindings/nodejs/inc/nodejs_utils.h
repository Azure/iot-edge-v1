// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef NODEJS_UTILS_H
#define NODEJS_UTILS_H

#include <string>
#include <cstdint>
#include <functional>

#include "uv.h"
#include "v8.h"

#include "azure_c_shared_utility/iot_logging.h"

namespace nodejs_module
{
    template<typename T>
    using PersistentCopyable = v8::Persistent<T, v8::CopyablePersistentTraits<T>>;

    class NodeJSUtils
    {
    public:
        /**
         * Runs a piece of JavaScript code.
         */
        static v8::Local<v8::Value> RunScript(
            v8::Isolate* isolate,
            v8::Local<v8::Context> context,
            std::string script_text
        );

        /**
         * Invokes 'callback' with an active node context passing a v8::Isolate*
         * and a v8::Local<v8::Context> object. 'callback' must look like this:
         *      [](v8::Isolate*, v8::Local<v8::Context>){}
         */
        template<typename TCallback>
        static void RunWithNodeContext(TCallback callback);

        /**
         * Invokes 'callback' after 'timeout_in_ms' milliseconds via libuv.
         * 'callback' is a function that has no params and returns nothing.
         */
        template<typename TCallback>
        static bool SetTimeout(TCallback callback, uint64_t timeout_in_ms);

        /**
         * Invokes 'callback' during idle time processing via libuv.
         * 'callback' is a function that has no params and returns nothing.
         */
        template<typename TCallback>
        static bool SetIdle(TCallback callback);

        /**
         * Creates a v8 object template with a single method with the given method
         * name.
         */
        static PersistentCopyable<v8::ObjectTemplate> CreateObjectTemplateWithMethod(std::string method_name, v8::FunctionCallback callback);

        /**
         * Creates a v8 object with a single method with the given method name.
         */
        static PersistentCopyable<v8::Object> CreateObjectWithMethod(std::string method_name, v8::FunctionCallback callback);

        /**
         * Adds an object with the name 'prop_name' to the global v8 context.
         */
        static bool AddObjectToGlobalContext(std::string prop_name, PersistentCopyable<v8::Object>& obj);

    private:
        template <typename T>
        static void on_libuv_handler(T* handle);
    };
};

template<typename TCallback>
void nodejs_module::NodeJSUtils::RunWithNodeContext(TCallback callback)
{
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    if (isolate != NULL)
    {
        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = isolate->GetCurrentContext();
        if (context.IsEmpty() == false)
        {
            v8::Context::Scope context_scope(context);
            callback(isolate, context);
        }
        else
        {
            LogError("v8 isolate does not have a context");
        }
    }
}

template <typename T>
void nodejs_module::NodeJSUtils::on_libuv_handler(T* handle)
{
    auto callback = reinterpret_cast<std::function<void(T*)>*>(
        reinterpret_cast<uv_handle_t*>(handle)->data
    );
    (*callback)(handle);
    delete callback;
}

template<typename TCallback>
bool nodejs_module::NodeJSUtils::SetTimeout(TCallback callback, uint64_t timeout_in_ms)
{
    bool result;

    uv_loop_t* default_loop = uv_default_loop();
    if (default_loop == nullptr)
    {
        LogError("uv_default_loop() failed");
        result = false;
    }
    else
    {
        uv_timer_t* timer = reinterpret_cast<uv_timer_t*>(malloc(sizeof(uv_timer_t)));
        if (timer == nullptr)
        {
            LogError("malloc failed");
            result = false;
        }
        else
        {
            if (uv_timer_init(default_loop, timer) != 0)
            {
                LogError("uv_timer_init failed");
                free(timer);
                result = false;
            }
            else
            {
                auto timer_handler = new std::function<void(uv_timer_t*)>([callback](uv_timer_t* handle) {
                    callback();

                    // cleanup
                    uv_timer_stop(handle);
                    free(handle);
                });

                // save the std::function pointer so we can free this later
                reinterpret_cast<uv_handle_t*>(timer)->data = reinterpret_cast<void*>(timer_handler);

                if (uv_timer_start(timer, NodeJSUtils::on_libuv_handler, timeout_in_ms, 0) != 0)
                {
                    LogError("uv_timer_start failed");
                    free(timer);
                    result = false;
                }
                else
                {
                    result = true;
                }
            }
        }
    }

    return result;
}

template<typename TCallback>
bool nodejs_module::NodeJSUtils::SetIdle(TCallback callback)
{
    bool result;

    uv_loop_t* default_loop = uv_default_loop();
    if (default_loop == nullptr)
    {
        LogError("uv_default_loop() failed");
        result = false;
    }
    else
    {
        uv_idle_t* idler = reinterpret_cast<uv_idle_t*>(malloc(sizeof(uv_idle_t)));
        if (idler == nullptr)
        {
            LogError("malloc failed");
            result = false;
        }
        else
        {
            if (uv_idle_init(default_loop, idler) != 0)
            {
                LogError("uv_idle_init failed");
                free(idler);
                result = false;
            }
            else
            {
                auto idle_handler = new std::function<void(uv_idle_t*)>([callback](uv_idle_t* handle) {
                    callback();

                    // cleanup
                    uv_idle_stop(handle);
                    free(handle);
                });

                // save the std::function pointer so we can free this later
                reinterpret_cast<uv_handle_t*>(idler)->data = reinterpret_cast<void*>(idle_handler);

                if (uv_idle_start(idler, NodeJSUtils::on_libuv_handler) != 0)
                {
                    LogError("uv_idle_start failed");
                    free(idler);
                    result = false;
                }
                else
                {
                    result = true;
                }
            }
        }
    }

    return result;
}

#endif /*NODEJS_UTILS_H*/
