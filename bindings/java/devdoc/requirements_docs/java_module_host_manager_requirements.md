# Java Module Host Manager Requirements

## Overview
This is the API to manage the number of currently connected Java Module Hosts connected to the gateway. The Java Module Host Manager is created as a singleton instance of the `JAVA_MODULE_HOST_MANAGER_HANDLE` structure.

## References

## Exposed API
```c
#ifndef JAVA_MODULE_HOST_MANAGER_H
#define JAVA_MODULE_HOST_MANAGER_H

#include "azure_c_shared_utility/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define JAVA_MODULE_HOST_MANAGER_RESULT_VALUES \
	MANAGER_OK, \
	MANAGER_ERROR \


DEFINE_ENUM(JAVA_MODULE_HOST_MANAGER_RESULT, JAVA_MODULE_HOST_MANAGER_RESULT_VALUES);

typedef struct JAVA_MODULE_HOST_MANAGER_DATA_TAG* JAVA_MODULE_HOST_MANAGER_HANDLE;

extern JAVA_MODULE_HOST_MANAGER_HANDLE JavaModuleHostManager_Create(JAVA_MODULE_HOST_CONFIG* config);

extern void JavaModuleHostManager_Destroy(JAVA_MODULE_HOST_MANAGER_HANDLE handle);

extern JAVA_MODULE_HOST_MANAGER_RESULT JavaModuleHostManager_Add(JAVA_MODULE_HOST_MANAGER_HANDLE handle);

extern JAVA_MODULE_HOST_MANAGER_RESULT JavaModuleHostManager_Remove(JAVA_MODULE_HOST_MANAGER_HANDLE handle);

extern size_t JavaModuleHostManager_Size(JAVA_MODULE_HOST_MANAGER_HANDLE handle);

#ifdef __cplusplus
}
#endif

#endif /*JAVA_MODULE_HOST_MANAGER_H*/
```

##JavaModuleHostManager_Create(JAVA_MODULE_HOST_CONFIG* config)
```c
extern JAVA_MODULE_HOST_MANAGER_HANDLE JavaModuleHostManager_Create(JAVA_MODULE_HOST_CONFIG* config);
```
JavaModuleHostManager_Create creates a new singleton instance of a `JAVA_MODULE_HOST_MANAGER_HANDLE` structure.

**SRS_JAVA_MODULE_HOST_MANAGER_14_030: [** The function shall check against the singleton `JAVA_MODULE_HOST_MANAGER_HANDLE` instance's `JAVA_MODULE_HOST_CONFIG` structure for equality in each field. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_031: [** The function shall return `NULL` if the `JAVA_MODULE_HOST_CONFIG` structures do not match. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_001: [** The function shall return the global `JAVA_MODULE_HOST_MANAGER_HANDLE` instance if it is not `NULL`. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_002: [** The function shall allocate memory for the `JAVA_MODULE_HOST_MANAGER_HANDLE` if the global instance is `NULL`. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_003: [** The function shall return `NULL` if memory allocation fails. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_004: [** The function shall initialize a new `LOCK_HANDLE` and set the `JAVA_MODULE_HOST_MANAGER_HANDLE` structures  `LOCK_HANDLE` member. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_005: [** The function shall return `NULL` if lock initialization fails. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_006: [** The function shall set the `module_count` member variable to 0. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_028: [** The function shall do a deep copy of the `config` parameter and keep a pointer to the copied `JAVA_MODULE_HOST_CONFIG`. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_029: [** If the deep copy fails, the function shall return `NULL`. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_007: [** The function shall shall set the global `JAVA_MODULE_HOST_MANAGER_HANDLE` instance. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_008: [** The function shall return the global `JAVA_MODULE_HOST_MANAGER_HANDLE` instance. **]**

##JavaModuleHostManager_Destroy(JAVA_MODULE_HOST_MANAGER_HANDLE handle)
```c
extern void JavaModuleHostManager_Destroy(JAVA_MODULE_HOST_MANAGER_HANDLE handle);
```
JavaModuleHostManager_Destroy will destroy the `JAVA_MODULE_HOST_MANAGER_HANDLE` if it is no longer tracking any more `JAVA_MODULE_HANDLE_DATA`s.

**SRS_JAVA_MODULE_HOST_MANAGER_14_009: [** The function shall do nothing if `handle` is `NULL`. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_010: [** The function shall do nothing if `handle->module_count` is not 0. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_011: [** The function shall deinitialize `handle->lock`. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_012: [** The function shall do nothing if lock deinitialization fails. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_032: [** The function shall destroy the `JAVA_MODULE_HOST_CONFIG` structure. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_013: [** The function shall free the `handle` parameter. **]**

##JavaModuleHostManager_Add(JAVA_MODULE_HOST_MANAGER_HANDLE handle)
##JavaModuleHostManager_Remove(JAVA_MODULE_HOST_MANAGER_HANDLE handle)
```c
extern JAVA_MODULE_HOST_MANAGER_RESULT JavaModuleHostManager_Add(JAVA_MODULE_HOST_MANAGER_HANDLE handle);
extern JAVA_MODULE_HOST_MANAGER_RESULT JavaModuleHostManager_Remove(JAVA_MODULE_HOST_MANAGER_HANDLE handle);
```
JavaModuleHostManager_Remove will safely increment/decrement the number of tracked Java Module Host modules.

**SRS_JAVA_MODULE_HOST_MANAGER_14_014: [** The function shall return `MANAGER_ERROR` if `handle` is `NULL`. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_015: [** The function shall acquire the `handle` parameters lock. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_016: [** If lock acquisition fails, this function shall return `MANAGER_ERROR`. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_017: [** The function shall increment or decrement `handle->module_count`. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_018: [** The function shall release the `handle` parameters lock. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_020: [** The function shall return `MANAGER_OK` if the all succeeds. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_027: [** The function shall return `MANAGER_ERROR` if an attempt is made to decrement past 0. **]**

##JavaModuleHostManager_Size(JAVA_MODULE_HOST_MANAGER_HANDLE handle)
```c
extern size_t JavaModuleHostManager_Size(JAVA_MODULE_HOST_MANAGER_HANDLE handle);
```
JavaModuleHostManager_Size will return the number of tracked Java Module Host modules.

**SRS_JAVA_MODULE_HOST_MANAGER_14_021: [** If `handle` is `NULL` the function shall return -1. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_023: [** The function shall acquire the `handle` parameters lock. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_024: [** If lock acquisition fails, this function shall return -1. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_022: [** The function shall return `handle->module_count`. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_025: [** The function shall release the `handle` parameters lock. **]**

**SRS_JAVA_MODULE_HOST_MANAGER_14_026: [** If the function fails to release the lock, this function shall report the error. **]**