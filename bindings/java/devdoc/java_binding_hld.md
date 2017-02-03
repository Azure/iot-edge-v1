Java Binding High Level Design
==============================

Overview
--------

This document describes the high level design of the Java binding mechanism used
by the Azure IoT Gateway SDK. It details how Java modules running inside of the
JVM will interact with the native gateway core.

Design
------

![](HLD1.png)

Java Module Host
----------------

The **Java Module Host** is a C module that

1.  Creates the JVM (Java Virtual Machine) the first time a Java module is
    attempting to connect to the gateway.

2.  Brokers calls **to** the Java module (create, destroy, receive) and
    facilitates publishing **from** the Java module to the native message broker.

Because [JNI](http://docs.oracle.com/javase/8/docs/technotes/guides/jni/) (Java
Native Interface) only allows one JVM instance per process, the **Java Module
Host** will create a JVM **only once** when the first Java module is loading.
Subsequent attempts to load additional Java modules will load and run those
modules in the same JVM that was originally created. The JSON configuration for
this module will be similar to the configuration for the Node Module Host:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ json
{
    "loaders": [
        {
            "type": "java",
            "name": "java",
            "configuration": {
                "jvm.options": {
                    "library.path": "./path/to/java_module_host.so|.dll",
                    "version": 8,
                    "debug": true,
                    "debug.port": 9876,
                    "verbose": false,
                    "additional.options": [
                        "-Djava.version=1.8"
                    ]
                }
            }
        }
    ],
    "modules": [
        {
            "name": "java_poller",
            "loader": {
                "name": "java",
                "entrypoint": {
                    "class.name": "Poller",
                    "class.path": "/path/to/relevant/class/files"
                },
                "args": {
                    "frequency": 30
                }
            }
        }
    ]
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

As usual, the `library.path` specifies the path to the DLL/SO that implements the
**Java Module Host**. The `entrypoint.class.path` specifies the path to the directory
where all necessary Java class files are located, `entrypoint.class.name` is the name
of the class that implements the module code. `entrypoint.jvm.options` is
a JSON object containing any options to be passed to the JVM upon creation.

 

Gateway Module (Java)
---------------------

The **Java Module Host** will handle calling into the gateway module written in
Java when necessary, therefore implementation methods for `create`, `start`, `destroy` and `receive` should not hang. 
Each module written in Java must implement the `IGatewayModule` interface shown below:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ java
public interface IGatewayModule {

    /**
     * The create method is called when the native Gateway creates the Module.
     *
     * @param moduleAddr The address of the native module pointer
     * @param broker The {@link Broker} to which this Module belongs
     * @param configuration The configuration for this module represented as a JSON
     * string
     */
    void create(long moduleAddr, Broker broker, String configuration);

    /**
     * The start method is called when the native Gateway has created all modules 
     * to notify each module that it is safe to start sending messages.
     *
     * This method is optional, but it is recommended that any long-running 
     * processes get started in this method.
     */
    void start();

    /**
     * The destroy method is called on a {@link GatewayModule} before it is about 
     * to be "destroyed" and removed from the gateway.
     * Once a module is removed from the gateway, it may no longer send or receive 
     * messages.
     *
     * The destroy() and receive() methods are guaranteed to not be called 
     * simultaneously.
     */
    void destroy();

    /**
     * The receive method is called on a {@link GatewayModule} whenever it receives
     * a message.
     *
     * The destroy() and receive() methods are guaranteed to not be called 
     * simultaneously.
     *
     * @param source The message content
     */
    void receive(byte[] source);
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The implementaion of `IGatewayModule` should save the moduleAddr and broker arguments from `create` method in order to use them later to send messages to gateway. 
To simplify this, the Azure IoT Gateway SDK provides an abstract `GatewayModule` class which implements the `IGatewayModule` interface. Module-implementers
may extend this abstract class when creating a module. If the module extends the `GatewayModule` class the constructor calls `create` method.

 

Exactly like a standard gateway module written in C, the gateway will handle
making calls to `Module_Create`, `Module_Start`, `Module_Receive`, and 
`Module_Destroy`. These three functions are implemented by the **Java Module Host** 
which handles the communication to the gateway module written in Java. Below is 
a description of what the **Java Module Host** will do in each of these cases:

### Module\_Create

When the **Java Module Host**’s `Module_Create` function is invoked by the
gateway, it:

-   Creates a JVM with the provided JVM configuration if this is the first Java
    module added to the gateway.

-   Constructs a `Broker` Java object using the `BROKER_HANDLE`.

-   Finds the module’s class with the name specified by the `entrypoint.class.name`,
    finds the constructor that matches these three arguments: the native `MODULE_HANDLE` address,
    the `Broker` object and the JSON args string for that module and invokes it. 
    If the constructor with three parameters is not found, finds no-argument constructor and invokes it and 
    invokes `create` method implemented by the Java module.

-   Gets a global reference to the newly created `GatewayModule` object to be
    saved by the `JAVA_MODULE_HOST_HANDLE`.

### Module\_Start

When the **Java Module Host**’s `Module_Start` function is invoked by the
gateway, it:

-   Attaches the current thread to the JVM

-   Invokes the `start` method implemented by the Java module if it exists.

### Module\_Receive

When the **Java Module Host**’s `Module_Receive` function is invoked by the
gateway, it:

-   Serializes the `MESSAGE_HANDLE` content and properties and invokes the
    `receive` method implemented by the Java module with the serialized message.

### Module\_Destroy

When the **Java Module Host**’s `Module_Destroy` function is invoked by the
gateway, it:

-   Attaches the current thread to the JVM

-   Invokes the `destroy` method implemented by the Java module.

-   Deletes the global reference to the Java module object.

-   Destroys the JVM if it is the last Java module to be destroyed.

Communication **FROM** the Java module
--------------------------------------

In order to communicate **from** the Java module to the native gateway process,
the `Broker` class must be used. The `Broker` class provides a method to
publish messages to the native message broker and loads a dynamic library that
implements the functions for publishing to the native message broker. So, the 
above diagram should look a bit more like this:

![](HLD2.png)
