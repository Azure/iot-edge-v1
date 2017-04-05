/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.remote;

/**
 * An enumeration of the supported reply statuses 
 *
 */
enum RemoteModuleReplyCode {
    DETACH(-1), OK(0), CONNECTION_ERROR(1), CREATION_ERROR(2);

    private final int value;

    private RemoteModuleReplyCode(int value) {
        this.value = value;
    }

    public int getValue() {
        return this.value;
    }
}
