/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.remote;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import org.junit.BeforeClass;
import org.junit.Test;

import com.microsoft.azure.gateway.core.Broker;
import com.microsoft.azure.gateway.remote.ProxyGateway.MessageListener;

import mockit.Expectations;
import mockit.Mocked;
import mockit.Verifications;

public class ProxyGatewayTest {

    private final static String identifier = "test";
    private static byte version = 1;
    private static ModuleConfiguration config;
    private final String dataSocketId = "data-test";
    private final String args = "";
    private final DataEndpointConfig endpointsConfig = new DataEndpointConfig(dataSocketId, 1);
    private final CreateMessage createMessage = new CreateMessage(endpointsConfig, args, version);
    private final ControlMessage startMessage = new ControlMessage(RemoteMessageType.START);
    private final ControlMessage destroyMessage = new ControlMessage(RemoteMessageType.DESTROY);
    private final byte[] content = new byte[] { (byte) 0xA1, (byte) 0x60 };
    private final DataMessage dataMessage = new DataMessage(content);

    @BeforeClass
    public static void setUp() {
        ModuleConfiguration.Builder configBuilder = new ModuleConfiguration.Builder();
        configBuilder.setIdentifier(identifier);
        configBuilder.setModuleClass(TestModuleImplementsInterface.class);
        configBuilder.setModuleVersion(version);

        config = configBuilder.build();
    }

    // Tests_SRS_JAVA_PROXY_GATEWAY_24_001: [ If `config` is `null` the constructor shall throw an IllegalArgumentException. ]
    @Test(expected = IllegalArgumentException.class)
    public void constructorShouldThrowIfInvalidArguments() {
        new ProxyGateway(null);
    }

    // Tests_SRS_JAVA_PROXY_GATEWAY_24_002: [ The constructor shall save `config` for later use. ]
    // Tests_SRS_JAVA_PROXY_GATEWAY_24_003: [ If already attached the function shall do nothing and return. ]
    @Test
    public void attachSuccessIfAlreadyAttached(@Mocked final NanomsgCommunicationEndpoint controlEndpoint)
            throws ConnectionException, MessageDeserializationException {
        
        final ProxyGateway proxy = new ProxyGateway(config);

        new Expectations(ProxyGateway.class) {
            {
                proxy.startListening();
            }
        };

        proxy.attach();
        proxy.attach();

        new Verifications() {
            {
                controlEndpoint.connect();
                times = 1;
            }
        };
        assertTrue(proxy.isAttached());
    }

    // Tests_SRS_JAVA_PROXY_GATEWAY_24_004: [ It shall instantiate the messages listener task.  ]
    @Test
    public void attachShouldInstantiateMessagesListener(@Mocked final NanomsgCommunicationEndpoint controlEndpoint)
            throws ConnectionException, MessageDeserializationException {

        final ProxyGateway proxy = new ProxyGateway(config);

        new Expectations(ProxyGateway.class) {
            {
                proxy.startListening();
            }
        };
        new Expectations() {
            {
                controlEndpoint.connect();
            }
        };

        proxy.attach();

        new Verifications() {
            {
                controlEndpoint.connect();
                times = 1;
            }
        };

        assertNotNull(proxy.getReceiveMessageListener());
        assertTrue(proxy.isAttached());
    }
    
    // Tests_SRS_JAVA_PROXY_GATEWAY_24_005: [ It shall instantiate a single-threaded executor that can schedule the task to execute periodically. ]
    @Test
    public void attachShouldInstantiateExecutor(@Mocked final NanomsgCommunicationEndpoint controlEndpoint)
            throws ConnectionException, MessageDeserializationException {

        final ProxyGateway proxy = new ProxyGateway(config);

        new Expectations(ProxyGateway.class) {
            {
                proxy.startListening();
            }
        };
        new Expectations() {
            {
                controlEndpoint.connect();
            }
        };

        proxy.attach();

        new Verifications() {
            {
                controlEndpoint.connect();
                times = 1;
            }
        };

        assertNotNull(proxy.getExecutor());
        assertTrue(proxy.isAttached());
    }
    
    // Tests_SRS_JAVA_PROXY_GATEWAY_24_006: [ It shall start executing the periodic task of listening for messages from the Gateway. ]
    @Test
    public void attachShouldCallStartListening(@Mocked final NanomsgCommunicationEndpoint controlEndpoint)
            throws ConnectionException, MessageDeserializationException {

        final ProxyGateway proxy = new ProxyGateway(config);

        new Expectations(ProxyGateway.class) {
            {
                proxy.startListening();
            }
        };

        proxy.attach();

        new Verifications() {
            {
                controlEndpoint.connect();
                times = 1;
                proxy.startListening();
                times = 1;
            }
        };
        
        assertFalse(proxy.getExecutor().isShutdown());
        assertFalse(proxy.getExecutor().isTerminated());
        assertTrue(proxy.isAttached());
    }

    // Tests_SRS_JAVA_PROXY_GATEWAY_24_007: [ *Message Listener task* - It shall create a new control channel with the Gateway. ]
    // Tests_SRS_JAVA_PROXY_GATEWAY_24_008: [ *Message Listener task* - It shall connect to the control channel. ]
    // Tests_SRS_JAVA_PROXY_GATEWAY_24_009: [ *Message Listener task* - If the connection with the control channel fails, it shall throw ConnectionException. ]
    @Test(expected = ConnectionException.class)
    public void attachShouldThrowIfConnectFails(@Mocked final NanomsgCommunicationEndpoint controlEndpoint)
            throws ConnectionException, MessageDeserializationException {

        final ProxyGateway proxy = new ProxyGateway(config);

        new Expectations() {
            {
                controlEndpoint.connect();
                result = new ConnectionException();
            }
        };

        proxy.attach();

        new Verifications() {
            {
                controlEndpoint.connect();
                times = 1;
            }
        };
        assertFalse(proxy.isAttached());
    }
    
    // Tests_SRS_JAVA_PROXY_GATEWAY_24_010: [ *Message Listener task* - It shall poll the gateway control channel for new messages. ]
    // Tests_SRS_JAVA_PROXY_GATEWAY_24_011: [ *Message Listener task* - If no message is available the listener shall do nothing. ]
    @Test
    public void attachNoOpIfNoMessageReceived(@Mocked final NanomsgCommunicationEndpoint controlEndpoint,
            @Mocked final NanomsgCommunicationEndpoint dataEndpoint, @Mocked final TestModuleImplementsInterface module)
            throws ConnectionException, MessageDeserializationException {

        final ProxyGateway proxy = new ProxyGateway(config);

        new Expectations(ProxyGateway.class) {
            {
                proxy.startListening();
            }
        };
        new Expectations() {
            {
                new NanomsgCommunicationEndpoint(config.getIdentifier(), (CommunicationControlStrategy) any);
                result = controlEndpoint;

                controlEndpoint.connect();
                controlEndpoint.receiveMessage();
                result = null;
            }
        };

        proxy.attach();
        Runnable receiveMessage = proxy.getReceiveMessageListener();
        receiveMessage.run();

        new Verifications() {
            {
                controlEndpoint.connect();
                times = 1;
            }
        };

        assertTrue(proxy.isAttached());
    }

    // Tests_SRS_JAVA_PROXY_GATEWAY_24_010: [ *Message Listener task* - It shall poll the gateway control channel for new messages. ]
    // Tests_SRS_JAVA_PROXY_GATEWAY_24_014: [ *Message Listener task* - If the message type is CREATE, it shall process the create message ]
    // Tests_SRS_JAVA_PROXY_GATEWAY_24_015: [ *Message Listener task - Create message* - Create message processing shall create the data message channel and connect to it. ]
    // Tests_SRS_JAVA_PROXY_GATEWAY_24_018: [ *Message Listener task - Create message* - Create message processing shall create a module instance and call `create` method. ]
    @Test
    public void attachShouldCreateModuleInstanceNoArgsConstructor(@Mocked final NanomsgCommunicationEndpoint controlEndpoint,
            @Mocked final NanomsgCommunicationEndpoint dataEndpoint, @Mocked final TestModuleImplementsInterface module)
            throws ConnectionException, MessageDeserializationException {

        final ProxyGateway proxy = new ProxyGateway(config);

        new Expectations(ProxyGateway.class) {
            {
                proxy.startListening();
            }
        };
        new Expectations() {
            {
                new NanomsgCommunicationEndpoint(config.getIdentifier(), (CommunicationControlStrategy) any);
                result = controlEndpoint;
                new NanomsgCommunicationEndpoint(dataSocketId, (CommunicationDataStrategy) any);
                result = dataEndpoint;

                new TestModuleImplementsInterface();
                result = module;

                controlEndpoint.connect();
                controlEndpoint.receiveMessage();
                result = createMessage;
                controlEndpoint.sendMessageNoWait((byte[]) any);
                result = true;

                dataEndpoint.connect();
                dataEndpoint.receiveMessage();
                result = null;
            }
        };

        proxy.attach();
        Runnable receiveMessage = proxy.getReceiveMessageListener();
        receiveMessage.run();

        new Verifications() {
            {
                controlEndpoint.connect();
                times = 1;
                controlEndpoint.sendMessageNoWait((byte[]) any);
                times = 1;
                dataEndpoint.connect();
                times = 1;

                module.create(anyLong, (Broker) any, anyString);
            }
        };

        assertTrue(proxy.isAttached());
    }

    // Tests_SRS_JAVA_PROXY_GATEWAY_24_010: [ *Message Listener task* - It shall poll the gateway control channel for new messages. ]
    // Tests_SRS_JAVA_PROXY_GATEWAY_24_014: [ *Message Listener task* - If the message type is CREATE, it shall process the create message ]
    // Tests_SRS_JAVA_PROXY_GATEWAY_24_015: [ *Message Listener task - Create message* - Create message processing shall create the data message channel and connect to it. ]
    // Tests_SRS_JAVA_PROXY_GATEWAY_24_018: [ *Message Listener task - Create message* - Create message processing shall create a module instance and call `create` method. ]
    @Test
    public void attachShouldCreateModuleInstanceArgsConstructor(@Mocked final NanomsgCommunicationEndpoint controlEndpoint,
            @Mocked final NanomsgCommunicationEndpoint dataEndpoint)
            throws ConnectionException, MessageDeserializationException {

        ModuleConfiguration.Builder configBuilder = new ModuleConfiguration.Builder();
        configBuilder.setIdentifier(identifier);
        configBuilder.setModuleClass(TestModuleExtendsAbstractClass.class);
        configBuilder.setModuleVersion(version);

        final ProxyGateway proxy = new ProxyGateway(configBuilder.build());

        new Expectations(ProxyGateway.class) {
            {
                proxy.startListening();
            }
        };
        new Expectations() {
            {
                new NanomsgCommunicationEndpoint(config.getIdentifier(), (CommunicationControlStrategy) any);
                result = controlEndpoint;
                new NanomsgCommunicationEndpoint(dataSocketId, (CommunicationDataStrategy) any);
                result = dataEndpoint;

                controlEndpoint.connect();
                controlEndpoint.receiveMessage();
                result = createMessage;
                controlEndpoint.sendMessageNoWait((byte[]) any);
                result = true;

                dataEndpoint.connect();
                dataEndpoint.receiveMessage();
                result = null;
            }
        };

        proxy.attach();
        Runnable receiveMessage = proxy.getReceiveMessageListener();
        receiveMessage.run();

        new Verifications() {
            {
                controlEndpoint.connect();
                times = 1;
                controlEndpoint.sendMessageNoWait((byte[]) any);
                times = 1;
                dataEndpoint.connect();
                times = 1;
            }
        };

        assertTrue(proxy.isAttached());
    }

    // Tests_SRS_JAVA_PROXY_GATEWAY_24_017: [ *Message Listener task - Create message* - If the creation process has already occurred, 
    // it shall call module destroy, disconnect from the message channel and continue processing the creation message ]
    @Test
    public void attachShouldHandleCreateMessageIfAlreadyCreated(@Mocked final NanomsgCommunicationEndpoint controlEndpoint,
            @Mocked final NanomsgCommunicationEndpoint dataEndpoint, @Mocked final TestModuleImplementsInterface module,
            @Mocked final MessageSerializer serializer) throws ConnectionException, MessageDeserializationException {

        final ProxyGateway proxy = new ProxyGateway(config);

        new Expectations(ProxyGateway.class) {
            {
                proxy.startListening();
            }
        };
        new Expectations() {
            {
                new NanomsgCommunicationEndpoint(config.getIdentifier(), (CommunicationControlStrategy) any);
                result = controlEndpoint;
                new NanomsgCommunicationEndpoint(dataSocketId, (CommunicationDataStrategy) any);
                result = dataEndpoint;

                new TestModuleImplementsInterface();
                result = module;

                controlEndpoint.connect();
                controlEndpoint.receiveMessage();
                returns(createMessage, createMessage);
                controlEndpoint.sendMessageNoWait((byte[]) any);
                result = true;

                serializer.serializeMessage(RemoteModuleReplyCode.OK.getValue(), anyByte);

                dataEndpoint.connect();
                dataEndpoint.receiveMessage();
                result = null;
            }
        };

        proxy.attach();
        ProxyGateway.MessageListener receiveMessage = proxy.getReceiveMessageListener();
        receiveMessage.executeControlMessage();
        receiveMessage.executeDataMessage();

        receiveMessage.executeControlMessage();
        receiveMessage.executeDataMessage();

        new Verifications() {
            {
                controlEndpoint.connect();
                times = 1;
                controlEndpoint.sendMessageNoWait((byte[]) any);
                times = 2;

                dataEndpoint.connect();
                times = 2;
                dataEndpoint.disconnect();
                times = 1;

                module.create(anyLong, (Broker) any, anyString);
                times = 2;
                module.destroy();
                times = 1;

                serializer.serializeMessage(RemoteModuleReplyCode.OK.getValue(), anyByte);
                times = 2;
            }
        };

        assertTrue(proxy.isAttached());
    }

    // Tests_SRS_JAVA_PROXY_GATEWAY_24_012: [ *Message Listener task* - If a control message is received and deserialization fails it shall send an error message to the Gateway. ]
    @Test
    public void attachShouldNotInstantiateModuleIfMessageDeserializationException(
            @Mocked final NanomsgCommunicationEndpoint controlEndpoint, @Mocked final TestModuleImplementsInterface module,
            @Mocked final MessageSerializer serializer) throws ConnectionException, MessageDeserializationException {

        final ProxyGateway proxy = new ProxyGateway(config);

        new Expectations(ProxyGateway.class) {
            {
                proxy.startListening();
            }
        };
        new Expectations() {
            {
                new NanomsgCommunicationEndpoint(config.getIdentifier(), (CommunicationControlStrategy) any);
                result = controlEndpoint;

                controlEndpoint.connect();
                controlEndpoint.receiveMessage();
                result = new MessageDeserializationException();

                serializer.serializeMessage(RemoteModuleReplyCode.CONNECTION_ERROR.getValue(), anyByte);

                controlEndpoint.sendMessageNoWait((byte[]) any);
                result = true;
            }
        };

        proxy.attach();
        ProxyGateway.MessageListener receiveMessage = proxy.getReceiveMessageListener();
        receiveMessage.executeControlMessage();
        receiveMessage.executeDataMessage();

        new Verifications() {
            {
                controlEndpoint.connect();
                times = 1;

                serializer.serializeMessage(RemoteModuleReplyCode.CONNECTION_ERROR.getValue(), anyByte);
                times = 1;

                controlEndpoint.sendMessageNoWait((byte[]) any);
                times = 1;

                new TestModuleImplementsInterface();
                times = 0;
            }
        };

        assertTrue(proxy.isAttached());
    }

    // Tests_SRS_JAVA_PROXY_GATEWAY_24_013: [ *Message Listener task* - If there is an error receiving the control message it shall send an error message to the Gateway. ]
    @Test
    public void attachShouldNotInstantiateModuleIfConnectionException(
            @Mocked final NanomsgCommunicationEndpoint controlEndpoint, @Mocked final TestModuleImplementsInterface module,
            @Mocked final MessageSerializer serializer) throws ConnectionException, MessageDeserializationException {

        final ProxyGateway proxy = new ProxyGateway(config);

        new Expectations(ProxyGateway.class) {
            {
                proxy.startListening();
            }
        };
        new Expectations() {
            {
                new NanomsgCommunicationEndpoint(config.getIdentifier(), (CommunicationControlStrategy) any);
                result = controlEndpoint;

                controlEndpoint.connect();
                controlEndpoint.receiveMessage();
                result = new ConnectionException();

                serializer.serializeMessage(RemoteModuleReplyCode.CONNECTION_ERROR.getValue(), anyByte);

                controlEndpoint.sendMessageNoWait((byte[]) any);
                result = true;
            }
        };

        proxy.attach();
        ProxyGateway.MessageListener receiveMessage = proxy.getReceiveMessageListener();
        receiveMessage.executeControlMessage();
        receiveMessage.executeDataMessage();

        new Verifications() {
            {
                controlEndpoint.connect();
                times = 1;

                serializer.serializeMessage(RemoteModuleReplyCode.CONNECTION_ERROR.getValue(), anyByte);
                times = 1;

                controlEndpoint.sendMessageNoWait((byte[]) any);
                times = 1;

                new TestModuleImplementsInterface();
                times = 0;
            }
        };

        assertTrue(proxy.isAttached());
    }

    // Tests_SRS_JAVA_PROXY_GATEWAY_24_016: [ *Message Listener task - Create message* - If connection to the message channel fails, it shall send an error message to the Gateway. ]
    // Tests_SRS_JAVA_PROXY_GATEWAY_24_025: [ *Message Listener task - Data message* - It shall not check for messages, if the message channel is not available. ]
    @Test
    public void attachShouldHandleCreateDataEndpointFail(@Mocked final NanomsgCommunicationEndpoint controlEndpoint,
            @Mocked final NanomsgCommunicationEndpoint dataEndpoint, @Mocked final MessageSerializer serializer)
            throws ConnectionException, MessageDeserializationException {

        final ProxyGateway proxy = new ProxyGateway(config);

        new Expectations(ProxyGateway.class) {
            {
                proxy.startListening();
            }
        };
        new Expectations() {
            {
                new NanomsgCommunicationEndpoint(config.getIdentifier(), (CommunicationControlStrategy) any);
                result = controlEndpoint;
                new NanomsgCommunicationEndpoint(dataSocketId, (CommunicationDataStrategy) any);
                result = dataEndpoint;

                controlEndpoint.connect();
                controlEndpoint.receiveMessage();
                result = createMessage;

                dataEndpoint.connect();
                result = new ConnectionException();

                serializer.serializeMessage(RemoteModuleReplyCode.CONNECTION_ERROR.getValue(), anyByte);

                controlEndpoint.sendMessageNoWait((byte[]) any);
                result = true;
            }
        };

        proxy.attach();
        ProxyGateway.MessageListener receiveMessage = proxy.getReceiveMessageListener();
        receiveMessage.executeControlMessage();
        receiveMessage.executeDataMessage();

        new Verifications() {
            {
                controlEndpoint.connect();
                times = 1;

                serializer.serializeMessage(RemoteModuleReplyCode.CONNECTION_ERROR.getValue(), anyByte);
                times = 1;

                controlEndpoint.sendMessageNoWait((byte[]) any);
                times = 1;
                
                dataEndpoint.receiveMessage();
                times = 0;
            }
        };

        assertTrue(proxy.isAttached());
    }

    // Tests_SRS_JAVA_PROXY_GATEWAY_24_019: [ *Message Listener task - Create message* - If module instance creation fails, it shall send an error message to the Gateway. ]
    @Test
    public void attachShouldSendCreationErrorIfModuleInstantionFails(
            @Mocked final NanomsgCommunicationEndpoint controlEndpoint, @Mocked final NanomsgCommunicationEndpoint dataEndpoint,
            @Mocked final TestModuleImplementsInterface module, @Mocked final MessageSerializer serializer)
            throws ConnectionException, MessageDeserializationException {

        final ProxyGateway proxy = new ProxyGateway(config);

        new Expectations(ProxyGateway.class) {
            {
                proxy.startListening();
            }
        };
        new Expectations() {
            {
                new NanomsgCommunicationEndpoint(config.getIdentifier(), (CommunicationControlStrategy) any);
                result = controlEndpoint;
                new NanomsgCommunicationEndpoint(dataSocketId, (CommunicationDataStrategy) any);
                result = dataEndpoint;

                controlEndpoint.connect();
                controlEndpoint.receiveMessage();
                result = createMessage;

                dataEndpoint.receiveMessage();
                result = null;

                serializer.serializeMessage(RemoteModuleReplyCode.CREATION_ERROR.getValue(), anyByte);

                controlEndpoint.sendMessageNoWait((byte[]) any);
                result = true;

                new TestModuleImplementsInterface();
                result = new InstantiationException();
            }
        };

        proxy.attach();
        ProxyGateway.MessageListener receiveMessage = proxy.getReceiveMessageListener();
        receiveMessage.executeControlMessage();
        receiveMessage.executeDataMessage();

        new Verifications() {
            {
                controlEndpoint.connect();
                times = 1;

                serializer.serializeMessage(RemoteModuleReplyCode.CREATION_ERROR.getValue(), anyByte);
                times = 1;

                controlEndpoint.sendMessageNoWait((byte[]) any);
                times = 1;

                new TestModuleImplementsInterface();
                times = 1;
            }
        };

        assertTrue(proxy.isAttached());
    }

    // Tests_SRS_JAVA_PROXY_GATEWAY_24_020: [ *Message Listener task - Create message* - If the Create message finished processing, it shall send an ok message to the Gateway. ]
    @Test
    public void attachSuccessShouldSendSuccessMessage(@Mocked final NanomsgCommunicationEndpoint controlEndpoint,
            @Mocked final NanomsgCommunicationEndpoint dataEndpoint, @Mocked final TestModuleImplementsInterface module,
            @Mocked final MessageSerializer serializer) throws ConnectionException, MessageDeserializationException {

        final ProxyGateway proxy = new ProxyGateway(config);

        new Expectations(ProxyGateway.class) {
            {
                proxy.startListening();
            }
        };
        new Expectations() {
            {
                new NanomsgCommunicationEndpoint(config.getIdentifier(), (CommunicationControlStrategy) any);
                result = controlEndpoint;
                new NanomsgCommunicationEndpoint(dataSocketId, (CommunicationDataStrategy) any);
                result = dataEndpoint;

                new TestModuleImplementsInterface();
                result = module;

                controlEndpoint.connect();
                controlEndpoint.receiveMessage();
                returns(createMessage, startMessage);
                controlEndpoint.sendMessageNoWait((byte[]) any);
                result = true;

                serializer.serializeMessage(RemoteModuleReplyCode.OK.getValue(), anyByte);

                dataEndpoint.connect();
                dataEndpoint.receiveMessage();
                returns(null, dataMessage);
            }
        };

        proxy.attach();
        ProxyGateway.MessageListener receiveMessage = proxy.getReceiveMessageListener();
        receiveMessage.executeControlMessage();
        receiveMessage.executeDataMessage();

        receiveMessage.executeControlMessage();
        receiveMessage.executeDataMessage();

        new Verifications() {
            {
                controlEndpoint.connect();
                times = 1;
                controlEndpoint.sendMessageNoWait((byte[]) any);
                times = 1;

                dataEndpoint.connect();
                times = 1;

                module.create(anyLong, (Broker) any, anyString);
                times = 1;
                module.start();
                times = 1;
                module.receive(content);

                serializer.serializeMessage(RemoteModuleReplyCode.OK.getValue(), anyByte);
                times = 1;
            }
        };

        assertTrue(proxy.isAttached());
    }
    
    // Tests_SRS_JAVA_PROXY_GATEWAY_24_021: [ *Message Listener task - Create message* - If ok message fails to be send to the Gateway, it shall do call module `destroy` and disconnect from message channel. ]
    @Test
    public void attachShouldDestroyIfSuccessMessageSendFails(@Mocked final NanomsgCommunicationEndpoint controlEndpoint,
            @Mocked final NanomsgCommunicationEndpoint dataEndpoint, @Mocked final TestModuleImplementsInterface module,
            @Mocked final MessageSerializer serializer) throws ConnectionException, MessageDeserializationException {

        final ProxyGateway proxy = new ProxyGateway(config);

        new Expectations(ProxyGateway.class) {
            {
                proxy.startListening();
            }
        };
        new Expectations() {
            {
                new NanomsgCommunicationEndpoint(config.getIdentifier(), (CommunicationControlStrategy) any);
                result = controlEndpoint;
                new NanomsgCommunicationEndpoint(dataSocketId, (CommunicationDataStrategy) any);
                result = dataEndpoint;

                new TestModuleImplementsInterface();
                result = module;

                controlEndpoint.connect();
                controlEndpoint.receiveMessage();
                result = createMessage;
                controlEndpoint.sendMessageNoWait((byte[]) any);
                result = false;

                serializer.serializeMessage(RemoteModuleReplyCode.OK.getValue(), anyByte);

                dataEndpoint.connect();
                dataEndpoint.receiveMessage();
                result = null;
            }
        };

        proxy.attach();
        ProxyGateway.MessageListener receiveMessage = proxy.getReceiveMessageListener();
        receiveMessage.executeControlMessage();
        receiveMessage.executeDataMessage();

        new Verifications() {
            {
                controlEndpoint.connect();
                times = 1;
                controlEndpoint.sendMessageNoWait((byte[]) any);
                times = 1;

                dataEndpoint.connect();
                times = 1;

                module.create(anyLong, (Broker) any, anyString);
                times = 1;
                module.destroy();
                times = 1;

                serializer.serializeMessage(RemoteModuleReplyCode.OK.getValue(), anyByte);
                times = 1;
            }
        };

        assertTrue(proxy.isAttached());
    }
    
    // Tests_SRS_JAVA_PROXY_GATEWAY_24_022: [ *Message Listener task - Start message* - If message type is START, it shall call module `start` method. ]
    @Test
    public void attachShouldCallModuleStart(@Mocked final NanomsgCommunicationEndpoint controlEndpoint,
            @Mocked final NanomsgCommunicationEndpoint dataEndpoint, @Mocked final TestModuleImplementsInterface module)
            throws ConnectionException, MessageDeserializationException {

        final ProxyGateway proxy = new ProxyGateway(config);

        new Expectations(ProxyGateway.class) {
            {
                proxy.startListening();
            }
        };
        new Expectations() {
            {
                new NanomsgCommunicationEndpoint(config.getIdentifier(), (CommunicationControlStrategy) any);
                result = controlEndpoint;
                new NanomsgCommunicationEndpoint(dataSocketId, (CommunicationDataStrategy) any);
                result = dataEndpoint;

                new TestModuleImplementsInterface();
                result = module;

                controlEndpoint.connect();
                controlEndpoint.receiveMessage();
                returns(createMessage, startMessage);
                controlEndpoint.sendMessageNoWait((byte[]) any);
                result = true;

                dataEndpoint.connect();
                dataEndpoint.receiveMessage();
                result = null;
            }
        };

        proxy.attach();
        MessageListener messageListener = proxy.getReceiveMessageListener();
        messageListener.executeControlMessage();
        messageListener.executeDataMessage();

        messageListener.executeControlMessage();
        messageListener.executeDataMessage();

        new Verifications() {
            {
                controlEndpoint.connect();
                times = 1;
                controlEndpoint.sendMessageNoWait((byte[]) any);
                times = 1;
                dataEndpoint.connect();
                times = 1;

                module.create(anyLong, (Broker) any, anyString);
                times = 1;
                module.start();
                times = 1;
            }
        };

        assertTrue(proxy.isAttached());
    }

    @Test
    public void attachShouldIgnoreMessageIfStartMessageBeforeCreate(@Mocked final NanomsgCommunicationEndpoint controlEndpoint,
            @Mocked final NanomsgCommunicationEndpoint dataEndpoint)
            throws ConnectionException, MessageDeserializationException {

        final ProxyGateway proxy = new ProxyGateway(config);

        new Expectations(ProxyGateway.class) {
            {
                proxy.startListening();
            }
        };
        new Expectations() {
            {
                new NanomsgCommunicationEndpoint(config.getIdentifier(), (CommunicationControlStrategy) any);
                result = controlEndpoint;
                controlEndpoint.receiveMessage();
                result = startMessage;
            }
        };

        proxy.attach();
        MessageListener messageListener = proxy.getReceiveMessageListener();
        messageListener.executeControlMessage();
        messageListener.executeDataMessage();

        assertTrue(proxy.isAttached());
    }

    // Tests_SRS_JAVA_PROXY_GATEWAY_24_023: [ *Message Listener task - Destroy message* - If message type is DESTROY, it shall call module `destroy` method. ]
    // Tests_SRS_JAVA_PROXY_GATEWAY_24_024: [ *Message Listener task - Destroy message* - If message type is DESTROY, it shall disconnect from the message channel. ]
    @Test
    public void attachShouldCallModuleDestroyIfReceivesDestroyMessage(@Mocked final NanomsgCommunicationEndpoint controlEndpoint,
            @Mocked final NanomsgCommunicationEndpoint dataEndpoint, @Mocked final TestModuleImplementsInterface module)
            throws ConnectionException, MessageDeserializationException {

        final ProxyGateway proxy = new ProxyGateway(config);

        new Expectations(ProxyGateway.class) {
            {
                proxy.startListening();
            }
        };
        new Expectations() {
            {
                new NanomsgCommunicationEndpoint(config.getIdentifier(), (CommunicationControlStrategy) any);
                result = controlEndpoint;
                controlEndpoint.receiveMessage();
                returns(createMessage, destroyMessage);

                controlEndpoint.sendMessageNoWait((byte[]) any);
                result = true;

                new NanomsgCommunicationEndpoint(dataSocketId, (CommunicationDataStrategy) any);
                result = dataEndpoint;
                dataEndpoint.receiveMessage();
                result = null;

                new TestModuleImplementsInterface();
                result = module;
            }
        };

        proxy.attach();

        MessageListener messageListener = proxy.getReceiveMessageListener();
        messageListener.executeControlMessage();
        messageListener.executeDataMessage();

        messageListener.executeControlMessage();
        messageListener.executeDataMessage();

        new Verifications() {
            {
                controlEndpoint.connect();
                times = 1;
                controlEndpoint.disconnect();
                times = 0;
                controlEndpoint.sendMessageNoWait((byte[]) any);
                times = 1;
                
                dataEndpoint.disconnect();
                times = 1;

                new TestModuleImplementsInterface();
                times = 1;

                module.destroy();
                times = 1;
            }
        };

        assertTrue(proxy.isAttached());
    }

    // Tests_SRS_JAVA_PROXY_GATEWAY_24_023: [ *Message Listener task - Destroy message* - If message type is DESTROY, it shall call module `destroy` method. ]
    @Test
    public void attachShouldDestroyIfDestroyMessageBeforeCreate(@Mocked final NanomsgCommunicationEndpoint controlEndpoint,
            @Mocked final NanomsgCommunicationEndpoint dataEndpoint)
            throws ConnectionException, MessageDeserializationException {

        final ProxyGateway proxy = new ProxyGateway(config);

        new Expectations(ProxyGateway.class) {
            {
                proxy.startListening();
            }
        };
        new Expectations() {
            {
                new NanomsgCommunicationEndpoint(config.getIdentifier(), (CommunicationControlStrategy) any);
                result = controlEndpoint;
                controlEndpoint.receiveMessage();
                result = destroyMessage;
            }
        };

        proxy.attach();
        MessageListener messageListener = proxy.getReceiveMessageListener();
        messageListener.executeControlMessage();
        messageListener.executeDataMessage();

        assertTrue(proxy.isAttached());
    }

    // Tests_SRS_JAVA_PROXY_GATEWAY_24_027: [ *Message Listener task - Data message* - If no data message is received or if an error occurs, it shall do nothing. ]
    // Tests_SRS_JAVA_PROXY_GATEWAY_24_026: [ *Message Listener task - Data message* - If data message is received, it shall forward it to the module by calling `receive` method. ]
    @Test
    public void attachShouldReceiveDataMessage(@Mocked final NanomsgCommunicationEndpoint controlEndpoint,
            @Mocked final NanomsgCommunicationEndpoint dataEndpoint, @Mocked final TestModuleImplementsInterface module,
            @Mocked final MessageSerializer serializer) throws ConnectionException, MessageDeserializationException {

        final ProxyGateway proxy = new ProxyGateway(config);

        new Expectations(ProxyGateway.class) {
            {
                proxy.startListening();
            }
        };
        new Expectations() {
            {
                new NanomsgCommunicationEndpoint(config.getIdentifier(), (CommunicationControlStrategy) any);
                result = controlEndpoint;
                new NanomsgCommunicationEndpoint(dataSocketId, (CommunicationDataStrategy) any);
                result = dataEndpoint;

                new TestModuleImplementsInterface();
                result = module;

                controlEndpoint.connect();
                controlEndpoint.receiveMessage();
                returns(createMessage, startMessage);
                controlEndpoint.sendMessageNoWait((byte[]) any);
                result = true;

                serializer.serializeMessage(RemoteModuleReplyCode.OK.getValue(), anyByte);

                dataEndpoint.connect();
                dataEndpoint.receiveMessage();
                returns(null, dataMessage);
            }
        };

        proxy.attach();
        ProxyGateway.MessageListener receiveMessage = proxy.getReceiveMessageListener();
        receiveMessage.executeControlMessage();
        receiveMessage.executeDataMessage();

        receiveMessage.executeControlMessage();
        receiveMessage.executeDataMessage();

        new Verifications() {
            {
                controlEndpoint.connect();
                times = 1;
                controlEndpoint.sendMessageNoWait((byte[]) any);
                times = 1;

                dataEndpoint.connect();
                times = 1;

                module.create(anyLong, (Broker) any, anyString);
                times = 1;
                module.start();
                times = 1;
                module.receive(content);

                serializer.serializeMessage(RemoteModuleReplyCode.OK.getValue(), anyByte);
                times = 1;
            }
        };

        assertTrue(proxy.isAttached());
    }

    // Tests_SRS_JAVA_PROXY_GATEWAY_24_028: [ If not attached the function shall do nothing and return. ]
    @Test
    public void detachNoOpIfNotAttached() throws ConnectionException, MessageDeserializationException {

        final ProxyGateway proxy = new ProxyGateway(config);

        proxy.detach();

        assertFalse(proxy.isAttached());
    }

    // Tests_SRS_JAVA_PROXY_GATEWAY_24_029: [ It shall attempt to stop the message listening tasks. ]
    // Tests_SRS_JAVA_PROXY_GATEWAY_24_030: [ It shall wait for a minute for executing task to terminate. ]
    // Tests_SRS_JAVA_PROXY_GATEWAY_24_031: [ It shall call module destroy. ]
    // Tests_SRS_JAVA_PROXY_GATEWAY_24_032: [ It shall attempt to notify the Gateway of the detachment. ]
    // Tests_SRS_JAVA_PROXY_GATEWAY_24_033: [ It shall disconnect from the Gateway control channel. ]
    // Tests_SRS_JAVA_PROXY_GATEWAY_24_034: [ It shall disconnect from the Gateway message channel. ]
    @Test
    public void detachSuccess(@Mocked final NanomsgCommunicationEndpoint controlEndpoint,
            @Mocked final NanomsgCommunicationEndpoint dataEndpoint, @Mocked final TestModuleImplementsInterface module,
            @Mocked final MessageSerializer serializer) throws ConnectionException, MessageDeserializationException {

        final ProxyGateway proxy = new ProxyGateway(config);

        new Expectations(ProxyGateway.class) {
            {
                proxy.startListening();
            }
        };
        new Expectations() {
            {
                new NanomsgCommunicationEndpoint(config.getIdentifier(), (CommunicationControlStrategy) any);
                result = controlEndpoint;
                new NanomsgCommunicationEndpoint(dataSocketId, (CommunicationDataStrategy) any);
                result = dataEndpoint;

                new TestModuleImplementsInterface();
                result = module;

                controlEndpoint.connect();
                controlEndpoint.receiveMessage();
                ControlMessage nullMessage = null;
                returns(createMessage, startMessage, nullMessage);
                controlEndpoint.sendMessageNoWait((byte[]) any);
                result = true;

                serializer.serializeMessage(RemoteModuleReplyCode.OK.getValue(), anyByte);
                serializer.serializeMessage(RemoteModuleReplyCode.DETACH.getValue(), anyByte);

                dataEndpoint.connect();
                dataEndpoint.receiveMessage();
                returns(null, dataMessage);
            }
        };

        proxy.attach();
        ProxyGateway.MessageListener receiveMessage = proxy.getReceiveMessageListener();
        receiveMessage.executeControlMessage();
        receiveMessage.executeDataMessage();

        receiveMessage.executeControlMessage();
        receiveMessage.executeDataMessage();

        proxy.detach();

        new Verifications() {
            {
                controlEndpoint.connect();
                times = 1;
                controlEndpoint.sendMessageNoWait((byte[]) any);
                times = 2;
                controlEndpoint.disconnect();
                times = 1;

                dataEndpoint.connect();
                times = 1;

                module.create(anyLong, (Broker) any, anyString);
                times = 1;
                module.start();
                times = 1;
                module.receive(content);
                module.destroy();
                times = 1;

                serializer.serializeMessage(RemoteModuleReplyCode.OK.getValue(), anyByte);
                times = 1;

                serializer.serializeMessage(RemoteModuleReplyCode.DETACH.getValue(), anyByte);
                times = 1;
            }
        };

        assertFalse(proxy.isAttached());
        assertTrue(proxy.getExecutor().isTerminated());
    }

    @Test
    public void attachAfterDetachSuccess(@Mocked final NanomsgCommunicationEndpoint controlEndpoint,
            @Mocked final NanomsgCommunicationEndpoint dataEndpoint, @Mocked final TestModuleImplementsInterface module,
            @Mocked final MessageSerializer serializer) throws ConnectionException, MessageDeserializationException {

        final ProxyGateway proxy = new ProxyGateway(config);

        new Expectations(ProxyGateway.class) {
            {
                proxy.startListening();
            }
        };
        new Expectations() {
            {
                new NanomsgCommunicationEndpoint(config.getIdentifier(), (CommunicationControlStrategy) any);
                result = controlEndpoint;
                new NanomsgCommunicationEndpoint(dataSocketId, (CommunicationDataStrategy) any);
                result = dataEndpoint;

                new TestModuleImplementsInterface();
                result = module;

                controlEndpoint.connect();
                controlEndpoint.receiveMessage();
                returns(createMessage, startMessage, createMessage, startMessage);
                controlEndpoint.sendMessageNoWait((byte[]) any);
                result = true;

                serializer.serializeMessage(RemoteModuleReplyCode.OK.getValue(), anyByte);
                serializer.serializeMessage(RemoteModuleReplyCode.DETACH.getValue(), anyByte);

                dataEndpoint.connect();
                dataEndpoint.receiveMessage();
                returns(null, dataMessage);
            }
        };

        proxy.attach();
        ProxyGateway.MessageListener receiveMessage = proxy.getReceiveMessageListener();
        receiveMessage.executeControlMessage();
        receiveMessage.executeDataMessage();

        receiveMessage.executeControlMessage();
        receiveMessage.executeDataMessage();

        proxy.detach();

        proxy.attach();
        receiveMessage = proxy.getReceiveMessageListener();
        receiveMessage.executeControlMessage();
        receiveMessage.executeDataMessage();

        receiveMessage.executeControlMessage();
        receiveMessage.executeDataMessage();

        new Verifications() {
            {
                controlEndpoint.connect();
                times = 2;
                controlEndpoint.sendMessageNoWait((byte[]) any);
                times = 3;

                dataEndpoint.connect();
                times = 2;

                module.create(anyLong, (Broker) any, anyString);
                times = 2;
                module.start();
                times = 2;
                module.receive(content);
                module.destroy();
                times = 1;

                serializer.serializeMessage(RemoteModuleReplyCode.OK.getValue(), anyByte);
                times = 2;

                serializer.serializeMessage(RemoteModuleReplyCode.DETACH.getValue(), anyByte);
                times = 1;
            }
        };

        assertTrue(proxy.isAttached());
    }
}
