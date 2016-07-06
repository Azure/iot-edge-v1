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
		// The copy constructor and the IVector<byte> constructor cause an overload warning ... making the copy
		// constructor a static Create method to avoid this warning and the subsequent issue in Javascript.
		static Message^ CreateMessage(Message^ message);
		Message(Windows::Foundation::Collections::IVector<byte>^ msgInByteArray);

		Message(Platform::String ^content, Windows::Foundation::Collections::IMapView<Platform::String^, Platform::String^>^ properties);

		Windows::Foundation::Collections::IVector<byte>^ GetContent();
		Windows::Foundation::Collections::IMapView<Platform::String^, Platform::String^>^ GetProperties();

		Windows::Foundation::Collections::IVector<byte>^ ToBytes();

	private:
		MESSAGE_HANDLE _message_handle;
	};

}}}};

