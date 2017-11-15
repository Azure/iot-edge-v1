/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.remote;

import java.nio.ByteBuffer;

/**
 * An endpoint that is used to communicate with the remote Gateway which uses
 * nanomsg to send and receive messages.
 *
 */
class NanomsgCommunicationEndpoint implements CommunicationEndpoint {

    private final String uri;
    private final NanomsgLibrary nano;
    private final CommunicationStrategy communicationStrategy;
    private byte version;
    private int socket;
    private int endpointId;

    public NanomsgCommunicationEndpoint(String identifier, CommunicationStrategy communicationStrategy) {
        if (identifier == null)
            throw new IllegalArgumentException("Identifier can not be null");
        if (communicationStrategy == null)
            throw new IllegalArgumentException("Communication strategy can not be null");

        this.communicationStrategy = communicationStrategy;
        this.nano = new NanomsgLibrary();
        this.uri = communicationStrategy.getEndpointUri(identifier);
    }

    /* (non-Javadoc)
     * @see com.microsoft.azure.gateway.remote.CommunicationEndpoint#getVersion()
     */
    @Override
    public byte getVersion() {
        return version;
    }

    /* (non-Javadoc)
     * @see com.microsoft.azure.gateway.remote.CommunicationEndpoint#setVersion(byte)
     */
    @Override
    public void setVersion(byte version) {
        this.version = version;
    }

    /* (non-Javadoc)
     * @see com.microsoft.azure.gateway.remote.CommunicationEndpoint#connect()
     */
    @Override
    public void connect() throws ConnectionException {
        this.createSocket();
        this.createEndpoint();
    }

    /* (non-Javadoc)
     * @see com.microsoft.azure.gateway.remote.CommunicationEndpoint#receiveMessage()
     */
    @Override
    public RemoteMessage receiveMessage() throws ConnectionException, MessageDeserializationException {

        byte[] messageBuffer = this.nano.receiveMessageNoWait(this.socket);
        if (messageBuffer == null) {
            return null;
        }
        return this.communicationStrategy.deserializeMessage(ByteBuffer.wrap(messageBuffer), version);
    }

    /* (non-Javadoc)
     * @see com.microsoft.azure.gateway.remote.CommunicationEndpoint#disconnect()
     */
    @Override
    public void disconnect() {
        this.nano.shutdown(socket, endpointId);
        this.nano.closeSocket(socket);
    }

    /* (non-Javadoc)
     * @see com.microsoft.azure.gateway.remote.CommunicationEndpoint#sendMessage(byte[])
     */
    @Override
    public void sendMessage(byte[] message) throws ConnectionException {
        this.nano.sendMessage(socket, message);
    }

    /* (non-Javadoc)
     * @see com.microsoft.azure.gateway.remote.CommunicationEndpoint#sendMessageNoWait(byte[])
     */
    @Override
    public boolean sendMessageNoWait(byte[] message) throws ConnectionException {
        return this.nano.sendMessageAsync(this.socket, message);
    }

    private void createSocket() throws ConnectionException {
        this.socket = this.nano.createSocket(this.communicationStrategy.getEndpointType());
    }

    private void createEndpoint() throws ConnectionException {
        this.endpointId = this.nano.bind(this.socket, this.uri);
    }
}
