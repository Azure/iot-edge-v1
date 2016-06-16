using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Microsoft.Azure.IoT.Gateway
{
    public interface InativeDotNetHostWrapper
    {
        bool PublishMessage(IntPtr messageBus, IntPtr sourceModule, byte[] source, Int32 size);
    }
}
