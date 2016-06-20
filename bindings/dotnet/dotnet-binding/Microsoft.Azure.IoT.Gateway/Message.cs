// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

using System;
using System.Collections.Generic;
using System.Collections;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Microsoft.Azure.IoT.Gateway
{
    /// <summary> Object that represents a message on the message bus. </summary>
    public class Message
    {
        private byte[] Content { set; get; }

        private Dictionary<string, string> Properties { set; get; }

        /// <summary>
        ///    Default constructor for Message. This would be an Empty message. 
        /// </summary>
        public Message()
        {
        }

        /// <summary>
        ///     Constructor for Message. This receives a byte array (defined at spec [message_requirements.md](../C:\repos\azure-iot-gateway-sdk\core\devdoc\message_requirements.md)).
        /// </summary>
        /// <param name="msgInByteArray">ByteArray with the Content and Properties of a message.</param>
        public Message(byte[] msgAsByteArray)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        ///     Constructor for Message. This constructor receives a byte[] as it's content and Properties.
        /// </summary>
        /// <param name="contentAsByteArray">Content of the Message</param>
        /// <param name="properties">Set of Properties that will be added to a message.</param>
        public Message(byte[] contentAsByteArray, Dictionary<string, string> properties)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        ///     Constructor for Message. This constructor receives a string as it's content and Properties.
        /// </summary>
        /// <param name="content">String with the ByteArray with the Content and Properties of a message.</param>
        /// <param name="properties">Set of Properties that will be added to a message.</param>
        public Message(string content, Dictionary<string, string> properties)
        {
            throw new NotImplementedException();
        }

        public Message(Message message)
        {
            throw new NotImplementedException();
        }


        /// <summary>
        ///    Converts the message into a byte array (defined at spec [message_requirements.md](../C:\repos\azure-iot-gateway-sdk\core\devdoc\message_requirements.md)).
        /// </summary>
        virtual public byte[] ToByteArray()
        {
            throw new NotImplementedException();
        }
    }
}
