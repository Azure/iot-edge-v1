# Module Requirements

## Overview

The GatewayModule abstract class must be extended by any module written in Java; all 
Java modules must extend from the same abstract class. All methods to be
implemented by the module-creator will be called by the gateway when necessary.

## References

[module.h](../../../../../../../../../core/devdoc/module.md)

## Exposed API
```java
public abstract class GatewayModule {
    protected GatewayModule(long address, MessageBus bus, String configuration);
    abstract void receive(Message message);
    abstract void destroy();
}
```

## Module
```java
public GatewayModule(long address, MessageBus bus, String configuration);
```
**SRS_JAVA_GATEWAY_MODULE_14_001: [** The constructor shall save `address`, `bus`, 
and `configuration` into class variables. **]**

**SRS_JAVA_GATEWAY_MODULE_14_002: [** If `address` or `bus` is `null` the constructor 
shall throw an IllegalArgumentException. **]**

When extending this abstract class, the module-creator must create their own 
constructor which calls this super constructor as the first statement.

## receive
```java
public void receive(Message message);
```
This method must be implemented by the module-creator. The method takes the 
native address of the `MODULE_HANDLE` and a byte array representing the serialized 
`Message`. This method will be called when a message is received for this Module.

## destroy
```java
public void destroy();
```
This method must be implemented by the module-creator. The method takes the
native address of the `MODULE_HANDLE`. This method will be called when the module
is about to be destroyed. Once destroyed, a module can no longer send or receive
messages.