/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.remote;

import java.util.Map;

class NanomsgLibrary {

    public static final int NN_PAIR;
    
    private static Map<String, Integer> symbols;
    private static final int NN_DONTWAIT;
    private static final int EAGAIN;
    private static final int AF_SP;

    static {
        loadNativeLibrary();

        symbols = getSymbols();
        NN_PAIR = symbols.get("NN_PAIR") != null ? symbols.get("NN_PAIR") : 16;
        NN_DONTWAIT = symbols.get("NN_DONTWAIT") != null ? symbols.get("NN_DONTWAIT") : 1;
        EAGAIN = symbols.get("EAGAIN") != null ? symbols.get("EAGAIN") : 11;
        AF_SP = symbols.get("AF_SP") != null ? symbols.get("AF_SP") : 1;
    }

    static void loadNativeLibrary() {
        System.loadLibrary("nanomsg");
        System.loadLibrary("java_nanomsg");
    }

    private native int nn_errno();

    private native String nn_strerror(int errnum);

    private native int nn_socket(int domain, int protocol);

    private native int nn_close(int socket);

    private native int nn_bind(int socket, String address);

    private native int nn_shutdown(int socket, int how);

    private native int nn_send(int socket, byte[] buffer, int flags);

    private native byte[] nn_recv(int socket, int flags);

    private static native Map<String, Integer> getSymbols();

    public int createSocket(int protocol) throws ConnectionException {
        int socket = this.nn_socket(AF_SP, protocol);
        if (socket < 0) {
            throw new ConnectionException(String.format("Error in nn_socket: %s\n", this.nn_strerror(this.nn_errno())));
        }

        return socket;
    }

    public int bind(int socket, String address) throws ConnectionException {
        int endpointId = this.nn_bind(socket, address);

        if (endpointId < 0) {
            int errn = this.nn_errno();
            throw new ConnectionException(String.format("Error: %d - %s\n", errn, this.nn_strerror(errn)));
        }

        return endpointId;
    }

    public void sendMessage(int socket, byte[] message) throws ConnectionException {
        int result = this.nn_send(socket, message, 0);
        if (result < 0) {
            int errn = this.nn_errno();
            throw new ConnectionException(String.format("Error: %d - %s\n", errn, this.nn_strerror(errn)));
        }
    }

    public boolean sendMessageAsync(int socket, byte[] message) throws ConnectionException {
        int result = this.nn_send(socket, message, NN_DONTWAIT);
        if (result < 0) {
            int errn = this.nn_errno();
            if (errn == EAGAIN) {
                return false;
            } else {
                throw new ConnectionException(String.format("Error: %d - %s\n", errn, this.nn_strerror(errn)));
            }
        }
        return true;
    }

    public byte[] receiveMessageNoWait(int socket) throws ConnectionException {
        byte[] messageBuffer = nn_recv(socket, NN_DONTWAIT);

        if (messageBuffer == null) {
            int errn = this.nn_errno();
            if (errn == EAGAIN) {
                return null;
            } else {
                throw new ConnectionException(String.format("Error: %d - %s\n", errn, this.nn_strerror(errn)));
            }
        }
        return messageBuffer;
    }

    public void shutdown(int socket, int endpointId) {
        this.nn_shutdown(socket, endpointId);
    }

    public void closeSocket(int socket) {
        this.nn_close(socket);
    }

}
