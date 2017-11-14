## Overview

The [GNOME GIO](https://developer.gnome.org/gio/2.46/) library provides APIs for dealing with various I/O operations in a given application. We use GIO's implementation of the [D-Bus](https://www.freedesktop.org/wiki/Software/dbus/) message bus system in order to communicate with the D-Bus objects that [BlueZ](http://www.bluez.org) - the *Bluetooth* stack implemenation on Linux - provides. Given that almost all D-Bus API calls are asynchronous, it proves to be useful to build an abstraction that allows us to succinctly express in code the intention that a set of asynchronous operations should be performed in sequence with support for early bail out in case one of the asynchronous operations produce an error. This document specifies the requirements for all the APIs provided by this library.

## References

- [GIO Asynchronous Sequence API - High Level Design Document](./gio_async_seq_hld.md)
- [GNOME GIO](https://developer.gnome.org/gio/2.46/)

## Types

```c
typedef struct GIO_ASYNCSEQ_HANDLE_DATA_TAG* GIO_ASYNCSEQ_HANDLE;

#define GIO_ASYNCSEQ_RESULT_VALUES \
    GIO_ASYNCSEQ_ERROR, \
    GIO_ASYNCSEQ_OK

DEFINE_ENUM(GIO_ASYNCSEQ_RESULT, GIO_ASYNCSEQ_RESULT_VALUES);

#define GIO_ASYNCSEQ_STATE_VALUES \
    GIO_ASYNCSEQ_STATE_PENDING, \
    GIO_ASYNCSEQ_STATE_RUNNING, \
    GIO_ASYNCSEQ_STATE_COMPLETE, \
    GIO_ASYNCSEQ_STATE_ERROR

DEFINE_ENUM(GIO_ASYNCSEQ_STATE, GIO_ASYNCSEQ_STATE_VALUES);

typedef void (*GIO_ASYNCSEQ_CALLBACK)(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    gpointer previous_result,
    gpointer callback_context,
    GAsyncReadyCallback async_callback
);

typedef void (*GIO_ASYNCSEQ_ERROR_CALLBACK)(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    const GError* error
);

typedef gpointer (*GIO_ASYNCSEQ_FINISH_CALLBACK)(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    GAsyncResult* result,
    GError** error
);

typedef void(*GIO_ASYNCSEQ_COMPLETE_CALLBACK)(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    gpointer previous_result
);

extern GIO_ASYNCSEQ_HANDLE GIO_Async_Seq_Create(
    gpointer async_seq_context,
    GIO_ASYNCSEQ_ERROR_CALLBACK error_callback,
    GIO_ASYNCSEQ_COMPLETE_CALLBACK complete_callback
);

extern void GIO_Async_Seq_Destroy(GIO_ASYNCSEQ_HANDLE async_seq_handle);

extern gpointer GIO_Async_Seq_GetContext(
    GIO_ASYNCSEQ_HANDLE async_seq_handle
);

extern GIO_ASYNCSEQ_RESULT GIO_Async_Seq_Add(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    gpointer callback_context,
    ...
);

extern GIO_ASYNCSEQ_RESULT GIO_Async_Seq_Run_Async(
    GIO_ASYNCSEQ_HANDLE async_seq_handle
);
```

## GIO_Async_Seq_Create
```c
GIO_ASYNCSEQ_HANDLE GIO_Async_Seq_Create(
    gpointer async_seq_context,
    GIO_ASYNCSEQ_ERROR_CALLBACK error_callback,
    GIO_ASYNCSEQ_COMPLETE_CALLBACK complete_callback
);
```

**SRS_GIO_ASYNCSEQ_13_001: [** `GIO_Async_Seq_Create` shall return a non-`NULL` handle on successful execution. **]**

**SRS_GIO_ASYNCSEQ_13_002: [** `GIO_Async_Seq_Create` shall return `NULL` when any of the underlying platform calls fail. **]**

**SRS_GIO_ASYNCSEQ_13_005: [** `GIO_Async_Seq_Create` shall save the `async_seq_context` pointer so that it can be retrieved later by calling `GIO_Async_Seq_GetContext`. **]**

## GIO_Async_Seq_Destroy
```c
void GIO_Async_Seq_Destroy(GIO_ASYNCSEQ_HANDLE async_seq_handle);
```

> If there are pending I/O operations in progress while this API is called no attempt is made to cancel/complete them.

**SRS_GIO_ASYNCSEQ_13_003: [** `GIO_Async_Seq_Destroy` shall do nothing if `async_seq_handle` is `NULL`. **]**

**SRS_GIO_ASYNCSEQ_13_004: [** `GIO_Async_Seq_Destroy` shall free all resources associated with the handle. **]**

## GIO_Async_Seq_GetContext
```c
gpointer GIO_Async_Seq_GetContext(GIO_ASYNCSEQ_HANDLE async_seq_handle);
```

**SRS_GIO_ASYNCSEQ_13_006: [** `GIO_Async_Seq_GetContext` shall return `NULL` if `async_seq_handle` is `NULL`. **]**

**SRS_GIO_ASYNCSEQ_13_007: [** `GIO_Async_Seq_GetContext` shall return the value of the `async_seq_context` parameter that was passed in when calling `GIO_Async_Seq_Create`. **]**

## GIO_Async_Seq_Add

```c
extern GIO_ASYNCSEQ_RESULT GIO_Async_Seq_Add(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    gpointer callback_context,
    ...
);
```

**SRS_GIO_ASYNCSEQ_13_008: [** `GIO_Async_Seq_Add` shall return `GIO_ASYNCSEQ_ERROR` if `async_seq_handle` is `NULL`. **]**

**SRS_GIO_ASYNCSEQ_13_009: [** `GIO_Async_Seq_Add` shall return `GIO_ASYNCSEQ_ERROR` if the async sequence's state is not equal to `GIO_ASYNCSEQ_STATE_PENDING`. **]**

**SRS_GIO_ASYNCSEQ_13_010: [** `GIO_Async_Seq_Add` shall *append* the new async operations to the end of the existing list of async operations if any. **]**

**SRS_GIO_ASYNCSEQ_13_011: [** `GIO_Async_Seq_Add` shall add callbacks from the variable arguments list till a callback whose value is `NULL` is encountered. **]**

**SRS_GIO_ASYNCSEQ_13_012: [** When a `GIO_ASYNCSEQ_CALLBACK` is encountered in the varargs, the next argument MUST be non-`NULL`. **]**

**SRS_GIO_ASYNCSEQ_13_013: [** `GIO_Async_Seq_Add` shall return `GIO_ASYNCSEQ_ERROR` when any of the underlying platform calls fail. **]**

**SRS_GIO_ASYNCSEQ_13_014: [** The `callback_context` pointer value shall be saved so that they can be passed to the callbacks later when they are invoked. **]**

**SRS_GIO_ASYNCSEQ_13_015: [** The list of callbacks that had been added to the sequence before calling this API will remain unchanged after the function returns when an error occurs. **]**

**SRS_GIO_ASYNCSEQ_13_016: [** `GIO_Async_Seq_Add` shall return `GIO_ASYNCSEQ_OK` if  the API executes successfully. **]**

## GIO_Async_Seq_Addv

```c
extern GIO_ASYNCSEQ_RESULT GIO_Async_Seq_Addv(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    gpointer callback_context,
    va_list args_list
);
```

**SRS_GIO_ASYNCSEQ_13_035: [** `GIO_Async_Seq_Addv` shall return `GIO_ASYNCSEQ_ERROR` if `async_seq_handle` is `NULL`. **]**

**SRS_GIO_ASYNCSEQ_13_036: [** `GIO_Async_Seq_Addv` shall return `GIO_ASYNCSEQ_ERROR` if the async sequence's state is not equal to `GIO_ASYNCSEQ_STATE_PENDING`. **]**

**SRS_GIO_ASYNCSEQ_13_037: [** `GIO_Async_Seq_Addv` shall *append* the new async operations to the end of the existing list of async operations if any. **]**

**SRS_GIO_ASYNCSEQ_13_038: [** `GIO_Async_Seq_Addv` shall add callbacks from the variable arguments list till a callback whose value is `NULL` is encountered. **]**

**SRS_GIO_ASYNCSEQ_13_039: [** When a `GIO_ASYNCSEQ_CALLBACK` is encountered in the varargs, the next argument MUST be non-`NULL`. **]**

**SRS_GIO_ASYNCSEQ_13_040: [** `GIO_Async_Seq_Addv` shall return `GIO_ASYNCSEQ_ERROR` when any of the underlying platform calls fail. **]**

**SRS_GIO_ASYNCSEQ_13_041: [** The `callback_context` pointer value shall be saved so that they can be passed to the callbacks later when they are invoked. **]**

**SRS_GIO_ASYNCSEQ_13_042: [** The list of callbacks that had been added to the sequence before calling this API will remain unchanged after the function returns when an error occurs. **]**

**SRS_GIO_ASYNCSEQ_13_043: [** `GIO_Async_Seq_Addv` shall return `GIO_ASYNCSEQ_OK` if  the API executes successfully. **]**

## GIO_Async_Seq_Run_Async
```c
GIO_ASYNCSEQ_RESULT GIO_Async_Seq_Run_Async(GIO_ASYNCSEQ_HANDLE async_seq_handle);
```

**SRS_GIO_ASYNCSEQ_13_017: [** `GIO_Async_Seq_Run_Async` shall return `GIO_ASYNCSEQ_ERROR` if `async_seq_handle` is `NULL`. **]**

**SRS_GIO_ASYNCSEQ_13_018: [** `GIO_Async_Seq_Run_Async` shall return `GIO_ASYNCSEQ_ERROR` if the sequence's state is already `GIO_ASYNCSEQ_STATE_RUNNING`. **]**

**SRS_GIO_ASYNCSEQ_13_019: [** `GIO_Async_Seq_Run_Async` shall complete the sequence and invoke the sequence's complete callback if there are no asynchronous operations to process. **]**

**SRS_GIO_ASYNCSEQ_13_020: [** `GIO_Async_Seq_Run_Async` shall invoke the '*start*' callback of the first async operation in the sequence. **]**

**SRS_GIO_ASYNCSEQ_13_021: [** `GIO_Async_Seq_Run_Async` shall pass the callback context that was supplied to `GIO_Async_Seq_Add` or `GIO_Async_Seq_Addv` when the first operation was added to the sequence when invoking the '*start*' callback. **]**

**SRS_GIO_ASYNCSEQ_13_031: [** `GIO_Async_Seq_Run_Async` shall supply `NULL` as the value of the `previous_result` parameter of the '*start*' callback. **]**

**SRS_GIO_ASYNCSEQ_13_022: [** `GIO_Async_Seq_Run_Async` shall return `GIO_ASYNCSEQ_OK` when there are no errors. **]**

**SRS_GIO_ASYNCSEQ_13_023: [** `GIO_Async_Seq_Run_Async` shall supply a non-`NULL` pointer to a function as the value of the `async_callback` parameter when calling the '*start*' callback. **]**

## resolve_callback
```c
static void resolve_callback(
    GObject* source_object,
    GAsyncResult* result,
    gpointer user_data
);
```

**SRS_GIO_ASYNCSEQ_13_024: [** `resolve_callback` shall do nothing else if `user_data` is `NULL`. **]**

**SRS_GIO_ASYNCSEQ_13_025: [** `resolve_callback` shall invoke the sequence's error callback and suspend execution of the sequence if the state of the sequence is not equal to `GIO_ASYNCSEQ_STATE_RUNNING`. **]**

**SRS_GIO_ASYNCSEQ_13_026: [** `resolve_callback` shall invoke the '*finish*' callback of the async operation that was just concluded. **]**

**SRS_GIO_ASYNCSEQ_13_027: [** `resolve_callback` shall invoke the sequence's error callback and suspend execution of the sequence if the '*finish*' callback returns a non-`NULL` `GError` pointer. **]**

**SRS_GIO_ASYNCSEQ_13_028: [** `resolve_callback` shall free the `GError` object by calling `g_clear_error` if the '*finish*' callback returns a non-`NULL` `GError` pointer. **]**

**SRS_GIO_ASYNCSEQ_13_029: [** `resolve_callback` shall invoke the '*start*' callback of the next async operation in the sequence. **]**

**SRS_GIO_ASYNCSEQ_13_030: [** `resolve_callback` shall supply the result of calling the '*finish*' callback of the current async operation as the value of the `previous_result` parameter of the '*start*' callback of the next async operation. **]**

**SRS_GIO_ASYNCSEQ_13_032: [** `resolve_callback` shall pass the callback context that was supplied to `GIO_Async_Seq_Add` when the next async operation was added to the sequence when invoking the '*start*' callback. **]**

**SRS_GIO_ASYNCSEQ_13_033: [** `resolve_callback` shall supply a non-`NULL` pointer to a function as the value of the `async_callback` parameter when calling the '*start*' callback. **]**

**SRS_GIO_ASYNCSEQ_13_034: [** `resolve_callback` shall complete the sequence and invoke the sequence's complete callback if there are no more asynchronous operations to process. **]**