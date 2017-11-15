.NET binding for Azure IoT Gateway Modules - Broker Class
===============================================================

Overview
--------


This document specifies the requirements for the .NET Broker Class which is part of Microsoft.Azure.IoT.Gateway namespace. 
An object of this class represents the message broker, to which messages will be published. 
More details can be found in the [high level design](./dotnet_binding_hld.md).

Types
-----
```C#
namespace Microsoft.Azure.IoT.Gateway
{
    /// <summary> Object that represents the message broker, to which messsages will be published </summary>
    public class Broker
    {
        private long brokerHandle;

        private long moduleHandle

        /// <summary>
        ///   This constructor is used by the native module hosting the CLR. The .NET module implementation will receive an instance of Broker and will never instantiate one directly. 
        /// </summary>
        /// <param name="broker">Handle to the native message broker object.</param>
        /// <param name="module">Handle to the native module.</param>
        public Broker(long broker, long module);

        /// <summary>
        ///     Publish a message to the message broker. 
        /// </summary>
        /// <param name="message">Object representing the message to be published to the broker.</param>
        /// <returns></returns>
        public void Publish(Message message);
    }
}
```

Broker Constructor
----------------------
```C#
public Broker(long broker, long module);
```
Creates a .NET Object of type Microsoft.Azure.IoT.Gateway.Broker

**SRS_DOTNET_BROKER_04_001: [** If broker is <= 0, Broker constructor shall throw a new ArgumentException **]**

**SRS_DOTNET_BROKER_04_007: [** If module is <= 0, Broker constructor shall throw a new ArgumentException  **]**

**SRS_DOTNET_BROKER_04_002: [** If broker and module are greater than 0, Broker constructor shall save this value and succeed. **]**


Publish
-------
```C#
public void Publish(IMessage message);
```
Publish transforms the message into a byte array and calls exported function (`Module_DotNetHost_PublishMessage`) to publish a message.

**SRS_DOTNET_BROKER_04_003: [** Publish shall call the Message.ToByteArray() method to get the Message object translated to byte array.  **]**

**SRS_DOTNET_BROKER_04_004: [** Publish shall not catch exception thrown by ToByteArray.  **]**

**SRS_DOTNET_BROKER_04_005: [** Publish shall call the native method `Module_DotNetHost_PublishMessage` passing the broker and module value saved by it's constructor, the byte[] got from Message and the size of the byte array. **]**

**SRS_DOTNET_BROKER_04_006: [** If `Module_DotNetHost_PublishMessage` fails, Publish shall throw an `ApplicationException` with message saying that Broker Publish failed. **]**