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
    public class Message : IMessage
    {
        private byte[] Content { set; get; }

        private Dictionary<string, string> Properties { set; get; }

        public Message()
        {
            throw new NotImplementedException();
        }

        public Message(byte[] msgInByteArray)
        {
            throw new NotImplementedException();
        }

        public Message(string content, Dictionary<string, string> properties)
        {
            throw new NotImplementedException();
        }

        public Message(Message message)
        {
            throw new NotImplementedException();
        }

        public byte[] ToByteArray()
        {
            throw new NotImplementedException();
        }
    }
}
