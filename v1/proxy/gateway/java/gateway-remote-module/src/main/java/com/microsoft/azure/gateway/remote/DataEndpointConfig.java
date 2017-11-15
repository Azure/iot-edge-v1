/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.remote;

/**
 * Configuration for the data channel. It contains an identification and a type
 * that are used to create the communication data channel. These configuration
 * details are received from the Gateway.
 *
 */
class DataEndpointConfig {
    private final String id;
    private final int type;

    public DataEndpointConfig(String id, int type) {
        this.id = id;
        this.type = type;
    }

    /**
     * 
     * @return Identification
     */
    public String getId() {
        return this.id;
    }

    /**
     * 
     * @return type that represents the communication channel type
     */
    public int getType() {
        return this.type;
    }
}
