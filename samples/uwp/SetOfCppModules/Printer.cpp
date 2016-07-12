// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pch.h"
#include "Printer.h"

using namespace SetOfCppModules;
using namespace Platform;
using namespace concurrency;

Printer::Printer()
{
}

void Printer::Create(Microsoft::Azure::IoT::Gateway::MessageBus ^bus, Windows::Foundation::Collections::IMapView<Platform::String^, Platform::String^>^ configuration)
{
	std::wstring formattedText = L"SetOfCppModules::Printer::Create\r\n";
	OutputDebugString(formattedText.c_str());
}

void Printer::Destroy()
{
	std::wstring formattedText = L"SetOfCppModules::Printer::Destroy\r\n";
	OutputDebugString(formattedText.c_str());
}

void Printer::Receive(Microsoft::Azure::IoT::Gateway::Message ^received_message)
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

	std::wstring output = L"SetOfCppModules.Printer.Receive: Content=";
	output += content.c_str();
	output += L"\r\n";
	output += L"SetOfCppModules.Printer.Receive: Properties={";
	output += propsString.c_str();
	output += L"}\r\n";
	OutputDebugString(output.c_str());
}
