/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.remote;

import java.io.IOException;
import java.util.HashMap;

import org.junit.Test;

import com.microsoft.azure.gateway.messaging.Message;

import mockit.Expectations;
import mockit.Mocked;

public class BrokerProxyTest {

    @Mocked
    CommunicationEndpoint communicationEndpoint;

    @Test
    public void publishMessageSuccess() throws IOException, ConnectionException {
        final Message message = new Message("Test".getBytes(), new HashMap<String, String>());

        new Expectations() {
            {
                communicationEndpoint.sendMessage(message.toByteArray());
                times = 1;
            }
        };

        long moduleAddr = 0;
        new BrokerProxy(communicationEndpoint).publishMessage(message, moduleAddr);
    }
    
    @Test(expected=IOException.class)
    public void publishMessageShouldThrowIfEndpointThrows() throws ConnectionException, IOException {
        final Message message = new Message("Test".getBytes(), new HashMap<String, String>());

        new Expectations() {
            {
                communicationEndpoint.sendMessage(message.toByteArray()); result = new ConnectionException();
                times = 1;

            }
        };

        long moduleAddr = 0;
        new BrokerProxy(communicationEndpoint).publishMessage(message, moduleAddr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void publishMessageShouldThrowIfEndpointIsNull() throws ConnectionException, IOException {
        final Message message = new Message("Test".getBytes(), new HashMap<String, String>());

        long moduleAddr = 0;
        new BrokerProxy(null).publishMessage(message, moduleAddr);
    }

}
