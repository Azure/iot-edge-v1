#include <string>
#include "v8.h"
#include "azure_c_shared_utility/xlogging.h"
#include "nodejs_utils.h"

using namespace nodejs_module;

v8::Local<v8::Value> nodejs_module::NodeJSUtils::RunScript(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    std::string script_text
)
{
    v8::Local<v8::Value> result;

    auto script_source = v8::String::NewFromUtf8(isolate, script_text.c_str());
    if (script_source.IsEmpty() == true)
    {
        LogError("Could not instantiate v8 string for JS script source - %s", script_text.c_str());
    }
    else
    {
        auto script = v8::Script::Compile(context, script_source).ToLocalChecked();
        if (script.IsEmpty() == true)
        {
            LogError("Could not compile JS script - %s", script_text.c_str());
        }
        else
        {
            result = script->Run(context).ToLocalChecked();
        }
    }

    return result;
}

PersistentCopyable<v8::ObjectTemplate> nodejs_module::NodeJSUtils::CreateObjectTemplateWithMethod(
    std::string method_name,
    v8::FunctionCallback callback
)
{
    // this is an 'empty' object by default
    PersistentCopyable<v8::ObjectTemplate> result;

    RunWithNodeContext([&result, &method_name, &callback](v8::Isolate* isolate, v8::Local<v8::Context> context) {
        auto obj_template = v8::ObjectTemplate::New(isolate);
        if (obj_template.IsEmpty() == true)
        {
            LogError("Could not instantiate an object template for the object");
        }
        else
        {
            auto method_name_str = v8::String::NewFromUtf8(isolate, method_name.c_str());
            if (method_name_str.IsEmpty() == true)
            {
                LogError("Could not instantiate v8 string for string");
            }
            else
            {
                auto callback_function = v8::FunctionTemplate::New(isolate, callback);
                if (callback_function.IsEmpty() == true)
                {
                    LogError("Could not instantiate a function template for the callback function");
                }
                else
                {
                    // add the function to the object template
                    obj_template->Set(method_name_str, callback_function);
                    result.Reset(isolate, obj_template);
                }
            }
        }
    });

    return result;
}

PersistentCopyable<v8::Object> nodejs_module::NodeJSUtils::CreateObjectWithMethod(std::string method_name, v8::FunctionCallback callback)
{
    // this is an 'empty' object by default
    PersistentCopyable<v8::Object> result;

    RunWithNodeContext([&result, &method_name, &callback](v8::Isolate* isolate, v8::Local<v8::Context> context) {
        auto obj_template = v8::ObjectTemplate::New(isolate);
        if (obj_template.IsEmpty() == true)
        {
            LogError("Could not instantiate an object template for the object");
        }
        else
        {
            auto method_name_str = v8::String::NewFromUtf8(isolate, method_name.c_str());
            if (method_name_str.IsEmpty() == true)
            {
                LogError("Could not instantiate v8 string for string");
            }
            else
            {
                auto callback_function = v8::FunctionTemplate::New(isolate, callback);
                if (callback_function.IsEmpty() == true)
                {
                    LogError("Could not instantiate a function template for the callback function");
                }
                else
                {
                    // add the function to the object template
                    obj_template->Set(method_name_str, callback_function);
                    if (obj_template.IsEmpty() == true)
                    {
                        LogError("Could not create v8 object template");
                    }
                    else
                    {
                        auto instance = obj_template->NewInstance(context).ToLocalChecked();
                        result.Reset(isolate, instance);
                    }
                }
            }
        }
    });

    return result;
}

bool nodejs_module::NodeJSUtils::AddObjectToGlobalContext(std::string prop_name, PersistentCopyable<v8::Object>& obj)
{
    bool result;

    RunWithNodeContext([&result, &prop_name, &obj](v8::Isolate* isolate, v8::Local<v8::Context> context) {
        auto global = context->Global();
        if (global.IsEmpty() == true)
        {
            LogError("v8 context has no global object");
            result = false;
        }
        else
        {
            auto prop_name_str = v8::String::NewFromUtf8(isolate, prop_name.c_str());
            if (prop_name_str.IsEmpty() == true)
            {
                LogError("Could not instantiate v8 string for string");
                result = false;
            }
            else
            {
                result = global->Set(prop_name_str, obj.Get(isolate));
            }
        }
    });

    return result;
}