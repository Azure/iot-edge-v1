// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

namespace SetOfCppModules
{
	public ref class Module2 sealed : Microsoft::Azure::IoT::Gateway::IGatewayModule
    {
    public:
		Module2();

		// Inherited via IGatewayModule
		virtual void Create(Microsoft::Azure::IoT::Gateway::MessageBus ^bus, Windows::Foundation::Collections::IMapView<Platform::String^, Platform::String^>^ configuration);
		virtual void Destroy();
		virtual void Receive(Microsoft::Azure::IoT::Gateway::Message ^received_message);
	};
}
