Building Azure IoT Gateway Modules for UWP
=============================================

Overview
--------

This document describes the high level design of the UWP Azure IoT Gateway SDK. 


UWP Wrappers and objects
-------------------------

This is a layer written in UWP that wraps what is required to publish a given message. 
For UWP Modules the following wrappers will be provided:
1. `Message` - Object that represents a message;

2. `MessageBus` - Object that represents the bus, which will be used to publish messages on to the bus;

3. `IGatewayModule` - interface that has to be implemented by the .NET Module; 

4. `Gateway` - Object that represents the gateway, which will be used to construct the gateway;

The high level design of these objects and interfaces is documented below:

### Gateway
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ C#
    
    namespace Microsoft.Azure.IoT.Gateway
    {
        /// <summary> Object that represents a message on the message bus. </summary>
        public sealed class Gateway
        {
            public Gateway(IList<IGatewayModule> modules);
        }        
    }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


### Message
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ C#
    
    namespace Microsoft.Azure.IoT.Gateway
    {
        /// <summary> Object that represents a message on the message bus. </summary>
        public sealed class Message
        {
            public static Message CreateMessage(Message message);
            
            public Message(IList<byte> msgInByteArray);
            public Message(string content, IReadOnlyDictionary<string, string> properties);

            public IList<byte> GetContent();
            public IReadOnlyDictionary<string, string> GetProperties();

            public IList<byte> ToBytes();
        }        
    }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


### MessageBus
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ C#
    
    namespace Microsoft.Azure.IoT.Gateway
    {
        /// <summary> Object that represents the bus, to where a messsage is going to be published </summary>
        public sealed class MessageBus
        {
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
            ///     Creates a module using the specified configuration connecting to the specified message bus.
            /// </summary>
            /// <param name="bus">The bus onto which this module will connect.</param>
            /// <param name="configuration">A string with user-defined configuration for this module.</param>
            /// <returns></returns>
            void Create(MessageBus bus, string configuration);
            
            /// <summary>
            ///     Disposes of the resources allocated by/for this module.
            /// </summary>
            /// <returns></returns>
            void Destroy();

            /// <summary>
            ///     The module's callback function that is called upon message receipt.
            /// </summary>
            /// <param name="received_message">The message being sent to the module.</param>
            /// <returns></returns>                
            void Receive(Message received_message);                
    }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

