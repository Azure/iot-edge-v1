# Gateway Requirements

## Overview
The Gateway API is used to create and manage a gateway. Modules are added to the gateway, and communication "links" are established between modules via a message broker.

## References

## Gateway Handle Implementation

This section details the internal structure defined in the gateway implementation used to track a gateway and the information it contains.

```
typedef struct GATEWAY_HANDLE_DATA_TAG {
    /** @brief Vector of MODULE_DATA modules that the Gateway must track */
    VECTOR_HANDLE modules;

    /** @brief The message broker used by this gateway */
    BROKER_HANDLE broker;

    /** @brief Vector of LINK_DATA links that the Gateway must track */
    VECTOR_HANDLE links;
} GATEWAY_HANDLE_DATA;
```

## Exposed API
```
#define GATEWAY_ADD_LINK_RESULT_VALUES \
    GATEWAY_ADD_LINK_SUCCESS, \
    GATEWAY_ADD_LINK_ERROR, \
    GATEWAY_ADD_LINK_INVALID_ARG

DEFINE_ENUM(GATEWAY_ADD_LINK_RESULT, GATEWAY_ADD_LINK_RESULT_VALUES);

#define GATEWAY_START_RESULT_VALUES \
    GATEWAY_START_SUCCESS, \
    GATEWAY_START_MODULE_FAIL, \
    GATEWAY_START_INVALID_ARGS

DEFINE_ENUM(GATEWAY_START_RESULT, GATEWAY_START_RESULT_VALUES);

typedef struct GATEWAY_LINK_ENTRY_TAG
{
    const char* module_source;
    const char* module_sink;
} GATEWAY_LINK_ENTRY;

typedef struct GATEWAY_HANDLE_DATA_TAG* GATEWAY_HANDLE;

typedef struct GATEWAY_MODULE_LOADER_INFO_TAG
{
    const MODULE_LOADER* loader;
    void* entrypoint;
} GATEWAY_MODULE_LOADER_INFO;

typedef struct GATEWAY_MODULES_ENTRY_TAG
{
    const char* module_name;
    GATEWAY_MODULE_LOADER_INFO module_loader_info;
    const void* module_configuration;
} GATEWAY_MODULES_ENTRY;

typedef struct GATEWAY_PROPERTIES_DATA_TAG
{
    VECTOR_HANDLE gateway_modules;
    VECTOR_HANDLE gateway_links;
} GATEWAY_PROPERTIES;

typedef struct GATEWAY_MODULE_INFO_TAG
{
    const char* module_name;
    VECTOR_HANDLE module_sources;
} GATEWAY_MODULE_INFO;

typedef enum GATEWAY_EVENT_TAG
{
    GATEWAY_CREATED = 0,
    GATEWAY_STARTED,
    GATEWAY_MODULE_LIST_CHANGED,
    GATEWAY_DESTROYED,
    GATEWAY_EVENTS_COUNT
} GATEWAY_EVENT;

typedef void* GATEWAY_EVENT_CTX;

typedef void(*GATEWAY_CALLBACK)(GATEWAY_HANDLE gateway, GATEWAY_EVENT event_type, GATEWAY_EVENT_CTX context, void* user_param);

extern GATEWAY_HANDLE Gateway_Create(const GATEWAY_PROPERTIES* properties);
extern GATEWAY_START_RESULT Gateway_Start(GATEWAY_HANDLE gw);
extern void Gateway_Destroy(GATEWAY_HANDLE gw);

extern MODULE_HANDLE Gateway_AddModule(GATEWAY_HANDLE gw, const GATEWAY_MODULES_ENTRY* entry);
extern void Gateway_StartModule(GATEWAY_HANDLE gw, MODULE_HANDLE module);
extern void Gateway_RemoveModule(GATEWAY_HANDLE gw, MODULE_HANDLE module);
extern int Gateway_RemoveModuleByName(GATEWAY_HANDLE gw, const char *module_name);

extern void Gateway_AddEventCallback(GATEWAY_HANDLE gw, GATEWAY_EVENT event_type, GATEWAY_CALLBACK callback, void* user_param);
extern VECTOR_HANDLE Gateway_GetModuleList(GATEWAY_HANDLE gw);
extern void Gateway_DestroyModuleList(VECTOR_HANDLE module_list);

extern GATEWAY_ADD_LINK_RESULT Gateway_AddLink(GATEWAY_HANDLE gw, const GATEWAY_LINK_ENTRY* entryLink);
extern void Gateway_RemoveLink(GATEWAY_HANDLE gw, const GATEWAY_LINK_ENTRY* entryLink);
```

## Gateway_Create
```
extern GATEWAY_HANDLE Gateway_Create(const GATEWAY_PROPERTIES* properties);
```
Gateway_Create creates a new gateway using information from the `GATEWAY_PROPERTIES` struct to create modules and associate them with a message broker.

**SRS_GATEWAY_14_001: [** This function shall create a `GATEWAY_HANDLE` representing the newly created gateway. **]**

**SRS_GATEWAY_14_002: [** This function shall return `NULL` upon any failure. **]**

**SRS_GATEWAY_17_016: [** This function shall initialize the default module loaders. **]**

**SRS_GATEWAY_17_017: [** This function shall destroy the default module loaders upon any failure. **]**

**SRS_GATEWAY_14_003: [** This function shall create a new `BROKER_HANDLE` for the gateway representing this gateway's message broker. **]**

**SRS_GATEWAY_14_004: [** This function shall return `NULL` if a `BROKER_HANDLE` cannot be created. **]**

**SRS_GATEWAY_17_001: [** This function shall not accept "*" as a module name. **]**

**SRS_GATEWAY_14_033: [** The function shall create a vector to store each `MODULE_DATA`. **]**

**SRS_GATEWAY_04_001: [** The function shall create a vector to store each `LINK_DATA` **]**

**SRS_GATEWAY_14_034: [** This function shall return `NULL` if a `VECTOR_HANDLE` cannot be created. **]**

**SRS_GATEWAY_14_035: [** This function shall destroy the previously created `BROKER_HANDLE` and free the `GATEWAY_HANDLE` if the `VECTOR_HANDLE` cannot be created. **]**

**SRS_GATEWAY_17_015: [** The function shall use the module's specified loader and the module's entrypoint to get each module's `MODULE_LIBRARY_HANDLE`. **]**

**SRS_GATEWAY_17_018: [** The function shall construct module configuration from module's `entrypoint` and module's `module_configuration`. **]**

**SRS_GATEWAY_17_020: [** The function shall clean up any constructed resources. **]**

**SRS_GATEWAY_14_009: [** The function shall use each of `GATEWAY_PROPERTIES`'s `gateway_modules` to create and add a module to the gateway's message broker. **]**

**SRS_GATEWAY_14_036: [** If any `MODULE_HANDLE` is unable to be created from a `GATEWAY_MODULES_ENTRY` the `GATEWAY_HANDLE` will be destroyed. **]**

**SRS_GATEWAY_04_004: [** If a module with the same `module_name` already exists, this function shall fail and the `GATEWAY_HANDLE` will be destroyed. **]**

**SRS_GATEWAY_17_002: [** The gateway shall accept a link with a source of "*" and a sink of a valid module. **]**

**SRS_GATEWAY_17_003: [** The gateway shall treat a source of "*" as link to the sink module from every other module in gateway. **]**

**SRS_GATEWAY_04_002: [** The function shall use each `GATEWAY_LINK_ENTRY` of `GATEWAY_PROPERTIES`'s `gateway_links` to add a `LINK` to `GATEWAY_HANDLE`'s broker. **]**

**SRS_GATEWAY_26_001: [** This function shall initialize attached Event System and report `GATEWAY_CREATED` event. **]**

**SRS_GATEWAY_26_002: [** If Event System module fails to be initialized the gateway module shall be destroyed and NULL returned with no events reported. **]**

**SRS_GATEWAY_26_010: [** This function shall report `GATEWAY_MODULE_LIST_CHANGED` event. **]**

**SRS_GATEWAY_04_003: [** If any `GATEWAY_LINK_ENTRY` is unable to be added to the broker the `GATEWAY_HANDLE` will be destroyed. **]**

## Gateway_Start
```
extern GATEWAY_START_RESULT Gateway_Start(GATEWAY_HANDLE gw);
```
Gateway_Start informs all modules that the gateway is ready to operate. This is a best effort attempt, all modules which implement a Module_Start function will be called.  The result of the attempt to start all modules will be reported in the `GATEWAY_START_RESULT`.

**SRS_GATEWAY_17_009: [** This function shall return `GATEWAY_START_INVALID_ARGS` if a NULL gateway is received. **]**

**SRS_GATEWAY_17_010: [** This function shall call `Module_Start` for every module which defines the start function. **]**

**SRS_GATEWAY_17_012: [** This function shall report a `GATEWAY_STARTED` event. **]**

**SRS_GATEWAY_17_013: [** This function shall return `GATEWAY_START_SUCCESS` upon completion. **]**


## Gateway_Destroy
```
extern void Gateway_Destroy(GATEWAY_HANDLE gw);
```
Gateway_Destroy destroys a gateway represented by the `gw` parameter.

**SRS_GATEWAY_14_005: [** If `gw` is `NULL` the function shall do nothing. **]**

**SRS_GATEWAY_14_028: [** The function shall remove each module in `GATEWAY_HANDLE_DATA`'s `modules` vector and destroy `GATEWAY_HANDLE_DATA`'s `modules`. **]**

**SRS_GATEWAY_04_014: [** The function shall remove each link in `GATEWAY_HANDLE_DATA`'s `links` vector and destroy `GATEWAY_HANDLE_DATA`'s `link`. **]**

**SRS_GATEWAY_14_037: [** If `GATEWAY_HANDLE_DATA`'s message broker cannot remove a module, the function shall log the error and continue removing modules from the `GATEWAY_HANDLE`. **]**

**SRS_GATEWAY_14_006: [** The function shall destroy the `GATEWAY_HANDLE_DATA`'s `broker` `BROKER_HANDLE`. **]**

**SRS_GATEWAY_17_019: [** The function shall destroy the module loader list. **]**

**SRS_GATEWAY_26_003: [** If the Event System module is initialized, this function shall report `GATEWAY_DESTROYED` event. **]**

**SRS_GATEWAY_26_004: [** This function shall destroy the attached Event System.  **]**

## Gateway_AddModule
```
extern MODULE_HANDLE Gateway_AddModule(GATEWAY_HANDLE gw, const GATEWAY_PROPERTIES_ENTRY* entry);
```
Gateway_AddModule adds a module to the gateway's message broker using the provided `GATEWAY_PROPERTIES_ENTRY`'s `loader_configuration`, `loader_api` and `module_configuration`.

**SRS_GATEWAY_14_011: [** If `gw`, `entry`, or `GATEWAY_MODULES_ENTRY`'s specified loader or entrypoint is `NULL` the function shall return `NULL`. **]**

**SRS_GATEWAY_14_012: [** The function shall load the module via the module's specified loader and the module's entrypoint to get each module's `MODULE_LIBRARY_HANDLE`. **]**

**SRS_GATEWAY_17_021: [** The function shall construct module configuration from module's `entrypoint` and module's `module_configuration`. **]**

**SRS_GATEWAY_17_022: [** The function shall clean up any constructed resources. **]**

**SRS_GATEWAY_14_031: [** If unsuccessful, the function shall return `NULL`. **]**

**SRS_GATEWAY_14_013: [** The function shall get the `const MODULE_API*` from the `MODULE_LIBRARY_HANDLE`. **]**

**SRS_GATEWAY_14_015: [** The function shall use the `MODULE_API` to create a `MODULE_HANDLE` using the `GATEWAY_MODULES_ENTRY`'s `module_properties`. **]**

**SRS_GATEWAY_14_016: [** If the module creation is unsuccessful, the function shall return `NULL`. **]**

**SRS_GATEWAY_14_017: [** The function shall attach the module to the `GATEWAY_HANDLE_DATA`'s `broker` using a call to `Broker_AddModule`. **]**

**SRS_GATEWAY_14_039: [** The function shall increment the `BROKER_HANDLE` reference count if the `MODULE_HANDLE` was successfully linked to the `GATEWAY_HANDLE_DATA`'s `broker`. **]**

**SRS_GATEWAY_14_018: [** If the function cannot attach the module to the message broker, the function shall return `NULL`. **]**

**SRS_GATEWAY_14_029: [** The function shall create a new `MODULE_DATA` containing the `MODULE_HANDLE`, `MODULE_LOADER_API` and `MODULE_LIBRARY_HANDLE` if the module was successfully linked to the message broker. **]**

**SRS_GATEWAY_14_032: [** The function shall add the new `MODULE_DATA` to `GATEWAY_HANDLE_DATA`'s `modules` if the module was successfully linked to the message broker. **]**

**SRS_GATEWAY_14_030: [** If any internal API call is unsuccessful after a module is created, the library will be unloaded and the module destroyed. **]**

**SRS_GATEWAY_14_019: [** The function shall return the newly created `MODULE_HANDLE` only if each API call returns successfully. **]**

**SRS_GATEWAY_99_011: [** The function shall assign `module_api` to `MODULE::module_api`. **]**

**SRS_GATEWAY_26_011: [** The function shall report `GATEWAY_MODULE_LIST_CHANGED` event after successfully adding the module. **]**

**SRS_GATEWAY_26_020: [** The function shall make a copy of the name of the module for internal use. **]**

## Gateway_StartModule
```
extern void Gateway_StartModule(GATEWAY_HANDLE gw, MODULE_HANDLE module);
```
When a module is added to the Gateway after it has been started, `Gateway_StartModule` informs a module when the broker is ready for the module to send and receive messages (i.e. after `Gateway_AddModule` and all `Gateway_AddLink` have been called).  This informational to the module, and the module is not required to implement a Module_Start function.

**SRS_GATEWAY_17_006: [** If `gw` is `NULL`, this function shall do nothing. **]**

**SRS_GATEWAY_17_007: [** If `module` is not found in the gateway, this function shall do nothing. **]**

**SRS_GATEWAY_17_008: [** When `module` is found, if the `Module_Start` function is defined for this module, the `Module_Start` function shall be called. **]**


## Gateway_RemoveModule
```
extern void Gateway_RemoveModule(GATEWAY_HANDLE gw, MODULE_HANDLE module);
```
Gateway_RemoveModule will remove the specified `module` from the message broker.

**SRS_GATEWAY_14_020: [** If `gw` or `module` is `NULL` the function shall return. **]**

**SRS_GATEWAY_14_023: [** The function shall locate the `MODULE_DATA` object in `GATEWAY_HANDLE_DATA`'s `modules` containing `module` and return if it cannot be found.  **]**

**SRS_GATEWAY_14_021: [** The function shall detach `module` from the `GATEWAY_HANDLE_DATA`'s `broker` `BROKER_HANDLE`. **]**

**SRS_GATEWAY_14_022: [** If `GATEWAY_HANDLE_DATA`'s `broker` cannot detach `module`, the function shall log the error and continue unloading the module from the `GATEWAY_HANDLE`. **]**

**SRS_GATEWAY_14_038: [** The function shall decrement the `BROKER_HANDLE` reference count. **]**

**SRS_GATEWAY_14_024: [** The function shall use the `MODULE_DATA`'s `library_handle` to retrieve the `MODULE_API` and destroy `module`. **]**

**SRS_GATEWAY_14_025: [** The function shall unload `MODULE_DATA`'s `library_handle`. **]**

**SRS_GATEWAY_14_026: [** The function shall remove that `MODULE_DATA` from `GATEWAY_HANDLE_DATA`'s `modules`. **]**

**SRS_GATEWAY_26_012: [** The function shall report `GATEWAY_MODULE_LIST_CHANGED` event after successfully removing the module. **]**

**SRS_GATEWAY_26_018: [** This function shall remove any links that contain the removed module either as a source or sink. **]**

## Gateway_RemoveModuleByName
```
int Gateway_RemoveModuleByName(GATEWAY_HANDLE gw, const char *module_name);
```

**SRS_GATEWAY_26_015: [** If `gw` or `module_name` is `NULL` the function shall do nothing and return with non-zero result. **]**

**SRS_GATEWAY_26_016: [** The function shall return 0 if the module was found. **]**

**SRS_GATEWAY_26_017: [** If module with `module_name` name is not found this function shall return non-zero and do nothing. **]**

To be consistent with the requirements of Gateway_RemoveModule, any other remove failures other than name not found or `NULL` parameters, shall follow Gateway_RemoveModule requirements, and thus be silent.
Furthermore, this function follows the removal procedure requirements of Gateway_RemoveModule.

## Gateway_AddEventCallback
```
extern void Gateway_AddEventCallback(GATEWAY_HANDLE gw, GATEWAY_EVENT event_type, GATEWAY_CALLBACK callback);
```
Gateway_AddEventCallback registers callback function to listen to some kind of `GATEWAY_EVENT`.
When the event happens the callback will be put in a queue and executed in a seperate callback thread in First-In-First-Out order of registration for that event
Also see `event_system_requirements.md` file for further requirements.

**SRS_GATEWAY_26_006: [** This function shall log a failure and do nothing else when `gw` parameter is NULL. **]**

## Gateway_GetModuleList
```
extern VECTOR_HANDLE Gateway_GetModuleList(GATEWAY_HANDLE gw);
```

**SRS_GATEWAY_26_007: [** This function shall return a snapshot copy of information about current gateway modules. **]**

**SRS_GATEWAY_26_013: [** For each module returned this function shall provide a snapshot copy vector of link sources for that module. **]**

**SRS_GATEWAY_26_014: [** For each module returned that has '*' as a link source this function shall provide NULL vector pointer as it's sources vector. **]**

**SRS_GATEWAY_26_008: [** If the `gw` parameter is NULL, the function shall return NULL handle and not allocate any data. **]**

**SRS_GATEWAY_26_009: [** This function shall return a NULL handle should any internal callbacks fail. **]**

## Gateway_DestroyModuleList
```
extern void Gateway_DestroyModuleList(VECTOR_HANDLE module_list);
```

**SRS_GATEWAY_26_011: [** This function shall destroy the `module_sources` list of each `GATEWAY_MODULE_INFO` **]**

**SRS_GATEWAY_26_012: [** This function shall destroy the list of `GATEWAY_MODULE_INFO` **]**

## Gateway_AddLink
```
extern GATEWAY_ADD_LINK_RESULT Gateway_AddLink(GATEWAY_HANDLE gw, const GATEWAY_LINK_ENTRY* entryLink);
```
Gateway_AddLink adds a link to the gateway's message broker using the provided `GATEWAY_LINK_ENTRY`'s `loader_configuration` and `GATEWAY_PROPERTIES_ENTRY`'s `module_configuration`.

**SRS_GATEWAY_04_008: [** If `gw` , `entryLink`, `entryLink->module_source` or `entryLink->module_source` is NULL the function shall return `GATEWAY_ADD_LINK_INVALID_ARG`. **]**

**SRS_GATEWAY_04_009: [** This function shall check if a given link already exists.  **]**

**SRS_GATEWAY_04_010: [** If the entryLink already exists it the function shall return `GATEWAY_ADD_LINK_ERROR` **]**

**SRS_GATEWAY_17_004: [** The gateway shall accept a link containing "*" as `entryLink->module_source`, and a valid module name as a `entryLink->module_sink`. **]**

**SRS_GATEWAY_17_005: [** For this link, the sink shall receive all messages publish by other modules. **]**

**SRS_GATEWAY_04_011: [** If the module referenced by the `entryLink->module_source` or `entryLink->module_sink` doesn't exists this function shall return `GATEWAY_ADD_LINK_ERROR` **]**

**SRS_GATEWAY_04_012: [** This function shall add the entryLink to the `gw->links` **]**

**SRS_GATEWAY_04_013: [** If adding the link succeed this function shall return `GATEWAY_ADD_LINK_SUCCESS` **]**

**SRS_GATEWAY_26_019: [** The function shall report `GATEWAY_MODULE_LIST_CHANGED` event after successfully adding the link. **]**

## Gateway_RemoveLink
```
extern void Gateway_RemoveLink(GATEWAY_HANDLE gw, const GATEWAY_LINK_ENTRY* entryLink);
```
Gateway_RemoveLink will remove the specified `link` from the message broker.

**SRS_GATEWAY_04_005: [** If `gw` or `entryLink` is `NULL` the function shall return. **]**

**SRS_GATEWAY_04_006: [** The function shall locate the `LINK_DATA` object in `GATEWAY_HANDLE_DATA`'s `links` containing `link` and return if it cannot be found. **]**

**SRS_GATEWAY_04_007: [** The functional shall remove that `LINK_DATA` from `GATEWAY_HANDLE_DATA`'s `links`. **]**

**SRS_GATEWAY_26_018: [** The function shall report `GATEWAY_MODULE_LIST_CHANGED` event. **]**
