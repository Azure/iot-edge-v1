using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Microsoft.Azure.IoT.Gateway
{
    public interface INativeDotNetHostWrapper
    {
        bool PublishMessage(IntPtr messageBus, IntPtr sourceModule, byte[] source);
    }
}
