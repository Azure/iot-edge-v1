/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.remote;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;

import org.junit.BeforeClass;
import org.junit.Test;

import mockit.Mock;
import mockit.MockUp;
import mockit.Mocked;
import mockit.Verifications;

public class CommunicationEndpointTest {

    private static int sentBytes = 0;
    private static int errorNo = 1;
    private static byte[] messageBuffer = new byte[] {};
    private static int endpointId = 1;
    private static int socket = 0;
    private final String identifier = "test";

    @Mocked
    CommunicationStrategy strategy;

    @BeforeClass
    public static void applySharedMockups() {
        new MockUp<NanomsgLibrary>() {
            @Mock
            private void loadNativeLibrary() {

            }

            @Mock
            private Map<String, Integer> getSymbols() {
                Map<String, Integer> map = new HashMap<String, Integer>();
                map.put("NN_PAIR", 16);
                map.put("NN_DONTWAIT", 1);
                map.put("EAGAIN", 11);
                map.put("AF_SP", 1);

                return map;
            }

            @Mock
            public int nn_socket(int i, int j) {
                return socket;
            }

            @Mock
            public int nn_errno() {
                return errorNo;
            }

            @Mock
            public String nn_strerror(int errnum) {
                return "Error " + errnum;
            }

            @Mock
            int nn_bind(int socket, String uri) {
                return endpointId;
            }

            @Mock
            public int nn_shutdown(int socket, int how) {
                return 0;
            }

            @Mock
            public int nn_close(int socket) {
                return 0;
            }
            
            @Mock
            public byte[] nn_recv(int socket, int flags) {
                return messageBuffer;
            }

            @Mock
            public int nn_send(int socket, byte[] str, int flags) {
                return sentBytes;
            }
        };
    }

    @Test(expected = IllegalArgumentException.class)
    public void constructorShouldThrowIfNullIdentifier() {
        new NanomsgCommunicationEndpoint(null, strategy);
    }

    @Test(expected = IllegalArgumentException.class)
    public void constructorShouldThrowIfNullCommunicationStrategy() {
        new NanomsgCommunicationEndpoint(identifier, null);
    }

    @Test
    public void connectSuccessWithControlChannel() throws ConnectionException {
        socket = 0;
        endpointId = 1;

        CommunicationEndpoint endpoint = new NanomsgCommunicationEndpoint(identifier, strategy);
        endpoint.connect();

        new Verifications() {
            {
                strategy.getEndpointUri(anyString);
                times = 1;
                strategy.getEndpointType();
                times = 1;
            }
        };
    }

    @Test(expected = ConnectionException.class)
    public void connectShouldThrowIfSocketFails() throws ConnectionException {
        socket = -1;

        CommunicationEndpoint endpoint = new NanomsgCommunicationEndpoint(identifier, strategy);
        endpoint.connect();
    }

    @Test(expected = ConnectionException.class)
    public void connectShouldThrowIfBindFails() throws ConnectionException {
        socket = 0;
        endpointId = -1;

        CommunicationEndpoint endpoint = new NanomsgCommunicationEndpoint(identifier, strategy);
        endpoint.connect();
    }

    @Test
    public void receiveMessageSuccessWithControlChannel() throws ConnectionException, MessageDeserializationException {
        final String identifier = "test";

        messageBuffer = new byte[0];
        CommunicationEndpoint endpoint = new NanomsgCommunicationEndpoint(identifier, strategy);
        endpoint.receiveMessage();

        new Verifications() {
            {
                strategy.deserializeMessage(ByteBuffer.wrap(messageBuffer), anyByte);
                times = 1;
            }
        };
    }

    @Test
    public void receiveMessageSuccessWithControlChannelNoMessage()
            throws ConnectionException, MessageDeserializationException {
        final String identifier = "test";

        messageBuffer = null;
        errorNo = 11;
        CommunicationEndpoint endpoint = new NanomsgCommunicationEndpoint(identifier, strategy);
        endpoint.receiveMessage();
    }

    @Test(expected = ConnectionException.class)
    public void receiveMessageShouldThrowIfNanofails() throws ConnectionException, MessageDeserializationException {
        final String identifier = "test";

        messageBuffer = null;

        CommunicationEndpoint endpoint = new NanomsgCommunicationEndpoint(identifier, strategy);
        endpoint.receiveMessage();
    }

    @Test
    public void disconnectMessageSuccess() throws ConnectionException, MessageDeserializationException {
        final String identifier = "test";
        socket = 0;
        endpointId = 1;

        CommunicationEndpoint endpoint = new NanomsgCommunicationEndpoint(identifier, strategy);
        endpoint.disconnect();
    }

    @Test
    public void sendMessageSuccess() throws ConnectionException, MessageDeserializationException {
        final String identifier = "test";
        messageBuffer = new byte[0];
        sentBytes = 0;

        CommunicationEndpoint endpoint = new NanomsgCommunicationEndpoint(identifier, strategy);
        endpoint.sendMessage(messageBuffer);
    }

    @Test(expected = ConnectionException.class)
    public void sendMessageShouldThrowIfMessageNotSent() throws ConnectionException, MessageDeserializationException {
        final String identifier = "test";
        messageBuffer = new byte[0];
        sentBytes = -1;

        CommunicationEndpoint endpoint = new NanomsgCommunicationEndpoint(identifier, strategy);
        endpoint.sendMessage(messageBuffer);
    }

    @Test
    public void sendMessageAsyncSuccess() throws ConnectionException, MessageDeserializationException {
        final String identifier = "test";
        messageBuffer = new byte[0];
        sentBytes = 0;

        CommunicationEndpoint endpoint = new NanomsgCommunicationEndpoint(identifier, strategy);
        boolean sent = endpoint.sendMessageNoWait(messageBuffer);

        assertTrue(sent);
    }

    @Test
    public void sendMessageAsyncSuccessIfNoReceiver() throws ConnectionException, MessageDeserializationException {
        final String identifier = "test";
        messageBuffer = new byte[0];
        sentBytes = -1;
        errorNo = 11;

        CommunicationEndpoint endpoint = new NanomsgCommunicationEndpoint(identifier, strategy);
        boolean sent = endpoint.sendMessageNoWait(messageBuffer);
        assertFalse(sent);
    }

    @Test(expected = ConnectionException.class)
    public void sendMessageAsyncShouldThrowIfMessageNotSent()
            throws ConnectionException, MessageDeserializationException {
        final String identifier = "test";
        messageBuffer = new byte[0];
        sentBytes = -1;
        errorNo = 22;

        CommunicationEndpoint endpoint = new NanomsgCommunicationEndpoint(identifier, strategy);
        endpoint.sendMessageNoWait(messageBuffer);
    }
}
