# Message Requirements

## Overview

A message to be used by the Java module implementer. A message will always be serialized before publishing to the native gateway message bus.

## References

[message.h](../../../../../../../../../core/devdoc/message_requirements.md)

## Exposed API
```java
public final class Message implements Serializable{

    private Map<String, String> properties;

    private String content;

    public Message(byte[] content, Map<String, String> properties);
    public Message(byte[] serializedMessage);
    public Map<String, String> getProperties();
    public String getContent();
    public byte[] toByteArray();
}
```

##  Message
```java
public Message(byte[] serializedMessage | byte[] content, Map<String, String> properties);
```
**SRS_JAVA_MESSAGE_14_001: [** The constructor shall create a Message object by deserializing the byte array. **]**

**SRS_JAVA_MESSAGE_14_002: [** If the byte array is malformed, the function shall throw an IllegalArgumentException. **]**

**SRS_JAVA_MESSAGE_14_003: [** The constructor shall save the message content and properties map. **]**

## toByteArray
```java
public byte[] toByteArray();
```
**SRS_JAVA_MESSAGE_14_004: [** The function shall serialize the Message content and properties according to the specification in [message.h](../../../../../../../../../core/devdoc/message_requirements.md) **]**

**SRS_JAVA_MESSAGE_14_005: [** The function shall return throw an IOException if the Message could not be serialized. **]**