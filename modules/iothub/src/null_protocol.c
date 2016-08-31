// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#ifndef NULL_PROTOCOL_H
#define NULL_PROTOCOL_H

#include <iothub_transport_ll.h>
#include <azure_c_shared_utility/xlogging.h>

#define NULL_PROTOCOL_MESSAGE " transport is not available"

#ifdef IOTHUBMODULE_NULL_HTTP
const TRANSPORT_PROVIDER* HTTP_Protocol(void)
{
    LogError("HTTP" NULL_PROTOCOL_MESSAGE);
    return NULL;
}
#endif

#ifdef IOTHUBMODULE_NULL_AMQP
const TRANSPORT_PROVIDER* AMQP_Protocol(void)
{
    LogError("AMQP" NULL_PROTOCOL_MESSAGE);
    return NULL;
}
#endif

#ifdef IOTHUBMODULE_NULL_MQTT
const TRANSPORT_PROVIDER* MQTT_Protocol(void)
{
    LogError("MQTT" NULL_PROTOCOL_MESSAGE);
    return NULL;
}
#endif

#endif // NULL_PROTOCOL_H