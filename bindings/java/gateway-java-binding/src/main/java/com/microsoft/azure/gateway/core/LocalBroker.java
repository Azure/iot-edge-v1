/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.core;

import com.microsoft.azure.gateway.messaging.Message;

class LocalBroker {

    // Loads the native library
    static {
        System.loadLibrary("java_module_host");
    }
    
  //Private Native Methods
    /**
     * Native Broker_Publish function. When this method is called, it will call into the native Broker_Publish
     * function to publish the provided {@link Message} to the native Broker.
     *
     * @see <a href="https://github.com/Azure/azure-iot-gateway-sdk/blob/master/core/devdoc/message_broker_requirements.md" target="_top">Message broker documentation</a>
     *
     * @param brokerAddr The address of the pointer to the native Broker.
     * @param moduleAddr The address of the pointer to the native module.
     * @param message The serialized {@link Message} to be published.
     * @return 0 on success, non-zero otherwise.
     */
    native int publishMessage(long brokerAddr, long moduleAddr, byte[] message);
}
