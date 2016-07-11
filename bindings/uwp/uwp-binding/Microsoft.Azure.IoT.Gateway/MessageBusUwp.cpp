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
	if (message == nullptr)
	{
		throw ref new Platform::InvalidArgumentException("message must be non-null.");
	}

	MESSAGE_HANDLE msg = message->MessageHandle;
	if (MessageBus_Publish(message_bus_handle, module_handle, msg) != MESSAGE_BUS_OK)
	{
		throw ref new Platform::FailureException("Failed to publish message.");
	}
}

