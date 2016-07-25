// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pch.h"

#include "GatewayUwp.h"
#include "MessageBusUwp.h"
#include "MessageUwp.h"

using namespace Windows::Foundation::Collections;
using namespace Microsoft::Azure::IoT::Gateway;


ref class Message;

MODULE_HANDLE InternalGatewayModule::Module_Create(MESSAGE_BUS_HANDLE busHandle, IMapView<Platform::String^, Platform::String^>^ properties)
{
	_moduleImpl->Create(ref new MessageBus(busHandle, (MODULE_HANDLE)this), properties);
	return (MODULE_HANDLE)this;
}
void InternalGatewayModule::Module_Destroy()
{
	_moduleImpl->Destroy();
}
void InternalGatewayModule::Module_Receive(MESSAGE_HANDLE messageHandle)
{
	auto msg = ref new Microsoft::Azure::IoT::Gateway::Message(messageHandle);
	_moduleImpl->Receive(msg);
}

Gateway::Gateway(IVector<IGatewayModule^>^ modules, IMapView<Platform::String^, Platform::String^>^ properties)
{
	if (modules == nullptr)
	{
		throw ref new Platform::InvalidArgumentException("modules must be non-null.");
	}
	if (properties == nullptr)
	{
		throw ref new Platform::InvalidArgumentException("properties must be non-null.");
	}

	messagebus_handle = MessageBus_Create();
	if (messagebus_handle == nullptr)
	{
		throw ref new Platform::FailureException("Failed to create MessageBus.");
	}

	modules_handle = VECTOR_create(sizeof(MODULE));
	if (modules_handle == nullptr)
	{
		throw ref new Platform::FailureException("Failed to create VECTOR for modules.");
	}

	 
 	for (auto mod : modules)
	{
		std::unique_ptr<InternalGatewayModule> imod(new InternalGatewayModule(mod));
		auto module_handle = imod->Module_Create(messagebus_handle, properties);

		MODULE module;
		module.module_instance = imod.get();

		auto ret = VECTOR_push_back(modules_handle, &module, 1);
		if (ret != 0)
		{
			throw ref new Platform::FailureException("Failed add module to VECTOR.");
		}

		gatewayModulesToDelete.push_back(std::move(imod));
	}

	gateway_handle = Gateway_LL_UwpCreate(modules_handle, messagebus_handle);
	if (gateway_handle == nullptr)
	{
		throw ref new Platform::FailureException("Failed to create Gateway.");
	}
}

Gateway::~Gateway()
{
	Gateway_LL_UwpDestroy(gateway_handle);
	gateway_handle = nullptr;

	VECTOR_destroy(modules_handle);
}
