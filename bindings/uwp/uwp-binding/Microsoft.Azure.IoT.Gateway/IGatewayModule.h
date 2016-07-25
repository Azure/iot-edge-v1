// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include "pch.h"

namespace Microsoft { namespace Azure { namespace IoT { namespace Gateway {

	ref class Message;
	ref class MessageBus;

	public interface class IGatewayModule
	{
		void Create(MessageBus^ bus, Windows::Foundation::Collections::IMapView<Platform::String^, Platform::String^>^ configuration);
		void Destroy();
		void Receive(Message ^received_message);
	};

}}}};

