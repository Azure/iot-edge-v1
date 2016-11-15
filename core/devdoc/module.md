# Module Requirements

##Overview
This is the documentation for a module that communicates with other modules through a message broker. 
Every module needs to implement the same interface. However, the implementation is module-specific.

##References

##Exposed API
```C
typedef struct MODULE_TAG MODULE;
typedef void* MODULE_HANDLE;
typedef struct MODULE_API_TAG MODULE_API;

struct MODULE_TAG
{
    const MODULE_API* module_apis;
    MODULE_HANDLE module_handle;
};

typedef MODULE_HANDLE(*pfModule_CreateFromJson)(BROKER_HANDLE broker, const char* configuration);
typedef MODULE_HANDLE(*pfModule_Create)(BROKER_HANDLE broker, const void* configuration);
typedef void(*pfModule_Destroy)(MODULE_HANDLE moduleHandle);
typedef void(*pfModule_Receive)(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle);
typedef void(*pfModule_Start)(MODULE_HANDLE moduleHandle);

typedef enum MODULE_API_VERSION_TAG
{
    MODULE_API_VERSION_1
} MODULE_API_VERSION;

static const MODULE_API_VERSION Module_ApiGatewayVersion = MODULE_API_VERSION_1;

struct MODULE_API_TAG
{
    MODULE_API_VERSION version;
};

typedef struct MODULE_API_1_TAG
{
    MODULE_API base;
    pfModule_CreateFromJson Module_CreateFromJson;
    pfModule_Create Module_Create;
    pfModule_Destroy Module_Destroy;
    pfModule_Receive Module_Receive;
    pfModule_Start Module_Start;
} MODULE_API_1;

typedef const MODULE_API* (*pfModule_GetApi)(MODULE_API_VERSION gateway_api_version);

MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version);
```

## Module_GetApi
```c
typedef MODULE_API* (*pfModule_GetApi)(MODULE_API_VERSION gateway_api_version);
```

This function is to be implemented by the module creator. It will return a 
pointer to a MODULES_API, or NULL if not successful.

The `MODULE_API` structure shall contain the `api_version` and shall fill in the corresponding api structure in the `api` union. The `gateway_api_version` is passed to the module so a module may decide how to fill in the `MODULE_API` structure.

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

## Module_Start
```c
static void Module_Start(MODULE_HANDLE moduleHandle);
```

This function may be implemented by the module creator.  It is allowed to be `NULL` in the `MODULE_API` structure. If defined, this function is called by the framework when the message broker is guaranteed to be ready to accept messages from the module.
