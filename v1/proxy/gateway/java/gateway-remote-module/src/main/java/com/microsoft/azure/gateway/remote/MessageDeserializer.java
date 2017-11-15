/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.remote;

import java.nio.ByteBuffer;

/**
 * Deserializer for a Gateway message. The messages from the Gateway are received as an array of bytes in a specific format. 
 * 
 */
class MessageDeserializer {

    // 0xA1 comes from (A)zure (I)oT
    private static final byte FIRST_MESSAGE_BYTE = (byte) 0xA1;
    // 0x6C comes from (G)ateway control message
    private static final byte SECOND_MESSAGE_BYTE = (byte) 0x6C;
    private static final byte BASE_MESSAGE_SIZE = 8;
    private static final byte BASE_CREATE_SIZE = BASE_MESSAGE_SIZE + 10;

    /**
     * Deserializes the message and constructs a {@link ControlMessage}.
     *
     * @param messageBuffer The message content
     * @param version The message version that should be parsed.
     *
     * @return
     * @throws MessageDeserializationException If the message is malformed.
     */
    public RemoteMessage deserialize(ByteBuffer messageBuffer, byte version) throws MessageDeserializationException {
        RemoteMessageType msgType = null;
        int totalSize = 0;
        messageBuffer.position(0);

        if (messageBuffer.limit() < BASE_MESSAGE_SIZE) {
            throw new MessageDeserializationException(
                    String.format("Message size %s should be >= %s", messageBuffer.limit(), BASE_MESSAGE_SIZE));
        }

        byte header1 = messageBuffer.get();
        byte header2 = messageBuffer.get();
        if (header1 == FIRST_MESSAGE_BYTE && header2 == SECOND_MESSAGE_BYTE) {
            byte messageVersion = messageBuffer.get();
            if (messageVersion > version)
                throw new MessageDeserializationException(String.format("Message version %s can not be higher than configured version %s", messageVersion, version));

            byte type = messageBuffer.get();
            msgType = RemoteMessageType.fromValue(type);
            if (msgType == null)
                throw new MessageDeserializationException("Invalid message type.");

            totalSize = messageBuffer.getInt();

            if (totalSize != messageBuffer.limit())
                throw new MessageDeserializationException(
                        String.format("Message size in header %s is different that actual size %s", totalSize,
                                messageBuffer.limit()));

        } else {
            throw new MessageDeserializationException("Invalid message header.");
        }

        switch (msgType) {
        case CREATE:
            return this.deserializeCreateMessage(messageBuffer, totalSize);
        case START:
            return this.deserializeStartMessage(messageBuffer);
        case DESTROY:
            return this.deserializeDestroyMessage(messageBuffer);
        default:
            return new ControlMessage(RemoteMessageType.ERROR);
        }
    }

    private RemoteMessage deserializeCreateMessage(ByteBuffer buffer, int totalSize)
            throws MessageDeserializationException {
        if (totalSize < BASE_CREATE_SIZE)
            throw new MessageDeserializationException(
                    String.format("Create message size %s should be >= %s", totalSize, BASE_CREATE_SIZE));

        CreateMessage message = null;
        byte version = buffer.get();

        byte uriType = buffer.get();
        int uriSize = buffer.getInt();

        String id = readNullTerminatedString(buffer, uriSize);
        DataEndpointConfig endpointConfig = new DataEndpointConfig(id, uriType);

        int argsSize = buffer.getInt();
        String moduleArgs = readNullTerminatedString(buffer, argsSize);

        message = new CreateMessage(endpointConfig, moduleArgs, version);

        return message;
    }

    private RemoteMessage deserializeDestroyMessage(ByteBuffer messageBuffer) {
        return new ControlMessage(RemoteMessageType.DESTROY);
    }

    private RemoteMessage deserializeStartMessage(ByteBuffer messageBuffer) {
        return new ControlMessage(RemoteMessageType.START);
    }

    private static String readNullTerminatedString(ByteBuffer bis, int size) throws MessageDeserializationException {
        byte[] result = new byte[size - 1];
        int index = 0;
        byte b = bis.get();

        while (b != '\0' && b != -1 && index < size - 1) {
            result[index] = b;
            index++;
            b = bis.get();
        }

        if (b != '\0' || index != size - 1)
            throw new MessageDeserializationException("Can not deserialize string arguments.");

        return new String(result);
    }

}
