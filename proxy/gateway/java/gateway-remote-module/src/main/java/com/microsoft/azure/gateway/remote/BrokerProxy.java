/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.remote;

import java.io.IOException;

import com.microsoft.azure.gateway.core.Broker;
import com.microsoft.azure.gateway.messaging.Message;

/**
 * A Proxy for the Broker that can send messages to the remote Gateway
 * 
 */
class BrokerProxy extends Broker {
    private final CommunicationEndpoint endpoint;

    public BrokerProxy(CommunicationEndpoint dataEndpoint) {
        if (dataEndpoint == null)
            throw new IllegalArgumentException("Communication endpoint was not initialized.");

        this.endpoint = dataEndpoint;
    }

    /**
     * Sends messages to the remote Gateway via the communication endpoint. This
     * method blocks until the Gateway receives the message.
     * 
     * @return 0 on success
     * @throws IOException
     *             if the message serialization fails or if it could not send
     */
    @Override
    public int publishMessage(Message message, long moduleAddr) throws IOException {
        try {
            this.endpoint.sendMessage(message.toByteArray());
        } catch (ConnectionException e) {
            throw new IOException(e);
        }
        return 0;
    }
}
