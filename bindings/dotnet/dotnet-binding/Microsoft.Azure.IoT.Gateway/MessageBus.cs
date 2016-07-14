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

        private NativeDotNetHostWrapper dotnetWrapper;

#if !DOXYGEN_SHOULD_SKIP_THIS
        public MessageBus(long msgBus, long module, NativeDotNetHostWrapper nativeWrapper)
        {
            /* Codes_SRS_DOTNET_MESSAGEBUS_04_001: [ If msgBus is <= 0, MessageBus constructor shall throw a new ArgumentException ] */
            if (msgBus <= 0)
            {
                throw new ArgumentOutOfRangeException("Invalid Msg Bus.");
            }
            /* Codes_SRS_DOTNET_MESSAGEBUS_04_007: [ If moduleHandle is <= 0, MessageBus constructor shall throw a new ArgumentException ] */
            else if (module <= 0)
            {
                throw new ArgumentOutOfRangeException("Invalid source Module Handle.");
            }
            else
            {
                /* Codes_SRS_DOTNET_MESSAGEBUS_04_002: [ If msgBus and moduleHandle is greater than 0, MessageBus constructor shall save this value and succeed. ] */
                this.msgBusHandle = msgBus;
                this.moduleHandle = module;
                this.dotnetWrapper = nativeWrapper;
            }
        }

        public MessageBus(long msgBus, long module) : this(msgBus, module, new NativeDotNetHostWrapper())
        {

        }

#endif // DOXYGEN_SHOULD_SKIP_THIS

        /// <summary>
        ///     Publish a message into the gateway message bus. 
        /// </summary>
        /// <param name="message">Object representing the message to be published into the bus.</param>
        /// <returns></returns>
        public void Publish(Message message)
        {
            /* Codes_SRS_DOTNET_MESSAGEBUS_04_004: [ Publish shall not catch exception thrown by ToByteArray. ] */
            /* Codes_SRS_DOTNET_MESSAGEBUS_04_003: [ Publish shall call the Message.ToByteArray() method to get the Message object translated to byte array. ] */
            byte[] messageObject = message.ToByteArray();

            /* Codes_SRS_DOTNET_MESSAGEBUS_04_005: [ Publish shall call the native method Module_DotNetHost_PublishMessage passing the msgBus and moduleHandle value saved by it's constructor, the byte[] got from Message and the size of the byte array. ] */
            /* Codes_SRS_DOTNET_MESSAGEBUS_04_006: [ If Module_DotNetHost_PublishMessage fails, Publish shall thrown an Application Exception with message saying that MessageBus Publish failed. ] */
            try
            {
                this.dotnetWrapper.PublishMessage((IntPtr)this.msgBusHandle, (IntPtr)this.moduleHandle, messageObject);
            }
            catch(Exception e)
            {
                throw new ApplicationException("Failed to Publish Message.", e);
            }
        }

    }
}
 
 