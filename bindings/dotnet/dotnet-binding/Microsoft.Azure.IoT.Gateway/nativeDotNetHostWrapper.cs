// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;

namespace Microsoft.Azure.IoT.Gateway
{
    /// <summary>
    ///     Wrapper Used for Native/Managed Interop.
    /// </summary>
    public class NativeDotNetHostWrapper
    {
        /// <summary>
        ///      Module_DotNetHost wrapper for publishing a message.
        /// </summary>
        /// <param name="messageBus">Address to the message Bus.</param>
        /// <param name="sourceModule">Address from the Source Module (Native created)</param>
        /// <param name="source">Message Content in Byte Array.</param>
        /// <param name="size">Size of data that is going to be published.</param>
        /// <returns></returns>
        [DllImport(@"dotnet.dll", EntryPoint = "Module_DotNetHost_PublishMessage", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool Module_DotNetHost_PublishMessage(IntPtr messageBus, IntPtr sourceModule, byte[] source, Int32 size);

        /// <summary>
        ///    Publishes a message to a given Message Bus.
        /// </summary>
        /// <param name="messageBus">Address to the message Bus.</param>
        /// <param name="sourceModule">Address from the Source Module (Native created)</param>
        /// <param name="source">Message Content in Byte Array.</param>
        /// <returns></returns>
        virtual public bool PublishMessage(IntPtr messageBus, IntPtr sourceModule, byte[] source)
        {
            return Module_DotNetHost_PublishMessage(messageBus, sourceModule, source, source.Length);
        }
    }
}
