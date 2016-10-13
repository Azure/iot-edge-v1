# Simulated Device Module Requirements

## Overview
This document describes the simulated device module.

## Reference

[module.h](../../../../devdoc/module.md)

### Expected Arguments

The argument to this module is a JSON object with the following structure:
```json
{
    "macAddress" : "<mac address in canonical form>"
}
```
### Example Arguments
```json
{
    "macAddress" : "01:01:01:01:01:01"
}
```

##Exposed API
```c
MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void);
```

## Module_GetAPIs

This is the primary public interface for the module.  It fills out the
provided `MODULE_APIS` structure containing the implementation functions for this
module.
The `MODULE_APIS` structure shall have non-`NULL` `Module_Create`, `Module_Destroy`, 
and `Module_Receive` fields.

## SimulatedDevice_CreateFromJson
```C
MODULE_HANDLE SimulatedDevice_CreateFromJson(BROKER_HANDLE broker, const char* configuration);
```
This function creates the simulated device module.
