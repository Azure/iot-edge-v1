Out of Process Module Loader Requirements
=========================================

Overview
--------

The out of process (outprocess) module loader implements loading of gateway modules that are proxies for modules hosted out of process.

References
----------

[Module loader design](./module_loaders.md)
[Out of Process Module HLD](./outprocess_hld.md)
[Out of Process Module requirements](./outprocess_module_requirements.md)

Exposed API
-----------

```C
#define OUTPROCESS_LOADER_ACTIVATION_TYPE_VALUES \
    OUTPROCESS_LOADER_ACTIVATION_NONE

/**
 * @brief Enumeration listing all supported module loaders
 */
DEFINE_ENUM(OUTPROCESS_LOADER_ACTIVATION_TYPE, OUTPROCESS_LOADER_ACTIVATION_TYPE_VALUES);

/** @brief Structure to load an out of process proxy module */
typedef struct OUTPROCESS_LOADER_ENTRYPOINT_TAG
{
    /**
     * @brief Tells the module and loader how much control the gateway 
     * has over the module host process.
     */
    OUTPROCESS_LOADER_ACTIVATION_TYPE activation_type;
    /** @brief The URI for the module host control channel.*/
    STRING_HANDLE control_id;
    /** @brief The URI for the gateway message channel.*/
    STRING_HANDLE message_id;
    /** @brief controls timeout for ipc retries. */
    unsigned int default_wait;
} OUTPROCESS_LOADER_ENTRYPOINT;

/** @brief      The API for the out of process proxy module loader. */
MOCKABLE_FUNCTION(, GATEWAY_EXPORT const MODULE_LOADER*, OutprocessLoader_Get);
```


OutprocessModuleLoader_Load
---------------------------

```C
MODULE_LIBRARY_HANDLE OutprocessModuleLoader_Load(const MODULE_LOADER* loader, const void* entrypoint)
```

Loads the outprocess module according to the `entrypoint` given.  The outprocess module is statically linked in the gateway library, so there is no shared library to load.

**SRS_OUTPROCESS_LOADER_17_001: [** If `loader` or `entrypoint` are `NULL`, then this function shall return `NULL`.  **]**

**SRS_OUTPROCESS_LOADER_17_042: [** If the loader type is not `OUTPROCESS`, then this function shall return `NULL`. **]**

**SRS_OUTPROCESS_LOADER_17_002: [** If the entrypoint's `control_id` is `NULL`, then this function shall return `NULL`.  **]**

**SRS_OUTPROCESS_LOADER_27_003: [** If the entrypoint's `activation_type` is invalid, then `OutprocessModuleLoader_Load` shall return `NULL`. **]**

**SRS_OUTPROCESS_LOADER_17_004: [** The loader shall allocate memory for the loader handle. **]**

**SRS_OUTPROCESS_LOADER_27_005: [** *Launch* - `OutprocessModuleLoader_Load` shall launch the child process identified by the entrypoint. **]**

**SRS_OUTPROCESS_LOADER_17_006: [** The loader shall store a pointer to the `MODULE_API` in the loader handle. **]**

**SRS_OUTPROCESS_LOADER_17_007: [** Upon success, this function shall return a valid pointer to the loader handle. **]**

**SRS_OUTPROCESS_LOADER_17_008: [** If any call in this function fails, this function shall return `NULL`. **]**


OutprocessModuleLoader_GetModuleApi
-----------------------------------

```C
extern const MODULE_API* `OutprocessModuleLoader_GetModuleApi`(const MODULE_LOADER* loader, MODULE_LIBRARY_HANDLE moduleLibraryHandle);
```

The out of process module is linked with the outprocess loader.

**SRS_OUTPROCESS_LOADER_17_009: [** This function shall return a valid pointer to the `MODULE_API` MODULE_API. **]**


OutprocessModuleLoader_Unload
-----------------------------

```C
void OutprocessModuleLoader_Unload(const MODULE_LOADER* loader, MODULE_LIBRARY_HANDLE moduleLibraryHandle);
```

**SRS_OUTPROCESS_LOADER_17_010: [** This function shall do nothing if `moduleLibraryHandle` is `NULL`. **]**

**SRS_OUTPROCESS_LOADER_17_011: [** This function shall release all resources created by this loader. **]**


OutprocessModuleLoader_ParseEntrypointFromJson
----------------------------------------------

```C
void* OutprocessModuleLoader_ParseEntrypointFromJson(const struct MODULE_LOADER_TAG* loader, const JSON_Value* json);
```

Parses entrypoint JSON as it applies to out of process modules and returns a pointer to the parsed data.

**SRS_OUTPROCESS_LOADER_17_012: [** This function shall return `NULL` if `json` is `NULL`. **]**

**SRS_OUTPROCESS_LOADER_17_013: [** This function shall return `NULL` if `activation.type` is not present in `json`. **]**

**SRS_OUTPROCESS_LOADER_27_014: [** This function shall return `NULL` if `activation.type` is `OUTPROCESS_LOADER_ACTIVATION_INVALID`. **]**

**SRS_OUTPROCESS_LOADER_27_015: [** *Launch* - `OutprocessModuleLoader_ParseEntrypointFromJson` shall validate the launch parameters. **]**

**SRS_OUTPROCESS_LOADER_17_041: [** This function shall return `NULL` if `control.id` is not present in `json`. **]**

**SRS_OUTPROCESS_LOADER_17_016: [** This function shall allocate a `OUTPROCESS_LOADER_ENTRYPOINT` structure. **]**

**SRS_OUTPROCESS_LOADER_17_043: [** This function shall read the `timeout` value. **]**

**SRS_OUTPROCESS_LOADER_17_044: [** If `timeout` is set, the `default_wait` shall be set to this value, else it will be set to a default of 1000 ms. **]**

This timeout controls how long a module will wait before retrying to connect to remote module on startup. If remote module is expected to take a long time to start, setting this will reduce the number of retires before success.

**SRS_OUTPROCESS_LOADER_17_017: [** This function shall assign the entrypoint `activation_type` to `NONE`. **]**

**SRS_OUTPROCESS_LOADER_17_018: [** This function shall assign the entrypoint `control_id` to the string value of "ipc://" + "control.id" in `json`. **]**

**SRS_OUTPROCESS_LOADER_27_020: [** *Launch* - `OutprocessModuleLoader_ParseEntrypointFromJson` shall update the entry point with the parsed launch parameters. **]**

**SRS_OUTPROCESS_LOADER_17_019: [** This function shall assign the entrypoint `message_id` to the string value of "ipc://" + "message.id" in `json`, `NULL` if not present. **]**

**SRS_OUTPROCESS_LOADER_17_021: [** This function shall return `NULL` if any calls fails. **]**

**SRS_OUTPROCESS_LOADER_17_022: [** This function shall return a valid pointer to an `OUTPROCESS_LOADER_ENTRYPOINT` on success. **]**


OutprocessModuleLoader_FreeEntrypoint
-------------------------------------

```C
void OutprocessModuleLoader_FreeEntrypoint(const struct MODULE_LOADER_TAG* loader, void* entrypoint)
```

**SRS_OUTPROCESS_LOADER_17_023: [** This function shall release all resources allocated by `OutprocessModuleLoader_ParseEntrypointFromJson`. **]**


OutprocessModuleLoader_ParseConfigurationFromJson
-------------------------------------------------

```C
MODULE_LOADER_BASE_CONFIGURATION* OutprocessModuleLoader_ParseConfigurationFromJson(const struct MODULE_LOADER_TAG* loader, const JSON_Value* json);
```

**SRS_OUTPROCESS_LOADER_17_024: [** The out of process loader does not have any configuration. So this method shall return `NULL`. **]**


OutprocessModuleLoader_FreeConfiguration
----------------------------------------

```C
void OutprocessModuleLoader_FreeConfiguration(const struct MODULE_LOADER_TAG* loader, MODULE_LOADER_BASE_CONFIGURATION* configuration);
```

The out of process loader does not have any configuration. So there is nothing to free here.

**SRS_OUTPROCESS_LOADER_17_025: [** This function shall move along, nothing to free here. **]**


OutprocessModuleLoader_BuildModuleConfiguration
-----------------------------------------------

```C
void* OutprocessModuleLoader_BuildModuleConfiguration(
    const MODULE_LOADER* loader,
    const void* entrypoint,
    const void* module_configuration
);
```

**SRS_OUTPROCESS_LOADER_17_026: [** This function shall return `NULL` if `entrypoint`, `control_id`, or `module_configuration` is `NULL`. **]**  Note: If there is no module configuration, this function expects the configuration will be the string "null".

**SRS_OUTPROCESS_LOADER_17_027: [** This function shall allocate a `OUTPROCESS_MODULE_CONFIG` structure. **]**

**SRS_OUTPROCESS_LOADER_17_029: [** If the entrypoint's `message_id` is `NULL`, then the loader shall construct an IPC uri. **]** 

**SRS_OUTPROCESS_LOADER_17_030: [** The loader shall create a unique id, if needed for URI constrution. **]**

**SRS_OUTPROCESS_LOADER_17_032: [** The message uri shall be composed of "ipc://" + unique id. **]**

**SRS_OUTPROCESS_LOADER_17_033: [** This function shall allocate and copy each string in `OUTPROCESS_LOADER_ENTRYPOINT` and assign them to the corresponding fields in `OUTPROCESS_MODULE_CONFIG`. **]**

**SRS_OUTPROCESS_LOADER_17_034: [** This function shall allocate and copy the `module_configuration` string and assign it the `OUTPROCESS_MODULE_CONFIG::outprocess_module_args` field. **]**

**SRS_OUTPROCESS_LOADER_17_035: [** Upon success, this function shall return a valid pointer to an `OUTPROCESS_MODULE_CONFIG` structure. **]**

**SRS_OUTPROCESS_LOADER_17_036: [** If any call fails, this function shall return `NULL`. **]**


OutprocessModuleLoader_FreeModuleConfiguration
----------------------------------------------

```C
void OutprocessModuleLoader_FreeModuleConfiguration(const struct MODULE_LOADER_TAG* loader, const void* module_configuration);
```

**SRS_OUTPROCESS_LOADER_17_037: [** This function shall release all memory allocated by `OutprocessModuleLoader_BuildModuleConfiguration`. **]**


OutprocessModuleLoader_Get
--------------------------

```C
const MODULE_LOADER* OutprocessModuleLoader_Get(void);
```

**SRS_OUTPROCESS_LOADER_17_038: [** `OutprocessModuleLoader_Get` shall return a non-`NULL` pointer to a `MODULE_LOADER` struct. **]**

**SRS_OUTPROCESS_LOADER_17_039: [** `MODULE_LOADER::type` shall be `OUTPROCESS`. **]**

**SRS_OUTPROCESS_LOADER_17_040: [** `MODULE_LOADER::name` shall be the string 'outprocess'. **]**


OutprocessLoader_SpawnChildProcesses
------------------------------------

```C
int OutprocessLoader_SpawnChildProcesses(void);
```

**SRS_OUTPROCESS_LOADER_27_045: [** *Prerequisite Check* - If no processes have been enqueued, then `OutprocessLoader_SpawnChildProcesses` shall take no action and return zero. **]**

**SRS_OUTPROCESS_LOADER_27_046: [** *Prerequisite Check* - If child processes are already running, then `OutprocessLoader_SpawnChildProcesses` shall take no action and return zero. **]**

**SRS_OUTPROCESS_LOADER_27_047: [** `OutprocessLoader_SpawnChildProcesses` shall launch the enqueued child processes by calling `THREADAPI_RESULT ThreadAPI_Create(THREAD_HANDLE * threadHandle, THREAD_START_FUNC func, void * arg)`. **]**

**SRS_OUTPROCESS_LOADER_27_048: [** If launching the enqueued child processes fails, then `OutprocessLoader_SpawnChildProcesses` shall return a non-zero value. **]**

**SRS_OUTPROCESS_LOADER_27_049: [** If no errors are encountered, then `OutprocessLoader_SpawnChildProcesses` shall return zero. **]**


OutprocessLoader_JoinChildProcesses
-----------------------------------

```C
void OutprocessLoader_JoinChildProcesses(void);
```

**SRS_OUTPROCESS_LOADER_27_050: [** *Prerequisite Check* - If no threads are running, then `OutprocessLoader_JoinChildProcesses` shall abandon the effort to join the child processes immediately. **]**

**SRS_OUTPROCESS_LOADER_27_064: [** `OutprocessLoader_JoinChildProcesses` shall get the count of child processes, by calling `size_t VECTOR_size(VECTOR_HANDLE handle)`. **]**

**SRS_OUTPROCESS_LOADER_27_063: [** If no processes are running, then `OutprocessLoader_JoinChildProcesses` shall immediately join the child process management thread. **]**

**SRS_OUTPROCESS_LOADER_27_051: [** `OutprocessLoader_JoinChildProcesses` shall create a timer to test for timeout, by calling `TICK_COUNTER_HANDLE tickcounter_create(void)`. **]**

**SRS_OUTPROCESS_LOADER_27_052: [** If unable to create a timer, `OutprocessLoader_JoinChildProcesses` shall abandon awaiting the grace period. **]**

**SRS_OUTPROCESS_LOADER_27_053: [** `OutprocessLoader_JoinChildProcesses` shall mark the begin time, by calling `int tickcounter_get_current_ms(TICK_COUNTER_HANDLE tick_counter, tickcounter_ms_t * current_ms)` using the handle called by the previous call to `tickcounter_create`. **]**

**SRS_OUTPROCESS_LOADER_27_054: [** If unable to mark the begin time, `OutprocessLoader_JoinChildProcesses` shall abandon awaiting the grace period. **]**

**SRS_OUTPROCESS_LOADER_27_055: [** While awaiting the grace period, `OutprocessLoader_JoinChildProcesses` shall mark the current time, by calling `int tickcounter_get_current_ms(TICK_COUNTER_HANDLE tick_counter, tickcounter_ms_t * current_ms)` using the handle called by the previous call to `tickcounter_create`. **]**

**SRS_OUTPROCESS_LOADER_27_056: [** If unable to mark the current time, `OutprocessLoader_JoinChildProcesses` shall abandon awaiting the grace period. **]**

**SRS_OUTPROCESS_LOADER_27_057: [** While awaiting the grace period, `OutprocessLoader_JoinChildProcesses` shall check if the processes are running, by calling `int uv_loop_alive(const uv_loop_t * loop)` using the result of `uv_default_loop()` for `loop`. **]**

**SRS_OUTPROCESS_LOADER_27_059: [** If the child processes are not running, `OutprocessLoader_JoinChildProcesses` shall shall immediately join the child process management thread. **]**

**SRS_OUTPROCESS_LOADER_27_058: [** `OutprocessLoader_JoinChildProcesses` shall await the grace period in 100ms increments, by calling `void ThreadAPI_Sleep(unsigned int milliseconds)` passing 100 for `milliseconds`. **]**

**SRS_OUTPROCESS_LOADER_27_060: [** If the grace period expired, `OutprocessLoader_JoinChildProcesses` shall get the handle of each child processes, by calling `void * VECTOR_element(VECTOR_HANDLE handle, size_t index)`. **]**

**SRS_OUTPROCESS_LOADER_27_061: [** If the grace period expired, `OutprocessLoader_JoinChildProcesses` shall signal each child, by calling `int uv_process_kill(uv_process_t * process, int signum)` passing `SIGTERM` for `signum`. **]**

**SRS_OUTPROCESS_LOADER_27_068: [** `OutprocessLoader_JoinChildProcesses` shall destroy the timer, by calling `void tickcounter_destroy(TICK_COUNTER_HANDLE tick_counter)`. **]**

**SRS_OUTPROCESS_LOADER_27_062: [** `OutprocessLoader_JoinChildProcesses` shall join the child process management thread, by calling `THREADAPI_RESULT ThreadAPI_Join(THREAD_HANDLE threadHandle, int * res)`. **]**

**SRS_OUTPROCESS_LOADER_27_065: [** `OutprocessLoader_JoinChildProcesses` shall get the handle of each child processes, by calling `void * VECTOR_element(VECTOR_HANDLE handle, size_t index)`. **]**

**SRS_OUTPROCESS_LOADER_27_066: [** `OutprocessLoader_JoinChildProcesses` shall free the resources allocated to each child, by calling `void free(void * _Block)` passing the child handle as `_Block`. **]**

**SRS_OUTPROCESS_LOADER_27_067: [** `OutprocessLoader_JoinChildProcesses` shall destroy the vector of child processes, by calling `void VECTOR_destroy(VECTOR_HANDLE handle)`. **]**


launch_child_process_from_entrypoint (*internal*)
-------------------------------------------------

```C
int launch_child_process_from_entrypoint (OUTPROCESS_LOADER_ENTRYPOINT * outprocess_entry);
```

**SRS_OUTPROCESS_LOADER_27_098: [** *Prerequisite Check* If a vector for child processes already exists, then `launch_child_process_from_entrypoint` shall not attempt to recreate the vector. **]**

**SRS_OUTPROCESS_LOADER_27_069: [** `launch_child_process_from_entrypoint` shall attempt to create a vector for child processes (unless previously created), by calling `VECTOR_HANDLE VECTOR_create(size_t elementSize)` using `sizeof(uv_process_t *)` as `elementSize`. **]**

**SRS_OUTPROCESS_LOADER_27_070: [** If a vector for the child processes does not exist, then `launch_child_process_from_entrypoint` shall return a non-zero value. **]**

**SRS_OUTPROCESS_LOADER_27_071: [** `launch_child_process_from_entrypoint` shall allocate the memory for the child handle, by calling `void * malloc(size_t _Size)` passing `sizeof(uv_process_t)` as `_Size`. **]**

**SRS_OUTPROCESS_LOADER_27_072: [** If unable to allocate memory for the child handle, then `launch_child_process_from_entrypoint` shall return a non-zero value. **]**

**SRS_OUTPROCESS_LOADER_27_073: [** `launch_child_process_from_entrypoint` shall store the child's handle, by calling `int VECTOR_push_back(VECTOR_HANDLE handle, const void * elements, size_t numElements)` passing the process vector as `handle` and 1 as `numElements`. **]**

**SRS_OUTPROCESS_LOADER_27_074: [** If unable to store the child's handle, then `launch_child_process_from_entrypoint` shall free the memory allocated to the child process handle and return a non-zero value. **]**

**SRS_OUTPROCESS_LOADER_27_075: [** `launch_child_process_from_entrypoint` shall enqueue the child process to be spawned, by calling `int uv_spawn(uv_loop_t * loop, uv_process_t * handle, const uv_process_options_t * options)` passing the result of `uv_default_loop()` as `loop`, the newly allocated process handle as `handle` and the options parsed from the entrypoint as `options`. **]**

**SRS_OUTPROCESS_LOADER_27_076: [** If unable to enqueue the child process, then `launch_child_process_from_entrypoint` shall remove the stored handle, free the memory allocated to the child process handle and return a non-zero value. **]**

**SRS_OUTPROCESS_LOADER_27_077: [** `launch_child_process_from_entrypoint` shall spawn the enqueued child processes. **]**

**SRS_OUTPROCESS_LOADER_27_078: [** If launching the enqueued child processes fails, then `launch_child_process_from_entrypoint` shall return a non-zero value. **]**

**SRS_OUTPROCESS_LOADER_27_079: [** If no errors are encountered, then `launch_child_process_from_entrypoint` shall return zero. **]**


spawn_child_processes (*internal*)
----------------------------------

```C
int spawn_child_processes (void * context);
```

**SRS_OUTPROCESS_LOADER_27_080: [** `spawn_child_processes` shall start the child process management thread, by calling `int uv_run(uv_loop_t * loop, uv_run_mode mode)` passing the result of `uv_default_loop()` for `loop` and `UV_RUN_DEFAULT` for `mode`. **]**

**SRS_OUTPROCESS_LOADER_27_099: [** `spawn_child_processes` shall return the result of the child process management thread to the parent thread, by calling `void ThreadAPI_Exit(int res)` passing the result of `uv_run()` as `res`. **]**

**SRS_OUTPROCESS_LOADER_27_081: [** If no errors are encountered, then `spawn_child_processes` shall return zero. **]**


update_entrypoint_with_launch_object (*internal*)
--------------------------------------------------------------

```C
int update_entrypoint_with_launch_object (OUTPROCESS_LOADER_ENTRYPOINT * outprocess_entry, const char * launch_path, JSON_Array * launch_args)
```

**SRS_OUTPROCESS_LOADER_27_082: [** `update_entrypoint_with_launch_object` shall retrieve the file path, by calling `const char * json_object_get_string(const JSON_Object * object, const char * name)` passing `path` as `name`. **]**

**SRS_OUTPROCESS_LOADER_27_083: [** `update_entrypoint_with_launch_object` shall retrieve the JSON arguments array, by calling `JSON_Array json_object_get_array(const JSON_Object * object, const char * name)` passing `args` as `name`. **]**

**SRS_OUTPROCESS_LOADER_27_084: [** `update_entrypoint_with_launch_object` shall determine the size of the JSON arguments array, by calling `size_t json_array_get_count(const JSON_Array * array)`. **]**

**SRS_OUTPROCESS_LOADER_27_085: [** `update_entrypoint_with_launch_object` shall allocate the argument array, by calling `void * malloc(size _Size)` passing the result of `json_array_get_count` plus two, one for passing the file path as the first argument and the second for the NULL terminating pointer required by libUV. **]**

**SRS_OUTPROCESS_LOADER_27_086: [** If unable to allocate the array, then `update_entrypoint_with_launch_object` shall return a non-zero value. **]**

**SRS_OUTPROCESS_LOADER_27_087: [** `update_entrypoint_with_launch_object` shall allocate the 
space necessary to copy the file path, by calling `void * malloc(size _Size)`. **]**

**SRS_OUTPROCESS_LOADER_27_088: [** If unable to allocate space for the file path, then `update_entrypoint_with_launch_object` shall free the argument array and return a non-zero value. **]**

**SRS_OUTPROCESS_LOADER_27_089: [** `update_entrypoint_with_launch_object` shall retrieve each argument from the JSON arguments array, by calling `const char * json_array_get_string(const JSON_Array * array, size_t index)`. **]**

**SRS_OUTPROCESS_LOADER_27_090: [** `update_entrypoint_with_launch_object` shall allocate the space necessary for each argument, by calling `void * malloc(size _Size)`. **]**

**SRS_OUTPROCESS_LOADER_27_091: [** If unable to allocate space for the argument, then `update_entrypoint_with_launch_object` shall free the argument array and return a non-zero value. **]**

**SRS_OUTPROCESS_LOADER_27_092: [** If no errors are encountered, then `update_entrypoint_with_launch_object` shall return zero. **]**


validate_launch_arguments (*internal*)
--------------------------------------

```C
int validate_launch_arguments (const JSON_Object * launch_object);
```

**SRS_OUTPROCESS_LOADER_27_093: [** `validate_launch_arguments` shall retrieve the file path, by calling `const char * json_object_get_string(const JSON_Object * object, const char * name)` passing `path` as `name`. **]**

**SRS_OUTPROCESS_LOADER_27_094: [** If unable to retrieve the file path, then `validate_launch_arguments` shall return a non-zero value. **]**

**SRS_OUTPROCESS_LOADER_27_095: [** `validate_launch_arguments` shall test for the optional parameter grace period, by calling `const char * json_object_get_string(const JSON_Object * object, const char * name)` passing `grace.period.ms` as `name`. **]**

**SRS_OUTPROCESS_LOADER_27_096: [** `validate_launch_arguments` shall retrieve the grace period, by calling `double json_object_get_number(const JSON_Object * object, const char * name)` passing `grace.period.ms` as `name`. **]**

**SRS_OUTPROCESS_LOADER_27_097: [** If no errors are encountered, then `validate_launch_arguments` shall return zero. **]**

