using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Microsoft.Azure.IoT.Gateway
{
    /// <summary> Interface to be implemented by a Message Type.</summary>
    public interface IMessage
    {
        byte[] ToByteArray();
    }
}
