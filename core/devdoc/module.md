# Module Requirements

##Overview
This is the documentation for a module that communicates with other modules through a message broker. 
Every module needs to implement the same interface. However, the implementation is module-specific.

##References

##Exposed API
```C
#ifndef MODULE_H
#define MODULE_H

#include "macro_utils.h"
#include "broker.h"
#include "message.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*This is the interface that every gateway module needs to implement.*/
/*A module can be associated with one message broker.*/
/*A message broker can have many modules associated with it.*/

typedef void* MODULE_HANDLE;

/*Every module library exports a function (Module_GetAPIs) that returns*/
/*a pointer to a well-known structure.*/

/*this API creates a new Module. Configuration is a pointer given by the instantiator*/
typedef MODULE_HANDLE (*pfModule_Create)(BROKER_HANDLE broker, const void* configuration);

/*this destroys (frees resources) of the module parameter*/
typedef void (*pfModule_Destroy)(MODULE_HANDLE moduleHandle);

/*this is the module's callback function - gets called when a message is to be received by the module*/
typedef void (*pfModule_Receive)(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle);

typedef struct MODULE_APIS_TAG
{
    pfModule_Create Module_Create;
    pfModule_Destroy Module_Destroy;
    pfModule_Receive Module_Receive;
}MODULE_APIS;

/*this is the only function exported by a module, under a "by convention" name*/
/*using the exported function, the called learns of the functions for that module*/
typedef const MODULE_APIS* (*pfModule_GetAPIS)(void);

/*return the module APIS*/
#define MODULE_GETAPIS_NAME ("Module_GetAPIS")

#ifdef _WIN32
    #define MODULE_EXPORT __declspec(dllexport)
#else
    #define MODULE_EXPORT
#endif // _WIN32

MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void);

#ifdef __cplusplus
}
#endif

#endif // MODULE_H

```

##Module_Create
```C
static MODULE_HANDLE Module_Create(BROKER_HANDLE broker, const void* configuration);
```
This function is to be implemented by the module creator. It receives the message broker 
to which the module will publish its messages, and a pointer provided by the user
containing configuration information (usually information needed by the module to start).

The function returns a non-`NULL` value when it succeeds, known as the module handle. 
If the function fails internally, it should return `NULL`.

##Module_Destroy
```C
static void Module_Destroy(MODULE_HANDLE moduleHandle);
```
This function is to be implemented by the module creator. The function receives a previously
created handle by `Module_Create`. If parameter `moduleHandle` is `NULL` the function should 
return without taking any action. Otherwise, the function should free all system resources
allocated by the module.

##Module_Receive
```C
static void Module_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle);
```
This function is to be implemented by the module creator. This function is called by the
framework. This function is not called re-entrant. This function shouldn't assume it is 
called from the same thread.