Module Configuration Requirements
===================

## Overview

A remote module configuration that should contain the identifier used to connect to the Gateway, the module class and the message version. The identifier has to be the same value that was configured on the Gateway side (more details about configuration can be found in Java Binding High Level Design). The module class has to be an implementation of IGatewayModule. Version is the messages version that has to be compatible with the message version the Gateway is sending. Default value is set to 1.

## References

[Java Binding High Level Design](../../../../bindings/java/devdoc/java_binding_hld.md)

## Exposed API

``` java
public class ModuleConfiguration {

    public String getIdentifier();
    public Class<? extends IGatewayModule> getModuleClass();
    public byte getVersion();

    public static class Builder {
        public Builder();
        public Builder setModuleClass(Class<? extends IGatewayModule> moduleClass);
        public Builder setIdentifier(String identifier);
        public Builder setModuleVersion(byte version);

        public ModuleConfiguration build();
}
```

## ModuleConfiguration
```java
private ProxyGateway(/* ... */);
```
**SRS_MODULE_CONFIGURATION_24_001: [** ModuleConfiguration shall not have a private constructor . **]**

**SRS_MODULE_CONFIGURATION_24_002: [** ModuleConfiguration shall have a Builder that creates new instances of ModuleConfiguration . **]**

## getIdentifier
```java
public String getIdentifier();
```

**SRS_MODULE_CONFIGURATION_24_003: [** It shall return the control channel identification. **]**

## getModuleClass
```java
public Class<? extends IGatewayModule> getModuleClass()
```
**SRS_MODULE_CONFIGURATION_24_004: [** It shall return the module class. **]**

## getVersion
```java
public byte getVersion();
```
**SRS_MODULE_CONFIGURATION_24_005: [** It shall return the version. **]**

## ModuleConfiguration.Builder 

## setModuleClass

```java
public Builder setModuleClass(Class<? extends IGatewayModule> moduleClass);
```
**SRS_MODULE_CONFIGURATION_BUILDER_24_006: [** It shall save `moduleClass` into member field.  **]**

## setIdentifier

```java
public Builder setIdentifier(String identifier);
```
**SRS_MODULE_CONFIGURATION_BUILDER_24_007: [** It shall save `identifier` into member field.  **]**


## setModuleVersion
```java
public Builder setModuleVersion(byte version);
```
**SRS_MODULE_CONFIGURATION_BUILDER_24_008: [** It shall save `version` into member field.  **]**

## build

**SRS_MODULE_CONFIGURATION_BUILDER_24_009: [** It shall create a new ModuleConfiguration instance. **]**

**SRS_MODULE_CONFIGURATION_BUILDER_24_010: [** If identifier is null it shall throw IllegalArgumentException. **]**

**SRS_MODULE_CONFIGURATION_BUILDER_24_011: [** If moduleClass is null it shall throw IllegalArgumentException. **]**

**SRS_MODULE_CONFIGURATION_BUILDER_24_012: [** If version is negative it shall throw IllegalArgumentException. **]**

**SRS_MODULE_CONFIGURATION_BUILDER_24_013: [** If version is not set it shall use default version. **]**