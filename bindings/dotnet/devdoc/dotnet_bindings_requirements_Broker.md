.NET bindings for Azure IoT Gateway Modules - Message Bus Class
===============================================================

Overview
--------


This document specifies the requirements for the .NET Message Bus Class which is part of Microsoft.Azure.IoT.Gateway namespace. 
An object of this class represents the bus, to which a message is going to be published. 
More details can be found in the [high level design](./dotnet_bindings_hld.md).

Types
-----
```C#
namespace Microsoft.Azure.IoT.Gateway
{
    /// <summary> Object that represents the bus, to which a messsage is going to be published </summary>
    public class MessageBus
    {
        private long msgBusHandle;

        private long moduleHandle

        /// <summary>
        ///   This constructor is used by the native module hosting the CLR. The .NET module implementation will receive an instance of MessageBus and will never instantiate one directly. 
        /// </summary>
        /// <param name="msgBus">Handle to the native message bus object.</param>
        /// <param name="sourceModuleHandle">Handle to the native module.</param>
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
public MessageBus(long msgBus, long moduleHandle);
```
Creates a .NET Object of type Microsoft.Azure.IoT.Gateway.MessageBus

**SRS_DOTNET_MESSAGEBUS_04_001: [** If msgBus is <= 0, MessageBus constructor shall throw a new ArgumentException **]**

**SRS_DOTNET_MESSAGEBUS_04_007: [** If moduleHandle is <= 0, MessageBus constructor shall throw a new ArgumentException  **]**

**SRS_DOTNET_MESSAGEBUS_04_002: [** If msgBus and moduleHandle is greater than 0, MessageBus constructor shall save this value and succeed. **]**


Publish
-------
```C#
public void Publish(IMessage message);
```
Publish transforms the message into a byte array and calls exported function (`Module_DotNetHost_PublishMessage`) to publish a message.

**SRS_DOTNET_MESSAGEBUS_04_003: [** Publish shall call the Message.ToByteArray() method to get the Message object translated to byte array.  **]**

**SRS_DOTNET_MESSAGEBUS_04_004: [** Publish shall not catch exception thrown by ToByteArray.  **]**

**SRS_DOTNET_MESSAGEBUS_04_005: [** Publish shall call the native method `Module_DotNetHost_PublishMessage` passing the msgBus and moduleHandle  value saved by it's constructor, the byte[] got from Message and the size of the byte array. **]**

**SRS_DOTNET_MESSAGEBUS_04_006: [** If `Module_DotNetHost_PublishMessage` fails, Publish shall throw an `ApplicationException` with message saying that MessageBus Publish failed. **]**