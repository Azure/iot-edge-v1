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
     * function to publish the provided {@link Message} onto the MessageBus.
     *
     * @param busAddr The address of the pointer to the native MessageBus.
     * @param message The serialized {@link Message} to be published.
     * @return 0 on success, non-zero otherwise.
     */
    private native int publishMessage(long busAddr, byte[] message);

    private long _busAddr;

    public MessageBus(long addr){
        this._busAddr = addr;
    }

    public int publishMessage(Message message) throws IOException {
        return this.publishMessage(this._busAddr, message.toByteArray());
    }

    public long getAddress(){
        return this._busAddr;
    }
}
