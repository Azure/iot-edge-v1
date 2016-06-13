/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.messaging;

import java.io.Serializable;
import java.util.Map;

public final class Message implements Serializable{

    private Map<String, String> properties;

    private byte[] content;

    public Message(String content, Map<String, String> properties){

    }

    public Message(String content){

    }

    public Message(byte[] serializedMessage){

    }

    public Map<String, String> getProperties(){
        return null;
    }

    public String getContent(){
        return null;
    }

    public byte[] toByteArray(){
        return null;
    }
}
