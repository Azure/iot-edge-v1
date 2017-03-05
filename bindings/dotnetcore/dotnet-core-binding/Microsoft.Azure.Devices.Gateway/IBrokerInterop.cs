using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;

namespace Microsoft.Azure.Devices.Gateway
{
    interface IBrokerInterop
    {
        bool PublishMessage(IntPtr broker, IntPtr sourceModule, byte[] message);
    }
}
