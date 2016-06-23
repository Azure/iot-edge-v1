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
    /// <summary> Object that represents a message on the message bus. </summary>
    public class Message
    {
        public byte[] Content { get; }

        public Dictionary<string, string> Properties { get; }

        /// <summary>
        ///     Constructor for Message. This receives a byte array (defined at spec [message_requirements.md](../C:\repos\azure-iot-gateway-sdk\core\devdoc\message_requirements.md)).
        /// </summary>
        /// <param name="msgInByteArray">ByteArray with the Content and Properties of a message.</param>
        public Message(byte[] msgInByteArray);

        /// <summary>
        ///     Constructor for Message. This constructor receives a string as it's content and Properties.
        /// </summary>
        /// <param name="content">content of the message represented as a string.</param>
        /// <param name="properties">Set of Properties that will be added to a message.</param>
        public Message(string content, Dictionary<string, string> properties);

        /// <summary>
        ///     Constructor for Message. This constructor receives a byte[] as it's content and Properties.
        /// </summary>
        /// <param name="contentAsByteArray">Content of the Message</param>
        /// <param name="properties">Set of Properties that will be added to a message.</param>
        public Message(byte[] contentAsByteArray, Dictionary<string, string> properties);

        /// <summary>
        ///    Converts the message into a byte array (defined at spec [message_requirements.md](../C:\repos\azure-iot-gateway-sdk\core\devdoc\message_requirements.md)).
        /// </summary>
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

**SRS_DOTNET_MESSAGE_04_008: [** If any parameter is null, constructor shall throw a `ArgumentNullException` **]**

**SRS_DOTNET_MESSAGE_04_002: [** Message class shall have a constructor that receives a byte array with it's content format as described in [message_requirements.md](../C:\repos\azure-iot-gateway-sdk\core\devdoc\message_requirements.md) and it's `Content` and `Properties` are extracted and saved. **]**

**SRS_DOTNET_MESSAGE_04_006: [** If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an `ArgumentException` **]**

**SRS_DOTNET_MESSAGE_04_003: [** Message class shall have a constructor that receives a content as string and properties and store it. This string shall be converted to byte array based on System.Text.Encoding.UTF8.GetBytes().  **]**

**SRS_DOTNET_MESSAGE_04_004: [** Message class shall have a constructor that receives a content as byte[] and properties, storing them. **]**


ToByteArray
-------
```C#
public byte[] ToByteArray();
```

Serializes the message into a byte array according to format described at: [message_requirements.md](../C:\repos\azure-iot-gateway-sdk\core\devdoc\message_requirements.md)

**SRS_DOTNET_MESSAGE_04_005: [** Message Class shall have a ToByteArray method which will convert it's byte array `Content` and it's `Properties` to a byte[] which format is described at [message_requirements.md](../C:\repos\azure-iot-gateway-sdk\core\devdoc\message_requirements.md) **]**