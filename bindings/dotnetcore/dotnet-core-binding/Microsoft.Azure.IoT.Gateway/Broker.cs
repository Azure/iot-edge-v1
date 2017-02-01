// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Microsoft.Azure.IoT.Gateway
{
    /// <summary> Object that represents the message broker, to which messsages will be published. </summary>
    public class Broker
    {
        private IntPtr brokerHandle;

        private IntPtr moduleHandle;

        private BrokerInterop brokerInterop;

#if !DOXYGEN_SHOULD_SKIP_THIS
        /// <summary>
        ///   Constructor for a Broker that receives a reference to a broker, module and a brokerInterop. NativeWrapper is used for Unit Tests.
        /// </summary>
        /// <param name="broker">A reference to an existing broker.</param>
        /// <param name="module">A reference to an existing module.</param>
        /// <param name="nativeWrapper">A Native DotNet Core Host Wrapper used for Mocking purposes on Unit Tests.</param>
        public Broker(IntPtr broker, IntPtr module, BrokerInterop brokerInterop)
        {
            /* Codes_SRS_DOTNET_CORE_BROKER_04_001: [ If broker is <= 0, Broker constructor shall throw a new ArgumentException ] */
            if (broker == IntPtr.Zero)
            {
                throw new ArgumentOutOfRangeException("Invalid broker");
            }
            /* Codes_SRS_DOTNET_CORE_BROKER_04_007: [ If module is <= 0, Broker constructor shall throw a new ArgumentException ] */
            else if (module == IntPtr.Zero)
            {
                throw new ArgumentOutOfRangeException("Invalid source module");
            }
            else
            {
                /* Codes_SRS_DOTNET_CORE_BROKER_04_002: [ If broker and module are greater than 0, Broker constructor shall save this value and succeed. ] */
                this.brokerHandle = broker;
                this.moduleHandle = module;
                this.brokerInterop = brokerInterop;
            }
        }

        /// <summary>
        ///   Broker Default Contructor that received a long for reference to an existing broker and another long to a reference to a module.
        /// </summary>
        /// <param name="broker">A reference to an existing broker.</param>
        /// <param name="module">A reference to an existing module.</param>
        public Broker(IntPtr broker, IntPtr module) : this(broker, module, new BrokerInterop())
        {

        }

#endif // DOXYGEN_SHOULD_SKIP_THIS

        /// <summary>
        ///     Publish a message to the gateway message broker. 
        /// </summary>
        /// <param name="message">Object representing the message to be published to the broker.</param>
        /// <returns></returns>
        public void Publish(Message message)
        {
            /* Codes_SRS_DOTNET_CORE_BROKER_04_004: [ Publish shall not catch exception thrown by ToByteArray. ] */
            /* Codes_SRS_DOTNET_CORE_BROKER_04_003: [ Publish shall call the Message.ToByteArray() method to get the Message object translated to byte array. ] */
            byte[] messageObject = message.ToByteArray();

            /* Codes_SRS_DOTNET_CORE_BROKER_04_005: [ Publish shall call the native method Module_DotNetCoreHost_PublishMessage passing the broker and module value saved by it's constructor, the byte[] got from Message and the size of the byte array. ] */
            /* Codes_SRS_DOTNET_CORE_BROKER_04_006: [ If Module_DotNetCoreHost_PublishMessage fails, Publish shall thrown an Application Exception with message saying that Broker Publish failed. ] */
            try
            {
                this.brokerInterop.PublishMessage(this.brokerHandle, this.moduleHandle, messageObject);
            }
            catch(Exception e)
            {
                throw new Exception("Failed to publish message.", e);
            }
        }

    }
}
 
 