// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Microsoft.Azure.IoT.Gateway
{
    /// <summary> Object that represents the bus, to where a messsage is going to be published </summary>
    public class MessageBus
    {
        private long msgBusHandle;

        private long moduleHandle;

        /// <summary>
        ///     Constructor for MessageBus. This is used by the Native level, the .NET User will receive an object of this. 
        /// </summary>
        /// <param name="msgBus">Adress of the native created msgBus, used internally.</param>
        /// /// <param name="module">Adress of the module to which Module Bus got created. This will be used by Message when published.</param>
        public MessageBus(long msgBus, long module)
        {
            this.msgBusHandle = msgBus;
            this.moduleHandle = module;
        }

        /// <summary>
        ///     Publish a message into the gateway message bus. 
        /// </summary>
        /// <param name="message">Object representing the message to be published into the bus.</param>
        /// <returns></returns>
        public void Publish(Message message)
        {
            byte[] messageObjetct = message.ToByteArray();

            nativeDotNetHostWrapper.dotnetHost_PublishMessage((IntPtr)this.msgBusHandle, messageObjetct, messageObjetct.Length);
        }

    }
}