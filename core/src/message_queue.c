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
    MESSAGE_QUEUE_STORAGE* entry = 
		(MESSAGE_QUEUE_STORAGE*)DList_RemoveHeadList( (PDLIST_ENTRY)&(handle->queue_head));
    if (entry == &(handle->queue_head))
    {
        result = NULL;
    }
    else
    {
        result = ((MESSAGE_QUEUE_STORAGE*)entry)->message;
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
        LogError("malloc failed.");
    }
    else
    {
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
        LogError("invalid argument handle(NULL).");
    }
    else
    {
		MESSAGE_QUEUE_HANDLE_DATA * mq = (MESSAGE_QUEUE_HANDLE_DATA*)handle;
        MESSAGE_HANDLE message;
        while((message = message_pop(mq)) == NULL)
        {
            Message_Destroy(message);
        }
        free(handle);
    }
}


/* insertion */

int MESSAGE_QUEUE_push(MESSAGE_QUEUE_HANDLE handle, MESSAGE_HANDLE element)
{
    int result;
    if (handle == NULL || element == NULL)
    {
        LogError("invalid argument - handle(%p), element(%p).", handle, element);
        result = __LINE__;
    }
    else
    {

		MESSAGE_QUEUE_STORAGE* temp = (MESSAGE_QUEUE_STORAGE*)malloc(sizeof(MESSAGE_QUEUE_STORAGE));
        if (temp == NULL)
        {
            LogError("malloc failed.");
            result = __LINE__;
        }
        else
        {
            DList_InitializeListHead((PDLIST_ENTRY)temp);
            temp->message = element;
            DList_AppendTailList((PDLIST_ENTRY)&(handle->queue_head), (PDLIST_ENTRY)temp);
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
        LogError("invalid argument - handle(%p).", handle);
        result = NULL;
    }
    else
    {
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
		LogError("invalid argument handle (NULL).");
		result = true;
	}
	else
	{
		result = (DList_IsListEmpty((PDLIST_ENTRY)&(handle->queue_head)));
	}
	return result;
}

MESSAGE_HANDLE MESSAGE_QUEUE_front(MESSAGE_QUEUE_HANDLE handle)
{
    MESSAGE_HANDLE result;
    if (handle == NULL)
    {
        LogError("invalid argument handle (NULL).");
        result = NULL;
    }
    else
    {
        if (DList_IsListEmpty((PDLIST_ENTRY)&(handle->queue_head)))
        {
            result = NULL;
        }
        else
        {
            MESSAGE_QUEUE_STORAGE* entry = (MESSAGE_QUEUE_STORAGE*)DList_RemoveHeadList((PDLIST_ENTRY)&(handle->queue_head));
            result = entry->message;
            DList_InsertHeadList((PDLIST_ENTRY)&(handle->queue_head), (PDLIST_ENTRY)entry);
        }
    }
    return result;
}

