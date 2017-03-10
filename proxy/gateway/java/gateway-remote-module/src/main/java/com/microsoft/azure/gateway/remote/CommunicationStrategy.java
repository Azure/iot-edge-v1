/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.remote;

import java.nio.ByteBuffer;

interface CommunicationStrategy {
    RemoteMessage deserializeMessage(ByteBuffer buffer, byte version) throws MessageDeserializationException;
    int getEndpointType();
    String getEndpointUri(String identifier);
}
