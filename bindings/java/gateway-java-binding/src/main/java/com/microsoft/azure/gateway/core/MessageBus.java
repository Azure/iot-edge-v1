/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.core;

import com.microsoft.azure.gateway.messaging.Message;

public class MessageBus {

    //Loads the native library
    static {
        System.loadLibrary("java_binding");
    }

    //Private Native Methods
    /**
     * Native MessageBus_Publish function. When this method is called, it will call into the native MessageBus_Publish
     * function to publish the provided {@link Message} onto the MessageBus.
     *
     * @param addr The address of the pointer to the native MessageBus.
     * @param message The serialized {@link Message} to be published.
     * @return 0 on success, non-zero otherwise.
     */
    private native int publishMessage(long addr, byte[] message);
}
