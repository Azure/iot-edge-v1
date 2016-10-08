/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.core;

public interface IGatewayModule {

    /**
     * The create method is called by the subclass constructor when the native Gateway creates the module. The constructor
     * should save the {@code broker} parameter.
     *
     * @param moduleAddr The address of the native module pointer
     * @param broker The {@link Broker} to which this module belongs
     * @param configuration The configuration for this module represented as a JSON string
     */
    void create(long moduleAddr, Broker broker, String configuration);

    /**
     * The start method is called when the native Gateway has created all modules to notify each module that it is safe
     * to start sending messages.
     *
     * This method is optional, but it is recommended that any long-running processes get started in this method.
     */
    void start();

    /**
     * The receive method is called on a {@link GatewayModule} whenever it receives a message.
     *
     * The destroy() and receive() methods are guaranteed to not be called simultaneously.
     *
     * @param source The serialized message
     */
    void receive(byte[] source);

    /**
     * The destroy method is called on a {@link GatewayModule} before it is about to be "destroyed" and removed from the gateway.
     * Once a module is removed from the gateway, it may no longer send or receive messages.
     *
     * The destroy() and receive() methods are guaranteed to not be called simultaneously.
     */
    void destroy();
}