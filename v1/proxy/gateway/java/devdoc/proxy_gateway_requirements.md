Java Proxy Gateway Requirements
===================

## Overview

This is the API documentation for a remote module that communicates with other modules through the Azure IoT Gateway. Remote modules need to implement `IGatewayModule` interface or extend `GatewayModule` abstract class. Each remote module will additionally need to `attach` and `detach` from the gateway.

## References

[Java Binding High Level Design](../../../../bindings/java/devdoc/java_binding_hld.md)

## Exposed API

``` java
public class ProxyGateway {
	private ModuleConfiguration config;
	public ProxyGateway(ModuleConfiguration config);
	public void attach();
	public void detach();
}
```

## ProxyGateway
```java
public ProxyGateway(ModuleConfiguration config);
```
**SRS_JAVA_PROXY_GATEWAY_24_001: [** If `config` is `null` the constructor shall throw an IllegalArgumentException. **]**

**SRS_JAVA_PROXY_GATEWAY_24_002: [** The constructor shall save `config` for later use. **]**

## attach

Attach the remote module to the Gateway. It creates the communication channel with the Gateway and starts a new thread that is listening for messages from the Gateway. 
When the Gateway sends a 'CREATE' message which contains the detail for creating the message channel and module arguments, ProxyGateway shall create a new instance of the module, call create method from the module and create the message channel. When the initialization is done, ProxyGateway shall send a notification message to the Gateway. When the Gateway sends a `START` message, GatewayProxy shall call module `start` so the module can start listening for messages from other modules or send messages to other modules.

```java
public void attach();
```

**SRS_JAVA_PROXY_GATEWAY_24_003: [** If already attached the function shall do nothing and return.  **]**

**SRS_JAVA_PROXY_GATEWAY_24_004: [** It shall instantiate the messages listener task.  **]**

**SRS_JAVA_PROXY_GATEWAY_24_005: [** It shall instantiate a single-threaded executor that can schedule the task to execute periodically.  **]**

**SRS_JAVA_PROXY_GATEWAY_24_006: [** It shall start executing the periodic task of listening for messages from the Gateway. **]**

**SRS_JAVA_PROXY_GATEWAY_24_007: [** *Message Listener task* - It shall create a new control channel with the Gateway. **]**

**SRS_JAVA_PROXY_GATEWAY_24_008: [** *Message Listener task* - It shall connect to the control channel. **]**

**SRS_JAVA_PROXY_GATEWAY_24_009: [** *Message Listener task* - If the connection with the control channel fails, it shall throw ConnectionException.  **]**

**SRS_JAVA_PROXY_GATEWAY_24_010: [** *Message Listener task* - It shall poll the gateway control channel for new messages. **]**

**SRS_JAVA_PROXY_GATEWAY_24_011: [** *Message Listener task* - If no message is available the listener shall do nothing. **]**

**SRS_JAVA_PROXY_GATEWAY_24_012: [** *Message Listener task* - If a control message is received and deserialization fails it shall send an error message to the Gateway. **]**

**SRS_JAVA_PROXY_GATEWAY_24_013: [** *Message Listener task* - If there is an error receiving the control message it shall send an error message to the Gateway. **]**

**SRS_JAVA_PROXY_GATEWAY_24_014: [** *Message Listener task* - If the message type is CREATE, it shall process the create message **]**

**SRS_JAVA_PROXY_GATEWAY_24_015: [** *Message Listener task - Create message* - Create message processing shall create the message channel and connect to it. **]**

**SRS_JAVA_PROXY_GATEWAY_24_016: [** *Message Listener task - Create message* - If connection to the message channel fails, it shall send an error message to the Gateway. **]**

**SRS_JAVA_PROXY_GATEWAY_24_017: [** *Message Listener task - Create message* - If the creation process has already occurred, it shall call module destroy, disconnect from the message channel and continue processing the creation message **]**

**SRS_JAVA_PROXY_GATEWAY_24_018: [** *Message Listener task - Create message* - Create message processing shall create a module instance and call `create` method. **]**
 
**SRS_JAVA_PROXY_GATEWAY_24_019: [** *Message Listener task - Create message* - If module instance creation fails, it shall send an error message to the Gateway. **]**

**SRS_JAVA_PROXY_GATEWAY_24_020: [** *Message Listener task - Create message* - If the Create message finished processing, it shall send an ok message to the Gateway. **]**

**SRS_JAVA_PROXY_GATEWAY_24_021: [** *Message Listener task - Create message* - If ok message fails to be send to the Gateway, it shall do call module `destroy` and disconnect from message channel. **]**

**SRS_JAVA_PROXY_GATEWAY_24_022: [** *Message Listener task - Start message* - If message type is START, it shall call module `start` method. **]**

**SRS_JAVA_PROXY_GATEWAY_24_023: [** *Message Listener task - Destroy message* - If message type is DESTROY, it shall call module `destroy` method. **]**

**SRS_JAVA_PROXY_GATEWAY_24_024: [** *Message Listener task - Destroy message* - If message type is DESTROY, it shall disconnect from the message channel. **]**

**SRS_JAVA_PROXY_GATEWAY_24_025: [** *Message Listener task - Data message* - It shall not check for messages, if the message channel is not available. **]**

**SRS_JAVA_PROXY_GATEWAY_24_026: [** *Message Listener task - Data message* - If data message is received, it shall forward it to the module by calling `receive` method. **]**

**SRS_JAVA_PROXY_GATEWAY_24_027: [** *Message Listener task - Data message* - If no data message is received or if an error occurs, it shall do nothing. **]**


## detach
```java
public void detach();
```

Detach from the Gateway. The listening of messages from the Gateway is terminated and destroy method on IGatewayModule instance is called. A Destroy message is sent to the Gateway. 

**SRS_JAVA_PROXY_GATEWAY_24_028: [** If not attached the function shall do nothing and return. **]**

**SRS_JAVA_PROXY_GATEWAY_24_029: [** It shall attempt to stop the message listening tasks. **]**

**SRS_JAVA_PROXY_GATEWAY_24_030: [** It shall wait for a minute for executing task to terminate. **]**

**SRS_JAVA_PROXY_GATEWAY_24_031: [** It shall call module destroy. **]**

**SRS_JAVA_PROXY_GATEWAY_24_032: [** It shall attempt to notify the Gateway of the detachment. **]**

**SRS_JAVA_PROXY_GATEWAY_24_033: [** It shall disconnect from the Gateway control channel. **]**

**SRS_JAVA_PROXY_GATEWAY_24_034: [** It shall disconnect from the Gateway message channel. **]**

