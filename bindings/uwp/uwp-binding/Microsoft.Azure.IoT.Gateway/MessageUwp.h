// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include "pch.h"

#include "..\..\..\core\inc\message.h"


namespace Microsoft { namespace Azure { namespace IoT { namespace Gateway {

	public ref class Message sealed
	{
	internal:
		Message(MESSAGE_HANDLE message_handle);

		property MESSAGE_HANDLE MessageHandle
		{
			MESSAGE_HANDLE get() { return _message_handle; }
		};

	public:
		// There are limitations to UWP and overloading ... these static CreateMessage
		// methods are added to avoid this warning and the subsequent ambiguity issue 
		// in Javascript.
		static Message^ CreateMessage(Message^ message);
		static Message^ CreateMessage(Windows::Foundation::Collections::IVector<byte>^ content, Windows::Foundation::Collections::IMapView<Platform::String^, Platform::String^>^ properties);

		Message(Windows::Foundation::Collections::IVector<byte>^ msgInByteArray);
		Message(Platform::String ^content, Windows::Foundation::Collections::IMapView<Platform::String^, Platform::String^>^ properties);

		Windows::Foundation::Collections::IVector<byte>^ GetContent();
		Windows::Foundation::Collections::IMapView<Platform::String^, Platform::String^>^ GetProperties();

		Windows::Foundation::Collections::IVector<byte>^ ToBytes();

	private:
		MESSAGE_HANDLE _message_handle;
	};

}}}};

