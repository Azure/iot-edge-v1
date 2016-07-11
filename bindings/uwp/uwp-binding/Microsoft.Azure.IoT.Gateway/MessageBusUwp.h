// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include "pch.h"
#include "..\..\..\core\inc\message_bus.h"

namespace Microsoft { namespace Azure { namespace IoT { namespace Gateway {

	interface class IGatewayModule;
	ref class Message;

	public ref class MessageBus sealed
	{
	internal:
		MessageBus(MESSAGE_BUS_HANDLE handle, MODULE_HANDLE module_handle)
		{ 
			this->message_bus_handle = handle;
			this->module_handle = module_handle;
		}

	public:
		void Publish(Message ^message);

	private:
		MESSAGE_BUS_HANDLE message_bus_handle;
		MODULE_HANDLE module_handle;
	};

}}}};
