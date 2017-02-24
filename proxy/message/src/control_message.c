// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include "control_message.h"

#include <stdlib.h>

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"

#include "message.h"

DEFINE_ENUM_STRINGS(CONTROL_MESSAGE_TYPE, CONTROL_MESSAGE_TYPE_VALUES);

#define FIRST_MESSAGE_BYTE 0xA1  /*0xA1 comes from (A)zure (I)oT*/
#define SECOND_MESSAGE_BYTE 0x6C /*0x6C comes from (G)ateway (C)ontrol */
#define BASE_MESSAGE_SIZE 8
#define BASE_CREATE_SIZE (BASE_MESSAGE_SIZE+10)
#define BASE_CREATE_REPLY_SIZE (BASE_MESSAGE_SIZE+1)

static int parse_int32_t(const unsigned char* source, size_t sourceSize, size_t position, int32_t *parsed, int32_t* value)
{
    int result;
    if (position + 4 > sourceSize)
    {
		/*Codes_SRS_CONTROL_MESSAGE_17_018: [ Reading past the end of the byte array shall cause this function to fail and return NULL. ]*/
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

static int parse_memory_chunk(const unsigned char* source, size_t sourceSize, size_t position, int32_t *parsed, int32_t *size, unsigned char** value)
{
    int result;
    int32_t chunk_size;
    int32_t current_parsed;
    result = parse_int32_t(source, sourceSize, position, &current_parsed, &chunk_size);
    if (result == 0)
    {
        position += current_parsed;
        if (position + chunk_size > sourceSize)
        {
			/*Codes_SRS_CONTROL_MESSAGE_17_018: [ Reading past the end of the byte array shall cause this function to fail and return NULL. ]*/
            LogError("unable to extract memory because it would go past end of source");
            result = __LINE__;
        }
        else
        {
			if (chunk_size == 0)
			{ 
				*value = NULL;
				*parsed = current_parsed;
				*size = chunk_size;
				result = 0;
			}
			else
			{
				/* allocate 1 more for null-termination */
				*value = (unsigned char *)malloc(chunk_size);
				if (*value == NULL)
				{
					LogError("unable to allocate memory chunk");
					result = __LINE__;
				}
				else
				{
					memcpy(*value, source + position, chunk_size);
					*parsed = current_parsed + chunk_size;
					*size = chunk_size;
					result = 0;
				}
			}
        }
    }
    return result;
}

static void init_create_message_contents(CONTROL_MESSAGE_MODULE_CREATE * create_msg)
{
    create_msg->gateway_message_version = 0x00;
	create_msg->uri.uri_type = CONTROL_MESSAGE_TYPE_ERROR;
	create_msg->uri.uri_size = 0;
    create_msg->uri.uri = NULL;
    create_msg->args_size = 0;
    create_msg->args = NULL;
}

static void free_create_message_contents(CONTROL_MESSAGE_MODULE_CREATE * create_msg)
{
    if (create_msg->uri.uri != NULL)
    {
        free(create_msg->uri.uri);
    }
    if (create_msg->args != NULL)
    {
        free(create_msg->args);
    }
}

int parse_create_message(const unsigned char* source, size_t sourceSize, size_t position, int32_t *parsed, CONTROL_MESSAGE_MODULE_CREATE * create_msg)
{
    int result;
    int32_t current_parsed; /*reused in all parsings*/
	init_create_message_contents(create_msg); /* initialize to NULL for easy cleanup */
    /* 
	 * Parse the number of uris
	 * We already know we won't extend past the end of the array - 
	 * it was checked before entering this function 
	 */

	if (position + 2 > sourceSize)
	{
		LogError("couldn't parse message uri");
		result = __LINE__;
	}
	else
	{
		create_msg->gateway_message_version = (uint8_t)source[position++];
		/*Codes_SRS_CONTROL_MESSAGE_17_012: [ This function shall read the uri_type, uri_size, and the uri. ]*/
		create_msg->uri.uri_type =	(uint8_t)source[position++];

		/*Codes_SRS_CONTROL_MESSAGE_17_013: [ This function shall allocate uri_size bytes for the url. ]*/
		if (parse_memory_chunk(source,
			sourceSize,
			position,
			&current_parsed,
			&(create_msg->uri.uri_size),
			&(create_msg->uri.uri)) != 0)
		{
			LogError("unable to parse a uri");
			result = __LINE__;
		}
		else
		{
			position += current_parsed;
			result = 0;
		}
	}
    if (result != 0)
    {
        /* Did not come out of parsing the URLs OK */
		/*Codes_SRS_CONTROL_MESSAGE_17_022: [ This function shall release all allocated memory upon failure. ]*/
        free_create_message_contents(create_msg);
    }
	else
	{
		/*Codes_SRS_CONTROL_MESSAGE_17_014: [ This function shall read the args_size and args from the byte stream. ]*/
		/*Codes_SRS_CONTROL_MESSAGE_17_015: [ This function shall allocate args_size + 1 bytes for the args and will null terminate the memory block. ]*/
		if (parse_memory_chunk(source,
			sourceSize,
			position,
			&current_parsed,
			&(create_msg->args_size),
			(unsigned char**)&(create_msg->args)) != 0)
		{
			LogError("unable to parse a module args");
			result = __LINE__;
			/*Codes_SRS_CONTROL_MESSAGE_17_022: [ This function shall release all allocated memory upon failure. ]*/
			free_create_message_contents(create_msg);
		}
		else
        {
            result = 0;
		}
	}
    return result;
}

CONTROL_MESSAGE * ControlMessage_CreateFromByteArray(const unsigned char* source, size_t size)
{
    CONTROL_MESSAGE * result;
	/*Codes_SRS_CONTROL_MESSAGE_17_001: [ If source is NULL, then this function shall return NULL. ]*/
	/*Codes_SRS_CONTROL_MESSAGE_17_002: [ If size is smaller than 8, then this function shall return NULL. ]*/
    if (
        (source == NULL) ||
        (size < BASE_MESSAGE_SIZE)
        )
    {
        LogError("invalid parameter source=[%p] size=%z" , source, size);
        result = NULL;
    }
    else
    {
		/*Codes_SRS_CONTROL_MESSAGE_17_003: [ If the first two bytes of source are not 0xA1 0x6C then this function shall fail and return NULL. ]*/
		/*Codes_SRS_CONTROL_MESSAGE_17_004: [ If the version is not equal to CONTROL_MESSAGE_VERSION_CURRENT, then this function shall return NULL. ]*/
        if (
            (source[0] != FIRST_MESSAGE_BYTE) ||
            (source[1] != SECOND_MESSAGE_BYTE) ||
			((uint8_t)source[2] != CONTROL_MESSAGE_VERSION_CURRENT) 
			)
        {
            LogError("byte array is not a control message serialization");
            result = NULL;
        }
        else
        {
            size_t currentPosition = 2; /*current position is always the first character that "we are about to look at"*/
            int32_t parsed; /*reused in all parsings*/
			/*Codes_SRS_CONTROL_MESSAGE_17_005: [ This function shall read the version, type and size from the byte stream. ]*/
            uint8_t messageVersion = (uint8_t)source[currentPosition++];
			uint8_t messageType = (uint8_t)source[currentPosition++];
			int32_t messageSize;
			/* we already know buffer is at least BASE_MESSAGE_SIZE - this will always return OK */
			(void)parse_int32_t(source, size, currentPosition, &parsed, &messageSize);
            currentPosition += parsed;
			/*Codes_SRS_CONTROL_MESSAGE_17_006: [ If the size embedded in the message is not the same as size parameter then this function shall fail and return NULL. ]*/
            if (messageSize != size)
            {
                LogError("message size is inconsistent");
                result = NULL;
            }
            else
            {
                if (messageType == CONTROL_MESSAGE_TYPE_MODULE_CREATE)
                {
					/*Codes_SRS_CONTROL_MESSAGE_17_009: [ If the total message size is not at least 20 bytes, then this function shall return NULL ]*/
                    if (size < BASE_CREATE_SIZE)
                    {
                        result = NULL;
                    }
                    else
                    {
						/*Codes_SRS_CONTROL_MESSAGE_17_008: [ This function shall allocate a CONTROL_MESSAGE_MODULE_CREATE structure. ]*/
                        result = (CONTROL_MESSAGE *)malloc(sizeof(CONTROL_MESSAGE_MODULE_CREATE));
                        if (result != NULL)
                        {
							/*Codes_SRS_CONTROL_MESSAGE_17_024: [ Upon valid reading of the byte stream, this function shall assign the message version and type into the CONTROL_MESSAGE base structure. ]*/
							result->version = messageVersion;
                            result->type = messageType;
                            if (parse_create_message(
                                    source, 
                                    size, 
                                    currentPosition, 
                                    &parsed, 
                                    (CONTROL_MESSAGE_MODULE_CREATE*)result) != 0)
                            {
								/*Codes_SRS_CONTROL_MESSAGE_17_022: [ This function shall release all allocated memory upon failure. ]*/
                                free(result);
                                result = NULL;
                            }
                        }
                    }
                }
                else if (messageType == CONTROL_MESSAGE_TYPE_MODULE_CREATE_REPLY)
                {
					/*Codes_SRS_CONTROL_MESSAGE_17_020: [ If the total message size is not at least 9 bytes, then this function shall fail and return NULL. ]*/
                    if (size < BASE_CREATE_REPLY_SIZE)
                    {
                        result = NULL;
                    }
                    else
                    {
						/*Codes_SRS_CONTROL_MESSAGE_17_019: [ This function shall allocate a CONTROL_MESSAGE_MODULE_REPLY structure. ]*/
                        result = (CONTROL_MESSAGE *)malloc(sizeof(CONTROL_MESSAGE_MODULE_REPLY));
                        if (result != NULL)
                        {
							/*Codes_SRS_CONTROL_MESSAGE_17_024: [ Upon valid reading of the byte stream, this function shall assign the message version and type into the CONTROL_MESSAGE base structure. ]*/
							result->version = messageVersion;
                            result->type = messageType;
							/*Codes_SRS_CONTROL_MESSAGE_17_021: [ This function shall read the status from the byte stream. ]*/
                            ((CONTROL_MESSAGE_MODULE_REPLY*)result)->status = 
                                (uint8_t)source[currentPosition];
                        }
                    }
                }
                else if (
                        (messageType == CONTROL_MESSAGE_TYPE_MODULE_START) || 
                        (messageType == CONTROL_MESSAGE_TYPE_MODULE_DESTROY)
                        )
                {
					/*Codes_SRS_CONTROL_MESSAGE_17_023: [ This function shall allocate a CONTROL_MESSAGE structure. ]*/
                    result = (CONTROL_MESSAGE *)malloc(sizeof(CONTROL_MESSAGE));
                    if (result != NULL)
                    {
						/*Codes_SRS_CONTROL_MESSAGE_17_024: [ Upon valid reading of the byte stream, this function shall assign the message version and type into the CONTROL_MESSAGE base structure. ]*/
                        result->version = messageVersion;
                        result->type = messageType;
                    }
                }
                else
                {
					/*Codes_SRS_CONTROL_MESSAGE_17_007: [ This function shall return NULL if the type is not a valid enum value of CONTROL_MESSAGE_TYPE or CONTROL_MESSAGE_TYPE_ERROR. ]*/
                    LogError("message is an unrecognized type: %s", ENUM_TO_STRING(CONTROL_MESSAGE_TYPE, messageType));
                    result = NULL;
                }
            }
        }
    }
	/*Codes_SRS_CONTROL_MESSAGE_17_025: [ Upon success, this function shall return a valid pointer to the CONTROL_MESSAGE base. ]*/
	/*Codes_SRS_CONTROL_MESSAGE_17_036: [ This function shall return NULL upon failure. ]*/
    return result;
}

void ControlMessage_Destroy(CONTROL_MESSAGE * message)
{
	/*Codes_SRS_CONTROL_MESSAGE_17_026: [ If message is NULL this function shall do nothing. ]*/
    if (message != NULL)
    {
		/*Codes_SRS_CONTROL_MESSAGE_17_027: [ This function shall release all memory allocated in this structure. ]*/
        if (message->type == CONTROL_MESSAGE_TYPE_MODULE_CREATE)
        {
			/*Codes_SRS_CONTROL_MESSAGE_17_028: [ For a CONTROL_MESSAGE_MODULE_CREATE message type, all memory allocated shall be defined as all the memory allocated by ControlMessage_CreateFromByteArray. ]*/
            free_create_message_contents((CONTROL_MESSAGE_MODULE_CREATE *)message);
        }
		/*Codes_SRS_CONTROL_MESSAGE_17_030: [ This function shall release message for all message types. ]*/
        free(message);
    }
}

static int32_t create_message_get_size(CONTROL_MESSAGE * message)
{
    int32_t result;

    CONTROL_MESSAGE_MODULE_CREATE * create_msg = (CONTROL_MESSAGE_MODULE_CREATE*)message;
    result = 
          1  /* gateway_message_version */
        + 1 /* uri_type */
		+ 4 /* uri_size */
        + 4; /* args_size */
	if (create_msg->uri.uri != NULL)
	{
		result +=
			(int32_t)strlen(create_msg->uri.uri)
			+ 1; /* for null char */
	}
    if (create_msg->args != NULL)
    {
        result +=
            (int32_t)strlen(create_msg->args)
            + 1; /* for null char */
    }
        
    return result;
}

static void create_message_serialize(CONTROL_MESSAGE_MODULE_CREATE * create_msg, unsigned char* buf, size_t currentPosition)
{
	buf[currentPosition++] = (create_msg->gateway_message_version);
    buf[currentPosition++] = create_msg->uri.uri_type;
    buf[currentPosition++] = (create_msg->uri.uri_size) >> 24;
    buf[currentPosition++] = ((create_msg->uri.uri_size) >> 16) & 0xFF;
    buf[currentPosition++] = ((create_msg->uri.uri_size) >> 8) & 0xFF;
    buf[currentPosition++] = (create_msg->uri.uri_size) & 0xFF;
    if (create_msg->uri.uri_size > 0)
    {
        memcpy(buf + currentPosition, create_msg->uri.uri, create_msg->uri.uri_size);
        currentPosition += create_msg->uri.uri_size;
    }
    buf[currentPosition++] = (create_msg->args_size) >> 24;
    buf[currentPosition++] = ((create_msg->args_size) >> 16) & 0xFF;
    buf[currentPosition++] = ((create_msg->args_size) >> 8) & 0xFF;
    buf[currentPosition++] = (create_msg->args_size) & 0xFF;
    if (create_msg->args_size > 0)
    {
        memcpy(buf + currentPosition, create_msg->args, create_msg->args_size);
        currentPosition += create_msg->args_size;
    }
}


int32_t ControlMessage_ToByteArray(CONTROL_MESSAGE * message, unsigned char* buf, int32_t size)
{
    int32_t result;
    if (message == NULL) 
    {
		/*Codes_SRS_CONTROL_MESSAGE_17_031: [ If message is NULL, then this function shall return -1. ]*/
		LogError("invalid (NULL) message parameter detected", message, size);
        result = -1;
    }
    else if (
        (buf == NULL) &&
        (size != 0)
        )
    {
		/*Codes_SRS_CONTROL_MESSAGE_17_032: [ If buf is NULL, but size is not zero, then this function shall return -1. ]*/
        LogError("Null buffer sent with a specific size buffer=[%p], size=[%d]", message, size);
        result = -1;
    }
    else
    {
        //MESSAGE_HANDLE_DATA* messageData = (MESSAGE_HANDLE_DATA*)message;
		/*Codes_SRS_CONTROL_MESSAGE_17_033: [ This function shall populate the memory with values as indicated in control messages in out process modules. ]*/
        size_t byteArraySize =
            + 2 /* header */
            + 1 /* message type*/
            + 1 /* message version */
            + 4 /* total size */
            + 0 /*an unknown at this moment number of bytes for message content*/
            ;
        if (message->type == CONTROL_MESSAGE_TYPE_MODULE_CREATE)
        {
            byteArraySize += create_message_get_size(message);
            result = 0;
        }
        else if (message->type == CONTROL_MESSAGE_TYPE_MODULE_CREATE_REPLY)
        {
            result = 0;
            byteArraySize += 1; /* status */
        }
        else if (
                 (message->type == CONTROL_MESSAGE_TYPE_MODULE_START) || 
                 (message->type == CONTROL_MESSAGE_TYPE_MODULE_DESTROY)
                )
        {
            /* no additional fields */
            result =0;
        }
        else
        {
			/*Codes_SRS_CONTROL_MESSAGE_17_034: [ If any of the above steps fails then this function shall fail and return -1. ]*/
            LogError("message is an unrecognized type: %s", ENUM_TO_STRING(CONTROL_MESSAGE_TYPE, message->type));
            result = -1;
        }

        if (result >=0 )
        {
            if (size == 0)
            {
                result = byteArraySize;
            }
            else if (byteArraySize > (size_t)size)
            {
				/*Codes_SRS_CONTROL_MESSAGE_17_034: [ If any of the above steps fails then this function shall fail and return -1. ]*/
				LogError("message is %u bytes, won't fit in buffer of %u bytes", byteArraySize, size);
                result = -1;
            }
            else
            {
                size_t currentPosition; /*always points to the byte we are about to write*/
                /*a header formed of the following hex characters in this order: 0xA1 0x60*/
                buf[0] = FIRST_MESSAGE_BYTE;
                buf[1] = SECOND_MESSAGE_BYTE;
				/*Codes_SRS_CONTROL_MESSAGE_17_037: [ This function shall read the gateway_message_version. ]*/
                buf[2] = message->version;
                buf[3] = (uint8_t)message->type;

                /*4 bytes in MSB order representing the total size of the byte array. */
                buf[4] = byteArraySize >> 24;
                buf[5] = (byteArraySize >> 16) & 0xFF;
                buf[6] = (byteArraySize >> 8) & 0xFF;
                buf[7] = (byteArraySize) & 0xFF;
                
                // 
                currentPosition = 8;
                if (message->type == CONTROL_MESSAGE_TYPE_MODULE_CREATE)
                {
                    create_message_serialize((CONTROL_MESSAGE_MODULE_CREATE *)message, buf, currentPosition);
                }
                else if (message->type == CONTROL_MESSAGE_TYPE_MODULE_CREATE_REPLY)
                {
                    CONTROL_MESSAGE_MODULE_REPLY * reply_msg = 
                            (CONTROL_MESSAGE_MODULE_REPLY*)message;
                    buf[currentPosition++] = (reply_msg->status);
                }
				/*Codes_SRS_CONTROL_MESSAGE_17_035: [ Upon success this function shall return the byte array size.*/
                result = byteArraySize;
            }
        }
    }
    return result;
}

