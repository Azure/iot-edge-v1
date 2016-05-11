// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <string>
#include <new>
#include <sstream>
#include <vector>

#include "uv.h"
#include "v8.h"
#include "node.h"

#include "azure_c_shared_utility/map.h"
#include "azure_c_shared_utility/constmap.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/iot_logging.h"
#include "azure_c_shared_utility/threadapi.h"
#include "parson.h"

#include "module.h"
#include "message.h"
#include "message_bus.h"
#include "messageproperties.h"

#include "nodejs_common.h"
#include "nodejs.h"
#include "modules_manager.h"

static void on_module_start(NODEJS_MODULE_HANDLE_DATA* handle_data);

static MODULE_HANDLE NODEJS_Create(MESSAGE_BUS_HANDLE bus, const void* configuration)
{
    MODULE_HANDLE result;
    const NODEJS_MODULE_CONFIG* module_config = reinterpret_cast<const NODEJS_MODULE_CONFIG*>(configuration);
    JSON_Value* json;

    /*Codes_SRS_NODEJS_13_001: [ NodeJS_Create shall return NULL if bus is NULL. ]*/
    /*Codes_SRS_NODEJS_13_002: [ NodeJS_Create shall return NULL if configuration is NULL. ]*/
    /*Codes_SRS_NODEJS_13_019: [ NodeJS_Create shall return NULL if configuration->configuration_json is not valid JSON. ]*/
    /*Codes_SRS_NODEJS_13_003: [ NodeJS_Create shall return NULL if configuration->main_path is NULL. ]*/
    /*Codes_SRS_NODEJS_13_013: [ NodeJS_Create shall return NULL if configuration->main_path is an invalid file system path. ]*/
    if (
            bus == NULL
            ||
            configuration == NULL
            ||
            module_config->main_path == NULL
            ||
            (
                module_config->configuration_json != NULL
                &&
                (json = json_parse_string(module_config->configuration_json)) == NULL
            )
            ||
            _access(module_config->main_path, 4) != 0
       )
    {
        /*Codes_SRS_NODEJS_13_004: [ NodeJS_Create shall return NULL if an underlying API call fails. ]*/
        LogError("Invalid input supplied.");
        result = NULL;
    }
    else
    {
        /*Codes_SRS_NODEJS_13_035: [ NodeJS_Create shall acquire a reference to the singleton ModulesManager object. ]*/
        auto modules_manager = nodejs_module::ModulesManager::Get();
        if (modules_manager == nullptr)
        {
            LogError("Could not get an instance of the modules manager object");
            result = NULL;
        }
        else
        {
            /*Codes_SRS_NODEJS_13_006: [ NodeJS_Create shall allocate memory for an instance of the NODEJS_MODULE_HANDLE_DATA structure and use that as the backing structure for the module handle. ]*/
            NODEJS_MODULE_HANDLE_DATA handle_data_input
            {
                bus,
                module_config->main_path,
                module_config->configuration_json,
                on_module_start
            };

            auto handle_data = modules_manager->AddModule(handle_data_input);
            if (handle_data == NULL)
            {
                LogError("Could not add Node JS module");
                result = NULL;
            }
            else
            {
                /*Codes_SRS_NODEJS_13_005: [ NodeJS_Create shall return a non-NULL MODULE_HANDLE when successful. ]*/
                result = reinterpret_cast<MODULE_HANDLE>(handle_data);
            }
        }
    }

    return result;
}

template<typename TValidator>
static bool validate_prop(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    v8::Local<v8::Object> object,
    const char* prop_name,
    TValidator validator
)
{
    bool result;
    auto prop_key = v8::String::NewFromUtf8(isolate, prop_name);
    if (prop_key.IsEmpty() == true)
    {
        LogError("Could not instantiate v8 string for constant '%s'", prop_name);
        result = false;
    }
    else
    {
        if (object->HasOwnProperty(prop_key) == false)
        {
            LogInfo("Object does not have expected property '%s'", prop_name);
            result = false;
        }
        else
        {
            auto prop_value = object->Get(prop_key);
            if (prop_value.IsEmpty() == true)
            {
                LogError("Unable to fetch property '%s' from the object", prop_name);
                result = false;
            }
            else
            {
                result = validator(prop_value);
                if (result == false)
                {
                    LogError("Validation failed for property '%s' on object", prop_name);
                }
            }
        }
    }

    return result;
}

static bool validate_object_prop(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    v8::Local<v8::Value> val,
    const char* prop_name)
{
    auto object = val->ToObject();
    return object.IsEmpty() == false
        &&
        validate_prop(
            isolate, context, object, prop_name,
            [](v8::Local<v8::Value> object)
    {
        return object->IsObject();
    }
    );
}

static bool validate_function_prop(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    v8::Local<v8::Value> val,
    const char* prop_name)
{
    auto object = val->ToObject();
    return object.IsEmpty() == false
        &&
        validate_prop(
            isolate, context, object, prop_name,
            [](v8::Local<v8::Value> object)
    {
        return object->IsFunction();
    }
    );
}

static bool validate_gateway(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    v8::Local<v8::Object> gateway
)
{
    // validate the shape of the object
    const char *prop_names[] =
    {
        "create",
        "receive",
        "destroy"
    };
    auto props_length = sizeof(prop_names) / sizeof(prop_names[0]);
    size_t i;
    for (i = 0; i < props_length; i++)
    {
        if (validate_function_prop(isolate, context, gateway, prop_names[i]) == false)
        {
            LogError("An error occurred when validating gateway object's property '%s'", prop_names[i]);
            break;
        }
    }

    // if i is not equal to props_length then something went wrong
    return i == props_length;
}

static bool validate_message(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    v8::Local<v8::Value> message
)
{
    // validate the shape of the object; it must have either properties
    // and/or contents; it's an error for both of the props to not exist
    return
        validate_object_prop(isolate, context, message, "properties") == true
        ||
        validate_object_prop(isolate, context, message, "contents") == true;
}

static std::string v8_to_std_string(v8::Local<v8::String> str)
{
    std::string str_value;

    try
    {
        auto buffer_length = str->Utf8Length() + 1;
        char* buffer = new char[buffer_length];
        if (str->WriteUtf8(buffer, buffer_length) != buffer_length)
        {
            LogError("An error occurred when converting string");
            delete[] buffer;
        }
        else
        {
            str_value = buffer;
        }
    }
    catch (std::bad_alloc err)
    {
        LogError("An error occurred when allocating buffer for string: %s", err.what());
    }

    return str_value;
}

static bool copy_properties_from_message(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    v8::Local<v8::Object> message,
    MAP_HANDLE message_properties
)
{
    bool result;
    if (validate_object_prop(isolate, context, message, "properties") == true)
    {
        auto prop_key = v8::String::NewFromUtf8(isolate, "properties");
        if (prop_key.IsEmpty() == true)
        {
            LogError("Could not instantiate v8 string for constant 'properties'");
            result = false;
        }
        else
        {
            // we know this is an object
            auto props = message->Get(prop_key)->ToObject();

            // copy every "own property" of this object where the key
            // is a string and the value is a string; ignore the rest
            auto prop_names = props->GetOwnPropertyNames();
            if (prop_names.IsEmpty() == false)
            {
                auto props_count = prop_names->Length();
                uint32_t i = 0;
                for (; i < props_count; ++i)
                {
                    // we are only interested in string property names
                    auto prop_name = prop_names->Get(i);
                    if (prop_name.IsEmpty() == false && prop_name->IsString() == true)
                    {
                        auto prop_name_str = prop_name->ToString();

                        // we are only interested in string property values
                        auto prop_value = props->Get(context, prop_name).ToLocalChecked();
                        if (prop_value.IsEmpty() == false && prop_value->IsString() == true)
                        {
                            auto prop_value_str = prop_value->ToString();

                            std::string name = v8_to_std_string(prop_name_str);
                            std::string value = v8_to_std_string(prop_value_str);
                            if (name.empty() == false && value.empty() == false)
                            {
                                if (Map_Add(message_properties, name.c_str(), value.c_str()) != MAP_OK)
                                {
                                    LogError("Map_Add failed for property '%s'", name.c_str());
                                    break;
                                }
                            }
                        }
                    }
                }

                // if i is not equal to props_count then something went wrong
                result = i == props_count;
            }
            else
            {
                // this is not an error; it's just a weird message so we log a warning and
                // continue on our merry way
                LogInfo("Warning: message object has a 'properties' property but it has no members");
                result = true;
            }
        }
    }
    else
    {
        result = true;
        LogInfo("Message doesn't have properties");
    }

    return result;
}

static unsigned char* copy_contents(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    v8::Local<v8::Object> message,
    size_t* psize
)
{
    unsigned char* result;

    *psize = 0;
    if (validate_object_prop(isolate, context, message, "contents") == true)
    {
        auto prop_key = v8::String::NewFromUtf8(isolate, "contents");
        if (prop_key.IsEmpty() == true)
        {
            LogError("Could not instantiate v8 string for constant 'contents'");
            result = NULL;
        }
        else
        {
            // we know this is an object
            auto contents = message->Get(prop_key)->ToObject();

            if (contents->IsUint8Array() == false)
            {
                LogInfo("Message contents is not a Uint8Array");
                result = NULL;
            }
            else
            {
                auto arr = contents.As<v8::Uint8Array>();
                if (arr.IsEmpty() == true)
                {
                    LogError("Could not convert message contents to a v8 Uint8Array");
                    result = NULL;
                }
                else
                {
                    *psize = arr->ByteLength();
                    result = (unsigned char*)malloc(*psize);
                    if (result == NULL)
                    {
                        *psize = 0;
                        LogError("malloc failed");
                    }
                    else
                    {
                        auto copied = arr->CopyContents(result, *psize);
                        if (copied != *psize)
                        {
                            LogError("CopyContents failed");
                            result = NULL;
                            *psize = 0;
                            free((void*)result);
                        }
                    }
                }
            }
        }
    }
    else
    {
        result = NULL;
    }

    return result;
}

static void message_bus_publish(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    // this MUST NOT be NULL
    auto isolate = info.GetIsolate();
    auto context = isolate->GetCurrentContext();

    // get a reference to NODEJS_MODULE_HANDLE_DATA; this MUST NOT be NULL
    NODEJS_MODULE_HANDLE_DATA* handle_data = reinterpret_cast<NODEJS_MODULE_HANDLE_DATA*>(isolate->GetData(0));

    // there must be one parameter whose type is 'object'
    if (
            info.Length() < 1
            ||
            info[0]->IsObject() == false
            ||
            validate_message(isolate, context, info[0]) == false
       )
    {
        LogError("message_bus_publish was called with invalid parameters");

        /*Codes_SRS_NODEJS_13_027: [ message_bus_publish shall set the return value to false if the arguments count is less than 1. ]*/
        /*Codes_SRS_NODEJS_13_028: [ message_bus_publish shall set the return value to false if the first argument is not a JS object. ]*/
        /*Codes_SRS_NODEJS_13_029: [ message_bus_publish shall set the return value to false if the first argument is an object whose shape does not conform to the following interface:
            interface StringMap {
                [key: string]: string;
            }

            interface Message {
                properties: StringMap;
                content: Uint8Array;
            }
        */
        info.GetReturnValue().Set(false);
    }
    else
    {
        // create and copy the message properties
        MAP_HANDLE message_properties = Map_Create(NULL);
        if (message_properties == NULL)
        {
            /*Codes_SRS_NODEJS_13_031: [ message_bus_publish shall set the return value to false if any underlying platform call fails. ]*/
            LogError("Map_Create() failed");
            info.GetReturnValue().Set(false);
        }
        else
        {
            auto message_obj = info[0]->ToObject();
            if (copy_properties_from_message(isolate, context, message_obj, message_properties) == false)
            {
                /*Codes_SRS_NODEJS_13_031: [ message_bus_publish shall set the return value to false if any underlying platform call fails. ]*/
                LogError("An error occurred when copying properties to the message handle");
                info.GetReturnValue().Set(false);
            }
            else
            {
                // copy the message contents
                MESSAGE_CONFIG message_config;
                message_config.sourceProperties = message_properties;
                message_config.source = copy_contents(
                    isolate, context, message_obj, &(message_config.size)
                );

                /*Codes_SRS_NODEJS_13_030: [ message_bus_publish shall construct and initialize a MESSAGE_HANDLE from the first argument. ]*/
                MESSAGE_HANDLE message = Message_Create(&message_config);
                if (message == NULL)
                {
                    /*Codes_SRS_NODEJS_13_031: [ message_bus_publish shall set the return value to false if any underlying platform call fails. ]*/
                    LogError("Message_Create() failed");
                    info.GetReturnValue().Set(false);
                }
                else
                {
                    /*Codes_SRS_NODEJS_13_032: [ message_bus_publish shall call MessageBus_Publish passing the newly constructed MESSAGE_HANDLE. ]*/
                    if (MessageBus_Publish(handle_data->bus, message) != MESSAGE_BUS_OK)
                    {
                        /*Codes_SRS_NODEJS_13_031: [ message_bus_publish shall set the return value to false if any underlying platform call fails. ]*/
                        LogError("MessageBus_Publish() failed");
                        info.GetReturnValue().Set(false);
                    }
                    else
                    {
                        /*Codes_SRS_NODEJS_13_033: [ message_bus_publish shall set the return value to true or false depending on the status of the MessageBus_Publish call. ]*/
                        info.GetReturnValue().Set(true);
                    }

                    /*Codes_SRS_NODEJS_13_034: [ message_bus_publish shall destroy the MESSAGE_HANDLE. ]*/
                    Message_Destroy(message);
                }

                free((void*)message_config.source);
            }

            Map_Destroy(message_properties);
        }
    }
}

static v8::Local<v8::Object> create_message_bus(v8::Isolate* isolate, v8::Local<v8::Context> context)
{
    // this is an 'empty' object by default
    v8::Local<v8::Object> result;

    auto message_bus_template = v8::ObjectTemplate::New(isolate);
    if (message_bus_template.IsEmpty() == true)
    {
        LogError("Could not instantiate an object template for the message bus");
    }
    else
    {
        auto publish_name = v8::String::NewFromUtf8(isolate, "publish");
        if (publish_name.IsEmpty() == true)
        {
            LogError("Could not instantiate v8 string for string constant 'publish'");
        }
        else
        {
            auto publish_function = v8::FunctionTemplate::New(isolate, message_bus_publish);
            if (publish_function.IsEmpty() == true)
            {
                LogError("Could not instantiate a function template for message_bus_publish");
            }
            else
            {
                // add the "publish" function to the object template
                message_bus_template->Set(publish_name, publish_function);

                // create a new instance of the template
                result = message_bus_template->NewInstance(context).ToLocalChecked();
            }
        }
    }

    return result;
}

static void register_module(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    // this MUST NOT be NULL
    auto isolate = info.GetIsolate();

    /*Codes_SRS_NODEJS_13_014: [ When the native implementation of GatewayModuleHost.registerModule is invoked it shall do nothing if at least 2 parameters have not been passed to it. ]*/
    /*Codes_SRS_NODEJS_13_015: [ When the native implementation of GatewayModuleHost.registerModule is invoked it shall do nothing if the first parameter passed to it is not a JavaScript object. ]*/
    /*Codes_SRS_NODEJS_13_037: [ When the native implementation of GatewayModuleHost.registerModule is invoked it shall do nothing if the second parameter passed to it is not a number. ]*/
    if (
        info.Length() < 2
        ||
        info[0]->IsObject() == false
        ||
        info[2]->IsNumber() == false
       )
    {
        LogError("register_module was called with invalid parameters");
        info.GetReturnValue().Set(false);
    }
    else
    {
        // validate the shape of the object
        auto gateway = info[0]->ToObject();
        auto context = isolate->GetCurrentContext();

        /*Codes_SRS_NODEJS_13_016: [ When the native implementation of GatewayModuleHost.registerModule is invoked it shall do nothing if the parameter is an object but does not conform to the following interface:
            interface GatewayModule {
                create: (messageBus: MessageBus, configuration: any) => boolean;
                receive: (message: Message) => void;
                destroy: () => void;
            }
        */
        if (validate_gateway(isolate, context, gateway) == false)
        {
            LogError("An invalid gateway object was provided");
            info.GetReturnValue().Set(false);
        }
        else
        {
            /*Codes_SRS_NODEJS_13_017: [ A JavaScript object conforming to the MessageBus interface defined shall be created:
                interface StringMap {
                    [key: string]: string;
                }

                interface Message {
                    properties: StringMap;
                    content: Uint8Array;
                }

                interface MessageBus {
                    publish: (message: Message) => boolean;
                }
            */
            auto message_bus = create_message_bus(isolate, context);
            if (message_bus.IsEmpty())
            {
                LogError("Could not instantiate a message bus object");
                info.GetReturnValue().Set(false);
            }
            else
            {
                // get a reference to NODEJS_MODULE_HANDLE_DATA; the second parameter passed in is the module ID;
                // this MUST NOT be NULL
                auto module_id = info[1]->ToNumber()->Uint32Value();
                NODEJS_MODULE_HANDLE_DATA& handle_data = nodejs_module::ModulesManager::Get()->GetModuleFromId(module_id);

                // save the gateway object reference in the handle; this will be freed
                // from NODEJS_Destroy
                handle_data.module_object.Reset(isolate, gateway);

                // invoke the 'create' method on 'gateway' passing the message bus
                auto create_method_name = v8::String::NewFromUtf8(isolate, "create");
                if (create_method_name.IsEmpty() == true)
                {
                    LogError("Could not instantiate a v8 string for the constant 'create'");
                    info.GetReturnValue().Set(false);
                }
                else
                {
                    // parse the configuration JSON
                    auto json_str = v8::String::NewFromUtf8(isolate, handle_data.configuration_json.c_str());
                    if (
                        handle_data.configuration_json.empty() == false &&
                        json_str.IsEmpty() == true
                       )
                    {
                        LogError("Could not instantiate a v8 string for configuration json");
                        info.GetReturnValue().Set(false);
                    }
                    else
                    {
                        v8::Local<v8::Value> config_json;
                        if (handle_data.configuration_json.empty() == false)
                        {
                            config_json = v8::JSON::Parse(isolate, json_str).ToLocalChecked();
                        }

                        // we already verified that this exists and is a function
                        auto create_method_value = gateway->Get(context, create_method_name).ToLocalChecked();
                        auto create_method = create_method_value.As<v8::Function>();

                        /*Codes_SRS_NODEJS_13_018: [ The GatewayModule.create method shall be invoked passing the MessageBus instance and a parsed instance of the configuration JSON string. ]*/
                        v8::Local<v8::Value> argv[] = { message_bus, config_json };
                        auto result = create_method->Call(context, gateway, 2, argv).ToLocalChecked();

                        // we expect result to be a boolean
                        if (result.IsEmpty() == true || result->IsBoolean() == false)
                        {
                            LogInfo("Warning: The gateway object's create method did not return a boolean");
                            info.GetReturnValue().Set(false);
                        }
                        else
                        {
                            if (result->BooleanValue() == false)
                            {
                                LogError("Gateway's create method failed.");
                                info.GetReturnValue().Set(false);
                            }
                            else
                            {
                                info.GetReturnValue().Set(true);
                            }
                        }
                    }
                }
            }
        }
    }
}

static bool create_gateway_host(v8::Isolate* isolate, v8::Local<v8::Context> context)
{
    // This function is always called from a libuv thread - so we are guaranteed to
    // be thread-safe. The gateway host object should be setup only once - the first
    // time that a node module is created. We track this via a static bool.
    static bool gateway_host_created = false;
    bool result;

    if (gateway_host_created == true)
    {
        result = gateway_host_created;
    }
    else
    {
        // get the global object from the context
        auto global = context->Global();
        if (global.IsEmpty() == true)
        {
            LogError("v8 context has no global object");
            result = false;
        }
        else
        {
            auto gateway_host_template = v8::ObjectTemplate::New(isolate);
            if (gateway_host_template.IsEmpty() == true)
            {
                LogError("Could not instantiate an object template for the GatewayModuleHost");
                result = false;
            }
            else
            {
                auto register_module_name = v8::String::NewFromUtf8(isolate, "registerModule");
                if (register_module_name.IsEmpty() == true)
                {
                    LogError("Could not instantiate v8 string for string constant 'registerModule'");
                    result = false;
                }
                else
                {
                    auto register_module_function = v8::FunctionTemplate::New(isolate, register_module);
                    if (register_module_function.IsEmpty() == true)
                    {
                        LogError("Could not instantiate a function template for register_module");
                        result = false;
                    }
                    else
                    {
                        // add the "registerModule" function to the object template
                        gateway_host_template->Set(register_module_name, register_module_function);

                        // create a new instance of the template
                        auto gateway_host = gateway_host_template->NewInstance(context).ToLocalChecked();
                        if (gateway_host.IsEmpty() == true)
                        {
                            LogError("Could not create an instance of the gateway host template");
                            result = false;
                        }
                        else
                        {
                            auto gateway_host_prop_name = v8::String::NewFromUtf8(isolate, "gatewayHost");
                            if (gateway_host_prop_name.IsEmpty() == true)
                            {
                                LogError("Could not instantiate v8 string for string contant 'gatewayHost'");
                                result = false;
                            }
                            else
                            {
                                // add the gatewayHost to the context global
                                result = global->Set(gateway_host_prop_name, gateway_host);

                                // save the creation status so we don't attempt this again
                                // if this was successful
                                gateway_host_created = result;
                            }
                        }
                    }
                }
            }
        }
    }

    return result;
}

static v8::Local<v8::Value> run_script(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    const char* script
)
{
    v8::Local<v8::Value> result;
    auto script_source = v8::String::NewFromUtf8(isolate, script);
    if (script_source.IsEmpty() == true)
    {
        LogError("Could not instantiate v8 string for JS script source");
    }
    else
    {
        auto script = v8::Script::Compile(context, script_source).ToLocalChecked();
        if (script.IsEmpty() == true)
        {
            LogError("Could not compile JS script");
        }
        else
        {
            result = script->Run(context).ToLocalChecked();
        }
    }

    return result;
}

static void on_module_start(NODEJS_MODULE_HANDLE_DATA* handle_data)
{
    v8::Isolate* isolate = handle_data->v8_isolate;
    if (isolate == NULL)
    {
        LogError("v8 isolate is NULL");
    }
    else
    {
        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope handle_scope(isolate);
        auto context = isolate->GetCurrentContext();
        if (context.IsEmpty() == true)
        {
            LogError("No active v8 context found.");
        }
        else
        {
            /*Codes_SRS_NODEJS_13_011: [ When the NODEJS_MODULE_HANDLE_DATA::on_module_start function is invoked, a new JavaScript object shall be created and added to the v8 global context with the name gatewayHost. The object shall implement the following interface (TypeScript syntax used below is just for the purpose of description):
                interface GatewayModuleHost {
                    registerModule: (module: GatewayModule, module_id: number) = > void;
                }
            */
            if (create_gateway_host(isolate, context) == false)
            {
                LogError("Could not create gateway host in v8 global context");
            }
            else
            {
                // build the piece of javascript we are going to run:
                //  gatewayHost.registerModule(require('<<js_main_path>>'), <<module_id>>);
                try
                {
                    std::stringstream script_str("gatewayHost.registerModule(require('");
                    script_str << handle_data->main_path << "'), " << handle_data->module_id << ");";

                    /*SRS_NODEJS_13_012: [ The following JavaScript is then executed supplying the contents of NODEJS_MODULE_HANDLE_DATA::main_path for the placeholder variable js_main_path:
                        gatewayHost.registerModule(require(js_main_path));
                    */
                    auto result = run_script(isolate, context, script_str.str().c_str());
                    if (result.IsEmpty() == true)
                    {
                        LogError("registerModule script returned NULL");
                    }
                    else
                    {
                        // this must be boolean and must evaluate to true
                        if (result->IsBoolean() == false)
                        {
                            LogError("Expected registerModule to return boolean but it did not.");
                        }
                        else
                        {
                            auto bool_result = result->ToBoolean();
                            if (bool_result->BooleanValue() == false)
                            {
                                LogError("Expected registerModule to return 'true' but it returned 'false'");
                            }
                        }
                    }
                }
                catch (std::bad_alloc& err)
                {
                    LogError("Memory allocation error when instantiating JS script source string - %s.", err.what());
                }
            }
        }
    }
}

static v8::Local<v8::Uint8Array> copy_contents_to_object(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    const CONSTBUFFER* content
)
{
    // this object does not have ownership of the buffer
    auto array_buffer = v8::ArrayBuffer::New(isolate, (void *)content->buffer, content->size);

    // create an Uint8Array object
    return v8::Uint8Array::New(array_buffer, 0, content->size);
}

static v8::Local<v8::Object> copy_properties_to_object(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    CONSTMAP_HANDLE message_properties
)
{
    const char * const *keys, *const *values;
    size_t count;
    v8::Local<v8::Object> result;

    if (ConstMap_GetInternals(message_properties, &keys, &values, &count) != CONSTMAP_OK)
    {
        LogError("Map_GetInternals failed");
    }
    else
    {
        v8::Local<v8::Object> props;
        size_t i = 0;
        for (; i < count; i++)
        {
            auto prop_key = v8::String::NewFromUtf8(isolate, keys[i]);
            if (prop_key.IsEmpty() == true)
            {
                LogError("Could not instantiate v8 string for property key '%s'", keys[i]);
                break;
            }
            else
            {
                auto prop_value = v8::String::NewFromUtf8(isolate, values[i]);
                if (prop_value.IsEmpty() == true)
                {
                    LogError("Could not instantiate v8 string for property value '%s'", values[i]);
                    break;
                }
                else
                {
                    auto status = props->CreateDataProperty(context, prop_key, prop_value);
                    if (status.FromMaybe(false) == false)
                    {
                        LogError("Could not create property '%s'", values[i]);
                        break;
                    }
                }
            }
        }

        // if i != count then something went wrong
        if (i == count)
        {
            result = props;
        }
    }

    return result;
}

struct RECEIVE_MESSAGE_CONTEXT
{
    MODULE_HANDLE module;
    MESSAGE_HANDLE message;
};

static void on_run_receive_message(uv_idle_t* handle)
{
    RECEIVE_MESSAGE_CONTEXT *call_context = reinterpret_cast<RECEIVE_MESSAGE_CONTEXT*>(handle->data);
    NODEJS_MODULE_HANDLE_DATA* handle_data = reinterpret_cast<NODEJS_MODULE_HANDLE_DATA*>(call_context->module);

    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    if (isolate == NULL)
    {
        LogError("v8 isolate is NULL");
    }
    else
    {
        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope handle_scope(isolate);
        auto context = isolate->GetCurrentContext();
        if (context.IsEmpty() == true)
        {
            LogError("No active v8 context found.");
        }
        else
        {
            /*Codes_SRS_NODEJS_13_022: [ NodeJS_Receive shall construct an instance of the Message interface as defined below:
                interface StringMap {
                    [key: string]: string;
                }

                interface Message {
                    properties: StringMap;
                    content: Uint8Array;
                }
            */

            // convert the message properties into a JS object
            auto js_props = copy_properties_to_object(
                isolate,
                context,
                Message_GetProperties(call_context->message)
            );

            // convert the contents into a JS Uint8Array
            v8::Local<v8::Uint8Array> js_contents;
            auto content = Message_GetContent(call_context->message);
            if (content != NULL && content->buffer != NULL && content->size > 0)
            {
                js_contents = copy_contents_to_object(isolate, context, content);
            }

            // create a JS object with 'properties' and 'content'
            v8::Local<v8::Object> js_message;
            if (js_props.IsEmpty() == false)
            {
                auto prop_key = v8::String::NewFromUtf8(isolate, "properties");
                if (prop_key.IsEmpty() == true)
                {
                    LogError("Could not instantiate v8 string for constant 'properties'");
                }
                else
                {
                    auto status = js_message->CreateDataProperty(context, prop_key, js_props);
                    if (status.FromMaybe(false) == false)
                    {
                        LogError("Could not add 'properties' property to JS message object");
                    }
                }
            }

            if (js_contents.IsEmpty() == false)
            {
                auto prop_key = v8::String::NewFromUtf8(isolate, "content");
                if (prop_key.IsEmpty() == true)
                {
                    LogError("Could not instantiate v8 string for constant 'content'");
                }
                else
                {
                    auto status = js_message->CreateDataProperty(context, prop_key, js_contents);
                    if (status.FromMaybe(false) == false)
                    {
                        LogError("Could not add 'content' property to JS message object");
                    }
                }
            }

            auto prop_key = v8::String::NewFromUtf8(isolate, "receive");
            if (prop_key.IsEmpty() == true)
            {
                LogError("Could not instantiate v8 string for constant 'receive'");
            }
            else
            {
                // invoke 'receive' method on gateway; we know this member
                // exists on the gateway
                auto gateway = handle_data->module_object.Get(isolate);
                auto receive_method = gateway->Get(context, prop_key).ToLocalChecked();
                if (receive_method.IsEmpty() == true || receive_method->IsFunction() == false)
                {
                    LogError("'receive' property on the gateway has an unexpected value");
                }
                else
                {
                    /*Codes_SRS_NODEJS_13_023: [ NodeJS_Receive shall invoke GatewayModule.receive passing the newly constructed Message instance. ]*/
                    auto receive_fn = receive_method.As<v8::Function>();
                    v8::Local<v8::Value> args[] = { js_message };
                    receive_fn->Call(gateway, 1, args);
                }
            }
        }
    }

    free(reinterpret_cast<void*>(call_context));
    uv_idle_stop(handle);
}

void NODEJS_Receive(MODULE_HANDLE module, MESSAGE_HANDLE message)
{
    /*Codes_SRS_NODEJS_13_020: [ NodeJS_Receive shall do nothing if module is NULL. ]*/
    /*Codes_SRS_NODEJS_13_021: [ NodeJS_Receive shall do nothing if message is NULL. ]*/
    if (module == NULL || message == NULL)
    {
        LogError("module/message handle is NULL");
    }
    else
    {
        uv_loop_t* default_loop = uv_default_loop();
        if (default_loop == NULL)
        {
            LogError("uv_default_loop() failed");
        }
        else
        {
            uv_idle_t idler;
            if (uv_idle_init(default_loop, &idler) != 0)
            {
                LogError("uv_idle_init failed");
            }
            else
            {
                RECEIVE_MESSAGE_CONTEXT* context = reinterpret_cast<RECEIVE_MESSAGE_CONTEXT*>(
                    malloc(sizeof(RECEIVE_MESSAGE_CONTEXT))
                );
                if (context == NULL)
                {
                    LogError("malloc failed");
                }
                else
                {
                    // save the handle data so we can later use this from the callback
                    context->module = module;
                    context->message = message;
                    reinterpret_cast<uv_handle_t*>(&idler)->data = reinterpret_cast<void*>(context);

                    /*Codes_SRS_NODEJS_13_038: [ NodeJS_Receive shall schedule a callback to be invoked on Node JS's event loop. ]*/
                    if (uv_idle_start(&idler, on_run_receive_message) != 0)
                    {
                        LogError("uv_idle_start failed");
                        uv_idle_stop(&idler);
                    }
                }
            }
        }
    }
}

static void on_quit_node(uv_idle_t* handle)
{
    NODEJS_MODULE_HANDLE_DATA* handle_data = reinterpret_cast<NODEJS_MODULE_HANDLE_DATA*>(handle->data);

    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    if (isolate == NULL)
    {
        LogError("v8 isolate is NULL");
    }
    else
    {
        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope handle_scope(isolate);
        auto context = isolate->GetCurrentContext();
        if (context.IsEmpty() == true)
        {
            LogError("No active v8 context found.");
        }
        else
        {
            // invoke 'destroy' on the module object
            auto destroy_method_name = v8::String::NewFromUtf8(isolate, "destroy");
            if (destroy_method_name.IsEmpty() == true)
            {
                LogError("Could not instantiate a v8 string for the constant 'destroy'");
            }
            else
            {
                auto gateway = handle_data->module_object.Get(isolate);

                // we already verified that this exists and is a function when the module
                // was registered
                auto destroy_method_value = gateway->Get(context, destroy_method_name).ToLocalChecked();
                auto destroy_method = destroy_method_value.As<v8::Function>();

                /*Codes_SRS_NODEJS_13_040: [ NodeJS_Destroy shall invoke the destroy method on module's JS implementation. ]*/
                destroy_method->Call(context, gateway, 0, nullptr);
            }

            /*Codes_SRS_NODEJS_13_025: [ NodeJS_Destroy shall free all resources. ]*/
            nodejs_module::ModulesManager::Get()->RemoveModule(handle_data->module_id);
        }
    }

    uv_idle_stop(handle);
}

static void NODEJS_Destroy(MODULE_HANDLE module)
{
    if (module == NULL)
    {
        /*Codes_SRS_NODEJS_13_024: [ NodeJS_Destroy shall do nothing if module is NULL. ]*/
        LogError("module handle is NULL");
    }
    else
    {
        uv_loop_t* default_loop = uv_default_loop();
        if (default_loop == NULL)
        {
            LogError("uv_default_loop() failed");
        }
        else
        {
            uv_idle_t idler;
            if (uv_idle_init(default_loop, &idler) != 0)
            {
                LogError("uv_idle_init failed");
            }
            else
            {
                reinterpret_cast<uv_handle_t*>(&idler)->data = reinterpret_cast<void*>(module);

                /*Codes_SRS_NODEJS_13_039: [ NodeJS_Destroy shall schedule a callback to be invoked on Node JS's event loop. ]*/
                if (uv_idle_start(&idler, on_quit_node) != 0)
                {
                    LogError("uv_idle_start failed");
                    uv_idle_stop(&idler);
                }
            }
        }
    }
}

/*
*	Required for all modules:  the public API and the designated implementation functions.
*/
static const MODULE_APIS NODEJS_APIS_all =
{
    NODEJS_Create,
    NODEJS_Destroy,
    NODEJS_Receive
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(NODEJS_MODULE)(void)
#else
MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void)
#endif
{
    /*Codes_SRS_NODEJS_13_026: [ Module_GetAPIS shall return a non-NULL pointer to a structure of type MODULE_APIS that has all fields initialized to non-NULL values. ]*/
    return &NODEJS_APIS_all;
}
