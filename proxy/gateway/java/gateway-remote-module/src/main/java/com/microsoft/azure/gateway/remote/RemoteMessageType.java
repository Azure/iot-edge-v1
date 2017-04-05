/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.remote;

/**
 * A enumeration of the supported control messages. 
 *
 */
enum RemoteMessageType {
    ERROR(0), CREATE(1), REPLY(2), START(3), DESTROY(4);

    private final int value;

    private RemoteMessageType(int value) {
        this.value = value;
    }

    public int getValue() {
        return this.value;
    }

    public static RemoteMessageType fromValue(int value) {
        for (RemoteMessageType type : RemoteMessageType.values()) {
            if (type.value == value) {
                return type;
            }
        }
        return null;
    }
}
