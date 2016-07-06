// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pch.h"

#include "MessageBusUwp.h"
#include "MessageUwp.h"
#include "IGatewayModule.h"

#include "message_bus.h"

using namespace Windows::Foundation::Collections;
using namespace Microsoft::Azure::IoT::Gateway;

void MessageBus::Publish(Message ^message)
{
	MESSAGE_HANDLE msg = message->MessageHandle;
	MessageBus_Publish(message_bus_handle, nullptr, msg);
}

