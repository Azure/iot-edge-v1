// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pch.h"
#include "Sensor.h"

using namespace SetOfCppModules;
using namespace Platform;
using namespace concurrency;

Sensor::Sensor()
{
}

void Sensor::Create(Microsoft::Azure::IoT::Gateway::MessageBus ^bus, Windows::Foundation::Collections::IMapView<Platform::String^, Platform::String^>^ configuration)
{
	std::wstring formattedText = L"SetOfCppModules::Sensor::Create\r\n";
	OutputDebugString(formattedText.c_str());
	
	create_async([bus]() {
		wchar_t count[5];
		int i = 0;
		while (true)
		{
			Sleep(1000);
		
			Platform::String^ content = "SetOfCppModules::Sensor reading";
			auto properties = ref new Platform::Collections::Map<Platform::String^, Platform::String^>();

			i = (i + 1) % 500;
			swprintf(count, 20, L"%d", i++);
			properties->Insert("ReadingNumber", ref new Platform::String(count));

			auto msg = ref new Microsoft::Azure::IoT::Gateway::Message(content, properties->GetView());
			bus->Publish(msg);
		}
	});
}

void Sensor::Destroy()
{
	std::wstring formattedText = L"SetOfCppModules::Sensor::Destroy\r\n";
	OutputDebugString(formattedText.c_str());
}

void Sensor::Receive(Microsoft::Azure::IoT::Gateway::Message ^received_message)
{
	// Sensor does not care about messages
}
