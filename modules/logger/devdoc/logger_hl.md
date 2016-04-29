LOGGER HL MODULE
===============

High level design
-----------------

### References
[logger](./logger.md)

[json](http://www.json.org)

[gateway](../../../../devdoc/gateway_requirements.md)

### Overview

This module is just a passthrough to LOGGER MODULE in all aspects except for creation that is done by means of a json value.


###Logger_HL_Create
```c
MODULE_HANDLE Logger_HL_Create(MESSAGE_BUS_HANDLE busHandle, const void* configuration);
```
Creates a new LOGGER MODULE HL instance. `configuration` is a pointer to a const char* that contains a json object as supplied by `Gateway_Create_From_JSON`.
By convention in the json object should contain 
```json
{"filename":"nameOfTheFile"}
``` 
where "nameOfTheFile" is the output log file name.

Example:
The following Gateway config file will contain a module called "logger_hl" build from F:\logger_hl.dll and instructs the logger_hl to output to
the file deviceCloudUploadGatewaylog.txt
```json
{
    "modules" :
    [ 
        {
            "module name" : "logger_hl",
            "module path" : "logger_hl.dll",
            "args" : 
            {
                "filename":"deviceCloudUploadGatewaylog.txt"}"
            }
        }
   ]
}```



**SRS_LOGGER_HL_02_001: [** If `busHandle` is NULL then `Logger_HL_Create` shall fail and return NULL. **]**
**SRS_LOGGER_HL_02_003: [** If `configuration` is NULL then `Logger_HL_Create` shall fail and return NULL. **]**
**SRS_LOGGER_HL_02_011: [** If configuration is not a JSON object, then `Logger_HL_Create` shall fail and return NULL. **]**
**SRS_LOGGER_HL_02_012: [** If the JSON object does not contain a value named "filename" then `Logger_HL_Create` shall fail and return NULL. **]**
**SRS_LOGGER_HL_02_005: [** `Logger_HL_Create` shall pass `busHandle` and the filename to `Logger_Create`. **]**
**SRS_LOGGER_HL_02_006: [** If `Logger_Create` succeeds then `Logger_HL_Create` shall succeed and return a non-NULL value. **]**
**SRS_LOGGER_HL_02_007: [** If `Logger_Create` fails then `Logger_HL_Create` shall fail and return NULL. **]**
    
###Logger_HL_Receive
```c
void Logger_HL_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle);
```
**SRS_LOGGER_HL_02_008: [** `Logger_HL_Receive` shall pass the received parameters to the underlying Logger's `_Receive` function. **]** 


###Logger_HL_Destroy
```c
void Logger_HL_Destroy(MODULE_HANDLE moduleHandle);
```
**SRS_LOGGER_HL_02_009: [** `Logger_HL_Destroy` shall destroy all used resources. **]**

###Module_GetAPIs
```c
extern const MODULE_APIS* Module_GetAPIS(void);
```
**SRS_LOGGER_HL_02_010: [** `Module_GetAPIS` shall return a non-NULL pointer to a structure of type MODULE_APIS that has all fields non-NULL. **]**