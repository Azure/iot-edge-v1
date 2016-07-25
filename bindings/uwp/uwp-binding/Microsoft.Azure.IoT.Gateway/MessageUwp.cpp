// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pch.h"
#include "MessageUwp.h"

#include <locale>
#include <codecvt>

using namespace Microsoft::Azure::IoT::Gateway;


Message::Message(MESSAGE_HANDLE message_handle)
{
	if (message_handle == nullptr)
	{
		throw ref new Platform::InvalidArgumentException("message_handle must be non-null.");
	}

	_message_handle = message_handle;
}

Message^ Message::CreateMessage(Message^ message)
{
	if (message == nullptr)
	{
		throw ref new Platform::InvalidArgumentException("message must be non-null.");
	}

	return ref new Message(message->_message_handle);
}

Message::Message(Windows::Foundation::Collections::IVector<byte> ^msgInByteArray)
{
	if (msgInByteArray == nullptr)
	{
		throw ref new Platform::InvalidArgumentException("msgInByteArray must be non-null.");
	}

	std::vector<byte> bytes(msgInByteArray->Size);
	std::copy(begin(msgInByteArray), end(msgInByteArray), bytes.begin());

	_message_handle = Message_CreateFromByteArray(bytes.data(), bytes.size());
	if (_message_handle == nullptr)
	{
		throw ref new Platform::FailureException("Failed to create message.");
	}
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
	if (properties == nullptr)
	{
		throw ref new Platform::InvalidArgumentException("properties must be non-null.");
	}

	// There is no distinction between an empty Platform::String and a NULL.
	std::string cntnt = "";
	if (content != nullptr)
	{
		cntnt = wstrtostr(content->Data());
	}

	MESSAGE_CONFIG msg;
	msg.source = (unsigned char*)cntnt.c_str();
	msg.size = cntnt.size();
	msg.sourceProperties = Map_Create(NULL);
	
	for (auto iter = begin(properties); iter != end(properties); iter++)
	{
		std::string key = wstrtostr((*iter)->Key->Data());
		std::string value = wstrtostr((*iter)->Value->ToString()->Data());
		Map_Add(msg.sourceProperties, key.c_str(), value.c_str());
	}
	
	_message_handle = Message_Create(&msg);
	if (_message_handle == nullptr)
	{
		throw ref new Platform::FailureException("Failed to create message.");
	}
}

Message^ Message::CreateMessage(Windows::Foundation::Collections::IVector<byte>^ content, Windows::Foundation::Collections::IMapView<Platform::String^, Platform::String^>^ properties)
{
	if (content == nullptr)
	{
		throw ref new Platform::InvalidArgumentException("content must be non-null.");
	}
	if (properties == nullptr)
	{
		throw ref new Platform::InvalidArgumentException("properties must be non-null.");
	}

	std::vector<byte> contentBytes(content->Size);
	std::copy(begin(content), end(content), contentBytes.begin());

	MESSAGE_CONFIG msg;
	msg.source = contentBytes.data();
	msg.size = contentBytes.size();
	msg.sourceProperties = Map_Create(NULL);
	if (msg.sourceProperties == nullptr)
	{
		throw ref new Platform::FailureException("Failed to create message's properties.");
	}

	for (auto iter = begin(properties); iter != end(properties); iter++)
	{
		std::string key = wstrtostr((*iter)->Key->Data());
		std::string value = wstrtostr((*iter)->Value->ToString()->Data());
		Map_Add(msg.sourceProperties, key.c_str(), value.c_str());
	}

	auto message_handle = Message_Create(&msg);
	if (message_handle == nullptr)
	{
		throw ref new Platform::FailureException("Failed to create message.");
	}
	return ref new Message(message_handle);
}

Windows::Foundation::Collections::IVector<byte>^ Message::ToBytes()
{
	int arrayLength = 0;
	auto byteArray = Message_ToByteArray(this->_message_handle, &arrayLength);
	if (byteArray == nullptr)
	{
		throw ref new Platform::FailureException("Failed to create byte array for message.");
	}

	Windows::Foundation::Collections::IVector<byte>^ messageBytes =
		ref new Platform::Collections::Vector<byte>(byteArray, arrayLength);
	return messageBytes;
}

Windows::Foundation::Collections::IVector<byte>^ Message::GetContent()
{
	auto content_buffer = Message_GetContent(_message_handle);
	if (content_buffer == nullptr)
	{
		throw ref new Platform::FailureException("Failed to get message content.");
	}

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
	if (constmap_handle == nullptr)
	{
		throw ref new Platform::FailureException("Failed to get message properties.");
	}

	if (ConstMap_GetInternals(constmap_handle, &keys, &values, &nProperties) == CONSTMAP_OK)
	{
		for (size_t i = 0; i < nProperties; i++)
		{
			std::wstring key = strtowstr(keys[i]);
			std::wstring value = strtowstr(values[i]);

			properties->Insert(ref new Platform::String(key.c_str()), ref new Platform::String(value.c_str()));
		}
	}
	else
	{
		throw ref new Platform::FailureException("Failed to get message property key/value pairs.");
	}

	return properties->GetView();
}
