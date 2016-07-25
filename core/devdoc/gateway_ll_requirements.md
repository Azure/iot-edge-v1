# Gateway_LL Requirements

## Overview
This is the API to create and manage a gateway. Contained within a gateway is a message bus onto which any number of modules may be loaded. This gateway API is essentially a convenience layer facilitating the easy addition and removal of modules from the message bus.

## References

##Gateway Handle Implementation

This section details the internal structure defined in the gateway implementation used to track a gateway and the information it contains.

```
typedef struct GATEWAY_HANDLE_DATA_TAG {
    /** @brief Vector of MODULE_DATA modules that the Gateway must track */
    VECTOR_HANDLE modules;
    
    /** @brief The message bus contained within this Gateway */
    MESSAGE_BUS_HANDLE bus;    
} GATEWAY_HANDLE;
```

## Exposed API
```
#ifndef GATEWAY_LL_H
#define GATEWAY_LL_H

#include "macro_utils.h"
#include "module.h"
#include "azure_c_shared_utility/vector.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct GATEWAY_HANDLE_DATA_TAG* GATEWAY_HANDLE;

/** @brief This struct represents a single entry of the GATEWAY_PROPERTIES. */
typedef struct GATEWAY_PROPERTIES_ENTRY_TAG
{
    /** @brief The (possibly NULL) name of the module */
    const char* module_name;
    
    /** @brief The path to the .dll or .so of the module */
    const char* module_path;
    
    /** @brief The user-defined properties object for the module */
    const void* module_properties;
} GATEWAY_PROPERTIES_ENTRY;

/** @brief This struct represents the properties that should be used when creating a module. */
typedef struct GATEWAY_PROPERTIES_DATA_TAG
{
    /** @brief Vector of GATEWAY_PROPERTIES_ENTRY objects. */
    VECTOR_HANDLE gateway_properties_entries;
} GATEWAY_PROPERTIES;

/** @breif Creates a new gateway using the provided GATEWAY_PROPERTIES and returns a GATEWAY_HANDLE for the newly created gateway */
extern GATEWAY_HANDLE Gateway_LL_Create(const GATEWAY_PROPERTIES* properties);

/** @breif Creates a new gateway using the provided VECTOR_HANDLE of MODULE instances, MESSAGE_BUS_HANDLE, and returns a GATEWAY_HANDLE for the newly created gateway */
extern GATEWAY_HANDLE Gateway_LL_UwpCreate(const VECTOR_HANDLE modules, MESSAGE_BUS_HANDLE bus);

/** @brief Destroys gw and all associated data. */
extern void Gateway_LL_Destroy(GATEWAY_HANDLE gw);

/** @brief Destroys gw and all associated data. */
extern void Gateway_LL_UwpDestroy(GATEWAY_HANDLE gw);

/** @brief Creates a new module based on the GATEWAY_PROPERTIES_ENTRY* and returns a MODULE_HANDLE if successful, NULL otherwise. */
extern MODULE_HANDLE Gateway_LL_AddModule(GATEWAY_HANDLE gw, const GATEWAY_PROPERTIES_ENTRY* entry);

/** @brief Removes the provided module from the gateway */
extern void Gateway_LL_RemoveModule(GATEWAY_HANDLE gw, MODULE_HANDLE module);

#ifdef __cplusplus
}
#endif

#endif // GATEWAY_LL_H

```

##Gateway_Create
```
extern GATEWAY_HANDLE Gateway_LL_Create(const GATEWAY_PROPERTIES* properties);
```
Gateway_LL_Create creates a new gateway using information about modules in the `GATEWAY_PROPERTIES` struct to initialize modules on the message bus.

**SRS_GATEWAY_LL_14_001: [** This function shall create a `GATEWAY_HANDLE` representing the newly created gateway. **]**

**SRS_GATEWAY_LL_14_002: [** This function shall return `NULL` upon any memory allocation failure. **]**

**SRS_GATEWAY_LL_14_003: [** This function shall create a new `MESSAGE_BUS_HANDLE` for the gateway representing this gateway's message bus. **]**

**SRS_GATEWAY_LL_14_004: [** This function shall return `NULL` if a `MESSAGE_BUS_HANDLE` cannot be created. **]**

**SRS_GATEWAY_LL_14_033: [** The function shall create a vector to store each `MODULE_DATA`. **]**

**SRS_GATEWAY_LL_14_034: [** This function shall return `NULL` if a `VECTOR_HANDLE` cannot be created. **]**

**SRS_GATEWAY_LL_14_035: [** This function shall destroy the previously created `MESSAGE_BUS_HANDLE` and free the `GATEWAY_HANDLE` if the `VECTOR_HANDLE` cannot be created. **]**

**SRS_GATEWAY_LL_14_009: [** The function shall use each `GATEWAY_PROPERTIES_ENTRY` use each of `GATEWAY_PROPERTIES`'s `gateway_properties_entries` to create and add a `MODULE_HANDLE` to the `GATEWAY_HANDLE` message bus. **]**

**SRS_GATEWAY_LL_14_036: [** If any `MODULE_HANDLE` is unable to be created from a `GATEWAY_PROPERTIES_ENTRY` the `GATEWAY_HANDLE` will be destroyed. **]**

```
extern GATEWAY_HANDLE Gateway_LL_UwpCreate(const VECTOR_HANDLE modules, MESSAGE_BUS_HANDLE bus);
```
Gateway_LL_UwpCreate creates a new gateway using modules in the `VECTOR_HANDLE` struct and the message bus described by the `MESSAGE_BUS_HANDLE` to configure the modules on the message bus.

**SRS_GATEWAY_LL_99_001: [** This function shall create a `GATEWAY_HANDLE` representing the newly created gateway. **]**

**SRS_GATEWAY_LL_99_002: [** This function shall return `NULL` upon any memory allocation failure. **]**

**SRS_GATEWAY_LL_99_003: [** This function shall return `NULL` if a `NULL` `MESSAGE_BUS_HANDLE` is received. **]**

**SRS_GATEWAY_LL_99_004: [** This function shall return `NULL` if a `NULL` `VECTOR_HANDLE` is received. **]**

**SRS_GATEWAY_LL_99_005: [** The function shall increment the MESSAGE_BUS_HANDLE reference count if the MODULE_HANDLE was successfully linked to the GATEWAY_HANDLE_DATA's bus. **]**

##Gateway_Destroy
```
extern void Gateway_LL_Destroy(GATEWAY_HANDLE gw);
```
Gateway_LL_Destroy destroys a gateway represented by the `gw` parameter.

**SRS_GATEWAY_LL_14_005: [** If `gw` is `NULL` the function shall do nothing. **]**

**SRS_GATEWAY_LL_14_028: [** The function shall remove each module in `GATEWAY_HANDLE_DATA`'s `modules` vector and destroy `GATEWAY_HANDLE_DATA`'s `modules`. **]**

**SRS_GATEWAY_LL_14_037: [** If `GATEWAY_HANDLE_DATA`'s message bus cannot unlink module, the function shall log the error and continue unloading the module from the `GATEWAY_HANDLE`. **]**

**SRS_GATEWAY_LL_14_006: [** The function shall destroy the `GATEWAY_HANDLE_DATA`'s `bus` `MESSAGE_BUS_HANDLE`. **]**

```
extern void Gateway_LL_UwpDestroy(GATEWAY_HANDLE gw);
```
Gateway_LL_UwpDestroy destroys a gateway represented by the `gw` parameter.

**SRS_GATEWAY_LL_99_006: [** If `gw` is `NULL` the function shall do nothing. **]**

**SRS_GATEWAY_LL_99_007: [** The function shall unlink module from the GATEWAY_HANDLE_DATA's bus MESSAGE_BUS_HANDLE. **]**

**SRS_GATEWAY_LL_99_008: [** If GATEWAY_HANDLE_DATA's bus cannot unlink module, the function shall log the error and continue unloading the module from the GATEWAY_HANDLE. **]**

**SRS_GATEWAY_LL_99_009: [** The function shall decrement the MESSAGE_BUS_HANDLE reference count. **]**

**SRS_GATEWAY_LL_99_010: [** The function shall destroy the `GATEWAY_HANDLE_DATA`'s `bus` `MESSAGE_BUS_HANDLE`. **]**

##Gateway_AddModule
```
extern MODULE_HANDLE Gateway_LL_AddModule(GATEWAY_HANDLE gw, const GATEWAY_PROPERTIES_ENTRY* entry);
```
Gateway_LL_AddModule adds a module to the gateway message bus using the provided `GATEWAY_PROPERTIES_ENTRY`'s `module_path` and `GATEWAY_PROPERTIES_ENTRY`'s `module_properties`.

**SRS_GATEWAY_LL_14_011: [** If `gw`, `entry`, or `GATEWAY_PROPERTIES_ENTRY`'s `module_path` is `NULL` the function shall return `NULL`. **]**

**SRS_GATEWAY_LL_14_012: [** The function shall load the module located at `GATEWAY_PROPERTIES_ENTRY`'s `module_path` into a `MODULE_LIBRARY_HANDLE`. **]**

**SRS_GATEWAY_LL_14_031: [** If unsuccessful, the function shall return `NULL`. **]**

**SRS_GATEWAY_LL_14_013: [** The function shall get the `const MODULE_APIS*` from the `MODULE_LIBRARY_HANDLE`. **]**

**SRS_GATEWAY_LL_14_015: [** The function shall use the `MODULE_APIS` to create a `MODULE_HANDLE` using the `GATEWAY_PROPERTIES_ENTRY`'s `module_properties`. **]**

**SRS_GATEWAY_LL_14_016: [** If the module creation is unsuccessful, the function shall return `NULL`. **]**

**SRS_GATEWAY_LL_14_017: [** The function shall link the module to the `GATEWAY_HANDLE_DATA`'s `bus` using a call to `MessageBus_AddModule`. **]**

**SRS_GATEWAY_LL_14_039: [** The function shall increment the `MESSAGE_BUS_HANDLE` reference count if the `MODULE_HANDLE` was successfully linked to the `GATEWAY_HANDLE_DATA`'s `bus`. **]**

**SRS_GATEWAY_LL_14_018: [** If the message bus linking is unsuccessful, the function shall return `NULL`. **]**

**SRS_GATEWAY_LL_14_029: [** The function shall create a new `MODULE_DATA` containting the `MODULE_HANDLE` and `MODULE_LIBRARY_HANDLE` if the module was successfully linked to the message bus. **]**

**SRS_GATEWAY_LL_14_032: [** The function shall add the new `MODULE_DATA` to `GATEWAY_HANDLE_DATA`'s `modules` if the module was successfully linked to the message bus. **]**

**SRS_GATEWAY_LL_14_030: [** If any internal API call is unsuccessful after a module is created, the library will be unloaded and the module destroyed. **]**

**SRS_GATEWAY_LL_14_019: [** The function shall return the newly created `MODULE_HANDLE` only if each API call returns successfully. **]**

**SRS_GATEWAY_LL_99_011: [** The function shall assign `module_apis` to `MODULE::module_apis`. **]**

##Gateway_RemoveModule
```
extern void Gateway_LL_RemoveModule(GATEWAY_HANDLE gw, MODULE_HANDLE module);
```
Gateway_RemoveModule will remove the specified `module` from the message bus.

**SRS_GATEWAY_LL_14_020: [** If `gw` or `module` is `NULL` the function shall return. **]**

**SRS_GATEWAY_LL_14_023: [** The function shall locate the `MODULE_DATA` object in `GATEWAY_HANDLE_DATA`'s `modules` containing `module` and return if it cannot be found.  **]**

**SRS_GATEWAY_LL_14_021: [** The function shall unlink `module` from the `GATEWAY_HANDLE_DATA`'s `bus` `MESSAGE_BUS_HANDLE`. **]**

**SRS_GATEWAY_LL_14_022: [** If `GATEWAY_HANDLE_DATA`'s `bus` cannot unlink `module`, the function shall log the error and continue unloading the module from the `GATEWAY_HANDLE`. **]**

**SRS_GATEWAY_LL_14_038: [** The function shall decrement the `MESSAGE_BUS_HANDLE` reference count. **]**

**SRS_GATEWAY_LL_14_024: [** The function shall use the `MODULE_DATA`'s `library_handle` to retrieve the `MODULE_APIS` and destroy `module`. **]**

**SRS_GATEWAY_LL_14_025: [** The function shall unload `MODULE_DATA`'s `library_handle`. **]**

**SRS_GATEWAY_LL_14_026: [** The function shall remove that `MODULE_DATA` from `GATEWAY_HANDLE_DATA`'s `modules`. **]**