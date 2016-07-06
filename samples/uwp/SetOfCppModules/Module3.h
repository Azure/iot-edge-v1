// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

namespace SetOfCppModules
{
	public ref class Module3 sealed : Microsoft::Azure::IoT::Gateway::IGatewayModule
    {
    public:
		Module3();

		// Inherited via IGatewayModule
		virtual void Create(Microsoft::Azure::IoT::Gateway::MessageBus ^bus, Platform::String ^configuration);
		virtual void Destroy();
		virtual void Receive(Microsoft::Azure::IoT::Gateway::Message ^received_message);
	};
}
