/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package tests.unit.com.microsoft.azure.gateway.messaging;

import com.microsoft.azure.gateway.messaging.Message;
import mockit.Mocked;
import mockit.NonStrictExpectations;
import org.junit.Test;

import java.io.DataOutputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

import static org.junit.Assert.*;

public class MessageTest {

    public byte[] minimalMessage =
            {
                    (byte) 0xA1, 0x60,      /*header*/
                    0x00, 0x00, 0x00, 14,   /*size of this array*/
                    0x00, 0x00, 0x00, 0x00, /*zero properties*/
                    0x00, 0x00, 0x00, 0x00  /*zero message content size*/
            };

    public byte[] validMessage =
            {
                    (byte) 0xA1, 0x60,       /*header*/
                    0x00, 0x00, 0x00, 64,   /*size of this array*/
                    0x00, 0x00, 0x00, 0x02, /*two properties*/
                    'B','l','e','e','d','i','n','g','E','d','g','e','\0','r','o','c','k','s','\0',
                    'A', 'z','u','r','e',' ','I','o','T',' ','G','a','t','e','w','a','y',' ','i','s','\0','a','w','e','s','o','m','e','\0',
                    0x00, 0x00, 0x00, 0x02,  /*2 message content size*/
                    '3', '4'
            };

    public byte[] validMessagePropertySwap =
            {
                    (byte) 0xA1, 0x60,       /*header*/
                    0x00, 0x00, 0x00, 64,   /*size of this array*/
                    0x00, 0x00, 0x00, 0x02, /*two properties*/
                    'A', 'z','u','r','e',' ','I','o','T',' ','G','a','t','e','w','a','y',' ','i','s','\0','a','w','e','s','o','m','e','\0',
                    'B','l','e','e','d','i','n','g','E','d','g','e','\0','r','o','c','k','s','\0',
                    0x00, 0x00, 0x00, 0x02,  /*2 message content size*/
                    '3', '4'
            };
    
    /*Tests_SRS_JAVA_MESSAGE_14_003: [ The constructor shall save the message content and properties map. ]*/
    @Test
    public void constructorSavesAllData(){
        final String content = "test-content";
        final Map<String, String> properties = new HashMap<String, String>();
        setDefaultProperties(properties, 5);

        Message message = new Message(content.getBytes(), properties);

        byte[] actualContent = message.getContent();
        Map<String, String> actualProperties = message.getProperties();

        assertTrue(Arrays.equals(content.getBytes(), actualContent));
        assertEquals(properties, actualProperties);
    }

    /*Tests_SRS_JAVA_MESSAGE_14_003: [ The constructor shall save the message content and properties map. ]*/
    @Test
    public void constructorSavesAllNullData(){
        final String content = "test-content";
        final Map<String, String> properties = null;

        Message message = new Message(content.getBytes(), properties);

        byte[] actualContent = message.getContent();
        Map<String, String> actualProperties = message.getProperties();

        assertTrue(Arrays.equals(content.getBytes(), actualContent));
        assertNotNull(message.getProperties());
        assertEquals(0, message.getProperties().size());
    }

    /*Tests_SRS_JAVA_MESSAGE_14_002: [ If the byte array is malformed, the function shall throw an IllegalArgumentException. ]*/
    @Test(expected = IllegalArgumentException.class)
    public void constructorThrowsExceptionForZeroInputArray(){
        final byte[] source = {0};

        Message message = new Message(source);
    }

    /*Tests_SRS_JAVA_MESSAGE_14_002: [ If the byte array is malformed, the function shall throw an IllegalArgumentException. ]*/
    @Test(expected = IllegalArgumentException.class)
    public void constructorThrowsExceptionForNullInputArray(){

        byte [] b = null;
        Message message = new Message(b);
    }

    /*Tests_SRS_JAVA_MESSAGE_14_002: [ If the byte array is malformed, the function shall throw an IllegalArgumentException. ]*/
    @Test(expected = IllegalArgumentException.class)
    public void constructorThrowsExceptionForIncompleteInputArray(){
        final byte[] source = {(byte)0xA1, (byte)0x60};

        Message message = new Message(source);
    }

    /*Tests_SRS_JAVA_MESSAGE_14_001: [ The constructor shall create a Message object by deserializing the byte array. ]*/
    @Test
    public void constructorSetsDataFromInputArray_Minimal() throws IOException {

        HashMap<String, String> expectedProperties = new HashMap<String, String>();

        Message message = new Message(minimalMessage);

        assertEquals(expectedProperties, message.getProperties());
        assertEquals(0, message.getContent().length);
    }

    /*Tests_SRS_JAVA_MESSAGE_14_001: [ The constructor shall create a Message object by deserializing the byte array. ]*/
    @Test
    public void constructorSetsDataFromInputArray_Valid() throws IOException {

        Map<String, String> expected = new HashMap<String, String>();
        expected.put("BleedingEdge", "rocks");
        expected.put("Azure IoT Gateway is", "awesome");
        byte[] expectedContent = "34".getBytes();

        Message message = new Message(validMessage);

        Map<String, String> actualProperties = message.getProperties();
        byte[] actualContent = message.getContent();

        assertEquals(expected, actualProperties);
        assertTrue(Arrays.equals(expectedContent, actualContent));
    }

    /*Tests_SRS_JAVA_MESSAGE_14_001: [ The constructor shall create a Message object by deserializing the byte array. ]*/
    @Test
    public void constructorSetsDataFromInputArray_NoProperties2Bytes() throws IOException {

        byte[] notFail__0Property_2bytes =
            {
                (byte)0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 16,   /*size of this array*/
                0x00, 0x00, 0x00, 0x00, /*0 properties*/
                0x00, 0x00, 0x00, 0x02,  /*1 message content size*/
                '3','4'
            };

        Map<String, String> expected = new HashMap<String, String>();
        byte[] expectedContent = "34".getBytes();

        Message message = new Message(notFail__0Property_2bytes);

        Map<String, String> actualProperties = message.getProperties();
        byte[] actualContent = message.getContent();

        assertEquals(expected, actualProperties);
        assertTrue(Arrays.equals(expectedContent, actualContent));

    }

    /*Tests_SRS_JAVA_MESSAGE_14_001: [ The constructor shall create a Message object by deserializing the byte array. ]*/
    @Test
    public void constructorSetsDataFromInputArray_1Property0Bytes() throws IOException {
        byte[] notFail__1Property_0bytes =
            {
                (byte)0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 18,   /*size of this array*/
                0x00, 0x00, 0x00, 0x01, /*one property*/
                '1', '\0', '2', '\0',
                0x00, 0x00, 0x00, 0x00  /*zero message content size*/
            };

        Map<String, String> expected = new HashMap<String, String>();
        expected.put("1", "2");
        byte[] expectedContent = new byte[]{};

        Message message = new Message(notFail__1Property_0bytes);

        Map<String, String> actualProperties = message.getProperties();
        byte[] actualContent = message.getContent();

        assertEquals(expected, actualProperties);
        assertTrue(Arrays.equals(expectedContent, actualContent));
    }

    /*Tests_SRS_JAVA_MESSAGE_14_004: [ The function shall serialize the Message content and properties according to the specification in message.h ]*/
    @Test
    public void toByteArraySerializesMinimalMessageSuccess() throws IOException {
        Message m = new Message(null, null);

        byte[] actualByteArray = m.toByteArray();

        assertTrue(Arrays.equals(minimalMessage, actualByteArray));
    }

    /*Tests_SRS_JAVA_MESSAGE_14_004: [ The function shall serialize the Message content and properties according to the specification in message.h ]*/
    @Test
    public void toByteArraySerializes_NoProperties2Bytes() throws IOException {

        byte[] notFail__0Property_2bytes =
            {
                (byte)0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 16,   /*size of this array*/
                0x00, 0x00, 0x00, 0x00, /*0 properties*/
                0x00, 0x00, 0x00, 0x02,  /*1 message content size*/
                '3','4'
            };

        Map<String, String> properties = new HashMap<String, String>();

        Message m = new Message("34".getBytes(), properties);

        byte[] actualByteArray = m.toByteArray();

        assertTrue(Arrays.equals(notFail__0Property_2bytes, actualByteArray));
    }

    /*Tests_SRS_JAVA_MESSAGE_14_004: [ The function shall serialize the Message content and properties according to the specification in message.h ]*/
    @Test
    public void toByteArraySerializes_1Property0Bytes() throws IOException {

        byte[] notFail__1Property_0bytes =
            {
                (byte)0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 18,   /*size of this array*/
                0x00, 0x00, 0x00, 0x01, /*one property*/
                '1', '\0', '2', '\0',
                0x00, 0x00, 0x00, 0x00  /*zero message content size*/
            };

        Map<String, String> properties = new HashMap<String, String>();
        properties.put("1", "2");

        Message m = new Message(null, properties);

        byte[] actualByteArray = m.toByteArray();

        assertTrue(Arrays.equals(notFail__1Property_0bytes, actualByteArray));
    }

    /*Tests_SRS_JAVA_MESSAGE_14_004: [ The function shall serialize the Message content and properties according to the specification in message.h ]*/
    @Test
    public void toByteArraySerializesMessageSuccess() throws IOException {
        Map<String, String> properties = new HashMap<String, String>();
        properties.put("BleedingEdge", "rocks");
        properties.put("Azure IoT Gateway is", "awesome");

        Message m = new Message("34".getBytes(), properties);

        byte[] actualByteArray = m.toByteArray();

        assertTrue(Arrays.equals(validMessage, actualByteArray) || Arrays.equals(validMessagePropertySwap, actualByteArray));
    }

    /*Tests_SRS_JAVA_MESSAGE_14_005: [ The function shall return throw an IOException if the Message could not be serialized. ]*/
    @Test(expected = IOException.class)
    public void toByteArrayThrowsExceptionOnFailure(
            @Mocked final DataOutputStream mockDataOutputStream)
            throws IOException {
        Message m = new Message(null, null);

        new NonStrictExpectations()
        {
            {
                mockDataOutputStream.writeInt(anyInt);
                result = new IOException();
            }
        };

        m.toByteArray();
    }

    /*Tests_SRS_JAVA_MESSAGE_14_001: [ The constructor shall create a Message object by deserializing the byte array. ]*/
    /*Tests_SRS_JAVA_MESSAGE_14_004: [ The function shall serialize the Message content and properties according to the specification in message.h ]*/
    @Test
    public void chineseCharacterTest() throws IOException {
        Map<String, String> properties = new HashMap<String, String>();
        properties.put("辉煌的混蛋", "辉煌的混蛋");
        properties.put("辉煌的混", "辉煌的混");

        Message m = new Message("辉煌的混蛋".getBytes(), properties);

        byte[] actualByteArray = m.toByteArray();
        Message newMessage = new Message(actualByteArray);

        byte[] actualContent = newMessage.getContent();
        Map<String, String> actualProperties = newMessage.getProperties();

        assertEquals("辉煌的混蛋", actualProperties.get("辉煌的混蛋"));
        assertEquals("辉煌的混", actualProperties.get("辉煌的混"));
        assertTrue(Arrays.equals("辉煌的混蛋".getBytes(), actualContent));
    }

    public void setDefaultProperties(Map<String, String> properties, int numProperties){
        for(int prop = 0; prop < numProperties; prop++){
            properties.put("test-key-"+prop, "test-value-"+prop);
        }
    }

}
