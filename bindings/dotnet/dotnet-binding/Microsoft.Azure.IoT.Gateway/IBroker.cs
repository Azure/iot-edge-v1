// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.IoT.Gateway
{
    /// <summary> Message broker interface for unit testing. </summary>
    public interface IBroker
    {
        void Publish(Message message);
    }
}
