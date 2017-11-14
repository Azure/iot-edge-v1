Building Azure IoT Gateway Modules in Node.js
=============================================

Overview
--------

This document outlines at a high level how the Azure IoT Gateway SDK enables the
building and running of gateway modules written using
[Node.js](https://nodejs.org/). The document describes how the Node.js engine is
embedded in the gateway process's memory and how the interaction between the
native and the Node.js runtime environment works.

Embedding Node.js
-----------------

The Node.js project supports building the engine as a dynamically loaded module
- a DLL or an SO. At the time of writing this is an experimental feature. Once
Node.js has been built as a shared library, building a simple
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
Node.js engine, it doesn't do a whole lot else. Ideally, we want to be able to
extend the Node.js runtime environment and introduce objects and functions. In
order to be able to do this however, we need to be able to access the [Google
V8](https://developers.google.com/v8/) instance that is initialized and setup by
the Node.js runtime.

### Intercepting the Node.js event loop

In order for us to be able to safely intercept the event loop that Node.js runs
we modified the source in a fork of the Node.js project. We essentially added an
additional callback function as a parameter to `node::Start`. The changes that
have been done to Node.js to enable this can be seen in [this
commit](https://github.com/avranju/node/commit/fb272256ef7743c9b1882aff39fe7f1685fb7cba).
With this change in place we are able to intercept Node.js’s event loop in the
following manner:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
node::Start(argc, argv, [](v8::Isolate* isolate) {
  // now we are in the Node.js's event loop y'all
});
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

### Where's my v8 `Isolate`?

In order to be able to do anything at all with the V8 JavaScript API you first
need an instance of the type
[v8::Isolate](http://v8.paulfryzel.com/docs/master/classv8_1_1_isolate.html).
The reference to the v8 `Isolate` object is passed as a parameter to the
callback we supply to `node::Start`. Once we have a reference to the `Isolate`
we are able to access pretty much everything else in the V8 API. Here's an
example code snippet that shows the introduction of a new object called
`gateway_host` into the global context. `gateway_host` has one method called
`get_message_broker` which simply prints a message on the console. Error checks
have been omitted for brevity:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ c
static void get_message_broker(const FunctionCallbackInfo<Value>& info) {
    printf("get_message_broker\n");
}

static void on_node_loop(v8::Isolate* isolate) {
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
    // get_message_broker function defined above when invoked
    gateway_host_template->Set(
        String::NewFromUtf8(isolate, "get_message_broker"),
        FunctionTemplate::New(isolate, get_message_broker)
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
        "gateway_host.get_message_broker();",
        NewStringType::kNormal).ToLocalChecked();
    Local<Script> script = Script::Compile(context, source).ToLocalChecked();
    script->Run(context);
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This might seem like a lot of code but is quite *par for the course* when
dealing with v8. This code does prove that we are able to interoperate with an
embedded instance of Node.js from a native application.

Gateway modules and Node.js
---------------------------

So how will an Azure IoT Gateway module written using Node.js integrate into a
gateway instance? Here's a picture that attempts to depict the relationship
between the gateway and the module implemented using Node.js.

![](./gateway_nodejs_binding.png)

### Node.js Module Host

The *Node.js Module Host* is a regular gateway module written in C. This module
hosts the Node.js engine and is responsible for managing the interaction between
the gateway process and the JavaScript code. There will only be a single
instance of the Node.js engine in the host. All Node.js modules participating in
a given gateway process will be hosted by the same Node.js engine. The Node.js
engine is instantiated and bootstrapped the first time that a gateway module
that uses Node.js is instantiated. Subsequent instances of Node.js modules
re-use the instance that was created before.

Once the Node.js engine has been initialized, it will stay resident in memory
indefinitely. The Node.js engine is not designed for being embedded and does not
support being unloaded from memory. The only way to terminate an instance of the
Node.js engine is to quit the process that is hosting the engine. Therefore, the
gateway does not *unload* Node.js once it has been loaded. Individual gateway
modules will however have their *destroy* callback invoked when it is time for
them to be unloaded.

The configuration for this module will include the path to the root JavaScript
file that is to be loaded and run when the module starts up. Here’s a sample
configuration provided in JSON syntax:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ json
{
    "modules": [
        {
            "name": "node_bot",
            "loader": {
                "name": "node",
                "entrypoint": {
                    "main.path": "/path/to/main.js",
                }
            },
            "args": "module configuration"
        }
    ]
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

We create a module loader for handling the loading of Node.js module. The
Node.js module loader essentially loads the Node.js binding module
implementation and hands off control to it. In the sample above we are using the
default Node.js loader that the gateway SDK ships with.

The `main.path` property specifies the path to the root JavaScript file that
should be loaded and `args` is configuration that’s specific to this gateway
module.

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
 * A message object that can be published to a message broker or
 * is received from a message broker.
 */
interface Message {
    properties: StringMap;
    content: Uint8Array;
}

/**
 * A message broker object.
 */
interface Broker {
    publish: (message: Message) => boolean;
}

/**
 * This interface is expected to be implemented by the gateway implementer.
 * This is where the core module functionality resides. The methods in this
 * interface have a 1:1 mapping with the functions expected of a regular C
 * gateway module.
 */
interface GatewayModule {
    create: (broker: Broker, configuration: any) => boolean;
    receive: (message: Message) => void;
    destroy: () => void;
}

/**
 * This interface defines the shape of an object made available in
 * the global context of the JavaScript code. The name of the global
 * is "gatewayHost".
 */
interface GatewayModuleHost {
    registerModule: (module: GatewayModule, module_id: Number) => void;
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

### Module\_Create

When the `Module_Create` function is invoked by the gateway, it performs the
following actions:

-   If this is the first time that this API is being called, then it starts a
    new thread to run the Node.js engine and starts up Node.js.

-   It sets up a callback function to be called from Node.js’s event loop.

-   When event loop callback is invoked, it creates an instance of a proxy
    object for the message broker (conforming to the `Broker` interface defined
    above).

-   If this is the first time that a Node.js module is being loaded, then it
    adds an object to V8’s global context that implements the
    `GatewayModuleHost` interface (defined in the TypeScript snippet above).
    This object is identified using the name `gatewayHost`.

-   The module then proceeds to execute the following piece of JavaScript where
    the variable `js_main_path` points to the fully qualified path to the root
    JavaScript file that *default exports* the object that implements the
    `GatewayModule` interface. The `module_id` is an internal identifier used by
    the Node.js hosting module to distinguish between different Node.js modules.

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ js
    gatewayHost.registerModule(require(js_main_path, module_id));
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

-   The `registerModule` call results in control shifting back to native code
    which now has a handle to the JavaScript object that implements
    `GatewayModule`. The module then proceeds to invoke `GatewayModule.create`
    passing the handle to the `Broker` object and the configuration that it has
    read from the module’s configuration

### Module\_Receive

When the `Module_Receive` function is invoked by the gateway, the module
constructs an object that implements the `Message` interface and invokes
`GatewayModule.receive` passing the message object instance to it.

### Module\_Destroy

The call to `Module_Destroy` is simply forwarded on to `GatewayModule.destroy`.
It then removes the module reference from it’s internal list of modules.

### Publishing of messages to the broker

Whenever the code running on the JavaScript side of the equation publishes a new
message to the message broker, it will first construct an object that implements
the `Message` interface and invoke `Broker.publish` at which point control
shifts to the native code where the implementation proceeds to construct a
`MESSAGE_HANDLE` from the JavaScript message object. Once the `MESSAGE_HANDLE`
has been initialized it calls `Broker_Publish`.

Developer Experience
--------------------

It is important that the developer experience while building a gateway module
using Node.js be seamless and familiar to Node.js developers. In order to enable
this, the following is proposed:

1.  We publish an NPM module to the public NPM repository containing binaries
    for a gateway executable, the Node.js hosting module, Node.js itself and a
    selection of pre-built modules such as the IoT Hub module and the Logger
    module.

2.  We provide a [Yeoman generator](http://yeoman.io/) that allows developers to
    quickly scaffold out a project that’s pre-configured with a gateway and a
    sample module implementation.

3.  The gateway executable that we package in the NPM module will include
    support for command line arguments that cause the Node.js engine to be
    bootstrapped with debugging enabled. This allows developers to attach
    debuggers to the Node.js runtime and use IDEs and tools such as WebStorm and
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
