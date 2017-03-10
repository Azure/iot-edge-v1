/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.remote;

/**
 * Thrown to indicate that an instance of the module could not be created.
 */
class ModuleInstantiationException extends Exception {

    private static final long serialVersionUID = -5305907884031406319L;

    public ModuleInstantiationException() {
        super();
    }

    public ModuleInstantiationException(String message) {
        super(message);
    }

    public ModuleInstantiationException(Throwable cause) {
        super(cause);
    }

    public ModuleInstantiationException(String message, Throwable cause) {
        super(message, cause);
    }

    public ModuleInstantiationException(String message, Throwable cause, boolean enableSuppression,
            boolean writableStackTrace) {
        super(message, cause, enableSuppression, writableStackTrace);
    }

}
