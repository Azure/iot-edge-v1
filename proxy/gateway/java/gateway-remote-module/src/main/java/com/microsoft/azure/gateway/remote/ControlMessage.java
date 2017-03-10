/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.remote;

/**
 * A control message that can represent a Create, Start, Destroy message
 * See {@link RemoteMessageType} for supported types.
 *
 */
class ControlMessage extends RemoteMessage {

    private RemoteMessageType messageType;

    public ControlMessage(RemoteMessageType type) {
        this.messageType = type;
    }

    public RemoteMessageType getMessageType() {
        return this.messageType;
    }

}
