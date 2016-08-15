// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pch.h"

#include "BrokerUwp.h"
#include "MessageUwp.h"
#include "IGatewayModule.h"

#include "broker.h"

using namespace Windows::Foundation::Collections;
using namespace Microsoft::Azure::IoT::Gateway;

void Broker::Publish(Message ^message)
{
	if (message == nullptr)
	{
		throw ref new Platform::InvalidArgumentException("message must be non-null.");
	}

	MESSAGE_HANDLE msg = message->MessageHandle;
	if (Broker_Publish(broker_handle, module_handle, msg) != BROKER_OK)
	{
		throw ref new Platform::FailureException("Failed to publish message.");
	}
}

