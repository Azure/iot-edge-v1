// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pch.h"

#include "GatewayUwp.h"
#include "MessageBusUwp.h"
#include "MessageUwp.h"

using namespace Windows::Foundation::Collections;
using namespace Microsoft::Azure::IoT::Gateway;


ref class Message;

void InternalGatewayModule::Module_Create(MESSAGE_BUS_HANDLE busHandle, const void* configuration)
{
	_moduleImpl->Create(ref new MessageBus(busHandle), L"");
}
void InternalGatewayModule::Module_Destroy()
{
	_moduleImpl->Destroy();
}
void InternalGatewayModule::Module_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
	auto msg = ref new Microsoft::Azure::IoT::Gateway::Message(messageHandle);
	_moduleImpl->Receive(msg);
}

Gateway::Gateway(IVector<IGatewayModule^>^ modules)
{
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
		imod->Module_Create(messagebus_handle, nullptr);

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


