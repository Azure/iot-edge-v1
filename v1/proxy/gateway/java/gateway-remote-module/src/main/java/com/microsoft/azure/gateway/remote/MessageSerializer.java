/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.remote;

import java.nio.ByteBuffer;

/**
 * Serializer for control messages
 *
 */
class MessageSerializer {

	// 0xA1 comes from (A)zure (I)oT
	private static final byte FIRST_MESSAGE_BYTE = (byte) 0xA1;
	// 0x6C comes from (G)ateway control message
	private static final byte SECOND_MESSAGE_BYTE = (byte) 0x6C;

	/**
	 * Serialize the control message with status and the version 
	 * @param status Status to be send to the Gateway. {@see RemoteModuleReplyCode}
	 * @param version Message version 
	 * @return
	 */
	public byte[] serializeMessage(int status, byte version) {
		byte[] array = new byte[9];
		ByteBuffer dos = ByteBuffer.wrap(array);

		// Write Header
		dos.put(FIRST_MESSAGE_BYTE);
		dos.put(SECOND_MESSAGE_BYTE);
		dos.put(version);
		dos.put((byte) RemoteMessageType.REPLY.getValue());
		int totalSize = dos.limit();
		dos.putInt(totalSize);

		// Write content
		dos.put((byte) status);

		return dos.array();
	}
}
