/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.core;

import com.microsoft.azure.gateway.messaging.Message;

import java.io.IOException;

/**
 * The Abstract {@link GatewayModule} class to be extended by the module-creator when creating any modules.
 */
public abstract class GatewayModule implements IGatewayModule{

    public abstract void receive(Message message);
    public abstract void destroy();

    /** The address of the native module pointer */
    private long _addr;

    /** The {@link MessageBus} to which this module belongs */
    private MessageBus bus;

    /** The module-specific configuration object. */
    private String configuration;

    /**
     * Constructs a {@link GatewayModule} from the provided address and {@link MessageBus}. A {@link GatewayModule} should always call this super
     * constructor before any module-specific constructor code.
     *
     * The {@code address} parameter must be passed to the super constructor but can be ignored by the module-implementor when writing a module implementation.
     *
     * @param address The address of the native module pointer.
     * @param bus The {@link MessageBus} to which this module belongs
     * @param configuration The module-specific configuration
     */
    public GatewayModule(long address, MessageBus bus, String configuration){
        /*Codes_SRS_JAVA_GATEWAY_MODULE_14_002: [ If address or bus is null the constructor shall throw an IllegalArgumentException. ]*/
        if(address == 0 || bus == null){
            throw new IllegalArgumentException("Address is invalid or MessageBus is null.");
        }

        /*Codes_SRS_JAVA_GATEWAY_MODULE_14_001: [ The constructor shall save address, bus, and configuration into class variables. ]*/
        this.create(address, bus, configuration);
    }

    public void create(long moduleAddr, MessageBus bus, String configuration){
        this._addr = moduleAddr;
        this.bus = bus;
        this.configuration = configuration;
    }

    public void receive(byte[] serializedMessage){
        this.receive(new Message(serializedMessage));
    }

    /**
     * Publishes the {@link Message} onto to the {@link MessageBus}.
     *
     * @param message The {@link Message} to be published
     * @return 0 on success, non-zero otherwise. See <a href="https://github.com/Azure/azure-iot-gateway-sdk/blob/develop/core/devdoc/message_bus_requirements.md" target="_top">Message Bus Documentation</a>.
     * @throws IOException If the {@link Message} cannot be serialized.
     */
    public int publish(Message message) throws IOException {
        return this.bus.publishMessage(message, this._addr);
    }

    //Public getter methods

    final public MessageBus getMessageBus(){
        return bus;
    }

    /**
     * Gets the JSON configuration string for this {@link GatewayModule}
     * @return The JSON configuration for this {@link GatewayModule}
     */
    final public String getConfiguration(){
        return configuration;
    }
}