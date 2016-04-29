# SIMULATED DEVICE Module HL Requirements

## Overview
This document describes the SIMULATED DEVICE high level module.  This module adapts
the existing lower layer SIMULATED DEVICE module for use with the Gateway HL library.  
It is mostly a passthrough to the existing module, with a specialized create 
function to interpret the serialized JSON module arguments.

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

This is the primary public interface for the module.  It returns a pointer to 
the `MODULE_APIS` structure containing the implementation functions for this
module. `Module_GetAPIs` returns a non-`NULL` pointer to a MODULE_APIS structure.
The `MODULE_APIS` structure shall have non-`NULL` `Module_Create`, `Module_Destroy`, 
and `Module_Receive` fields.

## SimulatedDevice_HL_Create
```C
MODULE_HANDLE SimulatedDevice_HL_Create(MESSAGE_BUS_HANDLE busHandle, const void* configuration);
```
This function creates the SIMULATED DEVICE HL module. This module reads a JSON 
object and passes the macAddress to the underlying SIMULATED DEVICE module's _Create.

## SimulatedDevice_HL_Destroy
```C
static void SimulatedDevice_HL_Destroy(MODULE_HANDLE moduleHandle);
```
Frees all used resources.

## SimulatedDevice_HL_Receive
```C
static void SimulatedDevice_HL_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle);
```
Calls underlying module (SIMULATED DEVICE) _Receive function.