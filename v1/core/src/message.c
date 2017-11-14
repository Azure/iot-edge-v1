// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>
#include "azure_c_shared_utility/gballoc.h"

#include "message.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/map.h"
#include "azure_c_shared_utility/constmap.h"
#include "azure_c_shared_utility/xlogging.h"

#include "azure_c_shared_utility/refcount.h"

#define FIRST_MESSAGE_BYTE 0xA1  /*0xA1 comes from (A)zure (I)oT*/
#define SECOND_MESSAGE_BYTE 0x60 /*0x60 comes from (G)ateway*/

#define MIN_MESSAGE_BUFFER_LENGTH 14 /*14 is the minimum message length that is still valid*/

typedef struct MESSAGE_HANDLE_DATA_TAG
{
    CONSTMAP_HANDLE properties;
    CONSTBUFFER_HANDLE content;
}MESSAGE_HANDLE_DATA;

DEFINE_REFCOUNT_TYPE(MESSAGE_HANDLE_DATA);

static MESSAGE_HANDLE_DATA* Message_CreateImpl(const MESSAGE_CONFIG * cfg)
{
    MESSAGE_HANDLE_DATA* result;
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
            LogError("CONSBUFFER_Create failed");
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
    return result;
}

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
        /*delegate to internal function that does not do validation*/
        result = Message_CreateImpl(cfg);
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

/*this function parses the buffer pointed to by source, having size sourceSize, starting at index position for a int32_t value*/
/*if the parsing succeeds then *parsed is updated to reflect how many characters have been consumed*/
/*and *value is updated to the parsed value and the function return 0*/
/*if parsing fails, the function returns different than 0*/
static int parse_int32_t(const unsigned char* source, int32_t sourceSize, int32_t position, int32_t *parsed, int32_t* value)
{
    int result;
    if (position + 4 > sourceSize)
    {
        /*Codes_SRS_MESSAGE_02_025: [ If while parsing the message content, a read would occur past the end of the array (as indicated by size) then Message_CreateFromByteArray shall fail and return NULL. ]*/
        LogError("unable to parse a int32_t because it would go past the end of the source");
        result = __LINE__;
    }
    else
    {
        *parsed = 4;
        *value = 
            (source[position + 0] << 24) |
            (source[position + 1] << 16) |
            (source[position + 2] <<  8) |
            (source[position + 3]);
        result = 0;
    }
    return result;
}

static int parse_null_terminated_const_char(const unsigned char* source, int32_t sourceSize, int32_t position, int32_t *parsed, const char** value)
{
    int result;
    const unsigned char* whereIsnull = (unsigned char*)memchr(source + position, '\0', sourceSize - position);
    
    if (whereIsnull == NULL)
    {
        /*Codes_SRS_MESSAGE_02_025: [ If while parsing the message content, a read would occur past the end of the array (as indicated by size) then Message_CreateFromByteArray shall fail and return NULL. ]*/
        LogError("was not able to find the end of the string");
        result = __LINE__;
    }
    else
    {
        *parsed = whereIsnull - ( source + position - 1);
        *value = (const char*)source + position;
        result = 0;
    }
    return result;
}

/*creates a MESSAGE_HANDLE from a serialized byte array*/
MESSAGE_HANDLE Message_CreateFromByteArray(const unsigned char* source, int32_t size)
{
    MESSAGE_HANDLE_DATA* result;
    /*Codes_SRS_MESSAGE_02_022: [ If source is NULL then Message_CreateFromByteArray shall fail and return NULL. ]*/
    /*Codes_SRS_MESSAGE_02_023: [ If source is not NULL and and size parameter is smaller than 14 then Message_CreateFromByteArray shall fail and return NULL. ]*/
    if (
        (source == NULL) ||
        (size < MIN_MESSAGE_BUFFER_LENGTH)
        )
    {
        LogError("invalid parameter source=[%p] size=%" PRId32, source, size);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_MESSAGE_02_024: [ If the first two bytes of source are not 0xA1 0x60 then Message_CreateFromByteArray shall fail and return NULL. ]*/
        if (
            (source[0] != FIRST_MESSAGE_BYTE) ||
            (source[1] != SECOND_MESSAGE_BYTE)
            )
        {
            LogError("byte array is not a gateway message serialization");
            result = NULL;
        }
        else
        {
            int32_t currentPosition = 2; /*current position is always the first character that "we are about to look at"*/
			int32_t parsed; /*reused in all parsings*/
			int32_t messageSize;
			/*Codes_SRS_MESSAGE_02_037: [ If the size embedded in the message is not the same as size parameter then Message_CreateFromByteArray shall fail and return NULL. ]*/
			if (parse_int32_t(source, size, currentPosition, &parsed, &messageSize) != 0)
			{
				LogError("unable to parse an int32_t");
				result = NULL;
			}
			else
			{
				currentPosition += parsed;
				if (messageSize != size)
				{
					LogError("message size is inconsistent");
					result = NULL;
				}
				else
				{
					/*Codes_SRS_MESSAGE_02_026: [ A MAP_HANDLE shall be created. ]*/
					MAP_HANDLE configMap = Map_Create(NULL);
					if (configMap == NULL)
					{
						/*Codes_SRS_MESSAGE_02_030: [ If any of the above steps fails, then Message_CreateFromByteArray shall fail and return NULL. ]*/
						LogError("failed to create a MAP_HANDLE");
						result = NULL;
					}
					else
					{
						/*add all the properties to the map*/
						int32_t propertiesCount;
						if (parse_int32_t(source, size, currentPosition, &parsed, &propertiesCount) != 0)
						{
							LogError("unable to parse an int32_t");
							result = NULL;
						}
						else
						{
							currentPosition += parsed;

							if (
								(propertiesCount < 0) ||
								(propertiesCount == INT32_MAX)
								)
							{
								/*Codes_SRS_MESSAGE_02_030: [ If any of the above steps fails, then Message_CreateFromByteArray shall fail and return NULL. ]*/
								LogError("invalid message detected with wrong number of properties =%" PRId32, propertiesCount);
								result = NULL;
							}
							else
							{
								int32_t i;

								for (i = 0; i < propertiesCount; i++)
								{
									const char* keyName;
									if (parse_null_terminated_const_char(source, size, currentPosition, &parsed, &keyName) != 0)
									{
										LogError("unable to parse the name string of the property");
										break;
									}
									else
									{
										const char* keyValue;
										currentPosition += parsed;
										if (parse_null_terminated_const_char(source, size, currentPosition, &parsed, &keyValue) != 0)
										{
											LogError("unable to parse the name string of the property");
											break;
										}
										else
										{
											currentPosition += parsed;
											/*Codes_SRS_MESSAGE_02_027: [ All the properties of the byte array shall be added to the MAP_HANDLE. ]*/
											if (Map_Add(configMap, keyName, keyValue) != MAP_OK)
											{
												LogError("Map_Add failed\n");
												break;
											}
											else
											{
												/*all is fine, proceed to the next property*/
											}
										}
									}
								}

								if (i != propertiesCount)
								{
									result = NULL;
								}
								else
								{
									/*all is fine*/
									int32_t messageContentSize;

									if (parse_int32_t(source, size, currentPosition, &parsed, &messageContentSize) != 0)
									{
										LogError("no space to read the number of bytes making the message");
										result = NULL;
									}
									else
									{
										currentPosition += parsed;
										if (currentPosition + messageContentSize != messageSize)
										{
											LogError("the message content doesn't up to the message size %" PRId32 " %" PRId32 "\n", (int32_t)(currentPosition + messageContentSize), messageSize);
											result = NULL;
										}
										else
										{
											/*Codes_SRS_MESSAGE_02_028: [ A structure of type MESSAGE_CONFIG shall be populated with the MAP_HANDLE previously constructed and the message content ]*/
											MESSAGE_CONFIG msgConfig = { (size_t)messageContentSize, source + currentPosition, configMap };

											/*Codes_SRS_MESSAGE_02_029: [ A MESSAGE_HANDLE shall be constructed from the MESSAGE_CONFIG. ]*/
											/*Codes_SRS_MESSAGE_02_031: [ Otherwise Message_CreateFromByteArray shall succeed and return a non-NULL handle. ]*/
											result = Message_CreateImpl(&msgConfig);

											/*return as is*/

										}
									}
								}
							}
						}
						Map_Destroy(configMap);
					}
				}
			}
        }
    }
    return (MESSAGE_HANDLE)result;

}

extern int32_t Message_ToByteArray(MESSAGE_HANDLE messageHandle, unsigned char* buf, int32_t size)
{
    int32_t result;
    if (messageHandle == NULL) 
    {
        /*Codes_SRS_MESSAGE_02_032: [ If messageHandle is NULL then Message_ToByteArray shall fail and return -1. ]*/
        LogError("invalid (NULL) messageHandle parameter detected", messageHandle, size);
        result = -1;
    }
    else if (
        (buf == NULL) &&
        (size != 0)
        )
    {
        /*Codes_SRS_MESSAGE_17_015: [ if buf is NULL and size is not equal to zero, Message_ToByteArray shall return -1; ]*/
        LogError("Null buffer sent with a specific size buffer=[%p], size=[%d]", messageHandle, size);
        result = -1;
    }
    else
    {
        MESSAGE_HANDLE_DATA* messageHandleData = (MESSAGE_HANDLE_DATA*)messageHandle;

        /*Codes_SRS_MESSAGE_02_033: [Message_ToByteArray shall precompute the needed memory size.]*/
        size_t byteArraySize =
            + 2 /*header*/
            + 4 /*total size of byte array*/
            + 4 /*total number of properties*/
            + 0 /*an unknown at this moment number of bytes for properties*/
            + 4 /*number of bytes in messageContent*/
            + 0 /*an unknown at this moment number of bytes for message content*/
            ;
        
        const char* const * keys;
        const char* const * values;
        size_t nProperties;

        /*Codes_SRS_MESSAGE_02_035: [ If any of the above steps fails then Message_ToByteArray shall fail and return -1. ]*/
        if (ConstMap_GetInternals(messageHandleData->properties, &keys, &values, &nProperties) != CONSTMAP_OK)
        {
            LogError("failed to get the keys and values from the message properties");
            result = -1;
        }
        else
        {
            size_t i;
            for (i = 0;i < nProperties;i++)
            {
                /*add to the needed size the name and value of property i*/
                byteArraySize += (strlen(keys[i]) + 1) + (strlen(values[i]) + 1);
            }

            const CONSTBUFFER* messageContent = CONSTBUFFER_GetContent(messageHandleData->content);
            byteArraySize += messageContent->size;
            
            if (size == 0)
            {
                /*Codes_SRS_MESSAGE_17_016: [ If buf is NULL and size is equal to zero, Message_ToByteArray shall return the needed memory size. ]*/
                result = byteArraySize;
            }
            else if (byteArraySize > (size_t)size)
            {
                /*Codes_SRS_MESSAGE_17_017: [ If buf is not NULL and size is less than the needed memory size, Message_ToByteArray shall return -1; ]*/
                LogError("message is %u bytes, won't fit in buffer of %u bytes", byteArraySize, size);
                result = -1;
            }
            else
            {
                /*Codes_SRS_MESSAGE_02_034: [ Message_ToByteArray shall populate the memory with values as indicated in the implementation details. ]*/

                size_t currentPosition; /*always points to the byte we are about to write*/
                /*a header formed of the following hex characters in this order: 0xA1 0x60*/
                buf[0] = FIRST_MESSAGE_BYTE;
                buf[1] = SECOND_MESSAGE_BYTE;
                /*4 bytes in MSB order representing the total size of the byte array. */
                buf[2] = byteArraySize >> 24;
                buf[3] = (byteArraySize >> 16) & 0xFF;
                buf[4] = (byteArraySize >> 8) & 0xFF;
                buf[5] = (byteArraySize) & 0xFF;
                /*4 bytes in MSB order representing the number of properties*/
                buf[6] = nProperties >> 24;
                buf[7] = (nProperties >> 16) & 0xFF;
                buf[8] = (nProperties >> 8) & 0xFF;
                buf[9] = nProperties & 0xFF;
                /*for every property, 2 arrays of null terminated characters representing the name of the property and the value.*/
				currentPosition = 10;
                for (i = 0;i < nProperties;i++)
                {
                    size_t nameLength = strlen(keys[i]) + 1;/*the +1 will take care of copying '\0' too*/
                    size_t valueLength = strlen(values[i]) + 1;/*the +1 will take care of copying '\0' too*/

                    /*copy name*/
                    memcpy(buf + currentPosition, keys[i], nameLength);
                    currentPosition += nameLength;
                    
                    /*copy value*/
                    memcpy(buf + currentPosition, values[i], valueLength);
                    currentPosition += valueLength;
                }

                /*4 bytes in MSB order representing the number of bytes in the message content array*/
                buf[currentPosition++] = (messageContent->size) >> 24;
                buf[currentPosition++] = ((messageContent->size) >> 16) & 0xFF;
                buf[currentPosition++] = ((messageContent->size) >> 8) & 0xFF;
                buf[currentPosition++] = (messageContent->size) & 0xFF;

                /*n bytes of message content follows.*/
                memcpy(buf + currentPosition, messageContent->buffer, messageContent->size);

                /*Codes_SRS_MESSAGE_02_036: [ Otherwise Message_ToByteArray shall succeed, and return the byte array size. ]*/
                result = byteArraySize;
            }
        }
    }
    return result;
}