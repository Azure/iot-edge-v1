/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.remote;

/**
 * A Create message that contains the details to connect to the data channel. 
 *
 */
class CreateMessage extends ControlMessage {

    private final DataEndpointConfig endpointsConfig;
    private final String args;
    private final int version;

    public CreateMessage(DataEndpointConfig endpointsConfig, String args, int version) {
        super(RemoteMessageType.CREATE);
        this.endpointsConfig = endpointsConfig;
        this.args = args;
        this.version = version;
    }

    /**
     * 
     * @return The data channel details
     */
    public DataEndpointConfig getDataEndpoint() {
        return this.endpointsConfig;
    }

    /**
     * 
     * @return Module arguments that were configured in the Gateway 
     */
    public String getArgs() {
        return this.args;
    }

    /**
     * 
     * @return Message version from the Gateway
     */
    public int getVersion() {
        return this.version;
    }
}
