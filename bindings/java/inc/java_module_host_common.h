// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef JAVA_MODULE_HOST_COMMON_H
#define JAVA_MODULE_HOST_COMMON_H

#define MESSAGE_BUS_CLASS_NAME "com/microsoft/azure/gateway/core/MessageBus"
#define CONSTRUCTOR_METHOD_NAME "<init>"
#define MODULE_DESTROY_METHOD_NAME "destroy"
#define MODULE_RECEIVE_METHOD_NAME "receive"
#define MODULE_DESTROY_DESCRIPTOR "()V"
#define MODULE_RECEIVE_DESCRIPTOR "([B)V"
#define MESSAGE_BUS_CONSTRUCTOR_DESCRIPTOR "(J)V"
#define MODULE_CONSTRUCTOR_DESCRIPTOR "(JLcom/microsoft/azure/gateway/core/MessageBus;Ljava/lang/String;)V"
#define DEBUG_PORT_DEFAULT 9876

#endif /*JAVA_MODULE_HOST_COMMON_H*/
