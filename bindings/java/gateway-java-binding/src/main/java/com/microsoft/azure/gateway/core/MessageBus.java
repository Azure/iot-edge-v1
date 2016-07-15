/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.core;

import com.microsoft.azure.gateway.messaging.Message;

import java.io.IOException;

public class MessageBus {

    //Loads the native library
    static {
        System.loadLibrary("java_module_host");
    }

    //Private Native Methods
    /**
     * Native MessageBus_Publish function. When this method is called, it will call into the native MessageBus_Publish
     * function to publish the provided {@link Message} onto the native MessageBus.
     *
     * @see <a href="https://github.com/Azure/azure-iot-gateway-sdk/blob/develop/core/devdoc/message_bus_requirements.md" target="_top">Message Bus Documentation</a>
     *
     * @param busAddr The address of the pointer to the native MessageBus.
     * @param moduleAddr The address of the pointer to the native module.
     * @param message The serialized {@link Message} to be published.
     * @return 0 on success, non-zero otherwise.
     */
    private native int publishMessage(long busAddr, long moduleAddr, byte[] message);

    private long _busAddr;

    public MessageBus(long addr){
        this._busAddr = addr;
    }

    /**
     * Publishes the {@link Message} onto to the {@link MessageBus}.
     *
     * @see <a href="https://github.com/Azure/azure-iot-gateway-sdk/blob/develop/core/devdoc/message_bus_requirements.md" target="_top">Message Bus Documentation</a>
     *
     * @param moduleAddr The address of the pointer to the native module.
     * @param message The serialized {@link Message} to be published.
     * @return 0 on success, non-zero otherwise.
     * @throws IOException If the {@link Message} cannot be serialized.
     */
    public int publishMessage(Message message, long moduleAddr) throws IOException {
        return this.publishMessage(this._busAddr, moduleAddr, message.toByteArray());
    }

    public long getAddress(){
        return this._busAddr;
    }
}
