Module Requirements
===================

Overview
--------

This is the documentation for a module that communicates with other modules
through a message broker. Every module needs to implement the same interface.
However, the implementation is module-specific.

Exposed API
-----------

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ c
typedef struct MODULE_TAG MODULE;
typedef void* MODULE_HANDLE;
typedef struct MODULE_API_TAG MODULE_API;

struct MODULE_TAG
{
    const MODULE_API* module_apis;
    MODULE_HANDLE module_handle;
};

typedef void*(*pfModule_ParseConfigurationFromJson)(const char* configuration);
typedef void(*pfModule_FreeConfiguration)(void* configuration);
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
    pfModule_ParseConfigurationFromJson Module_ParseConfigurationFromJson;
    pfModule_FreeConfiguration Module_FreeConfiguration;
    pfModule_Create Module_Create;
    pfModule_Destroy Module_Destroy;
    pfModule_Receive Module_Receive;
    pfModule_Start Module_Start;
} MODULE_API_1;

typedef const MODULE_API* (*pfModule_GetApi)(MODULE_API_VERSION gateway_api_version);

MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Module\_GetApi
--------------

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ c
typedef MODULE_API* (*pfModule_GetApi)(MODULE_API_VERSION gateway_api_version);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This function is to be implemented by the module creator. It will return a
pointer to a `MODULES_API`, or `NULL` if not successful.

The `MODULE_API` structure shall contain the `api_version` and shall fill in the
corresponding api structure in the `api` union. The `gateway_api_version` is
passed to the module so a module may decide how to fill in the `MODULE_API`
structure.

Module\_Create
--------------

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ c
static MODULE_HANDLE Module_Create(BROKER_HANDLE broker, const void* configuration);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This function is to be implemented by the module creator. It receives the
message broker to which the module will publish its messages, and a pointer
provided by the user containing configuration information (usually information
needed by the module to start).

The function returns a non-`NULL` value when it succeeds, known as the module
handle. If the function fails internally, it should return `NULL`.

Module\_Destroy
---------------

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ c
static void Module_Destroy(MODULE_HANDLE moduleHandle);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This function is to be implemented by the module creator. The function receives
a previously created handle by `Module_Create`. If parameter `moduleHandle` is
`NULL` the function should return without taking any action. Otherwise, the
function should free all system resources allocated by the module.

Module\_Receive
---------------

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ c
static void Module_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This function is to be implemented by the module creator. This function is
called by the framework. This function is not called re-entrant. This function
shouldn't assume it is called from the same thread.

Module\_Start
-------------

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ c
static void Module_Start(MODULE_HANDLE moduleHandle);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This function may be implemented by the module creator. It is allowed to be
`NULL` in the `MODULE_API` structure. If defined, this function is called by the
framework when the message broker is guaranteed to be ready to accept messages
from the module.

Module\_ParseConfigurationFromJson
----------------------------------

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ c
static void* Module_ParseConfigurationFromJson(const char* configuration);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This function may be implemented by the module creator. It is required if the
module is created from a JSON configuration. It receives a JSON string
corresponding to the module arguments in the JSON gateway description. It
returns a pointer the configuration expected by the `Module_Create` function,
which may be `NULL`.

Module\_FreeConfiguration
-------------------------

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ c
static void Module_FreeConfiguration(void* configuration);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This function may be implemented by the module creator. It is required if the
module is created from a JSON configuration.  The input is the `configuration`
as created by `Module_ParseConfigurationFromJson`, and should free any
resources allocated by that function.
