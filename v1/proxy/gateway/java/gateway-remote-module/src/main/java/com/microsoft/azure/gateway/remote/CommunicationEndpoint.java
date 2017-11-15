/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.remote;

interface CommunicationEndpoint {

    byte getVersion();

    /**
     * Set message version
     *
     * @param version
     */
    void setVersion(byte version);

    /**
     * Creates a socket and connects to it.
     * 
     */
    void connect() throws ConnectionException;

    /**
     * Checks if there are new messages to receive. This method does not block,
     * if there is no message it returns immediately.
     * 
     * @return Deserialized message
     *
     * @throws ConnectionException
     *             If there is any error receiving the message from the Gateway
     * @throws MessageDeserializationException
     *             If the message is not in the expected format.
     */
    RemoteMessage receiveMessage() throws ConnectionException, MessageDeserializationException;

    void disconnect();

    void sendMessage(byte[] message) throws ConnectionException;

    boolean sendMessageNoWait(byte[] message) throws ConnectionException;

}