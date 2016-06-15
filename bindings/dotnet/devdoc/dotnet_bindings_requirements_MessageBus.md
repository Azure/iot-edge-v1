.NET bindings for Azure IoT Gateway Modules - Message Bus Class
===============================================================

Overview
--------


This document specifies the requirements for the .NET Message Bus Class which is part of Microsoft.Azure.IoT.Gateway namespace. 
An object of this class represents the bus, to where a message is going to be published. 
More details on the [high level design](./dotnet_bindings_hld.md).

Types
-----
```C#
namespace Microsoft.Azure.IoT.Gateway
{
    /// <summary> Object that represents the bus, to where a messsage is going to be published </summary>
    public class MessageBus
    {
        private long msgBusHandle;

        private long moduleHandle

        /// <summary>
        ///     Constructor for MessageBus. This is used by the Native level, the .NET User will receive an object of this. 
        /// </summary>
        /// <param name="msgBus">Adress of the native created msgBus, used internally.</param>
        /// <param name="sourceModuleHandle">Adress of the native moduleHandle, used internally.</param>
        public MessageBus(long msgBus, long moduleHandle);

        /// <summary>
        ///     Publish a message into the gateway message bus. 
        /// </summary>
        /// <param name="message">Object representing the message to be published into the bus.</param>
        /// <returns></returns>
        public void Publish(Message message);
    }
}
```

MessageBus Constructor
----------------------
```C#
public MessageBus(long msgBus);
```
Creates a .NET Object of type Microsoft.Azure.IoT.Gateway.MessageBus

**SRS_DOTNET_MESSAGEBUS_04_001: [** If msgBus is <= 0, MessageBus constructor shall throw a new ArgumentException **]**

**SRS_DOTNET_MESSAGEBUS_04_007: [** If moduleHandle is <= 0, MessageBus constructor shall throw a new ArgumentException  **]**

**SRS_DOTNET_MESSAGEBUS_04_002: [** If msgBus and moduleHandle is greater than 0, MessageBus constructor shall save this value and succeed. **]**


Publish
-------
```C#
public void Publish(Message message);
```

Gets a byte array from a Message object and calls exported native function to publish a message. 

**SRS_DOTNET_MESSAGEBUS_04_003: [** Publish shall call the Message.ToByteArray() method to get the Message object translated to byte array.  **]**

**SRS_DOTNET_MESSAGEBUS_04_004: [** Publish shall not catch exception thrown by ToByteArray.  **]**

**SRS_DOTNET_MESSAGEBUS_04_005: [** Publish shall call the native method `Module_DotNetHost_PublishMessage` passing the msgBus and moduleHandle  value saved by it's constructor, the byte[] got from Message and the size of the byte array. **]**

**SRS_DOTNET_MESSAGEBUS_04_006: [** If `Module_DotNetHost_PublishMessage` fails, Publish shall thrown an Application Exception with message saying that MessageBus Publish failed. **]**