Building Azure IoT Gateway Modules in .NET
=============================================

Overview
--------

This document describes the high level design of the .NET binding mechanism used by Azure IoT Gateway SDK. 
It describes how the .NET CLR is hosted in the gateway process's memory and how the interaction between the
native and .NET Managed Modules will work.


Design
------
![](images/overall-design.png)


.NET Module Host
----------------
The **.NET Module Host** is a C module that

1. Creates a .NET CLR instance;

2. Brokers calls **to** the .NET Module (create, destroy, receive).

3. Brokers calls **from** the .NET Module (dotnetHost_PublishMessage, which invokes `MessageCreate_FromByteArray` and `Broker_Publish`);

###JSON Configuration

The JSON configuration for .NET Module will be similar to the configuration for Node and Java Module Host:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ JSON
{
    "modules": [
        {
            "name": "csharp_hello_world",
            "loader": {
                "name": "dotnet",
                "entrypoint": {
                    "assembly.name": "mymoduleassembly",
                    "entry.type": "mycsharpmodule.classname"
                }
            },
            "args": "module configuration"            
        }
    ]
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. `name` is the name of the module created (.NET Module) that will be stored by the gateway and used internally;

2. `loader` is the configuration specifically for the .NET Loader. 

    2.1 `loader->assembly.name`: Name of the managed assembly that contains the module implementation;
    
    2.2 `loader->entry.type`: Fully qualified name of the managed class that implements `IGatewayModule`; 
    
3. `args`  The value of this property is used to supply configuration information specific to a given .NET module. The value is passed as a byte[] to .NET module and it shall be converted as a UTF-8 String;  

##Native methods description
### Module\_Create

When the **.NET Module Host**’s `Module_Create` function is invoked by the
gateway process, it:

-   Creates a CLR instance; 

-   Loads the .NET module from the path indicated on the configuration (assembly_name) into Default App domain, by calling `ExecuteInDefaultAppDomain`, invokes default constructor at the class (entry_type) and calls `Create` method, implemented by the `IGatewayModule` interface;

### Module\_Start

When the **.NET Module Host**’s `Module_Start` function is invoked by the
gateway, it:

- Checks to see if the .NET module has implemented the `Start` method.
- If defined, invokes the `Start` method implemented by the .NET module.

### Module\_Receive

When the **.NET Module Host**’s `Module_Receive` function is invoked by the
gateway process, it:

-   Serializes (by calling `Message_ToByteArray` the message content and properties and invokes the `Receive` method implemented by the .NET module (`IGatewayInterface` below). The .NET module will deserialize this byte_array into a Message object.

### Module\_Destroy

When the **.NET Module Host**’s `Module_Destroy` function is invoked by the
gateway, it:

-   Releases resources allocated and calls the `Destroy` method implemented by the .NET Module.

.NET Wrappers and objects
-------------------------

This is going to be a layer written in .NET that will wrap a method in our host that is responsible to publish a given message. 
For .NET Modules the following wrappers will be provided:
1. `Message` - Object that represents a message;

2. `Broker` - Object that represents the broker, which passes messages between modules;

3. `IGatewayModule` - interface that has to be implemented by the .NET Module; 

4. `nativeDotNetHostWrapper` - Uses DLLImport to marshal call to dotnetHost_PublishMessage. This will be transparent to the .NET User, it will be called by the Broker Class when the user calls Publish.

The high level design of these objects and interfaces is documented below:

### Message
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ C#
    
    namespace Microsoft.Azure.IoT.Gateway
    {
        /// <summary> Object that represents a message passed between modules. </summary>
        public class Message
        {
            public byte[] Content { set; get; };
            
            public Dictionary<string,string> Properties  { set; get; };

            public Message();
            
            public Message(byte[] msgInByteArray);
            
            public Message(string content, Dictionary<string, string> properties); 
            
            public Message(Message message);
            
            public byte[] ToByteArray();
        }        
    }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


### Broker
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ C#
    
    namespace Microsoft.Azure.IoT.Gateway
    {
        /// <summary> Object that represents the message broker, to which messsages will be published. </summary>
        public class Broker
        {
            /// <summary>
            ///     Publish a message to the message broker. 
            /// </summary>
            /// <param name="message">Object representing the message to be published to the broker.</param>
            /// <returns></returns>
            public void Publish(Message message);
        }        
    }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

### IGatewayModule
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ C#
    
    /// <summary> Interface to be implemented by the .NET Module </summary>
    public interface IGatewayModule
    {
            /// <summary>
            ///     Creates a module using the specified configuration connecting to the specified message broker.
            /// </summary>
            /// <param name="broker">The broker to which this module will connect.</param>
            /// <param name="configuration">A byte[] with user-defined configuration for this module. This parameter shall be enconded to a UTF-8 String.</param>
            /// <returns></returns>
            void Create(Broker broker, byte[] configuration);
            
            /// <summary>
            ///     Disposes of the resources allocated by/for this module.
            /// </summary>
            /// <returns></returns>
            void   Destroy();

            /// <summary>
            ///     The module's callback function that is called upon message receipt.
            /// </summary>
            /// <param name="received_message">The message being sent to the module.</param>
            /// <returns></returns>                
            void Receive(Message received_message);                
    }
    
    /// <summary> Optional Start Interface to be implemented by the .NET Module </summary>
    public interface IGatewayModuleStart
    {

        /// <summary>
        ///     Informs module the gateway is ready to send and receive messages.
        /// </summary>
        /// <returns></returns>
        void Start(); 
        
    }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


Flow Diagram
------------

Following is the flow diagram of a lifecycle of the .NET module: 
![](images/flow_chart.png)
