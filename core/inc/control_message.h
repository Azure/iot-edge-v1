// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef CONTROL_MESSAGE_H
#define CONTROL_MESSAGE_H

#include "azure_c_shared_utility/macro_utils.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define CONTROL_MESSAGE_VERSION_1           0x0010
#define CONTROL_MESSAGE_VERSION_CURRENT     CONTROL_MESSAGE_VERSION_1

#define CONTROL_MESSAGE_TYPE_VALUES      \
    CONTROL_MESSAGE_TYPE_ERROR           \
    CONTROL_MESSAGE_TYPE_CREATE          \
    CONTROL_MESSAGE_TYPE_CREATE_REPLY    \
    CONTROL_MESSAGE_TYPE_START           \
    CONTROL_MESSAGE_TYPE_DESTROY

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

    /** @brief  The total size of this message including the size of the
     *          CONTROL_MESSAGE header struct.
     */
    uint32_t  total_size;
}CONTROL_MESSAGE;

/** @brief    Defines the structure of a nanomsg URL.
 */
typedef struct NN_URL_TAG
{
    /** @brief  The size of the URL string.
     */
    const uint32_t  url_size;

    /** @brief  The actual URL.
     */
    const uint8_t*  url;
}NN_URL;

/** @brief    Defines the structure of the message that is sent for the
 *            "create" control message.
 */
typedef struct CONTROL_MESSAGE_CREATE_TAG
{
    /** @brief  The "base" message information.
     */
    CONTROL_MESSAGE base;

    /** @brief  Size of the array pointed at by the "urls" field.
     */
    const uint32_t urls_count;

    /** @brief  An array of URLs defining nanomsg binding URLs. The size is
     *          is given by the `urls_count` field.
     */
    const NN_URL* urls;

    /** @brief  Size of the module arguments.
     */
    const uint32_t args_size;

    /** @brief  Serialized JSON string of module arguments.
     */
    const char* args;

    /** @brief  Size of the loader statement.
     */
    const uint32_t loader_size;

    /** @brief  Serialized JSON string of the loader arguments.
     */
    const char* loader_arguments;
}CONTROL_MESSAGE_CREATE;

/** @brief    Defines the structure of the message that is sent in reply to the
 *            "create" control message.
 */
typedef struct CONTROL_MESSAGE_CREATE_REPLY_TAG
{
    /** @brief  The "base" message information.
     */
    CONTROL_MESSAGE base;

    /** @brief  A boolean value indicating whether the "create" message was
     *          processed successfully or not. This field uses the value 1 to
     *          indicate success and the value 0 to indicate failure.
     */
    uint8_t create_status;
}CONTROL_MESSAGE_CREATE_REPLY;

#ifdef __cplusplus
}
#endif

#endif /*CONTROL_MESSAGE_H*/
