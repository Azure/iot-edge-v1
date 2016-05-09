Building Azure IoT Gateway Modules in Node JS
=============================================

Overview
--------

This document outlines at a high level how the Azure IoT Gateway SDK will enable
the building and running of gateway modules written using [Node
JS](https://nodejs.org/). The document describes how the Node JS engine is
embedded in the gateway process's memory and how the interaction between the
native and the Node JS runtime environment will work.

Embedding Node JS
-----------------

At the time of writing, the open source [Node JS
project](https://github.com/nodejs/node) does not directly support being
embedded in another application. It is, however, possible to enable embedding
and build it as a shared library - i.e., a `.so` on Linux and as a `.dll` on
Windows - with some minor tweaks to the build scripts. The Node JS project has
been forked to enable embedding and the changes in [this
commit](https://github.com/nodejs/node/compare/ce3e3c5fe15479475c068482c48eb9cbf1ac9df5...f4f891c4e921559fdd466363573715b79b2dc774)
show the changes necessary to make this possible. Once Node JS has been built as
a shared library, building a simple
[REPL](https://en.wikipedia.org/wiki/Read%E2%80%93eval%E2%80%93print_loop)
command line app turns out to be a single line of code:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ c
#include "node.h"
int main(int argc, char **argv) {
    // Look ma, I started Node!
    return node::Start(argc, argv);
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Extending the Node runtime
--------------------------

While the single line of code shown above is sufficient to bootstrap and run the
Node JS engine, it doesn't, clearly, do a whole lot. Ideally, we want to be able
to extend the Node JS runtime environment and introduce objects and functions.
In order to be able to do this however, we need to be able to access the [Google
V8](https://developers.google.com/v8/) instance that is initialized and setup by
the Node JS runtime. Enter [libuv](http://libuv.org/)!

### Intercepting the Node JS event loop

Node JS uses **libuv** for managing it's event loop. As it happens, Node JS uses
*libuv*'s `uv_default_loop` API to acquire a reference to an event loop
instance. `uv_default_loop` returns a pointer to a global, lazily initialized
static instance. This means that we are able to use the `uv_idle_*` APIs to have
a callback that we define invoked from Node JS's event loop. Since Node JS does
not run the *libuv* event loop till it has initialized the runtime envrionment
completely, we know for certain that by the time our callback gets invoked Node
JS is fully initialized and primed for action. Here's what setting up an idle
handler in *libuv* looks like:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ c
#include "node.h"

static void on_idle(uv_idle_t* idler) {
    // Node JS is ready for action now.
    uv_idle_stop(idler);
}

int main(int argc, char **argv) {
    uv_idle_t idler;
    uv_idle_init(uv_default_loop(), &idler);
    uv_idle_start(&idler, on_idle);
    
    return node::Start(argc, argv);
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

### Where's my v8 `Isolate`?

In order to be able to do anything at all with the V8 JavaScript API you first
need an instance of the type
[v8::Isolate](http://v8.paulfryzel.com/docs/master/classv8_1_1_isolate.html).
The `Isolate` class has a public static member called
[GetCurrent](http://v8.paulfryzel.com/docs/master/classv8_1_1_isolate.html#afd8c10d0f01e2ae43522c3ddf0bb053d)
which has been
[documented](http://v8.paulfryzel.com/docs/master/classv8_1_1_isolate.html#afd8c10d0f01e2ae43522c3ddf0bb053d)
like so:

>   Returns the entered isolate for the current thread or NULL in case there is
>   no current isolate.

Since the *libuv* idle callback function is being invoked on Node JS's event
loop it turns out that there is an active isolate on the current thread at that
time. Once we have a reference to the `Isolate` we are able to access pretty
much everything else in the V8 API. Here's an example code snippet that shows
the introduction of a new object called `gateway_host` into the global context.
`gateway_host` has one method called `get_message_bus` which simply prints a
message on the console. Error checks have been omitted for brevity:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ c
static void get_message_bus(const FunctionCallbackInfo<Value>& info) {
    printf("get_message_bus\n");
}

static void on_idle(uv_idle_t* handle) {
    // get the v8 isolate being used by Node
    Isolate* isolate = Isolate::GetCurrent();
    {
        // sets the isolate as the active one in this execution scope
        Isolate::Scope isolate_scope(isolate);

        // the handle scope automatically collects JS objects that have been
        // created on the stack
        HandleScope handle_scope(isolate);

        // fetch the current JS context on the isolate
        Local<Context> context = isolate->GetCurrentContext();

        // enter the JS context (i.e. make it the active context)
        Context::Scope context_scope(context);

        // define a new JS object template
        Local<ObjectTemplate> gateway_host_template = ObjectTemplate::New(isolate);
        
        // add a member method to the object template and have it call the
        // get_message_bus function defined above when invoked
        gateway_host_template->Set(
            String::NewFromUtf8(isolate, "get_message_bus"),
            FunctionTemplate::New(isolate, get_message_bus)
        );
        
        // create a new object instance from the template
        auto gateway_host_maybe = gateway_host_template->NewInstance(context);
        
        // acquire a refernce to the "global" object from the context
        Local<Object> global = context->Global();
        
        // add a new property on the global object and have it point to the
        // new object we setup above
        global->Set(
            String::NewFromUtf8(isolate, "gateway_host"),
            gateway_host_maybe.ToLocalChecked()
        );

        // run a piece JS code that calls our code
        Local<String> source = String::NewFromUtf8(
            isolate,
            "gateway_host.get_message_bus();",
            NewStringType::kNormal).ToLocalChecked();
        Local<Script> script = Script::Compile(context, source).ToLocalChecked();
        script->Run(context);
    }

    uv_idle_stop(handle);
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This might seem like a lot of code but is quite *par for the course* when
dealing with v8. This code does prove that we are able to interoperate with an
embedded instance of Node JS from a native application.

Gateway modules and Node JS
---------------------------

So how will an Azure IoT Gateway module written using Node JS integrate into a
gateway instance? Here's a picture that attempts to depict the relationship
between the gateway and the module implemented using Node JS.

![](./gateway_node_bindings.png)

### Node JS Module Host

The *Node JS Module Host* is a regular gateway module written in C. This module
hosts the Node JS engine and is responsible for managing the interaction between
the gateway process and the JavaScript code. The configuration for this module
will include the path to the root JavaScript file that is to be loaded and run
when the module starts up. Here’s a sample configuration provided in JSON
syntax:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ json
{
    "modules": [
        {
            "module name": "node_bot",
            "module path": "/path/to/json_module.so|.dll",
            "args": {
                "main_path": "/path/to/main.js",
                "args": "module configuration"
            }
        }
    ]
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The `module path` specifies the path to the DLL/SO that implements the Node JS
host. The `args.main_path` property specifies the path to the root JavaScript
file that should be loaded and `args.args` is configuration that’s specific to
the gateway module.

A brief description of what each of the 3 module callbacks do is outlined below.
The code on the JavaScript side of the implementation has access to an API
provided by the module host. It maybe useful to have the interface definitions
of this API handy while reviewing the work that the module host’s create,
receive and destroy functions do. The API definition is provided below using
TypeScript syntax for expository purposes:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ts
/**
 * A string-to-string associative collection.
 */
interface StringMap {
    [key: string]: string;
}

/**
 * A message object that can be published on to a message bus or
 * is received from a message bus.
 */
interface Message {
    properties: StringMap;
    content: Uint8Array;
}

/**
 * A message bus object.
 */
interface MessageBus {
    publish: (message: Message) => boolean;
}

/**
 * This interface is expected to be implemented by the gateway implementer.
 * This is where the core module functionality resides. The methods in this
 * interface have a 1:1 mapping with the functions expected of a regular C
 * gateway module.
 */
interface GatewayModule {
    create: (messageBus: MessageBus, configuration: any) => boolean;
    receive: (message: Message) => void;
    destroy: () => void;
}

/**
 * This interface defines the shape of an object made available in
 * the global context of the JavaScript code. The name of the global
 * is "gatewayHost".
 */
interface GatewayModuleHost {
    registerModule: (module: GatewayModule) => void;
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

### Module\_Create

When the `Module_Create` function is invoked by the gateway, it performs the
following actions:

-   Starts a new thread to run the Node JS engine and returns.

-   When the new thread starts up, it sets up a *libuv* idle handler to be
    called once the Node JS runtime has been initialized and starts up Node JS.

-   When the *libuv* idle handler is invoked, it creates an instance of a proxy
    object for the Message Bus (conforming to the `MessageBus` interface defined
    above).

-   It then adds an object to V8’s global context that implements the
    `GatewayModuleHost` interface (defined in the TypeScript snippet above).
    This object is identified using the name `gatewayHost`.

-   The module then proceeds to execute the following piece of JavaScript where
    the variable `js_main_path` points to the fully qualified path to the root
    JavaScript file that *default exports* the object that implements the
    `GatewayModule` interface.

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ js
    gatewayHost.registerModule(require(js_main_path));
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

-   The `registerModule` call results in control shifting back to native code
    which now has a handle to the JavaScript object that implements
    `GatewayModule`. The module then proceeds to invoke `GatewayModule.create`
    passing the handle to the `MessageBus` object and the configuration that it
    has read from the module’s configuration

### Module\_Receive

When the `Module_Receive` function is invoked by the gateway, the module
constructs an object that implements the `Message` interface and invokes
`GatewayModule.receive` passing the message object instance to it.

### Module\_Destroy

The call to `Module_Destroy` is simply forwarded on to `GatewayModule.destroy`.
Once that function returns, the module proceeds to shut down the Node JS engine
instance.

### Publishing of messages to the bus

Whenever the code running on the JavaScript side of the equation publishes a new
message on to the message bus, it will first construct an object that implements
the `Message` interface and invoke `MessageBus.publish` at which point control
shifts to the native code where the implementation proceeds to construct a
`MESSAGE_HANDLE` from the JavaScript message object. Once the `MESSAGE_HANDLE`
has been initialized it calls `MessageBus_Publish`.

Another implementation option could be to have the JavaScript code serialize the
message into a byte array according to the requirements of the
`Message_CreateFromByteArray` API and then transfer that over to the native code
which then proceeds to construct a message from the byte array. Choice of the
best approach to adopt is left up to the implementation.

Developer Experience
--------------------

It is important that the developer experience while building a gateway module
using Node JS be seamless and familiar to Node JS developers. In order to enable
this, the following is proposed:

1.  We publish an NPM module to the public NPM repository containing binaries
    for a gateway executable, the Node JS hosting module, Node JS itself and a
    selection of pre-built modules such as the IoT Hub module and the Logger
    module.

2.  We provide a [Yeoman generator](http://yeoman.io/) that allows developers to
    quickly scaffold out a project that’s pre-configured with a gateway and a
    sample module implementation.

3.  The gateway executable that we package in the NPM module will include
    support for command line arguments that cause the Node JS engine to be
    bootstrapped with debugging enabled. This allows developers to attach
    debuggers to the Node JS runtime and use IDEs and tools such as WebStorm and
    Visual Studio Code to debug their module code.

A typical developer session might look like this:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ bash
# install yo
npm install -g yo

# install Azure IoT Gateway SDK generator
npm install -g generator-azure-iot-gateway

# run the generator to scaffold out an app
yo azure-iot-gateway

# build and run the gateway
npm install
npm build
npm start

# build and run the gateway under debug mode
npm install
npm build
npm debug
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
