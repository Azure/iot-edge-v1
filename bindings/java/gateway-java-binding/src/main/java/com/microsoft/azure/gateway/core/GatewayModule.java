/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.core;

import com.microsoft.azure.gateway.messaging.Message;

import java.io.IOException;

/**
 * The Abstract Module class to be extended by the module-creator when creating any modules.
 */
public abstract class GatewayModule implements IGatewayModule{

    public abstract void receive(Message message);
    public abstract void destroy();

    /** The {@link MessageBus} to which this module belongs */
    private MessageBus bus;

    /** The module-specific configuration object. */
    private String configuration;

    /**
     * Constructs a {@link GatewayModule} from the provided address and {@link MessageBus}. A {@link GatewayModule} should always call this super
     * constructor before any module-specific constructor code.
     *
     * @param bus The {@link MessageBus} to which this module belongs
     * @param configuration The module-specific configuration
     */
    public GatewayModule(MessageBus bus, String configuration){
        /*Codes_SRS_JAVA_GATEWAY_MODULE_14_002: [ If address or bus is null the constructor shall throw an IllegalArgumentException. ]*/
        if(bus == null){
            throw new IllegalArgumentException("Address is invalid or MessageBus is null.");
        }

        /*Codes_SRS_JAVA_GATEWAY_MODULE_14_001: [ The constructor shall save address, bus, and configuration into class variables. ]*/
        this.create(bus, configuration);
    }

    public void create(MessageBus bus, String configuration){
        this.bus = bus;
        this.configuration = configuration;
    }

    public void receive(byte[] serializedMessage){
        this.receive(new Message(serializedMessage));
    }

    //Public getter methods

    final public MessageBus getMessageBus(){
        return bus;
    }

    final public String getConfiguration(){
        return configuration;
    }
}