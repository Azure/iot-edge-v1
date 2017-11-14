/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.remote;

import java.nio.ByteBuffer;

/**
 * Communication strategy for control messages to/from Gateway.
 *
 */
class CommunicationControlStrategy implements CommunicationStrategy {

    /**
     * Retrieves a specific endpoint type for the control channel
     *
     * @return Endpoint type for control channel
     */
    @Override
    public int getEndpointType() {
        return NanomsgLibrary.NN_PAIR;
    }

    /**
     * Deserialize the message bytes to a specific control message
     */
    @Override
    public RemoteMessage deserializeMessage(ByteBuffer messageBuffer, byte version)
            throws MessageDeserializationException {
        MessageDeserializer deserializer = new MessageDeserializer();
        return deserializer.deserialize(messageBuffer, version);
    }

    /**
     * Constructs the connection uri from control identifier
     *
     * @return The constructed connection uri
     */
    @Override
    public String getEndpointUri(String identifier) {
        return String.format("ipc://%s", identifier);
    }
}
