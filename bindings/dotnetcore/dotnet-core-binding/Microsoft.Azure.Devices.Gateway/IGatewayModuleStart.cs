// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Devices.Gateway
{
    /// <summary> Optional Start Interface to be implemented by the .NET Module </summary>
    public interface IGatewayModuleStart
    {
        /// <summary>
        ///     Informs module the gateway is ready to send and receive messages.
        /// </summary>
        /// <returns></returns>
        void Start();
    }
}