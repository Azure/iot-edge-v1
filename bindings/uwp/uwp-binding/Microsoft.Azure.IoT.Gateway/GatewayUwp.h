// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once
#include "pch.h"


#include "gateway.h"
#include "IGatewayModule.h"

namespace Microsoft { namespace Azure { namespace IoT { namespace Gateway {

	interface class IGatewayModule;
	ref class Broker;

	class InternalGatewayModule : public IInternalGatewayModule
	{
	public:
		InternalGatewayModule(IGatewayModule ^moduleImpl) { _moduleImpl = moduleImpl; }
		virtual ~InternalGatewayModule() 
		{
		}

		void Module_Create(BROKER_HANDLE broker, Windows::Foundation::Collections::IMapView<Platform::String^, Platform::String^>^ properties);
		void Module_Destroy();
		void Module_Receive(MESSAGE_HANDLE messageHandle);

	private:
		IGatewayModule^ _moduleImpl;
	};

	public ref class Gateway sealed
	{
	public:
		Gateway(Windows::Foundation::Collections::IVector<Microsoft::Azure::IoT::Gateway::IGatewayModule^>^ modules,
			    Windows::Foundation::Collections::IMapView<Platform::String^, Platform::String^>^ properties);
		virtual ~Gateway();

	private:
		VECTOR_HANDLE modules_handle;
		GATEWAY_HANDLE gateway_handle;
		BROKER_HANDLE broker_handle;

		std::vector<std::unique_ptr<InternalGatewayModule>> gatewayModulesToDelete;

	};

}}}};


