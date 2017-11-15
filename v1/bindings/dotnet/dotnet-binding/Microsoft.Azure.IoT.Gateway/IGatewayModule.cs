// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Microsoft.Azure.IoT.Gateway
{
    /// <summary> Interface to be implemented by the .NET Module </summary>
    public interface IGatewayModule
    {
        /// <summary>
        ///     Creates a module using the specified configuration connecting to the specified message broker.
        /// </summary>
        /// <param name="broker">The broker to which this module will connect.</param>
        /// <param name="configuration">A byte[] with user-defined configuration for this module. This parameter shall be enconded to a UTF-8 String.</param>
        /// <returns></returns>
        void Create(Broker broker, byte[] configuration);

        /// <summary>
        ///     Disposes of the resources allocated by/for this module.
        /// </summary>
        /// <returns></returns>
        void Destroy();

        /// <summary>
        ///     The module's callback function that is called upon message receipt.
        /// </summary>
        /// <param name="received_message">The message being sent to the module.</param>
        /// <returns></returns>                
        void Receive(Message received_message);
    }
}
