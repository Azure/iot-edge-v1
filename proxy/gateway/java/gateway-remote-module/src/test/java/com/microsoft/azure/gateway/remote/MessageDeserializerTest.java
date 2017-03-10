/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.remote;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.nio.ByteBuffer;

import org.junit.Test;

import mockit.Deencapsulation;

public class MessageDeserializerTest {

    private static final String MODULE_ARGS = "module args\0";
    private static final String DATA_MESSAGE_SOCKET_NAME = "data_message\0";
    private static final byte CREATE_MESSAGE_TYPE = (byte) RemoteMessageType.CREATE.getValue();
    private static final byte INVALID_MESSAGE_TYPE = (byte) 5;
    private static final byte VALID_MESSAGE_VERSION = CREATE_MESSAGE_TYPE;
    private static final byte VALID_HEADER2 = (byte) 0x6C;
    private static final byte VALID_HEADER1 = (byte) 0xA1;
    private static final byte INVALID_HEADER2 = (byte) 0x6A;
    private static final byte INVALID_HEADER1 = (byte) 0xA2;
    private static final byte VALID_URI_TYPE = (byte) 16;
    private static final int INVALID_URI_SIZE_TOO_SMALL = 5;
    private static final int INVALID_URI_SIZE_TOO_LARGE = 30;
    private static final byte MESSAGE_VERSION = 1;
    private static final byte INVALID_MESSAGE_VERSION = 2;
    private byte smallSize = 7;

    @Test
    public void deserializationShouldReturnCreateMessage() throws MessageDeserializationException {
        int size = 21 + DATA_MESSAGE_SOCKET_NAME.length() + MODULE_ARGS.length();
        ByteBuffer invalidSizeMessage = ByteBuffer.allocate(size);
        invalidSizeMessage.put(VALID_HEADER1);
        invalidSizeMessage.put(VALID_HEADER2);
        invalidSizeMessage.put(VALID_MESSAGE_VERSION);
        invalidSizeMessage.put(CREATE_MESSAGE_TYPE);
        invalidSizeMessage.putInt(size);
        invalidSizeMessage.put(VALID_MESSAGE_VERSION);
        invalidSizeMessage.put(VALID_URI_TYPE);
        invalidSizeMessage.putInt(DATA_MESSAGE_SOCKET_NAME.length());
        invalidSizeMessage.put(DATA_MESSAGE_SOCKET_NAME.getBytes());
        invalidSizeMessage.putInt(MODULE_ARGS.length());
        invalidSizeMessage.put(MODULE_ARGS.getBytes());

        MessageDeserializer deserializer = new MessageDeserializer();
        RemoteMessage message = deserializer.deserialize(invalidSizeMessage, MESSAGE_VERSION);
        assertTrue(message instanceof CreateMessage);
        CreateMessage createMessage = (CreateMessage) message;
        assertEquals(DATA_MESSAGE_SOCKET_NAME.trim(), createMessage.getDataEndpoint().getId());
        assertEquals(VALID_URI_TYPE, createMessage.getDataEndpoint().getType());
        assertEquals(VALID_MESSAGE_VERSION, createMessage.getVersion());
        assertEquals(MODULE_ARGS.trim(), createMessage.getArgs());
    }

    @Test
    public void deserializationShouldReturnStartMessage() throws MessageDeserializationException {
        int size = 8;
        ByteBuffer invalidSizeMessage = ByteBuffer.allocate(size);
        invalidSizeMessage.put(VALID_HEADER1);
        invalidSizeMessage.put(VALID_HEADER2);
        invalidSizeMessage.put(VALID_MESSAGE_VERSION);
        invalidSizeMessage.put((byte) RemoteMessageType.START.getValue());
        invalidSizeMessage.putInt(size);

        MessageDeserializer deserializer = new MessageDeserializer();
        ControlMessage message = (ControlMessage) deserializer.deserialize(invalidSizeMessage, MESSAGE_VERSION);
        assertEquals(RemoteMessageType.START, message.getMessageType());
    }

    @Test
    public void deserializationShouldReturnDestroyMessage() throws MessageDeserializationException {
        int size = 8;
        ByteBuffer invalidSizeMessage = ByteBuffer.allocate(size);
        invalidSizeMessage.put(VALID_HEADER1);
        invalidSizeMessage.put(VALID_HEADER2);
        invalidSizeMessage.put(VALID_MESSAGE_VERSION);
        invalidSizeMessage.put((byte) RemoteMessageType.DESTROY.getValue());
        invalidSizeMessage.putInt(size);

        MessageDeserializer deserializer = new MessageDeserializer();
        ControlMessage message = (ControlMessage) deserializer.deserialize(invalidSizeMessage, MESSAGE_VERSION);
        assertEquals(RemoteMessageType.DESTROY, message.getMessageType());
    }

    @Test
    public void deserializationShouldReturnControlMessageErrorIfMessageTypeIsError()
            throws MessageDeserializationException {
        int size = 8;
        ByteBuffer invalidSizeMessage = ByteBuffer.allocate(size);
        invalidSizeMessage.put(VALID_HEADER1);
        invalidSizeMessage.put(VALID_HEADER2);
        invalidSizeMessage.put(VALID_MESSAGE_VERSION);
        invalidSizeMessage.put((byte) RemoteMessageType.ERROR.getValue());
        invalidSizeMessage.putInt(size);

        MessageDeserializer deserializer = new MessageDeserializer();
        ControlMessage message = (ControlMessage) deserializer.deserialize(invalidSizeMessage, MESSAGE_VERSION);
        assertEquals(RemoteMessageType.ERROR, message.getMessageType());
    }

    @Test
    public void deserializationShouldThrowIfInvalidHeader() {
        ByteBuffer invalidHeader1Message = ByteBuffer.allocate(8);
        invalidHeader1Message.put(INVALID_HEADER1);
        invalidHeader1Message.put(INVALID_HEADER2);

        MessageDeserializer deserializer = new MessageDeserializer();
        try {
            deserializer.deserialize(invalidHeader1Message, MESSAGE_VERSION);
        } catch (MessageDeserializationException e) {
            assertEquals(e.getMessage(), "Invalid message header.");
        }

        ByteBuffer invalidHeader2Message = ByteBuffer.allocate(8);
        invalidHeader2Message.put(VALID_HEADER1);
        invalidHeader2Message.put(INVALID_HEADER2);

        try {
            deserializer.deserialize(invalidHeader2Message, MESSAGE_VERSION);
        } catch (MessageDeserializationException e) {
            assertEquals(e.getMessage(), "Invalid message header.");
        }
    }

    @Test
    public void deserializationShouldThrowIfInvalidMessageVersion() {
        ByteBuffer invalidSizeMessage = ByteBuffer.allocate(8);
        invalidSizeMessage.put(VALID_HEADER1);
        invalidSizeMessage.put(VALID_HEADER2);
        invalidSizeMessage.put(INVALID_MESSAGE_VERSION);

        MessageDeserializer deserializer = new MessageDeserializer();
        try {
            deserializer.deserialize(invalidSizeMessage, MESSAGE_VERSION);
        } catch (MessageDeserializationException e) {
            assertEquals(String.format("Message version %s can not be higher than configured version %s", INVALID_MESSAGE_VERSION, MESSAGE_VERSION), e.getMessage());
        }
    }

    @Test
    public void deserializationShouldThrowIfInvalidMessageType() {
        ByteBuffer invalidSizeMessage = ByteBuffer.allocate(8);
        invalidSizeMessage.put(VALID_HEADER1);
        invalidSizeMessage.put(VALID_HEADER2);
        invalidSizeMessage.put(VALID_MESSAGE_VERSION);
        invalidSizeMessage.put(INVALID_MESSAGE_TYPE);

        MessageDeserializer deserializer = new MessageDeserializer();
        try {
            deserializer.deserialize(invalidSizeMessage, MESSAGE_VERSION);
        } catch (MessageDeserializationException e) {
            assertEquals("Invalid message type.", e.getMessage());
        }
    }

    @Test
    public void deserializationShouldThrowIfHeaderIsMissing() {

        ByteBuffer invalidSizeMessage = ByteBuffer.allocate(1);
        invalidSizeMessage.put(VALID_HEADER1);

        MessageDeserializer deserializer = new MessageDeserializer();
        try {
            deserializer.deserialize(invalidSizeMessage, MESSAGE_VERSION);
        } catch (MessageDeserializationException e) {
            byte minSize = Deencapsulation.getField(MessageDeserializer.class, "BASE_MESSAGE_SIZE");
            assertEquals(String.format("Message size %s should be >= %s", invalidSizeMessage.limit(), minSize), e.getMessage());
        }

        invalidSizeMessage = ByteBuffer.allocate(0);

        deserializer = new MessageDeserializer();
        try {
            deserializer.deserialize(invalidSizeMessage, MESSAGE_VERSION);
        } catch (MessageDeserializationException e) {
            byte minSize = Deencapsulation.getField(MessageDeserializer.class, "BASE_MESSAGE_SIZE");
            assertEquals(String.format("Message size %s should be >= %s", invalidSizeMessage.limit(), minSize), e.getMessage());
        }
    }

    @Test
    public void deserializationShouldThrowIfInvalidMessageSize() {
        ByteBuffer invalidSizeMessage = ByteBuffer.allocate(7);
        invalidSizeMessage.put(VALID_HEADER1);
        invalidSizeMessage.put(VALID_HEADER2);
        invalidSizeMessage.put(VALID_MESSAGE_VERSION);
        invalidSizeMessage.put(CREATE_MESSAGE_TYPE);

        MessageDeserializer deserializer = new MessageDeserializer();
        try {
            deserializer.deserialize(invalidSizeMessage, MESSAGE_VERSION);
        } catch (MessageDeserializationException e) {
            byte minSize = Deencapsulation.getField(MessageDeserializer.class, "BASE_MESSAGE_SIZE");
            assertEquals(String.format("Message size %s should be >= %s", smallSize, minSize), e.getMessage());
        }

        invalidSizeMessage = ByteBuffer.allocate(8);
        invalidSizeMessage.put(VALID_HEADER1);
        invalidSizeMessage.put(VALID_HEADER2);
        invalidSizeMessage.put(VALID_MESSAGE_VERSION);
        invalidSizeMessage.put(CREATE_MESSAGE_TYPE);
        invalidSizeMessage.putInt(64);

        try {
            deserializer.deserialize(invalidSizeMessage, MESSAGE_VERSION);
        } catch (MessageDeserializationException e) {
            assertEquals(String.format("Message size in header %s is different that actual size %s", 64,
                    invalidSizeMessage.limit()), e.getMessage());
        }
    }

    @Test
    public void deserializationShouldThrowIfInvalidCreateSize() {
        ByteBuffer invalidSizeMessage = ByteBuffer.allocate(8);
        invalidSizeMessage.put(VALID_HEADER1);
        invalidSizeMessage.put(VALID_HEADER2);
        invalidSizeMessage.put(VALID_MESSAGE_VERSION);
        invalidSizeMessage.put(CREATE_MESSAGE_TYPE);
        invalidSizeMessage.putInt(8);

        MessageDeserializer deserializer = new MessageDeserializer();
        try {
            deserializer.deserialize(invalidSizeMessage, MESSAGE_VERSION);
        } catch (MessageDeserializationException e) {
            byte minSize = Deencapsulation.getField(MessageDeserializer.class, "BASE_CREATE_SIZE");
            assertEquals(String.format("Create message size %s should be >= %s", 8, minSize), e.getMessage());
        }
    }

    @Test
    public void deserializationShouldThrowIfInvalidUriSizeTooSmall() {
        int size = 17 + DATA_MESSAGE_SOCKET_NAME.length();
        ByteBuffer invalidSizeMessage = ByteBuffer.allocate(size);
        invalidSizeMessage.put(VALID_HEADER1);
        invalidSizeMessage.put(VALID_HEADER2);
        invalidSizeMessage.put(VALID_MESSAGE_VERSION);
        invalidSizeMessage.put(CREATE_MESSAGE_TYPE);
        invalidSizeMessage.putInt(size);
        invalidSizeMessage.put(VALID_MESSAGE_VERSION);
        invalidSizeMessage.put(VALID_URI_TYPE);
        invalidSizeMessage.putInt(INVALID_URI_SIZE_TOO_SMALL);
        invalidSizeMessage.put(DATA_MESSAGE_SOCKET_NAME.getBytes());

        MessageDeserializer deserializer = new MessageDeserializer();
        try {
            deserializer.deserialize(invalidSizeMessage, MESSAGE_VERSION);
        } catch (MessageDeserializationException e) {
            assertEquals("Can not deserialize string arguments.", e.getMessage());
        }
    }

    @Test
    public void deserializationShouldThrowIfInvalidUriSizeTooLarge() {
        int size = 17 + DATA_MESSAGE_SOCKET_NAME.length();
        ByteBuffer invalidSizeMessage = ByteBuffer.allocate(size);
        invalidSizeMessage.put(VALID_HEADER1);
        invalidSizeMessage.put(VALID_HEADER2);
        invalidSizeMessage.put(VALID_MESSAGE_VERSION);
        invalidSizeMessage.put(CREATE_MESSAGE_TYPE);
        invalidSizeMessage.putInt(size);
        invalidSizeMessage.put(VALID_MESSAGE_VERSION);
        invalidSizeMessage.put(VALID_URI_TYPE);
        invalidSizeMessage.putInt(INVALID_URI_SIZE_TOO_LARGE);
        invalidSizeMessage.put(DATA_MESSAGE_SOCKET_NAME.getBytes());

        MessageDeserializer deserializer = new MessageDeserializer();
        try {
            deserializer.deserialize(invalidSizeMessage, MESSAGE_VERSION);
        } catch (MessageDeserializationException e) {
            assertEquals("Can not deserialize string arguments.", e.getMessage());
        }
    }

    @Test
    public void deserializationShouldThrowIfInvalidArgsSizeTooLarge() {
        int size = 21 + DATA_MESSAGE_SOCKET_NAME.length() + MODULE_ARGS.length();
        ByteBuffer invalidSizeMessage = ByteBuffer.allocate(size);
        invalidSizeMessage.put(VALID_HEADER1);
        invalidSizeMessage.put(VALID_HEADER2);
        invalidSizeMessage.put(VALID_MESSAGE_VERSION);
        invalidSizeMessage.put(CREATE_MESSAGE_TYPE);
        invalidSizeMessage.putInt(size);
        invalidSizeMessage.put(VALID_MESSAGE_VERSION);
        invalidSizeMessage.put(VALID_URI_TYPE);
        invalidSizeMessage.putInt(DATA_MESSAGE_SOCKET_NAME.length());
        invalidSizeMessage.put(DATA_MESSAGE_SOCKET_NAME.getBytes());
        invalidSizeMessage.putInt(INVALID_URI_SIZE_TOO_LARGE);
        invalidSizeMessage.put(MODULE_ARGS.getBytes());

        MessageDeserializer deserializer = new MessageDeserializer();
        try {
            deserializer.deserialize(invalidSizeMessage, MESSAGE_VERSION);
        } catch (MessageDeserializationException e) {
            assertEquals("Can not deserialize string arguments.", e.getMessage());
        }
    }
}
