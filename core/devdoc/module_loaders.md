Module Loaders in the Gateway
=============================

Introduction
------------

The Gateway SDK is essentially a plugin based system that is composed of
*modules* that provide the functionality of an IoT Gateway device. One of the
design goals of the Gateway SDK is the idea that the SDK will allow for
flexibility with respect to how a module is packaged and distributed and loaded
in the gateway. This document describes at a high level how this works. Gateway
modules can be written using different technology stacks. At the time of writing
modules can be written using C, .NET, Node.js and Java. The responsibility of
bootstrapping the respective runtimes (in case of stacks that have a runtime
that is) and loading the module code lies with the module loader.

What is a Module Loader?
------------------------

The primary duty of a module loader in the gateway is to locate and load a
module - or in other words, to abstract away from the gateway the details of
locating and loading a module. A gateway module maybe native or managed (.NET,
Node.js or Java). If its a managed module then the loader is responsible for
ensuring that the runtime needed to successfully load and run the module is
loaded and initialized first.

From the perspective of the gateway, a module loader is a piece of code that
knows how to load a module implementation and hand the gateway a table of
function pointers that contain the module implementation. The gateway doesn’t
really care how the module loader goes about acquiring the said pointers.

Loader Configuration
--------------------

A loader is defined by the following attributes:

-   **Type**: Can be *native*, *java*, *node* or *dotnet*

-   **Name**: A string that can be used to reference this particular loader

-   **Module Path**: Optional path to a language binding implementation for
    non-native loaders

-   **Configuration**: Optional additional configuration parameters that may be
    used to configure the runtime that is to be loaded

When initializing a gateway from a JSON configuration file via the
`Gateway_CreateFromJson` API, the loader configuration can be specified via the
top level `loaders` array. For example:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
{
  "loaders": [
    {
      "type": "java",
      "name": "java_loader",
      "jvm.options": {
        ... jvm options here ...
      }
    },
    {
      "type": "node",
      "name": "node_loader",
      "module.path": "/path/to/nodejs_binding.so"
    }
  ],

  "modules": [
    ... modules listing goes here ...
  ]
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Only the `type` and `name` properties for a given loader are required values.
The `module.path` property is optional and can be used to override the path to
the DLL/SO that implements the language binding for a language binding loader.
There may be other supported properties that are specific to a given type of
loader (like `jvm.options` above which can be used to specify the Java Virtual
Machine options to use for the Java language binding loader).

Built-in Module Loaders
-----------------------

The Gateway SDK ships with a set of loaders that support dynamically loaded
modules and language binding loaders for .NET, Node.js and Java. They can be
referenced using the following loader names:

-   `native_loader`: This implements loading of native modules - that is, plain
    C modules.

-   `java_loader`: This implements loading of modules implemented using the Java
    programming language.

-   `node_loader`: This implements loading of modules implemented using Node.js.

-   `dotnet_loader`: This implements loading of modules implemented using a .NET
    language.

It is legal to completely omit specifying the `loaders` array in which case the
default loaders specified above will be made available using default options.
Here’s an example of a module configuration that makes use of the Java loader:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
{
  "modules": [
    {
       "name": "printer",
       "loader": {
         "name": "java_loader",
         "class.name": "org.contoso.gateway.module.Printer",
         "class.path": "./printer.jar"
       }
    }
  ]
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

How module loaders work
-----------------------

Briefly, here’s how the gateway bootstraps itself:

1.  A module loader is defined using a struct called `MODULE_LOADER` that looks
    like this:

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    /**
     * Function typedefs for a module loader's function
     * pointer table.
     */
    typedef MODULE_LIBRARY_HANDLE (*pfModuleLoader_Load)
        (const void * config);
    typedef void (*pfModuleLoader_Unload)
        (MODULE_LIBRARY_HANDLE handle);
    typedef const MODULE_APIS* (*pfModuleLoader_GetApi)
        (MODULE_LIBRARY_HANDLE handle);

    /**
     * The module loader's function pointer table.
     */
    typedef struct MODULE_LOADER_API_TAG
    {
        pfModuleLoader_Load    Load;
        pfModuleLoader_Unload  Unload;
        pfModuleLoader_GetApi  GetApi;
    } MODULE_LOADER_API;

    /**
     * Module loader type enumeration.
     */
    typedef enum MODULE_LOADER_TYPE_TAG
    {
        NATIVE,
        JAVA,
        DOTNET,
        NODEJS
    } MODULE_LOADER_TYPE;

    /**
     * The Module Loader.
     */
    typedef struct MODULE_LOADER_TAG
    {
        MODULE_LOADER_TYPE   type;
        const char*          name;
        const char*          binding_module_path;
        const void*          loader_configuration;
        MODULE_LOADER_API*   api;
    } MODULE_LOADER;
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

2.  Module loaders for a given language binding may or may not be available
    depending on what *CMake* build options were used when the source was built.

3.  Every module loader implementation provides a global function that is
    responsible for returning a pointer to a `MODULE_LOADER` instance. The
    *native* loader that implements dynamic module loading for instance might
    provide a function that looks like this:

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    extern const MODULE_LOADER *DynamicLoader_Get(void);
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

4.  The gateway, during initialization, would create and populate a vector of
    `MODULE_LOADER` instances so that it has a default set of module loaders to
    work with. As discussed before, the gateway SDK, depending on build options
    used will ship with a set of pre-defined loaders. Here’s an example how this
    initialization code might look like:

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    MODULE_LOADER* module_loaders[] = {

        DynamicLoader_Get()

    #ifdef JAVA_BINDING_ENABLED
        , JavaBindingLoader_Get()
    #endif

    #ifdef DOTNET_BINDING_ENABLED
        , DotnetBindingLoader_Get()
    #endif

    #ifdef NODE_BINDING_ENABLED
        , NodeBindingLoader_Get()
    #endif
    };
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

5.  The `GATEWAY_PROPERTIES` struct will include a vector of `MODULE_LOADER`
    instances to capture custom loaders that the gateway might want to make use
    of. If the name of a loader in this vector matches the name of a default
    loader then it has the effect of overriding the default loader.

6.  When initializing custom loaders, if the caller has provided a value in
    `MODULE_LOADER::binding_module_path` then we proceed to make use of it. If
    there’s no `binding_module_path` provided then the gateway looks for a
    DLL/SO with a well-known name by invoking the underlying operating system’s
    default DLL/SO search algorithm. We also search in the folder where the
    gateway binary was run from and from the `./bin` folder. If the module
    cannot be located at the end of this search then it is an error.

7.  In case of a *native* module loader, there is no binding module. In order to
    avoid writing specialized code for just this case we will have a shim
    *binding* module where the module API implementation simply delegates to the
    *real* module’s API.

8.  The gateway calls the module loader’s `Load` callback to acquire a
    `MODULE_LIBRARY_HANDLE`. Each module loader’s implementation of this
    function would do what’s necessary to also load the associated runtime (in
    case of language binding modules).

9.  The language binding module loader implementations will use the dynamic
    DLL/SO loading API to load the binding module DLLs/SOs.

10. The gateway then proceeds to acquire the module’s function pointer table and
    calls `Module_Create` or `Module_CreateFromJson` to instantiate the module.
    In case of language binding modules this callback would be on the binding
    module itself.

 
-

 
