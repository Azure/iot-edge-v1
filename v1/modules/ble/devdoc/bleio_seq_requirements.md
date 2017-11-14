# Bluetooth Low Energy GATT I/O Request Sequencer

## Overview

The Bluetooth Low Energy (BLE) GATT I/O Request Sequencer component is an abstraction that is designed to make the job of executing a sequence of BLE I/O operations simpler. It defines the notion of an *instruction* which represents a read or a write operation that needs to be executed on the BLE device potentially at some well defined interval.

## References

* [BLE GATT I/O Requirements](./ble_gatt_io_requirements.md)

## Data types

```c
typedef struct BLEIO_SEQ_HANDLE_DATA_TAG* BLEIO_SEQ_HANDLE;

#define BLEIO_SEQ_RESULT_VALUES \
    BLEIO_SEQ_ERROR, \
    BLEIO_SEQ_OK
DEFINE_ENUM(BLEIO_SEQ_RESULT, BLEIO_SEQ_RESULT_VALUES);

#define BLEIO_SEQ_INSTRUCTION_TYPE_VALUES \
    READ_ONCE, \
    READ_PERIODIC, \
    WRITE_ONCE, \
    WRITE_AT_INIT, \
    WRITE_AT_EXIT
DEFINE_ENUM(BLEIO_SEQ_INSTRUCTION_TYPE, BLEIO_SEQ_INSTRUCTION_TYPE_VALUES);

typedef struct BLEIO_SEQ_INSTRUCTION_TAG
{
    /**
     * The type of instruction this is from the BLEIO_SEQ_INSTRUCTION_TYPE enum.
     */
    BLEIO_SEQ_INSTRUCTION_TYPE  instruction_type;

    /**
     * The GATT characteristic to read from/write to.
     */
    const char*                 characteristic_uuid;

    /**
     * Context data that should be passed back to the callback that is invoked
     * when this instruction completes execution (or for every instance of
     * completion in case this is a recurring instruction).
     */
    void*                       context;

    union
    {
        /**
         * If 'instruction_type' is equal to READ_PERIODIC then this
         * value indicates the polling interval in milliseconds.
         */
        uint32_t                interval_in_ms;

        /**
         * If 'instruction_type' is equal to WRITE_AT_INIT or WRITE_AT_EXIT
         *  or WRITE_ONCE then this is the buffer that is to be written.
         */
        BUFFER_HANDLE          buffer;
    }data;
}BLEIO_SEQ_INSTRUCTION;

/**
 * Callback invoked when the sequencer completes a read operation.
 */
typedef void(*ON_BLEIO_SEQ_READ_COMPLETE)(
    BLEIO_SEQ_HANDLE bleio_seq_handle,
    void* context,
    const char* characteristic_uuid,
    BLEIO_SEQ_INSTRUCTION_TYPE type,
    BLEIO_SEQ_RESULT result,
    BUFFER_HANDLE data
);

/**
 * Callback invoked when the sequencer completes a write operation.
 */
typedef void(*ON_BLEIO_SEQ_WRITE_COMPLETE)(
    BLEIO_SEQ_HANDLE bleio_seq_handle,
    void* context,
    const char* characteristic_uuid,
    BLEIO_SEQ_INSTRUCTION_TYPE type,
    BLEIO_SEQ_RESULT result
);

/**
* Callback invoked when the sequence has been destroyed.
*/
typedef void(*ON_BLEIO_SEQ_DESTROY_COMPLETE)(BLEIO_SEQ_HANDLE bleio_seq_handle, void* context);

extern BLEIO_SEQ_HANDLE BLEIO_Seq_Create(
    BLEIO_GATT_HANDLE bleio_gatt_handle,
    VECTOR_HANDLE instructions,
    ON_BLEIO_SEQ_READ_COMPLETE on_read_complete,
    ON_BLEIO_SEQ_WRITE_COMPLETE on_write_complete
);

extern void BLEIO_Seq_Destroy(
    BLEIO_SEQ_HANDLE bleio_seq_handle,
    ON_BLEIO_SEQ_DESTROY_COMPLETE on_destroy_complete,
    void* context
);

extern BLEIO_SEQ_RESULT BLEIO_Seq_Run(BLEIO_SEQ_HANDLE bleio_seq_handle);

extern BLEIO_SEQ_RESULT BLEIO_Seq_AddInstruction(
    BLEIO_SEQ_HANDLE bleio_seq_handle,
    BLEIO_SEQ_INSTRUCTION* instruction
);
```

## BLEIO_Seq_Create
```c
extern BLEIO_SEQ_HANDLE BLEIO_Seq_Create(
    BLEIO_GATT_HANDLE bleio_gatt_handle,
    VECTOR_HANDLE instructions,
    ON_BLEIO_SEQ_READ_COMPLETE on_read_complete,
    ON_BLEIO_SEQ_WRITE_COMPLETE on_write_complete
);
```

**SRS_BLEIO_SEQ_13_001: [** `BLEIO_Seq_Create` shall return `NULL` if `bleio_gatt_handle` is `NULL`. **]**

**SRS_BLEIO_SEQ_13_002: [** `BLEIO_Seq_Create` shall return `NULL` if `instructions` is `NULL`. **]**

**SRS_BLEIO_SEQ_13_003: [** `BLEIO_Seq_Create` shall return `NULL` if the vector `instructions` is empty. **]**

**SRS_BLEIO_SEQ_13_004: [** `BLEIO_Seq_Create` shall return `NULL` if any of the underlying platform calls fail. **]**

**SRS_BLEIO_SEQ_13_005: [** `BLEIO_Seq_Create` shall return a non-`NULL` handle on successful execution. **]**

**SRS_BLEIO_SEQ_13_023: [** `BLEIO_Seq_Create` shall return `NULL` if a `READ_PERIODIC` instruction's `interval_in_ms` field is zero. **]**

**SRS_BLEIO_SEQ_13_024: [** `BLEIO_Seq_Create` shall return `NULL` if a `WRITE_AT_INIT` or a `WRITE_AT_EXIT` or a `WRITE_ONCE` instruction has a `NULL` value in the `buffer` field. **]**

**SRS_BLEIO_SEQ_13_025: [** `BLEIO_Seq_Create` shall return `NULL` if the `characteristic_uuid` field for any instruction is `NULL` or empty. **]**

**SRS_BLEIO_SEQ_13_028: [** On Windows, this function shall return `NULL`. **]**

## BLEIO_Seq_Destroy
```c
extern void BLEIO_Seq_Destroy(
    BLEIO_SEQ_HANDLE bleio_seq_handle,
    ON_BLEIO_SEQ_DESTROY_COMPLETE on_destroy_complete,
    void* context
);
```

**SRS_BLEIO_SEQ_13_007: [** If `bleio_seq_handle` is `NULL` then `BLEIO_Seq_Destroy` shall do nothing. **]**

**SRS_BLEIO_SEQ_13_006: [** `BLEIO_Seq_Destroy` shall free all resources associated with the handle once all the pending I/O operations are complete. **]**

**SRS_BLEIO_SEQ_13_011: [** `BLEIO_Seq_Destroy` shall schedule the execution of all `WRITE_AT_EXIT` instructions.
> **Note:** *This is a 'best effort' write attempt. If this write operation fails then that fact is logged and no further action is taken.* **]**

**SRS_BLEIO_SEQ_13_021: [** When the `WRITE_AT_EXIT` instruction completes execution this API shall invoke the `on_write_complete` callback passing in the status of the operation and the callback context that was passed in via the `BLEIO_SEQ_INSTRUCTION` structure. **]**

**SRS_BLEIO_SEQ_13_027: [** When the `WRITE_AT_EXIT` instruction completes execution this API shall free the buffer that was passed in via the instruction. **]**

**SRS_BLEIO_SEQ_13_009: [** If there are active instructions of type `READ_PERIODIC` in progress then the timers associated with those instructions shall be cancelled. **]**

**SRS_BLEIO_SEQ_13_029: [** On Windows, this function shall do nothing. **]**

**SRS_BLEIO_SEQ_13_031: [** If `on_destroy_complete` is not `NULL` then `BLEIO_Seq_Destroy` shall invoke `on_destroy_complete` once all `WRITE_AT_EXIT` instructions have been executed. **]**

**SRS_BLEIO_SEQ_13_032: [** If `on_destroy_complete` is not `NULL` then `BLEIO_Seq_Destroy` shall pass `context` as-is to `on_destroy_complete`. **]**

## BLEIO_Seq_Run
```c
extern BLEIO_SEQ_RESULT BLEIO_Seq_Run(BLEIO_SEQ_HANDLE bleio_seq_handle);
```

**SRS_BLEIO_SEQ_13_010: [** `BLEIO_Seq_Run` shall return `BLEIO_SEQ_ERROR` if `bleio_seq_handle` is `NULL`. **]**

**SRS_BLEIO_SEQ_13_013: [** `BLEIO_Seq_Run` shall return `BLEIO_SEQ_ERROR` if `BLEIO_Seq_Run` was previously called on this handle. **]**

**SRS_BLEIO_SEQ_13_014: [** `BLEIO_Seq_Run` shall return `BLEIO_SEQ_ERROR` if an underlying platform call fails. **]**

**SRS_BLEIO_SEQ_13_016: [** `BLEIO_Seq_Run` shall schedule execution of all `WRITE_AT_INIT` instructions. **]**

**SRS_BLEIO_SEQ_13_015: [** `BLEIO_Seq_Run` shall schedule execution of all `READ_ONCE` instructions. **]**

**SRS_BLEIO_SEQ_13_033: [** `BLEIO_Seq_Run` shall schedule execution of all `WRITE_ONCE` instructions. **]**

**SRS_BLEIO_SEQ_13_017: [** `BLEIO_Seq_Run` shall create timers at the specified intervals for scheduling execution of all `READ_PERIODIC` instructions. **]**

**SRS_BLEIO_SEQ_13_018: [** When a `READ_ONCE` or a `READ_PERIODIC` instruction completes execution this API shall invoke the `on_read_complete` callback passing in the data that was read along with the status of the operation and the callback context that was passed in via the `BLEIO_SEQ_INSTRUCTION` structure. **]**

**SRS_BLEIO_SEQ_13_020: [** When the `WRITE_AT_INIT` instruction completes execution this API shall invoke the `on_write_complete` callback passing in the status of the operation and the callback context that was passed in via the `BLEIO_SEQ_INSTRUCTION` structure. **]**

**SRS_BLEIO_SEQ_13_026: [** When the `WRITE_AT_INIT` instruction completes execution this API shall free the buffer that was passed in via the instruction. **]**

**SRS_BLEIO_SEQ_13_034: [** When the `WRITE_ONCE` instruction completes execution this API shall invoke the `on_write_complete` callback passing in the status of the operation and the callback context that was passed in via the `BLEIO_SEQ_INSTRUCTION` structure. **]**

**SRS_BLEIO_SEQ_13_035: [** When the `WRITE_ONCE` instruction completes execution this API shall free the buffer that was passed in via the instruction. **]**

**SRS_BLEIO_SEQ_13_030: [** On Windows this function shall return `BLEIO_SEQ_ERROR`. **]**

## BLEIO_Seq_AddInstruction
```c
extern BLEIO_SEQ_RESULT BLEIO_Seq_AddInstruction(
    BLEIO_SEQ_HANDLE bleio_seq_handle,
    BLEIO_SEQ_INSTRUCTION* instruction
);
```

**SRS_BLEIO_SEQ_13_036: [** `BLEIO_Seq_AddInstruction` shall return `BLEIO_SEQ_ERROR` if `bleio_seq_handle` is `NULL`. **]**

**SRS_BLEIO_SEQ_13_046: [** `BLEIO_Seq_AddInstruction` shall return `BLEIO_SEQ_ERROR` if `instruction` is `NULL`. **]**

**SRS_BLEIO_SEQ_13_047: [** `BLEIO_Seq_AddInstruction` shall return `BLEIO_SEQ_ERROR` if a `READ_PERIODIC` instruction's `interval_in_ms` field is zero. **]**

**SRS_BLEIO_SEQ_13_048: [** `BLEIO_Seq_AddInstruction` shall return `BLEIO_SEQ_ERROR` if a `WRITE_AT_INIT` or a `WRITE_AT_EXIT` or a `WRITE_ONCE` instruction has a `NULL` value in the `buffer` field. **]**

**SRS_BLEIO_SEQ_13_049: [** `BLEIO_Seq_AddInstruction` shall return `BLEIO_SEQ_ERROR` if the `characteristic_uuid` field for the instruction is `NULL` or empty. **]**

**SRS_BLEIO_SEQ_13_045: [** `BLEIO_Seq_AddInstruction` shall return `BLEIO_SEQ_ERROR` if `BLEIO_Seq_Run` was *NOT* called first. **]**

**SRS_BLEIO_SEQ_13_037: [** `BLEIO_Seq_AddInstruction` shall return `BLEIO_SEQ_ERROR` if an underlying platform call fails. **]**

**SRS_BLEIO_SEQ_13_038: [** `BLEIO_Seq_AddInstruction` shall schedule execution of the instruction. **]**

**SRS_BLEIO_SEQ_13_039: [** `BLEIO_Seq_AddInstruction` shall create a timer at the specified interval if the instruction is a `READ_PERIODIC` instruction. **]**

**SRS_BLEIO_SEQ_13_040: [** When a `READ_ONCE` or a `READ_PERIODIC` instruction completes execution this API shall invoke the `on_read_complete` callback passing in the data that was read along with the status of the operation and the callback context that was passed in via the `BLEIO_SEQ_INSTRUCTION` structure. **]**

**SRS_BLEIO_SEQ_13_041: [** When a `WRITE_AT_INIT` or a `WRITE_ONCE` instruction completes execution this API shall free the buffer that was passed in via the instruction. **]**

**SRS_BLEIO_SEQ_13_042: [** When a `WRITE_ONCE` or a `WRITE_AT_INIT` instruction completes execution this API shall invoke the `on_write_complete` callback passing in the status of the operation and the callback context that was passed in via the `BLEIO_SEQ_INSTRUCTION` structure. **]**

**SRS_BLEIO_SEQ_13_044: [** On Windows this function shall return `BLEIO_SEQ_ERROR`. **]**