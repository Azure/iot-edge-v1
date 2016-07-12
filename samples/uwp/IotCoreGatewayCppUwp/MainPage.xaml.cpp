// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"

using namespace IotCoreGatewayCppUwp;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

MainPage::MainPage()
{
	InitializeComponent();

	CreateGateway();
}


void MainPage::CreateGateway()
{
	auto modules =
		ref new Platform::Collections::Vector<Microsoft::Azure::IoT::Gateway::IGatewayModule^>();
	modules->Append(ref new SetOfCsModules::Sensor());
	modules->Append(ref new SetOfCsModules::Printer());
	modules->Append(ref new SetOfCppModules::Sensor());
	modules->Append(ref new SetOfCppModules::Printer());

	auto properties = ref new Platform::Collections::Map<Platform::String^, Platform::String^>();
	properties->Insert("ConfigProperty", "ConfigValue");
	gateway = ref new Microsoft::Azure::IoT::Gateway::Gateway(modules, properties->GetView());
}
