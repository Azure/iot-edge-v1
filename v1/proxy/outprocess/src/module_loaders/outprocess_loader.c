// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "module_loaders/outprocess_loader.h"

#include <signal.h>
#include <stdlib.h>
#include <uv.h>

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/uniqueid.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/xlogging.h"

#include "parson.h"
#include "module.h"
#include "module_loader.h"
#include "module_loaders/outprocess_module.h"

DEFINE_ENUM_STRINGS(OUTPROCESS_LOADER_ACTIVATION_TYPE, OUTPROCESS_LOADER_ACTIVATION_TYPE_VALUES);

#define LOADER_GUID_SIZE 37
#define IPC_URI_HEAD "ipc://"
#define IPC_URI_HEAD_SIZE 6
#define MESSAGE_URI_SIZE (INPROC_URI_HEAD_SIZE + LOADER_GUID_SIZE +1)

#define GRACE_PERIOD_MS_DEFAULT 3000
#define REMOTE_MESSAGE_WAIT_DEFAULT 1000
#define GRACE_AWAIT_DELAY_MS 100

typedef struct OUTPROCESS_MODULE_HANDLE_DATA_TAG
{
    const MODULE_API* api;

} OUTPROCESS_MODULE_HANDLE_DATA;

static VECTOR_HANDLE uv_processes = NULL;
static THREAD_HANDLE uv_thread = NULL;
static tickcounter_ms_t uv_process_grace_period_ms = 0;

void exit_cb(uv_process_t* p, int64_t exit_status, int term_signal)
{
    LogInfo("Out process: %d, exited with status: "INT64_PRINTF", signal: %d !!\n", p->pid, exit_status, term_signal);
    uv_close((uv_handle_t *) p, NULL);
}

int launch_child_process_from_entrypoint (OUTPROCESS_LOADER_ENTRYPOINT * outprocess_entry)
{
    int result;
    uv_process_t * child = NULL;
    const uv_process_options_t options = {
        .exit_cb = exit_cb,
        .file = outprocess_entry->process_argv[0],
        .args = outprocess_entry->process_argv,
        .flags = UV_PROCESS_WINDOWS_VERBATIM_ARGUMENTS
    };

    /* Codes_SRS_OUTPROCESS_LOADER_27_098: [ If a vector for child processes already exists, then `launch_child_process_from_entrypoint` shall not attempt to recreate the vector. ] */
    if (NULL == uv_processes)
    {
        /* Codes_SRS_OUTPROCESS_LOADER_27_069: [ `launch_child_process_from_entrypoint` shall attempt to create a vector for child processes(unless previously created), by calling `VECTOR_HANDLE VECTOR_create(size_t elementSize)` using `sizeof(uv_process_t *)` as `elementSize`. ]*/
        uv_processes = VECTOR_create(sizeof(uv_process_t *));
    }

    /* Codes_SRS_OUTPROCESS_LOADER_27_070: [ If a vector for the child processes does not exist, then `launch_child_process_from_entrypoint` shall return a non-zero value. ] */
    if (NULL == uv_processes)
    {
        /* Codes_SRS_OUTPROCESS_LOADER_27_072: [ If unable to allocate memory for the child handle, then `launch_child_process_from_entrypoint` shall return a non-zero value. ] */
        LogError("Unable to create uv_process_t vector");
        result = __LINE__;
    }
    /* Codes_SRS_OUTPROCESS_LOADER_27_071: [ `launch_child_process_from_entrypoint` shall allocate the memory for the child handle, by calling `void * malloc(size_t _Size)` passing `sizeof(uv_process_t)` as `_Size`. ] */
    else if (NULL == (child = (uv_process_t *)malloc(sizeof(uv_process_t))))
    {
        /* Codes_SRS_OUTPROCESS_LOADER_27_074: [ If unable to store the child's handle, then `launch_child_process_from_entrypoint` shall free the memory allocated to the child process handle and return a non-zero value. ] */
        LogError("Unable to allocate child handle");
        result = __LINE__;
    }
    /* Codes_SRS_OUTPROCESS_LOADER_27_073: [ `launch_child_process_from_entrypoint` shall store the child's handle, by calling `int VECTOR_push_back(VECTOR_HANDLE handle, const void * elements, size_t numElements)` passing the process vector as `handle` the pointer to the newly allocated memory for the process context as `elements` and 1 as `numElements`. ]*/
    else if (0 != VECTOR_push_back(uv_processes, &child, 1))
    {
        /* Codes_SRS_OUTPROCESS_LOADER_27_076: [ If unable to enqueue the child process, then `launch_child_process_from_entrypoint` shall remove the stored handle, free the memory allocated to the child process handle and return a non-zero value. ] */
        LogError("Unable to store child handle");
        free(child);
        result = __LINE__;
    }
    /* Codes_SRS_OUTPROCESS_LOADER_27_075: [ `launch_child_process_from_entrypoint` shall enqueue the child process to be spawned, by calling `int uv_spawn(uv_loop_t * loop, uv_process_t * handle, const uv_process_options_t * options)` passing the result of `uv_default_loop()` as `loop`, the newly allocated process handle as `handle`. ] */
    else if (0 != uv_spawn(uv_default_loop(), child, &options))
    {
        /* Codes_SRS_OUTPROCESS_LOADER_27_078: [ If launching the enqueued child processes fails, then `launch_child_process_from_entrypoint` shall return a non - zero value. ] */
        LogError("Unable to spawn child process");
        (void)VECTOR_erase(uv_processes, VECTOR_back(uv_processes), 1);
        free(child);
        result = __LINE__;
    }
    else
    {
        /* Codes_SRS_OUTPROCESS_LOADER_27_079: [ If no errors are encountered, then `launch_child_process_from_entrypoint` shall return zero. ] */
        result = 0;
    }

    return result;
}

int spawn_child_processes (void * context)
{
    (void)context;

    /* Codes_SRS_OUTPROCESS_LOADER_27_080: [ `spawn_child_processes` shall start the child process management thread, by calling `int uv_run(uv_loop_t * loop, uv_run_mode mode)` passing the result of `uv_default_loop()` for `loop` and `UV_RUN_DEFAULT` for `mode`. ] */
    /* Codes_SRS_OUTPROCESS_LOADER_27_099: [ `spawn_child_processes` shall return the result of the child process management thread to the parent thread, by calling `void ThreadAPI_Exit(int res)` passing the result of `uv_run()` as `res`. ] */
    ThreadAPI_Exit(uv_run(uv_default_loop(), UV_RUN_DEFAULT));

    /* Codes_SRS_OUTPROCESS_LOADER_27_081: [ If no errors are encountered, then `spawn_child_processes` shall return zero. ] */
    return 0;
}

int update_entrypoint_with_launch_object(OUTPROCESS_LOADER_ENTRYPOINT * outprocess_entry, const JSON_Object * launch_object)
{
    int result;

    /* Codes_SRS_OUTPROCESS_LOADER_27_082: [ `update_entrypoint_with_launch_object` shall retrieve the file path, by calling `const char * json_object_get_string(const JSON_Object * object, const char * name)` passing `path` as `name`. **] */
    const char * launch_path = json_object_get_string(launch_object, "path");
    /* Codes_SRS_OUTPROCESS_LOADER_27_083: [ `update_entrypoint_with_launch_object` shall retrieve the JSON arguments array, by calling `JSON_Array json_object_get_array(const JSON_Object * object, const char * name)` passing `args` as `name`. ] */
    JSON_Array * launch_args = json_object_get_array(launch_object, "args");

    /* Codes_SRS_OUTPROCESS_LOADER_27_084: [ `update_entrypoint_with_launch_object` shall determine the size of the JSON arguments array, by calling `size_t json_array_get_count(const JSON_Array * array)`. ] */
    outprocess_entry->process_argc = (json_array_get_count(launch_args) + 1);  // Add 1 to make room for launch path
    /* Codes_SRS_OUTPROCESS_LOADER_27_085: [ `update_entrypoint_with_launch_object` shall allocate the argument array, by calling `void * malloc(size _Size)` passing the result of `json_array_get_count` plus two, one for passing the file path as the first argument and the second for the NULL terminating pointer required by libUV. ] */
    if (NULL == (outprocess_entry->process_argv = (char **)malloc(sizeof(char *) * (outprocess_entry->process_argc + 1)))) // Add 1 to make room for NULL terminator
    {
        /* Codes_SRS_OUTPROCESS_LOADER_27_086: [ If unable to allocate the array, then `update_entrypoint_with_launch_object` shall return a non - zero value. ] */
        LogError("Unable to allocate argument string array.");
        result = __LINE__;
    }
    /* Codes_SRS_OUTPROCESS_LOADER_27_087: [ `update_entrypoint_with_launch_object` shall allocate the space necessary to copy the file path, by calling `void * malloc(size _Size)`. ] */
    else if (NULL == (outprocess_entry->process_argv[0] = (char *)malloc(strlen(launch_path) + 1)))
    {
        /* Codes_SRS_OUTPROCESS_LOADER_27_088: [ If unable to allocate space for the file path, then `update_entrypoint_with_launch_object` shall free the argument array and return a non-zero value. ] */
        LogError("Unable to allocate argument[0] string.");
        free(outprocess_entry->process_argv);
        result = __LINE__;
    }
    else if (NULL == strcpy(outprocess_entry->process_argv[0], launch_path))
    {
        LogError("Unable to copy argument[0] string.");
        free(outprocess_entry->process_argv[0]);
        free(outprocess_entry->process_argv);
        result = __LINE__;
    }
    else
    {
        int i;
        /* Codes_SRS_OUTPROCESS_LOADER_27_092: [ If no errors are encountered, then `update_entrypoint_with_launch_object` shall return zero. ] */
        for (result = 0, i = 1; i < (int)outprocess_entry->process_argc; ++i)
        {
            /* Codes_SRS_OUTPROCESS_LOADER_27_089: [ `update_entrypoint_with_launch_object` shall retrieve each argument from the JSON arguments array, by calling `const char * json_array_get_string(const JSON_Array * array, size_t index)`. ] */
            const char * arg = json_array_get_string(launch_args, (i - 1));
            /* Codes_SRS_OUTPROCESS_LOADER_27_090: [ `update_entrypoint_with_launch_object` shall allocate the space necessary for each argument, by calling `void * malloc(size _Size)`. ] */
            if (NULL == (outprocess_entry->process_argv[i] = (char *)malloc(strlen(arg) + 1)))
            {
                /* Codes_SRS_OUTPROCESS_LOADER_27_091: [ If unable to allocate space for the argument, then `update_entrypoint_with_launch_object` shall free the argument array and return a non - zero value. ] */
                LogError("Unable to allocate argument[%lu] string.", i);
                for (int j = (i - 1); 0 <= j; --j) { free(outprocess_entry->process_argv[j]); }
                free(outprocess_entry->process_argv);
                result = __LINE__;
                break;
            }
            else if (NULL == strcpy(outprocess_entry->process_argv[i], arg))
            {
                LogError("Unable to copy argument[%lu] string.", i);
                for (int j = i; 0 <= j; --j) { free(outprocess_entry->process_argv[j]); }
                free(outprocess_entry->process_argv);
                result = __LINE__;
                break;
            }
        }
        outprocess_entry->process_argv[i] = NULL;  // NULL terminate the array
    }

    return result;
}

int validate_launch_arguments(const JSON_Object * launch_object)
{
    int result;

    /* Codes_SRS_OUTPROCESS_LOADER_27_093: [ `validate_launch_arguments` shall retrieve the file path, by calling `const char * json_object_get_string(const JSON_Object * object, const char * name)` passing `path` as `name`. ] */
    if (NULL == json_object_get_string(launch_object, "path"))
    {
        /* Codes_SRS_OUTPROCESS_LOADER_27_094: [ If unable to retrieve the file path, then `validate_launch_arguments` shall return a non - zero value. ] */
        LogError("Activation type launch specified with nothing to launch!");
        result = __LINE__;
    }
    else
    {
        /* Codes_SRS_OUTPROCESS_LOADER_27_095: [ `validate_launch_arguments` shall test for the optional parameter grace period, by calling `const char * json_object_get_string(const JSON_Object * object, const char * name)` passing `grace.period.ms` as `name`. ] */
        if (NULL == json_object_get_string(launch_object, "grace.period.ms"))
        {
            // No value set, use default
            if (GRACE_PERIOD_MS_DEFAULT > uv_process_grace_period_ms)
            {
                uv_process_grace_period_ms = GRACE_PERIOD_MS_DEFAULT;
            }
        }
        else
        {
            /* Codes_SRS_OUTPROCESS_LOADER_27_096: [ `validate_launch_arguments` shall retrieve the grace period (if provided), by calling `double json_object_get_number(const JSON_Object * object, const char * name)` passing `grace.period.ms` as `name`. ] */
            const size_t ms = (size_t)json_object_get_number(launch_object, "grace.period.ms");
            if (ms > uv_process_grace_period_ms)
            {
                uv_process_grace_period_ms = ms;
            }
        }
        /* Codes_SRS_OUTPROCESS_LOADER_27_097: [ If no errors are encountered, then `validate_launch_arguments` shall return zero. ] */
        result = 0;
    }

    return result;
}

int OutprocessLoader_SpawnChildProcesses(void) {
    int result;

    if (NULL == uv_processes)
    {
        /* Codes_SRS_OUTPROCESS_LOADER_27_045: [ Prerequisite Check - If no processes have been enqueued, then `OutprocessLoader_SpawnChildProcesses` shall take no action and return zero. ] */
        LogInfo("No child process(es) scheduled.");
        result = 0;
    }
    /* Codes_SRS_OUTPROCESS_LOADER_27_046: [ Prerequisite Check - If child processes are already running, then `OutprocessLoader_SpawnChildProcesses` shall take no action and return zero. ] */
    else if (NULL != uv_thread)
    {
        LogInfo("Child process(es) already running!");
        result = 0;
    }
    /* Codes_SRS_OUTPROCESS_LOADER_27_047: [ `OutprocessLoader_SpawnChildProcesses` shall launch the enqueued child processes by calling `THREADAPI_RESULT ThreadAPI_Create(THREAD_HANDLE * threadHandle, THREAD_START_FUNC func, void * arg)`. ] */
    else if (THREADAPI_OK != ThreadAPI_Create(&uv_thread, spawn_child_processes, NULL))
    {
        /* Codes_SRS_OUTPROCESS_LOADER_27_048: [** If launching the enqueued child processes fails, then `OutprocessLoader_SpawnChildProcesses` shall return a non-zero value. ] */
        LogError("Unable to spawn child process(es)!");
        result = __LINE__;
    }
    else
    {
        /* Codes_SRS_OUTPROCESS_LOADER_27_049: [** If no errors are encountered, then `OutprocessLoader_SpawnChildProcesses` shall return zero. ] */
        result = 0;
    }

    return result;
}

void OutprocessLoader_JoinChildProcesses(void) {
    /* Codes_SRS_OUTPROCESS_LOADER_27_064: [ `OutprocessLoader_JoinChildProcesses` shall get the count of child processes, by calling `size_t VECTOR_size(VECTOR_HANDLE handle)`. ] */
    const size_t child_count = (uv_processes == NULL)? 0 :VECTOR_size(uv_processes);

    TICK_COUNTER_HANDLE ticks = NULL;
    tickcounter_ms_t started_waiting;
    int uv_thread_result = 0;
    bool timed_out;

    if (uv_loop_alive(uv_default_loop()))
    {
        /* Codes_SRS_OUTPROCESS_LOADER_27_051: [ `OutprocessLoader_JoinChildProcesses` shall create a timer to test for timeout, by calling `TICK_COUNTER_HANDLE tickcounter_create(void)`. ] */
        if (NULL == (ticks = tickcounter_create()))
        {
            /* Codes_SRS_OUTPROCESS_LOADER_27_052: [ If unable to create a timer, `OutprocessLoader_JoinChildProcesses` shall abandon awaiting the grace period. ] */
            timed_out = true;
            LogError("failed to create tickcounter");
        }
        /* Codes_SRS_OUTPROCESS_LOADER_27_053: [ `OutprocessLoader_JoinChildProcesses` shall mark the begin time, by calling `int tickcounter_get_current_ms(TICK_COUNTER_HANDLE tick_counter, tickcounter_ms_t * current_ms)` using the handle called by the previous call to `tickcounter_create`. ] */
        else if (tickcounter_get_current_ms(ticks, &started_waiting))
        {
            /* Codes_SRS_OUTPROCESS_LOADER_27_054: [** If unable to mark the begin time, `OutprocessLoader_JoinChildProcesses` shall abandon awaiting the grace period. ] */
            timed_out = true;
            LogError("failed to sample tickcounter");
        }
        else
        {
            // Wait for child to clean up
            tickcounter_ms_t now = started_waiting;
            /* Codes_SRS_OUTPROCESS_LOADER_27_058: [ `OutprocessLoader_JoinChildProcesses` shall await the grace period in 100ms increments, by calling `void ThreadAPI_Sleep(unsigned int milliseconds)` passing 100 for `milliseconds`. ] */
            for (timed_out = true; (now - started_waiting) < uv_process_grace_period_ms; ThreadAPI_Sleep(GRACE_AWAIT_DELAY_MS))
            {
                /* Codes_SRS_OUTPROCESS_LOADER_27_055: [ While awaiting the grace period, `OutprocessLoader_JoinChildProcesses` shall mark the current time, by calling `int tickcounter_get_current_ms(TICK_COUNTER_HANDLE tick_counter, tickcounter_ms_t * current_ms)` using the handle called by the previous call to `tickcounter_create`. ] */
                if (tickcounter_get_current_ms(ticks, &now))
                {
                    /* Codes_SRS_OUTPROCESS_LOADER_27_056: [ If unable to mark the current time, `OutprocessLoader_JoinChildProcesses` shall abandon awaiting the grace period. ] */
                    LogError("failed to sample tickcounter");
                    break;
                }
                /* Codes_SRS_OUTPROCESS_LOADER_27_057: [ While awaiting the grace period, `OutprocessLoader_JoinChildProcesses` shall check if the processes are running, by calling `int uv_loop_alive(const uv_loop_t * loop)` using the result of `uv_default_loop()` for `loop`. ] */
                else if (!uv_loop_alive(uv_default_loop()))
                {
                    /* Codes_SRS_OUTPROCESS_LOADER_27_059: [** If the child processes are not running, `OutprocessLoader_JoinChildProcesses` shall shall immediately join the child process management thread. ] */
                    timed_out = false;
                    break;
                }
            }
        }

        // Children did not clean up, now SIGNAL
        if (timed_out)
        {
            for (size_t i = 0; i < child_count; ++i)
            {
                /* Codes_SRS_OUTPROCESS_LOADER_27_060: [ If the grace period expired, `OutprocessLoader_JoinChildProcesses` shall get the handle of each child processes, by calling `void * VECTOR_element(VECTOR_HANDLE handle, size_t index)`. ] */
                uv_process_t * child = *((uv_process_t **)VECTOR_element(uv_processes, i));
                /* Codes_SRS_OUTPROCESS_LOADER_27_061: [ If the grace period expired, `OutprocessLoader_JoinChildProcesses` shall signal each child, by calling `int uv_process_kill(uv_process_t * process, int signum)` passing `SIGTERM` for `signum`. ] */
                (void)uv_process_kill(child, SIGTERM);
            }
        }
        /* Codes_SRS_OUTPROCESS_LOADER_27_068: [ `OutprocessLoader_JoinChildProcesses` shall destroy the timer, by calling `void tickcounter_destroy(TICK_COUNTER_HANDLE tick_counter)`. ] */
        tickcounter_destroy(ticks);
    }

    uv_process_grace_period_ms = 0;

    /* Codes_SRS_OUTPROCESS_LOADER_27_050: [ Prerequisite Check - If no threads are running, then `OutprocessLoader_JoinChildProcesses` shall abandon the effort to join the child processes immediately. ] */
    if (NULL != uv_thread)
    {
        /* Codes_SRS_OUTPROCESS_LOADER_27_062: [ `OutprocessLoader_JoinChildProcesses` shall join the child process management thread, by calling `THREADAPI_RESULT ThreadAPI_Join(THREAD_HANDLE threadHandle, int * res)`. ] */
        (void)ThreadAPI_Join(uv_thread, &uv_thread_result);
        uv_thread = NULL;
    }

    /*
     * Under normal conditions the process vector would not exist without
     * a process management thread, but under certain error conditions, it
     * is necessary to clean the process resources without regard for the 
     * existence of the process management thread.
     */
     /* Codes_SRS_OUTPROCESS_LOADER_27_063: [ If no processes are running, then `OutprocessLoader_JoinChildProcesses` shall immediately join the child process management thread. ] */
    if (NULL != uv_processes)
    {
        for (size_t i = 0; i < child_count; ++i) {
            /* Codes_SRS_OUTPROCESS_LOADER_27_065: [ `OutprocessLoader_JoinChildProcesses` shall get the handle of each child processes, by calling `void * VECTOR_element(VECTOR_HANDLE handle, size_t index)`. ] */
            uv_process_t * child = *((uv_process_t **)VECTOR_element(uv_processes, i));
            /* Codes_SRS_OUTPROCESS_LOADER_27_066: [ `OutprocessLoader_JoinChildProcesses` shall free the resources allocated to each child, by calling `void free(void * _Block)` passing the child handle as `_Block`. ] */
            free(child);
        }
        /* Codes_SRS_OUTPROCESS_LOADER_27_067: [ `OutprocessLoader_JoinChildProcesses` shall destroy the vector of child processes, by calling `void VECTOR_destroy(VECTOR_HANDLE handle)`. ] */
        VECTOR_destroy(uv_processes);
        uv_processes = NULL;
    }
}

static MODULE_LIBRARY_HANDLE OutprocessModuleLoader_Load(const MODULE_LOADER* loader, const void* entrypoint)
{
    OUTPROCESS_LOADER_ENTRYPOINT * outprocess_entry = (OUTPROCESS_LOADER_ENTRYPOINT *)entrypoint;
    OUTPROCESS_MODULE_HANDLE_DATA * result;

    if (loader == NULL || entrypoint == NULL)
    {
        /*Codes_SRS_OUTPROCESS_LOADER_17_001: [ If loader or entrypoint are NULL, then this function shall return NULL. ] */
        result = NULL;
        LogError(
            "invalid input - loader = %p, entrypoint = %p",
            loader, entrypoint
        );
    }
    else if (loader->type != OUTPROCESS)
    {
        /*Codes_SRS_OUTPROCESS_LOADER_17_042: [ If the loader type is not OUTPROCESS, then this function shall return NULL. ] */
        result = NULL;
        LogError("loader->type is not remote");
    }
    /*Codes_SRS_OUTPROCESS_LOADER_17_002: [  If the entrypoint's `control_id` is `NULL`, then this function shall return `NULL`. ] */
    /*Codes_SRS_OUTPROCESS_LOADER_27_003: [ If the entrypoint's `activation_type` is invalid, then `OutprocessModuleLoader_Load` shall return `NULL`. ] */
    else if ((outprocess_entry->control_id == NULL) ||
            ((outprocess_entry->activation_type != OUTPROCESS_LOADER_ACTIVATION_NONE) &&
            (outprocess_entry->activation_type != OUTPROCESS_LOADER_ACTIVATION_LAUNCH)))
    {
        result = NULL;
        LogError("Invalid arguments activation type");
    }
    /*Codes_SRS_OUTPROCESS_LOADER_27_005: [ Launch - `OutprocessModuleLoader_Load` shall launch the child process identified by the entrypoint. ]*/
    else if ((OUTPROCESS_LOADER_ACTIVATION_LAUNCH == outprocess_entry->activation_type) && launch_child_process_from_entrypoint(outprocess_entry))
    {
        /*Codes_SRS_OUTPROCESS_LOADER_17_008: [ If any call in this function fails, this function shall return NULL. ] */
        result = NULL;
        LogError("Unable to launch external process!");
    }
    /* Codes_SRS_OUTPROCESS_LOADER_27_077: [ Launch - `OutprocessModuleLoader_Load` shall spawn the enqueued child processes. ] */
    else if ((OUTPROCESS_LOADER_ACTIVATION_LAUNCH == outprocess_entry->activation_type) && OutprocessLoader_SpawnChildProcesses())
    {
        /*
        * Here, we are beyond the point of failing gracefully from inside the function.
        *
        * Due to the way libuv works, this function must be called after `uv_spawn` (in
        * order for the thread to stay alive) and the resources allocated during this
        * function will need to be left intact until they are reclaimed by a subsequent
        * call to `OutprocessLoader_JoinChildProcesses()`.
        *
        * To remedy this pitfall, the sychronous `Module_Create` behavior of the
        * outprocess module would need to be made asynchronous. This would in turn
        * eliminate the need to call `OutprocessLoader_SpawnChildProcesses` from
        * inside this function, and therefore the existence of this state altogether.
        */
        result = NULL;
        LogError("Unable to launch uv thread! Must call `OutprocessLoader_JoinChildProcesses` to recover.");
    }
    /*Codes_SRS_OUTPROCESS_LOADER_17_004: [ The loader shall allocate memory for the loader handle. ] */
    else if (NULL == (result = (OUTPROCESS_MODULE_HANDLE_DATA*)malloc(sizeof(OUTPROCESS_MODULE_HANDLE_DATA))))
    {
        /*Codes_SRS_OUTPROCESS_LOADER_17_008: [ If any call in this function fails, this function shall return NULL. ] */
        LogError("malloc(sizeof(OUTPROCESS_MODULE_HANDLE_DATA)) failed.");
    }
    else
    {
        /*Codes_SRS_OUTPROCESS_LOADER_17_006: [ The loader shall store the Outprocess_Module_API_all in the loader handle. ] */
        /*Codes_SRS_OUTPROCESS_LOADER_17_007: [ Upon success, this function shall return a valid pointer to the loader handle. ] */
        result->api = (const MODULE_API*)&Outprocess_Module_API_all;
    }

    return result;
}

static const MODULE_API* OutprocessModuleLoader_GetModuleApi(const MODULE_LOADER* loader, MODULE_LIBRARY_HANDLE moduleLibraryHandle)
{
    (void)loader;

    const MODULE_API* result;

    if (moduleLibraryHandle == NULL)
    {
        result = NULL;
        LogError("moduleLibraryHandle is NULL");
    }
    else
    {
        /*Codes_SRS_OUTPROCESS_LOADER_17_009: [ This function shall return a valid pointer to the Outprocess_Module_API_all MODULE_API. ] */
        OUTPROCESS_MODULE_HANDLE_DATA* loader_data = moduleLibraryHandle;
        result = loader_data->api;
    }
    return result;
}

static void OutprocessModuleLoader_Unload(const struct MODULE_LOADER_TAG* loader, MODULE_LIBRARY_HANDLE handle)
{
    (void)loader;

    if (handle != NULL)
    {
        /*Codes_SRS_OUTPROCESS_LOADER_17_011: [ This function shall release all resources created by this loader. ] */
        OUTPROCESS_MODULE_HANDLE_DATA* loader_data = (OUTPROCESS_MODULE_HANDLE_DATA*)handle;

        free(loader_data);
    }
    else
    {
        /*Codes_SRS_OUTPROCESS_LOADER_17_010: [ This function shall do nothing if moduleLibraryHandle is NULL. ] */
        LogError("moduleLibraryHandle is NULL");
    }
}

static void* OutprocessModuleLoader_ParseEntrypointFromJson(const struct MODULE_LOADER_TAG* loader, const JSON_Value* json)
{
    (void)loader;

    OUTPROCESS_LOADER_ENTRYPOINT * config;
    JSON_Object* entrypoint;

    if (json == NULL)
    {
        /*Codes_SRS_OUTPROCESS_LOADER_17_012: [ This function shall return NULL if json is NULL. ] */
        LogError("json input is NULL");
        config = NULL;
    }
    // "json" must be an "object" type
    else if (json_value_get_type(json) != JSONObject)
    {
        LogError("'json' is not an object value");
        config = NULL;
    }
    else if (NULL == (entrypoint = json_value_get_object(json)))
    {
        LogError("json_value_get_object failed");

        config = NULL;
    }
    else
    {
        OUTPROCESS_LOADER_ACTIVATION_TYPE activationType;
        const char* activationTypeString = json_object_get_string(entrypoint, "activation.type");
        const char* controlId = json_object_get_string(entrypoint, "control.id");
        JSON_Object* launchObject = json_object_get_object(entrypoint, "launch");
        const char* messageId = json_object_get_string(entrypoint, "message.id");

        // Transform activation type string into OUTPROCESS_LOADER_ACTIVATION_TYPE
        if (activationTypeString)
        {
            if (!strncmp("none", activationTypeString, sizeof("none")))
            {
                activationType = OUTPROCESS_LOADER_ACTIVATION_NONE;
            }
            /*Codes_SRS_OUTPROCESS_LOADER_27_015: [ Launch - `OutprocessModuleLoader_ParseEntrypointFromJson` shall validate the launch parameters. ]*/
            else if ((!strncmp("launch", activationTypeString, sizeof("launch"))) && (0 == validate_launch_arguments(launchObject)))
            {
                /*Codes_SRS_OUTPROCESS_LOADER_17_021: [ This function shall return NULL if any calls fails. ]*/
                activationType = OUTPROCESS_LOADER_ACTIVATION_LAUNCH;
            }
            else
            {
                activationType = OUTPROCESS_LOADER_ACTIVATION_INVALID;
                LogError("Invalid activation type specified!");
            }
        }
        else
        {
            activationType = OUTPROCESS_LOADER_ACTIVATION_INVALID;
            LogError("No activation type specified!");
        }

        /*Codes_SRS_OUTPROCESS_LOADER_17_013: [ This function shall return NULL if "activation.type" is not present in json. ] */
        /*Codes_SRS_OUTPROCESS_LOADER_27_014: [ This function shall return NULL if "activation.type" is `OUTPROCESS_LOADER_ACTIVATION_INVALID`. */
        /*Codes_SRS_OUTPROCESS_LOADER_17_041: [ This function shall return NULL if "control.id" is not present in json. ] */
        if ((activationType == OUTPROCESS_LOADER_ACTIVATION_INVALID) || (controlId == NULL))
        {
            LogError("Invalid JSON parameters, activation type=[%s], controlURI=[%p]", 
                activationTypeString, controlId);

            /*Codes_SRS_OUTPROCESS_LOADER_17_021: [ This function shall return NULL if any calls fails. ]*/
            config = NULL;
        }
        /*Codes_SRS_OUTPROCESS_LOADER_17_016: [ This function shall allocate a OUTPROCESS_LOADER_ENTRYPOINT structure. ] */
        else if (NULL == (config = (OUTPROCESS_LOADER_ENTRYPOINT*)malloc(sizeof(OUTPROCESS_LOADER_ENTRYPOINT))))
        {
            /*Codes_SRS_OUTPROCESS_LOADER_17_021: [ This function shall return NULL if any calls fails. ] */
            LogError("Entrypoint allocation failed");
        }
        else
        {
            // Initialize variables to ensure proper clean-up behavior
            config->process_argc = 0;
            config->process_argv = NULL;

            /*Codes_SRS_OUTPROCESS_LOADER_17_018: [ This function shall assign the entrypoint control_id to the string value of "control.id" in json, NULL if not present. ] */
            if (NULL == (config->control_id = STRING_construct(controlId)))
            {
                /*Codes_SRS_OUTPROCESS_LOADER_17_021: [ This function shall return NULL if any calls fails. ] */
                LogError("Could not allocate loader args string");
                free(config);
                config = NULL;
            }
            /*Codes_SRS_OUTPROCESS_LOADER_27_020: [ Launch - `OutprocessModuleLoader_ParseEntrypointFromJson` shall update the entry point with the parsed launch parameters. ]*/
            else if ((OUTPROCESS_LOADER_ACTIVATION_LAUNCH == activationType) && update_entrypoint_with_launch_object(config, launchObject))
            {
                /*Codes_SRS_OUTPROCESS_LOADER_17_021: [ This function shall return NULL if any calls fails. ] */
                LogError("Unable to update entrypoint with launch parameters!");
                free(config);
                config = NULL;
            }
            else
            {
                /*Codes_SRS_OUTPROCESS_LOADER_17_043: [ This function shall read the "timeout" value. ]*/
                /*Codes_SRS_OUTPROCESS_LOADER_17_044: [ If "timeout" is set, the remote_message_wait shall be set to this value, else it will be set to a default of 1000 ms. ]*/
                double timeout = json_object_get_number(entrypoint, "timeout");
                if (timeout == 0)
                {
                    config->remote_message_wait = REMOTE_MESSAGE_WAIT_DEFAULT;
                }
                else
                {
                    config->remote_message_wait = (unsigned int)timeout;
                }

                /*Codes_SRS_OUTPROCESS_LOADER_17_017: [ This function shall assign the entrypoint activation_type to the decoded value. ] */
                config->activation_type = activationType;

                /*Codes_SRS_OUTPROCESS_LOADER_17_019: [ This function shall assign the entrypoint message_id to the string value of "message.id" in json, NULL if not present. ] */
                config->message_id = STRING_construct(messageId);

                /*Codes_SRS_OUTPROCESS_LOADER_17_022: [ This function shall return a valid pointer to an OUTPROCESS_LOADER_ENTRYPOINT on success. ]*/
            }
        }
    }

    return (void*)config;
}

static void OutprocessModuleLoader_FreeEntrypoint(const struct MODULE_LOADER_TAG* loader, void* entrypoint)
{
    (void)loader;

    if (entrypoint != NULL)
    {
        /*Codes_SRS_OUTPROCESS_LOADER_17_023: [ This function shall release all resources allocated by OutprocessModuleLoader_ParseEntrypointFromJson. ] */
        OUTPROCESS_LOADER_ENTRYPOINT* ep = (OUTPROCESS_LOADER_ENTRYPOINT*)entrypoint;
        if (ep->message_id != NULL)
            STRING_delete(ep->message_id); 
        STRING_delete(ep->control_id);
        if (ep->process_argv) {
            for (size_t i = 0; i < ep->process_argc; ++i) {
                free(ep->process_argv[i]);
            }
            free(ep->process_argv);
        }

        free(ep);
    }
    else
    {
        LogError("entrypoint is NULL");
    }
}

static MODULE_LOADER_BASE_CONFIGURATION* OutprocessModuleLoader_ParseConfigurationFromJson(const struct MODULE_LOADER_TAG* loader, const JSON_Value* json)
{
    (void)json;
    (void)loader;
    /*Codes_SRS_OUTPROCESS_LOADER_17_024: [ The out of process loader does not have any configuration. So this method shall return NULL. ]*/
    return NULL;
}

static void OutprocessModuleLoader_FreeConfiguration(const struct MODULE_LOADER_TAG* loader, MODULE_LOADER_BASE_CONFIGURATION* configuration)
{
    (void)loader;
    (void)configuration;
    /*Codes_SRS_OUTPROCESS_LOADER_17_025: [ This function shall move along, nothing to free here. ]*/
}

static void* OutprocessModuleLoader_BuildModuleConfiguration(
    const MODULE_LOADER* loader,
    const void* entrypoint,
    const void* module_configuration
)
{
    (void)loader;
    OUTPROCESS_MODULE_CONFIG * fullModuleConfiguration;

    if ((entrypoint == NULL) || (module_configuration == NULL))
    {
        /*Codes_SRS_OUTPROCESS_LOADER_17_026: [ This function shall return NULL if entrypoint, control_id, or module_configuration is NULL. ] */
        LogError("Remote Loader needs both entry point and module configuration");
        fullModuleConfiguration = NULL;
    }
    /*Codes_SRS_OUTPROCESS_LOADER_17_027: [ This function shall allocate a OUTPROCESS_MODULE_CONFIG structure. ]*/
    else if (NULL == (fullModuleConfiguration = (OUTPROCESS_MODULE_CONFIG*)malloc(sizeof(OUTPROCESS_MODULE_CONFIG))))
    {
        /*Codes_SRS_OUTPROCESS_LOADER_17_036: [ If any call fails, this function shall return NULL. ]*/
        LogError("couldn't allocate module config");
    }
    else
    {
        OUTPROCESS_LOADER_ENTRYPOINT* ep = (OUTPROCESS_LOADER_ENTRYPOINT*)entrypoint;
        char uuid[LOADER_GUID_SIZE];
        UNIQUEID_RESULT uuid_result = UNIQUEID_OK;

        if (ep->message_id == NULL)
        {
            /*Codes_SRS_OUTPROCESS_LOADER_17_029: [ If the entrypoint's message_id is NULL, then the loader shall construct an IPC uri. ]*/
            memset(uuid, 0, LOADER_GUID_SIZE);
            /*Codes_SRS_OUTPROCESS_LOADER_17_030: [ The loader shall create a unique id, if needed for URI constrution. ]*/
            uuid_result = UniqueId_Generate(uuid, LOADER_GUID_SIZE);
            if (uuid_result != UNIQUEID_OK)
            {
                LogError("Unable to generate unique Id.");
                fullModuleConfiguration->message_uri = NULL;
            }
            else
            {
                /*Codes_SRS_OUTPROCESS_LOADER_17_032: [ The message uri shall be composed of "ipc://" + unique id . ]*/
                fullModuleConfiguration->message_uri = STRING_construct_sprintf("%s%s", IPC_URI_HEAD, uuid);
            }
        }
        else
        {
            /*Codes_SRS_OUTPROCESS_LOADER_17_033: [ This function shall allocate and copy each string in OUTPROCESS_LOADER_ENTRYPOINT and assign them to the corresponding fields in OUTPROCESS_MODULE_CONFIG. ]*/
            fullModuleConfiguration->message_uri = STRING_construct_sprintf("%s%s", IPC_URI_HEAD, STRING_c_str(ep->message_id));
        }

        if (fullModuleConfiguration->message_uri == NULL)
        {
            /*Codes_SRS_OUTPROCESS_LOADER_17_036: [ If any call fails, this function shall return NULL. ]*/
            LogError("unable to create a message channel URI");
            free(fullModuleConfiguration);
            fullModuleConfiguration = NULL;
        }
        /*Codes_SRS_OUTPROCESS_LOADER_17_033: [ This function shall allocate and copy each string in OUTPROCESS_LOADER_ENTRYPOINT and assign them to the corresponding fields in OUTPROCESS_MODULE_CONFIG. ]*/
        else if (NULL == (fullModuleConfiguration->control_uri = STRING_construct_sprintf("%s%s", IPC_URI_HEAD, STRING_c_str(ep->control_id))))
        {
            /*Codes_SRS_OUTPROCESS_LOADER_17_026: [ This function shall return NULL if entrypoint, control_id, or module_configuration is NULL. ] */
            /*Codes_SRS_OUTPROCESS_LOADER_17_036: [ If any call fails, this function shall return NULL. ]*/
            LogError("unable to allocate a control channel URI");
            STRING_delete(fullModuleConfiguration->message_uri);
            free(fullModuleConfiguration);
            fullModuleConfiguration = NULL;
        }
        /*Codes_SRS_OUTPROCESS_LOADER_17_033: [ This function shall allocate and copy each string in OUTPROCESS_LOADER_ENTRYPOINT and assign them to the corresponding fields in OUTPROCESS_MODULE_CONFIG. ]*/
        /*Codes_SRS_OUTPROCESS_LOADER_17_034: [ This function shall allocate and copy the module_configuration string and assign it the OUTPROCESS_MODULE_CONFIG::outprocess_module_args field. ]*/
        else if (NULL == (fullModuleConfiguration->outprocess_module_args = STRING_clone((STRING_HANDLE)module_configuration)))
        {
            /*Codes_SRS_OUTPROCESS_LOADER_17_036: [ If any call fails, this function shall return NULL. ]*/
            LogError("unable to allocate a loader arguments string");
            STRING_delete(fullModuleConfiguration->message_uri);
            STRING_delete(fullModuleConfiguration->control_uri);
            free(fullModuleConfiguration);
            fullModuleConfiguration = NULL;
        }
        else
        {
            /*Codes_SRS_OUTPROCESS_LOADER_17_035: [ Upon success, this function shall return a valid pointer to an OUTPROCESS_MODULE_CONFIG structure. ]*/
            fullModuleConfiguration->remote_message_wait = ep->remote_message_wait;
            fullModuleConfiguration->lifecycle_model = OUTPROCESS_LIFECYCLE_SYNC;
        }
    }

    return (void *)fullModuleConfiguration;
}

static void OutprocessModuleLoader_FreeModuleConfiguration(const struct MODULE_LOADER_TAG* loader, const void* module_configuration)
{
    (void)loader;
    if (module_configuration != NULL)
    {
        /*Codes_SRS_OUTPROCESS_LOADER_17_037: [ This function shall release all memory allocated by OutprocessModuleLoader_BuildModuleConfiguration. ]*/
        OUTPROCESS_MODULE_CONFIG * config = (OUTPROCESS_MODULE_CONFIG*)module_configuration;
        STRING_delete(config->control_uri);
        STRING_delete(config->message_uri);
        STRING_delete(config->outprocess_module_args);
        free(config);
    }
}


static MODULE_LOADER_API Outprocess_Module_Loader_API =
{
    .Load = OutprocessModuleLoader_Load,
    .Unload = OutprocessModuleLoader_Unload,
    .GetApi = OutprocessModuleLoader_GetModuleApi,

    .ParseEntrypointFromJson = OutprocessModuleLoader_ParseEntrypointFromJson,
    .FreeEntrypoint = OutprocessModuleLoader_FreeEntrypoint,

    .ParseConfigurationFromJson = OutprocessModuleLoader_ParseConfigurationFromJson,
    .FreeConfiguration = OutprocessModuleLoader_FreeConfiguration,

    .BuildModuleConfiguration = OutprocessModuleLoader_BuildModuleConfiguration,
    .FreeModuleConfiguration = OutprocessModuleLoader_FreeModuleConfiguration
};

/*Codes_SRS_OUTPROCESS_LOADER_17_039: [ MODULE_LOADER::type shall be OUTPROCESS. ]*/
/*Codes_SRS_OUTPROCESS_LOADER_17_040: [ MODULE_LOADER::name shall be the string 'outprocess'. ]*/
static MODULE_LOADER OutProcess_Module_Loader =
{
    OUTPROCESS,
    OUTPROCESS_LOADER_NAME,
    NULL,
    &Outprocess_Module_Loader_API
};

const MODULE_LOADER* OutprocessLoader_Get(void)
{
    /*Codes_SRS_OUTPROCESS_LOADER_17_038: [ OutprocessModuleLoader_Get shall return a non-NULL pointer to a MODULE_LOADER struct. ]*/
    return &OutProcess_Module_Loader;
}
