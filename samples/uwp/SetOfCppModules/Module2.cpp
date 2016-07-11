// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pch.h"
#include "Module2.h"

using namespace SetOfCppModules;
using namespace Platform;
using namespace concurrency;

Module2::Module2()
{
}

void SetOfCppModules::Module2::Create(Microsoft::Azure::IoT::Gateway::MessageBus ^bus, Windows::Foundation::Collections::IMapView<Platform::String^, Platform::String^>^ configuration)
{
	std::wstring formattedText = L"SetOfCppModules::Module2::Create\r\n";
	OutputDebugString(formattedText.c_str());

	create_async([bus]() {
		wchar_t count[3];
		for (int i = 0; i < 3; i++)
		{
			Sleep(1000);

			Platform::String^ content = "SetOfCppModules::Module2 says hello!!!";
			auto properties = ref new Platform::Collections::Map<Platform::String^, Platform::String^>();

			swprintf(count, 3, L"%d", i);
			properties->Insert("SetOfCppModules::Module2", ref new Platform::String(count));

			auto msg = ref new Microsoft::Azure::IoT::Gateway::Message(content, properties->GetView());
			bus->Publish(msg);
		}
	});
}

void SetOfCppModules::Module2::Destroy()
{
	std::wstring formattedText = L"SetOfCppModules::Module2::Destroy\r\n";
	OutputDebugString(formattedText.c_str());
}

void SetOfCppModules::Module2::Receive(Microsoft::Azure::IoT::Gateway::Message ^received_message)
{
	auto contentBytes = received_message->GetContent();
	std::wstring content;
	for (unsigned int i = 0; i < contentBytes->Size; i++)
	{
		content += (wchar_t)contentBytes->GetAt(i);
	}

	auto props = received_message->GetProperties();

	std::wstring propsString;
	for (auto iter = begin(props); iter != end(props); iter++)
	{
		propsString += (*iter)->Key->Data();
		propsString += L"=";
		propsString += (*iter)->Value->ToString()->Data();
		propsString += L", ";
	}

	std::wstring output = L"SetOfCppModules.Module2.Receive: Content=";
	output += content.c_str();
	output += L"\r\n";
	output += L"SetOfCppModules.Module2.Receive: Properties={";
	output += propsString.c_str();
	output += L"}\r\n";
	OutputDebugString(output.c_str());
}
