// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pch.h"
#include "MessageUwp.h"

#include <locale>
#include <codecvt>

using namespace Microsoft::Azure::IoT::Gateway;


Message::Message(MESSAGE_HANDLE message_handle)
{
	_message_handle = message_handle;
}

Message^ Message::CreateMessage(Message^ message)
{
	return ref new Message(message->_message_handle);
}

Message::Message(Windows::Foundation::Collections::IVector<byte> ^msgInByteArray)
{
	std::vector<byte> bytes(msgInByteArray->Size);
	std::copy(begin(msgInByteArray), end(msgInByteArray), bytes.begin());

	_message_handle = Message_CreateFromByteArray(bytes.data(), bytes.size());
}

std::string wstrtostr(const std::wstring &wstr)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> wstringToString;
	return wstringToString.to_bytes(wstr);
}

std::wstring strtowstr(const std::string &str)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> stringToWstring;
	return stringToWstring.from_bytes(str.data());
}

Message::Message(Platform::String ^content, Windows::Foundation::Collections::IMapView<Platform::String^, Platform::String^>^ properties)
{
	std::wstring wcntnt = content->Data();
	std::string cntnt = wstrtostr(wcntnt);

	MESSAGE_CONFIG msg;
	msg.source = (unsigned char*)cntnt.c_str();
	msg.size = cntnt.size();
	msg.sourceProperties = Map_Create(NULL);
	
	for (auto iter = begin(properties); iter != end(properties); iter++)
	{
		std::wstring wkey = (*iter)->Key->Data();
		std::wstring wvalue = (*iter)->Value->ToString()->Data();
		std::string key = wstrtostr(wkey);
		std::string value = wstrtostr(wvalue);
		Map_Add(msg.sourceProperties, key.c_str(), value.c_str());
	}
	
	_message_handle = Message_Create(&msg);
}

Message^ Message::CreateMessage(Windows::Foundation::Collections::IVector<byte>^ content, Windows::Foundation::Collections::IMapView<Platform::String^, Platform::String^>^ properties)
{
	std::vector<byte> contentBytes(content->Size);
	std::copy(begin(content), end(content), contentBytes.begin());

	MESSAGE_CONFIG msg;
	msg.source = contentBytes.data();
	msg.size = contentBytes.size();
	msg.sourceProperties = Map_Create(NULL);

	for (auto iter = begin(properties); iter != end(properties); iter++)
	{
		std::string key = wstrtostr((*iter)->Key->Data());
		std::string value = wstrtostr((*iter)->Value->ToString()->Data());
		Map_Add(msg.sourceProperties, key.c_str(), value.c_str());
	}

	auto message_handle = Message_Create(&msg);
	return ref new Message(message_handle);
}

Windows::Foundation::Collections::IVector<byte>^ Message::ToBytes()
{
	int arrayLength = 0;
	auto byteArray = Message_ToByteArray(this->_message_handle, &arrayLength);

	Windows::Foundation::Collections::IVector<byte>^ messageBytes =
		ref new Platform::Collections::Vector<byte>(byteArray, arrayLength);
	return messageBytes;
}

Windows::Foundation::Collections::IVector<byte>^ Message::GetContent()
{
	auto content_buffer = Message_GetContent(_message_handle);

	Windows::Foundation::Collections::IVector<byte>^ content = 
		ref new Platform::Collections::Vector<byte>(content_buffer->buffer, content_buffer->size);
	return content;
}

Windows::Foundation::Collections::IMapView<Platform::String^, Platform::String^>^ Message::GetProperties()
{
	Windows::Foundation::Collections::IMap<Platform::String^, Platform::String^>^ properties = ref new
		Platform::Collections::Map<Platform::String^, Platform::String^>();

	const char* const* keys;
	const char* const* values;
	size_t nProperties;

	auto constmap_handle = Message_GetProperties(_message_handle);

	if (ConstMap_GetInternals(constmap_handle, &keys, &values, &nProperties) == CONSTMAP_OK)
	{
		for (size_t i = 0; i < nProperties; i++)
		{
			std::wstring key = strtowstr(keys[i]);
			std::wstring value = strtowstr(values[i]);

			properties->Insert(ref new Platform::String(key.c_str()), ref new Platform::String(value.c_str()));
		}
	}

	return properties->GetView();
}
