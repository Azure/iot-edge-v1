/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.remote;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.microsoft.azure.gateway.core.Broker;
import com.microsoft.azure.gateway.core.IGatewayModule;

/**
 * A Proxy for the remote module that can attach to the Azure IoT Gateway to
 * receive and send messages. The proxy handles creating a module instance and
 * calls create and start methods from the module and handles the
 * communication to and from the Gateway and forwards the messages to the
 * module. The module that is instantiated by the proxy is specified in the 
 * configuration.
 * 
 * <p>
 * The {@code attach} method is creating a communication channel with the
 * Gateway and starts listening for messages from the Gateway. Once the
 * initialization messages are received from the Gateway it creates an instance
 * of the IGatewayModule, calls create and start on the module and forwards
 * received messages from the Gateway. If it receives a destroy message from the
 * Gateway it calls destroy on the module.
 * 
 * <p>
 * The {@code detach} method stops receiving new messages and sends a notify
 * message to the Gateway.
 * 
 * <h3>Usage Examples</h3>
 *
 * Here is how an existing Printer, that implements
 * {@link com.microsoft.azure.gateway.core.IGatewayModule IGatewayModule}
 * module, can be run used as a remote module:
 *
 * <pre>
 *  {@code
 *  String id = "control-id";
 *  byte version = 1;
 *  ModuleConfiguration.Builder configBuilder = new ModuleConfiguration.Builder();
 *  configBuilder.setIdentifier(id);
 *  configBuilder.setModuleClass(Printer.class);
 *  configBuilder.setModuleVersion(version);
 *      
 *  RemoteModuleProxy moduleProxy = new RemoteModuleProxy(configBuilder.build());
 *  try {
 *       moduleProxy.attach();
 *  } catch (ConnectionException e) {
 *       e.printStackTrace();
 *  }
 * }}
 * </pre>
 *
 */
public class ProxyGateway {

    private static final int DEFAULT_DELAY_MILLIS = 10;
    private final ModuleConfiguration config;
    private final Object lock = new Object();
    private boolean isAttached;
    private ScheduledExecutorService executor;
    private MessageListener receiveMessageListener;

    public ProxyGateway(ModuleConfiguration configuration) {
        //Codes_SRS_JAVA_PROXY_GATEWAY_24_001: [ If `config` is `null` the constructor shall throw an IllegalArgumentException. ]
        if (configuration == null)
            throw new IllegalArgumentException("Configuration can not be null.");

        //Codes_SRS_JAVA_PROXY_GATEWAY_24_002: [ The constructor shall save `config` for later use. ]
        this.config = configuration;
    }

    /**
     * Attach the remote module to the Gateway. It creates the communication
     * channel with the Gateway and starts a new thread that is listening for
     * messages.
     *
     * @throws ConnectionException
     *             If it can not connect to the Gateway communication channel
     */
    public void attach() throws ConnectionException {
        synchronized (lock) {
            //Codes_SRS_JAVA_PROXY_GATEWAY_24_003: [ If already attached the function shall do nothing and return. ]
            if (!this.isAttached) {
                //Codes_SRS_JAVA_PROXY_GATEWAY_24_004: [ The function shall instantiate the messages listener task. ]
                this.receiveMessageListener = new MessageListener(config);
                //Codes_SRS_JAVA_PROXY_GATEWAY_24_005: [ It shall instantiate a single-threaded executor that can schedule the task to execute periodically. ]
                this.executor = Executors.newSingleThreadScheduledExecutor();
                //Codes_SRS_JAVA_PROXY_GATEWAY_24_006: [ It shall start executing the periodic task of listening for messages from the Gateway. ]
                this.startListening();
                this.isAttached = true;
            }
        }
    }

    /**
     * Detach from the Gateway. The listening of messages from the Gateway is
     * terminated and destroy method on IGatewayModule instance is called. A
     * notification message is sent to the Gateway.
     */
    public void detach() {
        boolean sendDetachToGateway = true;
        this.detach(sendDetachToGateway);
    }

    void startListening() {
        this.executor.scheduleWithFixedDelay(receiveMessageListener, 0, DEFAULT_DELAY_MILLIS, TimeUnit.MILLISECONDS);
    }

    boolean isAttached() {
        return this.isAttached;
    }

    MessageListener getReceiveMessageListener() {
        return this.receiveMessageListener;
    }

    ScheduledExecutorService getExecutor() {
        return this.executor;
    }

    private void detach(boolean sendDetachToGateway) {
        synchronized (lock) {
            // Codes_SRS_JAVA_PROXY_GATEWAY_24_028: [ If not attached the function shall do nothing and return. ]
            if (this.isAttached) {
                // Codes_SRS_JAVA_PROXY_GATEWAY_24_029: [ Attempts to stop the message listening tasks. ]
                this.executor.shutdownNow();
                try {
                    // Codes_SRS_JAVA_PROXY_GATEWAY_24_030: [ It shall wait for a minute for executing task to terminate. ]
                    executor.awaitTermination(60, TimeUnit.SECONDS);
                } catch (InterruptedException e) {
                }

                this.receiveMessageListener.detach(true);
                this.isAttached = false;
            }
        }
    }

    class MessageListener implements Runnable {
        private final ModuleConfiguration config;
        private final Logger logger = LoggerFactory.getLogger(ProxyGateway.class);
        private CommunicationEndpoint controlEndpoint;
        private CommunicationEndpoint dataEndpoint;
        private IGatewayModule module;

        public MessageListener(ModuleConfiguration config) throws ConnectionException {
            this.config = config;
            // Codes_SRS_JAVA_PROXY_GATEWAY_24_007: [ *Message Listener task* - It shall create a new control channel with the Gateway. ]
            this.controlEndpoint = new NanomsgCommunicationEndpoint(this.config.getIdentifier(),
                    new CommunicationControlStrategy());
            this.controlEndpoint.setVersion(this.config.getVersion());
            // Codes_SRS_JAVA_PROXY_GATEWAY_24_008: [ *Message Listener task* - It shall connect to the control channel. ]
            // Codes_SRS_JAVA_PROXY_GATEWAY_24_009: [ *Message Listener task* - If the connection with the control channel fails, it shall throw ConnectionException. ]
            this.controlEndpoint.connect();
        }

        @Override
        public void run() {
            this.executeControlMessage();
            this.executeDataMessage();
        }

        public void detach(boolean sendDetachToGateway) {
            if (this.module != null)
                // Codes_SRS_JAVA_PROXY_GATEWAY_24_023: [ *Message Listener task - Destroy message* - If message type is DESTROY, it shall call module `destroy` method. ]
                // Codes_SRS_JAVA_PROXY_GATEWAY_24_031: [ It shall call module destroy. ]
                this.module.destroy();

            if (sendDetachToGateway) {
                logger.info("Sending DETACH to the Gateway...");
                // Codes_SRS_JAVA_PROXY_GATEWAY_24_032: [ It shall attempt to notify the Gateway of the detachment. ]
                this.sendControlReplyMessage(RemoteModuleReplyCode.DETACH.getValue());
                logger.info("DETACH sent successfully.");

                try {
                    // sleep for a second so the Gateway has time to receive the
                    // message before closing the connection
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                }

                // Codes_SRS_JAVA_PROXY_GATEWAY_24_033: [It shall disconnect from the Gateway control channel. ]
                this.controlEndpoint.disconnect();
            }

            // Codes_SRS_JAVA_PROXY_GATEWAY_24_024: [ *Message Listener task - Destroy message* - If message type is DESTROY, it shall disconnect from the message channel. ]
            // Codes_SRS_JAVA_PROXY_GATEWAY_24_034: [It shall disconnect from the Gateway message channel. ]
            if (this.dataEndpoint != null) {
                this.dataEndpoint.disconnect();
                this.dataEndpoint = null;
            }

            this.module = null;
        }

        void executeControlMessage() {
            RemoteMessage message = null;
            try {
                // Codes_SRS_JAVA_PROXY_GATEWAY_24_010: [ *Message Listener task* - It shall poll the gateway control channel for new messages. ]
                message = this.controlEndpoint.receiveMessage();
            } catch (MessageDeserializationException e) {
                logger.error(e.toString());
                // Codes_SRS_JAVA_PROXY_GATEWAY_24_012: [ *Message Listener task* - If a control message is received and deserialization fails it shall send an error message to the Gateway. ]
                this.sendControlReplyMessage(RemoteModuleReplyCode.CONNECTION_ERROR.getValue());
            } catch (ConnectionException e) {
                logger.error(e.toString());
                // Codes_SRS_JAVA_PROXY_GATEWAY_24_013: [ *Message Listener task* - If there is an error receiving the control message it shall send an error message to the Gateway. ]
                this.sendControlReplyMessage(RemoteModuleReplyCode.CONNECTION_ERROR.getValue());
            }
            // Codes_SRS_JAVA_PROXY_GATEWAY_24_011: [ *Message Listener task* - If no message is available the listener shall do nothing. ]
            if (message != null) {
                ControlMessage controlMessage = (ControlMessage) message;
                if (controlMessage.getMessageType() == RemoteMessageType.CREATE) {
                    logger.info("Received CREATE message from the Gateway");
                    try {
                        // Codes_SRS_JAVA_PROXY_GATEWAY_24_014: [ *Message Listener task* - If the message type is CREATE, it shall process the create message ]
                        this.processCreateMessage(message, this.controlEndpoint);
                        // Codes_SRS_JAVA_PROXY_GATEWAY_24_020: [ *Message Listener task - Create message* - If the Create message finished processing, it shall send an ok message to the Gateway. ]
                        boolean sent = sendControlReplyMessage(RemoteModuleReplyCode.OK.getValue());
                        // Codes_SRS_JAVA_PROXY_GATEWAY_24_021: [ *Message Listener task - Create message* - If ok message fails to be send to the Gateway, it shall do call module `destroy` and disconnect from message channel. ]
                        if (!sent)
                            this.disconnectDataMessage();
                    } catch (ConnectionException e) {
                        logger.error(e.toString());
                        this.sendControlReplyMessage(RemoteModuleReplyCode.CONNECTION_ERROR.getValue());
                    } catch (ModuleInstantiationException e) {
                        logger.error(e.toString());
                        this.sendControlReplyMessage(RemoteModuleReplyCode.CREATION_ERROR.getValue());
                    }
                    logger.info("Created successfully.");
                }

                if (controlMessage.getMessageType() == RemoteMessageType.START) {
                    logger.info("Received START message from the Gateway");
                    this.processStartMessage();
                    logger.info("Started successfully.");
                }

                if (controlMessage.getMessageType() == RemoteMessageType.DESTROY) {
                    logger.info("Received DESTROY message from the Gateway");
                    this.processDestroyMessage();
                    logger.info("Destroyed successfully.");
                }
            }
        }

        void executeDataMessage() {
            try {
                // Codes_SRS_JAVA_PROXY_GATEWAY_24_025: [ *Message Listener task - Data message* - It shall not check for messages, if the message channel is not available. ]
                if (this.dataEndpoint != null) {
                    RemoteMessage dataMessage = this.dataEndpoint.receiveMessage();
                    // Codes_SRS_JAVA_PROXY_GATEWAY_24_027: [ *Message Listener task - Data message* - If no data message is received or if an error occurs, it shall do nothing. ]
                    if (dataMessage != null) {
                        // Codes_SRS_JAVA_PROXY_GATEWAY_24_026: [ *Message Listener task - Data message* - If data message is received, it shall forward it to the module by calling `receive` method. ]
                        this.module.receive(((DataMessage) dataMessage).getContent());
                    }
                }
            } catch (ConnectionException e) {
                logger.error(e.toString());
            } catch (MessageDeserializationException e) {
                logger.error(e.toString());
            }
        }

        private void processDestroyMessage() {
            this.detach(false);
        }

        private void processStartMessage() {
            if (this.module == null)
                return;

            // Codes_SRS_JAVA_PROXY_GATEWAY_24_022: [ *Message Listener task - Start message* - If message type is START, it shall call module `start` method. ]
            this.module.start();
        }

        private void processCreateMessage(RemoteMessage message, CommunicationEndpoint endpoint)
                throws ConnectionException, ModuleInstantiationException {
            // Codes_SRS_JAVA_PROXY_GATEWAY_24_017: [ *Message Listener task - Create message* - If the creation process has already occurred, 
            // it shall call module destroy, disconnect from the message channel and continue processing the creation message ]
            this.disconnectDataMessage();

            CreateMessage controlMessage = (CreateMessage) message;
            // Codes_SRS_JAVA_PROXY_GATEWAY_24_015: [ *Message Listener task - Create message* - Create message processing shall create the data message channel and connect to it. ]
            // Codes_SRS_JAVA_PROXY_GATEWAY_24_016: [ *Message Listener task - Create message* - If connection to the message channel fails, it shall send an error message to the Gateway. ]
            this.dataEndpoint = this.createDataEndpoints(controlMessage.getDataEndpoint());

            try {
                // Codes_SRS_JAVA_PROXY_GATEWAY_24_018: [ *Message Listener task - Create message* - Create message processing shall create a module instance and call `create` method. ]
                this.createModuleInstanceWithArgsConstructor(controlMessage, this.dataEndpoint);

                if (this.module == null) {
                    this.createModuleInstanceNoArgsConstructor(controlMessage, this.dataEndpoint);
                }
            } catch (InstantiationException e) {
                logger.error(e.toString());
                // Codes_SRS_JAVA_PROXY_GATEWAY_24_019: [ *Message Listener task - Create message* - If module instance creation fails, it shall send an error message to the Gateway. ]
                throw new ModuleInstantiationException("Could not instantiate module", e);
            } catch (IllegalAccessException e) {
                logger.error(e.toString());
                // Codes_SRS_JAVA_PROXY_GATEWAY_24_019: [ *Message Listener task - Create message* - If module instance creation fails, it shall send an error message to the Gateway. ]
                throw new ModuleInstantiationException("Could not instantiate module", e);
            } catch (IllegalArgumentException e) {
                logger.error(e.toString());
                // Codes_SRS_JAVA_PROXY_GATEWAY_24_019: [ *Message Listener task - Create message* - If module instance creation fails, it shall send an error message to the Gateway. ]
                throw new ModuleInstantiationException("Could not instantiate module", e);
            } catch (InvocationTargetException e) {
                logger.error(e.toString());
                // Codes_SRS_JAVA_PROXY_GATEWAY_24_019: [ *Message Listener task - Create message* - If module instance creation fails, it shall send an error message to the Gateway. ]
                throw new ModuleInstantiationException("Could not instantiate module", e);
            }
        }

        private void disconnectDataMessage() {
            if (this.module != null) {
                this.module.destroy();
                this.module = null;
            }
            if (this.dataEndpoint != null)
                this.dataEndpoint.disconnect();
        }

        private boolean sendControlReplyMessage(int code) {
            byte[] createCompletedMessage = new MessageSerializer().serializeMessage(code,
                    this.controlEndpoint.getVersion());
            boolean sent = false;

            try {
                sent = this.controlEndpoint.sendMessageNoWait(createCompletedMessage);
            } catch (ConnectionException e) {
                logger.error(e.toString());
            }
            return sent;
        }

        private void createModuleInstanceNoArgsConstructor(CreateMessage controlMessage,
                CommunicationEndpoint dataEndpoint) throws InstantiationException, IllegalAccessException {
            final int emptyAddress = 0;
            this.module = this.config.getModuleClass().newInstance();
            this.module.create(emptyAddress, new BrokerProxy(dataEndpoint), controlMessage.getArgs());
        }

        private void createModuleInstanceWithArgsConstructor(CreateMessage controlMessage,
                CommunicationEndpoint dataEndpoint) throws InstantiationException, IllegalAccessException,
                IllegalArgumentException, InvocationTargetException {
            Class<? extends IGatewayModule> clazz = this.config.getModuleClass();
            final int emptyAddress = 0;
            try {
                Constructor<? extends IGatewayModule> ctor = clazz.getDeclaredConstructor(long.class, Broker.class,
                        String.class);
                this.module = ctor.newInstance(emptyAddress, new BrokerProxy(dataEndpoint), controlMessage.getArgs());
            } catch (NoSuchMethodException e) {
                logger.error(e.toString());
            }
        }

        private CommunicationEndpoint createDataEndpoints(DataEndpointConfig endpointConfig)
                throws ConnectionException {
            CommunicationEndpoint endpoint = new NanomsgCommunicationEndpoint(endpointConfig.getId(),
                    new CommunicationDataStrategy(endpointConfig.getType()));
            endpoint.connect();
            return endpoint;
        }
    }

}