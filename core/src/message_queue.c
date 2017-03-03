// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/xlogging.h"

#include "azure_c_shared_utility/doublylinkedlist.h"
#include "message.h"
#include "message_queue.h"

typedef struct MESSAGE_QUEUE_STORAGE_TAG
{
    DLIST_ENTRY queue_entry;
    MESSAGE_HANDLE message;
} MESSAGE_QUEUE_STORAGE;

typedef struct MESSAGE_QUEUE_TAG
{
    MESSAGE_QUEUE_STORAGE queue_head;
} MESSAGE_QUEUE_HANDLE_DATA;

static MESSAGE_HANDLE message_pop(MESSAGE_QUEUE_HANDLE_DATA* handle)
{
    MESSAGE_HANDLE result;
	if (DList_IsListEmpty((PDLIST_ENTRY)&(handle->queue_head)))
	{
        /*Codes_SRS_MESSAGE_QUEUE_17_013: [ MESSAGE_QUEUE_pop shall return NULL on an empty message queue. ]*/
		result = NULL;
	}
	else
	{
        /*Codes_SRS_MESSAGE_QUEUE_17_014: [ MESSAGE_QUEUE_pop shall remove messages from the queue in a first-in-first-out order. ]*/
        /*Codes_SRS_MESSAGE_QUEUE_17_015: [ A successful call to MESSAGE_QUEUE_pop on a queue with one message will cause the message queue to be empty. ]*/
		MESSAGE_QUEUE_STORAGE* entry =
		(MESSAGE_QUEUE_STORAGE*)DList_RemoveHeadList( (PDLIST_ENTRY)&(handle->queue_head));

        result = ((MESSAGE_QUEUE_STORAGE*)entry)->message;
        /*Codes_SRS_MESSAGE_QUEUE_17_006: [ MESSAGE_QUEUE_destroy shall free all allocated resources. ]*/
        free(entry);
    }
    return result;
}

MESSAGE_QUEUE_HANDLE MESSAGE_QUEUE_create()
{
	MESSAGE_QUEUE_HANDLE_DATA* result;

    result = (MESSAGE_QUEUE_HANDLE_DATA*)malloc(sizeof(MESSAGE_QUEUE_HANDLE_DATA));
    if (result == NULL)
    {
        /*Codes_SRS_MESSAGE_QUEUE_17_003: [ On a failure, MESSAGE_QUEUE_create shall return NULL. ]*/
        LogError("malloc failed.");
    }
    else
    {
        /*Codes_SRS_MESSAGE_QUEUE_17_001: [ On a successful call, MESSAGE_QUEUE_create shall return a non-NULL value in MESSAGE_QUEUE_HANDLE. ]*/
        /*Codes_SRS_MESSAGE_QUEUE_17_002: [ A newly created message queue shall be empty. ]*/
        DList_InitializeListHead((PDLIST_ENTRY)&(result->queue_head));
        result->queue_head.message = NULL;
    }
    return result;
}

void MESSAGE_QUEUE_destroy(MESSAGE_QUEUE_HANDLE handle)
{
    /* Codes_SRS_MESSAGE_QUEUE_10_009: [MESSAGE_QUEUE_destroy shall return if the given handle is NULL.] */
    if (handle == NULL)
    {
        /*Codes_SRS_MESSAGE_QUEUE_17_004: [ MESSAGE_QUEUE_destroy shall not perform any actions on a NULL message queue. ]*/
        LogError("invalid argument handle(NULL).");
    }
    else
    {
		MESSAGE_QUEUE_HANDLE_DATA * mq = (MESSAGE_QUEUE_HANDLE_DATA*)handle;
        MESSAGE_HANDLE message;
        while((message = message_pop(mq)) != NULL)
        {
            /*Codes_SRS_MESSAGE_QUEUE_17_005: [ If the message queue is not empty, MESSAGE_QUEUE_destroy shall destroy all messages in the queue. ]*/
            Message_Destroy(message);
        }
        /*Codes_SRS_MESSAGE_QUEUE_17_006: [ MESSAGE_QUEUE_destroy shall free all allocated resources. ]*/
        free(handle);
    }
}


/* insertion */

int MESSAGE_QUEUE_push(MESSAGE_QUEUE_HANDLE handle, MESSAGE_HANDLE element)
{
    int result;
    if (handle == NULL || element == NULL)
    {
        /*Codes_SRS_MESSAGE_QUEUE_17_007: [ MESSAGE_QUEUE_push shall return a non-zero value if handle or element are NULL. ]*/
        LogError("invalid argument - handle(%p), element(%p).", handle, element);
        result = __LINE__;
    }
    else
    {
		MESSAGE_QUEUE_STORAGE* temp = (MESSAGE_QUEUE_STORAGE*)malloc(sizeof(MESSAGE_QUEUE_STORAGE));
        if (temp == NULL)
        {
            /*Codes_SRS_MESSAGE_QUEUE_17_009: [ MESSAGE_QUEUE_push shall return a non-zero value if any system call fails. ]*/
            LogError("malloc failed.");
            result = __LINE__;
        }
        else
        {
            DList_InitializeListHead((PDLIST_ENTRY)temp);
            temp->message = element;
            /*Codes_SRS_MESSAGE_QUEUE_17_010: [ A successful call to MESSAGE_QUEUE_push on an empty queue will cause the message queue to be non-empty (MESSAGE_QUEUE_is_empty shall return false). ]*/
            /*Codes_SRS_MESSAGE_QUEUE_17_011: [ Messages shall be pushed into the queue in a first-in-first-out order. ]*/
            DList_AppendTailList((PDLIST_ENTRY)&(handle->queue_head), (PDLIST_ENTRY)temp);
            /*Codes_SRS_MESSAGE_QUEUE_17_008: [ MESSAGE_QUEUE_push shall return zero on success. ]*/
            result = 0;
        }
    }
    return result;
}

/* removal */

MESSAGE_HANDLE MESSAGE_QUEUE_pop(MESSAGE_QUEUE_HANDLE handle)
{
    MESSAGE_HANDLE result;
    if (handle == NULL)
    {
        /*Codes_SRS_MESSAGE_QUEUE_17_012: [ MESSAGE_QUEUE_pop shall return NULL on a NULL message queue. ]*/
        LogError("invalid argument - handle(%p).", handle);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_MESSAGE_QUEUE_17_013: [ MESSAGE_QUEUE_pop shall return NULL on an empty message queue. ]*/
        /*Codes_SRS_MESSAGE_QUEUE_17_014: [ MESSAGE_QUEUE_pop shall remove messages from the queue in a first-in-first-out order. ]*/
        /*Codes_SRS_MESSAGE_QUEUE_17_015: [ A successful call to MESSAGE_QUEUE_pop on a queue with one message will cause the message queue to be empty. ]*/
        result = message_pop(handle);
    }
    return result;
}

/* access */
bool MESSAGE_QUEUE_is_empty(MESSAGE_QUEUE_HANDLE handle)
{
	bool result;
	if (handle == NULL)
	{
        /*Codes_SRS_MESSAGE_QUEUE_17_016: [ MESSAGE_QUEUE_is_empty shall return true if handle is NULL. ]*/
		LogError("invalid argument handle (NULL).");
		result = true;
	}
	else
	{
        /*Codes_SRS_MESSAGE_QUEUE_17_017: [ MESSAGE_QUEUE_is_empty shall return true if there are no messages on the queue. ]*/
        /*Codes_SRS_MESSAGE_QUEUE_17_018: [ MESSAGE_QUEUE_is_empty shall return false if one or more messages have been pushed on the queue. ]*/
		result = (DList_IsListEmpty((PDLIST_ENTRY)&(handle->queue_head)));
	}
	return result;
}

MESSAGE_HANDLE MESSAGE_QUEUE_front(MESSAGE_QUEUE_HANDLE handle)
{
    MESSAGE_HANDLE result;
    if (handle == NULL)
    {
        /*Codes_SRS_MESSAGE_QUEUE_17_019: [ MESSAGE_QUEUE_front shall return NULL if handle is NULL. ]*/
        LogError("invalid argument handle (NULL).");
        result = NULL;
    }
    else
    {
        /*Codes_SRS_MESSAGE_QUEUE_17_020: [ MESSAGE_QUEUE_front shall return NULL if the message queue is empty. ]*/
        if (DList_IsListEmpty((PDLIST_ENTRY)&(handle->queue_head)))
        {
            result = NULL;
        }
        else
        {
            /*Codes_SRS_MESSAGE_QUEUE_17_021: [ On a non-empty queue, MESSAGE_QUEUE_front shall return the first remaining element that was pushed onto the message queue. ]*/
            
            MESSAGE_QUEUE_STORAGE* entry = (MESSAGE_QUEUE_STORAGE*)DList_RemoveHeadList((PDLIST_ENTRY)&(handle->queue_head));
            result = entry->message;
            /*Codes_SRS_MESSAGE_QUEUE_17_022: [ The content of the message queue shall not be changed after calling MESSAGE_QUEUE_front. ]*/
            DList_InsertHeadList((PDLIST_ENTRY)&(handle->queue_head), (PDLIST_ENTRY)entry);
        }
    }
    return result;
}

