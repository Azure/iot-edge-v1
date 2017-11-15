/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.core;

import com.microsoft.azure.gateway.messaging.Message;

import java.io.IOException;

public class Broker {

    private long brokerAddr;
    private LocalBroker localBroker;

    protected Broker() {
    }

    public Broker(long addr) {
        this.brokerAddr = addr;

        this.localBroker = new LocalBroker();
    }

    /**
     * Publishes the {@link Message} to the {@link Broker}.
     *
     * @see <a href=
     *      "https://github.com/Azure/azure-iot-gateway-sdk/blob/master/core/devdoc/message_broker_requirements.md"
     *      target="_top">Message broker documentation</a>
     *
     * @param moduleAddr
     *            The address of the pointer to the native module.
     * @param message
     *            The serialized {@link Message} to be published.
     * @return 0 on success, non-zero otherwise.
     * @throws IOException
     *             If the {@link Message} cannot be serialized.
     */
    public int publishMessage(Message message, long moduleAddr) throws IOException {
        return this.localBroker.publishMessage(this.brokerAddr, moduleAddr, message.toByteArray());
    }

    public long getAddress() {
        return this.brokerAddr;
    }
}
