Binding for Out of Process Azure IoT Gateway Modules
====================================================

Overview
--------

This document specifies the requirements for the gateway module that manages a module running out of process.  The module communicates to the remote module via the inter-process transport mechanism.  The [high level design](../../core/devdoc/on-out-process-gateway-modules.md) will provide an overview on how this module communicates to the module host process.

Types
-----
```c
typedef struct OUTPROCESS_MODULE_CONFIG_DATA
{
    STRING_HANDLE control_url;
    STRING_HANDLE message_url;
    STRING_HANDLE outprocess_loader_args;
    STRING_HANDLE outprocess_module_args;
} OUTPROCESS_MODULE_CONFIG;

extern const MODULE_API_1 Outprocess_Module_API_all =
{
    {gateway_api_version},
    Remote_ParseConfigurationFromJson,
    Remote_FreeConfiguration,
    Remote_Create,
    Remote_Destroy,
    Remote_Receive,
    Remote_Start
};
```

## References

[On out process gateway modules](on-out-process-gateway-modules.md)

[Control messages in out process modules](out-process-control-messages.md)


Outprocess_ParseConfigurationFromJson
-------------------------------------
```c
static void* Outprocess_ParseConfigurationFromJson(const char* configuration);
```

**SRS_OUTPROCESS_MODULE_17_001: [** This function shall return `NULL` if `configuration` is `NULL` **]**

**SRS_OUTPROCESS_MODULE_17_002: [** This function shall construct a `STRING_HANDLE` from the given `configuration` and return the result. **]**

Outprocess_Create
-----------------
```c
MODULE_HANDLE Outprocess_Create(BROKER_HANDLE broker, const void* configuration);
```

**SRS_OUTPROCESS_MODULE_17_005: [** If `broker` or `configuration` are `NULL`, this function shall return `NULL`. **]**

**SRS_OUTPROCESS_MODULE_17_006: [** This function shall allocate memory for the MODULE_HANDLE. **]**

**SRS_OUTPROCESS_MODULE_17_007: [** This function shall intialize a lock for thread management. **]**

**SRS_OUTPROCESS_MODULE_17_008: [** This function shall create a pair socket for sending gateway messages to the module host. **]** This shall be referred to as the message channel.

**SRS_OUTPROCESS_MODULE_17_009: [** This function shall bind and connect the pair socket to the `message_url`. **]**

**SRS_OUTPROCESS_MODULE_17_010: [** This function shall create a request/reply socket for sending control messages to the module host. **]** This shall be referred to as the control channel.

**SRS_OUTPROCESS_MODULE_17_011: [** This function shall connect the request/reply socket to the `control_url`. **]**

**SRS_OUTPROCESS_MODULE_17_012: [** This function shall construct a _Create Message_ from `configuration`. **]**

**SRS_OUTPROCESS_MODULE_17_013: [** This function shall send the _Create Message_ on the control channel. **]**

**SRS_OUTPROCESS_MODULE_17_014: [** This function shall wait for a _Create Response_ on the control channel. **]**

**SRS_OUTPROCESS_MODULE_17_015: [** This function shall expect a successful result from the _Create Response_ to consider the module creation a success. **]**

See [control messages in out process modules](out-process-control-messages.md) for content of a _Create Message_ and _Create Response_.

**SRS_OUTPROCESS_MODULE_17_016: [** If any step in the creation fails, this function shall deallocate all resources and return `NULL`. **]**

Outprocess_Start
----------------
```c
void Outprocess_Start(MODULE_HANDLE module);
```

**SRS_OUTPROCESS_MODULE_17_020: [** This function shall do nothing if `module` is `NULL`. **]**

**SRS_OUTPROCESS_MODULE_17_017: [** This function shall ensure thread safety on execution. **]**

**SRS_OUTPROCESS_MODULE_17_018: [** This function shall create a thread to handle receiving messages from module host. **]**

**SRS_OUTPROCESS_MODULE_17_019: [** This function shall send a _Start Message_ on the control channel. **]**

**SRS_OUTPROCESS_MODULE_17_021: [** This function shall free any resources created. **]**

Outprocess_Receive
-------------------
```c
void Outprocess_Receive(MODULE_HANDLE module, MESSAGE_HANDLE message);
```

**SRS_OUTPROCESS_MODULE_17_022: [** If `module` or `message_handle` is `NULL`, this function shall do nothing. **]**

**SRS_OUTPROCESS_MODULE_17_023: [** This function shall serialize the message for transmission on the message channel. **]**

**SRS_OUTPROCESS_MODULE_17_024: [** This function shall send the message on the message channel. **]**

**SRS_OUTPROCESS_MODULE_17_025: [** This function shall free any resources created. **]**

Outprocess_Destroy
------------------
```c
void Outprocess_Destroy(MODULE_HANDLE module);
```

**SRS_OUTPROCESS_MODULE_17_026: [** If `module` is `NULL`, this function shall do nothing. **]**

**SRS_OUTPROCESS_MODULE_17_027: [** This function shall ensure thread safety on execution. **]**

**SRS_OUTPROCESS_MODULE_17_028: [** This function shall construct a _Destroy Message_. **]**

**SRS_OUTPROCESS_MODULE_17_029: [** This function shall send the _Destroy Message_ on the control channel. **]**

**SRS_OUTPROCESS_MODULE_17_030: [** This function shall close the message channel socket. **]**

**SRS_OUTPROCESS_MODULE_17_031: [** This function shall close the control channel socket. **]**

**SRS_OUTPROCESS_MODULE_17_032: [** This function shall signal the messaging thread to close. **]**

**SRS_OUTPROCESS_MODULE_17_033: [** This function shall wait for the messaging thread to complete. **]**

**SRS_OUTPROCESS_MODULE_17_034: [** This function shall release all resources created by this module. **]**


Outprocess messaging thread
---------------------------
```c
int Outprocess_thread(void * param);
```

**SRS_OUTPROCESS_MODULE_17_036: [** This function shall ensure thread safety on execution. **]**

**SRS_OUTPROCESS_MODULE_17_037: [** This function shall receive the module handle data as the thread parameter. **]**

**SRS_OUTPROCESS_MODULE_17_038: [** This function shall read from the message channel for gateway messages from the module host. **]**

**SRS_OUTPROCESS_MODULE_17_039: [** Upon successful receiving a gateway message, this function shall deserialize the message. **]**

**SRS_OUTPROCESS_MODULE_17_040: [** This function shall publish any successfully created gateway message to the broker. **]**

Outprocess_FreeConfiguration
----------------------------
```c
void Outprocess_FreeConfiguration(void* configuration)
```

**SRS_OUTPROCESS_MODULE_17_003: [** If `configuration` is `NULL` this function shall do nothing. **]**

**SRS_OUTPROCESS_MODULE_17_004: [** This function shall delete the `STRING_HANDLE` represented by `configuration`. **]**
