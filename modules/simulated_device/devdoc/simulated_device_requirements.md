# Simulated Device Module Requirements

## Overview
This document describes the simulated device module.  

The simulated module will register the device on start, then periodically publish a telemetry message.

## Reference

[module.h](../../../core/devdoc/module.md)

### Expected Arguments

The argument to this module is a JSON object with the following structure:
```json
{
    "macAddress" : "<mac address in canonical form>",
    "messagePeriod" : <integer value>
}
```
### Example Arguments
```json
{
    "macAddress" : "01:01:01:01:01:01",
    "messagePeriod" : 2000
}
```

##Exposed API
```c
MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version);
```

## Module_GetApi

This is the primary public interface for the module.  It fills out the
provided `MODULE_API` structure containing the implementation functions for this
module.
The `MODULE_API` structure shall have non-`NULL` `Module_Create`, `Module_Start`, `Module_Destroy`, and `Module_Receive` fields.

## SimulatedDevice_CreateFromJson
```C
MODULE_HANDLE SimulatedDevice_CreateFromJson(BROKER_HANDLE broker, const char* configuration);
```
This function creates the simulated device module.
