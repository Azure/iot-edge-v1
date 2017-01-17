// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef CONTROL_MESSAGE_H
#define CONTROL_MESSAGE_H

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>
#include <cinttypes>
extern "C"
{
#else
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdbool.h>
#endif

#include "azure_c_shared_utility/umock_c_prod.h"

#include "azure_c_shared_utility/macro_utils.h"
#include "gateway_export.h"

#define CONTROL_MESSAGE_VERSION_1           0x01
#define CONTROL_MESSAGE_VERSION_CURRENT     CONTROL_MESSAGE_VERSION_1

#define CONTROL_MESSAGE_TYPE_VALUES      \
    CONTROL_MESSAGE_TYPE_ERROR,          \
    CONTROL_MESSAGE_TYPE_MODULE_CREATE,         \
    CONTROL_MESSAGE_TYPE_MODULE_CREATE_REPLY,   \
    CONTROL_MESSAGE_TYPE_MODULE_START,          \
    CONTROL_MESSAGE_TYPE_MODULE_DESTROY

/** @brief    Enumeration specifying the various types of control messages that
 *            can be sent from a gateway process to a module host process.
 */
DEFINE_ENUM(CONTROL_MESSAGE_TYPE, CONTROL_MESSAGE_TYPE_VALUES);

/** @brief    Defines the base control message structure.
 */
typedef struct CONTROL_MESSAGE_TAG
{
    /** @brief  The control message version. Must be equal to the constant
     *          CONTROL_MESSAGE_VERSION_CURRENT.
     */
    uint8_t  version;

    /** @brief  A value from the CONTROL_MESSAGE_TYPE enumeration indicating the
     *          the type of this control message.
     */
    CONTROL_MESSAGE_TYPE  type;

}CONTROL_MESSAGE;

/** @brief    Defines the structure of a nanomsg URL.
 */
typedef struct MESSAGE_URI_TAG
{
    /** @brief  The size of the URL string.
     */
    uint32_t  uri_size;

    /** @brief  Type of URL.
     */
    uint8_t  uri_type;

    /** @brief  The actual URL.
     */
    char*  uri;
}MESSAGE_URI;

/** @brief    Defines the structure of the message that is sent for the
 *            "create" control message.
 */
typedef struct CONTROL_MESSAGE_MODULE_CREATE_TAG
{
    /** @brief  The "base" message information.
     */
	CONTROL_MESSAGE base;

	/** @brief  The current version of gateway message. 
	*/
	uint8_t  gateway_message_version;

    /** @brief  An array of URLs defining nanomsg binding URLs. The size is
     *          is given by the `uris_count` field.
     */
    MESSAGE_URI uri;

    /** @brief  Size of the module arguments.
     */
    uint32_t args_size;

    /** @brief  Serialized JSON string of module arguments.
     */
    char* args;

}CONTROL_MESSAGE_MODULE_CREATE;

/** @brief    Defines the structure of the message that is sent in reply to the
 *            "create" control message.
 */
typedef struct CONTROL_MESSAGE_MODULE_REPLY_TAG
{
    /** @brief  The "base" message information.
     */
    CONTROL_MESSAGE base;

    /** @brief  A boolean value indicating whether the "create" message was
     *          processed successfully or not. This field uses the value 1 to
     *          indicate success and the value 0 to indicate failure.
     */
    uint8_t status;
}CONTROL_MESSAGE_MODULE_REPLY;


/** @brief      Creates a new control message from a byte array
 *              containing the serialized form.
 *
 *  @details    The newly created control message the same content as
 *              the original control message.
 *
 *  @param      source  Pointer to a byte array.
 *  @param      size    size in bytes of the array
 *
 *  @return     A non-NULL #CONTROL_MESSAGE pointer for the newly 
 *              created message, or NULL upon failure.
 */
MOCKABLE_FUNCTION (,GATEWAY_EXPORT CONTROL_MESSAGE *, ControlMessage_CreateFromByteArray, const unsigned char*, source, size_t, size);

/** @brief      Destroys a control message.
 *
 *  @details    Frees all uri, and module arguments.
 *
 *  @param      message  Pointer to a control message.
 *
 *  @return     none.
 */
MOCKABLE_FUNCTION(, GATEWAY_EXPORT void, ControlMessage_Destroy, CONTROL_MESSAGE *, message);


/** @brief      Creates a byte array representation of a CONTROL_MESSAGE. 
 *
 *  @details    The byte array created can be used with function
 *              #ControlMessage_CreateFromByteArray to reproduce the control 
 *              message. If buffer is not set, this function will return the 
 *              serialization size.
 *
 *  @param      message         A #CONTROL_MESSAGE. Must not be NULL.
 *  @param      buf             A byte array pointer in memory, or NULL.
 *  @param      size            An int32_t that specifies the size of buf.
 *
 *  @return     An int32_t that specifies the size of the serialized message
 *              written when "buf" is not NULL. If "buf" is NULL and "size" is zero, 
 *				returns the size required for a full successful serialization. Returns a 
 *              negative value when an error occurs.
 */
MOCKABLE_FUNCTION(, GATEWAY_EXPORT int32_t, ControlMessage_ToByteArray,CONTROL_MESSAGE *, message, unsigned char*, buf, int32_t, size);

#ifdef __cplusplus
}
#endif


#endif /*CONTROL_MESSAGE_H*/
