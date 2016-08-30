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
        /// <param name="broker">Handle to the message broker.</param>
        /// <param name="sourceModule">Handle to the (native) source module.</param>
        /// <param name="message">Message content as a byte array.</param>
        /// <param name="size">Size of the byte array.</param>
        /// <returns></returns>
        [DllImport(@"dotnet.dll", EntryPoint = "Module_DotNetHost_PublishMessage", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool Module_DotNetHost_PublishMessage(IntPtr broker, IntPtr sourceModule, byte[] message, Int32 size);

        /// <summary>
        ///    Publishes a message to a given message broker.
        /// </summary>
        /// <param name="broker">Handle to the message broker.</param>
        /// <param name="sourceModule">Handle to the (native) source module.</param>
        /// <param name="message">Message content as a byte array.</param>
        /// <returns></returns>
        virtual public bool PublishMessage(IntPtr broker, IntPtr sourceModule, byte[] message)
        {
            return Module_DotNetHost_PublishMessage(broker, sourceModule, message, message.Length);
        }
    }
}
