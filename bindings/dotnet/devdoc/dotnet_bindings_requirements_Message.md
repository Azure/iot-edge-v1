.NET bindings for Azure IoT Gateway Modules - Message Class
===========================================================

Overview
--------


This document specifies the requirements for the .NET Message which is part of Microsoft.Azure.IoT.Gateway namespace. 
An object of this class represents a message. 
More details can be found in the [high level design](./dotnet_bindings_hld.md).

Types
-----
```C#
namespace Microsoft.Azure.IoT.Gateway
{
    private byte[] Content { set; get; }

    private Dictionary<string, string> Properties { set; get; }

    /// <summary> Object that represents a message on the message bus. </summary>
    public class Message
    {
        /// <summary>
        ///    Default constructor for Message. This would be an Empty message. 
        /// </summary>
        public Message();

        /// <summary>
        ///     Constructor for Message. This receives a byte array (defined at spec [message_requirements.md](../C:\repos\azure-iot-gateway-sdk\core\devdoc\message_requirements.md)). 
        /// </summary>
        /// <param name="msgBus">Adress of the native created msgBus, used internally.</param>
        /// <param name="sourceModuleHandle">Adress of the native moduleHandle, used internally.</param>
        public Message(byte[] msgInByteArray);

        public Message(string content, Dictionary<string, string> properties);

        public byte[] ToByteArray();
    }
}
```

Message Constructors
--------------------
```C#
public Message();
public Message(byte[] msgInByteArray);
public Message(string content, Dictionary<string, string> properties);
```
Creates a .NET Object of type Microsoft.Azure.IoT.Gateway.Message

**SRS_DOTNET_MESSAGE_04_001: [** Message class shall have an empty contructor which will create a message with an empty `Content`and empry `Properties` **]**




ToByteArray
-------
```C#
public byte[] ToByteArray();
```

//Description here. 
