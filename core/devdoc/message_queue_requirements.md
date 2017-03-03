MESSAGE QUEUE REQUIREMENTS
==========================

Overview
--------

The message queue is a very simple queue intended to manage messages. The message queue is typed with MESSAGE_HANDLE because the destruction of the queue requires the destruction of the messages inside the queue.  **Unless the queue is destroyed, the user of this queue is expected to clone before pushing onto the queue, and is expected to destroy the message after popping the message off teh queue.**

References
----------

[Message requirements](message_requirements.md)

Exposed API
-----------

```c
/* creation */
MESSAGE_QUEUE_HANDLE MESSAGE_QUEUE_create());
/* destruction */
void MESSAGE_QUEUE_destroy(MESSAGE_QUEUE_HANDLE handle);

/* insertion */
int MESSAGE_QUEUE_push(MESSAGE_QUEUE_HANDLE handle, MESSAGE_HANDLE element);

/* removal */
MESSAGE_HANDLE MESSAGE_QUEUE_pop(MESSAGE_QUEUE_HANDLE handle);

/* access */
bool  MESSAGE_QUEUE_is_empty(MESSAGE_QUEUE_HANDLE handle);
MESSAGE_HANDLE MESSAGE_QUEUE_front(MESSAGE_QUEUE_HANDLE handle);
```

MESSAGE\_QUEUE\_create
----------------------
```c
MESSAGE_QUEUE_HANDLE MESSAGE_QUEUE_create());
```

Create an empty message queue.

**SRS_MESSAGE_QUEUE_17_001: [** On a successful call, MESSAGE\_QUEUE\_create shall return a non-`NULL` value in `MESSAGE_QUEUE_HANDLE`. **]**

**SRS_MESSAGE_QUEUE_17_002: [** A newly created message queue shall be empty. **]**

**SRS_MESSAGE_QUEUE_17_003: [** On a failure, MESSAGE\_QUEUE\_create shall return `NULL`. **]**


MESSAGE\_QUEUE\_destroy
----------------------
```c
void MESSAGE_QUEUE_destroy(MESSAGE_QUEUE_HANDLE handle);
```

Destroys a message queue.

**SRS_MESSAGE_QUEUE_17_004: [** MESSAGE\_QUEUE\_destroy shall not perform any actions on a `NULL` message queue. **]**

**SRS_MESSAGE_QUEUE_17_005: [** If the message queue is not empty, MESSAGE\_QUEUE\_destroy shall destroy all messages in the queue. **]**

**SRS_MESSAGE_QUEUE_17_006: [** MESSAGE\_QUEUE\_destroy shall free all allocated resources. **]**


MESSAGE\_QUEUE\_push
----------------------
```c
int MESSAGE_QUEUE_push(MESSAGE_QUEUE_HANDLE handle, MESSAGE_HANDLE element);
```

Inserts a mesage handle into the message queue.

**SRS_MESSAGE_QUEUE_17_007: [** MESSAGE\_QUEUE\_push shall return a non-zero value if `handle` or `element` are `NULL`. **]**

**SRS_MESSAGE_QUEUE_17_008: [** MESSAGE\_QUEUE\_push shall return zero on success. **]**

**SRS_MESSAGE_QUEUE_17_009: [** MESSAGE\_QUEUE\_push shall return a non-zero value if any system call fails. **]**

**SRS_MESSAGE_QUEUE_17_010: [** A successful call to MESSAGE\_QUEUE\_push on an empty queue will cause the message queue to be non-empty (MESSAGE\_QUEUE\_is\_empty shall return false). **]**

**SRS_MESSAGE_QUEUE_17_011: [** Messages shall be pushed into the queue in a first-in-first-out order. **]**


MESSAGE\_QUEUE\_pop
----------------------
```c
MESSAGE_HANDLE MESSAGE_QUEUE_pop(MESSAGE_QUEUE_HANDLE handle);
```

Removes the next available message from the message queue.

**SRS_MESSAGE_QUEUE_17_012: [** MESSAGE\_QUEUE\_pop shall return `NULL` on a `NULL` message queue. **]**

**SRS_MESSAGE_QUEUE_17_013: [** MESSAGE\_QUEUE\_pop shall return `NULL` on an empty message queue. **]**

**SRS_MESSAGE_QUEUE_17_014: [** MESSAGE\_QUEUE\_pop shall remove messages from the queue in a first-in-first-out order. **]**

**SRS_MESSAGE_QUEUE_17_015: [** A successful call to MESSAGE\_QUEUE\_pop on a queue with one message will cause the message queue to be empty. **]**


MESSAGE\_QUEUE\_is\_empty
----------------------
```c
bool  MESSAGE_QUEUE_is_empty(MESSAGE_QUEUE_HANDLE handle);
```

A check to see if the message queue is empty.

**SRS_MESSAGE_QUEUE_17_016: [** MESSAGE\_QUEUE\_is\_empty shall return true if `handle` is `NULL`. **]**

**SRS_MESSAGE_QUEUE_17_017: [** MESSAGE\_QUEUE\_is\_empty shall return true if there are no messages on the queue. **]**

**SRS_MESSAGE_QUEUE_17_018: [** MESSAGE\_QUEUE\_is\_empty shall return false if one or more messages have been pushed on the queue. **]**

MESSAGE\_QUEUE\_front
----------------------
```c
MESSAGE_HANDLE MESSAGE_QUEUE_front(MESSAGE_QUEUE_HANDLE handle);
```

Returns the item at the front of the queue without altering the queue.

**SRS_MESSAGE_QUEUE_17_019: [** MESSAGE\_QUEUE\_front shall return `NULL` if `handle` is `NULL`. **]**

**SRS_MESSAGE_QUEUE_17_020: [** MESSAGE\_QUEUE\_front shall return `NULL` if the message queue is empty. **]**

**SRS_MESSAGE_QUEUE_17_021: [** On a non-empty queue, MESSAGE\_QUEUE\_front shall return the first remaining element that was pushed onto the message queue. **]**

**SRS_MESSAGE_QUEUE_17_022: [** The content of the message queue shall not be changed after calling MESSAGE\_QUEUE\_front. **]**
