// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "message_bus.h"
#include "message_bus_proxy.h"
#include "azure_c_shared_utility\iot_logging.h"

/*
 * Class:     com_microsoft_azure_gateway_core_MessageBus
 * Method:    publishMessage
 * Signature: (J[B)I
 */
JNIEXPORT jint JNICALL Java_com_microsoft_azure_gateway_core_MessageBus_publishMessage(JNIEnv *env, jobject MessageBus, jlong addr, jobject message)
{
    return 0;
}