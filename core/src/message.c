// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "azure_c_shared_utility/gballoc.h"

#include <stddef.h>

#include "message.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/map.h"
#include "azure_c_shared_utility/constmap.h"
#include "azure_c_shared_utility/iot_logging.h"

#include "azure_c_shared_utility/refcount.h"

typedef struct MESSAGE_HANDLE_DATA_TAG
{
    CONSTMAP_HANDLE properties;
    CONSTBUFFER_HANDLE content;
}MESSAGE_HANDLE_DATA;

DEFINE_REFCOUNT_TYPE(MESSAGE_HANDLE_DATA);

MESSAGE_HANDLE Message_Create(const MESSAGE_CONFIG * cfg)
{
    MESSAGE_HANDLE_DATA* result;
    /*Codes_SRS_MESSAGE_02_002: [If cfg is NULL then Message_Create shall return NULL.]*/
    if (cfg == NULL)
    {
        result = NULL;
        LogError("invalid parameter (NULL).");
    }
    /*Codes_SRS_MESSAGE_02_003: [If field source of cfg is NULL and size is not zero, then Message_Create shall fail and return NULL.] */
    else if ((cfg->size > 0) && (cfg->source == NULL))
    {
        result = NULL;
        LogError("invalid parameter combination cfg->size=%zd, cfg->source=%p", cfg->size, cfg->source);
    }
    else
    {
        /*Codes_SRS_MESSAGE_02_006: [Otherwise, Message_Create shall return a non-NULL handle and shall set the internal ref count to "1".]*/
        result = REFCOUNT_TYPE_CREATE(MESSAGE_HANDLE_DATA);
        if (result == NULL)
        {
            LogError("malloc returned NULL");
            /*return as is*/
        }
        else
        {
            /*Codes_SRS_MESSAGE_02_004: [Mesages shall be allowed to be created from zero-size content.]*/
			/*Codes_SRS_MESSAGE_02_015: [The MESSAGE_CONTENT's field size shall have the same value as the cfg's field size.]*/
			/*Codes_SRS_MESSAGE_17_003: [Message_Create shall copy the source to a readonly CONSTBUFFER.]*/
			result->content = CONSTBUFFER_Create(cfg->source, cfg->size);
			if (result->content == NULL)
			{
				LogError("CONSBUFFER Create failed");
				free(result);
				result = NULL;
			}
			else
			{
				/*Codes_SRS_MESSAGE_02_019: [Message_Create shall clone the sourceProperties to a readonly CONSTMAP.]*/
				result->properties = ConstMap_Create(cfg->sourceProperties);
				if (result->properties == NULL)
				{
					/*Codes_SRS_MESSAGE_02_005: [If Message_Create encounters an error while building the internal structures of the message, then it shall return NULL.] */
					LogError("ConstMap_Create failed");
					CONSTBUFFER_Destroy(result->content);
					free(result);
					result = NULL;
				}
				else
				{
					/*all is fine, return as is.*/
				}
			}
        }
    }
    return (MESSAGE_HANDLE)result;
}

MESSAGE_HANDLE Message_CreateFromBuffer(const MESSAGE_BUFFER_CONFIG* cfg)
{
	MESSAGE_HANDLE_DATA* result;
	/*Codes_SRS_MESSAGE_17_008: [ If cfg is NULL then Message_CreateFromBuffer shall return NULL.] */
	if (cfg == NULL)
	{
		result = NULL;
		LogError("invalid parameter (NULL).");
	}
	/* Codes_SRS_MESSAGE_17_009: [If field sourceContent of cfg is NULL, then Message_CreateFromBuffer shall fail and return NULL.]*/
	else if (cfg->sourceContent == NULL)
	{
		result = NULL;
		LogError("invalid CONSTBUFFER (NULL)");
	}
	/*Codes_SRS_MESSAGE_17_010: [If field sourceProperties of cfg is NULL, then Message_CreateFromBuffer shall fail and return NULL.]*/
	else if (cfg->sourceProperties == NULL)
	{
		result = NULL;
		LogError("invalid properties (NULL)");
	}
	else
	{
		/*Codes_SRS_MESSAGE_17_011: [If Message_CreateFromBuffer encounters an error while building the internal structures of the message, then it shall return NULL.]*/
		/*Codes_SRS_MESSAGE_17_014: [On success, Message_CreateFromBuffer shall return a non-NULL handle and set the internal ref count to "1".]*/
		result = REFCOUNT_TYPE_CREATE(MESSAGE_HANDLE_DATA);
		if (result == NULL)
		{
			LogError("malloc returned NULL");
			/*return as is*/
		}
		else
		{
			/*Codes_SRS_MESSAGE_17_013: [Message_CreateFromBuffer shall clone the CONSTBUFFER sourceBuffer.]*/
			result->content = CONSTBUFFER_Clone(cfg->sourceContent);
			if (result->content == NULL)
			{
				LogError("CONSBUFFER Clone failed");
				free(result);
				result = NULL;
			}
			else
			{
				/*Codes_SRS_MESSAGE_17_012: [Message_CreateFromBuffer shall copy the sourceProperties to a readonly CONSTMAP.]*/
				result->properties = ConstMap_Create(cfg->sourceProperties);
				if (result->properties == NULL)
				{
					LogError("ConstMap_Create failed");
					CONSTBUFFER_Destroy(result->content);
					free(result);
					result = NULL;
				}
				else
				{
					/*all is fine, return as is.*/
				}
			}
		}
	}
	return (MESSAGE_HANDLE)result;
}

MESSAGE_HANDLE Message_Clone(MESSAGE_HANDLE message)
{
    
    if (message == NULL)
    {
        LogError("invalid arg: message is NULL");
        /*Codes_SRS_MESSAGE_02_007: [If messageHandle is NULL then Message_Clone shall return NULL.] */
    }
    else
    {
        /*Codes_SRS_MESSAGE_02_008: [Otherwise, Message_Clone shall increment the internal ref count.] */
        INC_REF(MESSAGE_HANDLE_DATA, message);
        MESSAGE_HANDLE_DATA* messageData = (MESSAGE_HANDLE_DATA*)message;
		/*Codes_SRS_MESSAGE_17_001: [Message_Clone shall clone the CONSTMAP handle.]*/
		(void)ConstMap_Clone(messageData->properties);
		/*Codes_SRS_MESSAGE_17_004: [Message_Clone shall clone the CONSTBUFFER handle]*/
		(void)CONSTBUFFER_Clone(messageData->content);
    }
    /*Codes_SRS_MESSAGE_02_010: [Message_Clone shall return messageHandle.]*/
    return message;
}

CONSTMAP_HANDLE Message_GetProperties(MESSAGE_HANDLE message)
{
    CONSTMAP_HANDLE result;
    /*Codes_SRS_MESSAGE_02_011: [If message is NULL then Message_GetProperties shall return NULL.] */
    if (message == NULL)
    {
        LogError("invalid arg: message is NULL");
        result = NULL;
    }
    else
    {
        /*Codes_SRS_MESSAGE_02_012: [Otherwise, Message_GetProperties shall shall clone and return the CONSTMAP handle representing the properties of the message.]*/
        MESSAGE_HANDLE_DATA* messageData = (MESSAGE_HANDLE_DATA*)message;
        result = ConstMap_Clone(messageData->properties);
    }
    return result;
}

const CONSTBUFFER * Message_GetContent(MESSAGE_HANDLE message)
{
	const CONSTBUFFER* result;
    /*Codes_SRS_MESSAGE_02_013: [If message is NULL then Message_GetContent shall return NULL.] */
    if (message == NULL)
    {
        LogError("invalid argument, message is NULL");
        result = NULL;
    }
    else
    {
        /*Codes_SRS_MESSAGE_02_014: [Otherwise, Message_GetContent shall return a non-NULL const pointer to a structure of type MESSAGE_CONTENT.]*/
		/*Codes_SRS_MESSAGE_02_016: [The CONSTBUFFER's field buffer shall compare equal byte-by-byte to the cfg's field source.]*/
        result = CONSTBUFFER_GetContent(((MESSAGE_HANDLE_DATA*)message)->content);
    }
    return result;
}

CONSTBUFFER_HANDLE Message_GetContentHandle(MESSAGE_HANDLE message) 
{
	CONSTBUFFER_HANDLE result;
	if (message == NULL)
	{
		/*Codes_SRS_MESSAGE_17_006: [If message is NULL then Message_GetContentHandle shall return NULL.] */
		LogError("invalid argument, message is NULL");
		result = NULL;
	}
	else
	{
		/*Codes_SRS_MESSAGE_17_007: [Otherwise, Message_GetContentHandle shall shall clone and return the CONSTBUFFER_HANDLE representing the message content.]*/
		result = CONSTBUFFER_Clone(((MESSAGE_HANDLE_DATA*)message)->content);
	}
	return result;
}

void Message_Destroy(MESSAGE_HANDLE message)
{
    /*Codes_SRS_MESSAGE_02_017: [If message is NULL then Message_Destroy shall do nothing.] */
    if (message == NULL)
    {
        LogError("invalid arg: message is NULL");
    }
    else
    {
        MESSAGE_HANDLE_DATA* messageData = (MESSAGE_HANDLE_DATA*)message;
		/*Codes_SRS_MESSAGE_17_002: [Message_Destroy shall destroy the CONSTMAP properties.]*/
		ConstMap_Destroy(messageData->properties);
		/*Codes_SRS_MESSAGE_17_005: [Message_Destroy shall destroy the CONSTBUFFER.]*/
		CONSTBUFFER_Destroy(messageData->content);
        /*Codes_SRS_MESSAGE_02_020: [Otherwise, Message_Destroy shall decrement the internal ref count of the message.]*/
        if (DEC_REF(MESSAGE_HANDLE_DATA, message) == DEC_RETURN_ZERO)
        {
            /*Codes_SRS_MESSAGE_02_021: [If the ref count is zero then the allocated resources are freed.]*/
            free(message);
        }
    }
}
