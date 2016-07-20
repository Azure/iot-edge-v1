// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef NODEJS_UTILS_H
#define NODEJS_UTILS_H

#include <string>
#include <cstdint>
#include <functional>
#include <memory>

#include "uv.h"
#include "v8.h"

#include "azure_c_shared_utility/xlogging.h"

#include "lock.h"

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

#endif /*NODEJS_UTILS_H*/
