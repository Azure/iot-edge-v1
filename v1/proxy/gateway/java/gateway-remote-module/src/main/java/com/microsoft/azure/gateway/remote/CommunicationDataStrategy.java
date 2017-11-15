/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.remote;

import java.nio.ByteBuffer;

/**
 * Communication strategy for data messages from the Gateway.
 *
 */
class CommunicationDataStrategy implements CommunicationStrategy {

	private final int type;
	
	public CommunicationDataStrategy(int type) {
	    this.type = type;
    }

	/**
     * 
     * @return Endpoint type for data channel
     */
	@Override
    public int getEndpointType() {
		return this.type;
	}

	/**
	 * @return deserialized data message
	 */
	@Override
    public RemoteMessage deserializeMessage(ByteBuffer messageBuffer, byte version) throws MessageDeserializationException {
		return new DataMessage(messageBuffer.array());
	}

	/**
	 * Endpoint uri is received from the Gateway already constructed in the right format, this method returns it the way it is.
	 * 
	 * @return endpoint uri
	 */
    @Override
    public String getEndpointUri(String identifier) {
        return identifier;
    }
}
