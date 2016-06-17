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
        /// <param name="msgInByteArray">ByteArray with the Content and Properties of a message.</param>
        public Message(byte[] msgInByteArray);

        /// <summary>
        ///     Constructor for Message. This constructor receives a string as it's content and Properties.
        /// </summary>
        /// <param name="content">String with the ByteArray with the Content and Properties of a message.</param>
        /// <param name="properties">Set of Properties that will be added to a message.</param>
        public Message(string content, Dictionary<string, string> properties);

        /// <summary>
        ///     Constructor for Message. This constructor receives a byte[] as it's content and Properties.
        /// </summary>
        /// <param name="contentInByteArray">Content of the Message</param>
        /// <param name="properties">Set of Properties that will be added to a message.</param>
        public Message(byte[] contentInByteArray, Dictionary<string, string> properties);

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
public Message(byte[] contentInByteArray, Dictionary<string, string> properties);
```
Creates a .NET Object of type Microsoft.Azure.IoT.Gateway.Message

**SRS_DOTNET_MESSAGE_04_001: [** Message class shall have an empty constructor which will create a message with an empty `Content`and empry `Properties` **]**

**SRS_DOTNET_MESSAGE_04_002: [** Message class shall have a constructor that receives a byte array which it's content format is described in [message_requirements.md](../C:\repos\azure-iot-gateway-sdk\core\devdoc\message_requirements.md) and it's `Content` and `Properties` are extracted and saved. **]**

**SRS_DOTNET_MESSAGE_04_006: [** If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it will throw an `ArgumentException` **]**

**SRS_DOTNET_MESSAGE_04_003: [** Message class shall have a constructor that receives a content as string and properties and store it. This string will be converted to byte array based on System.Text.Encoding.UTF8.GetBytes().  **]**

**SRS_DOTNET_MESSAGE_04_004: [** Message class shall have a constructor that receives a content as byte[] and properties, storing them. **]**


ToByteArray
-------
```C#
public byte[] ToByteArray();
```

//Description here. 
**SRS_DOTNET_MESSAGE_04_005: [** Message Class shall have a ToByteArray method which will convert it's byte array `Content` and it's `Properties` to a byte[] which format is described at [message_requirements.md](../C:\repos\azure-iot-gateway-sdk\core\devdoc\message_requirements.md) **]**