// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef JAVA_MODULE_HOST_COMMON_H
#define JAVA_MODULE_HOST_COMMON_H

#define BROKER_CLASS_NAME "com/microsoft/azure/gateway/core/Broker"
#define CONSTRUCTOR_METHOD_NAME "<init>"
#define MODULE_DESTROY_METHOD_NAME "destroy"
#define MODULE_RECEIVE_METHOD_NAME "receive"
#define MODULE_START_METHOD_NAME "start"
#define MODULE_DESTROY_DESCRIPTOR "()V"
#define MODULE_RECEIVE_DESCRIPTOR "([B)V"
#define MODULE_START_DESCRIPTOR "()V"
#define BROKER_CONSTRUCTOR_DESCRIPTOR "(J)V"
#define MODULE_CONSTRUCTOR_DESCRIPTOR "(JLcom/microsoft/azure/gateway/core/Broker;Ljava/lang/String;)V"
#define MODULE_EMPTY_CONSTRUCTOR_DESCRIPTOR "()V"
#define MODULE_CREATE_METHOD_NAME "create"
#define MODULE_CREATE_DESCRIPTOR "(JLcom/microsoft/azure/gateway/core/Broker;Ljava/lang/String;)V"
#define DEBUG_PORT_DEFAULT 9876
#define DEBUG_PORT_MAX_VALUE 65535
#define DEBUG_OPTIONS_STR_SIZE 64

#endif /*JAVA_MODULE_HOST_COMMON_H*/
