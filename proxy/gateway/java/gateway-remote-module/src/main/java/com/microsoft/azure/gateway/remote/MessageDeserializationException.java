/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.remote;

/**
 * Thrown to indicate that the message retrieved from the Gateway can not be
 * deserialized.
 */
class MessageDeserializationException extends Exception {

	private static final long serialVersionUID = 375190278270633771L;

	public MessageDeserializationException() {
		super();
	}

	public MessageDeserializationException(String message) {
		super(message);
	}

	public MessageDeserializationException(Throwable cause) {
		super(cause);
	}

	public MessageDeserializationException(String message, Throwable cause) {
		super(message, cause);
	}

	public MessageDeserializationException(String message, Throwable cause, boolean enableSuppression,
			boolean writableStackTrace) {
		super(message, cause, enableSuppression, writableStackTrace);
	}
}
