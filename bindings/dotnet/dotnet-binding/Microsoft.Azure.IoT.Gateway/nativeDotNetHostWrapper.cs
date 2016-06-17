// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;

namespace Microsoft.Azure.IoT.Gateway
{
    class NativeDotNetHostWrapper : INativeDotNetHostWrapper
    {
        [DllImport(@"dotnet.dll", EntryPoint = "Module_DotNetHost_PublishMessage", CallingConvention = CallingConvention.Cdecl)]
        static extern bool Module_DotNetHost_PublishMessage(IntPtr messageBus, IntPtr sourceModule, byte[] source, Int32 size);

        public bool PublishMessage(IntPtr messageBus, IntPtr sourceModule, byte[] source)
        {
            return Module_DotNetHost_PublishMessage(messageBus, sourceModule, source, source.Length);
        }
    }
}
