/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.messaging;

import java.io.*;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

public final class Message {

    private Map<String, String> properties;

    private byte[] content;

    /**
     * Constructor for a {@link Message} with {@code content} {@link byte[]} and {@link Map} {@code properties}.
     *
     * Both {@code content} and {@code properties} may be null.
     *
     * @param content The message content as a string. Null creates an empty content {@link byte[]}
     * @param properties The string to string map of peroperties for this message. Null creates an empty {@link Map}
     */
    public Message(byte[] content, Map<String, String> properties){
        /*Codes_SRS_JAVA_MESSAGE_14_003: [ The constructor shall save the message content and properties map. ]*/
        this.content = content != null ? content : new byte[0];
        this.properties = properties != null ? properties : new HashMap<String, String>();
    }

    /**
     * Construcor for a {@link Message} created from a fully and properly serialized message {@link byte[]}.
     *
     * @see <a href="https://github.com/Azure/azure-iot-gateway-sdk/blob/develop/core/devdoc/message_requirements.md" target="_top">Message Documentation</a>
     *
     * @param serializedMessage The fully serialized message.
     *
     * @throws IllegalArgumentException If the {@link byte[]} cannot be de-serialized.
     */
    public Message(byte[] serializedMessage){
        try {
            /*Codes_SRS_JAVA_MESSAGE_14_001: [ The constructor shall create a Message object by deserializing the byte array. ]*/
            /*Codes_SRS_JAVA_MESSAGE_14_002: [ If the byte array is malformed, the function shall throw an IllegalArgumentException. ]*/
            fromByteArray(serializedMessage);
        } catch (IOException e) {
            throw new IllegalArgumentException("Invalid byte array input.");
        }
    }

    /**
     * Serializes the {@link Message} to a {@link byte[]}.
     *
     * @see <a href="https://github.com/Azure/azure-iot-gateway-sdk/blob/develop/core/devdoc/message_requirements.md" target="_top">Message Documentation</a>
     *
     * @return The serialized {@link byte[]}.
     * @throws IOException If this {@link Message} cannot be serialized.
     */
    public byte[] toByteArray() throws IOException {
        /*Codes_SRS_JAVA_MESSAGE_14_004: [ The function shall serialize the Message content and properties according to the specification in message.h ]*/
        /*Codes_SRS_JAVA_MESSAGE_14_005: [ The function shall return throw an IOException if the Message could not be serialized. ]*/
        ByteArrayOutputStream _bos = new ByteArrayOutputStream();
        DataOutputStream _dos = new DataOutputStream(_bos);

        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        DataOutputStream dos = new DataOutputStream(bos);

        //Write Properties Count
        _dos.writeInt(this.properties.size());

        //Write Properties
        Object[] keys = this.properties.keySet().toArray();
        for(int index = 0; index < keys.length; index++){
            byte[] key = ((String)keys[index]).getBytes("UTF-8");
            _dos.write(key);
            _dos.writeByte('\0');
            byte[] value = this.properties.get(keys[index]).getBytes("UTF-8");
            _dos.write(value);
            _dos.writeByte('\0');
        }

        //Write message content size
        _dos.writeInt(this.content.length);

        //Write message content
        _dos.write(this.content);

        byte[] _result = _bos.toByteArray();

        //Write Header
        dos.writeByte(0xA1);
        dos.writeByte(0x60);

        //Write ArraySize
        dos.writeInt(_result.length + 6);
        dos.write(_result);
        byte[] result = bos.toByteArray();

        return result;
    }

    public Map<String, String> getProperties(){
        return properties;
    }

    public byte[] getContent(){
        return content;
    }

    public String toString(){
        return "Content: " + new String(this.content) + "\nProperties: " + this.properties.toString();
    }

    /**
     * Deserializes a byte array and sets the {@link Message#content} and {@link Message#properties}.
     *
     * @param serializedMessage The message to be deserialized.
     * @throws IOException if the byte array in malformed.
     */
    private void fromByteArray(byte[] serializedMessage) throws IOException {
        try {
            ByteArrayInputStream bis = new ByteArrayInputStream(serializedMessage);
            DataInputStream dis = new DataInputStream(bis);

            //Get Header
            byte header1 = dis.readByte();
            byte header2 = dis.readByte();
            if (header1 == (byte) 0xA1 && header2 == (byte) 0x60) {
                int arraySize = dis.readInt();
                if (arraySize >= 14) {
                    Map<String, String> _properties = new HashMap<String, String>();
                    int propCount = dis.readInt();

                    if (propCount > 0) {
                        for (int count = 0; count < propCount; count++) {
                            byte[] key = readNullTerminatedString(bis);
                            byte[] value = readNullTerminatedString(bis);
                            _properties.put(new String(key), new String(value));
                        }
                    }

                    int contentLength = dis.readInt();
                    byte[] content = new byte[contentLength];
                    dis.readFully(content);

                    //At this point it should be safe to set both properties and content
                    this.properties = _properties;
                    this.content = content;
                } else {
                    throw new IOException("Invalid byte array size.");
                }
            } else {
                throw new IOException("Invalid byte array header.");
            }
        } catch(Exception e){
            throw new IOException("Unexpected exception occurred: " + e.getMessage());
        }
    }

    /**
     * Returns the first null-terminated ('\0') sub-array.
     *
     * @param bis The {@link ByteArrayInputStream} object from which to read the string.
     * @return The null-terminated string in a byte array.
     * @throws IOException if the null-terminated string could not be read.
     */
    private byte[] readNullTerminatedString(ByteArrayInputStream bis) throws IOException {
        ArrayList<Byte> byteArray = new ArrayList<Byte>();
        byte b = (byte) bis.read();

        while(b != '\0' && b != -1){
            byteArray.add(b);
            b = (byte)bis.read();
        }

        byte[] result;

        if(b != -1) {

            result = new byte[byteArray.size()];
            for (int index = 0; index < result.length; index++) {
                result[index] = byteArray.get(index);
            }
        } else {
            throw new IOException("Could not read null-terminated string.");
        }

        return result;
    }
}
